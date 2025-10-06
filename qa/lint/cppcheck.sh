#!/bin/sh
# tools:
#   cppcheck: 2.18.3
cd $(dirname "$0")

SRC=../../src/main.cpp

date | tee cppcheck.log

cppcheck --std=c++23 --enable=all --suppress=missingIncludeSystem --check-level=exhaustive $SRC 2>&1 | tee -a cppcheck.log

date | tee -a clang-tidy.log
