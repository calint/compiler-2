#!/bin/sh
# runs a flat rv32i image on the fpga emulator, the image is loaded at 0 and
# the uart is the terminal
# usage: run-rv32i-fpga.sh [prog-rv32i-fpga.bin] [sdcard.img]
# the emulator exits with the program's exit code, without an sd card image a
# temporary one of 8 MB of zeros is used
set -eu

# resolve before changing directory since the arguments are relative to the
# caller
if [ $# -eq 1 ]; then
    set -- "$(realpath "$1")"
elif [ $# -eq 2 ]; then
    set -- "$(realpath "$1")" "$(realpath "$2")"
fi

cd "$(dirname "$0")"

IMAGE="${1:-prog-rv32i-fpga.bin}"

EMULATOR=fpga-emulator/osqa
if [ ! -x "$EMULATOR" ]; then
    fpga-emulator/make.sh
fi

TEMPORARY=""
if [ $# -eq 2 ]; then
    SDCARD="$2"
else
    TEMPORARY="$(mktemp)"
    truncate -s 8M "$TEMPORARY"
    SDCARD="$TEMPORARY"
fi

# ctrl-c skips the emulator's terminal restore
trap 'if [ -n "$TEMPORARY" ]; then rm -f "$TEMPORARY"; fi; stty sane 2>/dev/null || true' EXIT
"$EMULATOR" "$IMAGE" "$SDCARD"
