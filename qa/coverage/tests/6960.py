#!/usr/bin/python3

import os
import subprocess

# the command of the target, from the runner
command = os.environ['PROGRAM_COMMAND_LINES'].splitlines()

# the descriptors 3 and 4 are open to this file in the program
name = '6960.file'
opened = os.open(name, os.O_RDWR | os.O_CREAT | os.O_TRUNC, 0o644)
# note: 'os.open' makes a descriptor that 'exec' closes, 'dup2' to itself
#       keeps that
os.set_inheritable(opened, True)


def open_above_standard():
    os.dup2(opened, 3)
    os.dup2(opened, 4)


p = subprocess.run(command,
                   stdin=subprocess.DEVNULL,
                   stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE,
                   preexec_fn=open_above_standard,
                   close_fds=False)

os.close(opened)

print('exit code:', p.returncode)
print('output:', p.stdout)
print('errors:', p.stderr)
print('file bytes:', os.path.getsize(name))

os.remove(name)
