#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ../..

clang-format -i --style=file src/*
etc/readme/make.sh build
qa/lint/clang-tidy.sh
qa/coverage/run-tests-all.sh nobuild
./make.sh
etc/readme/make.sh
