#!/bin/sh
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TEST_DIR="$(mktemp -d /tmp/baz-rv32i-test.XXXXXX)"
trap 'rm -rf "$TEST_DIR"' EXIT HUP INT TERM

for tool in clang++ llvm-mc ld.lld qemu-riscv32; do
    command -v "$tool" >/dev/null || {
        printf 'Required RV32I test tool not found: %s\n' "$tool" >&2
        exit 1
    }
done

printf 'rv32i address lowering: compiling backend tests\n'
clang++ -std=c++26 -O3 -Wno-braced-scalar-init \
    "$SCRIPT_DIR/test-rv32i-address.cpp" -o "$TEST_DIR/generate"
"$TEST_DIR/generate" > "$TEST_DIR/test.s"
printf 'rv32i address lowering: assembling and linking\n'
llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
    "$TEST_DIR/test.s" -o "$TEST_DIR/test.o"
ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/test" "$TEST_DIR/test.o"
printf 'rv32i address lowering: executing with QEMU\n'
qemu-riscv32 "$TEST_DIR/test" < "$SCRIPT_DIR/tests/434.in" > "$TEST_DIR/output"
cmp "$TEST_DIR/output" "$SCRIPT_DIR/tests/434.out"
printf 'rv32i arithmetic: checking division by zero trap\n'
ld.lld -m elf32lriscv -e divide_by_zero -o "$TEST_DIR/divide-by-zero" "$TEST_DIR/test.o"
ulimit -c 0
status=0
{ qemu-riscv32 "$TEST_DIR/divide-by-zero" || status=$?; } 2> "$TEST_DIR/err"
if [ "$status" -ne 133 ]; then
    cat "$TEST_DIR/err" >&2
    printf 'Expected SIGTRAP (133), got %s\n' "$status" >&2
    exit 1
fi
printf 'rv32i arithmetic: division by zero trap: ok\n'
printf 'rv32i bounds: checking diagnostic line numbers\n'
for line in 0 9 123 4294967295; do
    ld.lld -m elf32lriscv -e "bounds_line_$line" -o "$TEST_DIR/bounds" "$TEST_DIR/test.o"
    status=0
    qemu-riscv32 "$TEST_DIR/bounds" 2> "$TEST_DIR/err" || status=$?
    test "$status" -eq 255
    printf 'panic: bounds at line %s\n' "$line" > "$TEST_DIR/expected"
    cmp "$TEST_DIR/err" "$TEST_DIR/expected"
done
for mode in bounds-matrix bounds-silent; do
    printf 'rv32i bounds: %s\n' "$mode"
    "$TEST_DIR/generate" "$mode" > "$TEST_DIR/bounds.s"
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
        "$TEST_DIR/bounds.s" -o "$TEST_DIR/bounds.o"
    ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/bounds" "$TEST_DIR/bounds.o"
    status=0
    qemu-riscv32 "$TEST_DIR/bounds" 2> "$TEST_DIR/err" || status=$?
    expected=0
    if [ "$mode" = bounds-silent ]; then
        expected=255
    fi
    test "$status" -eq "$expected"
    test ! -s "$TEST_DIR/err"
done
printf 'rv32i address lowering: ok\n'
printf 'rv32i strings and write: compiling and executing\n'
"$TEST_DIR/generate" strings-syscall > "$TEST_DIR/strings.s"
llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
    "$TEST_DIR/strings.s" -o "$TEST_DIR/strings.o"
ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/strings" "$TEST_DIR/strings.o"
qemu-riscv32 "$TEST_DIR/strings" > "$TEST_DIR/output"
printf '\101\000\007\010\011\012\013\014\015\033\042\047\140\134\000\177\200\377\101\102' > "$TEST_DIR/expected"
cmp "$TEST_DIR/output" "$TEST_DIR/expected"
printf 'rv32i strings and write: ok\n'
printf 'rv32i bulk operations: compiling and executing\n'
"$TEST_DIR/generate" bulk > "$TEST_DIR/bulk.s"
llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
    "$TEST_DIR/bulk.s" -o "$TEST_DIR/bulk.o"
ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/bulk" "$TEST_DIR/bulk.o"
qemu-riscv32 "$TEST_DIR/bulk"
printf 'rv32i bulk operations: ok\n'
printf 'rv32i array iteration: executing loop body larger than 4 KiB\n'
"$TEST_DIR/generate" long-loop > "$TEST_DIR/long-loop.s"
llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
    "$TEST_DIR/long-loop.s" -o "$TEST_DIR/long-loop.o"
ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/long-loop" "$TEST_DIR/long-loop.o"
timeout -k 1s 5s qemu-riscv32 "$TEST_DIR/long-loop"
printf 'rv32i array iteration: long loop: ok\n'
printf 'rv32i jumps: resolving and executing jumps beyond 4 KiB and 1 MiB\n'
for mode in far-jumps far-jumps-optimized; do
    "$TEST_DIR/generate" "$mode" > "$TEST_DIR/far-jumps.s"
    grep -q '^    jump far_loop_270000, ' "$TEST_DIR/far-jumps.s"
    grep -q '^    jump far_taken_270000, ' "$TEST_DIR/far-jumps.s"
    grep -q '^    jump far_failure, ' "$TEST_DIR/far-jumps.s"
    grep -q '^    jump far_skipped_270000, ' "$TEST_DIR/far-jumps.s"
    grep -q '^    j far_loop_2048$' "$TEST_DIR/far-jumps.s"
    grep -q '^    j far_taken_2048$' "$TEST_DIR/far-jumps.s"
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
        "$TEST_DIR/far-jumps.s" -o "$TEST_DIR/far-jumps.o"
    ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/far-jumps" "$TEST_DIR/far-jumps.o"
    timeout -k 1s 20s qemu-riscv32 "$TEST_DIR/far-jumps"
done
printf 'rv32i jumps: far jumps: ok\n'
printf 'rv32i jumps: compiling and executing foo, if, break and continue beyond 4 KiB and 1 MiB\n'
for mode in far-foo far-foo-optimized; do
    "$TEST_DIR/generate" "$mode" > "$TEST_DIR/far-foo.s"
    grep -qE '^ +j foo\.[0-9]+\.[0-9]+$' "$TEST_DIR/far-foo.s"
    grep -qE '^ +jump foo\.[0-9]+\.[0-9]+, ' "$TEST_DIR/far-foo.s"
    grep -qE '^ +jump foo\.[0-9]+\.[0-9]+\.continue, ' "$TEST_DIR/far-foo.s"
    grep -qE '^ +jump foo\.[0-9]+\.[0-9]+\.end, ' "$TEST_DIR/far-foo.s"
    grep -qE '^ +j if\.[0-9]+\.[0-9]+\.end$' "$TEST_DIR/far-foo.s"
    grep -qE '^ +jump if\.[0-9]+\.[0-9]+\.end, ' "$TEST_DIR/far-foo.s"
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
        "$TEST_DIR/far-foo.s" -o "$TEST_DIR/far-foo.o"
    ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/far-foo" "$TEST_DIR/far-foo.o"
    timeout -k 1s 20s qemu-riscv32 "$TEST_DIR/far-foo"
done
printf 'rv32i jumps: far foo: ok\n'
for mode in noninline frame-checks; do
    printf 'rv32i functions: %s\n' "$mode"
    "$TEST_DIR/generate" "$mode" > "$TEST_DIR/functions.s"
    llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
        "$TEST_DIR/functions.s" -o "$TEST_DIR/functions.o"
    ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/functions" "$TEST_DIR/functions.o"
    timeout -k 1s 5s qemu-riscv32 "$TEST_DIR/functions"
done
printf 'rv32i functions: ok\n'