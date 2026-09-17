cmake_minimum_required(VERSION 3.25)

project(Pacman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)

add_executable(game src/main.cpp)

target_link_libraries(game PRIVATE nlohmann_json::nlohmann_json)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT game)
set_target_properties(game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include(cmake/ClangFormat.cmake)