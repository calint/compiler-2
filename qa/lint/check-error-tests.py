#!/usr/bin/env python3
# checks that each compile error of the C++ files under 'src/' is the
# expectation of a test: the longest fixed part of its message, between the
# '{}' placeholders, appears in an expected output of 'qa/coverage/tests', in
# 'qa/coverage/test-cli.sh' or in 'qa/coverage/test-arena.py'; code that user
# input cannot reach is an 'assert' (AGENTS.md), so every message has a test
# usage: qa/lint/check-error-tests.py
# exits 1 and prints each message without a test when there is one
import glob
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
SRC = os.path.join(ROOT, 'src')
COVERAGE = os.path.join(ROOT, 'qa', 'coverage')

# where an error is made: the literals of the next lines are its message
ERROR = re.compile(r'compile_error\s*(?:error\s*)?\{')
LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')

# a fixed part shorter than this is too common to tell a message
MIN_FRAGMENT = 6

# the lines after the error that may hold its message
MESSAGE_LINES = 6


def fragment_of(message):
    parts = re.split(r'\{[^{}]*\}', message)
    longest = max(parts, key=len).strip()
    return longest if len(longest) >= MIN_FRAGMENT else None


# the messages in 'text': literals apart only by whitespace are one string,
# e.g. a message split over lines, others are alternatives, e.g. of a '?:'
def strings_of(text):
    strings = []
    end = None
    for match in LITERAL.finditer(text):
        if end is not None and text[end:match.start()].strip() == '':
            strings[-1] += match.group(1)
        else:
            strings.append(match.group(1))
        end = match.end()
    return strings


def messages():
    found = []
    for path in sorted(glob.glob(os.path.join(SRC, '*.hpp'))):
        with open(path, encoding='utf-8') as f:
            lines = f.read().split('\n')
        for ix, line in enumerate(lines):
            if not ERROR.search(line):
                continue
            text = '\n'.join(lines[ix:ix + MESSAGE_LINES])
            # the message ends at the brace that closes the error
            text = text[ERROR.search(text).end():]
            text = text.split('};')[0]
            for string in strings_of(text):
                fragment = fragment_of(string)
                if fragment:
                    found.append((os.path.basename(path), ix + 1, fragment))
    return found


def corpus():
    texts = []
    for pattern in ('tests/*.out', 'tests/*.diff'):
        for path in glob.glob(os.path.join(COVERAGE, pattern)):
            with open(path, encoding='utf-8', errors='replace') as f:
                texts.append(f.read())
    for name in ('test-cli.sh', 'test-arena.py'):
        with open(os.path.join(COVERAGE, name), encoding='utf-8') as f:
            texts.append(f.read())
    return '\n'.join(texts)


def main():
    text = corpus()
    missing = [m for m in messages() if m[2] not in text]
    for file_name, line_no, fragment in missing:
        print(f'{file_name}:{line_no}: no test expects "{fragment}"')
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main())
