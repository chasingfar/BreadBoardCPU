cmake_minimum_required(VERSION 3.13)
project(BBCPU)

set(CMAKE_CXX_STANDARD 20)
include_directories(include)
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)

add_executable(BBCPU main.cpp)
add_executable(tests test/main.cpp
        test/opcode.cpp
        test/basic.cpp
        test/statement.cpp
        test/function.cpp
        )