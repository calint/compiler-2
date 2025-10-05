#!/bin/sh
# tools:
#   cppcheck: 2.10
cd $(dirname "$0")

SRC=../../src/main.cpp

date | tee cppcheck.log
#    --suppress=knownConditionTrueFalse \
cppcheck --enable=all --inline-suppr \
  --suppress=missingIncludeSystem \
  $SRC 2>&1 | tee -a cppcheck.log
date | tee -a clang-tidy.log
