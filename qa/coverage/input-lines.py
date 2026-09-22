import os
import pty
import subprocess
import sys
import termios

master, slave = pty.openpty()
try:
    attributes = termios.tcgetattr(slave)
    attributes[0] &= ~(termios.ICRNL | termios.INLCR | termios.IGNCR | termios.IXON)
    attributes[3] &= ~termios.ECHO
    attributes[3] |= termios.ICANON
    termios.tcsetattr(slave, termios.TCSANOW, attributes)
    command = ['qemu-riscv32', './gen'] if sys.argv[1] == 'rv32i' else ['./gen']
    with subprocess.Popen(command, stdin=slave) as process:
        with open(sys.argv[2], 'rb') as source:
            data = source.read()
        data += b'\x04' if data.endswith(b'\n') else b'\x04\x04'
        while data:
            data = data[os.write(master, data):]
        sys.exit(process.wait())
finally:
    os.close(slave)
    os.close(master)