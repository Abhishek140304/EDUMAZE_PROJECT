#include <string>
#include <vector>
#include <unordered_map>

// Represents a single user (can be a student or teacher)
struct User {
    std::string userId; // Could be the email
    std::string username;
    std::string hashedPassword;
    bool isTeacher;
};

std::unordered_map<std::string, User> user_data;