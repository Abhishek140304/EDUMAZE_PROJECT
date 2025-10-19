#ifndef QUIZ_ATTEMPT_HPP
#define QUIZ_ATTEMPT_HPP

#include "crow.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <iostream>

using njson=nlohmann::json;

struct quiz_result_data{
    std::string resultId;
    std::string quizId;
    std::string studentUsername;
    int score;
    double timeTakenSeconds;
    std::vector<int> submittedAnswers;   // Index=Q#, Value=Option# (-1 if unanswered)

    quiz_result_data(): score(0), timeTakenSeconds(0.0){}

    quiz_result_data(const std::string& resId, const std::string& qId, const std::string& sUsername, int s, double t, const std::vector<int>& answers):
    resultId(resId), quizId(qId), studentUsername(sUsername), score(s), timeTakenSeconds(t), submittedAnswers(answers){}
};

struct quiz_result_link{
    quiz_result_data* data=nullptr;
    quiz_result_link* next=nullptr;
};

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

inline void from_json(const njson& j, quiz_result_data& r){
    r.resultId = j.value("resultId", "");
    r.quizId = j.value("quizId", "");
    r.studentUsername = j.value("studentUsername", "");
    r.score = j.value("score", 0);
    r.timeTakenSeconds = j.value("timeTakenSeconds", 0.0);
    r.submittedAnswers = j.value("submittedAnswers", std::vector<int>{});
}

class quiz_result_hashTable{
private:
    quiz_result_link** quiz_results;
    int size;

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

    void makeResults_hashtable(int& table_size, std::ifstream& file){
        njson data;

        file>>data;
        for (const auto& res_json : data) {
            quiz_result_data temp_res;
            from_json(res_json, temp_res);

            quiz_result_data* new_result=new quiz_result_data(temp_res);
            uint32_t index = fnv1a(new_result->resultId) % table_size;

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

    ~quiz_result_hashTable() {
        std::cout << "Saving quiz results to file..." << std::endl;
        saveResultsToFile();
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

    quiz_result_data* addResult(const std::string& quizId, const std::string& studentUsername, int score, double timeTaken, const std::vector<int>& answers) {
        std::string resId = generate_result_id();
        quiz_result_data* new_result = new quiz_result_data(resId, quizId, studentUsername, score, timeTaken, answers);

        uint32_t index = fnv1a(resId) % size;
        quiz_result_link* newnode = new quiz_result_link;
        newnode->data = new_result;

        if (quiz_results[index]) {
            newnode->next = quiz_results[index];
        }
        quiz_results[index] = newnode;

        return new_result;
    }

    std::vector<quiz_result_data*> findResultsForQuiz(const std::string& quizId) {
        std::vector<quiz_result_data*> quiz_attempts;
        for (int i = 0; i < size; ++i) {
            quiz_result_link* curr = quiz_results[i];
            while (curr != nullptr) {
                if (curr->data->quizId == quizId) {
                    quiz_attempts.push_back(curr->data);
                }
                curr = curr->next;
            }
        }
        return quiz_attempts;
    }

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