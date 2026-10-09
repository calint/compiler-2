#!/bin/bash
# checks the x86_64 string compares emitted for '==' and '!=' on source lines marked
# '# compares' against 'tests/NNN.x86_64.compares' so a known size keeps
# 'repe cmpsq' for more than 2 qwords and single compares for the rest
#
# usage: test-equal-compares.sh [update]
#   update  rewrites the expected files from the current compiler
#
# to drop the check remove this script, the '*.compares' files and its line in
# test-all.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"
TESTS="$SCRIPT_DIR/tests"
SOURCES=(0594)

MODE="${1:-check}"
if [[ "$MODE" != check && "$MODE" != update ]]; then
    echo "usage: $0 [update]" >&2
    exit 1
fi

export LLVM_PROFILE_FILE="$SCRIPT_DIR/equal-compares-%p.profraw"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# prints the compare instructions from each marked line's comparison comment to
# the instruction storing the result; label numbers depend on earlier code, so
# they are left out
extract() {
    awk '
        FNR == NR {
            if ($0 ~ /# compares$/) {
                selected[FNR] = $0
                order[++count] = FNR
            }
            next
        }

        match($0, /^[[:space:]]*;[[:space:]]*\[[0-9]+:[0-9]+\] \? .*(==|!=) /) {
            line = $0
            sub(/^[[:space:]]*;[[:space:]]*\[/, "", line)
            sub(/:.*/, "", line)
            group = (line + 0) in selected ? line + 0 : 0
            next
        }

        group == 0 || /^[[:space:]]*;/ {
            next
        }

        {
            text = $0
            sub(/^[[:space:]]+/, "", text)
            gsub(/\.Lbaz_equal\.[0-9]+/, ".Lbaz_equal.N", text)
        }

        text ~ /^(mov rcx,|repe cmps|cmps|jne \.Lbaz_equal|\.Lbaz_equal)/ {
            code[group] = code[group] text "\n"
        }

        text ~ /^set/ {
            split(text, words, " ")
            code[group] = code[group] words[1] "\n"
            group = 0
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
    expected="$TESTS/$src.x86_64.compares"
    echo -n "equal compares $src x86_64: "
    "$BIN" --target=x86_64 "$TESTS/$src.baz" >gen.s
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

    echo "FAILED. instructions differ:"
    diff "$expected" out || true
    echo "if intended, run: $0 update"
    exit 1
done
