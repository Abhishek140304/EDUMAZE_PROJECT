// #pragma once
// #include "include/User.hpp"
// #include "include/Auth.hpp"
// #include <functional> 
// #include <string>

// // An enum to represent the result of a login attempt
// enum class LoginStatus {
//     SUCCESS_TEACHER,
//     SUCCESS_STUDENT,
//     INVALID_CREDENTIALS
// };

// /**
//  * @brief Adds a new user to the system.
//  * * @param username The user's display name.
//  * @param email The user's email, used as a unique ID.
//  * @param password The user's raw password.
//  * @param isTeacher True if the user is a teacher, false for a student.
//  * @return True if user was added successfully, false if the email already exists.
//  */
// bool addNewUser(const std::string& username, const std::string& email, const std::string& password, bool isTeacher) {
//     if (user_data.count(email)) {
//         return false; // User already exists
//     }

//     std::string hashedPassword = hashPassword(password);

//     // The 'isTeacher' parameter directly sets the user's role upon creation.
//     User newUser = {email, username, hashedPassword, isTeacher};

//     user_data[email] = newUser;
//     return true;
// }

// /**
//  * @brief Checks user credentials against stored data.
//  * * @param email The user's email.
//  * @param password The user's password.
//  * @return A LoginStatus enum indicating success (and role) or failure.
//  */
// LoginStatus checkLoginDetails(const std::string& email, const std::string& password) {
//     if (!user_data.count(email)) {
//         return LoginStatus::INVALID_CREDENTIALS; // User not found
//     }

//     const User& storedUser = student_data.at(email);
//     std::string providedHashedPassword = hashPassword(password);

//     if (storedUser.hashedPassword == providedHashedPassword) {
//         // Here, we check the role *after* verifying the password.
//         // The role is retrieved from our database, not from the login form.
//         return storedUser.isTeacher ? LoginStatus::SUCCESS_TEACHER : LoginStatus::SUCCESS_STUDENT;
//     }

//     return LoginStatus::INVALID_CREDENTIALS;
// }