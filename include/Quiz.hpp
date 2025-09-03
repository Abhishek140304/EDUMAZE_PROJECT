#include <string>
#include <vector>
#include <unordered_map>

// Represents a single MCQ
struct Question {
    std::string questionText;
    std::vector<std::string> options; // e.g., options[0]="A", options[1]="B", ...
    int correctOptionIndex; // The index of the correct answer in the options vector
};

// Represents a full quiz
struct Quiz {
    std::string quizId;
    std::string classId;
    std::string title;
    int timeLimitMinutes; // Time limit for the quiz
    std::vector<Question> questions;
};

// Global store for quizzes
std::unordered_map<std::string, Quiz> quiz_data; // Keyed by quizId