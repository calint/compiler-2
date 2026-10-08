#!/bin/sh
set -eu

cd $(dirname "$0")

SRC=../../src/main.cpp

date | tee cppcheck.log

cppcheck --std=c++26 --enable=all --suppress=missingIncludeSystem --check-level=exhaustive $SRC 2>&1 | tee -a cppcheck.log

date | tee -a cppcheck.log
