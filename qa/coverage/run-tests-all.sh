#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

bash "$SCRIPT_DIR/run-tests-coverage.sh" "$@"
bash "$SCRIPT_DIR/run-tests-cli.sh"
bash "$SCRIPT_DIR/run-tests-arena.sh"
