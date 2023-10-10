#!/usr/bin/python3

import subprocess

process = subprocess.Popen(['./gen'],
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE,
                           universal_newlines=True)

print(process.stdout.readline(), end='') # hello
print(process.stdout.readline(), end='') # enter name
process.stdin.write('bob\n')
process.stdin.flush()
print(process.stdout.readline(), end='') # not a name
print(process.stdout.readline(), end='') # enter name
process.stdin.write('charlie\n')
process.stdin.flush()
print(process.stdout.readline(), end='') # hello charlie
print(process.stdout.readline(), end='') # enter name
process.stdin.write('\n')
process.stdin.flush()

process.stdin.close()
process.stdout.close()
process.stderr.close()
process.wait()
