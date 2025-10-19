#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include <iostream>
#include <filesystem>
#include "include/Common_Route.hpp"
#include <vector>
#include <string>
#include <any> 

using Session=crow::SessionMiddleware<crow::InMemoryStore>;

int main(){
    try{
    user_hashTable user_table;
    classroom_hashTable classroom_table;
    quiz_hashTable quiz_table;
    quiz_result_hashTable result_table;


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
            res.add_header("Location", "student_dashboard");
        } else {
            res.add_header("Location", "teacher_dashboard");
        }
        return res;
    });

    CROW_ROUTE(app, "/welcome_page")([](){
        auto page=crow::mustache::load("welcome.html");
        return page.render();
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
                    destination="student_dashboard";
                }
            }
            else if(role=="teacher"){
                teacher_data* data=user_table.findTeacher(*user);
                if(data && data->password==pass){
                    login_success=true;
                    destination="teacher_dashboard";
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

    registerStudentsRoutes(app,user_table, classroom_table);

    registerTeachersRoutes(app,user_table);

    registerClassroomRoutes(app,user_table, classroom_table, quiz_table);

    registerQuizRoutes(app,user_table, classroom_table, quiz_table);

    registerQuizAttemptRoutes(app, quiz_table, result_table);

    std::cout << "Server running at http://localhost:18080\n";

    app.port(18080).multithreaded().bindaddr("0.0.0.0").run();

    }catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
    }
}