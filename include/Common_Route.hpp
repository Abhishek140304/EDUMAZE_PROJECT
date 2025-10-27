#ifndef COMMON_ROUTE_HPP
#define COMMON_ROUTE_HPP

/*
 * Description: This is a central header file. Its main purpose is to:
 * 1.  Include all the major components of the application (Crow, Session, and all our custom data structure headers).
 * 2.  Define a common type alias for the Crow Session.
 * 3.  Provide forward declarations for all the route registration functions (which are defined in their respective .cpp files).
 *
 * This helps to reduce code duplication in the .cpp files, as they can all just `#include "Common_Route.hpp"` to get access to everything they need.
 */

#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include "users.hpp"
#include "Classroom.hpp"
#include "Quiz.hpp"
#include "QuizAttempt.hpp"

using Session=crow::SessionMiddleware<crow::InMemoryStore>;

// Registers all routes related to classrooms (create, join, view)
void registerClassroomRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

// Registers all routes related to quizzes (create, view)
void registerQuizRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

// Registers all routes related to attempting a quiz (start, submit, leaderboard)
void registerQuizAttemptRoutes(crow::App<crow::CookieParser,Session>& app, quiz_hashTable& quiz_table, quiz_result_hashTable& results_table
);

// Registers all routes specific to students (dashboard, etc.)
void registerStudentsRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

// Registers all routes specific to teachers (dashboard, etc.)
void registerTeachersRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

#endif