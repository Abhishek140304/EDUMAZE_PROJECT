#include <string>
#include <vector>
#include <unordered_map>

// Represents a classroom created by a teacher
struct Classroom {
    std::string classId;
    std::string className;
    std::string teacherId;
    std::vector<std::string> studentIds; // A list of student emails
};

std::unordered_map<std::string, Classroom> classroom_data;