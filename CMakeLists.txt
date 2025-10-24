cmake_minimum_required(VERSION 4.0.3)
project(dsa_project)

# Require C++17
set(CMAKE_CXX_STANDARD        17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS      OFF)

set(INCLUDE_PATHS C:/Users/lenovo/asio-1.36.0/asio-1.36.0/include C:/Crow/include)

add_executable(Edumaze
    main.cpp
    source/Classroom.cpp
    source/Quiz.cpp
    source/Students.cpp
    source/Teachers.cpp
    source/QuizAttempt.cpp
)

target_include_directories(Edumaze PUBLIC 
    ${INCLUDE_PATHS}
    ${CMAKE_SOURCE_DIR}/include
)


# On some toolchains you must link the filesystem library manually
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(Edumaze PRIVATE stdc++fs)
endif()

if (WIN32)
    target_link_libraries(Edumaze PRIVATE ws2_32 mswsock)
endif()
