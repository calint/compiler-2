#!/bin/bash
# tools:
#   clang-tidy: 22.1.8
cd $(dirname "$0")

SRC=../../src/main.cpp
if [ "$1" == "fix" ]; then
    FIX="-fix"
fi

date | tee clang-tidy.log
clang-tidy $FIX --config-file=clang-tidy.cfg --header-filter='^.*/src/.*' --enable-check-profile \
    $SRC -- -std=c++26 | tee -a clang-tidy.log
date | tee -a clang-tidy.log
