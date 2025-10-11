#!/usr/bin/python3

import subprocess

p = subprocess.Popen(['./gen'],
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE,
                           universal_newlines=True)

print(p.stdout.readline(), end='') # ok
p.stdin.write('array equal\n')
p.stdin.flush()
print(p.stdout.readline(), end='') # input 1 
p.stdin.write('array equal\n')
p.stdin.flush()
print(p.stdout.readline(), end='') # input 2 
print(p.stdout.readline(), end='') # ok 
p.stdin.write('true\n')
p.stdin.flush()
print(p.stdout.readline(), end='') # true 
print(p.stdout.readline(), end='') # ok
p.stdin.write('false\n')
p.stdin.flush()
print(p.stdout.readline(), end='') # true 
print(p.stdout.readline(), end='') # false
print(p.stdout.readline(), end='') # ok

p.stdin.close()
p.stdout.close()
p.stderr.close()
p.wait()
