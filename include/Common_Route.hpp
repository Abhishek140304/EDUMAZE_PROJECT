#ifndef COMMON_ROUTE_HPP
#define COMMON_ROUTE_HPP

#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include "users.hpp"
#include "Classroom.hpp"
#include "Quiz.hpp"
#include "QuizAttempt.hpp"

using Session=crow::SessionMiddleware<crow::InMemoryStore>;

void registerClassroomRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

void registerQuizRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

void registerQuizAttemptRoutes(crow::App<crow::CookieParser,Session>& app, quiz_hashTable& quiz_table, quiz_result_hashTable& results_table
);

void registerStudentsRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

void registerTeachersRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

#endif
