# Edumaze - A DSA-Powered Quiz Web App

Edumaze is a web-based quiz application. The entire backend logic, including user management, classroom creation, quiz handling, and data persistence, is built from scratch in C++ using the Crow micro-framework.

---

## 📚 Table of Contents
- [About The Project](#about-the-project)
- [Features](#features)
- [Core DSA Implementation](#core-dsa-implementation)
- [Tech Stack](#tech-stack)
- [Project Structure](#project-structure)
- [Setup and Installation](#setup-and-installation)
- [How It Works](#how-it-works)
- [Future Improvements](#future-improvements)

---

## 🎯 About The Project

This project simulates a real-world educational tool where teachers can create virtual classrooms, design quizzes, and track student performance. Students can join these classrooms and participate in quizzes under timed conditions. The application logic is powered by custom-built hash tables to ensure efficient data management for users, classrooms, and quizzes, and priority queue for making leaderboard demonstrating a hands-on approach to applying DSA concepts. All data is persisted in JSON format, handled via the `nlohmann/json` library.

---

## ✨ Features

### 👨‍🏫 Teacher Portal
* **Authentication:** Secure Signup and Login system for teachers.
* **Classroom Management:** Create unique classrooms, each with a randomly generated join code.
* **Quiz Creation:** Design multiple-choice quizzes with custom titles, time limits, and questions.
* **Student Progress:** View a list of students enrolled in each classroom and their quiz performance.
* **Leaderboard:** Access a leaderboard for each quiz, ranked by score and submission time.

### 🎓 Student Portal
* **Authentication:** Secure Signup and Login system for students.
* **Join Classrooms:** Enroll in classrooms using the unique code provided by the teacher.
* **View Quizzes:** See all available quizzes within joined classrooms.
* **Timed Quizzing:** Attempt quizzes within the time limit set by the teacher. The quiz auto-submits when the time is up.
* **Instant Feedback:** View the leaderboard immediately after completing a quiz to see rankings.
* **Attempt Prevention:** Students are prevented from taking the same quiz more than once.

---

## 🧠 Core DSA Implementation

The backbone of this project is the custom implementation of **Hash Tables** to manage all major entities. This choice was made to leverage the $O(1)$ average time complexity for insertions, searches, and deletions, which is crucial for a responsive web application.

### Implementation Details
* **Collision Resolution:** Collisions are handled using **Separate Chaining**. Each bucket in the hash table is a pointer to the head of a linked list. If multiple keys hash to the same index, the new element is simply added to the beginning of the list.
* **Hash Function:** The **FNV-1a (Fowler-Noll-Vo)** hash function is used for its excellent distribution properties and simplicity of implementation for string-based keys.

### Hash Tables Used
1.  **`user_hashTable` (in `Users.hpp`)**
    * **Purpose:** Manages both student and teacher data.
    * **Structure:** It internally uses three separate hash tables:
        * One for `student_data` keyed by `username`.
        * One for `teacher_data` keyed by `username`.
        * One for mapping `email` to `username` to prevent duplicate email signups.
    * **Operations:** Handles user creation, login authentication (by finding users via username), and data retrieval.

2.  **`classroom_hashTable` (in `Classroom.hpp`)**
    * **Purpose:** Manages all classrooms created by teachers.
    * **Key:** A unique, randomly generated 6-character `class_code`.
    * **Value:** A `classroom_data` struct containing the class name, subject, teacher's username, and vectors of student usernames and quiz IDs.
    * **Operations:** Efficiently adds new classrooms and finds existing ones using the class code.

3.  **`quiz_hashTable` (in `Quiz.hpp`)**
    * **Purpose:** Manages all quizzes created across the platform.
    * **Key:** A unique, randomly generated 6-character `quizId`.
    * **Value:** A `quiz_data` struct containing the quiz title, associated classroom ID, time limit, and a vector of `Question` structs.
    * **Operations:** Handles the creation and retrieval of quizzes, making it fast for both teachers to manage and students to access.

4.  **`quiz_result_hashTable` (in `QuizAttempt.hpp`)**
    * **Purpose:** Stores every individual quiz attempt made by students.
    * **Key:** A unique, randomly generated  `resultId`.
    * **Value:** A `quiz_result_data` struct containing the `quizId`, `studentUsername`, `score`, `timeTakenSeconds`, and a vector of the student's submitted answers.
    * **Operations:** This table is crucial for:
        * Persisting all quiz results.
        * Preventing re-attempts (using the `hasStudentAttempted` method).
        * Retrieving all results for a specific quiz (using `findResultsForQuiz`) to build its leaderboard.

## 📈 Other DSA Concepts Used

### 🥇Leaderboard Generation (Priority Queue)


While hash tables form the core storage, the project also demonstrates the practical use of a Priority Queue (`std::priority_queue`) for sorting quiz results efficiently.
* **Purpose:** To correctly rank and display the leaderboard for any given quiz.
* **Implementation:** When a user requests a leaderboard (at the `/quiz_leaderboard/<string>` route in `QuizAttempt.cpp`), all results for that quiz are fetched from the `quiz_result_hashTable`.
* **Custom Comparator:** These results are then pushed into a `std::priority_queue` that uses a custom comparator struct (`ResultComparator`). This comparator defines the "priority" for ranking:
    1. It first prioritizes the highest score (descending order).
    2. If two scores are tied, it breaks the tie by prioritizing the lowest time taken (ascending order).
* This ensures the $O(N \log N)$ sorting for the leaderboard is handled efficiently by the priority queue's insertion and extraction operations.
    
---

## 💻 Tech Stack

* **Backend:** **C++17**
* **Web Framework:** **Crow (C++ Micro Web Framework)** for routing, request/response handling, and middleware.
* **JSON Handling:** **nlohmann/json** for serialization and deserialization of data to/from `.json` files.
* **Frontend:** **HTML5** & **CSS3** with **Mustache** templating (via Crow).

---

## 📁 Project Structure

```
/Edumaze_Project
├── main.cpp                # Main application entry point, initializes Crow app
├── CMakeLists.txt          # Defines what to compile, where to find files, and how to link libraries. 
├── README.md               # User manual
├── include/
|   ├── Common_Route.hpp    # Header to include all route definitions
|   ├── Users.hpp           # Hash table for users (students & teachers)
|   ├── Classroom.hpp       # Hash table for classrooms
|   ├── Quiz.hpp            # Hash table for quizzes
|   ├── QuizAttempt.hpp     # Hash table for quiz results
|   └── json.hpp            # nlohmann/json library header
├── source/
|   ├── Students.cpp        # Route definitions for student dashboard
|   ├── Teachers.cpp        # Route definitions for teacher dashboard
|   ├── Classroom.cpp       # Route definitions for classroom actions
|   ├── Quiz.cpp            # Route definitions for quiz actions
|   └── QuizAttempt.cpp     # Route definitions for quiz attempt actions
├── static/
│   └── *.css               # CSS stylesheets
├── templates/
│   └── *.html              # HTML files with Mustache templates
├── Data/
│   ├── students.json       # Persisted student data
│   ├── teachers.json       # Persisted teacher data
│   ├── classrooms.json     # Persisted classroom data
│   ├── quizzes.json        # Persisted quiz data
│   └── quiz_results.json   # Persisted quiz result data

```

---

## 🚀 Setup and Installation

To get a local copy up and running, follow these simple steps.

### Prerequisites

* A C++17 compliant compiler (e.g., GCC, Clang, MSVC)
* CMake (version 3.10 or higher)

### Installation Steps

1.  **Clone the repository:**
    ```sh
    git clone (https://github.com/Abhishek140304/EDUMAZE_PROJECT.git)
    cd Edumaze_Project
    ```

2.  **Configure and build the project with CMake:**
    ```sh
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" ..
    cd..
    cmake --build ./build
    ```

3.  **Run the application:**
    ```sh
    .\build\Edumaze.exe
    ```
    The server will start, and you can access the web app at `http://localhost:18080`.

---

## ⚙️ How It Works

1.  **Authentication:** Users land on a welcome page and choose to log in as a student or teacher. If not registered, they can sign up. The `user_hashTable` validates credentials or creates new users. A session is established using Crow's middleware.
2.  **Teacher Flow:**
    * A logged-in teacher can create a classroom. This generates a unique code and stores the classroom data in the `classroom_hashTable`. The teacher's own data in the `user_hashTable` is updated with the new classroom ID.
    * The teacher then creates a quiz, providing questions, options, the correct answer, and a time limit. This quiz is stored in the `quiz_hashTable` and its ID is added to the relevant classroom.
3.  **Student Flow:**
    * A logged-in student can join a classroom using its code. This adds their username to the student list within the `classroom_data` struct.
    * The student can then view and attempt any active quizzes in their joined classrooms.
4.  **Quiz Attempt:** When a student starts a quiz, the frontend starts a timer. The student's answers are submitted to the server. The server checks the `quiz_result_hashTable` to ensure they haven't attempted it before.
5.  **Leaderboard Generation:** Upon submission, the server calculates the score and time taken, then saves the new entry to the `quiz_result_hashTable`. When any user (student or teacher) views the leaderboard, the server fetches all results for that quiz and uses the Priority Queue to efficiently rank them by score and time.

---

## 🔮 Future Improvements

-   [ ] **Database Integration:** Replace the JSON file storage with a robust database system like **SQLite** or **PostgreSQL** for better scalability and data integrity.
-   [ ] **Real-time Leaderboard:** Implement **WebSockets** to update the leaderboard in real-time as students submit their quizzes.
-   [ ] **More Question Types:** Expand beyond MCQs to include fill-in-the-blanks, true/false, and short answer questions.
-   [ ] **Enhanced Analytics:** Provide teachers with more detailed analytics on student and class performance.
-   [ ] **Containerization:** Dockerize the application for easier deployment.