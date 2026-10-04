#!/bin/bash
# runs the lines of roome.in on the real fpga over the serial port and
# compares the uart output with roome.out, the same test as test.sh
# the board must run roome-rv32i-fpga.bin freshly started, reset it when asked
# usage: fpga-test.sh [serial device]
# the device defaults to /dev/ttyUSB1, as in scripts/fpga-connect-serial.sh
# requires python3 with pyserial
set -eu
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

DEVICE=${1:-/dev/ttyUSB1}
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

python3 - "$DEVICE" "$DIR/roome.in" "$WORK/fpga.out" <<'EOF'
import sys
import serial

device, input_path, output_path = sys.argv[1:4]
prompt = b" > "
echo_timeout = 2
prompt_timeout = 10
idle_timeout = 1

with open(input_path, "rb") as f:
    lines = f.read().splitlines(keepends=True)

# the board sends bare line feeds so the bytes are compared unchanged
try:
    port = serial.Serial(device, 115200, bytesize=8, parity="N", stopbits=1,
                         xonxoff=False, rtscts=False, dsrdtr=False, timeout=echo_timeout)
except serial.SerialException as error:
    print(f"roome: {error}")
    print("roome: check that the board is connected, list devices with"
          " 'ls /dev/ttyUSB*', and pass the right one: fpga-test.sh <serial device>")
    sys.exit(1)

port.reset_input_buffer()
received = bytearray()


def fail(message):
    with open(output_path, "wb") as f:
        f.write(received)
    print(f"roome: {message}")
    sys.exit(1)


def read_until_prompt():
    port.timeout = prompt_timeout
    while not received.endswith(prompt):
        data = port.read(1)
        if not data:
            fail("timeout waiting for the prompt")
        received.extend(data)


def read_until_idle():
    port.timeout = idle_timeout
    while True:
        data = port.read(1)
        if not data:
            return
        received.extend(data)


# stdin carries this script, so the confirmation is read from the terminal
print("roome: reset the board, then press enter ", end="", flush=True)
with open("/dev/tty") as tty:
    tty.readline()
read_until_prompt()

for number, line in enumerate(lines):
    # the program echoes each byte it reads, a byte sent before the echo of
    # the previous one could be lost by the uart, so each byte waits for its echo
    for byte in line:
        port.write(bytes([byte]))
        port.timeout = echo_timeout
        data = port.read(1)
        if not data:
            fail(f"timeout waiting for the echo of byte {byte} in line {number + 1}")
        received.extend(data)
    if number == len(lines) - 1:
        # the last line ends the program, so there is no prompt to wait for
        read_until_idle()
    else:
        read_until_prompt()

with open(output_path, "wb") as f:
    f.write(received)
EOF

if ! diff -u "$DIR/roome.out" "$WORK/fpga.out"; then
    echo "roome: fpga output differs from $DIR/roome.out"
    exit 1
fi

echo "roome: fpga ok"
