#ifndef QUIZ_HPP
#define QUIZ_HPP

/*
 * Description: This header defines the data structures for Quizzes and Questions, and the hash table (`quiz_hashTable`) to manage them.
 *
 * DSA Concepts:
 * 1.  **Hash Table:** `quiz_hashTable` maps a unique `quizId` (string) to the corresponding `quiz_data`. This allows for O(1) average-case lookup when a student attempts a quiz or a teacher views its results.
 * 2.  **Separate Chaining:** Collisions are handled using a linked list (`quiz_link`).
 * 3.  **Hash Function:** The `fnv1a` function is used for hashing the `quizId`.
 * 4.  **Structs & Vectors:** `quiz_data` and `Question` structs use `std::vector` to store a dynamic list of questions and options.
 */

#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <sstream>
#include "json.hpp"

using njson=nlohmann::json;

// Represents a single MCQ question
struct Question{
    std::string questionText;
    std::vector<std::string> options;
    int correctAnswerIndex;
};

// Represents a full quiz
struct quiz_data{
    std::string quizId; // The primary key for the hash table
    std::string quizTitle;
    std::string classroomId;    // Links this quiz to a classroom
    int timeLimitMins;  // Time limit for the attempt
    std::vector<Question> questions;    // All questions for this quiz

    quiz_data() = default;

    quiz_data(const std::string& thequizId, const std::string& thequizTitle, const std::string& theclassroomId, int thetimeLimitMins, const std::vector<Question>& thequestions):
    quizId(thequizId), quizTitle(thequizTitle), classroomId(theclassroomId), timeLimitMins(thetimeLimitMins), questions(thequestions){}

};

// Linked list node for the quiz hash table (separate chaining)
struct quiz_link{
    quiz_data* data=nullptr;
    quiz_link* next=nullptr;
};

void to_json(njson& j, const Question& q);

void from_json(const njson& j, Question& q);

void to_json(njson& j, const quiz_data& q);

void from_json(const njson& j, quiz_data& q);


// Implements a hash table to store all quiz data.
// The key is the `quizId`
class quiz_hashTable {
private:
    quiz_link** quizzes;
    int size;

    // FNV-1a hash function
    uint32_t fnv1a(const std::string& s) {
        const uint32_t basis = 2166136261u;
        const uint32_t prime = 16777619u;
        uint32_t hash = basis;
        for (unsigned char c : s) {
            hash ^= c;
            hash *= prime;
        }
        return hash;
    }
    
    // Generates a random 6-character ID for the quiz
    std::string generate_quiz_id() {
        const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, chars.size() - 1);
        
        std::stringstream ss;
        for (int i = 0; i < 6; ++i) {
            ss << chars[distribution(generator)];
        }
        return ss.str();
    }

    // Loads quiz data from JSON file and populates the hash table
    void makeQuizzes_hashtable(int& table_size, std::ifstream& file) {
        njson data;

        file>>data;
        for (const auto& quiz_json : data) {
            // 1. Deserialize JSON into quiz_data object
            // This uses the from_json functions
            quiz_data temp_quiz;

            temp_quiz.quizId = quiz_json.value("quizId", "");
            temp_quiz.quizTitle = quiz_json.value("quizTitle", "");
            temp_quiz.classroomId = quiz_json.value("classroomId", "");
            temp_quiz.timeLimitMins = quiz_json.value("timeLimitMinutes", 0);
            temp_quiz.questions = quiz_json.value("questions", std::vector<Question>{});

            // 2. Create the data object on the heap
            quiz_data* new_quiz = new quiz_data(temp_quiz);

            // 3. Hash the key (quizId)
            uint32_t index = fnv1a(new_quiz->quizId) % table_size;

            // 4. Insert into hash table (add to front of list)
            quiz_link* newnode = new quiz_link;
            newnode->data=new_quiz;
            if (quizzes[index]) {
                newnode->next = quizzes[index];
                quizzes[index]=newnode;
            }
            else{
                quizzes[index] = newnode;
            }
        }
    }

public:
    // Constructor: Initializes and populates the hash table
    quiz_hashTable() {
        size = 50;
        quizzes  = new quiz_link*[size];

        for(int i=0; i<size; i++){
            quizzes[i]=nullptr;
        }
        std::ifstream quizFile("Data/quizzes.json");
        if (quizFile.is_open()) {
            makeQuizzes_hashtable(size, quizFile);
        } else {
            std::ofstream newFile("Data/quizzes.json");
            newFile << "[]";
            newFile.close();
        }
    }

    // Saves all quiz data back to the JSON file
    void saveQuizzesToFile() {
        njson quizzes_json_array = njson::array();
        for (int i = 0; i < size; ++i) {
            quiz_link* curr = quizzes[i];
            while (curr != nullptr) {
                quizzes_json_array.push_back(*(curr->data));
                curr = curr->next;
            }
        }
        std::ofstream quizFile("Data/quizzes.json");
        quizFile << quizzes_json_array.dump(4);
        quizFile.close();
    }

    /*
     * Creates a new quiz, adds it to the hash table, and returns its data.
     * Time Complexity: O(1) average.
     */
    quiz_data* createQuiz(const std::string& title, const std::string& classroomId, int timeLimit, const std::vector<Question>& questions) {
        std::string id = generate_quiz_id();
        uint32_t index = fnv1a(id) % size;

        quiz_data* new_quiz_data  = new quiz_data(id, title, classroomId, timeLimit, questions);
        quiz_link* newnode = new quiz_link;
        newnode->data=new_quiz_data;

        // Insert at the head of the linked list
        if (quizzes[index]) {
            newnode->next = quizzes[index];
        }
        quizzes[index] = newnode;

        return new_quiz_data;
    }

    /*
     * Finds a quiz by its ID.
     * Time Complexity:
     * - Average: O(1).
     * - Worst: O(n), where n is the total number of quizzes.
     */
    quiz_data* findQuiz(const std::string& quizId) {
        uint32_t index = fnv1a(quizId) % size;
        quiz_link* node = quizzes[index];
        while (node) {
            if (node->data->quizId  == quizId) return node->data;
            node = node->next;
        }
        return nullptr;
    }

    // Destructor: Saves data and deallocates all memory
    ~quiz_hashTable() {
        std::cout << "Saving quiz data to file..." << std::endl;

        njson quizzes_json_array = njson::array();
        for (int i = 0; i < size; ++i) {
            quiz_link* curr = quizzes[i];
            while (curr != nullptr) {
                quizzes_json_array.push_back(*(curr->data));
                quiz_link* next = curr->next;
                delete curr->data;
                delete curr;
                curr = next;
            }
        }
        std::ofstream quizFile("Data/quizzes.json");
        quizFile << quizzes_json_array.dump(4);
        quizFile.close();

        delete[] quizzes;
    }
};

#endif