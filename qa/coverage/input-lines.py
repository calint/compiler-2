#!/usr/bin/env python3
# usage: input-lines.py INPUT_FILE COMMAND [ARG...]
# runs COMMAND with a terminal on stdin so each read returns at most one line
import os
import pty
import subprocess
import sys
import termios

input_file = sys.argv[1]
command = sys.argv[2:]

master, slave = pty.openpty()

# pass input bytes through unchanged and keep them out of the output
attributes = termios.tcgetattr(slave)
attributes[0] &= ~(termios.ICRNL | termios.INLCR | termios.IGNCR | termios.IXON)
attributes[3] &= ~termios.ECHO
attributes[3] |= termios.ICANON
termios.tcsetattr(slave, termios.TCSANOW, attributes)

with open(input_file, "rb") as file:
    data = file.read()

# end of input makes a read past the last line return 0 instead of blocking
# a partial last line needs one EOF to flush it and another to signal end
data += b"\x04" if data.endswith(b"\n") else b"\x04\x04"

process = subprocess.Popen(command, stdin=slave)
os.close(slave)

while data:
    data = data[os.write(master, data) :]

# unread input is discarded when the child exits, so there is nothing to drain
exit_code = process.wait()
os.close(master)
sys.exit(exit_code)
