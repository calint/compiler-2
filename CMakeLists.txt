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
    -Wno-weak-vtables
    -Wno-padded
    -Wno-unqualified-std-cast-call
    -Wno-unsafe-buffer-usage
)
add_executable(baz src/main.cpp)
add_custom_command(TARGET baz POST_BUILD
    COMMAND echo "* compiling prog.baz to gen.s"
    COMMAND ../baz ../prog.baz > gen.s
#    COMMAND grep -v -e'^\\s*;.*$' -e'^\\s*$' gen.s > gen-m.s
    COMMAND echo "* compiling gen.s with ${NASM} to executable 'gen'"
    COMMAND ${NASM} -f elf64 gen.s
    COMMAND ld -s -o gen gen.o
    COMMAND echo
    COMMAND ls --color -la baz gen
    COMMAND rm -f gen.o diff.baz
    COMMAND echo
    COMMAND echo "* running gen"
    COMMAND echo "---------------------------------------------------------"
    COMMAND ./gen
    COMMAND echo "---------------------------------------------------------"
)
