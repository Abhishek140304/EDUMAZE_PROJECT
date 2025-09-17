#ifndef COMMON_ROUTE_HPP
#define COMMON_ROUTE_HPP

#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include "Users.hpp"
#include "Classroom.hpp"
#include "Quiz.hpp"

using Session=crow::SessionMiddleware<crow::InMemoryStore>;

void registerClassroomRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table);

void registerQuizRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table, classroom_hashTable& classroom_table, quiz_hashTable& quiz_table);

void registerStudentsRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table);

void registerTeachersRoutes(crow::App<crow::CookieParser,Session>& app, user_hashTable& user_table);

#endif