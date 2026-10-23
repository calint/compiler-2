#!/usr/bin/env python3
# checks that no format string of the C++ files under 'src/' puts quotes
# around a placeholder, e.g. "type '{}'"; a message quotes a source text or a
# name through 'in_quotes' ('src/compile_error.hpp'), which does not quote a
# character literal again
# usage: qa/lint/check-messages.py
# exits 1 and prints each call when there is one
import os
import re
import sys

# the calls that take a format string
CALLS = re.compile(r'(std::format|std::println|std::print|\bcomment)\s*(<[^<>()]*>)?\s*\(')

# 'in_quotes' itself is the one place that quotes
EXEMPT = {'compile_error.hpp'}


def skip_literal(src, ix):
    # 'ix' at the opening quote of a string or character literal, returns the
    # index after its closing quote
    quote = src[ix]
    ix += 1
    while src[ix] != quote:
        if src[ix] == '\\':
            ix += 1
        ix += 1
    return ix + 1


def format_string(src, ix):
    # the format string of the call whose '(' ends at 'ix': the first argument
    # made only of string literals, joined, or None
    depth = 0
    literal = None
    only_literals = True
    while True:
        ch = src[ix]
        if ch == '"' and depth == 0:
            end = skip_literal(src, ix)
            literal = (literal or '') + src[ix + 1 : end - 1]
            ix = end
            continue
        if ch in '"\'':
            ix = skip_literal(src, ix)
            continue
        if src.startswith('//', ix):
            ix = src.index('\n', ix)
            continue
        if ch.isspace():
            ix += 1
            continue
        if depth == 0 and ch in ',)':
            if literal is not None and only_literals:
                return literal
            if ch == ')':
                return None
            literal = None
            only_literals = True
            ix += 1
            continue
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        only_literals = False
        ix += 1


def main():
    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src'))
    found = 0
    for name in sorted(os.listdir('.')):
        if name in EXEMPT or not name.endswith(('.hpp', '.cpp')):
            continue
        src = open(name).read()
        for match in CALLS.finditer(src):
            text = format_string(src, match.end())
            if text is None or not re.search(r"'\{[^{}]*\}'", text):
                continue
            line = src.count('\n', 0, match.start()) + 1
            print(f"src/{name}:{line}: quotes around a placeholder, use 'in_quotes'")
            found += 1
    sys.exit(1 if found else 0)


main()
