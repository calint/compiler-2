#!/bin/bash
# runs a flat rv32i image on the fpga emulator, the image is loaded at 0 and
# the uart is the terminal
# usage: run-roome.sh [roome-rv32i-fpga.bin]
# the emulator exits with the program's exit code
set -eu
cd "$(dirname "$0")"

EMULATOR=fpga-emulator/osqa
if [[ ! -x $EMULATOR ]]; then
    fpga-emulator/make.sh
fi

# the emulator requires an sd card image which the program does not use
SDCARD="$(mktemp)"

./baz --target=rv32i-fpga --vars=131072 --checks=noub \
    etc/roome/roome.baz >etc/roome/roome.s

IMAGE=etc/roome/roome-rv32i-fpga.bin

# ctrl-c skips the emulator's terminal restore
trap 'rm -f "$SDCARD"; stty sane 2>/dev/null || true' EXIT
echo "$EMULATOR" "$IMAGE" "$SDCARD"
"$EMULATOR" "$IMAGE" "$SDCARD"
