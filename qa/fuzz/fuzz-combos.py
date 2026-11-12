#!/usr/bin/env python3
# model based fuzzer of features that the other fuzzers keep apart, combined
# in one program: methods ('mut' and not) on elements of arrays of records
# reached through run-time indexes, also inside 'foo' over those arrays and
# through 'e'; a generic type with a method that sees its constant and a
# generic function with an array parameter of each element type and length
# ('noinline' or inlined), and one over an array of records that calls
# the methods of 'e'; an inlined recursion ended by a constant argument
# and a 'noinline' recursion over an array parameter of two lengths, with an
# early 'return' and 'exit'; 'loop' and 'foo' with 'break' and 'continue';
# 'dat' globals written by called functions and read in arithmetic, in
# indexes of destinations and of operands and in the receivers of methods
# (left to right); 'read' and 'write' with run-time counts and start indexes
# into fields of elements; 'sector_read' and 'sector_write' of an sd card
# image into array fields of elements, with and without a start index;
# conversions of narrow types inside all of these
#
# python runs the same program (an interpreter of its tree): the output and
# the exit code, or the first panic, or the 'sd card' stop of a sector past
# the end of the card; on rv32i-qemu, which has no card, the stop at the
# first sector transfer. the program uses only the types 'i8', 'i16' and
# 'i32' in its arithmetic, so the model is the same on every target. it is
# built with every function as drawn, with '--checks=noub' on the four
# targets, and on x86_64 and rv32i-fpga with '--nopt', '--versions=nested',
# 'first' and 'last', '--checks=noub,line' and a subset of the checks that
# still holds the check of the panic the model ends with (none when it ends
# without one); the variants with every function that can be 'noinline'
# and with every function inlined that can be are built with
# '--checks=noub' on x86_64 and rv32i-fpga
#
#   combos-mismatch  a build ends differently from the model
#   combos-reject    a generated program is rejected; not the registers
#                    of x86_64 that an instruction fixes or that run out
#                    (README chapter 8, item 3 of the target)
#   combos-generator the model failed, a defect of this script
#   compiler-crash, compiler-hang, invalid-asm, link-failure, image-mismatch
#                    see fuzz-compiler.py
#
# usage: fuzz-combos.py [count] [seed] [jobs]
#        fuzz-combos.py --one SEED INDEX [mixed|noinline|inline]
# '--one' prints the program 'INDEX' of 'SEED', its input and the model's
# outcome
import os
import random
import shutil
import sys
import tempfile

import fuzzlib as fl

INTS = ("i8", "i16", "i32")
LIMIT = {"i8": 20, "i16": 300, "i32": 5000}
# sectors of the card image, a sector at or past it stops with 'sd card'
SECTORS = 4
SECTOR_BYTES = 512
TAG = 4
LETTERS = "abcdefghijklmnopqrstuvwxyz"


class Panic(Exception):
    def __init__(self, kind):
        super().__init__(kind)
        self.kind = kind


class Exit(Exception):
    def __init__(self, code):
        super().__init__(code)
        self.code = code


class SdStop(Exception):
    pass


class Return(Exception):
    pass


class Break(Exception):
    pass


class Continue(Exception):
    pass


class TooLong(Exception):
    pass


def card_image():
    # small values in the 'i32' view so that sums of a sector do not overflow
    data = bytearray()
    for s in range(SECTORS):
        for j in range(SECTOR_BYTES):
            data.append(0 if j % 4 >= 2 else (s * 7 + j * 13 + 1) & 0x7f)
    return data


def element_bytes(t):
    return fl.BITS[t] // 8


def to_bytes(values, t, start, count):
    size = element_bytes(t)
    out = bytearray()
    for v in values[start:start + count]:
        out += (v & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
    return out


def from_bytes(values, t, start, data):
    # writes 'data' into 'values' from element 'start', a partial last
    # element keeps its other bytes
    size = element_bytes(t)
    for k in range(0, len(data), size):
        index = start + k // size
        old = (values[index] & ((1 << (8 * size)) - 1)).to_bytes(size,
                                                               "little")
        chunk = data[k:k + size] + old[len(data[k:k + size]):]
        values[index] = fl.wrap(t, int.from_bytes(chunk, "little"))


class State:
    def __init__(self, stdin_line, has_card):
        self.out = bytearray()
        self.card = card_image()
        self.has_card = has_card
        self.pending = stdin_line
        self.steps = 0
        self.globals = {}

    def tick(self):
        self.steps += 1
        if self.steps > 200000:
            raise TooLong()


def check(t, value):
    if t != "int" and not fl.fits(t, value):
        raise Panic("overflow")
    return value


def bounds(index, length):
    if index < 0 or index >= length:
        raise Panic("bounds")


# places: 'ref' evaluates the indexes left to right and gives (container, key)


class Var:
    def __init__(self, name, t, root=None, readonly=False):
        self.name = name
        self.t = t
        self.root = root or name
        self.readonly = readonly

    def text(self):
        return self.name

    def ref(self, st, env):
        return env[self.name]

    def reads(self):
        return {self.root}


class Glob:
    def __init__(self, name, t):
        self.name = name
        self.t = t
        self.root = name

    def text(self):
        return self.name

    def ref(self, st, env):
        return (st.globals, self.name)

    def reads(self):
        return {self.name}


class Elem:
    def __init__(self, arr, index):
        self.arr = arr
        self.index = index
        self.t = arr.t[1]
        self.root = arr.root

    def text(self):
        return f"{self.arr.text()}[{self.index.text()}]"

    def ref(self, st, env):
        container, key = self.arr.ref(st, env)
        values = container[key]
        i = self.index.value(st, env)
        bounds(i, len(values))
        return (values, i)

    def reads(self):
        return self.arr.reads() | self.index.reads()


class Field:
    def __init__(self, rec, name, t):
        self.rec = rec
        self.name = name
        self.t = t
        self.root = rec.root

    def text(self):
        return f"{self.rec.text()}.{self.name}"

    def ref(self, st, env):
        container, key = self.rec.ref(st, env)
        return (container[key], self.name)

    def reads(self):
        return self.rec.reads()


# expressions


class Lit:
    def __init__(self, t, v):
        self.t = t
        self.v = v

    def text(self):
        return str(self.v)

    def value(self, st, env):
        return self.v

    def reads(self):
        return set()

    def effects(self):
        return False


class Get:
    def __init__(self, place):
        self.place = place
        self.t = place.t

    def text(self):
        return self.place.text()

    def value(self, st, env):
        container, key = self.place.ref(st, env)
        return container[key]

    def reads(self):
        return self.place.reads()

    def effects(self):
        return False


class Int:
    # a value of the default type that stays small: 'i' of a 'foo', 'n' of
    # an inlined recursion
    def __init__(self, name):
        self.name = name
        self.t = "int"

    def text(self):
        return self.name

    def value(self, st, env):
        container, key = env[self.name]
        return container[key]

    def reads(self):
        return set()

    def effects(self):
        return False


class Conv:
    # 'T(x)' wraps, also the '+', '-', '*', '&', '|' and '^' inside it
    def __init__(self, t, inner):
        self.t = t
        self.inner = inner

    def text(self):
        return f"{self.t}({self.inner.text()})"

    def value(self, st, env):
        if isinstance(self.inner, Bin):
            v = self.inner.value(st, env, wrapping=True)
        else:
            v = self.inner.value(st, env)
        return fl.wrap(self.t, v)

    def reads(self):
        return self.inner.reads()

    def effects(self):
        return self.inner.effects()


class Bin:
    def __init__(self, op, a, b):
        self.op = op
        self.a = a
        self.b = b
        self.t = a.t

    def text(self):
        return f"{fl.group(self.a.text())} {self.op} {fl.group(self.b.text())}"

    def value(self, st, env, wrapping=False):
        x = self.a.value(st, env, True) if wrapping and \
            isinstance(self.a, Bin) else self.a.value(st, env)
        y = self.b.value(st, env, True) if wrapping and \
            isinstance(self.b, Bin) else self.b.value(st, env)
        op = self.op
        t = self.t
        if op == "+":
            r = x + y
        elif op == "-":
            r = x - y
        elif op == "*":
            r = x * y
        elif op == "&":
            return x & y
        elif op == "|":
            return x | y
        elif op == "^":
            return x ^ y
        elif op in ("/", "%"):
            if y == 0 or (y == -1 and x == fl.limits(t)[0]):
                raise Panic("division")
            q = fl.trunc_div(x, y)
            return q if op == "/" else x - q * y
        else:
            if y < 0 or y >= fl.BITS[t]:
                raise Panic("shift")
            return fl.wrap(t, x << y) if op == "<<" else x >> y
        return r if wrapping else check(t, r)

    def reads(self):
        return self.a.reads() | self.b.reads()

    def effects(self):
        return self.a.effects() or self.b.effects()


class Index:
    # an index of the default type: 'x & mask', 'x % m', a constant, 'i' of
    # a 'foo' plus a constant and a mask, or a call
    def __init__(self, kind, x=None, m=0):
        self.kind = kind
        self.x = x
        self.m = m

    def text(self):
        if self.kind == "const":
            return str(self.m)
        if self.kind == "plain":
            return self.x.text()
        if self.kind == "mask":
            return f"{fl.group(self.x.text())} & {self.m}"
        if self.kind == "mod":
            return f"{fl.group(self.x.text())} % {self.m}"
        # 'foo' index plus a constant
        return f"({self.x.text()} + {self.m[0]}) & {self.m[1]}"

    def value(self, st, env):
        if self.kind == "const":
            return self.m
        v = self.x.value(st, env)
        if self.kind == "plain":
            return v
        if self.kind == "mask":
            return v & self.m
        if self.kind == "mod":
            m = env["N"][0][0] if self.m == "N" else self.m
            return v - m * fl.trunc_div(v, m)
        return (v + self.m[0]) & self.m[1]

    def reads(self):
        return self.x.reads() if self.x is not None else set()

    def effects(self):
        return self.x.effects() if self.x is not None else False


class Call:
    # a call of a function with a result, or a method on 'recv'
    def __init__(self, fn, args, recv=None, targs=""):
        self.fn = fn
        self.args = args
        self.recv = recv
        self.t = fn.result
        self.targs = targs

    def text(self):
        args = ", ".join(a.text() for a in self.args)
        if self.recv is not None:
            return f"{self.recv.text()}.{self.fn.short}({args})"
        return f"{self.fn.short}{self.targs}({args})"

    def value(self, st, env):
        return self.fn.invoke(st, env, self.recv, self.args)

    def reads(self):
        r = set(self.fn.uses)
        if self.recv is not None:
            r |= self.recv.reads()
        for a in self.args:
            r |= a.reads()
        return r

    def written(self):
        # the globals and the places the call writes
        w = set(self.fn.writes)
        if self.recv is not None and self.fn.mut_self:
            w |= {self.recv.root}
        for (name, kind, t), a in zip(self.fn.params, self.args):
            if kind == "mut" or (kind == "arr" and t[2]):
                w |= {a.place.root}
        return w

    def effects(self):
        return bool(self.fn.writes) or \
            any(a.effects() for a in self.args)


class ArrArg:
    # an array or record passed in place
    def __init__(self, place):
        self.place = place
        self.t = place.t

    def text(self):
        return self.place.text()

    def reads(self):
        return self.place.reads()

    def effects(self):
        return False


class Cond:
    def __init__(self, a, op, b):
        self.a = a
        self.op = op
        self.b = b

    def text(self):
        return f"{self.a.text()} {self.op} {self.b.text()}"

    def value(self, st, env):
        x = self.a.value(st, env)
        y = self.b.value(st, env)
        return {"==": x == y, "!=": x != y, "<": x < y, "<=": x <= y,
                ">": x > y, ">=": x >= y}[self.op]


class Logic:
    def __init__(self, op, a, b):
        self.op = op
        self.a = a
        self.b = b

    def text(self):
        return f"({self.a.text()}) {self.op} ({self.b.text()})"

    def value(self, st, env):
        x = self.a.value(st, env)
        if self.op == "and":
            return x and self.b.value(st, env)
        return x or self.b.value(st, env)


# statements


def run_block(st, env, body):
    for s in body:
        st.tick()
        s.run(st, env)


def block_text(body, indent):
    lines = []
    for s in body:
        lines += s.lines(indent)
    return lines


class Assign:
    def __init__(self, place, expr):
        self.place = place
        self.expr = expr

    def lines(self, indent):
        return ["    " * indent +
                f"{self.place.text()} = {self.expr.text()}"]

    def run(self, st, env):
        # note: the indexes of the destination before the value (test 1660)
        container, key = self.place.ref(st, env)
        v = self.expr.value(st, env)
        container[key] = v


class Decl:
    def __init__(self, name, expr):
        self.name = name
        self.expr = expr

    def lines(self, indent):
        return ["    " * indent + f"var {self.name} = {self.expr.text()}"]

    def run(self, st, env):
        env[self.name] = ([self.expr.value(st, env)], 0)


class DeclArray:
    def __init__(self, name, type_text, make):
        self.name = name
        self.type_text = type_text
        self.make = make

    def lines(self, indent):
        return ["    " * indent + f"var {self.name} = {self.type_text}"]

    def run(self, st, env):
        env[self.name] = ([self.make()], 0)


class Do:
    # a call as a statement
    def __init__(self, call):
        self.call = call

    def lines(self, indent):
        return ["    " * indent + self.call.text()]

    def run(self, st, env):
        self.call.value(st, env)


class If:
    def __init__(self, cond, then, other=None):
        self.cond = cond
        self.then = then
        self.other = other

    def lines(self, indent):
        pad = "    " * indent
        if len(self.then) == 1 and self.other is None and \
                isinstance(self.then[0], (Jump, ExitStmt)):
            return [pad + f"if {self.cond.text()} " +
                    self.then[0].lines(0)[0]]
        out = [pad + f"if {self.cond.text()} {{"]
        out += block_text(self.then, indent + 1)
        if self.other:
            out.append(pad + "} else {")
            out += block_text(self.other, indent + 1)
        out.append(pad + "}")
        return out

    def run(self, st, env):
        if self.cond.value(st, env):
            run_block(st, env, self.then)
        elif self.other:
            run_block(st, env, self.other)


class Jump:
    def __init__(self, word):
        self.word = word

    def lines(self, indent):
        return ["    " * indent + self.word]

    def run(self, st, env):
        raise {"break": Break, "continue": Continue,
               "return": Return}[self.word]()


class ExitStmt:
    def __init__(self, expr):
        self.expr = expr

    def lines(self, indent):
        return ["    " * indent + f"exit({fl.group(self.expr.text())} & 127)"]

    def run(self, st, env):
        raise Exit(self.expr.value(st, env) & 127)


class Loop:
    # 'loop' with a counter that ends it
    def __init__(self, counter, limit, body):
        self.counter = counter
        self.limit = limit
        self.body = body

    def lines(self, indent):
        pad = "    " * indent
        out = [pad + f"var {self.counter} = i8(0)", pad + "loop {",
               pad + f"    {self.counter} = {self.counter} + 1",
               pad + f"    if {self.counter} > {self.limit} break"]
        out += block_text(self.body, indent + 1)
        out.append(pad + "}")
        return out

    def run(self, st, env):
        cell = [0]
        env[self.counter] = (cell, 0)
        while True:
            st.tick()
            cell[0] += 1
            if cell[0] > self.limit:
                break
            try:
                run_block(st, env, self.body)
            except Break:
                break
            except Continue:
                continue


class Foo:
    def __init__(self, arr, count, body):
        self.arr = arr
        self.count = count
        self.body = body

    def lines(self, indent):
        pad = "    " * indent
        count = f", {self.count.text()}" if self.count is not None else ""
        out = [pad + f"foo {self.arr.text()}{count} {{"]
        out += block_text(self.body, indent + 1)
        out.append(pad + "}")
        return out

    def run(self, st, env):
        container, key = self.arr.ref(st, env)
        values = container[key]
        n = len(values)
        if self.count is not None:
            n = self.count.value(st, env)
            if n < 0 or n > len(values):
                raise Panic("bounds")
        saved = (env.get("e"), env.get("i"))
        for k in range(n):
            st.tick()
            env["e"] = (values, k)
            env["i"] = ([k], 0)
            try:
                run_block(st, env, self.body)
            except Break:
                break
            except Continue:
                continue
        env["e"], env["i"] = saved


class Io:
    # 'read', 'write', 'sector_read' and 'sector_write'
    def __init__(self, kind, arr, args, dest=None):
        self.kind = kind
        self.arr = arr
        self.args = args
        self.dest = dest

    def lines(self, indent):
        pad = "    " * indent
        if self.kind in ("read", "write"):
            fd = 0 if self.kind == "read" else 1
            parts = [str(fd), self.arr.text()] + [a.text() for a in self.args]
        else:
            parts = [self.args[0].text(), self.arr.text()] + \
                [a.text() for a in self.args[1:]]
        call = f"{self.kind}({', '.join(parts)})"
        if self.dest:
            return [pad + f"var {self.dest} = {call}"]
        return [pad + call]

    def run(self, st, env):
        if self.kind in ("sector_read", "sector_write"):
            sector = self.args[0].value(st, env)
            start = self.args[1].value(st, env) if len(self.args) > 1 else 0
            container, key = self.arr.ref(st, env)
            values = container[key]
            t = self.arr.t[1]
            count = SECTOR_BYTES // element_bytes(t)
            if start < 0 or start + count > len(values):
                raise Panic("bounds")
            if not st.has_card or not 0 <= sector < SECTORS:
                raise SdStop()
            at = sector * SECTOR_BYTES
            if self.kind == "sector_read":
                from_bytes(values, t, start, st.card[at:at + SECTOR_BYTES])
            else:
                st.card[at:at + SECTOR_BYTES] = to_bytes(values, t, start,
                                                         count)
            return
        container, key = self.arr.ref(st, env)
        values = container[key]
        t = self.arr.t[1]
        count = self.args[0].value(st, env) if self.args else len(values)
        start = self.args[1].value(st, env) if len(self.args) > 1 else 0
        if count < 0 or start < 0 or start + count > len(values):
            raise Panic("bounds")
        if self.kind == "write":
            st.out += to_bytes(values, t, start, count)
            got = count * element_bytes(t)
        else:
            line = st.pending
            st.pending = b""
            # note: the line fits in the count, see 'stdin_line'
            assert len(line) <= count * element_bytes(t)
            from_bytes(values, t, start, line)
            got = len(line)
        if self.dest:
            env[self.dest] = ([got], 0)


# functions


class Function:
    def __init__(self, short, params, result, body=None, owner=None,
                 mut_self=False, tparams="", noinline=False, can_inline=True,
                 can_noinline=True):
        self.short = short
        # (name, kind, type): kind 'val', 'mut', 'arr' (in place), 'int'
        self.params = params
        self.result = result
        self.body = body or []
        self.owner = owner
        self.mut_self = mut_self
        self.tparams = tparams
        self.noinline = noinline
        self.can_inline = can_inline
        self.can_noinline = can_noinline
        # the globals the function and its callees read and write
        self.uses = set()
        self.writes = set()
        # the generic instance a call runs: 'cell' constants
        self.constants = {}

    def invoke(self, st, env, recv, args):
        callee = {}
        if recv is not None:
            callee["self"] = recv.ref(st, env)
        for (name, kind, t), a in zip(self.params, args):
            if kind in ("arr", "mut"):
                callee[name] = a.place.ref(st, env)
            else:
                callee[name] = ([a.value(st, env)], 0)
        res = [0]
        callee["res"] = (res, 0)
        if recv is not None and recv.t[0] == "cell":
            callee["N"] = ([recv.t[1]], 0)
        try:
            run_block(st, callee, self.body)
        except Return:
            pass
        return res[0]

    def text(self, noinline):
        word = "noinline " if noinline else ""
        params = []
        for name, kind, t in self.params:
            if kind == "int":
                params.append(name)
            elif kind == "arr":
                mut = " mut" if t[2] else ""
                params.append(f"{name}{mut} {t[3]}")
            elif kind == "mut":
                params.append(f"{name} mut {type_name(t)}")
            else:
                params.append(f"{name} {t}")
        res = f" res {self.result}" if self.result else ""
        if self.owner:
            mut = "mut " if self.mut_self else ""
            head = f"func {word}{mut}{self.owner}.{self.short}"
        else:
            head = f"func {word}{self.short}{self.tparams}"
        out = [f"{head}({', '.join(params)}){res} {{"]
        if self.result:
            out.append("    res = 0")
        out += block_text(self.body, 1)
        out.append("}")
        return out


def type_name(t):
    if t == "rec":
        return "rec"
    return str(t)


def copy_value(v):
    if isinstance(v, dict):
        return {k: copy_value(x) for k, x in v.items()}
    if isinstance(v, list):
        return [copy_value(x) for x in v]
    return v


class Ctx:
    # what a block can see: scalar places by type, record places, arrays of
    # records, 'i8' arrays and the functions it may call
    def __init__(self, gen, fn=None):
        self.gen = gen
        self.fn = fn
        self.scalars = {t: [] for t in INTS}
        self.recs = []
        self.rec_arrays = []
        self.byte_arrays = []
        self.sector_arrays = []
        self.ints = []
        self.loop_depth = 0
        self.in_loop = False
        self.calls = []
        self.allow_globals = True
        # the 'ints' are constants in each inlined level, so an argument
        # made of them alone folds and can be a zero divisor in the callee
        self.constant_ints = False

    def copy(self):
        c = Ctx(self.gen, self.fn)
        c.scalars = {t: list(v) for t, v in self.scalars.items()}
        c.recs = list(self.recs)
        c.rec_arrays = list(self.rec_arrays)
        c.byte_arrays = list(self.byte_arrays)
        c.sector_arrays = list(self.sector_arrays)
        c.ints = list(self.ints)
        c.loop_depth = self.loop_depth
        c.in_loop = self.in_loop
        c.calls = self.calls
        c.allow_globals = self.allow_globals
        c.constant_ints = self.constant_ints
        return c


class Gen:
    def __init__(self, rnd):
        self.rnd = rnd
        self.names = 0
        self.functions = []
        self.head = []

    def name(self, prefix):
        self.names += 1
        return f"{prefix}{self.names}"

    def lit(self, t, low=None, high=None):
        rnd = self.rnd
        bound = LIMIT[t]
        if low is None:
            v = rnd.choice([rnd.randint(-bound, bound), rnd.randint(0, 9),
                            rnd.randint(1, 3)])
        else:
            v = rnd.randint(low, high)
        return Lit(t, v)

    # places

    def index(self, ctx, length, effects, simple=False):
        # 'simple': the operand of the index has only constant indexes
        rnd = self.rnd
        mask = length - 1 if length & (length - 1) == 0 else 3
        r = rnd.random()
        if r < 0.15:
            return Index("const", m=rnd.randrange(length))
        if r < 0.3 and ctx.ints:
            name = rnd.choice(ctx.ints)
            if name == "i":
                return Index("foo", Int("i"), (rnd.randint(0, 3), mask))
            return Index("mask", Int(name), mask)
        if r < 0.4 and effects and self.bump and ctx.allow_globals:
            return Index("mask", Call(self.bump, []), mask)
        if simple:
            return Index("const", m=rnd.randrange(length))
        t = rnd.choice(INTS)
        place = self.scalar_place(ctx, t, simple=True)
        if place is None:
            return Index("const", m=rnd.randrange(length))
        x = Get(place)
        # note: a mask of a length that is not a power of two can pass it
        if rnd.random() < 0.03:
            return Index("mod", x, length + rnd.randint(0, 1))
        return Index("mask", x, mask)

    def fresh(self, ctx, arr):
        # an array field of an element at a run-time index instead of the
        # constant one
        if isinstance(arr, Field) and isinstance(arr.rec, Elem) and \
                arr.rec.index.kind == "const" and arr.rec.arr.t[2] and \
                self.rnd.random() < 0.6:
            owner = arr.rec.arr
            index = self.index(ctx, owner.t[2], False)
            return Field(Elem(owner, index), arr.name, arr.t)
        return arr

    def rec_place(self, ctx, effects=False, simple=False):
        rnd = self.rnd
        choices = list(ctx.recs)
        if ctx.rec_arrays:
            choices += ["elem"] * 3
        if not choices:
            return None
        c = rnd.choice(choices)
        if c == "elem":
            arr, length = rnd.choice(ctx.rec_arrays)
            return Elem(arr, self.index(ctx, length, effects, simple))
        return c

    def scalar_place(self, ctx, t, effects=False, simple=False, write=False):
        rnd = self.rnd
        options = [v for v in ctx.scalars[t]
                   if not (write and getattr(v, "readonly", False))]
        if ctx.recs or ctx.rec_arrays:
            options += ["rec"] * 3
        if not options:
            return None
        c = rnd.choice(options)
        if c != "rec":
            return c if not callable(c) else c(ctx, effects)
        rec = self.rec_place(ctx, effects, simple)
        if t == "i8" and rnd.random() < 0.4:
            return Elem(Field(rec, "tag", ("arr", "i8", TAG)),
                        self.index(ctx, TAG, False, True))
        name = {"i8": "a", "i16": "b", "i32": "c"}[t]
        return Field(rec, name, t)

    # expressions

    def operand(self, ctx, t, depth, effects=True):
        # a value of type 't' that is not a literal
        rnd = self.rnd
        r = rnd.random()
        if r < 0.15:
            other = rnd.choice([x for x in INTS if x != t] + ["int"]
                               if ctx.ints else
                               [x for x in INTS if x != t])
            if other == "int":
                return Conv(t, Int(rnd.choice(ctx.ints)))
            place = self.scalar_place(ctx, other, effects)
            if place is not None:
                return Conv(t, Get(place))
        if r < 0.25 and depth > 0:
            a = self.operand(ctx, rnd.choice(INTS), depth - 1, effects)
            op = rnd.choice("+-*&|^")
            b = self.lit(a.t) if rnd.random() < 0.5 else \
                self.operand(ctx, a.t, 0, effects)
            return Conv(t, Bin(op, a, b))
        if r < 0.38 and effects:
            call = self.value_call(ctx, t)
            if call is not None:
                return call
        place = self.scalar_place(ctx, t, effects)
        if place is None:
            return Conv(t, Int(ctx.ints[0])) if ctx.ints else \
                Conv(t, Get(self.scalar_place(ctx, "i32", effects)))
        return Get(place)

    def expr(self, ctx, t, depth, effects=True):
        rnd = self.rnd
        if depth <= 0 or rnd.random() < 0.35:
            return self.operand(ctx, t, 1, effects)
        # note: most narrow arithmetic wraps in a conversion, checked it
        #       overflows soon
        if t != "i32" and rnd.random() < 0.5:
            inner = self.expr(ctx, t, depth - 1, effects)
            op = rnd.choice("+-*&|^")
            return Conv(t, Bin(op, inner, self.lit(t) if rnd.random() < 0.5
                               else self.operand(ctx, t, 0, effects)))
        a = self.expr(ctx, t, depth - 1, effects)
        op = rnd.choice(["+", "+", "-", "-", "*", "&", "|", "^", "/", "%",
                         "<<", ">>"])
        if op in ("<<", ">>"):
            bits = fl.BITS[t]
            if rnd.random() < 0.6:
                b = self.lit(t, 0, min(bits - 1, 9))
            else:
                b = Bin("&", self.operand(ctx, t, 0, effects),
                        Lit(t, rnd.choice([3, 7, 15, 31])))
            return Bin(op, a, b)
        if op in ("/", "%"):
            if rnd.random() < 0.85:
                b = Lit(t, rnd.choice([1, 2, 3, 5, 7, -1, -3]))
            else:
                b = self.operand(ctx, t, 0, effects)
            return Bin(op, a, b)
        if op == "*" and rnd.random() < 0.7:
            return Bin(op, a, self.lit(t, -3, 5))
        b = self.lit(t) if rnd.random() < 0.4 else \
            self.expr(ctx, t, depth - 1, effects)
        return Bin(op, a, b)

    def cond(self, ctx, effects=True):
        rnd = self.rnd
        t = rnd.choice(INTS)
        a = self.expr(ctx, t, 1, effects)
        b = self.lit(t) if rnd.random() < 0.6 else \
            self.operand(ctx, t, 0, effects=False)
        c = Cond(a, rnd.choice(["==", "!=", "<", "<=", ">", ">="]), b)
        if rnd.random() < 0.2:
            t = rnd.choice(INTS)
            d = Cond(self.operand(ctx, t, 0, effects=False),
                     rnd.choice(["<", ">", "!="]), self.lit(t))
            return Logic(rnd.choice(["and", "or"]), c, d)
        return c

    # calls

    def args_for(self, ctx, fn, exclude, allow_effects=True):
        # the first argument may call a function that writes, a later one
        # does not, nor reads what the function uses or writes in place
        args = []
        for k, (name, kind, t) in enumerate(fn.params):
            effects = k == 0 and allow_effects
            if kind in ("arr", "mut"):
                return None
            if kind == "int":
                args.append(Lit("int", self.rnd.randint(0, 3)))
                continue
            for _ in range(6):
                a = self.expr(ctx, t, 1, effects and not exclude)
                if not (a.reads() & (exclude | fn.uses | fn.writes)):
                    break
            else:
                return None
            # note: an odd argument is not a zero divisor for any constant
            args.append(Bin("|", a, Lit(t, 1)) if ctx.constant_ints else a)
        return args

    def value_call(self, ctx, t):
        rnd = self.rnd
        options = [fn for fn in ctx.calls if fn.result == "i32"]
        if not options:
            return None
        fn = rnd.choice(options)
        call = self.make_call(ctx, fn)
        if call is None:
            return None
        return call if t == "i32" else Conv(t, call)

    def make_call(self, ctx, fn):
        rnd = self.rnd
        if fn.owner == "rec":
            recv = self.rec_place(ctx, effects=ctx.allow_globals and
                                  rnd.random() < 0.3)
            if recv is None or recv.reads() & (fn.uses | fn.writes):
                return None
            args = self.args_for(ctx, fn, recv.reads() if fn.mut_self
                                 else set(), allow_effects=False)
            if args is None:
                return None
            return Call(fn, args, recv=recv)
        if fn.owner == "cell":
            cells = [c for c in self.cells if ctx.allow_globals or
                     c.root not in self.global_names]
            if not cells:
                return None
            recv = rnd.choice(cells)
            args = self.args_for(ctx, fn, {recv.root}, allow_effects=False)
            if args is None:
                return None
            return Call(fn, args, recv=recv)
        if fn.short == "gsum":
            arrays = [a for a in ctx.byte_arrays + ctx.sector_arrays
                      if not (a.reads() & fn.uses)]
            if not arrays:
                return None
            arr = self.fresh(ctx, rnd.choice(arrays))
            k = self.expr(ctx, "i32", 1, effects=False)
            if k.reads() & (fn.uses | fn.writes):
                return None
            return Call(fn, [ArrArg(arr), k], targs=f"<{arr.t[1]}>")
        if fn.short == "gcall":
            arrays = [a for a, _ in ctx.rec_arrays
                      if not isinstance(a, Elem) and
                      not (a.reads() & (fn.uses | fn.writes))]
            if not arrays:
                return None
            arr = rnd.choice(arrays)
            k = self.expr(ctx, "i8", 1, effects=False)
            if k.reads() & (fn.uses | fn.writes | {arr.root}):
                return None
            return Call(fn, [ArrArg(arr), k], targs="<rec>")
        if fn.params and fn.params[0][1] == "arr":
            arrays = [a for a, _ in ctx.rec_arrays
                      if not isinstance(a, Elem) and
                      not (a.reads() & (fn.uses | fn.writes))]
            if not arrays:
                return None
            arr = rnd.choice(arrays)
            rest = Function(fn.short, fn.params[1:], fn.result)
            rest.uses = fn.uses
            rest.writes = fn.writes
            if fn.short == "walk":
                length = dict((id(a), n) for a, n in ctx.rec_arrays)[id(arr)]
                more = [Lit("i32", self.rnd.randint(-1, length - 1)
                            if self.rnd.random() < 0.9 else length)]
            else:
                more = self.args_for(ctx, rest, {arr.root})
            if more is None:
                return None
            return Call(fn, [ArrArg(arr)] + more)
        if fn.params and fn.params[0][1] == "mut":
            rec = self.rec_place(ctx, effects=ctx.allow_globals and
                                 rnd.random() < 0.3)
            if rec is None or rec.reads() & (fn.uses | fn.writes):
                return None
            rest = Function(fn.short, fn.params[1:], fn.result)
            more = self.args_for(ctx, rest, {rec.root})
            if more is None:
                return None
            return Call(fn, [ArrArg(rec)] + more)
        if not ctx.allow_globals and fn is not self.mix and \
                fn.uses & self.global_names:
            return None
        args = self.args_for(ctx, fn, set())
        if args is None:
            return None
        return Call(fn, args)

    # statements

    def block(self, ctx, size, depth):
        body = []
        for _ in range(size):
            s = self.statement(ctx, depth)
            if s is not None:
                body.append(s)
        return body

    def statement(self, ctx, depth):
        rnd = self.rnd
        r = rnd.random()
        if r < 0.28:
            t = rnd.choice(INTS)
            place = self.scalar_place(ctx, t, effects=True, write=True)
            if place is None:
                return None
            expr = self.expr(ctx, t, 2)
            # a destination is not passed to a call of its value in place
            top = expr.inner if isinstance(expr, Conv) else expr
            # note: a result shares no storage with the arguments (README
            #       chapter 8, 'a result or a global sharing storage')
            if isinstance(top, Call) and (
                    top.reads() & {place.root} or
                    any(c.written() & place.reads()
                        for c in calls_of(expr))):
                return None
            return Assign(place, expr)
        if r < 0.36:
            t = rnd.choice(INTS)
            name = self.name("v")
            s = Decl(name, self.expr(ctx, t, 2))
            ctx.scalars[t].append(Var(name, t))
            return s
        if r < 0.5:
            options = [fn for fn in ctx.calls if fn.result is None]
            if not options:
                return None
            call = self.make_call(ctx, rnd.choice(options))
            return Do(call) if call is not None else None
        if r < 0.58:
            call = self.value_call(ctx, "i32")
            if call is None or self.mix is None or \
                    call.reads() & {"chk"}:
                return None
            return Do(Call(self.mix, [call]))
        if r < 0.66 and depth > 0:
            then = self.block(ctx.copy(), rnd.randint(1, 3), depth - 1)
            other = self.block(ctx.copy(), rnd.randint(1, 2), depth - 1) \
                if rnd.random() < 0.4 else None
            return If(self.cond(ctx), then, other)
        if r < 0.72 and depth > 0 and ctx.loop_depth < 2:
            inner = ctx.copy()
            inner.loop_depth += 1
            inner.in_loop = True
            counter = self.name("n")
            inner.ints = inner.ints + [counter]
            body = self.block(inner, rnd.randint(1, 4), depth - 1)
            return Loop(counter, rnd.randint(1, 4), body)
        if r < 0.8 and depth > 0 and ctx.loop_depth < 2 and \
                "i" not in ctx.ints:
            return self.foo(ctx, depth)
        if r < 0.86 and ctx.in_loop:
            word = rnd.choice(["break", "continue"])
            return If(self.cond(ctx, effects=False), [Jump(word)])
        if r < 0.89 and ctx.fn is not None:
            return If(self.cond(ctx, effects=False), [Jump("return")])
        if r < 0.905:
            t = rnd.choice(INTS)
            cond = Cond(self.operand(ctx, t, 0, effects=False), "==",
                        self.lit(t))
            return If(cond,
                      [ExitStmt(self.operand(ctx, "i32", 0, effects=False))])
        return self.io(ctx)

    def foo(self, ctx, depth):
        rnd = self.rnd
        inner = ctx.copy()
        inner.loop_depth += 1
        inner.in_loop = True
        inner.ints = inner.ints + ["i"]
        kinds = []
        if ctx.rec_arrays:
            kinds += ["rec"] * 3
        if ctx.byte_arrays:
            kinds.append("byte")
        if not kinds:
            return None
        if rnd.choice(kinds) == "rec":
            arr, length = rnd.choice(ctx.rec_arrays)
            e = Var("e", "rec", root=arr.root)
            inner.recs = inner.recs + [e] * 2
        else:
            arr = self.fresh(ctx, rnd.choice(ctx.byte_arrays))
            length = arr.t[2]
            inner.scalars["i8"] = inner.scalars["i8"] + \
                [Var("e", "i8", root=arr.root)] * 2
        count = None
        if rnd.random() < 0.3:
            if rnd.random() < 0.5 and length:
                count = Lit("int", rnd.randint(0, length))
            else:
                count = Index("mask", self.operand(ctx, "i8", 0, False),
                              rnd.choice([1, 3, 7]))
        body = self.block(inner, rnd.randint(1, 4), depth - 1)
        return Foo(arr, count, body)

    def io(self, ctx):
        rnd = self.rnd
        r = rnd.random()
        if r < 0.4 and ctx.byte_arrays:
            arr = self.fresh(ctx, rnd.choice(ctx.byte_arrays))
            length = arr.t[2] or TAG
            args = []
            if rnd.random() < 0.7:
                args.append(Index("mask", self.operand(ctx, "i8", 0, False),
                                  rnd.choice([1, 3]))
                            if rnd.random() < 0.5 else
                            Lit("int", rnd.randint(0, length)))
                if rnd.random() < 0.5:
                    args.append(Index("mask", self.operand(ctx, "i8", 0,
                                                           False), 1))
            return Io("write", arr, args)
        if r < 0.55 and ctx.byte_arrays and self.read_left > 0:
            count = len(self.stdin_line) + rnd.randint(0, 1)
            arrays = [a for a in ctx.byte_arrays if a.t[2] >= count]
            if not arrays:
                return None
            self.read_left -= 1
            arr = self.fresh(ctx, rnd.choice(arrays))
            # note: the count holds the whole line, the uart gives a read
            #       one line and a file as many bytes as fit
            count = Lit("int", count)
            start = Index("mask", self.operand(ctx, "i8", 0, False), 1) \
                if rnd.random() < 0.6 else Lit("int", 0)
            dest = self.name("got")
            ctx.ints.append(dest)
            return Io("read", arr, [count, start], dest)
        if r < 0.65 and ctx.sector_arrays and ctx.allow_globals:
            # the bytes of wider elements, a read of one line into one
            # element keeps the bytes past the line
            arr = self.fresh(ctx, rnd.choice(ctx.sector_arrays))
            if self.read_left > 0 and rnd.random() < 0.3:
                self.read_left -= 1
                start = Index("mask", self.operand(ctx, "i32", 0, False),
                              rnd.choice([3, 127]))
                dest = self.name("got")
                ctx.ints.append(dest)
                size = element_bytes(arr.t[1])
                count = (len(self.stdin_line) + size - 1) // size
                return Io("read", arr, [Lit("int", count), start], dest)
            count = Index("mask", self.operand(ctx, "i8", 0, False),
                          rnd.choice([1, 3]))
            start = Index("mask", self.operand(ctx, "i16", 0, False),
                          rnd.choice([3, 127, 255]))
            return Io("write", arr, [count, start])
        if ctx.sector_arrays and ctx.allow_globals:
            arr = self.fresh(ctx, rnd.choice(ctx.sector_arrays))
            # note: mostly a sector of the card, past it the 'sd card' stop
            mask = SECTORS - 1 if rnd.random() < 0.9 else 7
            sector = Index("mask", self.operand(ctx, "i32", 0, False), mask)
            # note: the sector before the array, a call in it may write
            #       what the array's index reads
            if rnd.random() < 0.25:
                sector = Index("mask", Call(self.bump, []), mask)
            args = [sector]
            per = SECTOR_BYTES // element_bytes(arr.t[1])
            if arr.t[2] > per:
                args.append(Lit("int", rnd.choice([0, per]))
                            if rnd.random() < 0.6 else
                            Index("mask", self.operand(ctx, "i16", 0, False),
                                  per | (per - 1)))
            kind = rnd.choice(["sector_read", "sector_write"])
            return Io(kind, arr, args)
        return None

    # the program

    def rec_literals(self, count):
        # 'count' records with small fields, kept for the model
        rnd = self.rnd
        values = []
        for _ in range(count):
            values.append({"a": rnd.randint(-9, 9), "b": rnd.randint(-99, 99),
                           "c": rnd.randint(-999, 999),
                           "tag": [rnd.randint(-9, 9) for _ in range(TAG)]})
        self.initial.append(values)
        texts = []
        for v in values:
            tag = ", ".join(map(str, v["tag"]))
            texts.append(f"{{{v['a']}, {v['b']}, {v['c']}, {{{tag}}}}}")
        return "{" + ", ".join(texts) + "}"

    def program(self):
        rnd = self.rnd
        self.initial = []
        self.stdin_line = "".join(rnd.choice(LETTERS) for _ in
                                  range(rnd.randint(0, 2))).encode() + b"\n"
        self.read_left = 1
        cell_sizes = sorted(rnd.sample([2, 3, 4, 5, 6, 8], 2))
        tab_len = 4
        loc_len = rnd.choice([2, 2, 8, 8, 3])
        self.g0 = rnd.randint(-5, 5)
        self.global_names = {"g", "chk", "tab", "cards", "big", "cg"}
        head = [
            "type rec {",
            "    a i8,",
            "    b i16,",
            "    c i32,",
            f"    tag i8[{TAG}],",
            "}",
            "",
            "type cell<N> {",
            "    v i16,",
            "    buf i8[N],",
            "}",
            "",
            f"type cell_a = cell<{cell_sizes[0]}>",
            f"type cell_b = cell<{cell_sizes[1]}>",
            "",
            "type blk {",
            "    n i16,",
            f"    data i32[{SECTOR_BYTES // 4}],",
            "}",
            "",
            f"dat g = i32({self.g0})",
            "dat chk = i32(0)",
            f"dat tab = rec[{tab_len}]{self.rec_literals(tab_len)}",
            "dat cards = blk[2]",
            f"dat big = i16[{SECTOR_BYTES}]",
            "dat cg = cell_b",
            "",
        ]
        self.head = head
        tab = Glob("tab", ("arr", "rec", tab_len))
        cards = Glob("cards", ("arr", "blk", 2))
        big = Glob("big", ("arr", "i16", SECTOR_BYTES))
        cg = Glob("cg", ("cell", cell_sizes[1]))
        g = Glob("g", "i32")
        chk = Glob("chk", "i32")
        self.cells = [cg]
        fns = []

        # 'mix' folds a value into 'chk'
        mix = Function("mix", [("v", "val", "i32")], None)
        mix_body = Assign(chk, Bin("&", Bin("+", Bin("*", Get(chk),
                                                     Lit("i32", 31)),
                                            Bin("&", Get(Var("v", "i32")),
                                                Lit("i32", 0xffff))),
                                   Lit("i32", 0xffff)))
        mix.body = [mix_body]
        mix.uses = {"chk"}
        mix.writes = {"chk"}
        mix.noinline = rnd.random() < 0.3
        self.mix = mix
        fns.append(mix)

        # 'bump' writes 'g' and gives the old or the new value
        bump = Function("bump", [], "i32")
        step = rnd.randint(1, 3)
        res = Var("res", "i32")
        if rnd.random() < 0.5:
            bump.body = [Assign(g, Bin("+", Get(g), Lit("i32", step))),
                         Assign(res, Get(g))]
        else:
            bump.body = [Assign(res, Get(g)),
                         Assign(g, Bin("^", Get(g), Lit("i32", step)))]
        bump.uses = {"g"}
        bump.writes = {"g"}
        bump.noinline = rnd.random() < 0.4
        self.bump = bump
        fns.append(bump)

        # methods of 'rec': 'step' writes 'self', 'sum' reads it
        sctx = Ctx(self)
        sctx.allow_globals = False
        selfp = Var("self", "rec")
        sctx.recs = [selfp]
        sctx.scalars["i8"].append(Var("k", "i8", readonly=True))
        sctx.calls = [mix]
        stepf = Function("step", [("k", "val", "i8")], None, owner="rec",
                         mut_self=True, noinline=rnd.random() < 0.4)
        stepf.body = self.block(sctx, rnd.randint(2, 4), 1)
        set_effects(stepf, {"chk"})
        fns.append(stepf)

        sumf = Function("sum", [("d", "val", "i32")], "i32", owner="rec",
                        noinline=rnd.random() < 0.4)
        rctx = Ctx(self)
        rctx.allow_globals = False
        rctx.recs = [selfp]
        rctx.scalars["i32"] = [Var("d", "i32", readonly=True), res]
        sumf.body = [Assign(res, self.expr(rctx, "i32", 2, effects=False))]
        if rnd.random() < 0.4:
            sumf.body.append(If(self.cond(rctx, effects=False),
                                [Jump("return")]))
            sumf.body.append(Assign(res, Bin("+", Get(res),
                                             Get(Field(selfp, "c", "i32")))))
        fns.append(sumf)

        # a method of the generic 'cell' that sees its constant 'N'
        put = Function("put", [("x", "val", "i8")], None, owner="cell",
                       mut_self=True, noinline=rnd.random() < 0.4)
        cself = Var("self", ("cell", 0))
        v = Field(cself, "v", "i16")
        put.body = [
            Assign(Elem(Field(cself, "buf", ("arr", "i8", None)),
                        Index("mod", Get(v), "N")),
                   Get(Var("x", "i8"))),
            Assign(v, Bin("+", Get(v), Lit("i16", rnd.randint(1, 2)))),
        ]
        fns.append(put)
        total = Function("total", [("d", "val", "i32")], "i32", owner="cell",
                         noinline=rnd.random() < 0.4)
        total.body = [Foo(Field(cself, "buf", ("arr", "i8", None)), None, [
            Assign(res, Bin("+", Bin("*", Get(res), Lit("i32", 3)),
                            Conv("i32", Get(Var("e", "i8")))))]),
            Assign(res, Bin("^", Get(res), Get(Var("d", "i32"))))]
        fns.append(total)

        # a generic function with an array parameter, an instance for each
        # element type and length
        gsum = Function("gsum", [("a", "arr", ("arr", "T", False, "T[]")),
                                 ("k", "val", "i32")], "i32",
                        tparams="<T type>", noinline=rnd.random() < 0.6)
        ge = Var("e", "T")
        body = []
        if rnd.random() < 0.4:
            body.append(If(Cond(Conv("i32", Get(ge)), "==",
                                Get(Var("k", "i32"))), [Jump("continue")]))
        body.append(Assign(res, Bin("+", Get(res),
                                    Bin("*", Conv("i32", Get(ge)),
                                        Get(Var("k", "i32"))))))
        if rnd.random() < 0.4:
            body.append(If(Cond(Get(res), ">",
                                Lit("i32", rnd.randint(50, 5000))),
                           [Jump(rnd.choice(["break", "return"]))]))
        gsum.body = [Foo(Var("a", ("arr", "T", None)), None, body)]
        fns.append(gsum)

        # a generic function over an array of records that calls the methods
        # of 'e', the type argument given
        gcall = Function("gcall", [("a", "arr", ("arr", "T", True, "T[]")),
                                   ("k", "val", "i8")], "i32",
                         tparams="<T type>", noinline=rnd.random() < 0.5)
        ce = Var("e", "rec")
        ck = Get(Var("k", "i8"))
        body = [Do(Call(stepf, [ck], recv=ce))]
        if rnd.random() < 0.5:
            body.append(If(Cond(Get(Field(ce, "a", "i8")), "==", ck),
                           [Jump(rnd.choice(["break", "continue",
                                             "return"]))]))
        body.append(Assign(res, Bin("+", Get(res),
                                    Call(sumf, [Conv("i32", ck)], recv=ce))))
        if rnd.random() < 0.5:
            body.reverse()
        gcall.body = [Foo(Var("a", ("arr", "rec", None)), None, body)]
        fns.append(gcall)

        # an inlined recursion that a constant argument ends
        depth = Function("depth", [("t", "mut", "rec"), ("n", "int", "int")],
                         None, can_noinline=True)
        tp = Var("t", "rec")
        dctx = Ctx(self, depth)
        dctx.allow_globals = False
        dctx.recs = [tp]
        dctx.ints = ["n"]
        dctx.constant_ints = True
        dctx.calls = [mix, stepf, sumf]
        pre = self.block(dctx, rnd.randint(1, 2), 1)
        post = self.block(dctx, rnd.randint(0, 2), 1)
        depth.body = [If(Cond(Int("n"), "==", Lit("int", 0)),
                         [Jump("return")])] + pre + \
            [Do(Call(depth, [ArrArg(tp),
                             Bin("-", Int("n"), Lit("int", 1))]))] + post
        depth.noinline = rnd.random() < 0.3
        fns.append(depth)

        # a noinline recursion over an array of records of two lengths
        walk = Function("walk", [("a", "arr", ("arr", "rec", True, "rec[]")),
                                 ("w", "val", "i32")], "i32",
                        noinline=True, can_inline=False)
        ap = Var("a", ("arr", "rec", None))
        wp = Var("w", "i32", readonly=True)
        wctx = Ctx(self, walk)
        wctx.allow_globals = False
        wctx.rec_arrays = [(ap, 2)]
        wctx.scalars["i32"] = [wp]
        wctx.calls = [mix, stepf, sumf, gsum, gcall, depth]
        wctx.byte_arrays = [Field(Elem(ap, Index("const", m=0)), "tag",
                                  ("arr", "i8", TAG))]
        elem = Elem(ap, Index("plain", Get(wp)))
        inner = self.block(wctx, rnd.randint(1, 3), 1)
        walk.body = [If(Cond(Get(wp), "<", Lit("i32", 0)), [Jump("return")])]
        walk.body += inner
        walk.body.append(Assign(res, Bin("+", Call(walk, [
            ArrArg(ap), Bin("-", Get(wp), Lit("i32", 1))]),
            Get(Field(elem, "c", "i32")))))
        fns.append(walk)

        # a function with a body drawn like 'main' over an array parameter
        work = Function("work", [("a", "arr", ("arr", "rec", True, "rec[]")),
                                 ("k", "val", "i32")], "i32",
                        noinline=rnd.random() < 0.5)
        kctx = Ctx(self, work)
        kctx.rec_arrays = [(Var("a", ("arr", "rec", None)), 2)]
        kctx.scalars["i32"] = [Var("k", "i32", readonly=True), res]
        kctx.calls = [mix, bump, stepf, sumf, gsum, gcall, depth]
        kctx.byte_arrays = [Field(Elem(kctx.rec_arrays[0][0],
                                       Index("const", m=1)), "tag",
                                  ("arr", "i8", TAG))]
        work.body = self.block(kctx, rnd.randint(2, 5), 2)
        fns.append(work)

        for fn in fns:
            if fn not in (mix, bump):
                set_effects(fn, set())
        # the first parameter of 'walk' and 'work' and the receiver of the
        # 'mut' methods are written in place
        self.functions = fns

        ctx = Ctx(self)
        ctx.rec_arrays = [(tab, tab_len)]
        loc = Var("loc", ("arr", "rec", loc_len))
        ctx.rec_arrays.append((loc, loc_len))
        ctx.scalars["i32"] = [g]
        ctx.byte_arrays = []
        cb = Var("cb", ("cell", cell_sizes[0]))
        self.cells.append(cb)
        ctx.byte_arrays = [Field(cb, "buf", ("arr", "i8", cell_sizes[0])),
                           Field(cg, "buf", ("arr", "i8", cell_sizes[1]))]
        ctx.calls = [bump, stepf, sumf, put, total, gsum, gcall, depth, walk,
                     work]
        for name in ("tab", "loc"):
            for k in range(2):
                arr = tab if name == "tab" else loc
                length = tab_len if name == "tab" else loc_len
                ctx.byte_arrays.append(Field(Elem(arr, Index("const",
                                                             m=k % length)),
                                             "tag", ("arr", "i8", TAG)))
        ctx.sector_arrays = [
            Field(Elem(cards, Index("const", m=0)), "data",
                  ("arr", "i32", SECTOR_BYTES // 4)),
            Field(Elem(cards, Index("const", m=1)), "data",
                  ("arr", "i32", SECTOR_BYTES // 4)),
            big]
        loc_text = self.rec_literals(loc_len)
        loc_values = self.initial[1]
        main = [DeclArray("loc", f"rec[{loc_len}]{loc_text}",
                          lambda: copy_value(loc_values)),
                DeclArray("cb", "cell_a",
                          lambda: {"v": 0, "buf": [0] * cell_sizes[0]})]
        main += self.block(ctx, rnd.randint(6, 14), 2)
        # every element of 'tab' and 'loc' is printed at the end
        self.main = main
        self.cell_sizes = cell_sizes
        self.tab_len = tab_len
        self.loc_len = loc_len

    def text(self, variant):
        lines = list(self.head)
        for fn in self.functions:
            if variant == "noinline":
                noinline = fn.can_noinline
            elif variant == "inline":
                noinline = not fn.can_inline
            else:
                noinline = fn.noinline
            if fn.short == "walk":
                noinline = True
            lines += fn.text(noinline)
            lines.append("")
        lines.append("func main() {")
        lines += block_text(self.main, 1)
        lines += ["    foo tab {", "        write(1, e.tag)",
                  "        mix(e.c)", "        mix(i32(e.b))", "    }",
                  "    foo loc {", "        mix(e.c + i32(e.a))", "    }",
                  "    mix(i32(cg.v))", "    mix(cg.total(g))",
                  "    exit(chk & 127)", "}"]
        return "\n".join(lines) + "\n"

    def model(self, has_card):
        st = State(self.stdin_line, has_card)
        st.globals = {
            "g": self.g0,
            "chk": 0,
            "tab": copy_value(self.initial[0]),
            "cards": [{"n": 0, "data": [0] * (SECTOR_BYTES // 4)}
                      for _ in range(2)],
            "big": [0] * SECTOR_BYTES,
            "cg": {"v": 0, "buf": [0] * self.cell_sizes[1]},
        }
        env = {}
        try:
            run_block(st, env, self.main)
            for rec in st.globals["tab"]:
                st.out += to_bytes(rec["tag"], "i8", 0, TAG)
                mix_value(st, rec["c"])
                mix_value(st, rec["b"])
            for rec in env["loc"][0][0]:
                mix_value(st, check("i32", rec["c"] + rec["a"]))
            cg = st.globals["cg"]
            mix_value(st, cg["v"])
            total = 0
            for x in cg["buf"]:
                total = check("i32", check("i32", total * 3) + x)
            mix_value(st, total ^ st.globals["g"])
            code = st.globals["chk"] & 127
            return ("exit", code, bytes(st.out)), None
        except Exit as e:
            return ("exit", e.code, bytes(st.out)), None
        except Panic as p:
            return ("panic", fl.PANIC_CODE[{"bounds": "bounds",
                                            "overflow": "overflow",
                                            "division": "division",
                                            "shift": "shift"}[p.kind]],
                    bytes(st.out)), p.kind
        except SdStop:
            return ("sdcard", fl.SDCARD_CODE, bytes(st.out)), "sdcard"


def mix_value(st, v):
    chk = st.globals["chk"]
    st.globals["chk"] = (check("i32", chk * 31) + (v & 0xffff)) & 0xffff


def calls_of(expr):
    out = []
    stack = [expr]
    while stack:
        x = stack.pop()
        if isinstance(x, Call):
            out.append(x)
            stack += x.args
            if x.recv is not None:
                stack.append(x.recv)
        elif isinstance(x, (Bin,)):
            stack += [x.a, x.b]
        elif isinstance(x, Conv):
            stack.append(x.inner)
        elif isinstance(x, Index) and x.x is not None:
            stack.append(x.x)
        elif isinstance(x, Get):
            stack.append(x.place)
        elif isinstance(x, Elem):
            stack += [x.arr, x.index]
        elif isinstance(x, Field):
            stack.append(x.rec)
    return out


def walk_body(body):
    for s in body:
        yield s
        for name in ("then", "other", "body"):
            inner = getattr(s, name, None)
            if inner:
                yield from walk_body(inner)


def set_effects(fn, seed):
    # the globals a function reads and writes, through its callees too
    uses = set(seed)
    writes = set(seed)
    for s in walk_body(fn.body):
        for attr in ("expr", "call", "cond", "place", "count", "arr"):
            x = getattr(s, attr, None)
            if x is None:
                continue
            items = [x]
            if isinstance(x, (Cond, Logic)):
                items = [x.a, x.b]
            for item in items:
                for c in calls_of(item) if not isinstance(item, Cond) else \
                        calls_of(item.a) + calls_of(item.b):
                    if c.fn is not fn:
                        uses |= c.fn.uses
                        writes |= c.fn.writes
    fn.uses = uses
    fn.writes = writes


# running


# the check each panic needs, for the build with a subset of the checks
CHECK_OF = {"bounds": "upper,lower", "overflow": "overflow",
            "division": "division", "shift": "shift"}
ALL_CHECKS = ("upper", "lower", "overflow", "division", "shift", "overlap",
              "frame", "stack")
OTHER_OPTIONS = (("--checks=noub", "--nopt"),
                 ("--checks=noub", "--versions=nested"),
                 ("--checks=noub", "--versions=first"),
                 ("--checks=noub", "--versions=last"),
                 ("--checks=noub,line",))
FIXED_REGISTER = "is always used by the operation"
ALL_TARGETS = ("x86_64", "rv32i-fpga", "rv32i", "rv32i-qemu")


def subset_options(rnd, panic):
    # a list of checks that keeps the one the model ends with
    if panic in CHECK_OF:
        keep = CHECK_OF[panic].split(",")
    else:
        keep = []
    rest = [c for c in ALL_CHECKS if c not in keep and rnd.random() < 0.4]
    checks = keep + rest
    if not checks:
        return ()
    if rnd.random() < 0.5 and not keep:
        drop = rnd.choice(ALL_CHECKS)
        return (f"--checks=noub,-{drop}",)
    return ("--checks=" + ",".join(checks),)


def build(work, source, target, options, stdin):
    # what the program did: (kind, code, stdout) or (problem, detail)
    path = os.path.join(work, "f.baz")
    with open(path, "w") as f:
        f.write(fl.split_include(work, source))
    card = os.path.join(work, "sdcard.img")
    with open(card, "wb") as f:
        f.write(card_image())
    options = list(options)
    if target == "x86_64":
        assembly = os.path.join(work, "f.s")
        c = fl.compile_source(fl.COMPILER_ASAN, path,
                              ["--target=x86_64"] + options, assembly)
        if not (c.returncode or c.timed_out):
            exe, problem = fl.assemble_x86(assembly, work)
            if problem:
                return problem[0], problem[1][:300]
            o = fl.outcome_x86(fl.run([exe], fl.RUN_TIMEOUT, cwd=work,
                                      stdin_bytes=stdin))
    elif target == "rv32i":
        assembly = os.path.join(work, "f.s")
        image = os.path.join(work, "f-rv32i.bin")
        c = fl.compile_source(fl.COMPILER_ASAN, path,
                              ["--target=rv32i", "--bin=" + image] + options,
                              assembly)
        if not (c.returncode or c.timed_out):
            exe, problem = fl.assemble_rv32i(assembly, image, work, "f")
            if problem:
                return problem[0], problem[1][:300]
            o = fl.outcome_x86(fl.run([fl.QEMU_USER, exe], fl.RUN_TIMEOUT,
                                      cwd=work, stdin_bytes=stdin))
    else:
        image = os.path.join(work, f"f-{target}.bin")
        c = fl.compile_source(fl.COMPILER_ASAN, path,
                              ["--target=" + target, "--bin=" + image] +
                              options, os.devnull)
        if not (c.returncode or c.timed_out):
            command = fl.QEMU_SYSTEM + [image] if target == "rv32i-qemu" \
                else [fl.EMULATOR, image, card]
            o = fl.outcome_fpga(fl.run(command, fl.RUN_TIMEOUT, cwd=work,
                                       stdin_bytes=stdin + b"\x04" * 64))
    if c.timed_out:
        return ("compiler-hang", "")
    if c.returncode:
        sanitizer = any(t in c.stderr for t in fl.SANITIZER_TEXT)
        if sanitizer or c.returncode < 0 or c.returncode in fl.SANITIZER_EXIT:
            return ("compiler-crash", c.stderr[-3000:])
        return ("compile", c.stderr[:800])
    if o.kind not in ("exit", "panic", "sdcard"):
        return (o.kind, f"{o.code} {o.detail} {o.stdout[-200:]!r}")
    return (o.kind, o.code, o.stdout)


def short(outcome):
    if len(outcome) == 3:
        return f"{outcome[0]} {outcome[1]} {outcome[2][-80:]!r}"
    return f"{outcome[0]} {outcome[1][:300]}"


def judge(gen, rnd, counts):
    found = []
    expected, panic = gen.model(True)
    expected_qemu, _ = gen.model(False)
    counts[f"end-{panic or 'exit'}"] = 1
    stdin = gen.stdin_line
    runs = []
    mixed = gen.text("mixed")
    targets = ALL_TARGETS if os.environ.get("FUZZ_TARGETS") != "fixed" \
        else fl.TARGETS
    for target in targets:
        runs.append(("mixed", mixed, target, ("--checks=noub",)))
    for target in fl.TARGETS:
        for options in OTHER_OPTIONS:
            runs.append(("mixed", mixed, target, options))
        runs.append(("mixed", mixed, target, subset_options(rnd, panic)))
    for variant in ("noinline", "inline"):
        text = gen.text(variant)
        if text == mixed:
            continue
        for target in fl.TARGETS:
            runs.append((variant, text, target, ("--checks=noub",)))
    with tempfile.TemporaryDirectory(prefix="fuzz-combos-") as work:
        for variant, text, target, options in runs:
            want = expected_qemu if target == "rv32i-qemu" else expected
            got = build(work, text, target, options, stdin)
            counts["builds"] = counts.get("builds", 0) + 1
            if got == want or got[0] == "skip":
                continue
            if got[0] == "compile" and fl.out_of_registers(got[1]):
                counts["out-of-registers"] = 1
                continue
            # the registers an instruction fixes, e.g. 'rdx' of a sector
            # transfer and of '%' on x86_64 (README chapter 8, item 3)
            if got[0] == "compile" and FIXED_REGISTER in got[1]:
                counts["fixed-register"] = 1
                continue
            where = f"{target} {variant} {' '.join(options)}"
            if got[0] == "compile":
                message = got[1].strip().splitlines()[0] if got[1] else ""
                found.append(("combos-reject",
                              f"{target} {message.split(': ', 1)[-1]}",
                              f"{where}: {message}"))
                return found, text
            if got[0] == "compiler-crash":
                found.append((got[0], fl.signature(got[1]),
                              f"{where}\n{got[1]}"))
                continue
            category = got[0] if got[0] in ("compiler-hang", "invalid-asm",
                                            "link-failure",
                                            "image-mismatch") \
                else "combos-mismatch"
            found.append((category, f"{where} {got[0]}",
                          f"stdin {stdin!r}\nexpected {short(want)}, "
                          f"got {short(got)}"))
    return found, mixed


def make(seed, index):
    rnd = random.Random(seed * 1000003 + index)
    gen = Gen(rnd)
    gen.program()
    return gen, rnd


def fuzz_one(args):
    seed, index = args
    gen, rnd = make(seed, index)
    counts = {}
    try:
        gen.model(True)
    except TooLong:
        return gen.text("mixed"), [], {"too-long": 1}
    except Exception as e:
        # a defect of the generator or its model, not of the compiler
        return gen.text("mixed"), [("combos-generator", type(e).__name__,
                                    repr(e))], {}
    found, text = judge(gen, rnd, counts)
    return text, found, counts


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == "--one":
        gen, _ = make(int(sys.argv[2]), int(sys.argv[3]))
        variant = sys.argv[4] if len(sys.argv) > 4 else "mixed"
        print(gen.text(variant), end="")
        print(f"# stdin {gen.stdin_line!r}")
        print(f"# model {short(gen.model(True)[0])}")
        print(f"# model without a card {short(gen.model(False)[0])}")
        return 0
    if shutil.which("nasm") is None:
        sys.exit("nasm is needed")
    fl.assert_fresh(fl.COMPILER_ASAN)
    count, seed, jobs = fl.arguments(200)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs, chunksize=1)


if __name__ == "__main__":
    sys.exit(main())
