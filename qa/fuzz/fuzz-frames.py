#!/usr/bin/env python3
# model based fuzzer of the frame check of noinline calls, after 4.1 of
# 'etc/docs/README.md'
#
# builds a noinline recursion 'rec' with a 'mut' unsized array parameter and
# random locals of i8, i16 and i32 and arrays of them up to 4097 elements,
# declared before and after the recursive call, and locals in 'main' before
# the first call. python lays the frames out as the compiler does (each frame
# after the storage of its caller, aligned to the size of an address) and
# finds the depth at which a frame passes '--vars'; the program is built with
# '--vars' near that limit, 16 B either side, so that '--checks=frame' panics
# at the depth of the model or the recursion ends. each program is built with
# one drawn option list on every target, the frames of x86_64 hold addresses
# of 8 B, those of rv32i of 4 B:
#
#   frames-mismatch  a build ends differently from the model
#   frames-reject    a generated program is rejected, other than for
#                    variables of 'main' that do not fit in '--vars'
#   compiler-hang, invalid-asm, link-failure, image-mismatch
#                    see fuzz-compiler.py
#
# usage: fuzz-frames.py [count] [seed] [jobs]
import random
import sys
import tempfile

import fuzzlib as fl

# name, size and alignment of each element type
TYPES = (("i8", 1, 1), ("i16", 2, 2), ("i32", 4, 4))
# lengths of the arrays, e.g. around the 12-bit offset of rv32i
LENGTHS = (1, 2, 3, 5, 7, 8, 13, 16, 31, 100, 2047, 2048, 2049, 4095, 4097)
TARGETS = ("x86_64", "rv32i", "rv32i-qemu", "rv32i-fpga")
# note: the frame check is what is tested, '--versions' and '--nopt' change
#       which body is kept and the code of the calls
OPTION_LISTS = (("--checks=frame,upper,lower",),
                ("--checks=noub",),
                ("--checks=frame,upper,lower", "--versions=last", "--nopt"),
                ("--checks=frame,upper,lower", "--versions=nested"),
                ("--checks=noub", "--versions=first"))
# the stack of the bare-metal targets, larger than any recursion here so
# that only the frame check stops one
STACK = "--stack=0x100000"
VARIABLES_REJECTED = "does not fit in the variable storage"
# an i32 of a frame: the variable 'r' or 't', the argument 'n' of the call
I32 = (4, 4)


def align(offset, alignment):
    return (offset + alignment - 1) // alignment * alignment


def end_of(storage, start):
    # the end of the (size, alignment) items placed in order after 'start'
    offset = start
    for size, alignment in storage:
        offset = align(offset, alignment) + size
    return offset


class Local:
    def __init__(self, rnd, name):
        element, size, alignment = rnd.choice(TYPES)
        self.storage = (size, alignment)
        if rnd.random() < 0.6:
            n = rnd.choice(LENGTHS)
            self.storage = (size * n, alignment)
            self.lines = [f"var {name} = {element}[{n}]",
                          f"{name}[{n - 1}] = 1"]
        else:
            self.lines = [f"var {name} = {element}", f"{name} = 1"]


class Program:
    def __init__(self, rnd):
        self.before = [Local(rnd, f"b{i}") for i in range(rnd.randint(0, 3))]
        self.after = [Local(rnd, f"c{i}") for i in range(rnd.randint(0, 2))]
        self.mains = [Local(rnd, f"m{i}") for i in range(rnd.randint(0, 3))]
        self.depth = rnd.randint(1, 12)

    def source(self):
        lines = ["func noinline rec(n i32, a mut i8[]) res i32 {",
                 "    if n == 0 {", "        res = 0", "        return",
                 "    }"]
        for local in self.before:
            lines += ["    " + line for line in local.lines]
        lines.append("    var r = rec(n - 1, a)")
        for local in self.after:
            lines += ["    " + line for line in local.lines]
        lines += ["    a[0] = a[0] + 1", "    res = r + 1", "}",
                  "func main() {"]
        for local in self.mains:
            lines += ["    " + line for line in local.lines]
        lines += ["    var s = i8[2]",
                  f"    var t = rec(i32({self.depth}), s)",
                  "    exit(int(t) + int(s[0]) * 0)", "}"]
        return "\n".join(lines) + "\n"

    def frames(self, address):
        # (start of the first frame of 'rec', distance from a frame to the
        # next, frame size of 'rec') for addresses of 'address' bytes; the
        # argument 'n' of a call lives until the call returns
        mains = [m.storage for m in self.mains]
        main_end = end_of(mains + [(2, 1), I32, I32], 0)
        # note: a frame starts with the addresses of 'n', 'a' and 'res'
        slots = 3 * address
        before = [b.storage for b in self.before]
        at_call = end_of(before + [I32, I32], slots)
        after_call = end_of([c.storage for c in self.after],
                            end_of(before + [I32], slots))
        return (align(main_end, address), align(at_call, address),
                max(at_call, after_call))

    def outcome(self, address, variables):
        # the exit code of the recursion, or 'frame overflow' at the first
        # call whose frame does not fit
        first, step, size = self.frames(address)
        # note: 'depth + 1' calls, 'n' from 'depth' down to 0
        for call in range(self.depth + 1):
            if first + call * step + size > variables:
                return ("panic", f"{fl.PANIC_CODE['frame overflow']} "
                        "frame overflow")
        return ("exit", self.depth, b"")

    def variables(self, rnd):
        # '--vars' 16 B around the size that lets 'k + 1' calls fit for one
        # of the address sizes
        address = rnd.choice((8, 4))
        first, step, size = self.frames(address)
        k = rnd.randint(0, self.depth)
        need = first + k * step + size
        return max(16, align(need + rnd.choice((-16, 0, 0, 16)), 16))


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    program = Program(rnd)
    source = program.source()
    variables = program.variables(rnd)
    options = rnd.choice(OPTION_LISTS) + (f"--vars={variables}",)
    found = []
    counts = {}
    with tempfile.TemporaryDirectory(prefix="fuzz-") as work:
        for target in TARGETS:
            target_options = options + (STACK,) \
                if target in fl.UART_TARGETS else options
            got = fl.build_and_run(work, source, target, target_options)
            if got[0] == "skip":
                continue
            # note: the variables of 'main' alone may not fit in '--vars'
            if got[0] == "compile" and VARIABLES_REJECTED in got[1]:
                counts["rejected"] = counts.get("rejected", 0) + 1
                continue
            expected = program.outcome(8 if target == "x86_64" else 4,
                                       variables)
            if got == expected:
                counts[got[0]] = counts.get(got[0], 0) + 1
                continue
            if got[0] == "compile":
                message = got[1].strip().splitlines()[0] if got[1] else ""
                found.append(("frames-reject",
                              f"{target} {message.split(': ', 1)[-1]}",
                              f"{' '.join(target_options)}\n{got[1]}"))
            elif got[0] in ("compile-hang", "invalid-asm", "link-failure",
                            "image-mismatch"):
                category = "compiler-hang" if got[0] == "compile-hang" \
                    else got[0]
                found.append((category, f"{target} {got[0]}",
                              f"{' '.join(target_options)}\n{got[1]}"))
            else:
                found.append(("frames-mismatch",
                              f"{target} {' '.join(options[:-1])} {got[0]}",
                              f"{' '.join(target_options)}\n"
                              f"expected {expected}, got {got}"))
    return source, found, counts


def main():
    count, seed, jobs = fl.arguments(1500)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
