#ifndef student_data_HPP
#define student_data_HPP
#include<iostream>
#include<string>
#include<vector>
#include "json.hpp"
#include <fstream> 
using json = nlohmann::json;

struct student_data {
    std::string name;
    std::string username;
    std::string email;
    std::string password;
    std::vector<std::string> classroomIds;
    
    student_data(const std:: string& thename,const std::string& theusername,const std::string& theemail,const std::string& thepassword):
    name(thename),
    username(theusername),
    email(theemail), 
    password(thepassword)
    {}
};
struct student_link{
    student_data* data=nullptr;
    student_link* next=NULL;
};

struct teacher_data {
    std::string name;
    std::string username;
    std::string email;
    std::string password;
    std::vector<std::string> classroomIds;
    
    teacher_data(const std:: string& thename,const std::string& theusername,const std::string& theemail,const std::string& thepassword):
    name(thename),
    username(theusername),
    email(theemail), 
    password(thepassword)
    {}
};
struct teacher_link{
    teacher_data* data=nullptr;
    teacher_link* next=NULL;
};

struct email_link{
    std::string* username=nullptr;
    std::string* email=nullptr;
    email_link* next=NULL;
};

void to_json(json &j, const student_data &s){
    j=json{
        {"name", s.name},
        {"email", s.email},
        {"password", s.password},
        {"username", s.username},
        {"classroomIds", s.classroomIds}
    };
}

void to_json(json &j, const teacher_data &s){
    j=json{
        {"name", s.name},
        {"email", s.email},
        {"password", s.password},
        {"username", s.username},
        {"classroomIds", s.classroomIds}
    };
}

class hashTables{
    student_link** students;
    teacher_link** teachers;
    email_link** emails;
    int size;

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

    void makeStudent_hashtable(int& size, std::ifstream &file){
        json data;
        
        file>>data;
        for(const auto& user:data){
            uint32_t index=fnv1a(user["username"])%size;
            student_data* new_user=new student_data(user["name"], user["username"],user["email"],user["password"]);
            student_link* newnode=new student_link;
            newnode->data=new_user;
            if(students[index]){
                newnode->next = students[index];
                students[index] = newnode;
            }
            else{
                students[index]=newnode;
            }

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

    void makeTeacher_hashtable(int& size, std::ifstream &file){
        json data;
        
        file>>data;
        for(const auto& user:data){
            uint32_t index=fnv1a(user["username"])%size;
            teacher_data* new_user=new teacher_data(user["name"], user["username"],user["email"],user["password"]);
            teacher_link* newnode=new teacher_link;
            newnode->data=new_user;
            if(teachers[index]){
                newnode->next = teachers[index];
                teachers[index] = newnode;
            }
            else{
                teachers[index]=newnode;
            }

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
    hashTables(){
        size=100;
        emails=new email_link*[size*2];
        students=new student_link*[size];
        teachers=new teacher_link*[size];
        for(int i=0;i<size*2;i++){
            emails[i]=nullptr;
        }
        for (int i = 0; i < size; ++i) {
            students[i] = nullptr;
            teachers[i] = nullptr;
        }

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

    student_data* findStudent(std::string& s){
        uint32_t index=fnv1a(s)%size;
        student_link* node=students[index];
        while(node){
            if(node->data->username==s) return node->data;
            node=node->next;
        }
        return nullptr;
    }

    teacher_data* findteacher(std::string& s){
        uint32_t index=fnv1a(s)%size;
        teacher_link* node=teachers[index];
        while(node){
            if(node->data->username==s) return node->data;
            node=node->next;
        }
        return nullptr;
    }

    void addStudent(student_data* new_user){
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
    }

    void addTeacher(teacher_data* new_user){
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
    }

    std::string* findUsername(std::string& theEmail){
        uint32_t index=fnv1a(theEmail)%(size*2);
        email_link* node=emails[index];
        while(node){
            if(*(node->email)==theEmail) return node->username;
            node=node->next;
        }
        return nullptr;
    }

    ~hashTables(){

        std::cout<<"Saving user data to files..."<<std::endl;

        //saving
        json students_json_array=json::array();
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

        json teachers_json_array=json::array();
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