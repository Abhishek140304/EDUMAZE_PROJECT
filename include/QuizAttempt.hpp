#ifndef QUIZ_ATTEMPT_HPP
#define QUIZ_ATTEMPT_HPP

/*
 * Description: This header defines the data structure for a student's quiz result/attempt (`quiz_result_data`) and the hash table (`quiz_result_hashTable`) to manage all results.
 *
 * DSA Concepts:
 * 1.  **Hash Table:** `quiz_result_hashTable` maps a unique `resultId` (string) to the `quiz_result_data`.
 * 2.  **Separate Chaining:** Collisions are handled with `quiz_result_link`.
 * 3.  **Hash Function:** `fnv1a` is used to hash the `resultId`.
 *
 * DSA Note on Lookups:
 * This implementation uses `resultId` as the primary key. This is O(1) for adding a new result.
 * However, finding all results for a *specific quiz*
 (`findResultsForQuiz`) or checking if a *student has attempted* a quiz (`hasStudentAttempted`) requires iterating over the *entire hash table* (all buckets and all chains). This is an O(N) operation, where N is the total number of results in the system.
 */

#include "crow.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <iostream>

using njson=nlohmann::json;

// Struct to represent a single completed quiz attempt by a student
struct quiz_result_data{
    std::string resultId;   // Primary key for the hash table
    std::string quizId; // Foreign key to the quiz
    std::string studentUsername;
    int score;  // Foreign key to the student
    double timeTakenSeconds;
    std::vector<int> submittedAnswers;   // Index=Q#, Value=Option# (-1 if unanswered)

    quiz_result_data(): score(0), timeTakenSeconds(0.0){}

    quiz_result_data(const std::string& resId, const std::string& qId, const std::string& sUsername, int s, double t, const std::vector<int>& answers):
    resultId(resId), quizId(qId), studentUsername(sUsername), score(s), timeTakenSeconds(t), submittedAnswers(answers){}
};

// Linked list node for the quiz result hash table
struct quiz_result_link{
    quiz_result_data* data=nullptr;
    quiz_result_link* next=nullptr;
};

// JSON serialization for quiz_result_data
inline void to_json(njson& j, const quiz_result_data& r){
    j=njson{
        {"resultId", r.resultId},
        {"quizId", r.quizId},
        {"studentUsername", r.studentUsername},
        {"score", r.score},
        {"timeTakenSeconds", r.timeTakenSeconds},
        {"submittedAnswers", r.submittedAnswers}
    };
}

// JSON deserialization for quiz_result_data
inline void from_json(const njson& j, quiz_result_data& r){
    r.resultId = j.value("resultId", "");
    r.quizId = j.value("quizId", "");
    r.studentUsername = j.value("studentUsername", "");
    r.score = j.value("score", 0);
    r.timeTakenSeconds = j.value("timeTakenSeconds", 0.0);
    r.submittedAnswers = j.value("submittedAnswers", std::vector<int>{});
}

// Implements a hash table to store all quiz attempts.
// The key is the `resultId`.
class quiz_result_hashTable{
private:
    quiz_result_link** quiz_results;
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
    
    // Generates a random 6-character ID for the result
    std::string generate_result_id() {
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

    // Loads result data from JSON file and populates the hash table
    void makeResults_hashtable(int& table_size, std::ifstream& file){
        njson data;

        file>>data;
        for (const auto& res_json : data) {
            // 1. Deserialize JSON into object
            quiz_result_data temp_res;
            from_json(res_json, temp_res);

            // 2. Create heap-allocated object
            quiz_result_data* new_result=new quiz_result_data(temp_res);

            // 3. Hash the key (resultId)
            uint32_t index = fnv1a(new_result->resultId) % table_size;

            // 4. Insert into hash table
            quiz_result_link* newnode=new quiz_result_link;
            newnode->data=new_result;

            if (quiz_results[index]) {
                newnode->next = quiz_results[index];
                quiz_results[index] = newnode;
            } else {
                quiz_results[index] = newnode;
            }
        }
    }

public:
    // Constructor: Initializes and populates the hash table
    quiz_result_hashTable(int table_size=50): size(table_size){
        quiz_results=new quiz_result_link*[size];
        for(int i=0; i<size; i++){
            quiz_results[i]=nullptr;
        }

        std::ifstream resultsFile("Data/quiz_results.json");
        if (resultsFile.is_open()) {
            makeResults_hashtable(size, resultsFile);
            resultsFile.close();
        } else {
            std::ofstream newFile("Data/quiz_results.json");
            newFile << "[]";
            newFile.close();
        }
    }

    // Destructor: Saves data and deallocates memory
    ~quiz_result_hashTable() {
        std::cout << "Saving quiz results to file..." << std::endl;
        saveResultsToFile();    // Save one last time
        for (int i = 0; i < size; ++i) {
            quiz_result_link* curr = quiz_results[i];
            while (curr != nullptr) {
                quiz_result_link* next = curr->next;
                delete curr->data;
                delete curr;
                curr = next;
            }
        }
        delete[] quiz_results;
    }

    // Saves all quiz result data back to the JSON file
    void saveResultsToFile() {
        njson results_json_array = njson::array();
        for (int i = 0; i < size; ++i) {
            quiz_result_link* curr = quiz_results[i];
            while (curr != nullptr) {
                results_json_array.push_back(*(curr->data));
                curr = curr->next;
            }
        }
        std::ofstream resultsFile("Data/quiz_results.json");
        resultsFile << results_json_array.dump(4);
        resultsFile.close();
    }

    /*
     * Adds a new quiz result to the hash table.
     * Time Complexity: O(1) average.
     */
    quiz_result_data* addResult(const std::string& quizId, const std::string& studentUsername, int score, double timeTaken, const std::vector<int>& answers) {
        std::string resId = generate_result_id();
        quiz_result_data* new_result = new quiz_result_data(resId, quizId, studentUsername, score, timeTaken, answers);

        uint32_t index = fnv1a(resId) % size;
        quiz_result_link* newnode = new quiz_result_link;
        newnode->data = new_result;

        // Insert at the head of the list
        if (quiz_results[index]) {
            newnode->next = quiz_results[index];
        }
        quiz_results[index] = newnode;

        return new_result;
    }

    /*
     * Finds all results for a specific quiz.
     * Time Complexity: O(N), where N is the *total number of results*
     * in the system. This function must iterate through every single bucket
     * and every single node in the hash table to find matches.
     */
    std::vector<quiz_result_data*> findResultsForQuiz(const std::string& quizId) {
        std::vector<quiz_result_data*> quiz_attempts;
        // Iterate over the array
        for (int i = 0; i < size; ++i) {
            quiz_result_link* curr = quiz_results[i];
            // Iterate over the linked list at this index
            while (curr != nullptr) {
                if (curr->data->quizId == quizId) {
                    quiz_attempts.push_back(curr->data);
                }
                curr = curr->next;
            }
        }
        return quiz_attempts;
    }

    /*
     * Checks if a specific student has already attempted a specific quiz.
     * Time Complexity: O(N), same as `findResultsForQuiz`. It must
     * iterate through the entire table to find a potential match.
     */
    bool hasStudentAttempted(const std::string& studentUsername, const std::string& quizId) {
        for (int i = 0; i < size; ++i) {
            quiz_result_link* curr = quiz_results[i];
            while (curr != nullptr) {
                if (curr->data->studentUsername == studentUsername && curr->data->quizId == quizId) {
                    return true;
                }
                curr = curr->next;
            }
        }
        return false;
    }
};

#endif