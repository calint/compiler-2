#!/usr/bin/python3

import os
import subprocess

# the command of the target, from the runner
command = os.environ['PROGRAM_COMMAND_LINES'].splitlines()

for closed in (0, 1, 2):
    p = subprocess.run(command,
                       stdin=subprocess.DEVNULL,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,
                       preexec_fn=lambda: os.close(closed))

    print('without descriptor', closed)
    print('  exit code:', p.returncode)
    print('  output:', p.stdout)
    print('  errors:', p.stderr)
    print('  card bytes:', os.path.getsize('sdcard.img'))
