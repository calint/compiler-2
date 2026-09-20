#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

python3 "$SCRIPT_DIR/test-arena.py"

TEST_BIN="$(mktemp "${TMPDIR:-/tmp}/baz-operands.XXXXXX")"
trap 'rm -f "$TEST_BIN"' EXIT

"${CXX:-clang++}" -std=c++26 -O3 -Wno-braced-scalar-init \
    "$SCRIPT_DIR/test-operands.cpp" -o "$TEST_BIN"
"$TEST_BIN"