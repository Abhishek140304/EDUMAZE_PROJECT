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

void registerClassroomRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table){
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
        classroom_table.saveClassroomsToFile();

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


    CROW_ROUTE(app, "/my_classrooms")
    ([&app, &user_table, &classroom_table](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if (user_type != "teacher" || username.empty()) {
            return crow::response(303, "/error");
        }

        teacher_data* teacher = user_table.findTeacher(username);
        if (!teacher) {
            return crow::response(303, "/error");
        }

        crow::mustache::context ctx;
        ctx["teacher_username"] = teacher->username;

        std::vector<crow::json::wvalue> classrooms_list;

        for (const auto& class_code : teacher->classroomIds) {
            classroom_data* room = classroom_table.findClassroom(class_code);
            if (room) {
                crow::json::wvalue classroom_obj;
                classroom_obj["class_name"] = room->class_name;
                classroom_obj["subject"] = room->subject;
                classroom_obj["class_code"] = room->class_code;
                classrooms_list.push_back(std::move(classroom_obj));
            }
        }

        if (!classrooms_list.empty()) {
            ctx["classrooms"] = std::move(classrooms_list);
        }

        auto page = crow::mustache::load("my_classrooms.html");
        return crow::response(page.render(ctx));
    });



    CROW_ROUTE(app, "/classroom/<string>")
    ([&app, &user_table, &classroom_table, &quiz_table](const crow::request& req, const std::string& class_code) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        if (user_type != "teacher") {
            CROW_LOG_INFO << "Error: User is not a teacher.";
            return crow::response(303, "/error");
        }

        classroom_data* room = classroom_table.findClassroom(class_code);
        if (!room) {
            return crow::response(404, "/error");
        }

        crow::mustache::context ctx;
        ctx["class_name"] = room->class_name;
        ctx["subject"] = room->subject;
        ctx["class_code"] = room->class_code;

        std::vector<crow::json::wvalue> students_list;
        for (const auto& student_username : room->student_usernames) {
            student_data* student = user_table.findStudent(student_username);
            if (student) {
                crow::json::wvalue student_obj;
                student_obj["name"] = student->name;
                student_obj["username"] = student->username;
                students_list.push_back(std::move(student_obj));
            }
        }
        if (!students_list.empty()) {
            ctx["students"] = std::move(students_list);
        }

        std::vector<crow::json::wvalue> quizzes_list;
        for (const auto& quiz_id : room->quizIds) {
            quiz_data* quiz = quiz_table.findQuiz(quiz_id);
            if (quiz) {
                crow::json::wvalue quiz_obj;
                quiz_obj["quizTitle"] = quiz->quizTitle;
                quiz_obj["quizId"] = quiz->quizId;
                quizzes_list.push_back(std::move(quiz_obj));
            }
        }
        if (!quizzes_list.empty()) {
            ctx["quizzes"] = std::move(quizzes_list);
        }

        auto page = crow::mustache::load("classroom_details.html");
        return crow::response(page.render(ctx));
    });

}