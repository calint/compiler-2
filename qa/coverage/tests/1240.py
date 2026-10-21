#!/usr/bin/python3

import os
import subprocess
import tempfile

# compiles a chain of noinline functions, each calling the one before it, of
# the length of the limit and longer, and prints the exit code and the message
machine = os.environ.get('MACHINE')
compiler = os.path.abspath('../../../baz')


def chain(length):
    lines = ['func noinline f0() res {', '    res = 1', '}', '']
    for i in range(1, length):
        lines += [f'func noinline f{i}() res {{', f'    res = f{i - 1}()', '}',
                  '']
    lines += ['func main() {', f'    exit(f{length - 1}())', '}', '']
    return '\n'.join(lines)


with tempfile.TemporaryDirectory() as root:
    for length in (100, 101, 1000):
        source = os.path.join(root, 'chain.baz')
        with open(source, 'w') as f:
            f.write(chain(length))
        result = subprocess.run(
            [compiler, 'chain.baz', '--target=' + machine,
             '--bin=chain.bin'], cwd=root, capture_output=True, text=True)
        lines = [line for line in result.stderr.splitlines() if line]
        print(length, result.returncode, *lines[:1])
