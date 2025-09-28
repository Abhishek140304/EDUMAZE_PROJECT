#include "Common_Route.hpp"

using njson = nlohmann::json;

void to_json(njson &j, const student_data &s){
    j=njson{
        {"name", s.name},
        {"email", s.email},
        {"password", s.password},
        {"username", s.username},
        {"classroomIds", s.classroomIds}
    };
}

void registerStudentsRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table){
    CROW_ROUTE(app, "/student_dashboard")([&app,&user_table](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");

        if(user_type!="student"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        std::string username=session.get<std::string>("username");
        student_data* data=user_table.findStudent(username);
        crow::mustache::context ctx;
        ctx["student_name"] = data->name;
        ctx["student_username"] = data->username;

        auto page=crow::mustache::load("student/student_dashboard.html");
        return crow::response(page.render(ctx));

    });
}