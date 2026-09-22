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
qemu-riscv32 "$TEST_DIR/test"
printf 'rv32i address lowering: ok\n'