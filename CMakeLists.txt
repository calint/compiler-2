cmake_minimum_required(VERSION 3.10)
project(compiler-2)
find_program(NASM nasm REQUIRED)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 23)
add_compile_options(
    -O3
    -Werror
    -Wfatal-errors
    -Weverything
    -Wno-c++98-compat
    -Wno-pre-c++20-compat-pedantic
    -Wno-weak-vtables
    -Wno-padded
    -Wno-unsafe-buffer-usage
)
add_executable(baz src/main.cpp)
