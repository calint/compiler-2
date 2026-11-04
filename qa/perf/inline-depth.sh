#!/bin/sh
# measures how compile time grows with the depth of nested inline calls,
# for each shape of call that makes the compiler try several versions
set -eu

cd "$(dirname "$0")"

usage() {
    cat <<'HELP'
Usage: ./inline-depth.sh [MAX_DEPTH] [TARGET]
  MAX_DEPTH  deepest chain of nested inline calls (default: 8)
  TARGET     baz --target (default: x86_64)

Environment: LIMIT=30 seconds per compile, a shape stops at the first
compile that exceeds it or fails; LEAF=50 statements in the leaf function,
so the compilations of the leaf dominate the time and the ratio approaches
the number of versions per level.

Shapes of the call in each level, 'a' and 'b' are local arrays and 'i' a
variable index:
  plain   'var v = f(x)'              no choice
  arith   'var v = f(x) + 1'          scalar memory destination
  arg     'var v = f(a[i])'           indexed argument, 3 body versions
  result  'b[i] = f(x)'               indexed result, 3 x 'compile_inline'
  all     'b[i] = f(a[i]) + 1'        all of the above

Columns: depth, seconds, ratio to the previous depth, lines of assembly.
HELP
}

if [ "${1:-}" = --help ] || [ "${1:-}" = -h ]; then
    usage
    exit 0
fi

max_depth=${1:-8}
target=${2:-x86_64}
limit=${LIMIT:-30}
leaf=${LEAF:-50}

work=$(mktemp -d "${TMPDIR:-/tmp}/inline-depth.XXXXXX")
trap 'rm -r "$work"' EXIT

# the statement of one level that calls the level below
call_statement() {
    callee=$1
    case $2 in
    plain) echo "    var v = $callee(x)" ;;
    arith) echo "    var v = $callee(x) + 1" ;;
    arg) echo "    var v = $callee(a[i])" ;;
    result) echo "    b[i] = $callee(x)" ;;
    all) echo "    b[i] = $callee(a[i]) + 1" ;;
    *) exit 2 ;;
    esac
}

# the value a level returns, from where its call statement wrote
result_expression() {
    case $1 in
    result | all) echo "b[i]" ;;
    *) echo "v" ;;
    esac
}

# 'f0' is the leaf, 'f<depth>' is called by 'main'
generate() {
    depth=$1
    shape=$2

    printf 'func f0(x) res {\n    res = x\n'

    statement=1
    while [ "$statement" -le "$leaf" ]; do
        printf '    res = res ^ x + %s\n' "$statement"
        statement=$((statement + 1))
    done

    printf '}\n'

    level=1
    while [ "$level" -le "$depth" ]; do
        printf '\nfunc f%s(x) res {\n' "$level"
        printf '    var i = x & 3\n'
        printf '    var a = [4]\n'
        printf '    a[i] = x\n'
        printf '    var b = [4]\n'
        call_statement "f$((level - 1))" "$shape"
        printf '    res = %s\n}\n' "$(result_expression "$shape")"
        level=$((level + 1))
    done

    printf '\nfunc main() {\n    exit(f%s(2))\n}\n' "$depth"
}

now_ns() {
    date +%s%N
}

printf 'target %s, limit %ss, leaf %s statements\n' "$target" "$limit" \
    "$leaf"

for shape in plain arith arg result all; do
    printf '\n%s\n' "$shape"
    printf '%6s %10s %8s %8s\n' depth seconds ratio lines

    previous_ns=
    depth=1
    while [ "$depth" -le "$max_depth" ]; do
        src="$work/$shape-$depth.baz"
        asm="$work/$shape-$depth.s"
        generate "$depth" "$shape" >"$src"

        start_ns=$(now_ns)
        status=0
        timeout "$limit" ../../baz --target="$target" "$src" >"$asm" \
            2>"$work/error.txt" || status=$?
        elapsed_ns=$(($(now_ns) - start_ns))

        if [ "$status" -eq 124 ]; then
            printf '%6s %10s\n' "$depth" ">$limit"
            break
        fi

        if [ "$status" -ne 0 ]; then
            printf '%6s failed, exit %s\n' "$depth" "$status"
            cat "$work/error.txt"
            break
        fi

        ratio=-
        if [ -n "$previous_ns" ] && [ "$previous_ns" -gt 0 ]; then
            ratio=$(awk "BEGIN { printf \"%.1f\", $elapsed_ns / $previous_ns }")
        fi

        printf '%6s %10s %8s %8s\n' "$depth" \
            "$(awk "BEGIN { printf \"%.3f\", $elapsed_ns / 1e9 }")" \
            "$ratio" "$(wc -l <"$asm")"

        previous_ns=$elapsed_ns
        depth=$((depth + 1))
    done
done
