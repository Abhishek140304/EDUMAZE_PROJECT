/*
 * Description: Implements JSON serialization for student_data and registers all Crow web routes specific to students, such as their dashboard and viewing their leaderboards.
 */

#include "Common_Route.hpp"

using njson = nlohmann::json;

/*
 * `to_json` overload for `student_data`.
 * Called by nlohmann::json when serializing a student (e.g., saving to file).
 */
void to_json(njson &j, const student_data &s){
    j=njson{
        {"name", s.name},
        {"email", s.email},
        {"password", s.password},
        {"username", s.username},
        {"classroomIds", s.classroomIds}
    };
}

/*
 * Registers all routes that are primarily accessed by a student.
 */
void registerStudentsRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table){

    /*
     * Route: /student_dashboard
     * Description: Displays the main dashboard for the student after login.
     * It fetches the student's data and lists all classrooms they are in.
     */
    CROW_ROUTE(app, "/student_dashboard")([&app,&user_table, &classroom_table](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        // Security Check
        if (user_type != "student" || username.empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // O(1) average-case hash table lookup
        student_data* data = user_table.findStudent(username);
        if (!data) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }
        crow::mustache::context ctx;
        ctx["student_name"] = data->name;
        ctx["student_username"] = data->username;

        std::vector<crow::json::wvalue> classrooms_list;

        // Iterate through the vector of class codes stored in the student's data
        for(const auto& class_code: data->classroomIds){
            // O(1) average-case hash table lookup for each classroom
            classroom_data* room=classroom_table.findClassroom(class_code);
            if(room){
                crow::json::wvalue classroom_obj;
                classroom_obj["class_name"]=room->class_name;
                classroom_obj["subject"]=room->subject;
                classroom_obj["class_code"]=room->class_code;
                classrooms_list.push_back(std::move(classroom_obj));
            }
        }

        if(!classrooms_list.empty()){
            ctx["classrooms"]=std::move(classrooms_list);
        }

        auto page=crow::mustache::load("student/student_dashboard.html");
        return crow::response(page.render(ctx));

    });

    /*
     * Route: /my_leaderboards
     * Description: (Student) Shows the student a list of all classrooms they are in and the quizzes within them, with links to each leaderboard. (Functionally similar to the teacher's /leaderboard route).
     */
    CROW_ROUTE(app, "/my_leaderboards")
    ([&app, &user_table, &classroom_table, &quiz_table](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if (user_type != "student" || username.empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // O(1) average-case lookup
        student_data* student = user_table.findStudent(username);
        if (!student) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["student_username"] = student->username;
        
        std::vector<crow::json::wvalue> classrooms_list;

        // Loop through all classrooms the student is in
        for (const auto& class_code : student->classroomIds) {
            // O(1) average-case lookup
            classroom_data* room = classroom_table.findClassroom(class_code);
            if (!room) continue;

            crow::json::wvalue classroom_obj;
            classroom_obj["class_name"] = room->class_name;
            
            std::vector<crow::json::wvalue> quizzes_list;
            
            // Loop through all quizzes in that classroom
            for (const auto& quiz_id : room->quizIds) {
                // O(1) average-case lookup
                quiz_data* quiz = quiz_table.findQuiz(quiz_id);
                if (quiz) {
                    crow::json::wvalue quiz_obj;
                    quiz_obj["quizTitle"] = quiz->quizTitle;
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

        auto page = crow::mustache::load("student/my_leaderboards.html");
        return crow::response(page.render(ctx));
    });
}