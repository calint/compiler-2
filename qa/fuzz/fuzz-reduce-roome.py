#!/usr/bin/env python3
# reduces a session of 'fuzz-roome.py' that makes the x86_64 build of roome
# print a given text on stderr
# usage: fuzz-reduce-roome.py SESSION TEXT
import subprocess
import sys

X86 = __file__.rsplit("/", 1)[0] + "/build/roome-x86"


def shows(lines, text):
    data = b"\n".join(lines) + b"\n"
    try:
        r = subprocess.run([X86], input=data, capture_output=True, timeout=5)
    except subprocess.TimeoutExpired:
        return False
    return text.encode() in r.stderr


def main():
    path, text = sys.argv[1], sys.argv[2]
    lines = open(path, "rb").read().split(b"\n")
    if not shows(lines, text):
        sys.exit("the session does not show the text")
    chunk = max(1, len(lines) // 2)
    while chunk >= 1:
        i = 0
        while i < len(lines):
            candidate = lines[:i] + lines[i + chunk:]
            if shows(candidate, text):
                lines = candidate
            else:
                i += chunk
        chunk //= 2
    for line in lines:
        print(repr(line[:120]))


main()
