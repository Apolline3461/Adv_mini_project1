cmake_minimum_required(VERSION 3.30)
project(mini_project)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)

set(CMAKE_AUTOMOC ON)

add_executable(server server/main.cpp
        server/server.cpp
)
add_executable(client client/main.cpp
        client/client.cpp
        client/include/client.hpp)

target_link_libraries(client
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
)

