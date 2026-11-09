#!/usr/bin/python3

import os
import subprocess
import tempfile

# compiles a chain of noinline functions as long as the limit, each calling
# an inlined function before the one before it, and prints the exit code
# and the message
machine = os.environ.get('MACHINE')
# x86_64 writes no image and takes no '--bin'
image_options = [] if machine == 'x86_64' else ['--bin=chain.bin']
compiler = os.path.abspath('../../../baz')


def chain(length):
    lines = ['func id(x) res {', '    res = x', '}', '',
             'func noinline f0() res {', '    res = 1', '}', '']
    for i in range(1, length):
        lines += [f'func noinline f{i}() res {{', '    var v = 1',
                  '    var w = id(v)', f'    res = f{i - 1}()', '}', '']
    lines += ['func main() {', f'    exit(f{length - 1}())', '}', '']
    return '\n'.join(lines)


with tempfile.TemporaryDirectory() as root:
    source = os.path.join(root, 'chain.baz')
    with open(source, 'w') as f:
        f.write(chain(100))
    result = subprocess.run(
        [compiler, 'chain.baz', '--target=' + machine, *image_options],
        cwd=root, capture_output=True, text=True)
    lines = [line for line in result.stderr.splitlines() if line]
    print(result.returncode, *lines[:1])
