#!/bin/sh
# runs a flat rv32i image on the fpga emulator, the image is loaded at 0 and
# the uart is the terminal
# usage: run-rv32i-fpga.sh [prog-rv32i-fpga.bin]
# the emulator exits with the program's exit code
set -eu

# resolve before changing directory since the argument is relative to the caller
if [ $# -gt 0 ]; then
    set -- "$(realpath "$1")"
fi

cd "$(dirname "$0")"

IMAGE="${1:-prog-rv32i-fpga.bin}"

EMULATOR=fpga-emulator/osqa
if [ ! -x "$EMULATOR" ]; then
    fpga-emulator/make.sh
fi

# the emulator requires an sd card image which the program does not use
SDCARD="$(mktemp)"

# ctrl-c skips the emulator's terminal restore
trap 'rm -f "$SDCARD"; stty sane 2>/dev/null || true' EXIT
"$EMULATOR" "$IMAGE" "$SDCARD"
