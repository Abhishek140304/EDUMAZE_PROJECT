#ifndef QUIZ_HPP
#define QUIZ_HPP

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

struct Question{
    std::string questionText;
    std::vector<std::string> options;
    int correctAnswerIndex;
};

struct quiz_data{
    std::string quizId;
    std::string quizTitle;
    std::string classroomId;
    int timeLimitMins;
    std::vector<Question> questions;

    quiz_data() = default;

    quiz_data(const std::string& thequizId, const std::string& thequizTitle, const std::string& theclassroomId, int thetimeLimitMins, const std::vector<Question>& thequestions):
    quizId(thequizId), quizTitle(thequizTitle), classroomId(theclassroomId), timeLimitMins(thetimeLimitMins), questions(thequestions){}

};

struct quiz_link{
    quiz_data* data=nullptr;
    quiz_link* next=nullptr;
};

void to_json(njson& j, const Question& q);

void from_json(const njson& j, Question& q);

void to_json(njson& j, const quiz_data& q);

void from_json(const njson& j, quiz_data& q);


class quiz_hashTable {
private:
    quiz_link** quizzes;
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

    void makeQuizzes_hashtable(int& table_size, std::ifstream& file) {
        njson data;

        file>>data;
        for (const auto& quiz_json : data) {
            quiz_data temp_quiz;

            temp_quiz.quizId = quiz_json.value("quizId", "");
            temp_quiz.quizTitle = quiz_json.value("quizTitle", "");
            temp_quiz.classroomId = quiz_json.value("classroomId", "");
            temp_quiz.timeLimitMins = quiz_json.value("timeLimitMinutes", 0);
            temp_quiz.questions = quiz_json.value("questions", std::vector<Question>{});

            quiz_data* new_quiz = new quiz_data(temp_quiz);
            uint32_t index = fnv1a(new_quiz->quizId) % table_size;

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

    quiz_data* createQuiz(const std::string& title, const std::string& classroomId, int timeLimit, const std::vector<Question>& questions) {
        std::string id = generate_quiz_id();
        uint32_t index = fnv1a(id) % size;

        quiz_data* new_quiz_data  = new quiz_data(id, title, classroomId, timeLimit, questions);
        quiz_link* newnode = new quiz_link;
        newnode->data=new_quiz_data;

        if (quizzes[index]) {
            newnode->next = quizzes[index];
        }
        quizzes[index] = newnode;

        return new_quiz_data;
    }

    quiz_data* findQuiz(const std::string& quizId) {
        uint32_t index = fnv1a(quizId) % size;
        quiz_link* node = quizzes[index];
        while (node) {
            if (node->data->quizId  == quizId) return node->data;
            node = node->next;
        }
        return nullptr;
    }

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