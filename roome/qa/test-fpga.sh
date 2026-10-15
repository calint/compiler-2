#!/bin/bash
# runs the lines of roome.in on the real fpga over the serial port and
# compares the uart output with roome.out, the same test as test.sh
# the board must run roome-rv32i-fpga.bin, the port is opened first, then the
# board is reset with S2 so that the whole output is received
# usage: test-fpga.sh [serial device]
# the device defaults to /dev/ttyUSB1, as in scripts/fpga-connect-serial.sh
# requires python3 with pyserial
set -eu
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

if ! python3 -c "import serial" 2>/dev/null; then
    echo "roome: the test needs pyserial, install the package 'python-pyserial'"
    echo "       or run 'pip install pyserial'"
    exit 1
fi

DEVICE=${1:-/dev/ttyUSB1}
if [ ! -e "$DEVICE" ]; then
    echo "roome: $DEVICE not found, is the board connected?"
    echo "roome: serial devices:"
    FOUND=0
    for candidate in /dev/ttyUSB*; do
        if [ -e "$candidate" ]; then
            echo "  $candidate"
            FOUND=1
        fi
    done
    if [ $FOUND -eq 0 ]; then
        echo "  none"
    fi
    echo "roome: pass the board's uart as: test-fpga.sh <serial device>"
    exit 1
fi

# kept when the test fails so the output can be inspected
WORK="$DIR/fpga-work"
mkdir -p "$WORK"

python3 - "$DEVICE" "$DIR/roome.in" "$WORK/fpga.out" "$DIR/roome.out" <<'EOF'
import signal
import sys
import serial

device, input_path, output_path, expected_path = sys.argv[1:5]
prompt = b" > "
prompt_timeout = 10
idle_timeout = 0.02
# how long the port must be quiet before the stale output counts as drained
drain_quiet = 0.5

with open(input_path, "rb") as f:
    lines = f.read().splitlines(keepends=True)

# the expected output as one part per prompt: the start, the reply to each
# line, the end of the program
with open(expected_path, "rb") as f:
    parts = f.read().split(prompt)
expected = [part + prompt for part in parts[:-1]] + [parts[-1]]

# ctrl-c ends the test without a stack trace
signal.signal(signal.SIGINT, lambda *_: (print("\nroome: interrupted"),
                                          sys.exit(130)))


# the board sends bare line feeds so the bytes are compared unchanged
try:
    port = serial.Serial(device, 115200, bytesize=8, parity="N", stopbits=1,
                         xonxoff=False, rtscts=False, dsrdtr=False, timeout=idle_timeout)
except serial.SerialException as error:
    print(f"roome: cannot open {device}: {error.strerror or error}")
    sys.exit(1)

port.reset_input_buffer()
received = bytearray()


def fail(message):
    with open(output_path, "wb") as f:
        f.write(received)
    print(f"roome: {message}")
    sys.exit(1)


def read_until_prompt(timeout):
    port.timeout = timeout
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


# returns the end of the key which starts at 'start': a plain byte or an
# escape sequence 'ESC [ ... final', the final byte is the first from '@' on
def key_end(line, start):
    if line[start] != 0x1b or line[start + 1:start + 2] != b"[":
        return start + 1
    # note: +2 to skip the escape and the '['
    end = start + 2
    while end < len(line) and line[end] < 0x40:
        end += 1
    return min(end + 1, len(line))


# output printed while the port was closed, e.g. after flashing, can wait in
# the usb chip and arrive after the open, it is read and dropped
port.timeout = drain_quiet
while port.read(1):
    pass
port.reset_input_buffer()

# the port is open, so the output after the reset is kept until it is read
# stdin carries this script, so the confirmation is read from the terminal
print("roome: press s2 to reset the board then 'enter' to start test ", end="",
      flush=True)
with open("/dev/tty") as tty:
    tty.readline()

try:
    read_until_prompt(prompt_timeout)
    reply_start = len(received)

    for number, line in enumerate(lines):
        print(f"roome: line {number + 1} of {len(lines)}", end="", file=sys.stderr, flush=True)
        # the program writes while it handles a byte (echo, redraw after an
        # edit), a byte sent meanwhile could be lost by the uart, so after each
        # key the output is read until it is idle; an escape sequence is sent
        # whole since the program reads it silently and answers only after its
        # last byte
        index = 0
        while index < len(line):
            end = key_end(line, index)
            port.write(line[index:end])
            read_until_idle()
            index = end
        if number == len(lines) - 1:
            # the last line ends the program, so there is no prompt to wait for
            read_until_idle()
        else:
            read_until_prompt(prompt_timeout)
        reply = bytes(received[reply_start:])
        reply_start = len(received)
        matches = number + 1 < len(expected) and reply == expected[number + 1]
        print(" ok" if matches else " differs", file=sys.stderr, flush=True)
except serial.SerialException as error:
    fail(f"the serial port failed ({error}), is a serial terminal open on "
         f"{device}, or was the board flashed or unplugged during the test?")

with open(output_path, "wb") as f:
    f.write(received)
EOF

if ! diff -u "$DIR/roome.out" "$WORK/fpga.out"; then
    echo "roome: fpga output differs from $DIR/roome.out, the output is in $WORK/fpga.out"
    exit 1
fi

rm -rf "$WORK"
echo "roome: fpga ok"
