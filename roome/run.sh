#!/bin/sh
# compiles roome and runs it on the fpga emulator, the uart is the terminal
# the emulator exits with the program's exit code
set -eu
cd "$(dirname "$0")"

SEP="--------------------------------------------------------------------------------"

# the image that is run and that the board runs
IMAGE=roome-rv32i-fpga.bin

# a '--checks' in the arguments replaces the default one
../baz --target=rv32i-fpga --vars=0x20000 --checks=noub,line --report=registers \
    --bin="$IMAGE" "$@" src/main.baz >roome.s
echo "$SEP"

# the report follows the last blank line
awk 'NF == 0 { report = ""; next } { report = report $0 "\n" } END { printf "%s", report }' \
    roome.s
echo "$SEP"

ls -l "$IMAGE"
echo "$SEP"

# the sd card of the sector commands keeps what is written between runs
SDCARD=sdcard.img
if [ ! -e "$SDCARD" ]; then
    truncate -s 8M "$SDCARD"
fi

../run-rv32i-fpga.sh "$IMAGE" "$SDCARD"
