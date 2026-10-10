#!/bin/sh
# builds the compiler with AddressSanitizer and UndefinedBehaviorSanitizer into
# 'build/baz-asan' for the fuzzers that look for crashes of the compiler
# usage: build-asan.sh
set -eu
cd "$(dirname "$0")"

mkdir -p build
clang++ -std=c++26 ../../src/main.cpp -o build/baz-asan -g -O1 \
    -Wno-everything \
    -fsanitize=address,undefined -fno-sanitize-recover=undefined \
    -fsanitize-address-use-after-scope -fno-omit-frame-pointer \
    -fno-optimize-sibling-calls \
    -D_GLIBCXX_ASSERTIONS -D_LIBCPP_ENABLE_ASSERTIONS=1
