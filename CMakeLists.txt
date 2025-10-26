cmake_minimum_required(VERSION 3.10)
project(Edumaze)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(ASIO_INCLUDE_DIR NAMES asio.hpp PATHS "${CMAKE_PREFIX_PATH}")
find_path(CROW_INCLUDE_DIR NAMES crow.h PATHS "${CMAKE_PREFIX_PATH}")

add_executable(Edumaze
    main.cpp
    source/Classroom.cpp
    source/Quiz.cpp
    source/Students.cpp
    source/Teachers.cpp
    source/QuizAttempt.cpp
)

target_include_directories(Edumaze PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${ASIO_INCLUDE_DIR}
    ${CROW_INCLUDE_DIR}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Edumaze PRIVATE stdc++fs)
endif()