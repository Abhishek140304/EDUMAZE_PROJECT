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

void registerStudentsRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table){
    CROW_ROUTE(app, "/student_dashboard")([&app,&user_table, &classroom_table](const crow::request& req)->crow::response {
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

        std::vector<crow::json::wvalue> classrooms_list;

        for(const auto& class_code: data->classroomIds){
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
}