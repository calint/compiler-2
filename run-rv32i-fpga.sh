#!/bin/bash
# runs a flat rv32i image on the fpga emulator, the image is loaded at 0 and
# the uart is the terminal
# usage: run-rv32i-fpga.sh [gen-rv32i.bin]
# the emulator exits with the program's exit code
set -eu
cd "$(dirname "$0")"

# a given image is relative to the caller's directory
IMAGE="$(realpath "${1:-$(dirname "$0")/gen-rv32i.bin}")"

EMULATOR=fpga-emulator/osqa
if [[ ! -x $EMULATOR ]]; then
    fpga-emulator/make.sh
fi

# the emulator requires an sd card image which the program does not use
SDCARD="$(mktemp)"

# ctrl-c skips the emulator's terminal restore
trap 'rm -f "$SDCARD"; stty sane 2>/dev/null || true' EXIT
"$EMULATOR" "$IMAGE" "$SDCARD"
