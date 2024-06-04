cmake_minimum_required(VERSION 3.25.1)

project(Messenger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE server.cpp)
set(HEADER server.hpp)

find_package(Boost 1.74.0 REQUIRED COMPONENTS system)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Failed to find boost 1.74!")
endif()

add_executable(server ${SOURCE})

target_include_directories(server PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(server PUBLIC Boost::system)
