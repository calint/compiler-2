#!/bin/bash
# checks the instructions emitted for source lines marked '# folds' against
# 'tests/NNN.TARGET.folds' so constant elements stay folded at compile
#
# usage: test-constant-folding.sh [update]
#   update  rewrites the expected files from the current compiler
#
# to drop the check remove this script, the '*.folds' files and its line in
# test-all.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"
TESTS="$SCRIPT_DIR/tests"
SOURCES=(593 595)
TARGETS=(x86_64 rv32i)

MODE="${1:-check}"
if [[ "$MODE" != check && "$MODE" != update ]]; then
    echo "usage: $0 [update]" >&2
    exit 1
fi

export LLVM_PROFILE_FILE="$SCRIPT_DIR/constant-folding-%p.profraw"

# the compiler may write an image into the working directory
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# prints the instructions of each marked source line, an inline function body
# line collects the instructions of every call
extract() {
    awk -v comment="$2" '
        FNR == NR {
            if ($0 ~ /# folds$/) {
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
            group = 0
            next
        }

        # indented labels such as an inline function end also end a group,
        # the code after them belongs to the caller line
        /^[[:space:]]*[^[:space:]]+:[[:space:]]*$/ {
            group = 0
            next
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
        }
    ' "$1" -
}

for src in "${SOURCES[@]}"; do
    for target in "${TARGETS[@]}"; do
        comment=';'
        if [[ "$target" == rv32i ]]; then
            comment='#'
        fi

        expected="$TESTS/$src.$target.folds"
        echo -n "constant folding $src $target: "
        "$BIN" --target="$target" --bin=gen-rv32i.bin "$TESTS/$src.baz" >gen.s
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
