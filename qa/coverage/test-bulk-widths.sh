#!/bin/bash
# checks the access width rv32i picks for each zero, copy and compare that is
# not unrolled against 'tests/NNN.rv32i.widths' so one that loses a proven
# alignment and falls back to bytes is noticed
#
# usage: test-bulk-widths.sh [update]
#   update  rewrites the expected files from the current compiler
#
# to drop the check remove this script, the '*.widths' files and its line in
# test-all.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"
TESTS="$SCRIPT_DIR/tests"
SOURCES=(590 592)

MODE="${1:-check}"
if [[ "$MODE" != check && "$MODE" != update ]]; then
    echo "usage: $0 [update]" >&2
    exit 1
fi

export LLVM_PROFILE_FILE="$SCRIPT_DIR/bulk-widths-%p.profraw"

# the compiler writes an image into the working directory
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# prints the source line of each width decision followed by the width and head
extract() {
    awk '
        FNR == NR {
            source[FNR] = $0
            sub(/^[[:space:]]+/, "", source[FNR])
            next
        }

        # the comment that decides the width names it and the head
        /# \[[0-9]+:[0-9]+\] / && match($0, /[124]-byte accesses( after a [0-9]+ B head)?/) {
            text = substr($0, RSTART, RLENGTH)
            line = $0
            sub(/^[^[]*\[/, "", line)
            sub(/:.*/, "", line)
            printf "%d: %s: %s\n", line, source[line + 0], text
        }
    ' "$1" -
}

for src in "${SOURCES[@]}"; do
    expected="$TESTS/$src.rv32i.widths"
    echo -n "bulk widths $src rv32i: "
    "$BIN" --target=rv32i --bin=gen-rv32i.bin "$TESTS/$src.baz" >gen.s
    extract "$TESTS/$src.baz" <gen.s >out

    if [[ "$MODE" == update ]]; then
        cp out "$expected"
        echo updated
        continue
    fi

    if cmp -s out "$expected"; then
        echo ok
        continue
    fi

    echo "FAILED. loop widths differ:"
    diff "$expected" out || true
    echo "if intended, run: $0 update"
    exit 1
done
