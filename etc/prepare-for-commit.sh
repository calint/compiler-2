#!/bin/sh
set -e
cd $(dirname "$0")

cd ..

clang-format -i src/*
qa/coverage/run-tests.sh
qa/lint/clang-tidy.sh
cp -a etc/prog.baz.bak prog.baz
./make.sh
etc/README.md.make.sh
