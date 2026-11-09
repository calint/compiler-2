#!/bin/bash
# runs cppcheck with every check on '../../src/main.cpp' and writes its output
# also to 'cppcheck.log'
# usage: qa/lint/cppcheck.sh
set -eu
set -o pipefail

cd "$(dirname "$0")"

SRC=../../src/main.cpp

date | tee cppcheck.log

cppcheck --std=c++26 --enable=all --suppress=missingIncludeSystem --check-level=exhaustive $SRC 2>&1 | tee -a cppcheck.log

date | tee -a cppcheck.log
