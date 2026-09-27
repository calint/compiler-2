#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ..

clang-format -i --style=file src/*
qa/lint/clang-tidy.sh
./make.sh build -O3
qa/coverage/test-coverage.sh --target=x86 run
qa/coverage/test-coverage.sh --target=rv32i run
qa/coverage/test-coverage.sh --target=rv32i-qemu run
qa/coverage/test-coverage.sh --target=rv32i-fpga run
./run.sh prog.baz --vars=131072 --checks=upper,lower,line
etc/readme/make.sh
