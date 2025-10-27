/*
 * Description: Implements the JSON serialization for teacher_data and registers all Crow web routes specific to teachers, such as the dashboard and viewing leaderboards.
 */

#include "Common_Route.hpp"

using njson = nlohmann::json;

/*
 * `to_json` overload for `teacher_data`.
 * This function is automatically called by the nlohmann::json library whenever a `teacher_data` object is serialized to JSON (e.g., when saving to a file).
 */
void to_json(njson &j, const teacher_data &s){
    j=njson{
        {"name", s.name},
        {"email", s.email},
        {"password", s.password},
        {"username", s.username},
        {"classroomIds", s.classroomIds}
    };
}

/*
 * Registers all routes that are primarily accessed by a teacher.
 * This function is called once from main.cpp to set up the web server.
 */
void registerTeachersRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table){

    /*
     * Route: /teacher_dashboard
     * Description: Displays the main dashboard for the teacher after login.
     * It fetches the teacher's data to display their name.
     */

    CROW_ROUTE(app, "/teacher_dashboard")([&app,&user_table](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        // Security Check: Ensure user is a logged-in teacher
        if (user_type != "teacher" || username.empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // O(1) average-case hash table lookup
        teacher_data* data = user_table.findTeacher(username);
        if (!data) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // Pass data to the HTML template
        crow::mustache::context ctx;
        ctx["teacher_name"] = data->name;
        ctx["teacher_username"] = data->username;

        auto page=crow::mustache::load("teacher/teacher_dashboard.html");
        return crow::response(page.render(ctx));

    });

    /*
     * Route: /leaderboard
     * Description: Shows the teacher a list of all classrooms they own and the quizzes within those classrooms, with links to each quiz's specific leaderboard.
     */
    CROW_ROUTE(app, "/leaderboard")([&app, &user_table, &classroom_table, &quiz_table](const crow::request& req) -> crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        // Security Check: Ensure user is a logged-in teacher
        if (user_type != "teacher" || username.empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // O(1) average-case hash table lookup
        teacher_data* teacher = user_table.findTeacher(username);
        if (!teacher) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["teacher_username"] = teacher->username;

        std::vector<crow::json::wvalue> classrooms_list;

        // Iterate through the vector of class codes stored in the teacher's data
        for (const auto& class_code : teacher->classroomIds) {
            // O(1) average-case hash table lookup for each classroom
            classroom_data* room = classroom_table.findClassroom(class_code);
            if (!room) continue;

            crow::json::wvalue classroom_obj;
            classroom_obj["class_name"] = room->class_name;
            
            std::vector<crow::json::wvalue> quizzes_list;
            
            // Iterate through the vector of quiz IDs stored in the classroom's data
            for (const auto& quiz_id : room->quizIds) {
                // O(1) average-case hash table lookup for each quiz
                quiz_data* quiz = quiz_table.findQuiz(quiz_id);
                if (quiz) {
                    crow::json::wvalue quiz_obj;
                    quiz_obj["quizTitle"] = quiz->quizTitle;
                    // Provide a direct link to the leaderboard for this specific quiz
                    quiz_obj["quizLink"] = "/quiz_leaderboard/" + quiz->quizId; 
                    quizzes_list.push_back(std::move(quiz_obj));
                }
            }

            // Only add the classroom if it actually has quizzes
            if (!quizzes_list.empty()) {
                classroom_obj["quizzes"] = std::move(quizzes_list);
                classrooms_list.push_back(std::move(classroom_obj));
            }
        }

        if (!classrooms_list.empty()) {
            ctx["classrooms_with_quizzes"] = std::move(classrooms_list);
        }

        auto page = crow::mustache::load("teacher/all_leaderboards.html");
        return crow::response(page.render(ctx));
    });
}