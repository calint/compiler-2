#!/usr/bin/env python3
# checks that each code name quoted in 'etc/docs/README.md' still exists under
# 'src/': a quoted identifier with an underscore or a 'scope::name', e.g.
# 'toc::storage_of' or 'plan_writes'; a quoted baz text without an underscore
# is not checked
# usage: qa/lint/check-readme-names.py
# exits 1 and prints each missing name when there is one
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
README = os.path.join(ROOT, 'etc', 'docs', 'README.md')
SRC = os.path.join(ROOT, 'src')

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


def main():
    words = source_words()
    missing = []
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
