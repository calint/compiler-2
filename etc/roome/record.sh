#!/bin/bash
# records an interactive roome session: what is typed becomes roome.in and,
# after confirming the session behaved correctly, roome.out is generated
# end the session with 'go home'
# usage: record.sh
set -eu
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/../.."

EMULATOR=fpga-emulator/osqa
if [[ ! -x $EMULATOR ]]; then
    fpga-emulator/make.sh
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

./baz --target=rv32i-fpga --vars=131072 --checks=noub,line "$DIR/roome.baz" >"$WORK/roome.s"

# the emulator requires an sd card image which the program does not use
: >"$WORK/sdcard"

echo "roome: recording, end the session with 'go home'"

# 'script' gives the emulator a terminal and logs the typed bytes, -e passes
# on the emulator's exit code so an aborted session is not saved
# ctrl-c skips the emulator's terminal restore
trap 'rm -rf "$WORK"; stty sane 2>/dev/null || true' EXIT
script -q -e -I "$WORK/typed" -c "$EMULATOR $DIR/roome-rv32i-fpga.bin $WORK/sdcard" || {
    echo "roome: session did not end normally, nothing saved"
    exit 1
}

# the terminal sends carriage returns for enter, 'script' adds a header line, a
# footer line and a blank line before the footer
tr '\r' '\n' <"$WORK/typed" | sed -e '1d' -e '$d' | sed -e '$d' >"$WORK/roome.in"

echo
echo "roome: recorded input:"
cat "$WORK/roome.in"

read -r -p "roome: was the session correct, save roome.in and roome.out? [y/N] " answer
if [[ $answer != y ]]; then
    echo "roome: nothing saved"
    exit 1
fi

cp "$WORK/roome.in" "$DIR/roome.in"

# the expected output comes from a replay so that it has the same form as in the test
"$DIR/test.sh" update
