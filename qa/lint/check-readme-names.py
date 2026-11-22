#!/usr/bin/env python3
# checks that each code name quoted in 'etc/docs/README.md' still exists under
# 'src/': a quoted identifier with an underscore or a 'scope::name', e.g.
# 'toc::storage_of' or 'plan_writes'; a quoted baz text without an underscore
# is not checked; and that each test of the column 'tests' of the catalog
# (8.1) is a case of 'qa/coverage/cases.sh', e.g. '0295' or '1289-1291'; and
# that the reading order of 1.1 names every file of 'src/', the built-ins as
# 'stmt_builtin_*.hpp'
# usage: qa/lint/check-readme-names.py
# exits 1 and prints each missing name or test when there is one
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
README = os.path.join(ROOT, 'etc', 'docs', 'README.md')
SRC = os.path.join(ROOT, 'src')
CASES = os.path.join(ROOT, 'qa', 'coverage', 'cases.sh')

# a 'scope::name' must also name a member of the class, struct, enum or
# namespace 'scope', e.g. 'toc::fold_let' fails when only another class of
# 'src/' has a 'fold_let'; a scope that 'src/' does not define, e.g. a function
# 'plan_writes::x', fails too, except these
EXTERNAL_SCOPES = {'std'}

# a whole quoted text that is a name, a scoped name or a file name
QUOTED = re.compile(r"'([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_~][A-Za-z0-9_]*)*(?:\.[ch]pp)?)'")
WORD = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')


def source_texts():
    texts = {}
    for name in os.listdir(SRC):
        with open(os.path.join(SRC, name), encoding='utf-8') as f:
            texts[name] = f.read()
    return texts


def source_words(texts):
    words = set(texts)
    for text in texts.values():
        words.update(WORD.findall(text))
    return words


# the code of a source without comments and literals, whose braces would
# end a body early or late
CODE_NOISE = re.compile(r'//[^\n]*|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'')


# the bodies of the definitions of 'scope', none when 'src/' defines none
def scope_bodies(texts, scope):
    head = re.compile(r'\b(?:class|struct|enum|namespace)\s+(?:class\s+)?'
                      + re.escape(scope) + r'\b[^;{()]*\{')
    bodies = []
    for source in texts.values():
        text = CODE_NOISE.sub('', source)
        for match in head.finditer(text):
            end = match.end()
            depth = 1
            while depth and end < len(text):
                if text[end] == '{':
                    depth += 1
                elif text[end] == '}':
                    depth -= 1
                end += 1
            bodies.append(text[match.end():end])
    return bodies


# each 'scope::member' pair of a scoped name names a member of its scope, in
# the body of the scope or in a definition out of it ('scope::member')
def is_member_of_scopes(texts, name):
    parts = name.split('::')
    for scope, member in zip(parts, parts[1:]):
        if scope in EXTERNAL_SCOPES:
            continue
        bodies = scope_bodies(texts, scope)
        word = re.compile(r'\b' + re.escape(member.lstrip('~')) + r'\b')
        out_of_body = f'{scope}::{member}'
        if not any(word.search(b) for b in bodies) and not any(
                out_of_body in t for t in texts.values()):
            return False
    return True


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
        for token in re.findall(r'\d+(?:-\d+)?', column):
            bounds = token.split('-')
            numbers = range(int(bounds[0]), int(bounds[-1]) + 1)
            # e.g. '296' or '1291-1289', which name no case
            if not re.fullmatch(r'\d{4}(?:-\d{4})?', token) or not numbers:
                tests.append((line_no + 1, token))
                continue
            tests += [(line_no + 1, f'{n:04}') for n in numbers]
    return tests


def missing_tests():
    with open(CASES, encoding='utf-8') as f:
        cases = set(re.findall(r'^[^#\n]*\bSRC=(\d{4})', f.read(), re.M))
    return [f'README.md:{line_no}: test {n} is not a case'
            for line_no, n in catalog_tests() if n not in cases]


# the files of 'src/' that the reading order of 1.1 does not name
def missing_from_reading_order():
    with open(README, encoding='utf-8') as f:
        text = f.read()
    start = text.index('Reading order, ')
    block = text[start:text.index('```', text.index('```text', start) + 3)]
    named = set(re.findall(r'[a-z0-9_]+\.[ch]pp', block))
    return [f'README.md: reading order of 1.1 does not name {name}'
            for name in sorted(os.listdir(SRC))
            if name not in named and not (name.startswith('stmt_builtin_')
                                          and 'stmt_builtin_*.hpp' in block)]


def main():
    texts = source_texts()
    words = source_words(texts)
    missing = missing_tests() + missing_from_reading_order()
    with open(README, encoding='utf-8') as f:
        for line_no, line in enumerate(f, start=1):
            for name in QUOTED.findall(line):
                if name.endswith(('.hpp', '.cpp')):
                    parts = [name]
                elif '::' in name or '_' in name:
                    parts = name.split('::')
                else:
                    continue
                if not all(p in words for p in parts) or (
                        '::' in name and not is_member_of_scopes(texts, name)):
                    missing.append(f'README.md:{line_no}: {name}')
    for m in missing:
        print(m)
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main())
