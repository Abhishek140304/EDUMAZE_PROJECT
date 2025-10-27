#ifndef CLASSROOM_HPP
#define CLASSROOM_HPP

/*
 * Description: This header defines the data structure for classrooms and the hash table (`classroom_hashTable`) to manage them.
 *
 * DSA Concepts:
 * 1.  **Hash Table:** `classroom_hashTable` maps a unique `class_code` (string) to the corresponding `classroom_data`. This allows for O(1)
 * average-case lookup when a student tries to join a class or a teacher views their class.
 * 2.  **Separate Chaining:** Collisions (if two class codes hash to the same index) are handled using a linked list (`classroom_link`).
 * 3.  **Hash Function:** The same `fnv1a` function is used for hashing the `class_code`.
 */

#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <any>

using njson = nlohmann::json;

// Struct to represent the data for a single classroom
struct classroom_data {
    std::string class_name;
    std::string subject;
    std::string class_code; // The primary key for the hash table
    std::string teacher_username;   // The "owner" of the class
    std::vector<std::string> student_usernames; // List of joined students
    std::vector<std::string> quizIds;   // List of quizzes in this class

    classroom_data(const std::string& theclass_name, const std::string& thesubject, const std::string& theclass_code, const std::string& theteacher_username, const std::vector<std::string>& thequizIds)
        : class_name(theclass_name), subject(thesubject), class_code(theclass_code), teacher_username(theteacher_username), quizIds(thequizIds) {}
};

// Linked list node for the classroom hash table (separate chaining)
struct classroom_link {
    classroom_data* data = nullptr;
    classroom_link* next = nullptr;
};

void to_json(njson& j, const classroom_data& c);


// Description: Implements a hash table to store all classroom data.
// The key is the `class_code`.
class classroom_hashTable {
private:
    classroom_link** classrooms;
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
    
    // Generates a random 6-character string for the class code
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

    // Loads classroom data from JSON file and populates the hash table
    void makeClassrooms_hashtable(int& table_size, std::ifstream& file) {
        njson data;

        file>>data;
        for (const auto& room : data) {
            // 1. Extract data from JSON
            std::string class_name = room.value("class_name", "");
            std::string subject = room.value("subject", "");
            std::string class_code = room.value("class_code", "");
            std::string teacher_username = room.value("teacher_username", "");
            std::vector<std::string> quizIds = room.value("quizIds", std::vector<std::string>{});
            std::vector<std::string> student_usernames = room.value("student_usernames", std::vector<std::string>{});

            // 2. Hash the key (class_code)
            uint32_t index = fnv1a(class_code) % table_size;

            // 3. Create the data object
            classroom_data* new_room = new classroom_data(class_name, subject, class_code, teacher_username, quizIds);
            new_room->student_usernames = student_usernames;

            // 4. Insert into hash table (add to front of list)
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
    // Constructor: Initializes and populates the hash table
    classroom_hashTable() {
        size = 50;
        classrooms = new classroom_link*[size];

        // Initialize all list heads to nullptr
        for(int i=0; i<size; i++){
            classrooms[i]=nullptr;
        }
        std::ifstream classroomFile("Data/classrooms.json");
        if (classroomFile.is_open()) {
            makeClassrooms_hashtable(size, classroomFile);
        } else {
            // If file doesn't exist, create an empty one
            std::ofstream newFile("Data/classrooms.json");
            newFile << "[]";
            newFile.close();
        }
    }

    // Saves all classroom data back to the JSON file
    void saveClassroomsToFile() {
        njson classrooms_json_array = njson::array();
        // Traverse the entire hash table array
        for (int i = 0; i < size; ++i) {
            classroom_link* curr = classrooms[i];
            // Traverse the linked list at this index
            while (curr != nullptr) {
                classrooms_json_array.push_back(*(curr->data));
                classroom_link* next = curr->next;
                curr = next;
            }
        }

        std::ofstream classroomFile("Data/classrooms.json");
        classroomFile << classrooms_json_array.dump(4);
        classroomFile.close();
    }


    /*
     * Creates a new classroom, adds it to the hash table, and returns the code.
     * Time Complexity: O(1) average. (Generation + Hash + Insertion)
     * Note: In a very rare case, `generate_class_code` could create a duplicate. A robust implementation would check for this and regenerate, but for this project, the collision chance is negligible.
     */
    std::string addClassroom(const std::string& name, const std::string& subject, teacher_data* teacher) {
        std::string code = generate_class_code();
        uint32_t index = fnv1a(code) % size;

        // Create new data and node
        classroom_data* new_room_data = new classroom_data(name, subject, code, teacher->username,{});
        classroom_link* newnode = new classroom_link;
        newnode->data=new_room_data;

        // Insert at the head of the linked list
        if (classrooms[index]) {
            newnode->next = classrooms[index];
        }
        classrooms[index] = newnode;

        return code;
    }

    /*
     * Finds a classroom by its class code.
     * Time Complexity:
     * - Average: O(1).
     * - Worst: O(n), where n is the total number of classrooms.
     */
    classroom_data* findClassroom(const std::string& code) {
        uint32_t index = fnv1a(code) % size;
        classroom_link* node = classrooms[index];
        // Traverse the linked list at the calculated index
        while (node) {
            if (node->data->class_code == code) return node->data;
            node = node->next;
        }
        return nullptr;
    }

    // Destructor: Saves data and deallocates all memory
    ~classroom_hashTable() {
        std::cout << "Saving classroom data to file..." << std::endl;

        njson classrooms_json_array = njson::array();
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