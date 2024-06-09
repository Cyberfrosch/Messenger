cmake_minimum_required(VERSION 3.25.1)

project(Messenger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SERVER_SRC server.cpp)
set(SERVER_HEAD server.hpp)

set(CLIENT_SRC client.cpp)
set(CLIENT_HEAD client.hpp)

find_package(Boost 1.74.0 REQUIRED COMPONENTS system)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Failed to find boost 1.74!")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    message(STATUS "Debug mode enabled")
endif()

add_executable(server ${SERVER_SRC})
target_include_directories(server  PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(server PUBLIC Boost::system)

add_executable(client ${CLIENT_SRC})
target_include_directories(client  PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(client PUBLIC Boost::system)
