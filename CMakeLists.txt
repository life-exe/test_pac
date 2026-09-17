cmake_minimum_required(VERSION 3.25)

project(Pacman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(game src/main.cpp)