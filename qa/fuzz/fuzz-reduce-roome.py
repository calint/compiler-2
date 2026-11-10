#!/usr/bin/env python3
# reduces a session of 'fuzz-roome.py' that makes the x86_64 build of roome
# print a given text on stderr
# usage: fuzz-reduce-roome.py SESSION TEXT
import os
import subprocess
import sys
import tempfile

import fuzzlib as fl

# the build of 'fuzz-roome.py'
X86 = os.path.join(fl.HERE, "build", "roome-x86.x")
# the size of the sd card image, as in 'fuzz-roome.py'
SDCARD_SIZE_BYTES = 1 << 20


def shows(lines, text):
    data = b"\n".join(lines) + b"\n"
    # the program opens 'sdcard.img' in its working directory, a new one per
    # run like in 'fuzz-roome.py'
    with tempfile.TemporaryDirectory() as work:
        with open(os.path.join(work, "sdcard.img"), "wb") as f:
            f.truncate(SDCARD_SIZE_BYTES)
        try:
            r = subprocess.run([X86], input=data, capture_output=True,
                               timeout=5, cwd=work)
        except subprocess.TimeoutExpired:
            return False
    return text.encode() in r.stderr


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: fuzz-reduce-roome.py SESSION TEXT")
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


if __name__ == "__main__":
    main()
