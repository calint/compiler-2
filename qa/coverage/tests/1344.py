#!/usr/bin/python3

import os
import subprocess
import tempfile

# compiles a chain of noinline functions longer than the limit, each calling
# the one before it, called from 'main' in ascending order, in descending
# order and only the last, and prints the exit code and the message of each
machine = os.environ.get('MACHINE')
compiler = os.path.abspath('../../../baz')
length = 200


def chain(calls):
    lines = ['func noinline f0() res {', '    res = 1', '}', '']
    for i in range(1, length):
        lines += [f'func noinline f{i}() res {{', f'    res = f{i - 1}()', '}',
                  '']
    lines += ['func main() {', '    var s = 0']
    lines += [f'    s = s + f{i}()' for i in calls]
    lines += ['    exit(s)', '}', '']
    return '\n'.join(lines)


orders = {
    'ascending': range(length),
    'descending': reversed(range(length)),
    'only-last': [length - 1],
}

with tempfile.TemporaryDirectory() as root:
    for name, calls in orders.items():
        source = os.path.join(root, 'chain.baz')
        with open(source, 'w') as f:
            f.write(chain(calls))
        result = subprocess.run(
            [compiler, 'chain.baz', '--target=' + machine,
             '--bin=chain.bin'], cwd=root, capture_output=True, text=True)
        lines = [line for line in result.stderr.splitlines() if line]
        print(name, result.returncode, *lines[:1])
