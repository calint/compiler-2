#!/usr/bin/env python3
# model based fuzzer of conditions: 'bool' variables, fields, array elements,
# parameters and results, 'let' bools, 'and' / 'or' / 'not' with short
# circuit, 'bool == bool', comparisons of mixed widths with literals on either
# side, conditions as arguments of inlined and 'noinline' functions whose
# bodies have 'loop', 'foo', 'break', 'continue' and an early 'return'
#
# every call adds its mark to a trace in a 'dat', python runs the same program
# and the program checks the trace after each statement and the bools at the
# end, exiting with the number of the first check that differs:
#
#   bools-mismatch  a build ends differently from the model
#   bools-reject    a generated program is rejected
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-bools.py [count] [seed] [jobs]
import random
import sys

import fuzzlib as fl

TYPES = ("i8", "i16", "i32")
COMPARES = {
    "<": lambda a, b: a < b,
    "<=": lambda a, b: a <= b,
    "==": lambda a, b: a == b,
    "!=": lambda a, b: a != b,
    ">": lambda a, b: a > b,
    ">=": lambda a, b: a >= b,
}
MIRRORED = {"<": ">", "<=": ">=", ">": "<", ">=": "<=", "==": "==", "!=": "!="}
OPTION_LISTS = fl.OPTION_LISTS + (("--checks=noub", "--versions=nested"),
                                  ("--checks=", "--nopt"))
# the trace is a hash of the marks of the calls in the order they run
TRACE_MODULUS = 1000003

HEADER = """dat trace = i32[1]
type R { a bool, n i8, b bool, m i16 }
func t(v i32) { trace[0] = (trace[0] * 7 + v) % 1000003 }
func bi(k i32, v bool) res bool { t(k)  res = v }
func noinline nbi(k i32, v bool) res bool { t(k)  res = v }
func eqb(k i32, a bool, b bool) res bool {
    t(k)
    res = a == b
}
func noinline neqb(k i32, a bool, b bool) res bool {
    t(k)
    if a == b {
        res = true
        return
    }
    res = false
}
func anyb(k i32, a bool[]) res bool {
    t(k)
    res = false
    foo a {
        if e {
            res = true
            break
        }
    }
}
func noinline nanyb(k i32, a bool[]) res bool {
    t(k)
    res = false
    foo a {
        if e and i >= 0 {
            res = true
            return
        }
    }
}
func lp(k i32, v bool) res bool {
    res = v
    var i = 0
    loop {
        i = i + 1
        if i > 2 break
        if v continue
        t(k)
    }
}
func noinline nlp(k i32, v bool) res bool {
    res = not v
    var i = 0
    loop {
        i = i + 1
        if i > 2 or v break
        t(k)
    }
    res = not res
}
func cii8(k i32, v i8) res i8 { t(k)  res = v }
func noinline ncii8(k i32, v i8) res i8 { t(k)  res = v }
func cii16(k i32, v i16) res i16 { t(k)  res = v }
func noinline ncii16(k i32, v i16) res i16 { t(k)  res = v }
func cii32(k i32, v i32) res i32 { t(k)  res = v }
func noinline ncii32(k i32, v i32) res i32 { t(k)  res = v }
"""


def spelled(value):
    return "true" if value else "false"


class State:
    # the values of a run of the model
    def __init__(self):
        self.trace = 0
        self.ints = {}
        self.bools = {}
        self.fields = {}
        self.array = []
        self.lets = {}

    def mark(self, k):
        self.trace = (self.trace * 7 + k) % TRACE_MODULUS


def called(s, k, value):
    # an inlined or 'noinline' 'bi' / 'nbi', its argument runs first
    v = value(s)
    s.mark(k)
    return v


def called_eq(s, k, a, b):
    x = a(s)
    y = b(s)
    s.mark(k)
    return x == y


def called_loop(s, k, value):
    # 'lp' / 'nlp' mark twice when the argument is false
    v = value(s)
    if not v:
        s.mark(k)
        s.mark(k)
    return v


class Generator:
    # the parts are (source, evaluate) with 'evaluate(state)' the value, an
    # integer side also has its type, None for a literal
    def __init__(self, rnd):
        self.r = rnd
        self.k = 0
        self.ints = {}
        self.bools = {}
        self.fields = {"r.a": True, "r.b": False}
        self.array = [False, True, False, True]
        self.lets = {"KT": True, "KF": False}
        # the variable a condition is assigned to, which a call that takes it
        # or a part of it would share storage with, e.g. 'b0 = nbi(1, b0)'
        self.destination = None

    def next_mark(self):
        self.k += 1
        return self.k

    def literal(self, t):
        lo, hi = fl.limits(t)
        v = self.r.randint(lo, hi)
        return (str(v), None, lambda s, v=v: v)

    def typed_constant(self, t):
        # a conversion of a literal that may wrap
        raw = self.r.randint(-70000, 70000)
        v = fl.wrap(t, raw)
        return (f"{t}({raw})", t, lambda s, v=v: v)

    def int_side(self, t, allow_literal=True):
        # a side of type at most 't'
        r = self.r
        choice = r.randrange(5)
        names = [n for n, nt in self.ints.items()
                 if fl.BITS[nt] <= fl.BITS[t]]
        if choice == 0 and allow_literal:
            lo, hi = fl.limits(t)
            v = r.choice([lo, hi, 0, -1, 1, r.randint(lo, hi)])
            return (str(v), None, lambda s, v=v: v)
        if choice == 1:
            return self.typed_constant(
                r.choice([x for x in TYPES if fl.BITS[x] <= fl.BITS[t]]))
        if choice == 2 and names:
            n = r.choice(names)
            k = self.next_mark()
            f = r.choice(["ci", "nci"])
            nt = self.ints[n]
            return (f"{f}{nt}({k}, {n})", nt,
                    lambda s, n=n, k=k: called(s, k, lambda s: s.ints[n]))
        if names:
            n = r.choice(names)
            return (n, self.ints[n], lambda s, n=n: s.ints[n])
        return self.literal(t)

    def comparison(self):
        r = self.r
        t = r.choice(TYPES)
        names = [n for n, nt in self.ints.items() if nt == t]
        op = r.choice(list(COMPARES))
        lhs = self.int_side(t, allow_literal=False)
        if lhs[1] != t:
            # the left side decides the type, make it exactly 't'
            if names and r.random() < 0.7:
                n = r.choice(names)
                lhs = (n, t, lambda s, n=n: s.ints[n])
            else:
                lhs = self.typed_constant(t)
        rhs = self.int_side(t)
        if r.random() < 0.15:
            # a literal on the left that fits the type of the right side
            lhs = self.literal(rhs[1] or t)
        if lhs[1] and rhs[1] and fl.BITS[rhs[1]] > fl.BITS[lhs[1]]:
            # a wider right side would be rejected, the wider goes first
            lhs, rhs = rhs, lhs
            op = MIRRORED[op]
        if lhs[1] is None and rhs[1] is not None and \
                not fl.fits(rhs[1], lhs[2](None)):
            return self.comparison()
        compare, left, right = COMPARES[op], lhs[2], rhs[2]

        def evaluate(s):
            a = left(s)
            return compare(a, right(s))
        return (f"{lhs[0]} {op} {rhs[0]}", evaluate)

    def plain_bool(self):
        # a bool variable, field or element of 'ba' not in the destination
        paths = [(n, lambda s, n=n: s.bools[n]) for n in self.bools]
        paths += [(n, lambda s, n=n: s.fields[n]) for n in self.fields]
        paths += [(f"ba[{i}]", lambda s, i=i: s.array[i])
                  for i in range(len(self.array))]
        return self.r.choice([p for p in paths
                              if p[0].split(".")[0].split("[")[0]
                              != self.destination])

    def bool_simple(self):
        # a bool without a list of its own, a side of 'bool == bool'
        r = self.r
        choice = r.randrange(5)
        if choice == 0:
            v = r.random() < 0.5
            return (spelled(v), lambda s, v=v: v)
        if choice == 3:
            k = self.next_mark()
            f = r.choice(["bi", "nbi"])
            sub = self.comparison()
            return (f"{f}({k}, {sub[0]})",
                    lambda s, k=k, e=sub[1]: called(s, k, e))
        return self.plain_bool()

    def bool_atom(self, depth):
        r = self.r
        choice = r.randrange(9)
        deeper = depth < 3
        if choice == 0:
            v = r.random() < 0.5
            return (spelled(v), lambda s, v=v: v)
        if choice == 1:
            n = r.choice(list(self.lets))
            return (n, lambda s, n=n: s.lets[n])
        if choice == 2 and self.destination != "ba" and r.random() < 0.3:
            # note: 'ix' stays 1
            k = self.next_mark()
            f = r.choice(["bi", "nbi"])
            return (f"{f}({k}, ba[ix])",
                    lambda s, k=k: called(s, k, lambda s: s.array[1]))
        if choice in (2, 3, 4):
            return self.plain_bool()
        if choice in (5, 6) and deeper:
            k = self.next_mark()
            f = r.choice(["bi", "nbi"])
            sub = self.condition(depth + 1)
            return (f"{f}({k}, {sub[0]})",
                    lambda s, k=k, e=sub[1]: called(s, k, e))
        if choice == 7 and deeper:
            k = self.next_mark()
            f = r.choice(["eqb", "neqb"])
            a = self.condition(depth + 1)
            b = self.condition(depth + 1)
            return (f"{f}({k}, {a[0]}, {b[0]})",
                    lambda s, k=k, a=a[1], b=b[1]: called_eq(s, k, a, b))
        if choice == 8 and deeper and self.destination != "ba" and \
                r.random() < 0.4:
            k = self.next_mark()
            f = r.choice(["anyb", "nanyb"])
            return (f"{f}({k}, ba)",
                    lambda s, k=k: called(s, k, lambda s: any(s.array)))
        if choice == 8 and deeper and r.random() < 0.5:
            k = self.next_mark()
            f = r.choice(["lp", "nlp"])
            sub = self.condition(depth + 1)
            return (f"{f}({k}, {sub[0]})",
                    lambda s, k=k, e=sub[1]: called_loop(s, k, e))
        if choice == 8 and deeper:
            a = self.bool_simple()
            b = self.bool_simple()
            op = r.choice(["==", "!="])
            return (f"{a[0]} {op} {b[0]}",
                    lambda s, a=a[1], b=b[1], op=op:
                        (a(s) == b(s)) == (op == "=="))
        return self.comparison()

    def condition(self, depth=0):
        # (source, evaluate, element count)
        r = self.r
        count = r.choice([1, 1, 2, 2, 3, 4]) if depth < 3 else 1
        connective = r.choice(["and", "or"])
        elements = []
        for _ in range(count):
            if depth < 3 and count > 1 and r.random() < 0.25:
                sub = self.condition(depth + 1)
                if sub[2] > 1:
                    nots = r.choice(["", "", "not ", "not not "])
                    elements.append((f"{nots}({sub[0]})", sub[1],
                                     len(nots.split())))
                    continue
            atom = self.bool_atom(depth)
            # 'not' before a comparison or 'bool == bool' applies to it whole
            nots = r.choice(["", "", "", "not ", "not not "])
            elements.append((f"{nots}{atom[0]}", atom[1], len(nots.split())))
        source = f" {connective} ".join(e[0] for e in elements)

        def evaluate(s):
            for _, value, nots in elements:
                v = value(s) != (nots % 2 == 1)
                if connective == "and" and not v:
                    return False
                if connective == "or" and v:
                    return True
            return connective == "and"
        return (source, evaluate, count)

    def loop_statement(self, s, body):
        # a loop whose conditions mark and 'continue'
        r = self.r
        counter = f"c{self.next_mark()}"
        repeats = r.randint(1, 3)
        first, second = self.condition(), self.condition()
        m1, m2 = self.next_mark(), self.next_mark()
        # note: the counter keeps a constant true from making 't(m2)'
        #       unreachable, which is rejected
        guard = f"({second[0]})" if second[2] > 1 else second[0]
        body.append(f"    var {counter} = 0\n"
                    f"    loop {{\n"
                    f"        {counter} = {counter} + 1\n"
                    f"        if {first[0]} {{\n"
                    f"            t({m1})\n"
                    f"        }}\n"
                    f"        if {counter} > {repeats} break\n"
                    f"        if {counter} > 0 and {guard} continue\n"
                    f"        t({m2})\n"
                    f"    }}")
        for turn in range(1, repeats + 2):
            if first[1](s):
                s.mark(m1)
            if turn > repeats:
                break
            if second[1](s):
                continue
            s.mark(m2)

    def if_statement(self, s, body):
        r = self.r
        branches = r.randint(1, 3)
        conditions = [self.condition() for _ in range(branches)]
        marks = [self.next_mark() for _ in range(branches + 1)]
        has_else = r.random() < 0.6
        parts = []
        for i, c in enumerate(conditions):
            keyword = "if" if i == 0 else "} else if"
            parts.append(f"    {keyword} {c[0]} {{\n        t({marks[i]})")
        text = "\n".join(parts)
        if has_else:
            text += f"\n    }} else {{\n        t({marks[-1]})"
        body.append(text + "\n    }")
        for i, c in enumerate(conditions):
            if c[1](s):
                s.mark(marks[i])
                return
        if has_else:
            s.mark(marks[-1])

    def statement(self, s, body):
        r = self.r
        kind = r.randrange(6)
        if kind == 0 and self.bools:
            n = r.choice(list(self.bools))
            self.destination = n
            c = self.condition()
            body.append(f"    {n} = {c[0]}")
            s.bools[n] = c[1](s)
        elif kind == 1:
            n = r.choice(list(self.fields))
            self.destination = "r"
            c = self.condition()
            body.append(f"    {n} = {c[0]}")
            s.fields[n] = c[1](s)
        elif kind == 2:
            i = r.randrange(len(self.array))
            self.destination = "ba"
            c = self.condition()
            body.append(f"    ba[{i}] = {c[0]}")
            s.array[i] = c[1](s)
        elif kind == 3:
            n = f"d{self.next_mark()}"
            c = self.condition()
            body.append(f"    var {n} = {c[0]}")
            v = c[1](s)
            self.bools[n] = v
            s.bools[n] = v
        elif kind == 4:
            self.if_statement(s, body)
        else:
            self.loop_statement(s, body)
        self.destination = None

    def program(self):
        # (source, expected)
        r = self.r
        s = State()
        s.lets = dict(self.lets)
        s.fields = dict(self.fields)
        s.array = list(self.array)
        body = [f"    let {n} = {spelled(v)}" for n, v in self.lets.items()]
        for i in range(r.randint(2, 5)):
            t = r.choice(TYPES)
            lo, hi = fl.limits(t)
            v = r.choice([lo, hi, 0, -1, 1, r.randint(lo, hi)])
            self.ints[f"v{i}"] = t
            s.ints[f"v{i}"] = v
            body.append(f"    var v{i} = {t}({v})")
        for i in range(r.randint(1, 3)):
            v = r.random() < 0.5
            self.bools[f"b{i}"] = v
            s.bools[f"b{i}"] = v
            body.append(f"    var b{i} = {spelled(v)}")
        body += ["    var r = R{true, 1, false, 2}",
                 "    var ba = bool[4]",
                 "    ba[1] = true",
                 "    ba[3] = true",
                 "    var ix = 1"]
        # note: the checks exit with 2 and up, the final ones with 201 to 203
        for check in range(2, 2 + r.randint(3, 10)):
            self.statement(s, body)
            body.append(f"    if trace[0] != {s.trace} exit({check})")
        for n in self.bools:
            body.append(f"    if {n} != {spelled(s.bools[n])} exit(201)")
        for n in self.fields:
            body.append(f"    if {n} != {spelled(s.fields[n])} exit(202)")
        for i, v in enumerate(s.array):
            body.append(f"    if ba[{i}] != {spelled(v)} exit(203)")
        body.append("    exit(0)")
        source = HEADER + "func main() {\n" + "\n".join(body) + "\n}\n"
        return source, ("exit", 0, b"")


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    source, expected = Generator(rnd).program()
    found = fl.judge(source, expected, option_lists=OPTION_LISTS,
                     prefix="bools")
    return source, found


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
