#!/usr/bin/env python3
# model based fuzzer of 'read' and 'write', after the section 'strings and
# data' of 'etc/tutorial.baz'
#
# builds programs that write a whole array, a count of elements and a count
# from a start index of strings, 'dat', variables and array fields, and that
# read lines of the input into an array at a start index with a count. the
# descriptor, the count and the start are numbers or values computed with a
# division or a call that writes itself, which need the registers of the
# call on x86_64 and rv32i. a read
# returns the number of elements it read and 0 at the end of the input. python
# runs the same operations: the program must write the bytes of the model and
# end with the checksum of the numbers the reads returned. the input is one
# line because the uart of rv32i-fpga gives one line to a read while a file
# gives as many bytes as fit
#
#   io-mismatch   a build ends differently from the model
#   io-reject     a generated program is rejected
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-io.py [count] [seed] [jobs]
import random
import shutil
import sys
import tempfile

import fuzzlib as fl

LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJ0123456789 .,:;-_"


def string_literal(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.lines = []
        self.out = bytearray()
        self.code = 0
        self.stdin = b""

    def emit(self, text):
        self.lines.append("    " + text)

    # 'n' as written or computed, see the description of the fuzzer
    def value(self, n):
        kind = self.rnd.random()
        if kind < 0.6:
            return str(n)
        if kind < 0.8:
            return f"(six * {n}) / six"
        return f"quiet({n})"

    def text(self, low=0, high=12):
        return "".join(self.rnd.choice(LETTERS)
                       for _ in range(self.rnd.randint(low, high)))

    def write_slice(self, name, data, size):
        # a whole array, a count, a count from a start index
        rnd = self.rnd
        kind = rnd.choice(["all", "count", "range"])
        if kind == "all":
            self.emit(f"write({self.value(1)}, {name})")
            self.out += bytes(data)
        elif kind == "count":
            n = rnd.randint(0, size)
            self.emit(f"write(1, {name}, {self.value(n)})")
            self.out += bytes(data[:n])
        else:
            n = rnd.randint(0, size)
            start = rnd.randint(0, size - n)
            self.emit(f"write(1, {name}, {self.value(n)}, "
                      f"{self.value(start)})")
            self.out += bytes(data[start:start + n])

    def program(self):
        rnd = self.rnd
        head = []
        arrays = []

        # 'dat' strings and variables initialized from strings
        for i in range(rnd.randint(1, 2)):
            text = self.text(1, 14)
            head.append(f"dat d{i} = {string_literal(text)}")
            arrays.append((f"d{i}", bytearray(text.encode()), len(text)))
        head.append("type holder { n, data i8[24] }")
        # writes nothing and returns 'x'
        head.append("func quiet(x) res {\n    var none = i8[1]\n"
                    "    res = x + write(1, none, 0)\n}")

        for i in range(rnd.randint(1, 3)):
            text = self.text(1, 14)
            self.emit(f"var s{i} = {string_literal(text)}")
            arrays.append((f"s{i}", bytearray(text.encode()), len(text)))

        # a field of an instance holds an array too
        text = self.text(1, 20)
        self.emit(f"var h = holder{{3, {string_literal(text)}}}")
        field = bytearray(text.encode()) + bytearray(24 - len(text))
        arrays.append(("h.data", field, 24))

        # one line of the input, the first read takes it and the others are at the
        # end of the input
        lines = [self.text(0, 10)]
        self.stdin = "".join(line + "\n" for line in lines).encode()
        buffers = []
        for i in range(rnd.randint(1, 2)):
            size = rnd.randint(16, 24)
            self.emit(f"var r{i} = i8[{size}]")
            buffers.append([f"r{i}", bytearray(size), size])

        pending = list(lines)
        code = 0
        for _ in range(rnd.randint(2, 8)):
            if rnd.random() < 0.55 and buffers:
                b = rnd.choice(buffers)
                name, data, size = b
                count = rnd.randint(12, size)
                start = rnd.randint(0, size - count)
                call = (f"read({self.value(0)}, {name}, {self.value(count)}, "
                        f"{self.value(start)})")
                if start == 0 and rnd.random() < 0.3 and count == size:
                    call = f"read(0, {name})"
                    count = size
                got_var = f"g{len(self.lines)}"
                self.emit(f"var {got_var} = {call}")
                if pending:
                    line = pending.pop(0).encode() + b"\n"
                    got = len(line)
                    data[start:start + got] = line
                else:
                    got = 0
                code = (code * 7 + got) & 0xffff
                self.emit(f"acc = (acc * 7 + {got_var}) & 0xffff")
                self.write_slice(name, data, size)
            else:
                name, data, size = rnd.choice(arrays + [tuple(b) for b in buffers])
                self.write_slice(name, data, size)
        self.code = code
        self.lines.insert(0, "    var acc = 0")
        self.lines.insert(1, "    var six = 6")
        self.emit("exit(acc & 127)")
        source = "\n".join(head) + "\nfunc main() {\n" + \
            "\n".join(self.lines) + "\n}\n"
        return source, ("exit", self.code & 127, bytes(self.out))


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    gen = Generator(rnd)
    source, expected = gen.program()
    work = tempfile.mkdtemp(prefix="fuzzi-")
    try:
        found = fl.judge(work, source, expected, stdin=gen.stdin, prefix="io")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
