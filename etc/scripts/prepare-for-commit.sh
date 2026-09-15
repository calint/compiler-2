#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ../..

clang-format -i --style=file src/*
qa/lint/clang-tidy.sh
qa/coverage/run-tests.sh
./make.sh
etc/readme/make.sh
