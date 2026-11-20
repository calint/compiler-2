#!/usr/bin/python3

import os
import subprocess

# the command of the target, from the runner
command = os.environ['PROGRAM_COMMAND_LINES'].splitlines()

p = subprocess.Popen(command,
                     stdin=subprocess.PIPE,
                     stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE)

# the image is open once the program writes its first line
first = p.stdout.readline()
os.truncate('sdcard.img', 512)
rest, errors = p.communicate(b'x')

print('first line:', first)
print('exit code:', p.returncode)
print('output:', rest)
print('errors:', errors)
print('card bytes:', os.path.getsize('sdcard.img'))
