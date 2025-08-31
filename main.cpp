#include "crow.h"
int main(){
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
    app.port(18080).multithreaded().run();
}