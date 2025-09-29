#!/bin/sh
set -e
cd $(dirname "$0")

clang-format -i ../src/*
../qa/coverage/run-tests.sh
../make.sh
./README.md.make.sh
../qa/lint/clang-tidy.sh
