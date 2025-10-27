/*
 * Description: Implements JSON serialization for quiz and question data.
 * Registers all Crow web routes related to quiz creation and management
 */

#include "Common_Route.hpp"

using njson=nlohmann::json;

/*
 * `to_json` overload for `Question`.
 * Used when serializing a `quiz_data` object, which contains a vector of Questions.
 */
void to_json(njson& j, const Question& q) {
    j=njson{
        {"questionText", q.questionText},
        {"options", q.options},
        {"correctAnswerIndex", q.correctAnswerIndex}
    };
}

/*
 * `from_json` overload for `Question`.
 * Used when deserializing a `quiz_data` object from a JSON file.
 */
void from_json(const njson& j, Question& q) {
    q.questionText = j.value("questionText", "");
    q.options = j.value("options", std::vector<std::string>{});
    q.correctAnswerIndex = j.value("correctAnswerIndex", 0);
}

/*
 * `to_json` overload for `quiz_data`.
 * Called by nlohmann::json when serializing a quiz (e.g., saving to file).
 */
void to_json(njson& j, const quiz_data& q) {
    j=njson{
        {"quizId", q.quizId},
        {"quizTitle", q.quizTitle},
        {"classroomId", q.classroomId},
        {"timeLimitMinutes", q.timeLimitMins},
        {"questions", q.questions}
    };
}

/*
 * `from_json` overload for `quiz_data`.
 * Called by nlohmann::json when deserializing a quiz (e.g., loading from file).
 */
void from_json(const njson& j, quiz_data& q) {
    q.quizId = j.value("quizId", "");
    q.quizTitle = j.value("quizTitle", "");
    q.classroomId = j.value("classroomId", "");
    q.timeLimitMins = j.value("timeLimitMinutes", 0);
    q.questions = j.value("questions", std::vector<Question>{});
}

/*
 * Registers all routes related to quiz creation.
 */
void registerQuizRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table){

    /*
     * Route: /create_quiz (GET)
     * Description: Displays the HTML form for a teacher to create a new quiz.
     * It fetches all the teacher's classrooms to populate a dropdown menu.
     */

    CROW_ROUTE(app, "/create_quiz")([&app, &user_table, &classroom_table](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        // Security Check
        if(user_type!="teacher" || username.empty()){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // O(1) average-case lookup
        teacher_data* teacher = user_table.findTeacher(username);
        if(!teacher){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        std::vector<crow::json::wvalue> classrooms_for_template;

        // Iterate through the teacher's classrooms to list them in the form
        for(const auto& class_code: teacher->classroomIds){
            // O(1) average-case lookup for each classroom
            classroom_data* room = classroom_table.findClassroom(class_code);

            if(room){
                crow::json::wvalue classroom_obj = crow::json::wvalue::object();
                classroom_obj["class_code"] = room->class_code;
                classroom_obj["class_name"] = room->class_name;
                classroom_obj["subject"] = room->subject;
                classrooms_for_template.push_back(std::move(classroom_obj));
            }
        }

        ctx["classrooms"] = std::move(classrooms_for_template);

        auto page=crow::mustache::load("teacher/create_quiz.html");
        return crow::response(page.render(ctx));

    });

    /*
     * Route: /create_quiz_post (POST)
     * Description: Handles the submission of the new quiz form.
     * It parses the form data, creates a new quiz object, and saves it.
     */
    CROW_ROUTE(app, "/create_quiz_post").methods("POST"_method)([&app, &classroom_table, &quiz_table](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        if(session.get<std::string>("user_type")!="teacher"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        auto body_params = crow::query_string(("?" + req.body).c_str());

        std::string quiz_title = body_params.get("quiz_title");
        std::string classroom_id = body_params.get("classroom_id");
        int time_limit = 0;
        if(body_params.get("time_limit")) {
            time_limit = std::stoi(body_params.get("time_limit"));
        }

        if (quiz_title.empty() || classroom_id.empty() || time_limit <= 0) {
            return crow::response(400, "Invalid form data.");
        }

        std::vector<Question> questions_list;

        // Loop dynamically to parse questions. The loop breaks when
        // "question_text_i" is not found in the form data.
        for(int i=0; ; ++i){
            std::string q_text_key = "question_text_" + std::to_string(i);
            const char* question_text_val = body_params.get(q_text_key);
            if(!question_text_val) break;   // No more questions

            Question new_question;
            new_question.questionText = question_text_val;

            std::string correct_ans_key = "correct_answer_" + std::to_string(i);
            new_question.correctAnswerIndex = std::stoi(body_params.get(correct_ans_key));

            // Parse the 4 options for this question
            for (int j = 0; j < 4; ++j) {
                std::string option_key = "option_" + std::to_string(i) + "_" + std::to_string(j);
                new_question.options.push_back(body_params.get(option_key));
            }
            questions_list.push_back(new_question);
        }

        if (questions_list.empty()) {
            return crow::response(400, "A quiz must have at least one question.");
        }

        // O(1) average-case insertion into the quiz hash table
        quiz_data* new_quiz = quiz_table.createQuiz(quiz_title, classroom_id, time_limit, questions_list);

        // O(1) average-case lookup to find the classroom
        classroom_data* classroom = classroom_table.findClassroom(classroom_id);

        if (classroom) {
            // Link the quiz to the classroom
            classroom->quizIds.push_back(new_quiz->quizId);
        } else {
            // This should ideally not happen if the form is correct
            return crow::response(500, "Could not find classroom.");
        }

        // Persist the changes to disk
        quiz_table.saveQuizzesToFile();
        classroom_table.saveClassroomsToFile();

        crow::response res(303);
        res.add_header("Location","/quiz_created");
        return res;

    });

    /*
     * Route: /quiz_created
     * Description: A simple success page shown after a quiz is created.
     */
    CROW_ROUTE(app,"/quiz_created")([&app](const crow::request& req){
        auto& session=app.get_context<Session>(req);
        if (session.get<std::string>("user_type") != "teacher") {
            return crow::response(303, "/error");
        }
        
        auto page=crow::mustache::load("teacher/quiz_created.html");
        return crow::response(page.render());
    });
}