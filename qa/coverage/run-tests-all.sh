#!/bin/sh
set -e

cd "$(dirname "$0")"

MACHINE=x86_64 ./run-tests-coverage.sh
MACHINE=rv32i SKIP_BUILD=1 ./run-tests-coverage.sh
./run-tests-cli.sh
./run-tests-arena.sh
