#!/usr/bin/env python3
# fuzzer of the frame and stack checks at tight limits: the programs of
# 'fuzz-programs.py' with every function 'noinline', built with a small
# '--vars' and a small '--stack'
#
# a program first runs on rv32i-fpga with the default limits, the full run.
# then it is built with two '--vars' of 64 B to 2 KiB on every target and
# with one '--stack' of 48 B to 512 B on the bare-metal targets. a build with
# a small limit must end like the full run, or panic 'frame overflow'
# respectively 'stack overflow' after a prefix of its output, or be rejected
# because its variables do not fit or, on x86_64, an expression needs more
# registers than it has; the rv32i targets must end alike at the same limit,
# x86_64 has larger frames:
#
#   limits-mismatch  a build is neither the full run nor the panic of its
#                    limit after a prefix of the output
#   limits-disagree  the rv32i targets end differently at the same limit
#   compiler-crash, compiler-hang, invalid-asm, link-failure, image-mismatch
#                    see fuzz-compiler.py
#
# usage: fuzz-limits.py [count] [seed] [jobs]
import importlib.util
import os
import random
import sys
import tempfile

import fuzzlib as fl

HERE = os.path.dirname(os.path.abspath(__file__))
VARIABLES = ((64, 128, 256, 512, 1024, 2048), (96, 160, 320, 768))
STACKS = (48, 64, 96, 128, 256, 512)
CHECKS = "--checks=noub"
# the stack of the bare-metal targets when '--vars' is tight, larger than
# any program here needs
STACK = "--stack=0x10000"
VARIABLES_REJECTED = "does not fit in the variable storage"


def load_programs():
    # 'fuzz-programs.py', whose name is no module name
    spec = importlib.util.spec_from_file_location(
        "fuzz_programs", os.path.join(HERE, "fuzz-programs.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


programs = load_programs()


def outcome(work, path, target, options, found):
    # the outcome of 'path' on 'target' as a comparable tuple, a finding of
    # the compiler or the tools added to 'found'; None for a run that says
    # nothing
    c, o, problem = fl.run_target(fl.COMPILER_ASAN, path, list(options),
                                  work, target)
    crash = fl.classify_compile(c)
    if crash:
        found.append((crash[0], f"{target} {crash[1]}",
                      f"{' '.join(options)}\n{c.stderr[-1500:]}"))
        return None
    if c.returncode:
        message = c.stderr.strip().splitlines()[0] if c.stderr.strip() else ""
        return ("compile", None, message.split(": ", 1)[-1], b"")
    if problem and problem[0] == "skip":
        return None
    if problem:
        found.append((problem[0], f"{target} {problem[1][:60]}",
                      f"{' '.join(options)}\n{problem[1][-1500:]}"))
        return None
    return (o.kind, o.code, o.detail, o.stdout)


def within_limit(got, full, panic):
    # 'got' is the full run, the panic of the limit after a prefix of the
    # output, the rejection of variables that do not fit or of an expression
    # that needs more registers than the target has (x86_64, the full run is
    # that of rv32i-fpga)
    if got[0] == "compile":
        return VARIABLES_REJECTED in got[2] or fl.out_of_registers(got[2])
    if got == full:
        return True
    return got[0] == "panic" and got[2] == panic and \
        full[3].startswith(got[3])


def options_of(target, limit):
    if limit.startswith("--vars=") and target in fl.UART_TARGETS:
        return (CHECKS, limit, STACK)
    return (CHECKS, limit)


def check_limit(work, path, targets, limit, panic, full, found, counts):
    # builds the program with the option 'limit' for 'targets', whose
    # overflow is the panic 'panic'
    results = {}
    for target in targets:
        options = options_of(target, limit)
        got = outcome(work, path, target, options, found)
        if got is None:
            continue
        results[target] = got
        kind = "full" if got == full else \
            "rejected" if got[0] == "compile" else got[2] or got[0]
        counts[kind] = counts.get(kind, 0) + 1
        if not within_limit(got, full, panic):
            found.append(("limits-mismatch",
                          f"{target} {panic} {got[0]} {got[2]}",
                          f"{' '.join(options)}\n"
                          f"got {got[:3]} {got[3][:200]!r}\n"
                          f"full {full[:3]} {full[3][:200]!r}"))
    rv32i = {t: r for t, r in results.items() if t != "x86_64"}
    if len(set(rv32i.values())) > 1:
        found.append(("limits-disagree",
                      f"{panic} {sorted(set(r[:3] for r in rv32i.values()))}",
                      f"{limit}\n" +
                      "\n".join(f"{t} {r[:3]} {r[3][:200]!r}"
                                for t, r in rv32i.items())))


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    source = programs.with_inlining(programs.Generator(rnd).program(), True)
    found = []
    counts = {}
    with tempfile.TemporaryDirectory(prefix="fuzz-") as work:
        path = os.path.join(work, "f.baz")
        with open(path, "w") as f:
            f.write(source)
        full = outcome(work, path, "rv32i-fpga", (CHECKS,), found)
        # note: 'fuzz-programs.py' reports a program that does not end
        if full is None or full[0] not in ("exit", "panic"):
            return source, found, "skipped"
        for choices in VARIABLES:
            limit = f"--vars={rnd.choice(choices)}"
            check_limit(work, path, ("x86_64",) + fl.ROTATED_TARGETS +
                        ("rv32i-fpga",), limit, "frame overflow", full, found,
                        counts)
        limit = f"--stack={rnd.choice(STACKS)}"
        check_limit(work, path, fl.UART_TARGETS, limit, "stack overflow",
                    full, found, counts)
    return source, found, counts


def main():
    count, seed, jobs = fl.arguments(250)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
