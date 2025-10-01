#!/bin/sh
set -e
cd $(dirname "$0")

cd ..

cp -a etc/prog.baz.bak prog.baz
clang-format -i src/*
qa/coverage/run-tests.sh
qa/lint/clang-tidy.sh
./make.sh build
etc/README.md.make.sh
./make.sh
