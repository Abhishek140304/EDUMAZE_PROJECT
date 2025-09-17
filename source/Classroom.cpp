#include "Common_Route.hpp"

using njson = nlohmann::json;

void to_json(njson& j, const classroom_data& c) {
    j = njson{
        {"class_name", c.class_name},
        {"subject", c.subject},
        {"class_code", c.class_code},
        {"teacher_username", c.teacher_username},
        {"student_usernames", c.student_usernames},
        {"quizIds", c.quizIds}
    };
}

void registerClassroomRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table){
    CROW_ROUTE(app, "/create_classroom")([&app](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");

        if(user_type!="teacher"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["teacher_username"]=session.get<std::string>("username");

        auto page=crow::mustache::load("create_classroom.html");
        return crow::response(page.render(ctx));

    });

    CROW_ROUTE(app,"/create_classroom_post").methods("POST"_method)([&app,&user_table, &classroom_table](const crow::request& req) -> crow::response {
        auto req_body = crow::query_string(("?" + req.body).c_str());

        auto& session=app.get_context<Session>(req);

        std::string user_type=session.get<std::string>("user_type");
        std::string username=session.get<std::string>("username");

        if(user_type!="teacher" || username.empty()){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        std::string classname=req_body.get("classname");
        std::string subject=req_body.get("subject");

        teacher_data* teacher=user_table.findTeacher(username);
        if(!teacher){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        std::string new_class_code=classroom_table.addClassroom(classname,subject,teacher);
        teacher->classroomIds.push_back(new_class_code);

        user_table.saveTeachersToFile();

        crow::response res(303);
        res.add_header("Location","/classroom_created?code="+new_class_code);
        return res;

    });

    CROW_ROUTE(app,"/classroom_created")([](const crow::request& req){
        const char* code = req.url_params.get("code");

        if (!code || std::string(code).empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["class_code"]=req.url_params.get("code");
        
        auto page=crow::mustache::load("classroom_created.html");
        return crow::response(page.render(ctx));
    });
}