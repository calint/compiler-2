cmake_minimum_required(VERSION 3.0)
project(compiler-2)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 20)
add_compile_options(
    -Os
#    -fno-inline # without this option the executable is smaller (?)
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
#add_custom_command(TARGET baz POST_BUILD
#    COMMAND echo "* compiling prog.baz to gen.s"
#    COMMAND ./baz > gen.s
#    COMMAND rm diff.baz
#    COMMAND grep -v -e'^\s*;.*$' -e'^\s*$' gen.s > gen-m.s
#    COMMAND echo "* compiling gen.s with nasm to executable gen"
#    COMMAND nasm -f elf64 gen.s
#    COMMAND ld -s -o gen gen.o
#    COMMAND ls --color -la baz gen.s gen
#    COMMAND rm -f gen.o diff.baz
#    COMMAND echo "* running gen"
#    COMMAND ./gen
#)
