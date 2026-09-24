#!/bin/sh
set -e

cd "$(dirname "$0")"

./test-coverage.sh clean
./test-coverage.sh build
./test-coverage.sh --target=x86 run
./test-coverage.sh --target=rv32i run
export LLVM_PROFILE_FILE="$PWD/extra-%p.profraw"
./test-cli.sh
./test-arena.py
./test-coverage.sh report
