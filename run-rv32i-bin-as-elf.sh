#!/bin/sh
# runs a flat rv32i image in qemu user mode by wrapping it unchanged in an elf
# usage: run-rv32i-bin-as-elf.sh [prog-rv32i.bin] [vars size, the compiler's --vars]
set -eu

# resolve before changing directory since the argument is relative to the caller
if [ $# -gt 0 ]; then
    set -- "$(realpath "$1")"
fi

cd "$(dirname "$0")"

IMAGE="$(realpath "${1:-prog-rv32i.bin}")"
# the image ends at the data, the variables area follows it unmapped
VARS_SIZE="${2:-65536}"
WORK="$(mktemp -d /tmp/baz-rv32i-image.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT

cd "$WORK"
cp "$IMAGE" image.bin
llvm-objcopy -I binary -O elf32-littleriscv --rename-section=.data=.image \
    image.bin image.o

# one writable and executable segment, the image uses only pc-relative addresses
cat >image.ld <<EOF
PHDRS { all PT_LOAD FLAGS(7); }
SECTIONS {
    . = 0x10000;
    .image : { *(.image) } :all
    .vars (NOLOAD) : ALIGN(16) { . += $VARS_SIZE; } :all
}
EOF

ld.lld -m elf32lriscv -T image.ld -e 0x10000 -o image image.o
qemu-riscv32 ./image
