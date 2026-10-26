#!/usr/bin/env python3
# checks the include rule of the layer table in 'etc/docs/README.md' section
# 1.1: a header includes only headers of its own layer or of a lower one;
# also reports a file of 'src/' that the table does not list and a name of
# the table without a file
# usage: qa/lint/check-layers.py
# exits 1 and prints each problem when there is one
import glob
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
README = os.path.join(ROOT, 'etc', 'docs', 'README.md')
SRC = os.path.join(ROOT, 'src')

# a row of the table, e.g. '1. vocabulary        token, lut, ...'
ROW = re.compile(r'(\d)\.\s+[a-z ]+?\s{2,}(.*)')
INCLUDE = re.compile(r'\s*#include\s+"([\w/]+)\.hpp"')


def read_layers():
    readme = open(README).read()
    block = readme.split('The headers in layers')[1]
    block = block.split('```text')[1].split('```')[0]

    layers = {}
    current = None

    for line in block.split('\n'):
        row = ROW.match(line)
        rest = line

        if row:
            current = int(row.group(1))
            rest = row.group(2)

        for name in re.findall(r'[\w.*]+', rest):
            layers[name.replace('.cpp', '')] = current

    return layers


def layer_of(layers, name):
    if name in layers:
        return layers[name]

    # e.g. 'stmt_*'
    for pattern, layer in layers.items():
        if '*' in pattern and re.fullmatch(pattern.replace('*', '.*'), name):
            return layer

    return None


def main():
    layers = read_layers()
    problems = []

    files = sorted(glob.glob(os.path.join(SRC, '*.hpp')) +
                   glob.glob(os.path.join(SRC, '*.cpp')))

    for path in files:
        base = os.path.basename(path)
        name = base.rsplit('.', 1)[0]
        own = layer_of(layers, name)

        if own is None:
            problems.append(f'src/{base}: not in the layer table')
            continue

        for number, line in enumerate(open(path), start=1):
            include = INCLUDE.match(line)

            if not include:
                continue

            target = layer_of(layers, include.group(1))

            if target is None:
                problems.append(f'src/{base}:{number}: includes '
                                f'{include.group(1)}, not in the table')
            elif target > own:
                problems.append(f'src/{base}:{number}: layer {own} includes '
                                f'{include.group(1)} of layer {target}')

    for name in layers:
        exists = any(os.path.exists(os.path.join(SRC, name + extension))
                     for extension in ('.hpp', '.cpp'))

        if '*' not in name and not exists:
            problems.append(f'{name}: in the layer table without a file')

    for problem in problems:
        print(problem)

    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
