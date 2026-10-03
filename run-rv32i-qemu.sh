#!/bin/sh
# runs a flat rv32i image on the qemu 'virt' machine without firmware, the
# image is loaded at 0x80000000 and the uart is the terminal
# usage: run-rv32i-qemu.sh [prog-rv32i-qemu.bin]
# qemu exits with the program's exit code, ctrl-d ends the input
set -eu

# resolve before changing directory since the argument is relative to the caller
if [ $# -gt 0 ]; then
    set -- "$(realpath "$1")"
fi

cd "$(dirname "$0")"

IMAGE="${1:-prog-rv32i-qemu.bin}"

# a plain serial console passes every input byte to the program
exec qemu-system-riscv32 -machine virt -bios none -display none \
    -serial stdio -monitor none -kernel "$IMAGE"
