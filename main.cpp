#include "crow.h"
int main(){
    crow::mustache::set_base("templates");

    crow::SimpleApp app;
    CROW_ROUTE(app, "/")([](){
        auto page=crow::mustache::load("welcome.html");
        return page.render();
    });
    CROW_ROUTE(app, "/loginpage")([](){
        return "Hello";
    });
    app.port(18080).multithreaded().run();
}