#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

MACHINE=x86_64 bash "$SCRIPT_DIR/run-tests-coverage.sh" "$@"
MACHINE=rv32i bash "$SCRIPT_DIR/run-tests-coverage.sh" "$@"
bash "$SCRIPT_DIR/run-tests-cli.sh"
bash "$SCRIPT_DIR/run-tests-arena.sh"
