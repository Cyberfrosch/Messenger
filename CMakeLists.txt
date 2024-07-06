cmake_minimum_required(VERSION 3.25.1)

project(Messenger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BIN_DIR ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${BIN_DIR})

set(COMMON_DIR common)
set(COMMON_INCLUDE_DIR ${COMMON_DIR}/include())

set(SERVER_DIR server)
set(SERVER_SRC_DIR ${SERVER_DIR}/src)
set(SERVER_INCLUDE_DIR ${SERVER_DIR}/include)

set(CLIENT_DIR client)
set(CLIENT_SRC_DIR ${CLIENT_DIR}/src)
set(CLIENT_INCLUDE_DIR ${CLIENT_DIR}/include)

set(SERVER_SRC
     ${SERVER_SRC_DIR}/database.cpp
     ${SERVER_SRC_DIR}/server.cpp
     ${SERVER_SRC_DIR}/main.cpp)

set(CLIENT_SRC
     ${CLIENT_SRC_DIR}/client.cpp
     ${CLIENT_SRC_DIR}/main.cpp)

find_package(Boost 1.74.0 REQUIRED COMPONENTS system)
if(NOT Boost_FOUND)
     message(FATAL_ERROR "Failed to find boost 1.74!")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
     add_compile_definitions(DEBUG)
     message(STATUS "Debug mode enabled")
endif()

add_executable(server ${SERVER_SRC})
target_include_directories(server PUBLIC
     ${Boost_INCLUDE_DIR}
     ${PQXX_INCLUDE_DIRS}
     ${COMMON_INCLUDE_DIR}
     ${SERVER_INCLUDE_DIR})

target_link_libraries(server PUBLIC Boost::system ${PQXX_LIBRARIES})
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})

add_executable(client ${CLIENT_SRC})
target_include_directories(client PUBLIC
     ${Boost_INCLUDE_DIR}
     ${COMMON_INCLUDE_DIR}
     ${CLIENT_INCLUDE_DIR})

target_link_libraries(client PUBLIC Boost::system)
set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
