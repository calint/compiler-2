#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ../..

clang-format -i --style=file src/*
qa/lint/clang-tidy.sh
./make.sh build
qa/coverage/run-tests.sh nobuild
etc/readme/make.sh
