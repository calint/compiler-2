#!/bin/sh
# prints the peak heap of the compiler for each test source, measured with
# valgrind massif: 'NNNN.baz: BYTES mem_heap_B'
# usage: scripts/run-tests-and-print-peak-mem-usage.sh
set -eu
cd "$(dirname "$0")"

cd ../qa/coverage/tests/

for file in *.baz; do
    printf '%s: ' "$file"
    # baz may exit non-zero for tests that expect an error
    valgrind --tool=massif --massif-out-file=massif.out ../../../baz "$file" >/dev/null 2>/dev/null || true
    # the largest snapshot, compared as a number
    sed -n 's/^mem_heap_B=//p' massif.out | sort -n | tail -n 1 | awk '{print $1 " mem_heap_B"}'
    rm massif.out
done
