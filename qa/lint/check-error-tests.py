#!/usr/bin/env python3
# checks that each error of the C++ files under 'src/' (a 'compile_error', an
# 'unlocated_error', an 'option_error') is the expectation of a test: the
# longest fixed part of its message, between the '{}' placeholders, appears in
# an expected output of 'qa/coverage/tests', in a line of
# 'qa/coverage/test-cli.sh' that is not a comment or in
# 'qa/coverage/test-arena.py'; code that user input cannot reach is an
# 'assert' (AGENTS.md), so every message has a test
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
ERROR = re.compile(r'(?:compile_error|unlocated_error|option_error)\s*'
                   r'(?:error\s*)?\{|compile_error::file_level\(')

# the messages that only a suite outside the "enough" set expects, with why
EXCEPTIONS = {
    # 'test-far-jumps-no-scratch.sh' needs a jump past 1 MiB, too slow under
    # the sanitizers of 'test-all.sh'
    'exceeds 1 MiB and no scratch register is free',
}
LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')

# a fixed part shorter than this is too common to tell a message
MIN_FRAGMENT = 6

# the lines after the error that may hold its message
MESSAGE_LINES = 6


# the fixed parts of a message between its '{}' placeholders
def parts_of(message):
    return [p.strip() for p in re.split(r'\{[^{}]*\}', message)]


def fragment_of(message):
    longest = max(parts_of(message), key=len)
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
    paths = glob.glob(os.path.join(SRC, '*.hpp'))
    paths += glob.glob(os.path.join(SRC, '*.cpp'))
    for path in sorted(paths):
        with open(path, encoding='utf-8') as f:
            lines = f.read().split('\n')
        for ix, line in enumerate(lines):
            if not ERROR.search(line):
                continue
            text = '\n'.join(lines[ix:ix + MESSAGE_LINES])
            # the message ends at the brace that closes the error
            text = text[ERROR.search(text).end():]
            text = re.split(r'\}\s*;|\)\s*;', text)[0]
            for string in strings_of(text):
                fragment = fragment_of(string)
                if fragment:
                    found.append((os.path.basename(path), ix + 1, fragment,
                                  parts_of(string)))
    return found


# the texts that 'test-cli.sh' looks for in an error with 'grep -F', each a
# part of a message, e.g. "of device memory"
def grepped():
    with open(os.path.join(COVERAGE, 'test-cli.sh'), encoding='utf-8') as f:
        return [g for g in re.findall(r'grep -F\w* "([^"]+)"', f.read())
                if len(g) >= MIN_EXPECTED // 2]


def corpus():
    texts = []
    for pattern in ('tests/*.out', 'tests/*.diff'):
        for path in glob.glob(os.path.join(COVERAGE, pattern)):
            with open(path, encoding='utf-8', errors='replace') as f:
                texts.append(f.read())
    with open(os.path.join(COVERAGE, 'test-cli.sh'), encoding='utf-8') as f:
        texts += [line for line in f if not line.lstrip().startswith('#')]
    with open(os.path.join(COVERAGE, 'test-arena.py'), encoding='utf-8') as f:
        texts.append(f.read())
    return '\n'.join(texts)


# a test may expect a part of the message, e.g. 'grep -Fq' of its end: a run
# of whole words of the fragment of at least 'MIN_EXPECTED' characters, or the
# whole fragment when it is shorter
MIN_EXPECTED = 20


def is_expected(fragment, text):
    words = fragment.split(' ')
    least = min(MIN_EXPECTED, len(fragment))
    for first in range(len(words)):
        for last in range(len(words), first, -1):
            part = ' '.join(words[first:last])
            if len(part) < least:
                break
            if part in text:
                return True
    return False


def main():
    text = corpus()
    greps = grepped()
    missing = [m for m in messages()
               if m[2] not in EXCEPTIONS and not is_expected(m[2], text)
               and not any(g in part for g in greps for part in m[3])]
    for file_name, line_no, fragment, _ in missing:
        print(f'{file_name}:{line_no}: no test expects "{fragment}"')
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main())
