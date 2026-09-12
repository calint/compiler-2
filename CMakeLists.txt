cmake_minimum_required(VERSION 3.25)
project(compiler-2)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 26)
add_compile_options(
    -O3
    -Werror
    -Wfatal-errors
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-pre-c++20-compat-pedantic
    -Wno-weak-vtables
    -Wno-padded
    -Wno-braced-scalar-init
)
add_executable(baz src/main.cpp)
