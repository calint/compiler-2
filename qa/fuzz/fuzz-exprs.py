#!/usr/bin/env python3
# model based fuzzer of expressions
#
# builds programs of nested expressions on i8, i16, i32 (and i64 on x86_64):
# + - * / % << >> & | ^, unary - and ~, conversions that wrap, comparisons, and,
# or, not with short circuit evaluation, constants of the type and plain
# literals, in assignments, 'if' and bounded loops. each program is evaluated
# exactly in python. the program must end with the modeled exit code, or with
# the modeled kind of panic, or be rejected by the compiler when the model
# finds a constant expression that is undefined. when the evaluation order of
# the operands of an operation decides which of two panics comes first, either
# is accepted
#
# usage: [FUZZ_CHECKS=noub] fuzz-exprs.py [count] [seed] [jobs]
import concurrent.futures
import importlib.util
import os
import random
import shutil
import sys
import tempfile
import time

import fuzzlib as fl

BITS = {"i8": 8, "i16": 16, "i32": 32, "i64": 64}
CHECKS = os.environ.get("FUZZ_CHECKS", "noub")


class Reject(Exception):
    pass


def limits(t):
    return -(1 << (BITS[t] - 1)), (1 << (BITS[t] - 1)) - 1


def fits(t, v):
    low, high = limits(t)
    return low <= v <= high


def wrap(t, v):
    bits = BITS[t]
    v &= (1 << bits) - 1
    return v - (1 << bits) if v >> (bits - 1) else v


def trunc_div(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


class Value:
    # the result of the evaluation: a value, or the set of panics that
    # the evaluation can end with
    def __init__(self, value=None, panics=()):
        self.value = value
        self.panics = frozenset(panics)


class Node:
    def __init__(self, text, const=False):
        self.text = text
        self.const = const


class Generator:
    def __init__(self, rnd, t):
        self.rnd = rnd
        self.t = t
        self.names = ["v0", "v1", "v2", "v3", "v4", "v5"]
        self.values = {}
        self.benign = rnd.random() < 0.6
        for n in self.names:
            self.values[n] = self.constant_value()

    def constant_value(self, benign=None):
        rnd = self.rnd
        low, high = limits(self.t)
        if self.benign if benign is None else benign:
            return rnd.choice([rnd.randint(-9, 9), rnd.randint(-9, 9), 0, 1,
                               -1, 2, 3, rnd.randint(-100, 100)]) \
                if fits(self.t, 100) else rnd.randint(-9, 9)
        v = rnd.choice([low, low + 1, -1, 0, 1, 2, high - 1, high, 3, -2,
                        rnd.randint(low, high), rnd.randint(-20, 20)])
        return max(v, low + 1) if self.t == "i64" else v

    # an expression is a tuple tree: ("var", name) ("const", value, typed)
    # ("bin", op, a, b) ("neg", a) ("not", a) ("conv", u, a)
    def expr(self, depth):
        rnd = self.rnd
        if depth <= 0 or rnd.random() < 0.22:
            if rnd.random() < 0.7:
                return ("var", rnd.choice(self.names))
            return ("const", self.constant_value(), rnd.random() < 0.6)
        r = rnd.random()
        if r < 0.40:
            op = rnd.choice(["+", "-", "*"])
            return ("bin", op, self.expr(depth - 1), self.operand(depth - 1))
        if r < 0.52:
            op = rnd.choice(["/", "%"])
            return ("bin", op, self.expr(depth - 1), self.operand(depth - 1))
        if r < 0.64:
            op = rnd.choice(["&", "|", "^"])
            return ("bin", op, self.expr(depth - 1), self.operand(depth - 1))
        if r < 0.74:
            op = rnd.choice(["<<", ">>"])
            return ("bin", op, self.expr(depth - 1), self.count(depth - 1))
        if r < 0.82:
            return ("neg", self.expr(depth - 1))
        if r < 0.88:
            return ("inv", self.expr(depth - 1))
        if r < 0.95 and self.t != "i8":
            u = rnd.choice([x for x in BITS if BITS[x] < BITS[self.t]])
            return ("conv", u, self.arith_chain(2))
        return ("var", rnd.choice(self.names))

    def operand(self, depth):
        # a literal is allowed as the right operand only
        if self.rnd.random() < 0.2:
            return ("literal", self.constant_value())
        return self.expr(depth)

    def count(self, depth):
        rnd = self.rnd
        bits = BITS[self.t]
        if rnd.random() < 0.4:
            return ("literal", rnd.randint(0, bits - 1))
        if rnd.random() < 0.1:
            return ("literal", rnd.choice([bits, bits + 1, -1]))
        return self.expr(depth)

    def arith_chain(self, depth):
        # only + - * so that wrapping at any width gives the same low bits
        rnd = self.rnd
        if depth <= 0 or rnd.random() < 0.3:
            if rnd.random() < 0.75:
                return ("var", rnd.choice(self.names))
            return ("const", self.constant_value(), True)
        return ("bin", rnd.choice(["+", "-", "*"]), self.arith_chain(depth - 1),
                self.arith_chain(depth - 1) if rnd.random() < 0.8 else
                ("literal", self.constant_value()))

    def cond(self, depth):
        rnd = self.rnd
        r = rnd.random()
        if depth > 0 and r < 0.2:
            return ("and" if rnd.random() < 0.5 else "or", self.cond(depth - 1),
                    self.cond(depth - 1))
        if depth > 0 and r < 0.3:
            return ("bnot", self.cond(depth - 1))
        return ("cmp", rnd.choice(["==", "!=", "<", "<=", ">", ">="]),
                self.expr(2), self.operand(2))

    def statements(self):
        rnd = self.rnd
        out = []
        for _ in range(rnd.randint(2, 8)):
            r = rnd.random()
            dst = rnd.choice(self.names)
            if r < 0.6:
                out.append(("assign", dst, self.expr(rnd.randint(1, 4))))
            elif r < 0.85:
                out.append(("if", self.cond(2),
                            [("assign", dst, self.expr(2))],
                            [("assign", rnd.choice(self.names), self.expr(2))]
                            if rnd.random() < 0.5 else []))
            else:
                out.append(("loop", rnd.randint(1, 3),
                            [("assign", dst, self.expr(2))]))
        return out


class Program:
    name = "exprs"

    def __init__(self, rnd):
        self.rnd = rnd
        self.t = rnd.choice(list(BITS))
        self.g = Generator(rnd, self.t)
        self.targets = ["x86_64"] if self.t == "i64" else ["x86_64", "rv32i-fpga"]
        self.statements = self.g.statements()
        self.loops = 0
        self.lenient = False

    # text

    def text(self, e):
        t = self.t
        k = e[0]
        if k == "var":
            return e[1]
        if k == "const":
            # a typed constant, or a bare literal where the type is deduced
            return f"{t}({e[1]})"
        if k == "literal":
            return str(e[1]) if e[1] >= 0 else f"({e[1]})"
        if k == "bin":
            return f"({self.text(e[2])} {e[1]} {self.text(e[3])})"
        if k == "neg":
            return f"(-{self.text(e[1])})"
        if k == "inv":
            return f"(~{self.text(e[1])})"
        if k == "conv":
            return f"{t}({e[1]}({self.text(e[2])}))"
        if k == "cmp":
            return f"({self.text(e[2])} {e[1]} {self.text(e[3])})"
        if k == "and" or k == "or":
            return f"({self.text(e[1])} {k} {self.text(e[2])})"
        if k == "bnot":
            return f"(not {self.text(e[1])})"
        raise ValueError(k)

    def lines(self, statements, indent):
        pad = "    " * indent
        out = []
        for s in statements:
            if s[0] == "assign":
                out.append(f"{pad}{s[1]} = {self.text(s[2])}")
            elif s[0] == "if":
                out.append(f"{pad}if {self.text(s[1])} {{")
                out += self.lines(s[2], indent + 1)
                if s[3]:
                    out.append(f"{pad}}} else {{")
                    out += self.lines(s[3], indent + 1)
                out.append(f"{pad}}}")
            else:
                self.loops += 1
                c = f"c{self.loops}"
                out.append(f"{pad}var {c} = {self.t}(0)")
                out.append(f"{pad}loop {{")
                out.append(f"{pad}    if {c} >= {s[1]} break")
                out.append(f"{pad}    {c} = {c} + 1")
                out += self.lines(s[2], indent + 1)
                out.append(f"{pad}}}")
        return out

    def source(self):
        out = ["func main() {"]
        for n in self.g.names:
            out.append(f"    var {n} = {self.t}({self.g.values[n]})")
        out += self.lines(self.statements, 1)
        out.append("    exit((v0 ^ v1 ^ v2 ^ v3 ^ v4 ^ v5) & 127)")
        out.append("}")
        return "\n".join(out) + "\n"

    # model

    def value(self, e, env, top=True):
        t = self.t
        k = e[0]
        if k == "var":
            return Value(env[e[1]])
        if k in ("const", "literal"):
            return Value(e[1])
        if k == "neg":
            a = self.value(e[1], env)
            if a.value is None:
                return a
            if a.value == limits(t)[0]:
                if self.is_const(e) and not self.lenient:
                    raise Reject("constant expression " + self.text(e))
                return Value(None, {"overflow"})
            return Value(-a.value)
        if k == "inv":
            a = self.value(e[1], env)
            return a if a.value is None else Value(~a.value)
        if k == "conv":
            return Value(wrap(e[1], self.wrapped(e[2], env)))
        if k == "bin":
            a = self.value(e[2], env)
            b = self.value(e[3], env)
            if a.value is None or b.value is None:
                return Value(None, a.panics | b.panics)
            return self.constant_check(e, self.binary(e[1], a.value, b.value))
        raise ValueError(k)

    def is_const(self, e):
        k = e[0]
        if k in ("const", "literal"):
            return True
        if k == "var":
            return False
        return all(self.is_const(x) for x in e[1:] if isinstance(x, tuple))

    def constant_check(self, e, result):
        # an operation on constants that is undefined is a compile error
        if not self.lenient and "overflow" in result.panics and \
                e[0] == "bin" and e[1] in ("+", "-", "*") and self.is_const(e):
            raise Reject("constant expression " + self.text(e))
        return result

    def binary(self, op, a, b):
        t = self.t
        if op in "+-*":
            r = a + b if op == "+" else a - b if op == "-" else a * b
            return Value(r) if fits(t, r) else Value(None, {"overflow"})
        if op in "/%":
            if b == 0 or not fits(t, trunc_div(a, b)):
                return Value(None, {"division"})
            q = trunc_div(a, b)
            return Value(q if op == "/" else a - b * q)
        if op in ("<<", ">>"):
            if not 0 <= b < BITS[t]:
                return Value(None, {"shift"})
            return Value(wrap(t, a << b) if op == "<<" else a >> b)
        if op == "&":
            return Value(a & b)
        if op == "|":
            return Value(a | b)
        return Value(a ^ b)

    def wrapped(self, e, env):
        # inside a conversion + - * wrap, the low bits are those of the
        # exact result
        k = e[0]
        if k == "var":
            return env[e[1]]
        if k in ("const", "literal"):
            return e[1]
        a, b = self.wrapped(e[2], env), self.wrapped(e[3], env)
        return a + b if e[1] == "+" else a - b if e[1] == "-" else a * b

    def truth(self, c, env):
        # (True or False, panics) with short circuit evaluation
        k = c[0]
        if k == "cmp":
            a = self.value(c[2], env)
            b = self.value(c[3], env)
            if a.value is None or b.value is None:
                return None, a.panics | b.panics
            x, y, op = a.value, b.value, c[1]
            return (x == y if op == "==" else x != y if op == "!=" else
                    x < y if op == "<" else x <= y if op == "<=" else
                    x > y if op == ">" else x >= y), frozenset()
        if k == "bnot":
            v, p = self.truth(c[1], env)
            return (None if v is None else not v), p
        left, p = self.truth(c[1], env)
        if left is None:
            return None, p
        if (k == "and" and not left) or (k == "or" and left):
            return left, frozenset()
        return self.truth(c[2], env)

    def run_statements(self, statements, env):
        for s in statements:
            if s[0] == "assign":
                v = self.value(s[2], env)
                if v.value is None:
                    return v.panics
                env[s[1]] = v.value
            elif s[0] == "if":
                v, p = self.truth(s[1], env)
                if v is None:
                    return p
                panics = self.run_statements(s[2] if v else s[3], env)
                if panics:
                    return panics
            else:
                for _ in range(s[1]):
                    panics = self.run_statements(s[2], env)
                    if panics:
                        return panics
        return None

    def static_expr(self, e):
        # the compiler evaluates constant operations while it compiles, also in
        # code that does not run
        if not isinstance(e, tuple):
            return
        for x in e[1:]:
            if isinstance(x, tuple):
                self.static_expr(x)
        if e[0] == "bin":
            if e[1] in ("<<", ">>") and self.is_const(e[3]) and \
                    not self.lenient:
                count = self.value(e[3], {}).value
                if count is None or not 0 <= count < BITS[self.t]:
                    raise Reject("constant shift count")
        # a division by zero panics when the program runs
        if e[0] == "neg" and self.is_const(e) and not self.lenient:
            self.value(e, {})
        if e[0] == "bin" and e[1] in "+-*" and self.is_const(e) and \
                not self.lenient:
            if "overflow" in self.value(e, {}).panics:
                raise Reject("constant expression " + self.text(e))

    def static_statements(self, statements):
        for s in statements:
            if s[0] == "assign":
                self.static_expr(s[2])
            elif s[0] == "if":
                self.static_expr(s[1])
                self.static_statements(s[2])
                self.static_statements(s[3])
            else:
                self.static_statements(s[2])

    def evaluate(self):
        self.static_statements(self.statements)
        env = dict(self.g.values)
        panics = self.run_statements(self.statements, env)
        if panics:
            return ("panic", tuple(sorted(panics)))
        return ("exit", (env["v0"] ^ env["v1"] ^ env["v2"] ^ env["v3"]
                         ^ env["v4"] ^ env["v5"]) & 127)


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    program = Program(rnd)
    alternative = None
    try:
        expected = program.evaluate()
    except Reject as r:
        expected = ("compile", str(r))
        # a constant that overflows may also be left to the check at run
        # time, then the program is judged as if the constant were a variable
        program.lenient = True
        try:
            alternative = program.evaluate()
        except Reject:
            alternative = None
    source = program.source()
    work = tempfile.mkdtemp(prefix="fuzze-")
    found = []
    try:
        for target in program.targets:
            got = run_target(work, source, target)
            if fl.known_crash(str(got[1])):
                continue
            if not matches(expected, got) and not (
                    alternative and expected[0] == "compile"
                    and constant_overflow(expected)
                    and matches(alternative, got)):
                found.append((f"expr-{expected[0]}",
                              f"{program.t} {target} expected {expected[0]} "
                              f"{got[0]}",
                              f"expected {expected}, got {got[0]} "
                              f"{str(got[1])[:300]}"))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found, expected[0]


def run_target(work, source, target):
    path = os.path.join(work, "f.baz")
    with open(path, "w") as f:
        f.write(source)
    options = ["--checks=" + CHECKS]
    if target == "x86_64":
        c, o, problem = fl.run_x86(fl.COMPILER, path, options, work)
    else:
        c, o = fl.run_fpga(fl.COMPILER, path, options, work)
        problem = None
    if c.timed_out:
        return ("timeout", "")
    if c.returncode:
        return ("compile", c.stderr)
    if problem:
        return (problem[0], problem[1])
    if o.kind == "panic":
        return ("panic", o.code)
    if o.kind == "signal":
        return ("signal", o.detail)
    if o.kind == "cpu-error":
        return ("cpu-error", o.detail)
    if o.kind == "timeout":
        return ("timeout", "")
    if o.code in (249, 250, 251, 252, 253, 254) and target != "x86_64":
        return ("panic", o.code)
    return ("exit", o.code)


def constant_overflow(expected):
    return expected[1].startswith("constant")


KIND_CODE = {"overflow": 249, "overlap": 251, "shift": 252, "division": 253}


def matches(expected, got):
    if expected[0] == "compile":
        return got[0] == "compile"
    if expected[0] == "panic":
        codes = {KIND_CODE[k] for k in expected[1]}
        # a failed bounds check has no kind here; the model has none
        return got[0] == "panic" and got[1] in codes
    return got == expected


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 500
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(1 << 30)
    jobs = int(sys.argv[3]) if len(sys.argv) > 3 else max(1, (os.cpu_count() or 2) - 2)
    findings = fl.Findings()
    print(f"seed {seed}, {count} programs, {jobs} jobs, checks {CHECKS}")
    tally = {"exit": 0, "panic": 0, "compile": 0}
    new = 0
    total = 0
    begin = time.time()
    with concurrent.futures.ProcessPoolExecutor(jobs) as pool:
        for n, (source, found, kind) in enumerate(
                pool.map(fuzz_one, [(seed, i) for i in range(count)],
                         chunksize=4), 1):
            tally[kind] += 1
            for category, key, detail in found:
                total += 1
                if findings.add(category, key, source, detail):
                    new += 1
                    print(f"NEW {category}: {key[:150]}", flush=True)
            if n % 500 == 0:
                print(f"  {n}/{count} {time.time() - begin:.0f}s {tally}", flush=True)
    print(f"{count} programs: {tally['exit']} ran to the end, "
          f"{tally['panic']} modeled panics, {tally['compile']} rejected, "
          f"{total} mismatches, {time.time() - begin:.0f}s")
    print(f"{new} new findings in {fl.FINDINGS}")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
