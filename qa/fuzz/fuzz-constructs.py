#!/usr/bin/env python3
# one small program per construct of 'etc/tutorial.baz', see 'constructs.py'
#
# every program ends with exit code 0 and the bytes of the model on both
# targets, with and without checks and without optimization. a program of
# the rejected list must be rejected on the line that ends with '# error'.
# the tutorial itself must run to the end
#
#   construct-mismatch   a build ends differently from the model
#   construct-reject     a valid program is rejected
#   construct-accepted   a program with an error is accepted
#   construct-position   the error is reported on another line
#
# usage: fuzz-constructs.py [rounds] [seed] [jobs]
# a round builds every program once, rounds only repeat the run
import os
import re
import shutil
import sys
import tempfile

import fuzzlib as fl
import constructs as cs

OPTION_LISTS = ((), ("--checks=noub",), ("--checks=noub", "--nopt"))
TUTORIAL = os.path.join(fl.ROOT, "etc", "tutorial.baz")
TUTORIAL_STDOUT = b"hello from the tutorial\naz\n"


def write_files(work, files):
    for path, text in files.items():
        full = os.path.join(work, path)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, "w") as f:
            f.write(text)
    return os.path.join(work, "main.baz")


def build(work, main, target, options, stdin=b""):
    # (compile result, outcome, problem)
    if target == "x86_64":
        return fl.run_x86(fl.COMPILER_ASAN, main, list(options), work,
                         stdin=stdin)
    c, o = fl.run_fpga(fl.COMPILER_ASAN, main, list(options), work,
                       stdin=stdin)
    return c, o, None


def check_valid(work, construct, found):
    main = write_files(work, construct.files)
    source = construct.files["main.baz"]
    expected = fl.Outcome("exit", 0, construct.stdout)
    for target in construct.targets:
        for options in OPTION_LISTS:
            options = list(options) + list(construct.options)
            key = f"{construct.name} {target} {' '.join(options)}"
            c, o, problem = build(work, main, target, options, construct.stdin)
            if c.timed_out:
                found.append(("compiler-hang", key, "timeout"))
            elif c.returncode:
                found.append(("construct-reject", key, c.stderr[:300]))
            elif problem:
                found.append((problem[0], key, problem[1][:300]))
            elif not o.same(expected):
                found.append(("construct-mismatch", key,
                              f"expected {expected}, got {o}"))
    return source


def check_rejected(work, construct, found):
    main = write_files(work, construct.files)
    source = construct.files["main.baz"]
    line = next(n for n, text in enumerate(source.split("\n"), 1)
                if text.endswith("# error"))
    for target in construct.targets:
        key = f"{construct.name} {target}"
        c = fl.compile_source(
            fl.COMPILER_ASAN, main,
            ["--target=" + target, "--checks=noub"] + list(construct.options),
            os.devnull)
        if c.returncode == 0:
            found.append(("construct-accepted", key, source))
            continue
        lines = [int(m.group(1)) for m in
                 re.finditer(r"^.+?:(\d+):\d+: ", c.stderr, re.M)]
        if not lines:
            found.append(("construct-position", key, c.stderr[:300]))
        elif lines[0] != line:
            found.append(("construct-position", key,
                          f"expected line {line}, got {lines[0]}: "
                          f"{c.stderr[:200]}"))
    return source


def check_tutorial(work, found):
    expected = fl.Outcome("exit", 0, TUTORIAL_STDOUT)
    source = open(TUTORIAL).read()
    for target in fl.TARGETS:
        for options in OPTION_LISTS:
            key = f"tutorial {target} {' '.join(options)}"
            c, o, problem = build(work, TUTORIAL, target, options)
            if c.returncode or c.timed_out:
                found.append(("construct-reject", key, c.stderr[:300]))
            elif problem:
                found.append((problem[0], key, problem[1][:300]))
            elif not o.same(expected):
                found.append(("construct-mismatch", key,
                              f"expected {expected}, got {o}"))
    return source


def fuzz_one(args):
    _, index = args
    everything = [("tutorial", None)] + \
        [("valid", c) for c in cs.CONSTRUCTS] + \
        [("rejected", c) for c in cs.REJECTED]
    kind, construct = everything[index % len(everything)]
    work = tempfile.mkdtemp(prefix="fuzzc-")
    found = []
    try:
        if kind == "tutorial":
            source = check_tutorial(work, found)
        elif kind == "valid":
            source = check_valid(work, construct, found)
        else:
            source = check_rejected(work, construct, found)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def main():
    total = 1 + len(cs.CONSTRUCTS) + len(cs.REJECTED)
    rounds, seed, jobs = fl.arguments(1)
    return fl.run_fuzzer(fuzz_one, rounds * total, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
