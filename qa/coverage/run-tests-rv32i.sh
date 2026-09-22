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
qemu-riscv32 "$TEST_DIR/test" < "$SCRIPT_DIR/t486.in" > "$TEST_DIR/output"
cmp "$TEST_DIR/output" "$SCRIPT_DIR/t486.out"
printf 'rv32i arithmetic: checking division by zero trap\n'
ld.lld -m elf32lriscv -e divide_by_zero -o "$TEST_DIR/divide-by-zero" "$TEST_DIR/test.o"
ulimit -c 0
status=0
qemu-riscv32 "$TEST_DIR/divide-by-zero" || status=$?
if [ "$status" -ne 133 ]; then
    printf 'Expected SIGTRAP (133), got %s\n' "$status" >&2
    exit 1
fi
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
printf 'rv32i strings and raw syscalls: compiling and executing\n'
"$TEST_DIR/generate" strings-syscall > "$TEST_DIR/strings.s"
llvm-mc -triple=riscv32 -mattr=-m,-a,-f,-d,-c -filetype=obj \
    "$TEST_DIR/strings.s" -o "$TEST_DIR/strings.o"
ld.lld -m elf32lriscv -e _start -o "$TEST_DIR/strings" "$TEST_DIR/strings.o"
qemu-riscv32 "$TEST_DIR/strings" > "$TEST_DIR/output"
printf '\101\000\007\010\011\012\013\014\015\033\042\047\140\134\000\177\200\377\101\102' > "$TEST_DIR/expected"
cmp "$TEST_DIR/output" "$TEST_DIR/expected"
printf 'rv32i strings and raw syscalls: ok\n'