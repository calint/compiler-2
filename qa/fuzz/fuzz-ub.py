#!/usr/bin/env python3
# differential fuzzer for the undefined behavior checks
#
# undefined behavior of a baz program and the check that catches it:
#   1. signed overflow of + - * and unary -           overflow
#   2. division by zero, MIN / -1, MIN % -1           division
#   3. shift count outside the width of the type      shift
#   4. index outside an array, also nested in fields  bounds (upper, lower)
#   5. range outside an array in array_copy,
#      arrays_equal, read and write                   bounds
#   6. array_copy whose destination starts inside
#      the source                                     overlap
#   7. unbounded recursion of a noinline function     frame, stack or the
#                                                     operating system
#   8. narrowing, an unset result, an unset variable  rejected by the compiler
#   9. aliasing arguments and results                 rejected by the compiler,
#                                                     see fuzz-alias.sh
#
# generates random programs of these kinds, models each program exactly in
# python and compares what the compiled program does on x86_64 and rv32i-fpga
# (i64 only on x86_64) under '--checks=noub': a program that the model runs to
# the end must exit with the modeled value, a program the model stops with a
# panic must panic with the same kind, a program that the model rejects must be
# rejected by the compiler
#
# scenarios: arith (i8 .. i64), widen (mixed widths), arrays (nested arrays and
# records), calls (recursion and array parameters), deep (unbounded recursion)
#
# usage: [FUZZ_CHECKS=noub,-overflow] fuzz-ub.py [count] [seed] [scenario]
# the checks can be changed to see that the fuzzer catches a missing one
import os
import random
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..")
COMPILER = os.path.join(ROOT, "baz")
EMULATOR = os.path.join(ROOT, "fpga-emulator", "osqa")

BITS = {"i8": 8, "i16": 16, "i32": 32, "i64": 64}
# exit codes of a panic on rv32i-fpga
FPGA_PANIC = {"overflow": 249, "stack": 250, "overlap": 251, "shift": 252,
              "division": 253, "frame": 254, "bounds": 255}
# stderr of a panic on x86_64, a failed bounds check prints nothing without
# '--checks=line'
X86_PANIC = {"overflow": "panic: overflow", "overlap": "panic: overlap",
             "shift": "panic: shift", "division": "panic: division",
             "frame": "panic: frame overflow"}
CHECKS = os.environ.get("FUZZ_CHECKS", "noub")
SIGSEGV = 139
TIMEOUT = 1000


class Panic(Exception):
    # 'kinds' are the failures that are accepted when a program fails in more
    # than one way and the order of the checks is not defined
    def __init__(self, *kinds):
        super().__init__(kinds[0])
        self.kinds = kinds


class Rejected(Exception):
    pass


class Skip(Exception):
    # a program whose outcome the model does not decide
    pass


def limits(t):
    bits = BITS[t]
    return -(1 << (bits - 1)), (1 << (bits - 1)) - 1


def fits(t, value):
    low, high = limits(t)
    return low <= value <= high


def wrap(t, value):
    bits = BITS[t]
    value &= (1 << bits) - 1
    return value - (1 << bits) if value >> (bits - 1) else value


def interesting(rnd, t, benign=False):
    low, high = limits(t)
    # benign programs rarely panic, so their last statements run too
    if benign:
        return rnd.choice([rnd.randint(-12, 12), rnd.randint(-12, 12), 0, 1, -1,
                           rnd.randint(low, high) if rnd.random() < 0.1 else 2])
    choices = [low, low + 1, -1, 0, 1, 2, high - 1, high, 3, -2]
    if rnd.random() < 0.6:
        value = rnd.choice(choices)
    elif rnd.random() < 0.5:
        value = rnd.randint(-20, 20)
    else:
        value = rnd.randint(low, high)
    # the literal of the i64 minimum is out of range, it is reached by
    # arithmetic
    return max(value, low + 1) if t == "i64" else value


def literal(t, value):
    return f"{t}({value})"


def trunc_div(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def apply(t, op, a, b):
    low, _ = limits(t)
    if op in "+-*":
        r = a + b if op == "+" else a - b if op == "-" else a * b
        if not fits(t, r):
            raise Panic("overflow")
        return r
    if op in "/%":
        if b == 0 or not fits(t, trunc_div(a, b)):
            raise Panic("division")
        q = trunc_div(a, b)
        return q if op == "/" else a - b * q
    if op in ("<<", ">>"):
        if not 0 <= b < BITS[t]:
            raise Panic("shift")
        return wrap(t, a << b) if op == "<<" else a >> b
    raise ValueError(op)


def evaluate_chain(t, vals, ops):
    # '*' binds tighter than '+' and '-', left to right
    items = [vals[0]]
    pending = []
    for o, v in zip(ops, vals[1:]):
        if o == "*":
            items[-1] = apply(t, "*", items[-1], v)
        else:
            pending.append(o)
            items.append(v)
    result = items[0]
    for o, v in zip(pending, items[1:]):
        result = apply(t, o, result, v)
    return result


def targets_of(types):
    return ["x86_64"] if "i64" in types else ["x86_64", "rv32i-fpga"]


# arith: one type, operations at the width of the type

class Arith:
    name = "arith"

    def __init__(self, rnd):
        self.rnd = rnd
        self.t = rnd.choice(list(BITS))
        self.targets = targets_of([self.t])
        self.benign = rnd.random() < 0.5
        self.names = ["v0", "v1", "v2", "v3"]
        self.counts = ["c0", "c1"]
        self.values = {n: interesting(rnd, self.t, self.benign)
                       for n in self.names}
        bits = BITS[self.t]
        for c in self.counts:
            self.values[c] = (
                rnd.randint(0, bits - 1) if rnd.random() < 0.8
                else rnd.choice([-1, bits, bits + 1, bits - 1, 0]))
            if not fits(self.t, self.values[c]):
                self.values[c] = bits - 1
        self.array = [interesting(rnd, self.t, self.benign)
                      for _ in range(8)]
        self.statements = []
        for _ in range(rnd.randint(2, 10)):
            self.statements.append(self.make_statement())

    def operand(self, allow_constant):
        rnd = self.rnd
        if allow_constant and rnd.random() < 0.25:
            return ("const", interesting(rnd, self.t, self.benign))
        return ("var", rnd.choice(self.names))

    def make_statement(self):
        rnd = self.rnd
        dst = rnd.choice(self.names)
        kind = rnd.random()
        if kind < 0.5:
            count = rnd.choice([1, 1, 2, 3])
            terms = [self.operand(True)]
            ops = []
            for _ in range(count):
                ops.append(rnd.choice("+-*"))
                terms.append(self.operand(True))
            # constants next to each other are folded by the compiler and a
            # first constant is reordered, keep a variable in between
            if terms[0][0] == "const":
                terms[0] = ("var", rnd.choice(self.names))
            for i in range(1, len(terms)):
                if terms[i][0] == "const" and terms[i - 1][0] == "const":
                    terms[i] = ("var", rnd.choice(self.names))
            return ("chain", dst, terms, ops)
        if kind < 0.7:
            return ("binary", dst, rnd.choice(["/", "%"]),
                    ("var", rnd.choice(self.names)),
                    ("var", rnd.choice(self.names)))
        if kind < 0.82:
            return ("binary", dst, rnd.choice(["<<", ">>"]),
                    ("var", rnd.choice(self.names)),
                    ("var", rnd.choice(self.counts)))
        if kind < 0.92:
            return ("neg", dst, ("var", rnd.choice(self.names)))
        return ("index", dst, ("var", rnd.choice(self.names)))

    @staticmethod
    def operand_text(operand):
        if operand[0] == "const":
            return str(operand[1])
        return operand[1]

    def source(self):
        t = self.t
        out = ["func main() {"]
        for n in self.names:
            out.append(f"    var {n} = {literal(t, self.values[n])}")
        for c in self.counts:
            out.append(f"    var {c} = {literal(t, self.values[c])}")
        out.append(f"    var arr = {t}[8]")
        for i, v in enumerate(self.array):
            out.append(f"    arr[{i}] = {v}")
        for s in self.statements:
            if s[0] == "chain":
                _, dst, terms, ops = s
                text = self.operand_text(terms[0])
                for o, term in zip(ops, terms[1:]):
                    text += f" {o} {self.operand_text(term)}"
                out.append(f"    {dst} = {text}")
            elif s[0] == "binary":
                _, dst, op, a, b = s
                out.append(f"    {dst} = {self.operand_text(a)} {op} "
                           f"{self.operand_text(b)}")
            elif s[0] == "neg":
                out.append(f"    {s[1]} = -{self.operand_text(s[2])}")
            else:
                out.append(f"    {s[1]} = arr[{self.operand_text(s[2])}]")
        out.append("    exit(v0 & 127)")
        out.append("}")
        return "\n".join(out) + "\n"

    def evaluate(self):
        t = self.t
        env = dict(self.values)
        for s in self.statements:
            if s[0] == "chain":
                _, dst, terms, ops = s
                vals = [self.value_of(env, x) for x in terms]
                env[dst] = evaluate_chain(t, vals, ops)
            elif s[0] == "binary":
                _, dst, op, a, b = s
                env[dst] = apply(t, op, self.value_of(env, a),
                                 self.value_of(env, b))
            elif s[0] == "neg":
                a = self.value_of(env, s[2])
                if a == limits(t)[0]:
                    raise Panic("overflow")
                env[s[1]] = -a
            else:
                index = self.value_of(env, s[2])
                if not 0 <= index < 8:
                    raise Panic("bounds")
                env[s[1]] = self.array[index]
        return env["v0"] & 127

    @staticmethod
    def value_of(env, operand):
        return operand[1] if operand[0] == "const" else env[operand[1]]


# widen: variables of different widths, a result is computed at the width of
# the destination, a destination narrower than an operand is rejected

class Widen:
    name = "widen"

    def __init__(self, rnd):
        self.rnd = rnd
        self.benign = rnd.random() < 0.5
        self.types = {}
        self.values = {}
        for n in ["a", "b", "c", "d", "e"]:
            t = rnd.choice(list(BITS))
            self.types[n] = t
            self.values[n] = interesting(rnd, t, self.benign)
        self.targets = targets_of(self.types.values())
        self.statements = []
        names = list(self.types)
        for _ in range(rnd.randint(2, 8)):
            dst = rnd.choice(names)
            # operands wider than the destination are rejected, mostly
            # generate programs that are accepted
            fitting = [n for n in names
                       if BITS[self.types[n]] <= BITS[self.types[dst]]]
            pool = names if rnd.random() < 0.1 else fitting
            count = rnd.choice([1, 1, 2])
            terms = [rnd.choice(pool)]
            ops = []
            for _ in range(count):
                ops.append(rnd.choice(["+", "-", "*", "/", "%"]))
                terms.append(rnd.choice(pool))
            self.statements.append((dst, terms, ops))

    def source(self):
        out = ["func main() {"]
        for n, v in self.values.items():
            out.append(f"    var {n} = {literal(self.types[n], v)}")
        for dst, terms, ops in self.statements:
            text = terms[0]
            for o, term in zip(ops, terms[1:]):
                text += f" {o} {term}"
            out.append(f"    {dst} = {text}")
        out.append("    exit(a & 127)")
        out.append("}")
        return "\n".join(out) + "\n"

    def evaluate(self):
        env = dict(self.values)
        for dst, terms, ops in self.statements:
            t = self.types[dst]
            if any(BITS[self.types[x]] > BITS[t] for x in terms):
                raise Rejected("narrowed")
        for dst, terms, ops in self.statements:
            t = self.types[dst]
            # '/' and '%' bind like '*'
            vals = [env[x] for x in terms]
            items = [vals[0]]
            pending = []
            for o, v in zip(ops, vals[1:]):
                if o in "*/%":
                    items[-1] = apply(t, o, items[-1], v)
                else:
                    pending.append(o)
                    items.append(v)
            result = items[0]
            for o, v in zip(pending, items[1:]):
                result = apply(t, o, result, v)
            env[dst] = result
        return env["a"] & 127


# arrays: runtime indexes into nested arrays of records, array_copy,
# arrays_equal, read and write with runtime ranges

TYPES_SOURCE = """type cell { a i32, b i32 }
type row { cells cell[4], tag i32 }
type grid { rows row[3], flat i32[6], bytes i8[8] }
"""


class Place:
    # an array that a statement can name: source text, length, element kind,
    # the indexes of the path to it with their lengths and how to read it from
    # the model
    def __init__(self, text, length, kind, path, get):
        self.text = text
        self.length = length
        self.kind = kind
        self.path = path
        self.get = get


class Arrays:
    name = "arrays"

    def __init__(self, rnd):
        self.rnd = rnd
        self.targets = ["x86_64", "rv32i-fpga"]
        # benign programs keep every index and count in range
        self.benign = rnd.random() < 0.6
        self.indexes = {}
        for n in ["i0", "i1", "i2", "i3"]:
            self.indexes[n] = self.index_value(8)
        self.counts = {}
        for n in ["n0", "n1"]:
            self.counts[n] = self.index_value(6)
        self.values = {"v0": rnd.randint(-20, 20), "v1": rnd.randint(-20, 20)}
        self.bytes_values = {"b0": rnd.randint(-100, 100)}
        self.places = [
            Place("g.flat", 6, "i32", [], lambda m, ix: m["flat"]),
            Place("h", 6, "i32", [], lambda m, ix: m["h"]),
            Place("w", 6, "i32", [], lambda m, ix: m["w"]),
            Place("g.bytes", 8, "i8", [], lambda m, ix: m["bytes"]),
            Place("g.rows", 3, "row", [], lambda m, ix: m["rows"]),
        ]
        for name in ["i0", "i1", "i2", "i3"]:
            self.places.append(Place(
                f"g.rows[{name}].cells", 4, "cell", [(name, 3)],
                lambda m, ix, name=name: m["rows"][ix[name]]["cells"]))
        self.statements = []
        for _ in range(rnd.randint(3, 12)):
            self.statements.append(self.make_statement())

    def index_value(self, length):
        rnd = self.rnd
        if self.benign:
            return rnd.randint(0, 2)
        if rnd.random() < 0.75:
            return rnd.randint(0, length - 1)
        return rnd.choice([-1, length, length + 1, -3, 0, length - 1])

    def index_name(self):
        return self.rnd.choice(list(self.indexes))

    def count_name(self):
        return self.rnd.choice(list(self.counts))

    def make_statement(self):
        rnd = self.rnd
        kind = rnd.random()
        if kind < 0.1:
            return ("set_flat", self.index_name(), rnd.randint(-20, 20))
        if kind < 0.18:
            return ("get_flat", self.index_name(), rnd.choice(["v0", "v1"]))
        if kind < 0.26:
            return ("set_h", self.index_name(), rnd.choice(["v0", "v1"]))
        if kind < 0.34:
            return ("set_bytes", self.index_name(), rnd.randint(-100, 100))
        if kind < 0.40:
            return ("get_bytes", self.index_name(), rnd.choice(["v0", "v1"]))
        if kind < 0.50:
            return ("set_cell", self.index_name(), self.index_name(),
                    rnd.choice("ab"), rnd.randint(-20, 20))
        if kind < 0.56:
            return ("get_cell", self.index_name(), self.index_name(),
                    rnd.choice("ab"), rnd.choice(["v0", "v1"]))
        if kind < 0.60:
            return ("set_tag", self.index_name(), rnd.randint(-20, 20))
        if kind < 0.64:
            return ("equal", rnd.choice([0, 1, 2]), rnd.choice([0, 1, 2]),
                    self.count_name())
        if kind < 0.68:
            return ("write", rnd.choice([0, 1, 2]), self.count_name())
        if kind < 0.72:
            # a read in range waits for input that the fuzzer does not give,
            # only a read that the check stops is generated
            bad = [n for n, v in self.counts.items() if not 0 <= v <= 6]
            if bad:
                return ("read", rnd.choice([0, 1, 2]), rnd.choice(bad))
            return ("write", rnd.choice([0, 1, 2]), self.count_name())
        return self.make_copy()

    def make_copy(self):
        rnd = self.rnd
        # a copy needs two places of the same element kind
        while True:
            src = rnd.choice(self.places)
            dst = rnd.choice(self.places)
            # half of the copies stay in one array, where they can overlap
            if rnd.random() < 0.5:
                dst = src
            if src.kind == dst.kind:
                break
        return ("copy", self.places.index(src), self.index_name(),
                self.places.index(dst), self.index_name(), self.count_name())

    def source(self):
        out = [TYPES_SOURCE + "func main() {"]
        out.append("    var g = grid")
        out.append("    var h = i32[6]")
        out.append("    var w = i32[6]")
        for n, v in self.indexes.items():
            out.append(f"    var {n} = i32({v})")
        for n, v in self.counts.items():
            out.append(f"    var {n} = i32({v})")
        for n, v in self.values.items():
            out.append(f"    var {n} = i32({v})")
        for n, v in self.bytes_values.items():
            out.append(f"    var {n} = i8({v})")
        for s in self.statements:
            out.append("    " + self.statement_text(s))
        out.append("    var sum = i32(0)")
        for i, text in enumerate(self.checksum_places()):
            out.append(f"    sum = sum + {text} * {i % 13 + 1}")
        out.append("    exit(sum & 127)")
        out.append("}")
        return "\n".join(out) + "\n"

    @staticmethod
    def checksum_places():
        texts = [f"g.flat[{i}]" for i in range(6)]
        texts += [f"h[{i}]" for i in range(6)]
        texts += [f"w[{i}]" for i in range(6)]
        texts += [f"g.bytes[{i}]" for i in range(8)]
        for r in range(3):
            texts.append(f"g.rows[{r}].tag")
            for c in range(4):
                texts.append(f"g.rows[{r}].cells[{c}].a")
                texts.append(f"g.rows[{r}].cells[{c}].b")
        return texts

    def statement_text(self, s):
        k = s[0]
        if k == "set_flat":
            return f"g.flat[{s[1]}] = {s[2]}"
        if k == "get_flat":
            return f"{s[2]} = g.flat[{s[1]}]"
        if k == "set_h":
            return f"h[{s[1]}] = {s[2]}"
        if k == "set_bytes":
            return f"g.bytes[{s[1]}] = {s[2]}"
        if k == "get_bytes":
            return f"{s[2]} = g.bytes[{s[1]}]"
        if k == "set_cell":
            return f"g.rows[{s[1]}].cells[{s[2]}].{s[3]} = {s[4]}"
        if k == "get_cell":
            return f"{s[4]} = g.rows[{s[1]}].cells[{s[2]}].{s[3]}"
        if k == "set_tag":
            return f"g.rows[{s[1]}].tag = {s[2]}"
        if k == "equal":
            a, b = ["g.flat", "h", "w"][s[1]], ["g.flat", "h", "w"][s[2]]
            return f"if arrays_equal({a}, {b}, {s[3]}) {{ v0 = v0 + 1 }}"
        if k == "write":
            return f"write(1, {['g.flat', 'h', 'w'][s[1]]}, {s[2]})"
        if k == "read":
            return f"read(0, {['g.flat', 'h', 'w'][s[1]]}, {s[2]})"
        src, dst = self.places[s[1]], self.places[s[3]]
        return (f"array_copy({src.text}[{s[2]}], {dst.text}[{s[4]}], "
                f"{s[5]})")

    def evaluate(self):
        m = {
            "flat": [0] * 6, "h": [0] * 6, "w": [0] * 6, "bytes": [0] * 8,
            "rows": [{"tag": 0, "cells": [{"a": 0, "b": 0} for _ in range(4)]}
                     for _ in range(3)],
        }
        ix = dict(self.indexes)
        counts = dict(self.counts)
        v = dict(self.values)
        for s in self.statements:
            k = s[0]
            if k == "set_flat":
                self.element(m["flat"], ix[s[1]])
                m["flat"][ix[s[1]]] = s[2]
            elif k == "get_flat":
                self.element(m["flat"], ix[s[1]])
                v[s[2]] = m["flat"][ix[s[1]]]
            elif k == "set_h":
                self.element(m["h"], ix[s[1]])
                m["h"][ix[s[1]]] = v[s[2]]
            elif k == "set_bytes":
                self.element(m["bytes"], ix[s[1]])
                m["bytes"][ix[s[1]]] = s[2]
            elif k == "get_bytes":
                self.element(m["bytes"], ix[s[1]])
                v[s[2]] = m["bytes"][ix[s[1]]]
            elif k in ("set_cell", "get_cell"):
                self.element(m["rows"], ix[s[1]])
                cells = m["rows"][ix[s[1]]]["cells"]
                self.element(cells, ix[s[2]])
                cell = cells[ix[s[2]]]
                if k == "set_cell":
                    cell[s[3]] = s[4]
                else:
                    v[s[4]] = cell[s[3]]
            elif k == "set_tag":
                self.element(m["rows"], ix[s[1]])
                m["rows"][ix[s[1]]]["tag"] = s[2]
            elif k == "equal":
                a = m[["flat", "h", "w"][s[1]]]
                b = m[["flat", "h", "w"][s[2]]]
                n = counts[s[3]]
                if n < 0 or n > len(a) or n > len(b):
                    raise Panic("bounds")
                if a[:n] == b[:n]:
                    v["v0"] += 1
            elif k == "write":
                n = counts[s[2]]
                if n < 0 or n > 6:
                    raise Panic("bounds")
            elif k == "read":
                n = counts[s[2]]
                if n < 0 or n > 6:
                    raise Panic("bounds")
            else:
                self.copy(m, ix, counts, s)
        total = 0
        flat = (m["flat"] + m["h"] + m["w"] + m["bytes"])
        for r in m["rows"]:
            flat.append(r["tag"])
            for c in r["cells"]:
                flat.append(c["a"])
                flat.append(c["b"])
        for i, x in enumerate(flat):
            total += x * (i % 13 + 1)
        return total & 127

    @staticmethod
    def element(array, index):
        if not 0 <= index < len(array):
            raise Panic("bounds")

    def copy(self, m, ix, counts, s):
        src, dst = self.places[s[1]], self.places[s[3]]
        si, di, n = ix[s[2]], ix[s[4]], counts[s[5]]
        for place in (src, dst):
            for name, length in place.path:
                if not 0 <= ix[name] < length:
                    raise Panic("bounds")
        from_array = src.get(m, ix)
        to_array = dst.get(m, ix)
        # a start at the end with a count of zero touches nothing
        in_range = (n >= 0 and 0 <= si and 0 <= di
                    and si + n <= len(from_array)
                    and di + n <= len(to_array))
        same = from_array is to_array
        overlap = same and si < di < si + n
        if not in_range and overlap:
            raise Panic("bounds", "overlap")
        if not in_range:
            raise Panic("bounds")
        if overlap:
            raise Panic("overlap")
        # a record is copied by value, with its nested arrays
        chunk = [self.deep(x) for x in from_array[si:si + n]]
        to_array[di:di + n] = chunk

    @staticmethod
    def deep(x):
        if isinstance(x, dict):
            return {k: ([dict(c) for c in val] if isinstance(val, list)
                        else val) for k, val in x.items()}
        return x


# calls: recursion of noinline functions and array parameters

class Calls:
    name = "calls"

    def __init__(self, rnd):
        self.rnd = rnd
        self.t = rnd.choice(["i8", "i16", "i32"])
        self.targets = targets_of([self.t])
        self.shape = rnd.choice(["tail", "unwind", "helper", "array"])
        self.op = rnd.choice(["+", "-", "*"])
        low, high = limits(self.t)
        self.k = rnd.randint(1, 3)
        self.depth = rnd.randint(0, min(high, 40))
        self.start = interesting(rnd, self.t, True)
        self.index = rnd.randint(-2, 9)
        self.length = rnd.randint(1, 8)

    def source(self):
        t, k, op = self.t, self.k, self.op
        if self.shape == "tail":
            return (f"func noinline rec(n {t}, acc {t}) res {t} {{\n"
                    f"    if n == 0 {{\n        res = acc\n        return\n"
                    f"    }}\n    res = rec(n - 1, acc {op} {k})\n}}\n"
                    f"func main() {{\n    var x = rec({literal(t, self.depth)}"
                    f", {literal(t, self.start)})\n    exit(x & 127)\n}}\n")
        if self.shape == "unwind":
            return (f"func noinline rec(n {t}, acc {t}) res {t} {{\n"
                    f"    if n == 0 {{\n        res = acc\n        return\n"
                    f"    }}\n    res = rec(n - 1, acc) {op} {k}\n}}\n"
                    f"func main() {{\n    var x = rec({literal(t, self.depth)}"
                    f", {literal(t, self.start)})\n    exit(x & 127)\n}}\n")
        if self.shape == "helper":
            return (f"func noinline step(a {t}) res {t} {{\n"
                    f"    res = a {op} {k}\n}}\n"
                    f"func noinline rec(n {t}, acc {t}) res {t} {{\n"
                    f"    if n == 0 {{\n        res = acc\n        return\n"
                    f"    }}\n    res = rec(n - 1, step(acc))\n}}\n"
                    f"func main() {{\n    var x = rec({literal(t, self.depth)}"
                    f", {literal(t, self.start)})\n    exit(x & 127)\n}}\n")
        return (f"func noinline get(a {t}[], i {t}) res {t} {{\n"
                f"    res = a[i]\n}}\n"
                f"func noinline put(a mut {t}[], i {t}, v {t}) {{\n"
                f"    a[i] = v\n}}\n"
                f"func main() {{\n    var arr = {t}[{self.length}]\n"
                f"    var i = {literal(t, self.index)}\n"
                f"    put(arr, i, {literal(t, self.k)})\n"
                f"    var x = get(arr, i)\n    exit(x & 127)\n}}\n")

    def evaluate(self):
        t, k, op = self.t, self.k, self.op
        if self.shape in ("tail", "helper"):
            acc = self.start
            for _ in range(self.depth):
                acc = apply(t, op, acc, k)
            return acc & 127
        if self.shape == "unwind":
            acc = self.start
            for _ in range(self.depth):
                acc = apply(t, op, acc, k)
            return acc & 127
        if not 0 <= self.index < self.length:
            raise Panic("bounds")
        return self.k & 127


# deep: recursion that never ends or ends too deep, caught by the frame check,
# the stack check or the operating system

class Deep:
    name = "deep"

    def __init__(self, rnd):
        self.rnd = rnd
        self.t = "i32"
        self.targets = ["x86_64", "rv32i-fpga"]
        self.pad = rnd.choice([0, 0, 16, 256, 4096])
        self.shape = rnd.choice(["linear", "two", "endless"])

    def source(self):
        pad = ""
        if self.pad:
            pad = f"    var pad = i8[{self.pad}]\n    pad[0] = 1\n"
        if self.shape == "linear":
            return (f"func noinline rec(n i32) res i32 {{\n{pad}"
                    f"    res = rec(n + 1) + 1\n}}\n"
                    f"func main() {{\n    exit(rec(0) & 127)\n}}\n")
        if self.shape == "two":
            return (f"func noinline rec(n i32) res i32 {{\n{pad}"
                    f"    res = rec(n + 1) - rec(n + 2)\n}}\n"
                    f"func main() {{\n    exit(rec(0) & 127)\n}}\n")
        return (f"func noinline rec(n i32) res i32 {{\n{pad}"
                f"    res = n\n    if n >= 0 {{\n        res = rec(n + 1)\n"
                f"    }}\n}}\n"
                f"func main() {{\n    exit(rec(0) & 127)\n}}\n")

    def evaluate(self):
        raise Panic("frame", "stack", "signal")


# indexes: index and count variables of every width with the values around the
# limits of the type and of the array, also computed ('i + k', 'i * k'),
# against arrays of scalars and of a record whose size is not a power of two;
# programs that the compiler rejects are not reported

INDEX_TYPES_SOURCE = "type tri { a i8, b i16, c i32 }\n"
INDEX_SCALARS = ["i8", "i16", "i32", "i64"]


class Indexes:
    name = "indexes"
    lenient = True

    def __init__(self, rnd):
        self.rnd = rnd
        self.benign = rnd.random() < 0.2
        self.kind = rnd.choice(INDEX_SCALARS + ["tri"])
        self.length = rnd.randint(1, 9)
        self.length2 = rnd.randint(1, 9)
        self.index_vars = {}
        for n in ["i0", "i1", "i2"]:
            t = rnd.choice(INDEX_SCALARS)
            self.index_vars[n] = (t, self.index_value(t))
        self.count_vars = {}
        for n in ["n0", "n1"]:
            t = rnd.choice(INDEX_SCALARS)
            self.count_vars[n] = (t, self.index_value(t))
        used = [t for t, _ in self.index_vars.values()]
        used += [t for t, _ in self.count_vars.values()] + [self.kind]
        self.targets = targets_of(used)
        # the sum is as wide as the widest element
        self.sum_type = "i64" if "i64" in used else "i32"
        self.statements = [self.make_statement()
                           for _ in range(rnd.randint(2, 9))]

    def index_value(self, t):
        rnd = self.rnd
        low, high = limits(t)
        if self.benign or rnd.random() < 0.7:
            return rnd.randint(0, 2)
        choices = [rnd.randint(0, 9), rnd.randint(0, 9), -1, 10, 9, low, high]
        for k in range(BITS[t] - 1):
            for d in (-1, 0, 1):
                if fits(t, (1 << k) + d):
                    choices.append((1 << k) + d)
                if fits(t, -(1 << k) + d):
                    choices.append(-(1 << k) + d)
        value = rnd.choice(choices)
        return max(value, low + 1) if t == "i64" else value

    def expression(self, names):
        # ('v', name) or (operator, name, literal or name of the same type)
        rnd = self.rnd
        name = rnd.choice(names)
        form = rnd.choice(["v", "v", "+k", "-k", "*k", "+w"])
        k = rnd.choice([1, 2, 3, 8])
        if form == "+w":
            same = [n for n in names if n != name
                    and self.var_type(n) == self.var_type(name)]
            if same:
                return ("+", name, rnd.choice(same))
            form = "v"
        if form == "v":
            return ("v", name)
        return (form[0], name, k)

    def make_statement(self):
        rnd = self.rnd
        iv, cv = list(self.index_vars), list(self.count_vars)
        kind = rnd.random()
        if kind < 0.25:
            return ("set", self.expression(iv), rnd.randint(-20, 20),
                    rnd.choice("abc"))
        if kind < 0.45:
            return ("get", self.expression(iv), rnd.choice("abc"))
        if kind < 0.7:
            return ("copy", rnd.choice([0, 1]), self.expression(iv),
                    self.expression(iv), self.expression(cv))
        if kind < 0.8 and self.kind != "tri":
            return ("equal", self.expression(cv))
        if kind < 0.9 and self.kind != "tri":
            return ("write", self.expression(cv), self.expression(iv),
                    rnd.random() < 0.5)
        return ("get", self.expression(iv), rnd.choice("abc"))

    # ---- source

    def var_type(self, name):
        return (self.index_vars.get(name) or self.count_vars[name])[0]

    @staticmethod
    def text(e):
        if e[0] == "v":
            return e[1]
        return f"{e[1]} {e[0]} {e[2]}"

    def source(self):
        out = [INDEX_TYPES_SOURCE + "func main() {"]
        out.append(f"    var x = {self.kind}[{self.length}]")
        out.append(f"    var y = {self.kind}[{self.length2}]")
        for n, (vt, v) in {**self.index_vars, **self.count_vars}.items():
            out.append(f"    var {n} = {literal(vt, v)}")
        out.append(f"    var sum = {self.sum_type}(0)")
        for s in self.statements:
            out.append("    " + self.statement_text(s))
        out.append("    exit(sum & 127)")
        out.append("}")
        return "\n".join(out) + "\n"

    def statement_text(self, s):
        k = s[0]
        tri = self.kind == "tri"
        if k == "set":
            f = f".{s[3]}" if tri else ""
            return f"x[{self.text(s[1])}]{f} = {s[2]}"
        if k == "get":
            f = f".{s[2]}" if tri else ""
            return f"sum = sum + x[{self.text(s[1])}]{f}"
        if k == "copy":
            dst = "y" if s[1] == 0 else "x"
            return (f"array_copy(x[{self.text(s[2])}], "
                    f"{dst}[{self.text(s[3])}], {self.text(s[4])})")
        if k == "equal":
            return (f"if arrays_equal(x, y, {self.text(s[1])}) "
                    f"{{ sum = sum + 1 }}")
        start = f", {self.text(s[2])}" if s[3] else ""
        return f"write(1, x, {self.text(s[1])}{start})"

    # ---- model

    def value_of(self, e, env):
        # an expression is evaluated in the type of its variable
        t = self.var_type(e[1])
        a = env[e[1]]
        if e[0] == "v":
            return a
        b = env[e[2]] if isinstance(e[2], str) else e[2]
        if not fits(t, b):
            raise Rejected("literal")
        return apply(t, e[0], a, b)

    def evaluate(self):
        env = {n: v for n, (_, v) in {**self.index_vars,
                                       **self.count_vars}.items()}
        tri = self.kind == "tri"
        make = (lambda: {"a": 0, "b": 0, "c": 0}) if tri else (lambda: 0)
        x = [make() for _ in range(self.length)]
        y = [make() for _ in range(self.length2)]
        total = 0
        for s in self.statements:
            k = s[0]
            try:
                if k in ("set", "get"):
                    i = self.value_of(s[1], env)
                    if not 0 <= i < len(x):
                        raise Panic("bounds")
                    if k == "set":
                        if tri:
                            x[i][s[3]] = s[2]
                        else:
                            x[i] = s[2]
                    else:
                        total += x[i][s[2]] if tri else x[i]
                elif k == "copy":
                    si = self.value_of(s[2], env)
                    di = self.value_of(s[3], env)
                    n = self.value_of(s[4], env)
                    to = x if s[1] == 1 else y
                    # a start at the end with a count of zero touches nothing
                    in_range = (n >= 0 and 0 <= si and 0 <= di
                                and si + n <= len(x) and di + n <= len(to))
                    overlap = to is x and si < di < si + n
                    if not in_range and overlap:
                        raise Panic("bounds", "overlap")
                    if not in_range:
                        raise Panic("bounds")
                    if overlap:
                        raise Panic("overlap")
                    chunk = [dict(e) if tri else e for e in x[si:si + n]]
                    to[di:di + n] = chunk
                elif k == "equal":
                    n = self.value_of(s[1], env)
                    if n < 0 or n > len(x) or n > len(y):
                        raise Panic("bounds")
                    if x[:n] == y[:n]:
                        total += 1
                else:
                    n = self.value_of(s[1], env)
                    if s[3]:
                        start = self.value_of(s[2], env)
                        if start < 0 or n < 0 or start + n > len(x):
                            raise Panic("bounds")
                    elif n < 0 or n > len(x):
                        raise Panic("bounds")
            except Panic as p:
                # the order of the checks of the arguments is not defined
                if p.kinds[0] == "overflow":
                    raise Panic("overflow", "bounds", "overlap")
                raise
        return total & 127


# shifts: value, count and destination of different widths, counts around the
# width of the destination and the limits of the type of the count, a count or
# value wider than the destination is rejected

class Shifts:
    name = "shifts"
    lenient = True

    def __init__(self, rnd):
        self.rnd = rnd
        self.types = {n: rnd.choice(list(BITS)) for n in "abcd"}
        self.values = {}
        for n, t in self.types.items():
            self.values[n] = self.value(t)
        self.targets = targets_of(self.types.values())
        names = list(self.types)
        self.statements = []
        for _ in range(rnd.randint(1, 5)):
            self.statements.append((rnd.choice(names), rnd.choice(names),
                                    rnd.choice(["<<", ">>"]),
                                    rnd.choice(names)))

    def value(self, t):
        rnd = self.rnd
        low, high = limits(t)
        if rnd.random() < 0.6:
            return rnd.randint(0, 40) if rnd.random() < 0.7 else rnd.randint(-40, 40)
        choices = [low + 1, high, -1, 0, 1, 7, 8, 15, 16, 31, 32, 63, 64, 65,
                   127, 255, 256, 1 << 31, 1 << 32, (1 << 32) + 3, -(1 << 32) + 3]
        value = rnd.choice(choices)
        return value if fits(t, value) else rnd.randint(-5, 70)

    def source(self):
        out = ["func main() {"]
        for n, v in self.values.items():
            out.append(f"    var {n} = {literal(self.types[n], v)}")
        for dst, left, op, count in self.statements:
            out.append(f"    {dst} = {left} {op} {count}")
        out.append("    exit(a & 127)")
        out.append("}")
        return "\n".join(out) + "\n"

    def evaluate(self):
        env = dict(self.values)
        for dst, left, op, count in self.statements:
            # the count is narrowed like any operand
            if (BITS[self.types[left]] > BITS[self.types[dst]]
                    or BITS[self.types[count]] > BITS[self.types[dst]]):
                raise Rejected("narrowed")
        for dst, left, op, count in self.statements:
            env[dst] = apply(self.types[dst], op, env[left], env[count])
        return env["a"] & 127


# params: the accesses of 'indexes' made inside functions that take the arrays
# as parameters (a body for each array length), inlined or not, with index and
# count parameters of the type of the argument

class Params(Indexes):
    name = "params"

    def __init__(self, rnd):
        super().__init__(rnd)
        self.noinline = rnd.choice(["", "noinline "])
        self.defs = {}
        self.temps = 0

    def declare(self, key, text):
        self.defs.setdefault(key, text)

    def tname(self, e):
        return self.var_type(e[1])

    def statement_text(self, s):
        k = s[0]
        tri = self.kind == "tri"
        kind = self.kind
        n = self.noinline
        if k == "set":
            t = self.tname(s[1])
            f = f".{s[3]}" if tri else ""
            value = str(s[2]).replace("-", "m")
            name = f"put_{t}_{s[3] if tri else 'v'}_{value}"
            self.declare(name, f"func {n}{name}(a mut {kind}[], i {t}) "
                         f"{{\n    a[i]{f} = {s[2]}\n}}\n")
            return f"{name}(x, {self.text(s[1])})"
        if k == "get":
            t = self.tname(s[1])
            f = f".{s[2]}" if tri else ""
            elem = {"a": "i8", "b": "i16", "c": "i32"}[s[2]] if tri else kind
            name = f"get_{t}_{s[2] if tri else 'v'}"
            self.declare(name, f"func {n}{name}(a {kind}[], i {t}) res {elem} "
                         f"{{\n    res = a[i]{f}\n}}\n")
            self.temps += 1
            return (f"var r{self.temps} = {name}(x, {self.text(s[1])})\n"
                    f"    sum = sum + r{self.temps}")
        if k == "copy":
            ts, td, tn = (self.tname(s[2]), self.tname(s[3]),
                          self.tname(s[4]))
            name = f"cp_{ts}_{td}_{tn}"
            if s[1] == 0:
                self.declare(name + "_xy", f"func {n}{name}_xy(a {kind}[], "
                             f"b mut {kind}[], s {ts}, d {td}, n {tn}) {{\n"
                             f"    array_copy(a[s], b[d], n)\n}}\n")
                return (f"{name}_xy(x, y, {self.text(s[2])}, "
                        f"{self.text(s[3])}, {self.text(s[4])})")
            self.declare(name + "_xx", f"func {n}{name}_xx(a mut {kind}[], "
                         f"s {ts}, d {td}, n {tn}) {{\n"
                         f"    array_copy(a[s], a[d], n)\n}}\n")
            return (f"{name}_xx(x, {self.text(s[2])}, {self.text(s[3])}, "
                    f"{self.text(s[4])})")
        if k == "equal":
            t = self.tname(s[1])
            name = f"eq_{t}"
            self.declare(name, f"func {n}{name}(a {kind}[], b {kind}[], "
                         f"n {t}) res bool {{\n"
                         f"    res = arrays_equal(a, b, n)\n}}\n")
            return f"if {name}(x, y, {self.text(s[1])}) {{ sum = sum + 1 }}"
        t, ti = self.tname(s[1]), self.tname(s[2]) if s[3] else None
        if s[3]:
            name = f"wr_{t}_{ti}"
            self.declare(name, f"func {n}{name}(a {kind}[], n {t}, s {ti}) "
                         f"{{\n    write(1, a, n, s)\n}}\n")
            return f"{name}(x, {self.text(s[1])}, {self.text(s[2])})"
        name = f"wr_{t}"
        self.declare(name, f"func {n}{name}(a {kind}[], n {t}) {{\n"
                     f"    write(1, a, n)\n}}\n")
        return f"{name}(x, {self.text(s[1])})"

    def source(self):
        self.defs = {}
        self.temps = 0
        body = super().source()
        head, rest = body.split("func main() {", 1)
        return head + "".join(self.defs.values()) + "func main() {" + rest


# contexts: one expression of one type, with + - * that can overflow, used as a
# declaration, an argument, an index, a count, a divisor, a shift count, a
# comparison, a truth test or the result of a function

class Contexts:
    name = "contexts"

    def __init__(self, rnd):
        self.rnd = rnd
        self.t = rnd.choice(list(BITS))
        self.targets = targets_of([self.t])
        benign = rnd.random() < 0.4
        self.values = {f"v{i}": interesting(rnd, self.t, benign)
                       for i in range(3)}
        terms = []
        ops = []
        for n in range(rnd.randint(2, 4)):
            # the first element decides the width, a constant is of the
            # default type
            if n == 0 or rnd.random() < 0.75:
                terms.append(rnd.choice(list(self.values)))
            else:
                terms.append(rnd.randint(0, 9))
            ops.append(rnd.choice("+-*"))
        self.terms = terms
        self.ops = ops[:-1]
        self.context = rnd.choice(
            ["decl", "arg", "arg_noinline", "index", "copy", "write", "div",
             "mod", "shift", "cmp", "cmp_rhs", "truth", "result", "result_noinline",
             "array_arg"])
        self.length = 8

    def expression(self):
        text = str(self.terms[0])
        for o, term in zip(self.ops, self.terms[1:]):
            text += f" {o} {term}"
        return text

    def value(self):
        vals = [self.values[x] if isinstance(x, str) else x
                for x in self.terms]
        for v in vals:
            if not fits(self.t, v):
                raise Rejected("literal")
        return evaluate_chain(self.t, vals, self.ops)

    def exact_value(self):
        vals = [self.values[x] if isinstance(x, str) else x
                for x in self.terms]
        seen = list(vals)
        items = [vals[0]]
        pending = []
        for o, v in zip(self.ops, vals[1:]):
            if o == "*":
                items[-1] *= v
                seen.append(items[-1])
            else:
                pending.append(o)
                items.append(v)
        result = items[0]
        for o, v in zip(pending, items[1:]):
            result = result + v if o == "+" else result - v
            seen.append(result)
        self.intermediate_outside_int32 = any(
            not fits("i32", x) for x in seen)
        return result

    def source(self):
        t, e, c = self.t, self.expression(), self.context
        out = []
        if c == "arg" or c == "arg_noinline":
            n = "noinline " if c == "arg_noinline" else ""
            out.append(f"func {n}f(a {t}) res {t} {{\n    res = a\n}}")
        if c == "result" or c == "result_noinline":
            n = "noinline " if c == "result_noinline" else ""
            sig = ", ".join(f"{k} {t}" for k in self.values)
            out.append(f"func {n}f({sig}) res {t} {{\n    res = {e}\n}}")
        if c == "array_arg":
            out.append(f"func noinline f(a {t}[], i {t}) res {t} {{\n"
                       f"    res = a[i]\n}}")
        out.append("func main() {")
        for k, v in self.values.items():
            out.append(f"    var {k} = {literal(t, v)}")
        out.append(f"    var arr = {t}[{self.length}]")
        out.append(f"    var dst = {t}[{self.length}]")
        out.append("    arr[1] = 3")
        wrapped = f"({e})"
        if c == "decl":
            out.append(f"    var q = {t}(0)")
            out.append(f"    q = {e}")
            out.append("    exit(q & 127)")
        elif c in ("arg", "arg_noinline"):
            out.append(f"    var q = f({e})")
            out.append("    exit(q & 127)")
        elif c in ("result", "result_noinline"):
            out.append(f"    var q = f({', '.join(self.values)})")
            out.append("    exit(q & 127)")
        elif c == "array_arg":
            out.append(f"    var q = f(arr, {e})")
            out.append("    exit(q & 127)")
        elif c == "index":
            out.append(f"    arr[{e}] = 5")
            out.append("    exit(arr[1] + 1)")
        elif c == "copy":
            out.append(f"    array_copy(arr, dst, {e})")
            out.append("    exit(dst[1] + 1)")
        elif c == "write":
            out.append(f"    write(1, arr, {e})")
            out.append("    exit(7)")
        elif c in ("div", "mod"):
            o = "/" if c == "div" else "%"
            out.append(f"    var n = {t}(100)")
            out.append(f"    var q = {t}(0)")
            out.append(f"    q = n {o} {wrapped}")
            out.append("    exit(q & 127)")
        elif c == "shift":
            out.append(f"    var s = {t}(1)")
            out.append(f"    s = s << {wrapped}")
            out.append("    exit(s & 127)")
        elif c == "cmp":
            out.append(f"    if {e} > 3 {{ exit(11) }}")
            out.append("    exit(12)")
        elif c == "cmp_rhs":
            out.append(f"    if 3 > {e} {{ exit(11) }}")
            out.append("    exit(12)")
        else:
            out.append(f"    if {e} {{ exit(11) }}")
            out.append("    exit(12)")
        out.append("}")
        return "\n".join(out) + "\n"

    def evaluate(self):
        t, c = self.t, self.context
        if c in ("index", "copy", "write"):
            # an index or a count is computed in the default integer type of
            # the target, the final value is checked against the range
            v = self.exact_value()
            if not 0 <= v <= self.length:
                raise Panic("overflow", "bounds")
            if self.intermediate_outside_int32:
                raise Skip("differs between the targets")
        else:
            v = self.value()
        if c in ("decl", "arg", "arg_noinline", "result", "result_noinline"):
            return v & 127
        if c == "array_arg":
            if not 0 <= v < self.length:
                raise Panic("bounds")
            return (3 if v == 1 else 0) & 127
        if c == "index":
            if not 0 <= v < self.length:
                raise Panic("bounds")
            return (5 if v == 1 else 3) + 1
        if c == "copy":
            if not 0 <= v <= self.length:
                raise Panic("bounds")
            return (3 if v >= 2 else 0) + 1
        if c == "write":
            if not 0 <= v <= self.length:
                raise Panic("bounds")
            return 7
        if c in ("div", "mod"):
            return apply(t, "/" if c == "div" else "%", 100, v) & 127 \
                if fits(t, 100) else self.reject()
        if c == "shift":
            return apply(t, "<<", 1, v) & 127
        if c == "cmp":
            return 11 if v > 3 else 12
        if c == "cmp_rhs":
            return 11 if 3 > v else 12
        return 11 if v != 0 else 12

    @staticmethod
    def reject():
        raise Rejected("literal")


SCENARIOS = [Arith, Widen, Arrays, Calls, Deep, Indexes, Shifts, Params, Contexts]


def run(command, **kw):
    try:
        return subprocess.run(command, capture_output=True, text=True,
                              errors="replace", stdin=subprocess.DEVNULL, **kw)
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(command, TIMEOUT, "", "timeout")


def compile_run(work, source, target):
    path = os.path.join(work, "f.baz")
    with open(path, "w") as f:
        f.write(source)
    if target == "x86_64":
        c = run([COMPILER, "--target=x86_64", "--checks=" + CHECKS, path])
        if c.returncode:
            return ("compile", c.stderr)
        with open(os.path.join(work, "f.s"), "w") as f:
            f.write(c.stdout)
        a = run(["nasm", "-f", "elf64", os.path.join(work, "f.s"), "-o",
                 os.path.join(work, "f.o")])
        a2 = run(["ld", "-s", "-T", os.path.join(ROOT, "baz.ld"), os.path.join(work, "f.o"), "-o",
                  os.path.join(work, "f.x")])
        if a.returncode or a2.returncode:
            return ("assemble", a.stderr + a2.stderr)
        r = run([os.path.join(work, "f.x")], timeout=20)
        if r.returncode == TIMEOUT:
            return ("timeout", "")
        if r.returncode < 0 or r.returncode == SIGSEGV:
            return ("panic", "signal")
        if r.returncode == 255:
            for kind, message in X86_PANIC.items():
                if message in r.stderr:
                    return ("panic", kind)
            # the bounds failure prints nothing without '--checks=line'
            if not r.stderr:
                return ("panic", "bounds")

            return ("panic", "unknown:" + r.stderr.strip())
        return ("exit", r.returncode)
    image = os.path.join(work, "f.bin")
    c = run([COMPILER, "--target=rv32i-fpga", "--checks=" + CHECKS,
             "--bin=" + image, path])
    if c.returncode:
        return ("compile", c.stderr)
    r = run([EMULATOR, image, "/dev/null"], timeout=20)
    if r.returncode == TIMEOUT:
        return ("timeout", "")
    for kind, code in FPGA_PANIC.items():
        if r.returncode == code:
            return ("panic", kind)
    return ("exit", r.returncode)


def matches(expected, got):
    # the compiler may fold the call chain and reject the overflow at compile
    # time, that is an error exit too
    if (expected[0] == "panic" and "overflow" in expected[1]
            and got[0] == "compile" and "overflows the type" in got[1]):
        return True
    if (expected[0] == "panic" and "shift" in expected[1]
            and got[0] == "compile" and "shift count must be" in got[1]):
        return True
    if expected[0] == "panic":
        return got[0] == "panic" and got[1] in expected[1]
    if expected[0] == "compile":
        return got[0] == "compile"
    return got == expected


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(1 << 30)
    only = sys.argv[3] if len(sys.argv) > 3 else None
    scenarios = [s for s in SCENARIOS if only in (None, s.name)]
    print(f"seed {seed}, {count} programs")
    failures = 0
    tally = {}
    with tempfile.TemporaryDirectory() as work:
        for n in range(count):
            rnd = random.Random(seed * 100003 + n)
            program = scenarios[n % len(scenarios)](rnd)
            try:
                expected = ("exit", program.evaluate())
            except Panic as p:
                expected = ("panic", p.kinds)
            except Rejected as r:
                expected = ("compile", str(r))
            except Skip:
                continue
            source = program.source()
            counts = tally.setdefault(program.name, [0, 0, 0])
            counts[0 if expected[0] == "exit" else
                   1 if expected[0] == "panic" else 2] += 1
            for target in program.targets:
                got = compile_run(work, source, target)
                if getattr(program, "lenient", False) and got[0] == "compile":
                    counts[2] += 1
                    continue
                if not matches(expected, got):
                    failures += 1
                    keep = os.path.join(tempfile.gettempdir(),
                                        f"fuzz-ub-{seed}-{n}.baz")
                    with open(keep, "w") as f:
                        f.write(source)
                    print(f"MISMATCH {program.name} {target}: expected "
                          f"{expected}, got {got[0]} {str(got[1])[:200]}; "
                          f"program in {keep}")
    for name, (ran, panicked, rejected) in tally.items():
        print(f"{name}: {ran} ran to the end, {panicked} modeled panics, "
              f"{rejected} rejected")
    print(f"{count} programs, {failures} mismatches")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
