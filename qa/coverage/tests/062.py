#!/usr/bin/python3

import subprocess
import os

machine = os.environ.get('MACHINE')
command = ['./gen']
if machine == 'rv32i':
    command = ['qemu-riscv32', './gen']
if machine == 'rv32i-qemu':
    command = ['qemu-system-riscv32', '-machine', 'virt', '-bios', 'none',
               '-display', 'none', '-serial', 'stdio', '-monitor', 'none',
               '-kernel', 'gen-rv32i.bin']
p = subprocess.Popen(command,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE,
                           universal_newlines=True)

print(p.stdout.readline(), end='') # hello
print(p.stdout.readline(), end='') # enter name
p.stdin.write('bob\n')
p.stdin.flush()
print(p.stdout.readline(), end='') # not a name
print(p.stdout.readline(), end='') # enter name
p.stdin.write('charlie\n')
p.stdin.flush()
print(p.stdout.readline(), end='') # hello charlie
print(p.stdout.readline(), end='') # enter name
p.stdin.write('\n')
p.stdin.flush()

p.stdin.close()
p.stdout.close()
p.stderr.close()
p.wait()
