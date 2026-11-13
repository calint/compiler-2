#!/bin/sh
# runs the tests of this directory and builds the coverage report: the
# compiler cases on all four targets, then the other suites with their
# coverage profiles added to the report; of the far jumps only the short
# bodies: 'test-far-jumps.sh' without 'short' and
# 'test-far-jumps-no-scratch.sh' take unreasonable time with the sanitizers,
# they are run with the plain build
#
# usage: test-all.sh
# replaces './baz' with the coverage build; run './make.sh -O3 build' afterwards
set -e

cd "$(dirname "$0")"

# compiler cases, one run per target
./test-coverage.sh clean
./test-coverage.sh build
./test-coverage.sh --target=x86 run
./test-coverage.sh --target=rv32i run
./test-coverage.sh --target=rv32i-qemu run
./test-coverage.sh --target=rv32i-fpga run
# the same cases with the versions that the default choice drops
./test-versions.sh

# other suites
./test-cli.sh
./test-arena.py
./test-far-jumps.sh short
./test-coverage.sh report
