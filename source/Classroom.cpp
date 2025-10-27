/*
 * Description: Implements JSON serialization for classroom_data and registers all Crow routes related to classroom management, including creation (teacher) and joining (student).
 */

#include "Common_Route.hpp"

using njson = nlohmann::json;

/*
 * `to_json` overload for `classroom_data`.
 * Called by nlohmann::json when serializing a classroom (e.g., saving to file).
 */
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

/*
 * Registers all routes related to classroom management (for both teachers and students).
 */
void registerClassroomRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table){

    /*
     * Route: /create_classroom (GET)
     * Description: Displays the HTML form for a teacher to create a new classroom.
     */

    CROW_ROUTE(app, "/create_classroom")([&app](const crow::request& req)->crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");

        // Security Check
        if(user_type!="teacher"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["teacher_username"]=session.get<std::string>("username");

        auto page=crow::mustache::load("teacher/create_classroom.html");
        return crow::response(page.render(ctx));

    });

    /*
     * Route: /create_classroom_post (POST)
     * Description: Handles the submission of the new classroom form.
     */
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

        // O(1) average-case lookup
        teacher_data* teacher=user_table.findTeacher(username);
        if(!teacher){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // O(1) average-case insertion
        std::string new_class_code=classroom_table.addClassroom(classname,subject,teacher);

        // Link the new classroom to the teacher
        teacher->classroomIds.push_back(new_class_code);

        // Persist changes
        user_table.saveTeachersToFile();
        classroom_table.saveClassroomsToFile();

        // Redirect to a success page displaying the new code
        crow::response res(303);
        res.add_header("Location","/classroom_created?code="+new_class_code);
        return res;

    });

    /*
     * Route: /classroom_created
     * Description: Success page that displays the newly generated classroom code.
     */
    CROW_ROUTE(app,"/classroom_created")([](const crow::request& req){
        const char* code = req.url_params.get("code");

        if (!code || std::string(code).empty()) {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["class_code"]=req.url_params.get("code");
        
        auto page=crow::mustache::load("teacher/classroom_created.html");
        return crow::response(page.render(ctx));
    });

    /*
     * Route: /my_classrooms
     * Description: (Teacher) Displays a list of all classrooms owned by the teacher.
     */
    CROW_ROUTE(app, "/my_classrooms")
    ([&app, &user_table, &classroom_table](const crow::request& req) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        std::string username = session.get<std::string>("username");

        if (user_type != "teacher" || username.empty()) {
            return crow::response(303, "/error");
        }

        // O(1) average-case lookup
        teacher_data* teacher = user_table.findTeacher(username);
        if (!teacher) {
            return crow::response(303, "/error");
        }

        crow::mustache::context ctx;
        ctx["teacher_username"] = teacher->username;

        std::vector<crow::json::wvalue> classrooms_list;

        // Iterate through the teacher's list of class codes
        for (const auto& class_code : teacher->classroomIds) {
            // O(1) average-case lookup for each class
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

        auto page = crow::mustache::load("teacher/my_classrooms.html");
        return crow::response(page.render(ctx));
    });


    /*
     * Route: /classroom/<string>
     * Description: (Teacher) Displays a detailed view of a single classroom,
     * showing the list of students and quizzes.
     */
    CROW_ROUTE(app, "/classroom/<string>")
    ([&app, &user_table, &classroom_table, &quiz_table](const crow::request& req, const std::string& class_code) -> crow::response {
        auto& session = app.get_context<Session>(req);
        std::string user_type = session.get<std::string>("user_type");
        if (user_type != "teacher") {
            CROW_LOG_INFO << "Error: User is not a teacher.";
            return crow::response(303, "/error");
        }

        // O(1) average-case lookup
        classroom_data* room = classroom_table.findClassroom(class_code);
        if (!room) {
            return crow::response(404, "/error");
        }

        crow::mustache::context ctx;
        ctx["class_name"] = room->class_name;
        ctx["subject"] = room->subject;
        ctx["class_code"] = room->class_code;

        // Populate the list of students in the classroom
        std::vector<crow::json::wvalue> students_list;
        for (const auto& student_username : room->student_usernames) {
            // O(1) average-case lookup for each student
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

        // Populate the list of quizzes in the classroom
        std::vector<crow::json::wvalue> quizzes_list;
        for (const auto& quiz_id : room->quizIds) {
            // O(1) average-case lookup for each quiz
            quiz_data* quiz = quiz_table.findQuiz(quiz_id);
            if (quiz) {
                crow::json::wvalue quiz_obj;
                quiz_obj["quizTitle"] = quiz->quizTitle;
                quiz_obj["quizId"] = quiz->quizId;
                quiz_obj["quizLink"] = "/quiz_leaderboard/" + quiz->quizId;
                quizzes_list.push_back(std::move(quiz_obj));
            }
        }
        if (!quizzes_list.empty()) {
            ctx["quizzes"] = std::move(quizzes_list);
        }

        auto page = crow::mustache::load("teacher/classroom_details.html");
        return crow::response(page.render(ctx));
    });

    /*
     * Route: /join_classroom (GET)
     * Description: (Student) Displays the HTML form for a student to join a classroom using a class code.
     */
    CROW_ROUTE(app, "/join_classroom")([&app](const crow::request& req){
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");

        if(user_type!="student"){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;

        auto page=crow::mustache::load("student/join_classroom.html");
        return crow::response(page.render(ctx));
    });

    /*
     * Route: /join_classroom_post (POST)
     * Description: (Student) Handles the submission of the "join classroom" form.
     */
    CROW_ROUTE(app, "/join_classroom_post").methods("POST"_method)([&app, &user_table, &classroom_table](const crow::request& req) -> crow::response {
        auto& session=app.get_context<Session>(req);

        std::string user_type=session.get<std::string>("user_type");
        std::string username=session.get<std::string>("username");
        
        if(user_type!="student" || username.empty()){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        auto body=crow::query_string("?"+req.body);

        const char* code_cstr=body.get("class_code");
        if(!code_cstr){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }
        std::string class_code(code_cstr);

        // O(1) average-case lookup to find the classroom
        classroom_data* classroom=classroom_table.findClassroom(class_code);
        if(!classroom){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // O(1) average-case lookup to find the student
        student_data* student=user_table.findStudent(username);
        if(!student){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        // Check if student is already in the class
        auto& students_in_class=classroom->student_usernames;
        if (std::find(students_in_class.begin(), students_in_class.end(), username) != students_in_class.end()) {
            return crow::response(303, "You are already in this classroom.");
        }

        // Create the two-way link
        classroom->student_usernames.push_back(username);   // Add student to class
        student->classroomIds.push_back(class_code);    // Add class to student

        // Persist changes
        classroom_table.saveClassroomsToFile();
        user_table.saveStudentsToFile();

        crow::response res(303);
        res.add_header("Location", "/classroom_joined?code="+class_code);
        return res;

    });

    /*
     * Route: /classroom_joined
     * Description: (Student) Success page shown after joining a classroom.
     */
    CROW_ROUTE(app, "/classroom_joined")([&app, &classroom_table](const crow::request& req){
        auto& session = app.get_context<Session>(req);
        if (session.get<std::string>("user_type") != "student") {
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }
        
        const char* class_code = req.url_params.get("code");

        // O(1) average-case lookup
        classroom_data* room=classroom_table.findClassroom(class_code);
        if(!room){
            crow::response res(303);
            res.add_header("Location", "/error");
            return res;
        }

        crow::mustache::context ctx;
        ctx["class_name"]=room->class_name;
        ctx["class_code"]=room->class_code;

        auto page=crow::mustache::load("student/classroom_joined.html");
        return crow::response(page.render(ctx));
    });

    /*
     * Route: /student/classroom/<string>
     * Description: (Student) Displays a detailed view of a classroom,
     * showing the quizzes available to attempt.
     */
    CROW_ROUTE(app, "/student/classroom/<string>")([&app, &classroom_table, &quiz_table](const crow::request& req, const std::string& class_code) -> crow::response {
        auto& session=app.get_context<Session>(req);
        std::string user_type=session.get<std::string>("user_type");
        if(user_type!="student"){
            return crow::response(303,"/error");
        }

        // O(1) average-case lookup
        classroom_data* room=classroom_table.findClassroom(class_code);
        if(!room){
            return crow::response(404, "/error");
        }

        crow::mustache::context ctx;
        ctx["class_name"]=room->class_name;
        ctx["class_code"]=room->class_code;
        ctx["subject"]=room->subject;

        std::vector<crow::json::wvalue> quizzes_list;
        // Iterate through the classroom's quiz IDs
        for(const auto& quiz_id: room->quizIds){
            // O(1) average-case lookup for each quiz
            quiz_data* quiz=quiz_table.findQuiz(quiz_id);
            if(quiz){
                crow::json::wvalue quiz_obj;
                quiz_obj["quizTitle"]=quiz->quizTitle;
                quiz_obj["quizId"]=quiz->quizId;
                quiz_obj["timeLimitMins"]=quiz->timeLimitMins;
                quiz_obj["quizLink"] = "/student/attempt_quiz/" + quiz->quizId;
                quizzes_list.push_back(std::move(quiz_obj));
            }
        }

        if(!quizzes_list.empty()){
            ctx["quizzes"]=std::move(quizzes_list);
        }

        auto page=crow::mustache::load("student/classroom_details.html");
        return crow::response(page.render(ctx));
    });

}