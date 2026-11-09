#!/bin/bash
# runs clang-tidy with 'clang-tidy.cfg' on '../../src/main.cpp', then the
# include check on each header, and writes its output also to 'clang-tidy.log'
# usage: qa/lint/clang-tidy.sh [fix]
#   fix  applies the fixes clang-tidy offers
set -e
set -o pipefail

cd "$(dirname "$0")"

SRC=../../src/main.cpp
if [ "$1" == "fix" ]; then
    FIX="-fix"
fi

date | tee clang-tidy.log
clang-tidy $FIX --config-file=clang-tidy.cfg --header-filter='^.*/src/.*' \
    $SRC -- -std=c++26 | tee -a clang-tidy.log

# 'misc-include-cleaner' checks only the file clang-tidy runs on, so each
# header is checked for unused includes on its own, parsed as a header
ls ../../src/*.hpp | xargs -P "$(nproc)" -I{} \
    clang-tidy $FIX --config-file=clang-tidy.cfg --checks='-*,misc-include-cleaner' \
    --quiet {} -- -std=c++26 -x c++-header 2>/dev/null | tee -a clang-tidy.log
date | tee -a clang-tidy.log

# --enable-check-profile to show time usage of the linters
