#include "Common_Route.hpp"

using njson = nlohmann::json;

void to_json(njson &j, const teacher_data &s){
    j=njson{
        {"name", s.name},
        {"email", s.email},
        {"password", s.password},
        {"username", s.username},
        {"classroomIds", s.classroomIds}
    };
}

void registerTeachersRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table){
    CROW_ROUTE(app, "/teacher_dashboard")([&app,&user_table](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if (user_type != "teacher" || username.empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        teacher_data* data = user_table.findTeacher(username);
        if (!data) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["teacher_name"] = data->name;
        ctx["teacher_username"] = data->username;

        auto page=crow::mustache::load("teacher/teacher_dashboard.html");
        return crow::response(page.render(ctx));

    });

    CROW_ROUTE(app, "/leaderboard")([&app, &user_table, &classroom_table, &quiz_table](const crow::request& req) -> crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if (user_type != "teacher" || username.empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        teacher_data* teacher = user_table.findTeacher(username);
        if (!teacher) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["teacher_username"] = teacher->username;

        std::vector<crow::json::wvalue> classrooms_list;

        // Loop through all classrooms the teacher owns
        for (const auto& class_code : teacher->classroomIds) {
            classroom_data* room = classroom_table.findClassroom(class_code);
            if (!room) continue;

            crow::json::wvalue classroom_obj;
            classroom_obj["class_name"] = room->class_name;
            
            std::vector<crow::json::wvalue> quizzes_list;
            
            // Loop through all quizzes in that classroom
            for (const auto& quiz_id : room->quizIds) {
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

        auto page = crow::mustache::load("teacher/all_leaderboards.html");
        return crow::response(page.render(ctx));
    });
}