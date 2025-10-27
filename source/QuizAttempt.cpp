/*
 * Description: Registers all Crow routes related to the process of a student *taking* a quiz. This includes displaying the quiz questions, submitting answers, and calculating results. It also defines the shared leaderboard route.
 *
 * DSA Concepts:
 * 1.  **Priority Queue:** A `std::priority_queue` with a custom comparator (`ResultComparator`) is used to efficiently sort the leaderboard by score (descending) and time (ascending).
 * 2.  **Hash Table (O(N) Scan):** Functions like `hasStudentAttempted` and `findResultsForQuiz` require a full scan of the `results_table`, making them O(N) operations (where N is total attempts in the system).
 * This is a key design trade-off.
 */

#include "Common_Route.hpp"
#include <queue>
#include <map>
#include <chrono>

using Session=crow::SessionMiddleware<crow::InMemoryStore>;

/*
 * DSA: Custom Comparator for the Priority Queue.
 *
 * This struct defines the sorting logic for the leaderboard.
 * A priority_queue is a max-heap by default, so:
 * 1. `a->score < b->score`: Makes higher scores have higher priority (come out first).
 * 2. `a->timeTakenSeconds > b->timeTakenSeconds`: Makes *lower* times have higher priority.
 *
 * Result: The queue will always pop the entry with the highest score, and in case of a tie, the lowest time.
 */
struct ResultComparator {
    bool operator()(const quiz_result_data* a, const quiz_result_data* b) {
        if (a->score != b->score) {
            return a->score < b->score; // Higher score has higher priority
        }
        return a->timeTakenSeconds > b->timeTakenSeconds; // Lower time has higher priority
    }
};

// Helper function to decode URL-encoded strings (e.g., "Hello+World" -> "Hello World")
std::string urlDecode(const std::string& str) {
    std::string decoded;
    char ch;
    int i, ii;
    for (i = 0; i < str.length(); i++) {
        if (int(str[i]) == 37) { // '%'
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            decoded += ch;
            i = i + 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}


/*
 * Helper function to parse answers from the raw POST body.
 * The body looks like "answer_0=1&answer_1=3&quizId=XYZ".
 * This function finds all "answer_X=Y" pairs.
 *
 * DSA: Uses `std::map` to store answers. This provides O(log k) insertion and lookup, where k is the number of questions.
 */
std::map<int, int> parseQuizAnswers(const std::string& body) {
    std::map<int, int> answers;
    std::stringstream ss(body);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = urlDecode(pair.substr(0, eq_pos));
        std::string value = urlDecode(pair.substr(eq_pos + 1));

        // Check if key is an answer, e.g., "answer_0", "answer_1"
        if (key.rfind("answer_", 0) == 0) {
            try {
                int q_index = std::stoi(key.substr(7)); // Get number after "answer_"
                int o_index = std::stoi(value);
                answers[q_index] = o_index; // Store in map
            } catch (const std::exception& e) {
                CROW_LOG_ERROR << "Failed to parse answer: " << e.what();
            }
        }
    }
    return answers;
}

/*
 * Registers all routes related to a student taking a quiz.
 */
void registerQuizAttemptRoutes(
    crow::App<crow::CookieParser,Session>& app, 
    quiz_hashTable& quiz_table,
    quiz_result_hashTable& results_table
) {

    /*
     * Route: /student/attempt_quiz/<string> (GET)
     * Description: (Student) Displays the quiz page with questions and options.
     * This is where the student actively takes the quiz.
     */
    CROW_ROUTE(app, "/student/attempt_quiz/<string>")
    ([&app, &quiz_table, &results_table](const crow::request& req, const std::string& quiz_id) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if (user_type != "student" || username.empty()) {
            return crow::response(303, "/error");
        }

        // O(N) Hash Table Scan
        // Check if the student has already taken this quiz.
        if (results_table.hasStudentAttempted(username, quiz_id)) {
            // Redirect to leaderboard if already taken
            crow::response res(303);
            res.add_header("Location", "/quiz_leaderboard/" + quiz_id + "?error=attempted");
            return res;
        }

        // O(1) average-case lookup
        quiz_data* quiz = quiz_table.findQuiz(quiz_id);
        if (!quiz) {
            return crow::response(404, "/error");
        }

        crow::mustache::context ctx;
        ctx["quizTitle"] = quiz->quizTitle;
        ctx["quizId"] = quiz->quizId;
        ctx["timeLimitMins"] = quiz->timeLimitMins;

        // Capture the start time (in seconds) to calculate total time taken
        auto startTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        ctx["startTime"] = std::to_string(startTime);

        // Prepare quiz questions for the HTML template
        std::vector<crow::json::wvalue> questions_list;
        int q_index = 0;
        for (const auto& q : quiz->questions) {
            crow::json::wvalue question_obj;
            question_obj["questionText"] = q.questionText;
            question_obj["questionIndex"] = q_index;

            std::vector<crow::json::wvalue> options_list;
            int o_index = 0;
            for (const auto& opt : q.options) {
                crow::json::wvalue option_obj;
                option_obj["optionText"] = opt;
                option_obj["optionIndex"] = o_index;
                options_list.push_back(std::move(option_obj));
                o_index++;
            }
            question_obj["options"] = std::move(options_list);
            questions_list.push_back(std::move(question_obj));
            q_index++;
        }
        ctx["questions"] = std::move(questions_list);

        auto page = crow::mustache::load("student/attempt_quiz.html");
        return crow::response(page.render(ctx));
    });

    /*
     * Route: /student/submit_quiz (POST)
     * Description: (Student) Handles the submission of the quiz answers.
     * It calculates the score, saves the result, and redirects to the leaderboard.
     */
    CROW_ROUTE(app, "/student/submit_quiz").methods("POST"_method)
    ([&app, &quiz_table, &results_table](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if (user_type != "student" || username.empty()) {
            return crow::response(403, "/error");
        }
        
        auto body_params = crow::query_string(("?" + req.body).c_str());
        std::string quiz_id = body_params.get("quizId");
        long long startTime = 0;
        if(body_params.get("startTime")) {
            try {
                startTime = std::stoll(body_params.get("startTime"));
            } catch (...) { /* ignore invalid start time */ }
        }

        if (quiz_id.empty() || startTime == 0) {
            return crow::response(400, "Invalid submission.");
        }
        
        // Calculate time taken
        auto endTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        double timeTaken = static_cast<double>(endTime - startTime);

        // O(1) average-case lookup
        quiz_data* quiz = quiz_table.findQuiz(quiz_id);
        if (!quiz) {
            return crow::response(404, "Quiz not found.");
        }
        
        // Check for re-submission just in case
        if (results_table.hasStudentAttempted(username, quiz_id)) {
             return crow::response(303, "/quiz_leaderboard/" + quiz_id);
        }

        // --- Calculate Score ---
        std::map<int, int> submitted_answers_map = parseQuizAnswers(req.body);
        std::vector<int> submitted_answers_vec(quiz->questions.size(), -1); // -1 for unanswered
        int score = 0;

        for (int i = 0; i < quiz->questions.size(); ++i) {
            // Check if student answered question 'i'
            if (submitted_answers_map.count(i)) {
                int chosen_option = submitted_answers_map[i];
                submitted_answers_vec[i] = chosen_option;   // Store their answer
                // Check if the answer was correct
                if (chosen_option == quiz->questions[i].correctAnswerIndex) {
                    score++;
                }
            }
        }

        // Save the result
        results_table.addResult(quiz_id, username, score, timeTaken, submitted_answers_vec);
        results_table.saveResultsToFile(); 

        // Redirect to leaderboard
        crow::response res(303);
        res.add_header("Location", "/quiz_leaderboard/" + quiz_id);
        return res;
    });

    /*
     * Route: /quiz_leaderboard/<string> (GET)
     * Description: (Common) Displays the leaderboard for a specific quiz.
     * Accessible by both students and teachers.
     *
     * This is a great example of using a Priority Queue.
     */
    CROW_ROUTE(app, "/quiz_leaderboard/<string>")
    ([&app, &quiz_table, &results_table](const crow::request& req, const std::string& quiz_id) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        
        if (user_type != "student" && user_type != "teacher") {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }
        
        // O(1) average-case lookup
        quiz_data* quiz = quiz_table.findQuiz(quiz_id);
        if (!quiz) {
            return crow::response(404, "Quiz not found.");
        }

        // O(N) Hash Table Scan
        // Get all results for this specific quiz.
        std::vector<quiz_result_data*> results = results_table.findResultsForQuiz(quiz_id);

        // --- Use Priority Queue for Sorting ---
        // Time: O(k log k), where k is the number of results for *this* quiz.
        
        // 1. Initialize the priority queue with our custom comparator
        std::priority_queue<quiz_result_data*, std::vector<quiz_result_data*>, ResultComparator> leaderboard_pq;

        // 2. Push all results into the queue (O(k log k))
        for (quiz_result_data* res : results) {
            leaderboard_pq.push(res);
        }

        crow::mustache::context ctx;
        ctx["quizTitle"] = quiz->quizTitle;
        ctx["totalQuestions"] = quiz->questions.size();

        // Check for error message (e.g., if already attempted)
        if (req.url_params.get("error") && std::string(req.url_params.get("error")) == "attempted") {
            ctx["error_message"] = "You have already attempted this quiz. Here are the results:";
        }

        std::vector<crow::json::wvalue> sorted_results_list;
        int rank = 1;

        // 3. Pop from the queue to get the sorted list (O(k log k))
        // The highest-priority item (top score, lowest time) comes out first.
        while (!leaderboard_pq.empty()) {
            quiz_result_data* top_result = leaderboard_pq.top();
            leaderboard_pq.pop();

            crow::json::wvalue res_obj;
            res_obj["rank"] = rank++;
            res_obj["studentUsername"] = top_result->studentUsername;
            res_obj["score"] = top_result->score;
            
            // Format timeTaken to M:SS
            int total_seconds = static_cast<int>(top_result->timeTakenSeconds);
            int minutes = total_seconds / 60;
            int seconds = total_seconds % 60;
            std::stringstream time_ss;
            time_ss << minutes << ":" << (seconds < 10 ? "0" : "") << seconds;
            res_obj["timeTaken"] = time_ss.str();
            
            sorted_results_list.push_back(std::move(res_obj));
        }

        if (!sorted_results_list.empty()) {
            ctx["leaderboard"] = std::move(sorted_results_list);
        }

        auto page = crow::mustache::load("common/leaderboard.html");
        return crow::response(page.render(ctx));
    });
}