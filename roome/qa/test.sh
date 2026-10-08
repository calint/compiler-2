#!/bin/bash
# runs main.baz with the lines of roome.in as input, on the fpga emulator
# (rv32i-fpga) and as a native program (x86_64), and compares the output of
# each with roome.out
# usage: test.sh [update]
# 'update' rewrites roome.out with the output of rv32i-fpga, x86_64 is still
# compared
set -eu
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/../.."

EMULATOR=fpga-emulator/osqa
if [[ ! -x $EMULATOR ]]; then
    fpga-emulator/make.sh
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
IMAGE=$WORK/roome-rv32i-fpga.bin

./baz --target=rv32i-fpga --vars=0x20000 --checks=noub,line --bin="$IMAGE" "$DIR/../src/main.baz" >"$WORK/roome.s"

# the emulator requires an sd card image which the program does not use
: >"$WORK/sdcard"

# the input must end with 'go home', at the end of input the emulator would
# make the program wait for input forever, so the run is limited by a timeout
status=0
timeout 10 "$EMULATOR" "$IMAGE" "$WORK/sdcard" <"$DIR/roome.in" >"$WORK/diff" || status=$?

if [[ $status -eq 124 ]]; then
    echo "roome: rv32i-fpga timeout, does roome.in end with 'go home'?"
    exit 1
fi

if [[ ${1:-} == update ]]; then
    cp "$WORK/diff" "$DIR/roome.out"
    echo "roome: updated $DIR/roome.out"
fi

if ! diff -u "$DIR/roome.out" "$WORK/diff"; then
    echo "roome: rv32i-fpga output differs from $DIR/roome.out"
    exit 1
fi

echo "roome: rv32i-fpga ok"

# the native build catches what rv32i-fpga does not, such as running out of
# registers
./baz --target=x86_64 --vars=0x20000 --checks=noub,line "$DIR/../src/main.baz" >"$WORK/roome-x86_64.s"
nasm -f elf64 "$WORK/roome-x86_64.s" -o "$WORK/roome-x86_64.o"
ld -s -T baz.ld -o "$WORK/roome-x86_64" "$WORK/roome-x86_64.o"

status=0
timeout 10 "$WORK/roome-x86_64" <"$DIR/roome.in" >"$WORK/x86_64.out" || status=$?

if [[ $status -eq 124 ]]; then
    echo "roome: x86_64 timeout, does roome.in end with 'go home'?"
    exit 1
fi

if ! diff -u "$DIR/roome.out" "$WORK/x86_64.out"; then
    echo "roome: x86_64 output differs from $DIR/roome.out"
    exit 1
fi

echo "roome: x86_64 ok"
