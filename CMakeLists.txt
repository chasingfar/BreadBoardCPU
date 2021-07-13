cmake_minimum_required(VERSION 3.13)
project(BreadBoardCPU)

set(CMAKE_CXX_STANDARD 20)
include_directories(include)

add_executable(BreadBoardCPU main.cpp)
add_executable(tests test/main.cpp
        test/opcode.cpp
        test/basic.cpp
        )