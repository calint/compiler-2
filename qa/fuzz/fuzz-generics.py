#!/usr/bin/env python3
# model based fuzzer of generics, after the section 'generics' of
# 'etc/tutorial.baz'
#
# builds programs of generic types with constant parameters ('buffer<capacity>',
# 'grid<w, h>') and with type parameters ('pair<first type, second type>'),
# aliases of instances (several names for one instance, several instances of
# one type), methods that see the constants, generic functions called with
# type arguments and with the type deduced from an argument or from the
# destination. python runs the same operations, the program mixes what it reads
# into a checksum and ends with it as the exit code:
#
#   generics-mismatch   a build ends differently from the model
#   generics-reject     a generated program is rejected
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-generics.py [count] [seed] [jobs]
import random
import shutil
import sys
import tempfile

import fuzzlib as fl
from fuzzlib import mix


class Buffer:
    # an instance of 'buffer<capacity>'
    def __init__(self, capacity):
        self.capacity = capacity
        self.items = [0] * capacity
        self.count = 0

    def copy(self):
        other = Buffer(self.capacity)
        other.items = list(self.items)
        other.count = self.count
        return other

    def full(self):
        return self.count == self.capacity

    def add(self, v):
        self.items[self.count] = ((v + 128) & 255) - 128
        self.count += 1


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.lines = []
        self.acc = 0
        self.names = 0
        self.indent = 1

    def name(self, prefix):
        self.names += 1
        return f"{prefix}{self.names}"

    def emit(self, text):
        self.lines.append("    " * self.indent + text)

    def mix_value(self, text, value):
        self.emit(f"mix(acc, i32({text}))")
        self.acc = mix(self.acc, value)

    def mix_buffer(self, name, b):
        self.mix_value(f"{name}.count", b.count)
        self.emit(f"foo {name}.items {{ mix(acc, i32(e) + i32(i)) }}")
        for i, v in enumerate(b.items):
            self.acc = mix(self.acc, v + i)
        self.mix_value(f"array_length({name}.items)", b.capacity)
        full = b.full()
        self.emit(f"if {name}.is_full() mix(acc, 1) else mix(acc, 2)")
        self.acc = mix(self.acc, 1 if full else 2)

    def program(self):
        rnd = self.rnd
        head = [
            "type buffer<capacity> {",
            "    items i8[capacity],",
            "    count,",
            "}",
            "func mut buffer.add(v) {",
            "    self.items[self.count] = i8(v)",
            "    self.count = self.count + 1",
            "}",
            "func buffer.is_full() res bool {",
            "    res = self.count == capacity",
            "}",
            "func make<T type>(n) res T {",
            "    res.items = {}",
            "    res.count = n",
            "}",
            "func fill<T type>(b mut T, v) {",
            "    loop {",
            "        if b.is_full() break",
            "        b.add(v)",
            "    }",
            "}",
            "type grid<w, h> {",
            "    a i8[w],",
            "    b i8[h],",
            "}",
            "func grid.area() res {",
            "    res = w * h",
            "}",
            "func grid.perimeter() res {",
            "    res = array_length(self.a) + array_length(self.b)",
            "}",
            "type pair<first type, second type> {",
            "    x first,",
            "    y second,",
            "}",
            "func mix(acc mut i32, v i32) "
            "{ acc = ((acc ^ (v & 0xffff)) * 31) & 0xffff }",
        ]
        # aliases of instances: several names may name one instance
        caps = [rnd.randint(1, 8) for _ in range(rnd.randint(1, 3))]
        aliases = []
        for i, cap in enumerate(caps):
            head.append(f"type s{i} = buffer<{cap}>")
            aliases.append((f"s{i}", cap))
        if rnd.random() < 0.5:
            alias, cap = rnd.choice(aliases)
            head.append(f"type twin = buffer<{cap}>")
            aliases.append(("twin", cap))
        grids = []
        for i in range(rnd.randint(1, 2)):
            w, h = rnd.randint(1, 5), rnd.randint(1, 5)
            head.append(f"type g{i} = grid<{w}, {h}>")
            grids.append((f"g{i}", w, h))
        first = rnd.choice(aliases)
        second = rnd.choice(aliases)
        head.append(f"type duo = pair<{first[0]}, {second[0]}>")
        head.append("type mixed = pair<i16, " + first[0] + ">")

        self.emit("var acc = i32(0)")
        for _ in range(rnd.randint(2, 4)):
            alias, cap = rnd.choice(aliases)
            var = self.name("b")
            b = Buffer(cap)
            self.emit(f"var {var} = {alias}")
            for _ in range(rnd.randint(1, 6)):
                kind = rnd.choice(["add", "add", "fill", "fill_deduced",
                                   "make", "make_deduced", "copy"])
                if kind == "add":
                    v = rnd.randint(-100, 100)
                    self.emit(f"if not {var}.is_full() {var}.add({v})")
                    if not b.full():
                        b.add(v)
                elif kind == "fill":
                    v = rnd.randint(-100, 100)
                    self.emit(f"fill<{alias}>({var}, {v})")
                    while not b.full():
                        b.add(v)
                elif kind == "fill_deduced":
                    v = rnd.randint(-100, 100)
                    self.emit(f"fill({var}, {v})")
                    while not b.full():
                        b.add(v)
                elif kind == "make":
                    n = rnd.randint(0, cap)
                    self.emit(f"{var} = make<{alias}>({n})")
                    b = Buffer(cap)
                    b.count = n
                elif kind == "make_deduced":
                    n = rnd.randint(0, cap)
                    self.emit(f"{var} = make({n})")
                    b = Buffer(cap)
                    b.count = n
                else:
                    other = self.name("c")
                    self.emit(f"var {other} = {var}")
                    self.emit(f"if {other} != {var} exit(1)")
                    c = b.copy()
                    if not c.full():
                        self.emit(f"if not {other}.is_full() "
                                  f"{other}.add(7)")
                        c.add(7)
                    # note: a variable name is declared once per scope
                    eq = self.name("q")
                    same = c.items == b.items and c.count == b.count
                    self.emit(f"var {eq} = {other} == {var}")
                    self.emit(f"if {eq} != {str(same).lower()} exit(2)")
                    self.mix_buffer(other, c)
            self.mix_buffer(var, b)

        for alias, w, h in grids:
            var = self.name("g")
            self.emit(f"var {var} = {alias}")
            self.mix_value(f"{var}.area()", w * h)
            self.mix_value(f"{var}.perimeter()", w + h)
            self.emit(f"{var}.a[0] = 3")
            self.emit(f"{var}.b[{h - 1}] = 4")
            self.mix_value(f"{var}.a[0] + {var}.b[{h - 1}]", 7)

        d = self.name("d")
        self.emit(f"var {d} = duo")
        fb = Buffer(first[1])
        sb = Buffer(second[1])
        v = rnd.randint(-50, 50)
        self.emit(f"{d}.x.add({v})")
        fb.add(v)
        self.emit(f"fill({d}.y, 9)")
        while not sb.full():
            sb.add(9)
        self.mix_buffer(f"{d}.x", fb)
        self.mix_buffer(f"{d}.y", sb)
        m = self.name("m")
        self.emit(f"var {m} = mixed")
        self.emit(f"{m}.x = 300")
        self.emit(f"{m}.y.add(5)")
        mb = Buffer(first[1])
        mb.add(5)
        self.mix_value(f"{m}.x", 300)
        self.mix_buffer(f"{m}.y", mb)
        self.emit("exit(acc & 127)")
        source = "\n".join(head) + "\nfunc main() {\n" + \
            "\n".join(self.lines) + "\n}\n"
        return source, ("exit", self.acc & 127, b"")


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    source, expected = Generator(rnd).program()
    work = tempfile.mkdtemp(prefix="fuzzg-")
    try:
        found = fl.judge(work, source, expected, prefix="generics")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
