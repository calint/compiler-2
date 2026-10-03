#!/bin/sh
set -eu
cd "$(dirname "$0")"

cd ../qa/coverage/tests/

for file in *.baz; do
    printf '%s: ' "$file"
    # baz may exit non-zero for tests that expect an error
    valgrind --tool=massif --massif-out-file=massif.out ../../../baz "$file" >/dev/null 2>/dev/null || true
    grep mem_heap_B massif.out | sort -r | head -n 1 | awk '{printf "%s ", $2; print $1}'
    rm massif.out
done
