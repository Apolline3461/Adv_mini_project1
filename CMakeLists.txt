cmake_minimum_required(VERSION 3.30)
project(mini_project)

set(CMAKE_CXX_STANDARD 20)

add_executable(mini_project server/main.cpp
        server/server.cpp
        server/include/server.hpp
)
