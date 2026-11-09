#!/usr/bin/python3

import os
import subprocess
import tempfile

# compiles the source and the source without the functions after the marker,
# each in a directory of its own under the same name, with every check and
# the register report, and compares the outputs
machine = os.environ.get('MACHINE')
# x86_64 writes no image and takes no '--bin'
image_options = [] if machine == 'x86_64' else ['--bin=1206.bin']
compiler = os.path.abspath('../../../baz')
source = open('1206.baz').read()
marker = '# not called'
called_only = source[:source.index(marker)]
options = ['--target=' + machine, '--vars=0x40000',
           '--checks=upper,lower,line,overflow,division,shift,frame',
           '--report=registers']

def compile_in(directory, text):
    os.mkdir(directory)
    with open(os.path.join(directory, '1206.baz'), 'w') as f:
        f.write(text)
    result = subprocess.run(
        [compiler, '1206.baz', *options, *image_options], cwd=directory,
        capture_output=True, text=True)
    image = os.path.join(directory, '1206.bin')
    binary = open(image, 'rb').read() if os.path.exists(image) else b''
    return result.returncode, result.stdout, result.stderr, binary

with tempfile.TemporaryDirectory() as root:
    whole = compile_in(os.path.join(root, 'whole'), source)
    without = compile_in(os.path.join(root, 'without'), called_only)

print('compiled:', whole[0] == 0 and without[0] == 0)
print('same assembly:', whole[1] == without[1])
print('same messages:', whole[2] == without[2])
print('same image:', whole[3] == without[3])
