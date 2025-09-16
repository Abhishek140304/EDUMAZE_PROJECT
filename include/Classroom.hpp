#ifndef CLASSROOM_HPP
#define CLASSROOM_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <sstream>
#include "json.hpp"
#include "users.hpp"

using json = nlohmann::json;

struct classroom_data {
    std::string class_name;
    std::string subject;
    std::string class_code;
    std::string teacher_username;
    std::vector<std::string> student_usernames;
    std::vector<std::string> quizIds;

    classroom_data(const std::string& theclass_name, const std::string& thesubject, const std::string& theclass_code, const std::string& theteacher_username, const std::vector<std::string>& thequizIds)
        : class_name(theclass_name), subject(thesubject), class_code(theclass_code), teacher_username(theteacher_username), quizIds(thequizIds) {}
};

struct classroom_link {
    classroom_data* data = nullptr;
    classroom_link* next = nullptr;
};

void to_json(json& j, const classroom_data& c) {
    j = json{
        {"class_name", c.class_name},
        {"subject", c.subject},
        {"class_code", c.class_code},
        {"teacher_username", c.teacher_username},
        {"student_usernames", c.student_usernames},
        {"quizIds", c.quizIds}
    };
}

class classroom_hashTable {
private:
    classroom_link** classrooms;
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
    
    std::string generate_class_code() {
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

    void makeClassrooms_hashtable(int& table_size, std::ifstream& file) {
        json data;

        file>>data;
        for (const auto& room : data) {
            std::string class_name = room.value("class_name", "");
            std::string subject = room.value("subject", "");
            std::string class_code = room.value("class_code", "");
            std::string teacher_username = room.value("teacher_username", "");
            std::vector<std::string> quizIds = room.value("quizIds", std::vector<std::string>{});
            std::vector<std::string> student_usernames = room.value("student_usernames", std::vector<std::string>{});

            uint32_t index = fnv1a(class_code) % table_size;
            classroom_data* new_room = new classroom_data(class_name, subject, class_code, teacher_username, quizIds);
            new_room->student_usernames = student_usernames;

            classroom_link* newnode = new classroom_link;
            newnode->data=new_room;
            if (classrooms[index]) {
                newnode->next = classrooms[index];
                classrooms[index]=newnode;
            }
            else{
                classrooms[index] = newnode;
            }
        }
    }

public:
    classroom_hashTable() {
        size = 50;
        classrooms = new classroom_link*[size];

        for(int i=0; i<size; i++){
            classrooms[i]=nullptr;
        }
        std::ifstream classroomFile("Data/classrooms.json");
        if (classroomFile.is_open()) {
            makeClassrooms_hashtable(size, classroomFile);
        } else {
            std::ofstream newFile("Data/classrooms.json");
            newFile << "[]";
            newFile.close();
        }
    }


    std::string addClassroom(const std::string& name, const std::string& subject, teacher_data* teacher) {
        std::string code = generate_class_code();
        uint32_t index = fnv1a(code) % size;

        classroom_data* new_room_data = new classroom_data(name, subject, code, teacher->username,{});
        classroom_link* newnode = new classroom_link;
        newnode->data=new_room_data;

        if (classrooms[index]) {
            newnode->next = classrooms[index];
        }
        classrooms[index] = newnode;

        teacher->classroomIds.push_back(code);

        return code;
    }

    classroom_data* findClassroom(const std::string& code) {
        uint32_t index = fnv1a(code) % size;
        classroom_link* node = classrooms[index];
        while (node) {
            if (node->data->class_code == code) return node->data;
            node = node->next;
        }
        return nullptr;
    }

    ~classroom_hashTable() {
        std::cout << "Saving classroom data to file..." << std::endl;

        json classrooms_json_array = json::array();
        for (int i = 0; i < size; ++i) {
            classroom_link* curr = classrooms[i];
            while (curr != nullptr) {
                classrooms_json_array.push_back(*(curr->data));
                classroom_link* next = curr->next;
                delete curr->data;
                delete curr;
                curr = next;
            }
        }
        std::ofstream classroomFile("Data/classrooms.json");
        classroomFile << classrooms_json_array.dump(4);
        classroomFile.close();

        delete[] classrooms;
    }
};

#endif