#!/usr/bin/python3

import os
import subprocess
import tempfile

# compiles a noinline function whose body is a chain of inlined calls, called
# at the end of a long chain of inlined calls, with and without a call of it
# in 'main' first, and prints the exit code and the message of each
machine = os.environ.get('MACHINE')
compiler = os.path.abspath('../../../baz')


def program(calls_first):
    lines = ['func g0(x) res {', '    res = x', '}', '']
    for i in range(1, 40):
        lines += [f'func g{i}(x) res {{', f'    res = g{i - 1}(x)', '}', '']
    lines += ['func noinline f() res {', '    res = g39(1)', '}', '',
              'func w0() res {', '    res = f()', '}', '']
    for i in range(1, 100):
        lines += [f'func w{i}() res {{', f'    res = w{i - 1}()', '}', '']
    lines += ['func main() {', '    var s = 0']
    if calls_first:
        lines += ['    s = f()']
    lines += ['    s = w99()', '    exit(s)', '}', '']
    return '\n'.join(lines)


with tempfile.TemporaryDirectory() as root:
    for calls_first in (False, True):
        source = os.path.join(root, 'order.baz')
        with open(source, 'w') as f:
            f.write(program(calls_first))
        result = subprocess.run(
            [compiler, 'order.baz', '--target=' + machine, '--bin=order.bin'],
            cwd=root, capture_output=True, text=True)
        lines = [line for line in result.stderr.splitlines() if line]
        print(result.returncode, *lines[:1])
