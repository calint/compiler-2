cmake_minimum_required(VERSION 3.5)
project(compiler_2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES
    src/call_asm.hpp
    src/call_asm_int.hpp
    src/call_asm_mov.hpp
    src/call_asm_syscall.hpp
    src/call_asm_xor.hpp
    src/compiler_error.hpp
    src/decouple.hpp
    src/expr_ops_list.hpp
    src/expression.hpp
    src/lut.hpp
    src/main.cpp
    src/statement.hpp
    src/stmt_assign_var.hpp
    src/stmt_block.hpp
    src/stmt_break.hpp
    src/stmt_call.hpp
    src/stmt_comment.hpp
    src/stmt_continue.hpp
    src/stmt_def_field.hpp
    src/stmt_def_func.hpp
    src/stmt_def_func_param.hpp
    src/stmt_def_table.hpp
    src/stmt_def_var.hpp
    src/stmt_if.hpp
    src/stmt_if_branch.hpp
    src/stmt_loop.hpp
    src/stmt_program.hpp
    src/toc.hpp
    src/token.hpp
    src/tokenizer.hpp
    )

add_executable(compiler_2 ${SOURCE_FILES})