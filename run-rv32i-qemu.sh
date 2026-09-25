#!/bin/bash
# runs a flat rv32i image on the qemu 'virt' machine without firmware, the
# image is loaded at 0x80000000 and the uart is the terminal
# usage: run-rv32i-qemu.sh [gen-rv32i.bin]
# qemu exits with the program's exit code, ctrl-d ends the input
# tools:
#   qemu-system-riscv32: 11.1.1
set -eu
# a given image is relative to the caller's directory
IMAGE="$(realpath "${1:-$(dirname "$0")/gen-rv32i.bin}")"
cd "$(dirname "$0")"

# a plain serial console passes every input byte to the program
exec qemu-system-riscv32 -machine virt -bios none -display none \
    -serial stdio -monitor none -kernel "$IMAGE"
