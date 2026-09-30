#!/bin/bash
# runs roome.baz on the fpga emulator with the lines of roome.in as uart input
# and compares the uart output with roome.out
# usage: test-roome.sh [update]
# 'update' rewrites roome.out with the current output
set -eu
cd "$(dirname "$0")/../.."

EMULATOR=fpga-emulator/osqa
if [[ ! -x $EMULATOR ]]; then
    fpga-emulator/make.sh
fi

DIR=etc/roome
IMAGE=$DIR/roome-rv32i-fpga.bin
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

./baz --target=rv32i-fpga --checks=noub,line "$DIR/roome.baz" >"$WORK/roome.s"

# the emulator requires an sd card image which the program does not use
: >"$WORK/sdcard"

# the input must end with 'go home', at the end of input the emulator would
# make the program wait for input forever, so the run is limited by a timeout
status=0
timeout 10 "$EMULATOR" "$IMAGE" "$WORK/sdcard" <"$DIR/roome.in" >"$WORK/actual" || status=$?

if [[ $status -eq 124 ]]; then
    echo "roome: timeout, does roome.in end with 'go home'?"
    exit 1
fi

if [[ ${1:-} == update ]]; then
    cp "$WORK/actual" "$DIR/roome.out"
    echo "roome: updated $DIR/roome.out"
    exit 0
fi

if ! diff -u "$DIR/roome.out" "$WORK/actual"; then
    echo "roome: output differs from $DIR/roome.out"
    exit 1
fi

echo "roome: ok"
