#!/usr/bin/env python3
# generator fuzzer: random valid programs without undefined behavior
#
# builds terminating programs of functions (inlined and 'noinline', 'mut'
# arguments, array arguments, methods, constructors, bounded recursion), user
# types with nested records and arrays, loops, 'foo', 'array_copy', conversions
# and bit operations. every value is kept in a range that cannot overflow, every
# index is masked, every divisor is odd, so a correct compiler runs each program
# to the end without a panic and every build of it must end the same:
#
#   gen-reject    a generated program is rejected (the generator or the
#                 compiler is wrong)
#   gen-panic     a check fires in a program without undefined behavior
#   diff-target   x86_64 and rv32i-fpga end differently
#   diff-nopt     '--nopt' ends differently
#   diff-checks   the program without checks ends differently
#   diff-line     '--checks=noub,line' ends differently
#   cpu-error     the emulator stops
#   ub-signal     a signal ends the program
#   compiler-crash, compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-programs.py [count] [seed] [jobs] [compiler]
# 'compiler' is 'asan' (default, the sanitizer build) or 'plain'
import concurrent.futures
import os
import random
import re
import shutil
import sys
import tempfile
import time

import fuzzlib as fl

LIMIT = 1 << 30
VAR_BOUND = 1 << 16


class Expr:
    def __init__(self, text, bound):
        self.text = text
        self.bound = bound


def norm16(e):
    return Expr(f"(({e.text}) & 0xffff) - 32768", 1 << 15)


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.scalars = []
        self.read_only = set()
        self.arrays = []
        self.bytes_arrays = []
        self.records = []
        self.functions = []
        self.lines = []
        self.depth = 0
        self.label = 0
        self.indent = 1

    # expressions

    def atom(self):
        rnd = self.rnd
        r = rnd.random()
        if r < 0.3 or not self.scalars:
            v = rnd.choice([0, 1, -1, 2, 3, 7, 100, -100, 255, 1000,
                            rnd.randint(-5000, 5000)])
            return Expr(f"i32({v})", abs(v))
        if r < 0.65:
            return Expr(rnd.choice(self.scalars), VAR_BOUND)
        if r < 0.75 and self.arrays:
            name, size = rnd.choice(self.arrays)
            return Expr(f"{name}[{self.index(size)}]", VAR_BOUND)
        if r < 0.82 and self.bytes_arrays:
            name, size = rnd.choice(self.bytes_arrays)
            return Expr(f"i32({name}[{self.index(size)}])", 128)
        if r < 0.9 and self.records:
            return Expr(rnd.choice(self.records), VAR_BOUND)
        name = rnd.choice(self.scalars)
        return Expr(name, VAR_BOUND)

    def target(self):
        names = [n for n in self.scalars if n not in self.read_only]
        return self.rnd.choice(names)

    def index(self, size):
        # sizes are powers of two, a mask keeps the index in range
        e = self.expr(1)
        return f"({e.text}) & {size - 1}"

    def expr(self, depth):
        rnd = self.rnd
        if depth <= 0 or rnd.random() < 0.25:
            return self.atom()
        kind = rnd.random()
        if kind < 0.45:
            a, b = self.expr(depth - 1), self.expr(depth - 1)
            op = rnd.choice(["+", "-", "*"])
            return self.combine(a, op, b)
        if kind < 0.55:
            a, b = self.expr(depth - 1), self.expr(depth - 1)
            op = rnd.choice(["&", "|", "^"])
            bound = (1 << max(a.bound, b.bound).bit_length()) - 1
            return Expr(f"({a.text}) {op} ({b.text})", bound)
        if kind < 0.65:
            a = self.expr(depth - 1)
            b = self.expr(depth - 1)
            op = rnd.choice(["/", "%"])
            return Expr(f"({a.text}) {op} (({b.text}) | 1)", a.bound)
        if kind < 0.75:
            a = self.expr(depth - 1)
            k = rnd.randint(0, 3)
            if rnd.random() < 0.5:
                if a.bound * (1 << k) > LIMIT:
                    a = norm16(a)
                return Expr(f"({a.text}) << {k}", a.bound * (1 << k))
            return Expr(f"({a.text}) >> {k}", a.bound)
        if kind < 0.8:
            a = self.expr(depth - 1)
            count = self.expr(depth - 1)
            # a count of 0 to 7 multiplies by up to 128
            if a.bound * 128 > LIMIT:
                a = norm16(a)
            op = rnd.choice(["<<", ">>"])
            return Expr(f"({a.text}) {op} (({count.text}) & 7)",
                        a.bound * 128 if op == "<<" else a.bound)
        if kind < 0.87:
            a = self.expr(depth - 1)
            t = rnd.choice(["i8", "i16"])
            return Expr(f"i32({t}({a.text}))", 128 if t == "i8" else 1 << 15)
        if kind < 0.92:
            a = self.expr(depth - 1)
            return Expr(f"-({a.text})", a.bound)
        if kind < 0.95:
            a = self.expr(depth - 1)
            return Expr(f"~({a.text})", a.bound + 1)
        return self.atom()

    def combine(self, a, op, b):
        if op == "*":
            while a.bound * b.bound > LIMIT:
                a, b = (norm16(a), b) if a.bound >= b.bound else (a, norm16(b))
            return Expr(f"({a.text}) * ({b.text})", a.bound * b.bound)
        if a.bound + b.bound > LIMIT:
            a, b = norm16(a), norm16(b)
        return Expr(f"({a.text}) {op} ({b.text})", a.bound + b.bound)

    def assignable(self, e):
        if e.bound > VAR_BOUND:
            return norm16(e).text
        return e.text

    def condition(self, depth=2):
        rnd = self.rnd
        r = rnd.random()
        if depth > 0 and r < 0.2:
            return (f"({self.condition(depth - 1)}) "
                    f"{rnd.choice(['and', 'or'])} ({self.condition(depth - 1)})")
        if depth > 0 and r < 0.28:
            return f"not ({self.condition(depth - 1)})"
        a, b = self.expr(2), self.expr(2)
        op = rnd.choice(["==", "!=", "<", ">", "<=", ">="])
        return f"({a.text}) {op} ({b.text})"

    # statements

    def emit(self, text):
        self.lines.append("    " * self.indent + text)

    def block(self, count, in_loop=False):
        self.indent += 1
        self.depth += 1
        for _ in range(count):
            self.statement(in_loop)
        self.depth -= 1
        self.indent -= 1

    def statement(self, in_loop=False):
        rnd = self.rnd
        r = rnd.random()
        nested = self.depth < 3
        if r < 0.25 or not nested and r < 0.5:
            self.emit(f"{self.target()} = "
                      f"{self.assignable(self.expr(3))}")
        elif r < 0.33 and self.arrays:
            name, size = rnd.choice(self.arrays)
            self.emit(f"{name}[{self.index(size)}] = "
                      f"{self.assignable(self.expr(2))}")
        elif r < 0.37 and self.bytes_arrays:
            name, size = rnd.choice(self.bytes_arrays)
            self.emit(f"{name}[{self.index(size)}] = "
                      f"i8({self.expr(2).text})")
        elif r < 0.42 and self.records:
            self.emit(f"{rnd.choice(self.records)} = "
                      f"{self.assignable(self.expr(2))}")
        elif r < 0.52 and self.functions:
            self.call_statement()
        elif r < 0.58 and len(self.arrays) >= 2:
            self.copy_statement()
        elif r < 0.66 and nested:
            self.emit(f"if {self.condition()} {{")
            self.block(rnd.randint(1, 3), in_loop)
            if rnd.random() < 0.5:
                self.emit("} else {")
                self.block(rnd.randint(1, 3), in_loop)
            self.emit("}")
        elif r < 0.74 and nested:
            self.loop_statement(in_loop)
        elif r < 0.80 and nested and self.arrays:
            name, size = rnd.choice(self.arrays)
            self.emit(f"foo {name} {{")
            self.indent += 1
            self.emit("e = (e + i32(i) * 3 + i32(n)) & 0xfff")
            if rnd.random() < 0.5:
                self.emit(f"{self.target()} = "
                          f"(e + {rnd.choice(self.scalars)}) & 0xffff")
            self.indent -= 1
            self.emit("}")
        elif r < 0.84 and in_loop:
            self.emit(f"if {self.condition(1)} {{")
            self.indent += 1
            self.emit(rnd.choice(["break", "continue"]))
            self.indent -= 1
            self.emit("}")
        elif r < 0.88 and nested:
            self.emit("{")
            self.indent += 1
            self.label += 1
            name = f"t{self.label}"
            self.emit(f"var {name} = i32({self.assignable(self.expr(2))})")
            self.scalars.append(name)
            self.block(rnd.randint(1, 2), in_loop)
            self.scalars.remove(name)
            self.indent -= 1
            self.emit("}")
        else:
            self.emit(f"{self.target()} = "
                      f"{self.assignable(self.expr(2))}")

    def loop_statement(self, in_loop):
        rnd = self.rnd
        self.label += 1
        counter = f"c{self.label}"
        self.emit(f"var {counter} = i32(0)")
        self.emit("loop {")
        self.indent += 1
        self.emit(f"if {counter} >= {rnd.randint(1, 6)} break")
        self.emit(f"{counter} = {counter} + 1")
        self.scalars.append(counter)
        self.indent -= 1
        self.block(rnd.randint(1, 3), True)
        self.scalars.remove(counter)
        self.emit("}")

    def copy_statement(self):
        rnd = self.rnd
        (a, na), (b, nb) = rnd.sample(self.arrays, 2)
        count = min(na, nb)
        self.emit(f"array_copy({a}, {b}, {rnd.randint(0, count)})")

    def call_statement(self):
        rnd = self.rnd
        name, params, mutable = rnd.choice(self.functions)
        args = []
        scalars = [n for n in self.scalars if n not in self.read_only]
        rnd.shuffle(scalars)
        for i, kind in enumerate(params):
            if kind == "mut":
                if not scalars:
                    return
                args.append(scalars.pop())
            elif kind == "array":
                if not self.arrays:
                    return
                args.append(rnd.choice(self.arrays)[0])
            else:
                args.append(self.assignable(self.expr(1)))
        # an argument that is written must not share storage with another one
        written = [a for a, k in zip(args, params) if k in ("mut", "array")]
        reads = [a for a, k in zip(args, params) if k not in ("mut", "array")]
        if any(w in r for w in written for r in reads):
            return
        self.label += 1
        result = f"r{self.label}"
        self.emit(f"var {result} = {name}({', '.join(args)})")
        self.emit(f"{self.target()} = "
                  f"({result} & 0xffff) - 32768")

    # functions

    def make_function(self, index):
        rnd = self.rnd
        name = f"f{index}"
        noinline = rnd.random() < 0.5
        kinds = [rnd.choice(["val", "val", "mut", "array"]) for _ in range(rnd.randint(1, 3))]
        # at most one array and one 'mut' to keep the aliasing rules simple
        seen = set()
        kinds = [k for k in kinds if k == "val" or not (k in seen or seen.add(k))]
        if not kinds:
            kinds = ["val"]
        params = []
        names = []
        saved = (self.scalars, self.arrays, self.records, self.functions)
        self.scalars, self.arrays = [], []
        self.records = []
        self.functions = [f for f in saved[3] if f[0] != name]
        for i, kind in enumerate(kinds):
            p = f"p{i}"
            if kind == "val":
                params.append(f"{p} i32")
                self.scalars.append(p)
                self.read_only.add(p)
            elif kind == "mut":
                params.append(f"{p} mut i32")
                self.scalars.append(p)
            else:
                params.append(f"{p} mut i32[]")
                self.arrays.append((p, 4))
            names.append(p)
        self.lines = []
        self.emit(f"var acc{index} = i32(0)")
        self.scalars.append(f"acc{index}")
        # the array has four elements at the call sites
        self.block(rnd.randint(1, 4))
        self.emit(f"res = (acc{index} & 0xffff) - 32768")
        body = self.lines
        self.lines = []
        self.scalars, self.arrays, self.records, self.functions = saved
        self.read_only = set()
        text = ["func " + ("noinline " if noinline else "") + name + "(" +
                ", ".join(p.replace("i32[]", "i32[]") for p in params) +
                ") res i32 {", "    res = 0"] + body + ["}"]
        joined = "\n".join(text)
        return joined, (name, kinds, True)

    def recursive_function(self):
        return """func noinline rec(n i32, acc i32) res i32 {
    res = acc
    if n <= 0 return
    var next = i32((((acc * 3) + n) & 0xffff) - 32768)
    var r = rec(n - 1, next)
    res = r
}
"""

    def program(self):
        rnd = self.rnd
        out = ["type pt { x i32, y i32 }",
               "type box { a pt, items i32[4], tag i8 }",
               "dat table = i32[]{3, 1, 4, 1, 5, 9, 2, 6}",
               "func pt.sum() res i32 { res = self.x + self.y }",
               "func mut pt.shift(dx i32) { self.x = (self.x + dx) & 0xfff }",
               "func pt.at(x i32, y i32) self { self.x = x self.y = y }",
               self.recursive_function()]
        count = rnd.randint(0, 4)
        for i in range(count):
            text, info = self.make_function(i)
            out.append(text)
            self.functions.append(info)
        self.lines = []
        out.append("func main() {")
        self.indent = 1
        for i in range(rnd.randint(4, 6)):
            self.emit(f"var v{i} = i32({rnd.randint(-3000, 3000)})")
            self.scalars.append(f"v{i}")
        self.emit("var arr = i32[4]")
        self.emit("var big = i32[8]")
        self.emit("var bytes = i8[8]")
        self.emit("var p = pt")
        self.emit("var q = pt.at(3, 4)")
        self.emit("var bx = box")
        for k in range(4):
            self.emit(f"arr[{k}] = {k * 7 - 5}")
        self.arrays = [("arr", 4), ("big", 8), ("bx.items", 4)]
        self.bytes_arrays = [("bytes", 8)]
        self.records = ["p.x", "p.y", "q.x", "bx.a.x", "bx.a.y"]
        self.statements_main()
        self.emit("var rr = rec(" + str(rnd.randint(0, 40)) + ", v0)")
        self.emit("v1 = (rr & 0xffff) - 32768")
        self.emit("var s1 = p.sum()")
        self.emit("var s2 = q.sum()")
        self.emit("var s3 = bx.a.sum()")
        self.emit("var s = i32(s1 + s2 + s3)")
        self.emit("p.shift(v0 & 0xff)")
        self.emit("var cmp = p == q")
        self.emit("if cmp { v2 = v2 + 1 }")
        self.emit("var eq = arrays_equal(arr, big, 4)")
        self.emit("if eq { v3 = v3 + 1 }")
        self.emit(f"v0 = (v0 + s + i32(array_length(arr)) + table[v1 & 7]) & 0xffff")
        self.emit("var out = i8[16]")
        names = self.scalars[:6]
        for k in range(16):
            self.emit(f"out[{k}] = i8({names[k % len(names)]} >> {k % 5})")
        self.emit("write(1, out, 16)")
        self.emit("exit(v0 & 127)")
        out.extend(self.lines)
        out.append("}")
        return "\n".join(out) + "\n"

    def statements_main(self):
        for _ in range(self.rnd.randint(4, 14)):
            self.statement()


def fuzz_one(args):
    seed, index, kind = args
    rnd = random.Random(seed * 1000003 + index)
    source = Generator(rnd).program()
    work = tempfile.mkdtemp(prefix="fuzzp-")
    compiler = fl.COMPILER_ASAN if kind == "asan" else fl.COMPILER
    found = []
    stats = {"ok": 0, "rejected": 0}
    try:
        path = os.path.join(work, "f.baz")
        with open(path, "w") as f:
            f.write(source)
        base = ["--checks=noub"]
        c, x86, problem = fl.run_x86(compiler, path, base, work)
        if c.timed_out:
            found.append(("compiler-hang", "timeout", ""))
            return source, found, stats
        if c.returncode and fl.known_crash(c.stderr):
            return source, found, stats
        if c.returncode:
            sanitizer = any(t in c.stderr for t in fl.SANITIZER_TEXT)
            if c.returncode in fl.SANITIZER_EXIT or c.returncode < 0 or sanitizer:
                found.append(("compiler-crash", fl.signature(c.stderr),
                              c.stderr[-3000:]))
            elif "reduce expression complexity" in c.stderr:
                # the registers of the target are used up, a limit of the
                # language and not a defect
                stats["rejected"] += 1
            else:
                stats["rejected"] += 1
                message = c.stderr.strip().split("\n")
                key = re.sub(r"'[^']*' of type", "X of type",
                             message[0].split(": ", 1)[-1])[:100]
                found.append(("gen-reject", key, c.stderr[-1500:]))
            return source, found, stats
        if problem and problem[0] == "skip":
            return source, found, stats
        if problem:
            found.append((problem[0], problem[1][:80], problem[1][-1500:]))
            return source, found, stats
        stats["ok"] += 1
        if x86.kind == "panic":
            found.append(("gen-panic", x86.detail, str(x86)))
        if x86.kind == "signal":
            found.append(("ub-signal", x86.detail, str(x86)))
        c2, rv = fl.run_fpga(compiler, path, base, work)
        if c2.returncode and fl.known_crash(c2.stderr):
            pass
        elif c2.returncode:
            sanitizer = any(t in c2.stderr for t in fl.SANITIZER_TEXT)
            kindname = "compiler-crash" if sanitizer or c2.returncode in fl.SANITIZER_EXIT else "gen-reject"
            if kindname == "gen-reject" and "reduce expression complexity" in c2.stderr:
                return source, found, stats
            found.append((kindname, "rv32i " + (fl.signature(c2.stderr) if sanitizer else c2.stderr.strip().split("\n")[0][-60:]),
                          c2.stderr[-1500:]))
        elif rv is not None:
            if rv.kind in ("cpu-error", "emulator-signal"):
                found.append(("cpu-error", rv.detail[:60], str(rv)))
            if x86.finished() and rv.finished() and not x86.same(rv):
                found.append(("diff-target", f"{x86.kind} vs {rv.kind}",
                              f"x86 {x86}\nfpga {rv}"))
        variants = [("diff-nopt", base + ["--nopt"]),
                    ("diff-checks", []),
                    ("diff-line", ["--checks=noub,line"])]
        if x86.finished():
            for category, options in variants:
                c3, other, p3 = fl.run_x86(compiler, path, options, work, "v")
                if c3.returncode and "reduce expression complexity" in c3.stderr:
                    # folding changes how many registers an expression needs
                    continue
                if c3.returncode or p3:
                    found.append((category, "rejected", c3.stderr[-500:]))
                elif other.finished() and not x86.same(other):
                    found.append((category, f"{x86.kind} vs {other.kind}",
                                  f"base {x86}\nvariant {other}"))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found, stats


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 300
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(1 << 30)
    jobs = int(sys.argv[3]) if len(sys.argv) > 3 else max(1, (os.cpu_count() or 2) - 2)
    kind = sys.argv[4] if len(sys.argv) > 4 else "asan"
    if kind == "asan" and not os.path.exists(fl.COMPILER_ASAN):
        sys.exit("build the compiler first: build-asan.sh")
    findings = fl.Findings()
    print(f"seed {seed}, {count} programs, {jobs} jobs, {kind} compiler")
    totals = {"ok": 0, "rejected": 0}
    categories = {}
    new = 0
    begin = time.time()
    with concurrent.futures.ProcessPoolExecutor(jobs) as pool:
        for n, (source, found, stats) in enumerate(
                pool.map(fuzz_one, [(seed, i, kind) for i in range(count)],
                         chunksize=2), 1):
            for k in totals:
                totals[k] += stats[k]
            for category, key, detail in found:
                categories[category] = categories.get(category, 0) + 1
                if findings.add("gen-" + category if not category.startswith("gen-") else category, key, source, detail):
                    new += 1
                    print(f"NEW {category}: {key[:150]}", flush=True)
            if n % 200 == 0:
                print(f"  {n}/{count} {time.time() - begin:.0f}s {totals}",
                      flush=True)
    print(f"{count} programs: {totals['ok']} ran, {totals['rejected']} "
          f"rejected, {time.time() - begin:.0f}s")
    print("findings by category (all occurrences):", dict(sorted(categories.items())))
    print(f"{new} new findings in {fl.FINDINGS}")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
