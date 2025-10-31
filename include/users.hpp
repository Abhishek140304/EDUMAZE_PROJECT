#ifndef USERS_HPP
#define USERS_HPP

/*
 * Description: This header file defines the data structures for users (students and teachers and the hash table class (`user_hashTable`) to manage them.
 * DSA Concepts:
 * 1.  **Hash Table:** The `user_hashTable` class is a hash table. This is chosen for its excellent average-case time complexity for lookups, insertions, and deletions, which is O(1).
 * 2.  **Separate Chaining:** The hash table resolves collisions using separate chaining. Each index in the `students`, `teachers`, and `emails` arrays is a pointer to the head of a linked list (`student_link`, `teacher_link`, `email_link`).
 * 3.  **Hash Function:** A custom hash function (`fnv1a`) is used to map string keys (like username and email) to an integer index in the table.
 * 4.  **Linked List:** The `_link` structs act as nodes in a singly linked list.
 
 */

#include<iostream>
#include<string>
#include<vector>
#include "json.hpp"
#include <fstream> 
using njson = nlohmann::json;

// Struct to represent the data for a single student
struct student_data {
    std::string name;
    std::string username;   // Primary key for the student hash table
    std::string email;  // Primary key for the email hash table
    std::string password;
    std::vector<std::string> classroomIds;  // Stores codes of all joined classrooms
    
    student_data(const std:: string& thename,const std::string& theusername,const std::string& theemail,const std::string& thepassword, const std::vector<std::string>& theclassroomIds={}):
    name(thename),
    username(theusername),
    email(theemail), 
    password(thepassword),
    classroomIds(theclassroomIds)
    {}
};

// Represents a node in the linked list for the student hash table (for separate chaining)
struct student_link{
    student_data* data=nullptr;
    student_link* next=NULL;
};

// Struct to represent the data for a single teacher
struct teacher_data {
    std::string name;
    std::string username;
    std::string email;
    std::string password;
    std::vector<std::string> classroomIds;  // Stores codes of all created classrooms
    
    teacher_data(const std:: string& thename,const std::string& theusername,const std::string& theemail,const std::string& thepassword, const std::vector<std::string>& theclassroomIds={}):
    name(thename),
    username(theusername),
    email(theemail), 
    password(thepassword),
    classroomIds(theclassroomIds)
    {}
};

// Represents a node in the linked list for the teacher hash table (for separate chaining)
struct teacher_link{
    teacher_data* data=nullptr;
    teacher_link* next=NULL;
};

/*
 * This struct is for a separate hash table used for email lookups.
 * It allows for O(1) average-case checking if an email is already in use and finding a username by email.
 */
struct email_link{
    std::string* username=nullptr;
    std::string* email=nullptr;
    email_link* next=NULL;
};

void to_json(njson &j, const student_data &s);

void to_json(njson &j, const teacher_data &s);

/*
 * Class: user_hashTable
 *
 * Description: Implements a hash table to store all user data.
 * It maintains three separate hash tables internally:
 * 1. `students`: Maps `username` to `student_data`
 * 2. `teachers`: Maps `username` to `teacher_data`
 * 3. `emails`:   Maps `email` to `username` (for quick email existence checks)
 */
class user_hashTable{
    student_link** students;    // Array of pointers to student linked lists
    teacher_link** teachers;  // Array of pointers to teacher linked lists  
    email_link** emails;    // Array of pointers to email linked lists
    int size;


    // FNV-1a hash function.
    uint32_t fnv1a(std::string s){
        const uint32_t bais=2166136261u;
        const uint32_t prime=16777619u;
        uint32_t hash=bais;
        for(unsigned char c:s){
            hash^=c;
            hash*=prime;
        }
        return hash;
    }

    // Loads student data from JSON file and populates the hash tables
    void makeStudent_hashtable(int& size, std::ifstream &file){
        njson data;
        
        file>>data;
        for(const auto& user:data){
            // 1. Create the student_data object
            uint32_t index=fnv1a(user["username"])%size;

            std::vector<std::string> classrooms;
            if(user.contains("classroomIds") && user["classroomIds"].is_array()){
                classrooms = user["classroomIds"].get<std::vector<std::string>>();
            }

            student_data* new_user=new student_data(user["name"], user["username"],user["email"],user["password"], classrooms);
            
            // 2. Insert into the `students` hash table (using separate chaining)
            student_link* newnode=new student_link;
            newnode->data=new_user;
            // Add to the front of the linked list at this index
            if(students[index]){
                newnode->next = students[index];
                students[index] = newnode;
            }
            else{
                students[index]=newnode;
            }

            // 3. Insert into the `emails` hash table
            index=fnv1a(user["email"])%(size*2);
            std::string* new_email=new std::string(user["email"]);
            std::string* new_username=new std::string(user["username"]);
            email_link* email_node=new email_link;
            email_node->email=new_email;
            email_node->username=new_username;
            // Add to the front of the linked list
            if(emails[index]){
                email_node->next=emails[index];
                emails[index]=email_node;
            }
            else{
                emails[index]=email_node;
            }
        }
    }

    // Loads teacher data from JSON file and populates the hash tables
    void makeTeacher_hashtable(int& size, std::ifstream &file){
        njson data;
        
        file>>data;
        for(const auto& user:data){
            // 1. Create the teacher_data object
            uint32_t index=fnv1a(user["username"])%size;

            std::vector<std::string> classrooms;
            if(user.contains("classroomIds") && user["classroomIds"].is_array()){
                classrooms = user["classroomIds"].get<std::vector<std::string>>();
            }
            teacher_data* new_user=new teacher_data(user["name"], user["username"],user["email"],user["password"], classrooms);

            // 2. Insert into the `teachers` hash table
            teacher_link* newnode=new teacher_link;
            newnode->data=new_user;
            if(teachers[index]){
                newnode->next = teachers[index];
                teachers[index] = newnode;
            }
            else{
                teachers[index]=newnode;
            }

            // 3. Insert into the `emails` hash table
            index=fnv1a(user["email"])%(size*2);
            std::string* new_email=new std::string(user["email"]);
            std::string* new_username=new std::string(user["username"]);
            email_link* email_node=new email_link;
            email_node->email=new_email;
            email_node->username=new_username;
            if(emails[index]){
                email_node->next=emails[index];
                emails[index]=email_node;
            }
            else{
                emails[index]=email_node;
            }
        }
    }

public:
    // Constructor: Initializes and populates the hash tables from files
    user_hashTable(){
        size=100;
        // Allocate memory for the arrays of linked list heads
        emails=new email_link*[size*2];
        students=new student_link*[size];
        teachers=new teacher_link*[size];

        // Initialize all heads to nullptr
        for(int i=0;i<size*2;i++){
            emails[i]=nullptr;
        }
        for (int i = 0; i < size; ++i) {
            students[i] = nullptr;
            teachers[i] = nullptr;
        }

        // Open and read data files
        std::ifstream studentFile("Data/students.json");
        if(!studentFile.is_open()){
            throw std::runtime_error("Could not open students.json");
        }

        std::ifstream teacherFile("Data/teachers.json");
        if(!teacherFile.is_open()){
            throw std::runtime_error("Could not open teachers.json");
        }

        makeStudent_hashtable(size, studentFile);
        makeTeacher_hashtable(size, teacherFile);
    }


    /*
     * Finds a student by username.
     * Time Complexity:
     * - Average: O(1). Hash function computes index, direct lookup.
     * - Worst: O(n), where n is the number of students. This happens if
     * all students hash to the same index (a very bad hash function or
     * extreme bad luck).
     */
    student_data* findStudent(const std::string& s){
        uint32_t index=fnv1a(s)%size;
        student_link* node=students[index];
        // Traverse the linked list at this index
        while(node){
            if(node->data->username==s) return node->data;
            node=node->next;
        }
        return nullptr; // Not found
    }

    // Finds a teacher by username. Same O(1) average complexity.
    teacher_data* findTeacher(std::string& s){
        uint32_t index=fnv1a(s)%size;
        teacher_link* node=teachers[index];
        while(node){
            if(node->data->username==s) return node->data;
            node=node->next;
        }
        return nullptr;
    }

    /*
     * Adds a new student to the hash tables.
     * Time Complexity: O(1) average. Involves two hash calculations
     * and two O(1) linked list insertions (at the head).
     */
    void addStudent(student_data* new_user){
        // Add to `students` table
        uint32_t index=fnv1a(new_user->username)%size;
        student_link* newnode=new student_link;
        newnode->data=new_user;
        if(students[index]){
            newnode->next = students[index];
            students[index] = newnode;
        }
        else{
            students[index]=newnode;
        }

        // Add to `emails` table
        index=fnv1a(new_user->email)%(size*2);
        std::string* new_email=new std::string(new_user->email);
        std::string* new_username=new std::string(new_user->username);
        email_link* email_node=new email_link;
        email_node->email=new_email;
        email_node->username=new_username;
        if(emails[index]){
            email_node->next=emails[index];
            emails[index]=email_node;
        }
        else{
            emails[index]=email_node;
        }
        saveStudentsToFile();   // Persist change
    }

    // Adds a new teacher to the hash tables. O(1) average complexity.
    void addTeacher(teacher_data* new_user){
        // Add to `teachers` table
        uint32_t index=fnv1a(new_user->username)%size;
        teacher_link* newnode=new teacher_link;
        newnode->data=new_user;
        if(teachers[index]){
            newnode->next = teachers[index];
            teachers[index] = newnode;
        }
        else{
            teachers[index]=newnode;
        }

        // Add to `emails` table
        index=fnv1a(new_user->email)%(size*2);
        std::string* new_email=new std::string(new_user->email);
        std::string* new_username=new std::string(new_user->username);
        email_link* email_node=new email_link;
        email_node->email=new_email;
        email_node->username=new_username;
        if(emails[index]){
            email_node->next=emails[index];
            emails[index]=email_node;
        }
        else{
            emails[index]=email_node;
        }
        saveTeachersToFile();   // Persist change
    }

    /*
     * Finds a username by email. Used for checking if email is taken.
     * Time Complexity: O(1) average.
     */
    std::string* findUsername(std::string& theEmail){
        uint32_t index=fnv1a(theEmail)%(size*2);
        email_link* node=emails[index];
        while(node){
            if(*(node->email)==theEmail) return node->username;
            node=node->next;
        }
        return nullptr;
    }

    // Saves all student data back to the JSON file
    void saveStudentsToFile(){
        njson j = njson::array();
        // Traverse the entire hash table array
        for(int i=0;i<size;i++){ student_link* curr=students[i]; 
            // Traverse each linked list
            while(curr){ 
                j.push_back(*(curr->data)); curr=curr->next;
            } 
        }
        std::ofstream("Data/students.json") << j.dump(4);
    }

    // Saves all teacher data back to the JSON file
    void saveTeachersToFile(){
        njson j = njson::array();
        for(int i=0;i<size;i++){ teacher_link* curr=teachers[i]; 
            while(curr){
                j.push_back(*(curr->data)); curr=curr->next; 
            } 
        }
        std::ofstream("Data/teachers.json") << j.dump(4);
    }


    // Destructor: Cleans up all dynamically allocated memory
    ~user_hashTable(){

        std::cout<<"Saving user data to files..."<<std::endl;

        //saving
        njson students_json_array=njson::array();
        for(int i=0; i<size; ++i){
            student_link* curr=students[i];
            while(curr!=nullptr){
                students_json_array.push_back(*(curr->data));
                curr=curr->next;
            }
        }
        std::ofstream studentFile("Data/students.json");
        studentFile<<students_json_array.dump(4);
        studentFile.close();

        njson teachers_json_array=njson::array();
        for(int i=0; i<size; ++i){
            teacher_link* curr=teachers[i];
            while(curr!=nullptr){
                teachers_json_array.push_back(*(curr->data));
                curr=curr->next;
            }
        }
        std::ofstream teacherFile("Data/teachers.json");
        teacherFile<<teachers_json_array.dump(4);
        teacherFile.close();

        // Deallocate students and teachers tables
        for(int i=0;i<size;i++){
            student_link* curr1=students[i];
            while(curr1){
                student_link* next=curr1->next;
                delete curr1->data;
                delete curr1;
                curr1=next;
            }
            teacher_link* curr2=teachers[i];
            while(curr2){
                teacher_link* next=curr2->next;
                delete curr2->data;
                delete curr2;
                curr2=next;
            }
        }
        delete[] students;
        delete[] teachers;

        // Deallocate emails table
        for(int i=0;i<size*2;i++){
            email_link* curr=emails[i];
            while(curr){
                email_link* next=curr->next;
                delete curr->email;
                delete curr->username;
                delete curr;
                curr=next;
            }
        }
        delete[] emails;
    }
};


#endif
