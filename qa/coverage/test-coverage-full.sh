#!/bin/sh
# like 'test-all.sh' for the targets that need no qemu system or fpga emulator:
# the compiler cases on x86_64 and rv32i, then 'test-cli.sh' and
# 'test-arena.py'
set -e

cd "$(dirname "$0")"

./test-coverage.sh clean
./test-coverage.sh build
./test-coverage.sh --target=x86 run
./test-coverage.sh --target=rv32i run
./test-cli.sh
./test-arena.py
./test-coverage.sh report
