#!/bin/sh
# runs every test of this directory and builds the coverage report: the
# compiler cases on all four targets, then the other suites with their
# coverage profiles added to the report
set -e

cd "$(dirname "$0")"

# compiler cases, one run per target
./test-coverage.sh clean
./test-coverage.sh build
./test-coverage.sh --target=x86 run
./test-coverage.sh --target=rv32i run
./test-coverage.sh --target=rv32i-qemu run
./test-coverage.sh --target=rv32i-fpga run

# other suites
export LLVM_PROFILE_FILE="$PWD/extra-%p.profraw"
./test-cli.sh
./test-arena.py
./test-rv32i.sh
./test-string-stores.sh
./test-bulk-widths.sh
./test-constant-folding.sh
./test-equal-compares.sh
./test-deduced-types.sh
./test-coverage.sh report