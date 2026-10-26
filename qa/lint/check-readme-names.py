#!/usr/bin/env python3
# checks that each code name quoted in 'etc/docs/README.md' still exists under
# 'src/': a quoted identifier with an underscore or a 'scope::name', e.g.
# 'toc::storage_of' or 'plan_writes'; a quoted baz text without an underscore
# is not checked; and that each test of the column 'tests' of the catalog
# (8.1) is a case of 'qa/coverage/cases.sh', e.g. '0302' or '1301-1303'
# usage: qa/lint/check-readme-names.py
# exits 1 and prints each missing name or test when there is one
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
README = os.path.join(ROOT, 'etc', 'docs', 'README.md')
SRC = os.path.join(ROOT, 'src')
CASES = os.path.join(ROOT, 'qa', 'coverage', 'cases.sh')

# a whole quoted text that is a name, a scoped name or a file name
QUOTED = re.compile(r"'([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_~][A-Za-z0-9_]*)*(?:\.[ch]pp)?)'")
WORD = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')


def source_words():
    words = set()
    for name in os.listdir(SRC):
        words.add(name)
        with open(os.path.join(SRC, name), encoding='utf-8') as f:
            words.update(WORD.findall(f.read()))
    return words


# the numbers of the column 'tests' of the catalog with their line
def catalog_tests():
    tests = []
    with open(README, encoding='utf-8') as f:
        lines = f.read().split('\n')
    start = next(i for i, l in enumerate(lines) if l.startswith('### 8.1 '))
    for line_no in range(start + 1, len(lines)):
        line = lines[line_no]
        if line.startswith('#'):
            break
        if not line.startswith('|'):
            continue
        column = line.rstrip(' |').split('|')[-1]
        for first, last in re.findall(r'\b(\d{4})(?:-(\d{4}))?\b', column):
            for n in range(int(first), int(last or first) + 1):
                tests.append((line_no + 1, f'{n:04}'))
    return tests


def missing_tests():
    with open(CASES, encoding='utf-8') as f:
        cases = set(re.findall(r'^[^#\n]*\bSRC=(\d{4})', f.read(), re.M))
    return [f'README.md:{line_no}: test {n} is not a case'
            for line_no, n in catalog_tests() if n not in cases]


def main():
    words = source_words()
    missing = missing_tests()
    with open(README, encoding='utf-8') as f:
        for line_no, line in enumerate(f, start=1):
            for name in QUOTED.findall(line):
                if name.endswith(('.hpp', '.cpp')):
                    parts = [name]
                elif '::' in name or '_' in name:
                    parts = name.split('::')
                else:
                    continue
                if not all(p in words for p in parts):
                    missing.append(f'README.md:{line_no}: {name}')
    for m in missing:
        print(m)
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main())
