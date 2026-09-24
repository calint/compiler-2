#!/bin/sh
set -e

cd "$(dirname "$0")"

./run-tests-coverage.sh clean
./run-tests-coverage.sh build
./run-tests-coverage.sh --target=x86 run
./run-tests-coverage.sh --target=rv32i run
export LLVM_PROFILE_FILE="$PWD/extra-%p.profraw"
./run-tests-cli.sh
./run-tests-arena.sh
./run-tests-rv32i.sh
./run-tests-coverage.sh report