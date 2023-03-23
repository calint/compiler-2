cmake_minimum_required(VERSION 3.0)
project(compiler-2)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 20)
add_compile_options(
    -Os
    -fno-inline
    -Werror
    -Wfatal-errors
    -Weverything
    -Wno-c++98-compat
    -Wno-weak-vtables
    -Wno-unqualified-std-cast-call
    -Wno-padded
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-private-field
)
add_executable(baz src/main.cpp)
