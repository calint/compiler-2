#!/usr/bin/python3

import os
import subprocess
import tempfile

# compiles a chain of noinline functions, each calling the one before it,
# where the first calls a chain of inlined functions that needs more frames
# than are left, and prints the exit code and the message: the frames run out
# at an inlined call, and at a noinline call that the last inlined function
# makes
machine = os.environ.get('MACHINE')
compiler = os.path.abspath('../../../baz')


def chain(length, inlined_length, calls_noinline):
    lines = []
    if calls_noinline:
        lines += ['func noinline h(x) res {', '    res = x', '}', '',
                  'func g0(x) res {', '    res = h(x)', '}', '']
    else:
        lines += ['func g0(x) res {', '    res = x', '}', '']
    for i in range(1, inlined_length):
        lines += [f'func g{i}(x) res {{', f'    res = g{i - 1}(x)', '}', '']
    lines += ['func noinline f0() res {', f'    res = g{inlined_length - 1}(1)',
              '}', '']
    for i in range(1, length):
        lines += [f'func noinline f{i}() res {{', f'    res = f{i - 1}()', '}',
                  '']
    lines += ['func main() {', f'    exit(f{length - 1}())', '}', '']
    return '\n'.join(lines)


with tempfile.TemporaryDirectory() as root:
    for length, inlined_length, calls_noinline in ((100, 40, False),
                                                   (99, 27, True)):
        source = os.path.join(root, 'chain.baz')
        with open(source, 'w') as f:
            f.write(chain(length, inlined_length, calls_noinline))
        result = subprocess.run(
            [compiler, 'chain.baz', '--target=' + machine, '--bin=chain.bin'],
            cwd=root, capture_output=True, text=True)
        lines = [line for line in result.stderr.splitlines() if line]
        print(result.returncode, *lines[:1])
