#!/usr/bin/python3

import os
import subprocess

# compiles '1370.baz' and prints the lines that name 'rcx' and the exit code
compiler = os.path.abspath('../../../baz')

result = subprocess.run([compiler, '1370.baz', '--target=x86_64'],
                        capture_output=True, text=True)
for line in result.stdout.splitlines():
    if 'rcx' in line:
        print(line.strip())
print(result.returncode)
