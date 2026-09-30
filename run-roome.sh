#!/bin/bash
# runs a flat rv32i image on the fpga emulator, the image is loaded at 0 and
# the uart is the terminal
# the emulator exits with the program's exit code
set -eu
cd "$(dirname "$0")"

EMULATOR=fpga-emulator/osqa
if [[ ! -x $EMULATOR ]]; then
    fpga-emulator/make.sh
fi

# the emulator requires an sd card image which the program does not use
SDCARD="$(mktemp)"

SEP="--------------------------------------------------------------------------------"

# a '--checks' in the arguments replaces the default one
./baz --target=rv32i-fpga --checks=noub,line "$@" etc/roome/roome.baz >etc/roome/roome.s
echo $SEP

tail -n 10 etc/roome/roome.s
echo $SEP

IMAGE=etc/roome/roome-rv32i-fpga.bin

ls -l "$IMAGE"
echo $SEP

# ctrl-c skips the emulator's terminal restore
trap 'rm -f "$SDCARD"; stty sane 2>/dev/null || true' EXIT
echo "$EMULATOR" "$IMAGE" "$SDCARD"
echo $SEP
"$EMULATOR" "$IMAGE" "$SDCARD"
