#!/bin/bash
# runs a flat rv32i image in qemu user mode by wrapping it unchanged in an elf
# usage: run-rv32i-bin-as-elf.sh [gen-rv32i.bin]
# tools:
#   llvm-objcopy: 22.1.8
#         ld.lld: 22.1.8
#   qemu-riscv32: 11.1.1
set -eu
cd "$(dirname "$0")"

IMAGE="$(realpath "${1:-gen-rv32i.bin}")"
WORK="$(mktemp -d /tmp/baz-rv32i-image.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT

cd "$WORK"
cp "$IMAGE" image.bin
llvm-objcopy -I binary -O elf32-littleriscv --rename-section=.data=.image \
    image.bin image.o

# one writable and executable segment, the image uses only pc-relative addresses
cat >image.ld <<'EOF'
PHDRS { all PT_LOAD FLAGS(7); }
SECTIONS {
    . = 0x10000;
    .image : { *(.image) } :all
}
EOF

ld.lld -m elf32lriscv -T image.ld -e 0x10000 -o image image.o
qemu-riscv32 ./image
