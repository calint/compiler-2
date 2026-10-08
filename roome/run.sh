#!/bin/sh
# compiles roome and runs it on the fpga emulator, the uart is the terminal
# the emulator exits with the program's exit code
set -eu
cd "$(dirname "$0")"

SEP="--------------------------------------------------------------------------------"

# a '--checks' in the arguments replaces the default one
../baz --target=rv32i-fpga --vars=0x20000 --checks=noub,line --report=registers \
    "$@" src/main.baz >roome.s
echo "$SEP"

# the report follows the last blank line
awk 'NF == 0 { report = ""; next } { report = report $0 "\n" } END { printf "%s", report }' \
    roome.s
echo "$SEP"

IMAGE=roome-rv32i-fpga.bin

ls -l "$IMAGE"
echo "$SEP"

../run-rv32i-fpga.sh "$IMAGE"
