#!/usr/bin/python3

import os
import subprocess
import tempfile

# compiles a chain of generic functions, each calling the instance of the one
# before it, and a chain of included files, each including the next, and
# prints the exit code and the message of each
machine = os.environ.get('MACHINE')
compiler = os.path.abspath('../../../baz')


def generic_chain(length):
    lines = ['func g0<T type>(n T) res T {', '    res = n', '}', '']
    for i in range(1, length):
        lines += [f'func g{i}<T type>(n T) res T {{',
                  f'    res = g{i - 1}<T>(n)', '}', '']
    lines += ['func main() {', f'    exit(int(g{length - 1}<i8>(3)))', '}', '']
    return '\n'.join(lines)


def compile_source(root, name):
    result = subprocess.run(
        [compiler, name, '--target=' + machine, '--bin=chain.bin'],
        cwd=root, capture_output=True, text=True)
    lines = [line for line in result.stderr.splitlines() if line]
    print(result.returncode, *lines[:1])


with tempfile.TemporaryDirectory() as root:
    with open(os.path.join(root, 'generic.baz'), 'w') as f:
        f.write(generic_chain(200))
    compile_source(root, 'generic.baz')

    length = 200
    for i in range(1, length):
        with open(os.path.join(root, f'i{i}.baz'), 'w') as f:
            if i + 1 < length:
                f.write(f'include "i{i + 1}.baz"\n\n')
            f.write(f'func f{i}() {{\n}}\n')
    with open(os.path.join(root, 'include.baz'), 'w') as f:
        f.write('include "i1.baz"\n\nfunc main() {\n    exit(0)\n}\n')
    compile_source(root, 'include.baz')
