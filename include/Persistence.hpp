// #include "include/User.hpp"
// #include "crow.h"   // For Crow's JSON functionality
// #include <fstream>
// #include <iostream>

// // Declares the functions responsible for reading from and writing to the users.json file.
// const std::string JSON_FILE_PATH = "users.json";

// // In Persistence.cpp

// void saveUsersToJson() {
//     // This is just a standard C++ vector
//     crow::json::wvalue::list user_list_data;

//     // Iterate through the in-memory map and fill the vector
//     for (const auto& pair : user_data) {
//         const User& user = pair.second;
//         crow::json::wvalue user_json;
//         user_json["username"] = user.username;
//         user_json["email"] = user.userId;
//         user_json["hashedPassword"] = user.hashedPassword;
//         user_json["isTeacher"] = user.isTeacher;
//         user_list_data.push_back(user_json);
//     }

//     // --- THE FIX IS HERE ---
//     // 1. Create a top-level wvalue object to represent the entire JSON content.
//     crow::json::wvalue json_to_write;

//     // 2. Assign your vector to it. This tells the wvalue object it should be a JSON list.
//     json_to_write = user_list_data;
//     // -----------------------

//     // Open the file for writing
//     std::ofstream file(JSON_FILE_PATH);
//     if (file.is_open()) {
//         // 3. Now, write the main wvalue object to the file. This works!
//         file << json_to_write;
//         file.close();
//     } else {
//         std::cerr << "Error: Could not open " << JSON_FILE_PATH << " for writing." << std::endl;
//     }
// }

// void loadUsersFromJson() {
//     std::ifstream file(JSON_FILE_PATH);
//     if (!file.is_open()) {
//         std::cout << "Info: " << JSON_FILE_PATH << " not found. Starting with empty user data." << std::endl;
//         return; // File doesn't exist yet, which is fine on the first run
//     }

//     std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//     file.close();

//     crow::json::rvalue parsed_json = crow::json::load(content);
//     if (!parsed_json || parsed_json.t() != crow::json::type::List) {
//         std::cerr << "Error: Could not parse " << JSON_FILE_PATH << " or it's not a list." << std::endl;
//         return;
//     }
    
//     // Clear existing in-memory data before loading
//     user_data.clear();

//     // Iterate through the JSON array and populate our in-memory map
//     for (const auto& user_json : parsed_json) {
//         User user;
//         user.username = user_json["username"].s();
//         user.userId = user_json["email"].s();
//         user.hashedPassword = user_json["hashedPassword"].s();
//         user.isTeacher = user_json["isTeacher"].b();
        
//         user_data[user.userId] = user;
//     }
    
//     std::cout << "Successfully loaded " << user_data.size() << " users from " << JSON_FILE_PATH << "." << std::endl;
// }