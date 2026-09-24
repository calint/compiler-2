#!/bin/sh
# tools:
#   cppcheck: 2.22.0
cd $(dirname "$0")

SRC=../../src/main.cpp

date | tee cppcheck.log

cppcheck --std=c++26 --enable=all --suppress=missingIncludeSystem --check-level=exhaustive $SRC 2>&1 | tee -a cppcheck.log

date | tee -a clang-tidy.log
