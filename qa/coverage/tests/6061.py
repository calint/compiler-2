#!/usr/bin/python3

import os
import subprocess

# compiles '6061.baz' for the target of the case with the register report and
# prints the line of the footer with the most scratch registers in use, the
# report, which follows the footer, and the exit code
compiler = os.path.abspath('../../../baz')
machine = os.environ['MACHINE']
options = [f'--target={machine}', '--report=registers']
if machine != 'x86_64':
    options.append('--bin=gen-rv32i.bin')

result = subprocess.run([compiler, '6061.baz', *options],
                        capture_output=True, text=True)
lines = result.stdout.splitlines()
for line in lines:
    if 'max scratch registers in use' in line:
        print(line[1:].strip())
start = next(i for i, line in enumerate(lines) if 'register use at' in line)
for line in lines[start:]:
    print(line[2:].rstrip())
print(result.returncode)
