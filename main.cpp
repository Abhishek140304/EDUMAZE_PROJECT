#include "crow.h"
#include <iostream>
#include <filesystem> 
#include "include/load.hpp"

int main(){
    try{
    hashTables table;

    crow::mustache::set_base("templates");

    crow::SimpleApp app;
    CROW_ROUTE(app, "/")([](){
        auto page=crow::mustache::load("welcome.html");
        return page.render();
    });

    CROW_ROUTE(app, "/loginpage")([](){
        auto page=crow::mustache::load("login.html");
        return page.render();
    });

    CROW_ROUTE(app, "/login_post").methods("POST"_method)([&table](const crow::request& req) -> crow::response {
        auto req_body = crow::query_string(("?" + req.body).c_str());

        std::string email=req_body.get("email");
        std::string pass=req_body.get("password");
        std::string role=req_body.get("role");
        std::string* user=table.findUsername(email);

        if(user){
            if(role=="student"){
                student_data* data=table.findStudent(*user);
                if(data){
                    if(data->password==pass){
                        auto page=crow::mustache::load("student_dashboard.html");
                        return crow::response(page.render());
                    }
                }
                return crow::response("NO SUCH USER");
            }
            else if(role=="teacher"){
                teacher_data* data=table.findteacher(*user);
                if(data){
                    if(data->password==pass){
                        auto page=crow::mustache::load("teacher_dashboard.html");
                        return crow::response(page.render());
                    }
                }
                return crow::response("NO SUCH USER");
            }
        }
        return crow::response("NO SUCH USER");
    });

    CROW_ROUTE(app, "/signup")([](){
        auto page=crow::mustache::load("signup.html");
        return page.render();
    });

    CROW_ROUTE(app, "/signup_post").methods("POST"_method)([&table](const crow::request& req) -> crow::response {
        auto req_body = crow::query_string(("?" + req.body).c_str());

        std::string name=req_body.get("fullname");
        std::string username=req_body.get("username");
        std::string email=req_body.get("email");
        std::string pass=req_body.get("password");
        std::string role=req_body.get("role");

        if(role=="student"){
            student_data* new_user=new student_data(name,username,email,pass);
            table.addStudent(new_user);
            auto page=crow::mustache::load("student_dashboard.html");
            return crow::response(page.render());
        }
        else{
            teacher_data* new_user=new teacher_data(name,username,email,pass);
            table.addTeacher(new_user);
            auto page=crow::mustache::load("teacher_dashboard.html");
            return crow::response(page.render());
        }
    });

    CROW_ROUTE(app, "/signToLoginpage")([](){
        auto page=crow::mustache::load("login.html");
        return page.render();
    });
    std::cout << "Server running at http://localhost:18080\n";

    app.port(18080).multithreaded().run();
    }catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
    }
}