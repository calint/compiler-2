#!/usr/bin/python3

import os
import subprocess

# the command of the target, from the runner
command = os.environ['PROGRAM_COMMAND_LINES'].splitlines()


def close_all(descriptors):
    for d in descriptors:
        os.close(d)


for closed in ((0, 1), (0, 2), (1, 2), (0, 1, 2)):
    p = subprocess.run(command,
                       stdin=subprocess.DEVNULL,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,
                       preexec_fn=lambda: close_all(closed))

    with open('sdcard.img', 'rb') as card:
        start = card.read(16)

    print('without descriptors', closed)
    print('  exit code:', p.returncode)
    print('  output:', p.stdout)
    print('  errors:', p.stderr)
    print('  card start:', start)
