#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ../..

clang-format -i --style=file src/*
etc/readme/make.sh build
qa/lint/clang-tidy.sh
qa/coverage/run-tests-all.sh nobuild
./run-baz.sh --vars=131072 --checks=upper,lower,line
etc/readme/make.sh
