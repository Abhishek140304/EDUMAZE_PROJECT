/*
 * Description: This is the main entry point for the Crow web application.
 * It initializes all the core data structures (hash tables), sets up the web server and middleware, and defines the primary routes for authentication (login, signup, logout) and navigation.
 */

 // --- Crow Libraries ---
#include "crow.h"   // The core Crow micro-framework
#include "crow/middlewares/cookie_parser.h" // Middleware for handling cookies
#include "crow/middlewares/session.h"   // Middleware for in-memory session management

// --- Standard Libraries ---
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <any> 

// This single header includes all our data structures and route declarations
#include "include/Common_Route.hpp"


int main(){
    /*
     * A try...catch block wraps the entire application. If any part of the setup fails (e.g., can't open a JSON file in a hash table constructor), it will be caught and printed, preventing a silent crash.
     */
    try{
        // This is where all our custom data structures are instantiated.
    user_hashTable user_table;
    classroom_hashTable classroom_table;
    quiz_hashTable quiz_table;
    quiz_result_hashTable result_table;

    // Tell Crow where to find the HTML template files
    crow::mustache::set_base("templates");

    // Initialize the Crow application with Cookie and Session middleware
    crow::App<crow::CookieParser,Session> app;

    // Enable the session middleware
    app.get_middleware<Session>();

    /*
     * Route: / (Root)
     * Description: The main entry point. It checks the user's session.
     * - If logged in, redirects to their respective dashboard.
     * - If not logged in, redirects to the /welcome_page.
     */
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

    /*
     * Route: /welcome_page
     * Description: Displays the main welcome/landing page.
     */
    CROW_ROUTE(app, "/welcome_page")([](){
        auto page=crow::mustache::load("common/welcome.html");
        return page.render();
    });

    /*
     * Route: /loginpage
     * Description: Displays the login form.
     */
    CROW_ROUTE(app, "/loginpage")([](){
        auto page=crow::mustache::load("common/login.html");
        return page.render();
    });

    /*
     * Route: /login_post (POST)
     * Description: Handles the login form submission.
     * This is a key route demonstrating the hash table lookups.
     */
    CROW_ROUTE(app, "/login_post").methods("POST"_method)([&app,&user_table](const crow::request& req) -> crow::response {
        auto req_body = crow::query_string(("?" + req.body).c_str());

        crow::response res;
        std::string email=req_body.get("email");
        std::string pass=req_body.get("password");
        std::string role=req_body.get("role");

        // O(1) average-case lookup in the `emails` hash table.
        // This finds the username associated with the email.
        std::string* user=user_table.findUsername(email);

        if(!user){
            // Email not found in the hash table
            res.code = 303;
            res.add_header("Location", "/error");
            return res;
        }

        bool login_success=false;

        std::string destination;

        // Now that we have the username, we check the correct user type table
        if(user){
            if(role=="student"){
                // O(1) average-case lookup in the `students` hash table.
                student_data* data=user_table.findStudent(*user);
                // Check if the student exists and the password matches
                if(data && data->password==pass){
                    login_success=true;
                    destination="student_dashboard";
                }
            }
            else if(role=="teacher"){
                // O(1) average-case lookup in the `teachers` hash table.
                teacher_data* data=user_table.findTeacher(*user);
                // Check if the teacher exists and the password matches
                if(data && data->password==pass){
                    login_success=true;
                    destination="teacher_dashboard";
                }
            }
        }

        if(login_success){
            // Store user info in the session
            auto& session=app.get_context<Session>(req);
            session.set("username",*user);
            session.set("user_type",role);

            res.code=303;
            res.add_header("Location",destination);
        }
        else{
            // Login failed (e.g., wrong password or wrong role selected)
            res.code=303;
            res.add_header("Location","/error");
        }
        return res;
    });

    /*
     * Route: /signup
     * Description: Displays the signup form.
     */
    CROW_ROUTE(app, "/signup")([](){
        auto page=crow::mustache::load("common/signup.html");
        return page.render();
    });

    /*
     * Route: /signup_post (POST)
     * Description: Handles the signup form submission.
     * Demonstrates hash table lookups (for checking existence) and insertions.
     */
    CROW_ROUTE(app, "/signup_post").methods("POST"_method)([&app,&user_table](const crow::request& req) -> crow::response {
        auto req_body = crow::query_string(("?" + req.body).c_str());

        crow::response res;
        std::string name=req_body.get("fullname");
        std::string username=req_body.get("username");
        std::string email=req_body.get("email");
        std::string pass=req_body.get("password");
        std::string role=req_body.get("role");

        // O(1) average-case lookup to check if email is already taken.
        if(user_table.findUsername(email)){
            res.code = 303;
            res.add_header("Location", "/error");
            return res;
        }

        // Create the session for the new user
        auto& session=app.get_context<Session>(req);
        session.set("username",username);
        session.set("user_type",role);
        res.code=303;

        if(role=="student"){
            student_data* new_user=new student_data(name,username,email,pass);
            // O(1) average-case insertion into both `students` and `emails` tables.
            user_table.addStudent(new_user);
            res.add_header("Location","/student_dashboard");
        }
        else{
            teacher_data* new_user=new teacher_data(name,username,email,pass);
            // O(1) average-case insertion into both `teachers` and `emails` tables.
            user_table.addTeacher(new_user);
            res.add_header("Location","/teacher_dashboard");
        }
        return res;
    });

    /*
     * Route: /logout
     * Description: Clears all data from the user's session and redirects to the welcome page.
     */
    CROW_ROUTE(app, "/logout")([&app](const crow::request& req, crow::response& res){
        auto& session=app.get_context<Session>(req);

        // Iterate over all keys in the session and remove them
        for (const auto& key : session.keys()) {
            session.remove(key);
        }

        res.add_header("Location","/welcome_page");
        res.code=303;
        res.end();
    });

    /*
     * Route: /error
     * Description: A generic error page.
     */
    CROW_ROUTE(app,"/error")([](){
        auto page=crow::mustache::load("common/error.html");
        return crow::response(page.render());
    });

    /*
     * Route: /change_password (GET)
     * Description: Displays the "change password" form for a logged-in user.
     * It also displays error/success messages passed as URL parameters.
     */
    CROW_ROUTE(app, "/change_password")
    ([&app](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string username = session.get<std::string>("username");

        if (username.empty()) {
            return crow::response(303, "/error");
        }

        crow::mustache::context ctx;
        ctx["username"] = username;

        // Check for error/success messages from a redirect
        if (req.url_params.get("error")) {
            std::string error = req.url_params.get("error");
            if (error == "mismatch") {
                ctx["error_message"] = "New passwords do not match. Please try again.";
            } else if (error == "incorrect") {
                ctx["error_message"] = "Your current password was incorrect.";
            } else if (error == "notfound") {
                 ctx["error_message"] = "Could not find user account.";
            }
        }
        if (req.url_params.get("success")) {
             ctx["success_message"] = "Password updated successfully!";
        }
        
        auto page = crow::mustache::load("common/change_password.html");
        return crow::response(page.render(ctx));
    });

    /*
     * Route: /change_password_post (POST)
     * Description: Handles the "change password" form submission.
     */
    CROW_ROUTE(app, "/change_password_post").methods("POST"_method)
    ([&app, &user_table](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string username = session.get<std::string>("username");
        std::string user_type = session.get<std::string>("user_type");

        if (username.empty() || user_type.empty()) {
            return crow::response(303, "/error"); 
        }

        auto body = crow::query_string("?" + req.body);
        std::string current_pass = body.get("current_password");
        std::string new_pass = body.get("new_password");
        std::string confirm_pass = body.get("confirm_password");

        // Validate new password
        if (new_pass.empty() || new_pass != confirm_pass) {
            return crow::response(303, "/change_password?error=mismatch");
        }

        bool password_updated = false;

        if (user_type == "student") {
            // O(1) average-case lookup
            student_data* student = user_table.findStudent(username);
            if (!student) {
                return crow::response(303, "/change_password?error=notfound");
            }
            // Check if the current password is correct
            if (student->password == current_pass) {
                student->password = new_pass; // Update password in memory
                user_table.saveStudentsToFile(); // Save to students.json
                password_updated = true;
            }

        } else if (user_type == "teacher") {
            // O(1) average-case lookup
            teacher_data* teacher = user_table.findTeacher(username);
            if (!teacher) {
                return crow::response(303, "/change_password?error=notfound");
            }
            // Check if the current password is correct
            if (teacher->password == current_pass) {
                teacher->password = new_pass; // Update password in memory
                user_table.saveTeachersToFile(); // Save to teachers.json
                password_updated = true;
            }
        }

        if (password_updated) {
            // Success! Redirect back to the same page with a success message
            return crow::response(303, "/change_password?success=true");
        } else {
            // Failure! (Incorrect current password)
            return crow::response(303, "/change_password?error=incorrect");
        }
    });

    // These functions (defined in their respective .cpp files) are called to register all other routes (e.g., /create_classroom, /attempt_quiz).
    // We pass the data structures to them so they can interact with the data.
    registerStudentsRoutes(app, user_table, classroom_table, quiz_table);

    registerTeachersRoutes(app,user_table,classroom_table,quiz_table);

    registerClassroomRoutes(app,user_table, classroom_table, quiz_table);

    registerQuizRoutes(app,user_table, classroom_table, quiz_table);

    registerQuizAttemptRoutes(app, quiz_table, result_table);

    // Start Server
    std::cout << "Server running at http://localhost:18080\n";

    app.port(18080).multithreaded().bindaddr("0.0.0.0").run();

    }catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
    }
}