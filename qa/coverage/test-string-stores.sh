#!/bin/bash
# checks the instructions emitted for statements with string literals against
# 'tests/NNN.TARGET.stores' so short strings stay stored with immediates
#
# usage: test-string-stores.sh [update]
#   update  rewrites the expected files from the current compiler
#
# to drop the check remove this script, the '*.stores' files and its line in
# test-all.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"
TESTS="$SCRIPT_DIR/tests"
SOURCES=(557)
TARGETS=(x86_64 rv32i)

MODE="${1:-check}"
if [[ "$MODE" != check && "$MODE" != update ]]; then
    echo "usage: $0 [update]" >&2
    exit 1
fi

export LLVM_PROFILE_FILE="$SCRIPT_DIR/string-stores-%p.profraw"

# the compiler may write an image into the working directory
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# prints the instructions of each source line holding a string literal, then
# the read-only string constants with their alignment
extract() {
    awk -v comment="$2" '
        # source lines with a string literal that are not comments
        FNR == NR {
            if ($0 ~ /"/ && $0 !~ /^[[:space:]]*#/) {
                selected[FNR] = $0
                order[++count] = FNR
            }
            next
        }

        # a located comment starts or continues the group of its source line
        match($0, "^[[:space:]]*" comment "[[:space:]]*\\[[0-9]+:") {
            line = substr($0, RSTART, RLENGTH)
            sub("^[[:space:]]*" comment "[[:space:]]*\\[", "", line)
            sub(":$", "", line)
            group = line + 0
            next
        }

        # labels, directives and data at the first column end a group
        /^[^[:space:]]/ {
            if ($0 ~ /^str\.[0-9.]+:$/) {
                if (previous ~ /^[[:space:]]*\.balign/) {
                    constants = constants previous "\n"
                }
                constants = constants $0 "\n"
                after_label = 1
                previous = $0
                group = 0
                next
            }
            if (after_label) {
                constants = constants $0 "\n"
            }
            after_label = 0
            previous = $0
            group = 0
            next
        }

        {
            previous = $0
            after_label = 0
        }

        # instructions of a selected line, other comments are left out
        group in selected && $0 !~ "^[[:space:]]*" comment && $0 ~ /[^[:space:]]/ {
            code[group] = code[group] $0 "\n"
        }

        END {
            for (i = 1; i <= count; ++i) {
                n = order[i]
                printf "=== %d: %s\n%s", n, selected[n], code[n]
            }
            printf "=== read-only constants\n%s", constants
        }
    ' "$1" -
}

for src in "${SOURCES[@]}"; do
    for target in "${TARGETS[@]}"; do
        comment=';'
        if [[ "$target" == rv32i ]]; then
            comment='#'
        fi

        expected="$TESTS/$src.$target.stores"
        echo -n "string stores $src $target: "
        "$BIN" --target="$target" "$TESTS/$src.baz" >gen.s
        extract "$TESTS/$src.baz" "$comment" <gen.s >out

        if [[ "$MODE" == update ]]; then
            cp out "$expected"
            echo updated
            continue
        fi

        if cmp -s out "$expected"; then
            echo ok
            continue
        fi

        echo "FAILED. instructions differ:"
        diff "$expected" out || true
        echo "if intended, run: $0 update"
        exit 1
    done
done
