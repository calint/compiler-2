#!/usr/bin/env python3
# model based fuzzer of side effects: calls that write globals, a field of a
# global record and 'mut' arguments, next to the other operands of the same
# statement, see 'etc/docs/README.md' chapter 8
#
# evaluation is left to right. a statement is rejected when a name that an
# operand reads may be written by a call in a later operand of the same list:
# the arguments of a call and the sides of a comparison; an arithmetic list,
# nested groups included, reads each operand at its place, before a later call
# writes it; the index of a destination is computed before the value; a value
# written in parts is rejected when a part reads, or a call in it reads or
# writes, a part written before it; an assignment whose
# value is one call is rejected when the call uses the destination. the model
# applies the same rules and runs an accepted program left to right; each
# program is built with every function inlined and again with every function
# 'noinline', on both targets, all must agree with the model:
#
#   effects-mismatch  a build ends differently from the model
#   effects-reject    a build rejects a program the model accepts, or rejects
#                     it for another reason than the model
#   effects-accept    a build accepts a program the model rejects
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-effects.py [count] [seed] [jobs]
import random
import shutil
import sys
import tempfile

import fuzzlib as fl

MASK = 1023
OPS = {"+": lambda a, b: a + b, "-": lambda a, b: a - b,
       "&": lambda a, b: a & b, "|": lambda a, b: a | b,
       "^": lambda a, b: a ^ b}

# '{word}' is empty or 'noinline ', every function is inlined or none is
FUNCTIONS = '''\
func {word}wg0(k) res {{
    g0 = (g0 + k) & 1023
    res = g0
}}
func {word}wg1(k) res {{
    g1 = (g1 * 3 + k) & 1023
    res = g1 + 1
}}
func {word}rg0() res {{
    res = (g0 * 2 + 1) & 1023
}}
func {word}wm(x mut, k) res {{
    x = (x + k) & 1023
    res = x
}}
func {word}f2(a, b) res {{
    res = (a * 3 + b) & 1023
}}
func {word}wpx(k) res {{
    p.x = (p.x + k) & 1023
    res = p.x
}}
func {word}rpx() res {{
    res = p.x + 1
}}
func {word}rpy() res {{
    res = p.y + 2
}}
'''

# what each function reads and writes of the globals, as storage keys
USES = {
    "wg0": ({"g0"}, {"g0"}),
    "wg1": ({"g1"}, {"g1"}),
    "rg0": ({"g0"}, set()),
    "wm": (set(), set()),
    "f2": (set(), set()),
    "wpx": ({"p.x"}, {"p.x"}),
    "rpx": ({"p.x"}, set()),
    "rpy": ({"p.y"}, set()),
}

LEAVES = ["l0", "l1", "g0", "g1", "p.x", "p.y", "la[0]", "la[2]",
          "la[l1 & 3]"]

# the places a 'mut' argument names
PLACES = ["l0", "l1", "g0", "g1", "la[0]", "la[2]", "la[l1 & 3]"]

# what each rejection says
REJECTIONS = {
    "order": "may be written by the call",
    "parts": "in the same statement",
    "alias": "may share storage",
}


def key_of(text):
    # e.g. 'la[l1 & 3]' is an element at a run-time index, 'la[?]'
    return "la[?]" if text.startswith("la[l") else text


def overlaps(a, b):
    # the same name, or elements of 'la' where one is at a run-time index
    if a == b:
        return True
    if a.startswith("la[") and b.startswith("la["):
        return "la[?]" in (a, b)
    return False


def any_overlap(lhs, rhs):
    return any(overlaps(a, b) for a in lhs for b in rhs)


# a node: ('lit', n) ('name', text) ('call', function, [args])


def reads(node):
    # the names a node reads as written, the arguments of a call too
    if node[0] == "lit":
        return set()
    if node[0] == "name":
        result = {key_of(node[1])}
        if node[1] == "la[l1 & 3]":
            result.add("l1")
        return result
    if node[0] == "group":
        result = set()
        for a in node[1]:
            result |= reads(a)
        return result
    result = set()
    for a in node[2]:
        result |= reads(a)
    return result


def writes(node):
    # what the calls in a node write: the globals and a 'mut' argument
    if node[0] == "group":
        result = set()
        for a in node[1]:
            result |= writes(a)
        return result
    if node[0] != "call":
        return set()
    result = set(USES[node[1]][1])
    if node[1] == "wm":
        result.add(key_of(node[2][0][1]))
    for a in node[2]:
        result |= writes(a)
    return result


def effects(node):
    # what the calls in a node read or write
    if node[0] == "group":
        result = set()
        for a in node[1]:
            result |= effects(a)
        return result
    if node[0] != "call":
        return set()
    result = USES[node[1]][0] | writes(node)
    for a in node[2]:
        result |= effects(a)
    return result


def alias_call(node, dst):
    # the shared storage check of a call: its result destination 'dst' and a
    # global its body uses or a name it is given, or a name it is given and a
    # global its body writes
    used, written = USES[node[1]]
    names = {key_of(arg[1]) for arg in node[2] if arg[0] == "name"}
    if dst and (any_overlap({dst}, used | written | names)):
        return True
    return any_overlap(names, written)


def check_list(operands, ordered=True):
    # the rejection of a list as the compiler finds it: the order of its
    # operands ('assert_not_written_later') first, unless an arithmetic list
    # reads each at its place, then each operand in turn
    if ordered and any(any_overlap(reads(operands[j]), writes(operands[k]))
                       for k in range(1, len(operands)) for j in range(k)):
        return "order"
    for operand in operands:
        if operand[0] == "group":
            rejection = check_list(operand[1], ordered=False)
            if rejection:
                return rejection
        if operand[0] == "call":
            rejection = check_call(operand, None)
            if rejection:
                return rejection
    return None


def check_call(node, dst):
    # the shared storage of a call first, then its arguments as a list
    if alias_call(node, dst):
        return "alias"
    return check_list(node[2])


def show(node):
    if node[0] == "group":
        return "(" + show(node[1][0]) + "".join(
            f" {o} {show(x)}" for o, x in zip(node[2], node[1][1:])) + ")"
    if node[0] == "lit":
        return str(node[1])
    if node[0] == "name":
        return node[1]
    return f"{node[1]}({', '.join(show(a) for a in node[2])})"


class Machine:
    # the state of a run: the globals, the record 'p', the locals and 'la'
    def __init__(self):
        self.v = {"g0": 1, "g1": 2, "p.x": 3, "p.y": 4, "l0": 5, "l1": 6}
        self.la = [1, 2, 3, 4]

    def index(self, text):
        inner = text[3:-1]
        return self.v["l1"] & 3 if inner == "l1 & 3" else int(inner)

    def get(self, text):
        if text.startswith("la["):
            return self.la[self.index(text)]
        return self.v[text]

    def put(self, text, value):
        if text.startswith("la["):
            self.la[self.index(text)] = value
        else:
            self.v[text] = value

    def eval(self, node):
        if node[0] == "lit":
            return node[1]
        if node[0] == "name":
            return self.get(node[1])
        if node[0] == "group":
            total = self.eval(node[1][0])
            for o, x in zip(node[2], node[1][1:]):
                total = OPS[o](total, self.eval(x))
            return total
        name, args = node[1], node[2]
        if name == "wm":
            # note: the place is passed, its element chosen before the call
            place, k = args[0][1], self.eval(args[1])
            self.put(place, (self.get(place) + k) & MASK)
            return self.get(place)
        values = [self.eval(a) for a in args]
        if name == "wg0":
            self.v["g0"] = (self.v["g0"] + values[0]) & MASK
            return self.v["g0"]
        if name == "wg1":
            self.v["g1"] = (self.v["g1"] * 3 + values[0]) & MASK
            return self.v["g1"] + 1
        if name == "rg0":
            return (self.v["g0"] * 2 + 1) & MASK
        if name == "f2":
            return (values[0] * 3 + values[1]) & MASK
        if name == "wpx":
            self.v["p.x"] = (self.v["p.x"] + values[0]) & MASK
            return self.v["p.x"]
        if name == "rpx":
            return self.v["p.x"] + 1
        return self.v["p.y"] + 2


class Statement:
    # the source lines, the expression of the value mixed into 'acc', what a
    # run does on a machine and returns, and the rejection, none when accepted
    def __init__(self, lines, mixed, run, rejection=None):
        self.lines = lines
        self.mixed = mixed
        self.run = run
        self.rejection = rejection


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.names = 0

    def fresh(self):
        self.names += 1
        return f"v{self.names}"

    def operand(self, depth):
        rnd = self.rnd
        if depth <= 0 or rnd.random() < 0.45:
            if rnd.random() < 0.25:
                return ("lit", rnd.randint(0, 9))
            return ("name", rnd.choice(LEAVES))
        name = rnd.choice(list(USES))
        if name == "wm":
            return ("call", "wm", [("name", rnd.choice(PLACES)),
                                   ("lit", rnd.randint(1, 9))])
        if name in ("wg0", "wg1", "wpx"):
            return ("call", name, [self.operand(depth - 1)])
        if name == "f2":
            return ("call", name, [self.operand(depth - 1),
                                   self.operand(depth - 1)])
        return ("call", name, [])

    def group_or_operand(self):
        rnd = self.rnd
        if rnd.random() < 0.3:
            items = [self.operand(2) for _ in range(rnd.randint(2, 3))]
            family = rnd.choice([["+", "-"], ["&"], ["|"], ["^"]])
            return ("group", items, [rnd.choice(family) for _ in items[1:]])
        return self.operand(2)

    def statement(self):
        kind = self.rnd.choice(["list", "global_", "compare", "parts",
                                "call"])
        return getattr(self, kind)()

    def sum(self):
        rnd = self.rnd
        operands = [self.group_or_operand() for _ in range(rnd.randint(1, 4))]
        family = rnd.choice([["+", "-"], ["&"], ["|"], ["^"]])
        signs = [rnd.choice(family) for _ in operands[1:]]
        text = show(operands[0]) + "".join(
            f" {s} {show(o)}" for s, o in zip(signs, operands[1:]))

        def value(m):
            total = m.eval(operands[0])
            for s, o in zip(signs, operands[1:]):
                v = m.eval(o)
                total = OPS[s](total, v)
            return total
        return operands, text, value

    def list(self):
        operands, text, value = self.sum()
        name = self.fresh()
        return Statement([f"var {name} = {text}"], name, value,
                         check_list(operands, ordered=False))

    def call(self):
        call = ("call", "f2", [self.operand(2), self.operand(2)])
        name = self.fresh()
        return Statement([f"var {name} = {show(call)}"], name,
                         lambda m: m.eval(call), check_call(call, None))

    def compare(self):
        lhs, rhs = self.operand(2), self.operand(2)
        name = self.fresh()

        def run(m):
            a = m.eval(lhs)
            b = m.eval(rhs)
            return 1 if a < b else 0
        return Statement([f"var {name} = 0",
                          f"if {show(lhs)} < {show(rhs)} {{ {name} = 1 }}"],
                         name, run, check_list([lhs, rhs]))

    def global_(self):
        # a destination that a call in the value writes is written last; one
        # call whose body uses the destination shares storage with its result
        operands, text, value = self.sum()
        dst = self.rnd.choice(["g0", "g1", "la[l1 & 3]", "la[2]", "l1"])
        rejection = check_list(operands, ordered=False)
        mask = MASK

        def place(m):
            # the destination's index is computed before the value
            if dst == "la[l1 & 3]":
                return f"la[{m.v['l1'] & 3}]"
            return dst

        def run(m):
            at = place(m)
            m.put(at, value(m) & mask)
            return (m.la[0] + m.la[1] * 3 + m.la[2] * 5 + m.la[3] * 7
                    + m.v["l1"])
        if len(operands) == 1 and operands[0][0] == "call":
            # note: one operand is the value itself, no '& 1023'
            rejection = check_call(operands[0], key_of(dst))
            line = f"{dst} = {text}"
            mask = -1
        elif len(operands) == 1:
            line = f"{dst} = {text} & 1023"
        else:
            line = f"{dst} = ({text}) & 1023"
        return Statement([line], "la[0] + la[1] * 3 + la[2] * 5 + la[3] * 7 + l1", run, rejection)

    def parts(self):
        # rule A first: a part reads, or a call in it reads or writes, no part
        # written before it; then each part as it is compiled: a part that is
        # one call writes its result in place, so its body uses no part of its
        # field, then the order of the arguments
        a, b = self.operand(2), self.operand(2)
        rejection = None
        if any_overlap(reads(b) | effects(b), {"p.x"}):
            rejection = "parts"
        for part, field in ((a, "p.x"), (b, "p.y")):
            if rejection or part[0] != "call":
                continue
            rejection = check_call(part, field)

        def run(m):
            m.v["p.x"] = m.eval(a)
            m.v["p.y"] = m.eval(b)
            return m.v["p.x"] * 7 + m.v["p.y"]
        return Statement([f"p = {{{show(a)}, {show(b)}}}"], "p.x * 7 + p.y",
                         run, rejection)

    def program(self):
        # the statements up to the first rejected one, which ends the program
        statements = []
        for _ in range(self.rnd.randint(1, 4)):
            s = self.statement()
            statements.append(s)
            if s.rejection:
                break
        return statements


def source_of(statements, word):
    body = ["var acc = 0", "var l0 = 5", "var l1 = 6",
            "var la = []{1, 2, 3, 4}"]
    for s in statements:
        body += s.lines
        body.append(f"acc = (acc * 31 + {s.mixed}) & 0xffff")
    body.append("exit(acc & 127)")
    return ("type pt {x, y}\nvar g0 = 1\nvar g1 = 2\nvar p = pt{3, 4}\n"
            + FUNCTIONS.format(word=word) + "func main() {\n"
            + "".join(f"    {line}\n" for line in body) + "}\n")


def expected_of(statements):
    # ('reject', kind) or ('exit', code, b'') of a run left to right
    if statements[-1].rejection:
        return ("reject", statements[-1].rejection)
    m = Machine()
    acc = 0
    for s in statements:
        acc = (acc * 31 + s.run(m)) & 0xffff
    return ("exit", acc & 127, b"")


def judge(work, source, expected):
    found = []
    for target in fl.TARGETS:
        got = fl.build_and_run(work, source, target, ("--checks=noub",))
        if expected[0] == "reject":
            if got[0] == "compile" and REJECTIONS[expected[1]] in got[1]:
                continue
            if got[0] == "compile":
                message = got[1].strip().splitlines()[0] if got[1] else ""
                found.append(("effects-reject",
                              f"{target} {message.split(': ', 1)[-1]}",
                              f"expected a rejection '{expected[1]}', got "
                              f"{got}"))
                continue
            found.append(("effects-accept", f"{target} {expected[1]}",
                          f"expected a rejection '{expected[1]}', got {got}"))
            continue
        if got == expected:
            continue
        if got[0] == "compile":
            message = got[1].strip().splitlines()[0] if got[1] else ""
            found.append(("effects-reject",
                          f"{target} {message.split(': ', 1)[-1]}",
                          f"expected {expected}, got {got}"))
        elif got[0] in ("compile-hang", "invalid-asm", "link-failure"):
            found.append((got[0] if got[0] != "compile-hang"
                          else "compiler-hang", target, str(got)))
        else:
            found.append(("effects-mismatch", f"{target} {got[0]}",
                          f"expected {expected}, got {got}"))
    return found


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    statements = Generator(rnd).program()
    expected = expected_of(statements)
    work = tempfile.mkdtemp(prefix="fuzze-")
    found = []
    try:
        for word in ("", "noinline "):
            source = source_of(statements, word)
            found += [(c, f"{word.strip() or 'inline'} {k}", d)
                      for c, k, d in judge(work, source, expected)]
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source_of(statements, ""), found


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
