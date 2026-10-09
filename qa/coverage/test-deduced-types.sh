#!/bin/bash
# checks the types deduced for untyped variables on source lines marked
# '# is TYPE' against the variable comments in the generated assembly, 'default'
# stands for the default type of the target and an array has its element type
#
# usage: test-deduced-types.sh
#
# to drop the check remove this script, the markers in the sources and its line
# in test-all.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/../../baz"
TESTS="$SCRIPT_DIR/tests"
SOURCES=(0605)

export LLVM_PROFILE_FILE="$SCRIPT_DIR/deduced-types-%p.profraw"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# prints one line per marked variable whose type differs or is missing, e.g.
# '35: less: expected bool, got i8'
check() {
    awk -v default_type="$2" '
        FNR == NR {
            if (match($0, /var [a-z_0-9]+[[ ].*# is [a-z_0-9]+$/)) {
                name = $0
                sub(/^.*var /, "", name)
                sub(/[[ ].*/, "", name)
                type = $0
                sub(/^.*# is /, "", type)
                if (type == "default") {
                    type = default_type
                }
                expected[FNR] = type
                names[FNR] = name
                order[++count] = FNR
            }
            next
        }

        match($0, /^[[:space:]]*[;#][[:space:]]*\[[0-9]+:[0-9]+\] [a-z_0-9]+: [a-z_0-9]+(\[[0-9]+\])? \(/) {
            text = $0
            sub(/^[[:space:]]*[;#][[:space:]]*\[/, "", text)
            line = text
            sub(/:.*/, "", line)
            line += 0
            sub(/^[0-9]+:[0-9]+\] /, "", text)
            split(text, words, /:? /)
            sub(/\[.*/, "", words[2])
            if (line in names && words[1] == names[line] && !(line in found)) {
                found[line] = words[2]
            }
        }

        END {
            for (i = 1; i <= count; ++i) {
                n = order[i]
                if (!(n in found)) {
                    printf "%d: %s: expected %s, got no variable comment\n", n, names[n], expected[n]
                    continue
                }
                if (found[n] != expected[n]) {
                    printf "%d: %s: expected %s, got %s\n", n, names[n], expected[n], found[n]
                }
            }
        }
    ' "$1" -
}

for src in "${SOURCES[@]}"; do
    for target in x86_64:i64 rv32i:i32; do
        echo -n "deduced types $src ${target%%:*}: "
        "$BIN" --target="${target%%:*}" --bin=gen-rv32i.bin "$TESTS/$src.baz" >gen.s
        check "$TESTS/$src.baz" "${target##*:}" <gen.s >out

        if [[ ! -s out ]]; then
            echo "ok ($(grep -c '# is [a-z_0-9]*$' "$TESTS/$src.baz") variables)"
            continue
        fi

        echo "FAILED"
        cat out
        exit 1
    done
done
