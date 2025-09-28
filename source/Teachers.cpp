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

void registerTeachersRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table){
    CROW_ROUTE(app, "/teacher_dashboard")([&app,&user_table](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");

        if(user_type!="teacher"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        std::string username=session.get<std::string>("username");
        teacher_data* data=user_table.findTeacher(username);
        crow::mustache::context ctx;
        ctx["teacher_name"] = data->name;
        ctx["teacher_username"] = data->username;

        auto page=crow::mustache::load("teacher/teacher_dashboard.html");
        return crow::response(page.render(ctx));

    });
}