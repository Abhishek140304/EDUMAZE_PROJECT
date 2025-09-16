#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include <iostream>
#include <filesystem>
#include "include/users.hpp"
#include "include/classroom.hpp"
#include "include/quiz.hpp"
#include <vector>
#include <string>
#include <any> 

using Session=crow::SessionMiddleware<crow::InMemoryStore>;

int main(){
    try{
    user_hashTable user_table;
    classroom_hashTable classroom_table;
    quiz_hashTable quiz_table;

    crow::mustache::set_base("templates");

    crow::App<crow::CookieParser,Session> app;

    app.get_middleware<Session>();

    CROW_ROUTE(app, "/")([&app](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");

        if (user_type.empty()) {
            crow::response res(303);
            res.add_header("Location", "/welcome_page");
            return res;
        }

        crow::response res(303);
        if (user_type == "student") {
            res.add_header("Location", "/student_dashboard");
        } else {
            res.add_header("Location", "/teacher_dashboard");
        }
        return res;
    });

    CROW_ROUTE(app, "/welcome_page")([](){
        auto page=crow::mustache::load("welcome.html");
        return page.render();
    });

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

        auto page=crow::mustache::load("student_dashboard.html");
        return crow::response(page.render(ctx));

    });

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

        auto page=crow::mustache::load("teacher_dashboard.html");
        return crow::response(page.render(ctx));

    });

    CROW_ROUTE(app, "/loginpage")([](){
        auto page=crow::mustache::load("login.html");
        return page.render();
    });

    CROW_ROUTE(app, "/login_post").methods("POST"_method)([&app,&user_table](const crow::request& req) -> crow::response {
        auto req_body = crow::query_string(("?" + req.body).c_str());

        crow::response res;
        std::string email=req_body.get("email");
        std::string pass=req_body.get("password");
        std::string role=req_body.get("role");
        std::string* user=user_table.findUsername(email);

        if(!user){
            res.code = 303;
            res.add_header("Location", "/error");
            return res;
        }

        bool login_success=false;

        std::string destination;
        if(user){
            if(role=="student"){
                student_data* data=user_table.findStudent(*user);
                if(data && data->password==pass){
                    login_success=true;
                    destination="/student_dashboard";
                }
            }
            else if(role=="teacher"){
                teacher_data* data=user_table.findTeacher(*user);
                if(data && data->password==pass){
                    login_success=true;
                    destination="/teacher_dashboard";
                }
            }
        }

        if(login_success){
            auto& session=app.get_context<Session>(req);
            session.set("username",*user);
            session.set("user_type",role);

            res.code=303;
            res.add_header("Location",destination);
        }
        else{
            res.code=303;
            res.add_header("Location","/error");
        }
        return res;
    });

    CROW_ROUTE(app, "/signup")([](){
        auto page=crow::mustache::load("signup.html");
        return page.render();
    });

    CROW_ROUTE(app, "/signup_post").methods("POST"_method)([&app,&user_table](const crow::request& req) -> crow::response {
        auto req_body = crow::query_string(("?" + req.body).c_str());

        crow::response res;
        std::string name=req_body.get("fullname");
        std::string username=req_body.get("username");
        std::string email=req_body.get("email");
        std::string pass=req_body.get("password");
        std::string role=req_body.get("role");

        auto& session=app.get_context<Session>(req);
        session.set("username",username);
        session.set("user_type",role);
        res.code=303;

        if(role=="student"){
            student_data* new_user=new student_data(name,username,email,pass);
            user_table.addStudent(new_user);
            res.add_header("Location","/student_dashboard");
        }
        else{
            teacher_data* new_user=new teacher_data(name,username,email,pass);
            user_table.addTeacher(new_user);
            res.add_header("Location","/teacher_dashboard");
        }
        return res;
    });

    CROW_ROUTE(app, "/logout")([&app](const crow::request& req, crow::response& res){
        auto& session=app.get_context<Session>(req);

        for (const auto& key : session.keys()) {
            session.remove(key);
        }

        res.add_header("Location","/welcome_page");
        res.code=303;
        res.end();
    });

    CROW_ROUTE(app,"/error")([](){
        auto page=crow::mustache::load("error.html");
        return crow::response(page.render());
    });

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

    CROW_ROUTE(app, "/create_quiz")([&app, &user_table, &classroom_table](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if(user_type!="teacher" || username.empty()){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        teacher_data* teacher = user_table.findTeacher(username);
        if(!teacher){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        std::vector<crow::json::wvalue> classrooms_for_template;

        for(const auto& class_code: teacher->classroomIds){
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

        auto page=crow::mustache::load("create_quiz.html");
        return crow::response(page.render(ctx));

    });

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

        for(int i=0; ; ++i){
            std::string q_text_key = "question_text_" + std::to_string(i);
            const char* question_text_val = body_params.get(q_text_key);
            if(!question_text_val) break;

            Question new_question;
            new_question.questionText = question_text_val;

            std::string correct_ans_key = "correct_answer_" + std::to_string(i);
            new_question.correctAnswerIndex = std::stoi(body_params.get(correct_ans_key));

            for (int j = 0; j < 4; ++j) {
                std::string option_key = "option_" + std::to_string(i) + "_" + std::to_string(j);
                new_question.options.push_back(body_params.get(option_key));
            }
            questions_list.push_back(new_question);
        }

        if (questions_list.empty()) {
            return crow::response(400, "A quiz must have at least one question.");
        }

        quiz_data* new_quiz = quiz_table.createQuiz(quiz_title, classroom_id, time_limit, questions_list);

        classroom_data* classroom = classroom_table.findClassroom(classroom_id);

        if (classroom) {
            classroom->quizIds.push_back(new_quiz->quizId);
        } else {
            return crow::response(500, "Could not find classroom.");
        }

        crow::response res(303);
        res.add_header("Location","/quiz_created");
        return res;

    });

    CROW_ROUTE(app,"/quiz_created")([&app](const crow::request& req){
        auto& session=app.get_context<Session>(req);
        if (session.get<std::string>("user_type") != "teacher") {
            return crow::response(303, "/error");
        }
        
        auto page=crow::mustache::load("quiz_created.html");
        return crow::response(page.render());
    });

    CROW_ROUTE(app, "/quiz_attempt")([&app](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");

        if(user_type!="student"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        auto page=crow::mustache::load("quiz_attempt.html");
        return crow::response(page.render());

    });

    CROW_ROUTE(app, "/leaderboard")([&app](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");

        if(user_type!="student"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        auto page=crow::mustache::load("leaderboard.html");
        return crow::response(page.render());

    });

    std::cout << "Server running at http://localhost:18080\n";

    app.port(18080).multithreaded().run();
    }catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
    }
}