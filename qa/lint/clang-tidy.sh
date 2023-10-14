#!/bin/bash
# tools:
#   clang-tidy: 15.0.7

# change to directory of the script
cd $(dirname "$0")

SRC=../../src/main.cpp
if [ "$1" == "fix" ]; then
    FIX=-fix
fi

date | tee clang-tidy.log
clang-tidy $FIX --config-file=clang-tidy.cfg -header-filter=.* $SRC -- -std=c++20 | tee -a clang-tidy.log
date | tee -a clang-tidy.log

