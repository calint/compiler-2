#!/bin/sh
set -e
cd $(dirname "$0")

cd ..
clang-format -i src/*
cp prog.baz.bak prog.baz
qa/coverage/run-tests.sh
./make.sh
etc/README.md.make.sh
qa/lint/clang-tidy.sh
