#!/usr/bin/env python3
# model based fuzzer of the whole language: random valid programs that mix
# user types with nested arrays and records, generic types and functions (type
# arguments given and deduced), methods, constructors, inlined and 'noinline'
# functions with 'mut' and unsized array parameters, bounded recursion, 'foo'
# with a count and nested, 'loop' with 'break' and 'continue', 'if' chains,
# shadowing blocks, conversions between widths, every operator, literals in
# decimal, hex and binary, 'let', 'dat', globals, 'array_copy',
# 'arrays_equal', 'write' and a printed trace of values, 'read' of one line of
# input at the top of 'main'
#
# python runs the same program (an interpreter of its tree) and gives the
# output and the exit code, or the first panic under '--checks=noub', for a
# default integer of 64 bits (x86_64) and of 32 bits (the rv32i targets). the
# program is built with every function inlined, with every function
# 'noinline' and with each as drawn, each with '--checks=noub' on x86_64,
# rv32i-fpga and one of rv32i and rv32i-qemu ('fuzzlib.with_rotated'), and on
# one of them with one of 'OTHER_OPTIONS' ('--nopt', '--versions', ...) and,
# when the model ends without a panic, without checks:
#
#   whole-mismatch  a build ends differently from the model
#   whole-reject    a generated program is rejected
#   compiler-crash, compiler-hang, invalid-asm, link-failure, image-mismatch
#                   see fuzz-compiler.py
#
# usage: fuzz-whole.py [count] [seed] [jobs]
#        fuzz-whole.py --one SEED INDEX [--noinline]
# '--one' prints the program 'INDEX' of 'SEED' and what the model gives
import os
import random
import shutil
import sys
import tempfile

import fuzzlib as fl

INTS = ("i8", "i16", "i32", "int")
RANK = {"i8": 0, "i16": 1, "i32": 2, "int": 3}
FIXED_BITS = {"i8": 8, "i16": 16, "i32": 32}


def is_int(t):
    return isinstance(t, str) and t in RANK


def bits_of(t, intw):
    return intw if t == "int" else FIXED_BITS[t]


def fits_bits(bits, v):
    return -(1 << (bits - 1)) <= v <= (1 << (bits - 1)) - 1


def wrap_bits(bits, v):
    v &= (1 << bits) - 1
    return v - (1 << bits) if v >> (bits - 1) else v


def wider(a, b):
    return a if RANK[a] >= RANK[b] else b


# types


class Arr:
    def __init__(self, elem, n):
        self.elem = elem
        self.n = n

    def __eq__(self, other):
        return isinstance(other, Arr) and self.elem == other.elem and \
            self.n == other.n

    def __hash__(self):
        return hash(("arr", self.n))

    def text(self):
        return f"{type_text(self.elem)}[{self.n}]"


class UArr:
    # an unsized array parameter, also the array field of a generic seen
    # from its methods; 'bare' writes an array of 'int' as '[]'
    def __init__(self, elem, bare=False):
        self.elem = elem
        self.bare = bare

    def __eq__(self, other):
        return isinstance(other, UArr) and self.elem == other.elem

    def __hash__(self):
        return hash(("uarr",))

    def text(self):
        if self.bare and self.elem == "int":
            return "[]"
        return f"{type_text(self.elem)}[]"


class Rec:
    # a record type: a plain type, an alias of a generic instance or the
    # proto of a generic seen from its methods
    def __init__(self, name, fields, bases=None, consts=None):
        self.name = name
        self.fields = fields
        # the names whose methods apply, the generic and the generics it is
        # based on
        self.bases = bases or [name]
        self.consts = consts or {}
        # the generic itself, seen from its methods
        self.is_proto = False

    def text(self):
        return self.name


def type_text(t):
    return t if isinstance(t, str) else t.text()


def zero_of(t):
    if is_int(t):
        return 0
    if t == "bool":
        return False
    if isinstance(t, Arr):
        return [zero_of(t.elem) for _ in range(t.n)]
    if isinstance(t, Rec):
        return {f: zero_of(ft) for f, ft in t.fields}
    raise ValueError(t)


def zero_like(v):
    if isinstance(v, bool):
        return False
    if isinstance(v, list):
        return [zero_like(x) for x in v]
    if isinstance(v, dict):
        return {k: zero_like(x) for k, x in v.items()}
    return 0


def copy_value(v):
    if isinstance(v, list):
        return [copy_value(x) for x in v]
    if isinstance(v, dict):
        return {k: copy_value(x) for k, x in v.items()}
    return v


# expressions


PREC = {"+": 1, "-": 1, "*": 2, "/": 2, "%": 2, "|": 3, "&": 4, "^": 5,
        "<<": 6, ">>": 6}


class Expr:
    const = False
    leaf = True
    unary = False

    def operand(self):
        t = self.text()
        if self.unary:
            return f"({t})"
        return t


class Lit(Expr):
    # 'radix' 16 or 2 writes a value that is not negative as '0x' or '0b'
    def __init__(self, v, radix=10):
        self.v = v
        self.const = True
        self.unary = v < 0
        self.radix = radix

    def text(self):
        if self.radix == 16 and self.v >= 0:
            return hex(self.v)
        if self.radix == 2 and self.v >= 0:
            return bin(self.v)
        return str(self.v)


class Const(Expr):
    # a 'let' constant or a constant of a generic
    def __init__(self, name):
        self.name = name
        self.const = True

    def text(self):
        return self.name


class Path:
    def __init__(self, root, steps, typ):
        self.root = root
        # ('f', name) or ('i', Expr)
        self.steps = steps
        self.typ = typ
        # the array of a path that ends in a start index, see 'maybe_start'
        self.parent_type = None

    def text(self):
        out = self.root
        for kind, x in self.steps:
            out += f".{x}" if kind == "f" else f"[{x.text()}]"
        return out


class Read(Expr):
    def __init__(self, path):
        self.path = path

    def text(self):
        return self.path.text()


class Un(Expr):
    def __init__(self, op, x):
        self.op = op
        self.x = x
        self.const = x.const
        self.unary = True

    def text(self):
        inner = self.x.text()
        if not self.x.leaf or self.x.unary:
            inner = f"({inner})"
        return f"{self.op}{inner}"


class Bin(Expr):
    leaf = False

    def __init__(self, op, lhs, rhs):
        self.op = op
        self.lhs = lhs
        self.rhs = rhs
        self.const = lhs.const and rhs.const

    def side(self, e, left):
        if e.leaf:
            return e.operand()
        p = PREC[e.op]
        if p > PREC[self.op] or (left and p == PREC[self.op]):
            return e.text()
        return f"({e.text()})"

    def text(self):
        return f"{self.side(self.lhs, True)} {self.op} " \
            f"{self.side(self.rhs, False)}"


class Conv(Expr):
    def __init__(self, t, x, inner_lead):
        self.t = t
        self.x = x
        self.inner_lead = inner_lead
        self.const = x.const

    def text(self):
        return f"{self.t}({self.x.text()})"


class BoolToInt(Expr):
    def __init__(self, b):
        self.b = b

    def text(self):
        return f"int({self.b.text()})"


class ALen(Expr):
    def __init__(self, path):
        self.path = path
        self.const = True

    def text(self):
        return f"array_length({self.path.text()})"


class Call(Expr):
    # a call of a function, of a method on 'recv' or of a constructor
    def __init__(self, fn, args, recv=None, targs=None):
        self.fn = fn
        self.args = args
        self.recv = recv
        self.targs = targs
        # the type of the result 'res T' of a generic function
        self.dest_type = None

    def text(self):
        args = ", ".join(a.text() for a in self.args)
        targs = f"<{', '.join(self.targs)}>" if self.targs else ""
        if self.fn.kind == "ctor":
            return f"{self.fn.owner_text}.{self.fn.short}({args})"
        if self.recv is not None:
            return f"{self.recv.text()}.{self.fn.short}({args})"
        return f"{self.fn.short}{targs}({args})"


# bool expressions


class BLit(Expr):
    def __init__(self, v):
        self.v = v

    def text(self):
        return "true" if self.v else "false"


class Cmp(Expr):
    leaf = False

    def __init__(self, op, lhs, rhs, lt, rt):
        self.op = op
        self.lhs = lhs
        self.rhs = rhs
        self.lt = lt
        self.rt = rt

    def text(self):
        return f"{self.lhs.text()} {self.op} {self.rhs.text()}"


class Logic(Expr):
    leaf = False

    def __init__(self, op, lhs, rhs):
        self.op = op
        self.lhs = lhs
        self.rhs = rhs

    def text(self):
        def side(e):
            return f"({e.text()})" if isinstance(e, Logic) else e.text()
        return f"{side(self.lhs)} {self.op} {side(self.rhs)}"


class Not(Expr):
    def __init__(self, x):
        self.x = x

    def text(self):
        if self.x.leaf and not isinstance(self.x, Not):
            return f"not {self.x.text()}"
        return f"not ({self.x.text()})"


class AEq(Expr):
    def __init__(self, a, b, cnt):
        self.a = a
        self.b = b
        self.cnt = cnt

    def text(self):
        return f"arrays_equal({self.a.text()}, {self.b.text()}, " \
            f"{self.cnt.text()})"


# values of records and arrays


class Lit2(Expr):
    # a record or array literal, 'named' writes the type before the braces
    def __init__(self, typ, elems, named):
        self.typ = typ
        self.elems = elems
        self.named = named

    def text(self):
        inner = ", ".join(e.text() for e in self.elems)
        head = type_text(self.typ) if self.named else ""
        if isinstance(self.typ, Arr) and self.named and \
                self.typ.elem == "int" and self.typ.n == len(self.elems):
            head = "[]"
        return f"{head}{{{inner}}}"


class Zero(Expr):
    def __init__(self, typ, named):
        self.typ = typ
        self.named = named

    def text(self):
        return type_text(self.typ) if self.named else "{}"


class Input(Expr):
    # 'read(0, path, cnt, start)', 'cnt' and 'start' None when left out
    def __init__(self, path, cnt, start):
        self.path = path
        self.cnt = cnt
        self.start = start

    def text(self):
        args = [self.path.text()] + [x.text() for x in (self.cnt, self.start)
                                     if x is not None]
        return f"read(0, {', '.join(args)})"


class StrLit(Expr):
    def __init__(self, text):
        self.text_ = text

    def text(self):
        return f'"{self.text_}"'


# statements


class Stmt:
    pass


class VarDecl(Stmt):
    def __init__(self, name, value, typ, let=False):
        self.name = name
        self.value = value
        self.typ = typ
        self.let = let


class Assign(Stmt):
    def __init__(self, path, value):
        self.path = path
        self.value = value


class If(Stmt):
    def __init__(self, branches, other):
        self.branches = branches
        self.other = other


class LoopS(Stmt):
    def __init__(self, body):
        self.body = body


class Simple(Stmt):
    # 'break', 'continue', 'return'
    def __init__(self, word):
        self.word = word


class Block(Stmt):
    def __init__(self, body):
        self.body = body


class Foo(Stmt):
    def __init__(self, path, cnt, body):
        self.path = path
        self.cnt = cnt
        self.body = body


class CallS(Stmt):
    def __init__(self, call):
        self.call = call


class Show(Stmt):
    def __init__(self, value):
        self.value = value


class WriteS(Stmt):
    # 'cnt' and 'start' None when left out
    def __init__(self, path, cnt, start):
        self.path = path
        self.cnt = cnt
        self.start = start


class Copy(Stmt):
    # 'array_copy(src, dst, cnt)', a path ending in an index starts there
    def __init__(self, src, dst, cnt):
        self.src = src
        self.dst = dst
        self.cnt = cnt


class ExitS(Stmt):
    def __init__(self, code):
        self.code = code


class Fn:
    def __init__(self, name, kind):
        self.name = name
        # 'func', 'method', 'mmethod', 'ctor', 'generic'
        self.kind = kind
        self.short = name
        self.params = []
        self.result = None
        self.body = []
        self.pure = True
        self.globals_used = set()
        self.owner = None
        self.owner_text = ""
        self.tparams = []
        self.noinline = False
        self.recursive = False
        self.mixed = False


# printing


def block_lines(stmts, indent):
    out = []
    for n, s in enumerate(stmts):
        lines = stmt_lines(s, indent)
        # note: a block on the line after a declaration that ends with a type
        # is parsed as its literal (etc/todo.txt)
        if isinstance(s, VarDecl) and isinstance(s.value, Zero) and \
                n + 1 < len(stmts) and isinstance(stmts[n + 1], Block):
            lines[-1] += "{}"
        out += lines
    return out


def stmt_lines(s, indent):
    pad = "    " * indent
    if isinstance(s, VarDecl):
        word = "let" if s.let else "var"
        return [f"{pad}{word} {s.name} = {s.value.text()}"]
    if isinstance(s, Assign):
        return [f"{pad}{s.path.text()} = {s.value.text()}"]
    if isinstance(s, If):
        out = []
        for n, (cond, body) in enumerate(s.branches):
            head = "if" if n == 0 else "} else if"
            out.append(f"{pad}{head} {cond.text()} {{")
            out += block_lines(body, indent + 1)
        if s.other is not None:
            out.append(f"{pad}}} else {{")
            out += block_lines(s.other, indent + 1)
        out.append(f"{pad}}}")
        return out
    if isinstance(s, LoopS):
        return [f"{pad}loop {{"] + block_lines(s.body, indent + 1) + \
            [f"{pad}}}"]
    if isinstance(s, Simple):
        return [f"{pad}{s.word}"]
    if isinstance(s, Block):
        return [f"{pad}{{"] + block_lines(s.body, indent + 1) + [f"{pad}}}"]
    if isinstance(s, Foo):
        cnt = f", {s.cnt.text()}" if s.cnt is not None else ""
        return [f"{pad}foo {s.path.text()}{cnt} {{"] + \
            block_lines(s.body, indent + 1) + [f"{pad}}}"]
    if isinstance(s, CallS):
        return [f"{pad}{s.call.text()}"]
    if isinstance(s, Show):
        return [f"{pad}show({s.value.text()}) # trace"]
    if isinstance(s, WriteS):
        args = [s.path.text()] + [x.text() for x in (s.cnt, s.start)
                                  if x is not None]
        return [f"{pad}write(1, {', '.join(args)})"]
    if isinstance(s, Copy):
        return [f"{pad}array_copy({s.src.text()}, {s.dst.text()}, "
                f"{s.cnt.text()})"]
    if isinstance(s, ExitS):
        return [f"{pad}exit({s.code})"]
    raise ValueError(s)


SHOW = """func noinline show(v) {
    var buf = i8[24]
    var m = v
    var neg = false
    if m < 0 {
        neg = true
    }
    if m > 0 {
        m = -m
    }
    var k = 23
    buf[k] = '\\n'
    loop {
        k = k - 1
        buf[k] = i8('0' - m % 10)
        m = m / 10
        if m == 0 break
    }
    if neg {
        k = k - 1
        buf[k] = '-'
    }
    write(1, buf, 24 - k, k)
}
"""


# the model


class Panic(Exception):
    def __init__(self, name):
        self.name = name


class ExitProgram(Exception):
    def __init__(self, code):
        self.code = code


class BreakEx(Exception):
    pass


class ContinueEx(Exception):
    pass


class ReturnEx(Exception):
    pass


class TooLong(Exception):
    pass


class Ref:
    def __init__(self, container, key):
        self.c = container
        self.k = key

    def get(self):
        return self.c[self.k]

    def set(self, v):
        self.c[self.k] = v


def box(v):
    return Ref([v], 0)


class Frame:
    def __init__(self, consts=None):
        self.scopes = [{}]
        self.consts = consts or {}

    def lookup(self, name):
        for s in reversed(self.scopes):
            if name in s:
                return s[name]
        return None


class Model:
    def __init__(self, program, intw):
        self.p = program
        self.intw = intw
        self.out = bytearray()
        self.steps = 0
        self.globals = {}
        # what the reads have not taken of the input
        self.input = program.stdin

    def w(self, t):
        return bits_of(t, self.intw)

    def tick(self):
        self.steps += 1
        if self.steps > 200000:
            raise TooLong()

    def run(self):
        try:
            for name, (t, value) in self.p.dats.items():
                self.globals[name] = box(self.const_value(value, t))
            self.call_body(self.p.main, Frame())
            return ("exit", 0, bytes(self.out))
        except Panic as p:
            return ("panic", fl.PANIC_CODE[p.name], bytes(self.out))
        except ExitProgram as e:
            return ("exit", e.code & 255, bytes(self.out))

    def const_value(self, value, t):
        return self.value_of(value, t, Frame())

    # names

    def ref_of_name(self, name, frame):
        r = frame.lookup(name)
        if r is not None:
            return r
        return self.globals[name]

    def resolve(self, path, frame):
        r = self.ref_of_name(path.root, frame)
        for kind, x in path.steps:
            v = r.get()
            if kind == "f":
                r = Ref(v, x)
            else:
                i = self.ev(x, "int", True, frame)
                if i < 0 or i >= len(v):
                    raise Panic("bounds")
                r = Ref(v, i)
        return r

    # integer expressions: 'ev' computes at the width of 't'

    def ev(self, e, t, checked, frame):
        bits = self.w(t)
        if isinstance(e, Lit):
            return e.v
        if isinstance(e, Const):
            local = frame.lookup(e.name)
            if local is not None and e.name != "n":
                return local.get()
            if e.name == "n":
                return len(frame.lookup("e").c)
            if e.name in frame.consts:
                return frame.consts[e.name]
            return self.p.consts[e.name]
        if isinstance(e, Read):
            return self.resolve(e.path, frame).get()
        if isinstance(e, Un):
            x = self.ev(e.x, t, checked, frame)
            if e.op == "~":
                return ~x
            r = -x
            if not fits_bits(bits, r):
                if checked:
                    raise Panic("overflow")
                r = wrap_bits(bits, r)
            return r
        if isinstance(e, Bin):
            a = self.ev(e.lhs, t, checked, frame)
            b = self.ev(e.rhs, t, checked, frame)
            return self.binary(e.op, a, b, bits, checked)
        if isinstance(e, Conv):
            inner = wider(e.t, e.inner_lead)
            v = self.ev(e.x, inner, False, frame)
            return wrap_bits(self.w(e.t), v)
        if isinstance(e, BoolToInt):
            return 1 if self.evb(e.b, frame) else 0
        if isinstance(e, ALen):
            if isinstance(e.path.typ, Arr):
                return e.path.typ.n
            return len(self.resolve(e.path, frame).get())
        if isinstance(e, Call):
            return self.call(e, frame)
        if isinstance(e, Input):
            return self.read_input(e, frame)
        raise ValueError(e)

    def read_input(self, e, frame):
        # note: a read on a uart returns one line, on a file as many bytes as
        #       fit; the input is one line that fits in the first read
        arr = self.resolve(e.path, frame).get()
        cnt = len(arr) if e.cnt is None else self.ev(e.cnt, "int", True, frame)
        start = 0 if e.start is None else self.ev(e.start, "int", True, frame)
        if cnt < 0 or start < 0 or start + cnt > len(arr):
            raise Panic("bounds")
        got = self.input
        assert len(got) <= cnt
        self.input = b""
        arr[start:start + len(got)] = list(got)
        return len(got)

    def binary(self, op, a, b, bits, checked):
        if op in "+-*":
            r = a + b if op == "+" else a - b if op == "-" else a * b
            if not fits_bits(bits, r):
                if checked:
                    raise Panic("overflow")
                r = wrap_bits(bits, r)
            return r
        if op in "/%":
            if b == 0 or (a == -(1 << (bits - 1)) and b == -1):
                raise Panic("division")
            q = fl.trunc_div(a, b)
            return q if op == "/" else a - b * q
        if op == "&":
            return a & b
        if op == "|":
            return a | b
        if op == "^":
            return a ^ b
        if b < 0 or b >= bits:
            raise Panic("shift")
        if op == "<<":
            return wrap_bits(bits, a << b)
        return a >> b

    def evb(self, e, frame):
        if isinstance(e, BLit):
            return e.v
        if isinstance(e, Read):
            return self.resolve(e.path, frame).get()
        if isinstance(e, Cmp):
            if e.lt is None:
                a = self.value_of(e.lhs, None, frame)
                b = self.value_of(e.rhs, None, frame)
            else:
                a = self.ev(e.lhs, e.lt, True, frame)
                b = self.ev(e.rhs, e.rt, True, frame)
            if e.op == "==":
                return a == b
            if e.op == "!=":
                return a != b
            return {"<": a < b, "<=": a <= b, ">": a > b, ">=": a >= b}[e.op]
        if isinstance(e, Logic):
            a = self.evb(e.lhs, frame)
            if e.op == "and" and not a:
                return False
            if e.op == "or" and a:
                return True
            return self.evb(e.rhs, frame)
        if isinstance(e, Not):
            return not self.evb(e.x, frame)
        if isinstance(e, AEq):
            a, ai = self.start_of(e.a, frame)
            b, bi = self.start_of(e.b, frame)
            n = self.ev(e.cnt, "int", True, frame)
            if n < 0 or ai + n > len(a) or bi + n > len(b):
                raise Panic("bounds")
            return a[ai:ai + n] == b[bi:bi + n]
        if isinstance(e, Call):
            return self.call(e, frame)
        raise ValueError(e)

    def start_of(self, path, frame):
        # an array path, or a path ending in an index: the array and the start
        if path.steps and path.steps[-1][0] == "i":
            r = self.resolve(path, frame)
            return r.c, r.k
        return self.resolve(path, frame).get(), 0

    # any value: 't' is the type of the destination

    def value_of(self, e, t, frame):
        if isinstance(e, Lit2):
            typ = e.typ
            v = zero_of(typ)
            if isinstance(typ, Arr):
                for n, x in enumerate(e.elems):
                    v[n] = self.value_of(x, typ.elem, frame)
            else:
                for (f, ft), x in zip(typ.fields, e.elems):
                    v[f] = self.value_of(x, ft, frame)
            return v
        if isinstance(e, Zero):
            return zero_of(e.typ)
        if isinstance(e, StrLit):
            return [ord(ch) for ch in e.text_.replace("\\n", "\n")]
        if isinstance(e, Read):
            return copy_value(self.resolve(e.path, frame).get())
        if isinstance(e, Call):
            return copy_value(self.call(e, frame))
        if t == "bool" or isinstance(e, (BLit, Cmp, Logic, Not, AEq)):
            return self.evb(e, frame)
        return self.ev(e, t, True, frame)

    # calls

    def call(self, c, frame):
        self.tick()
        fn = c.fn
        callee = Frame()
        if c.recv is not None:
            callee.consts = dict(c.recv.typ.consts or frame.consts)
            callee.scopes[0]["self"] = self.resolve(c.recv, frame)
        for (name, ptype, mut), a in zip(fn.params, c.args):
            if isinstance(a, Read) and not is_int(ptype) or \
                    isinstance(a, Read) and mut:
                callee.scopes[0][name] = self.resolve(a.path, frame)
                if ptype == "T":
                    callee.consts = dict(a.path.typ.consts or frame.consts)
            elif is_int(ptype):
                callee.scopes[0][name] = box(self.ev(a, ptype, True, frame))
            else:
                callee.scopes[0][name] = box(self.value_of(a, ptype, frame))
        result = None
        if fn.kind == "ctor":
            result = box(zero_of(fn.owner))
            callee.scopes[0]["self"] = result
        elif fn.result is not None:
            rt = fn.result
            if rt == "T":
                rt = c.dest_type
                callee.consts = dict(rt.consts)
            result = box(zero_of(rt))
            callee.scopes[0]["res"] = result
        self.call_body(fn, callee)
        return result.get() if result is not None else None

    def call_body(self, fn, frame):
        try:
            self.block(fn.body, frame)
        except ReturnEx:
            pass

    # statements

    def block(self, stmts, frame):
        frame.scopes.append({})
        try:
            for s in stmts:
                self.stmt(s, frame)
        finally:
            frame.scopes.pop()

    def stmt(self, s, frame):
        self.tick()
        if isinstance(s, VarDecl):
            v = self.value_of(s.value, s.typ, frame)
            frame.scopes[-1][s.name] = box(v)
        elif isinstance(s, Assign):
            r = self.resolve(s.path, frame)
            if isinstance(s.value, Zero):
                r.set(zero_like(r.get()))
                return
            r.set(self.value_of(s.value, s.path.typ, frame))
        elif isinstance(s, If):
            for cond, body in s.branches:
                if self.evb(cond, frame):
                    self.block(body, frame)
                    return
            if s.other is not None:
                self.block(s.other, frame)
        elif isinstance(s, LoopS):
            while True:
                self.tick()
                try:
                    self.block(s.body, frame)
                except BreakEx:
                    break
                except ContinueEx:
                    pass
        elif isinstance(s, Simple):
            raise {"break": BreakEx, "continue": ContinueEx,
                   "return": ReturnEx}[s.word]()
        elif isinstance(s, Block):
            self.block(s.body, frame)
        elif isinstance(s, Foo):
            arr = self.resolve(s.path, frame).get()
            cnt = len(arr)
            if s.cnt is not None:
                cnt = self.ev(s.cnt, "int", True, frame)
                if cnt < 0 or cnt > len(arr):
                    raise Panic("bounds")
            for k in range(cnt):
                self.tick()
                frame.scopes.append({"e": Ref(arr, k), "i": box(k)})
                try:
                    self.block(s.body, frame)
                except ContinueEx:
                    pass
                except BreakEx:
                    frame.scopes.pop()
                    break
                frame.scopes.pop()
        elif isinstance(s, CallS):
            self.call(s.call, frame)
        elif isinstance(s, Show):
            v = self.ev(s.value, "int", True, frame)
            self.out += f"{v}\n".encode()
        elif isinstance(s, WriteS):
            arr = self.resolve(s.path, frame).get()
            cnt = len(arr) if s.cnt is None else \
                self.ev(s.cnt, "int", True, frame)
            start = 0 if s.start is None else \
                self.ev(s.start, "int", True, frame)
            if cnt < 0 or start < 0 or start + cnt > len(arr):
                raise Panic("bounds")
            self.out += bytes(x & 255 for x in arr[start:start + cnt])
        elif isinstance(s, Copy):
            src, si = self.start_of(s.src, frame)
            dst, di = self.start_of(s.dst, frame)
            n = self.ev(s.cnt, "int", True, frame)
            if n < 0 or si + n > len(src) or di + n > len(dst):
                raise Panic("bounds")
            if src is dst and si < di < si + n:
                raise Panic("overlap")
            part = copy_value(src[si:si + n])
            dst[di:di + n] = part
        elif isinstance(s, ExitS):
            raise ExitProgram(s.code)
        else:
            raise ValueError(s)


# the generator


class Var:
    const_value = None

    def __init__(self, name, typ, writable, is_global=False):
        self.name = name
        self.typ = typ
        self.writable = writable
        self.is_global = is_global


class Program:
    def __init__(self):
        self.consts = {}
        self.dats = {}
        self.functions = []
        self.main = None
        self.head = []
        # the input, one line
        self.stdin = b""


class Ctx:
    # where a statement is generated
    def __init__(self, fn):
        self.fn = fn
        self.scopes = [{}]
        self.loops = 0
        # roots that the body must not name: the array of a 'foo', loop
        # counters are read-only
        self.hidden = set()
        self.readonly = set()
        self.in_foo_global = False
        self.consts = {}
        # the length of the array of the innermost 'foo', None when unknown
        self.foo_n = None

    def lookup(self, name):
        for s in reversed(self.scopes):
            if name in s:
                return s[name]
        return None

    def visible(self):
        seen = {}
        for s in self.scopes:
            seen.update(s)
        return [v for v in seen.values() if v.name not in self.hidden]


class Gen:
    def __init__(self, rnd):
        self.rnd = rnd
        self.p = Program()
        self.names = 0
        self.records = []
        self.generic_bases = []
        self.globals = {}
        self.in_index = 0
        self.const_args = rnd.random() < 0.5
        self.reads = 0
        # note: a uart gives the end of transmission to a read of 'fuzzlib'
        #       at the end of the input, a few reads do not use them up
        self.p.stdin = ("".join(rnd.choice("abcxyz") for _ in
                                range(rnd.randint(0, 6))) + "\n").encode()

    def name(self, prefix):
        self.names += 1
        return f"{prefix}{self.names}"

    def chance(self, p):
        return self.rnd.random() < p

    # types

    def gen_types(self):
        rnd = self.rnd
        for _ in range(rnd.randint(1, 4)):
            fields = []
            for k in range(rnd.randint(1, 4)):
                fname = f"f{k}"
                r = rnd.random()
                if r < 0.45:
                    ft = rnd.choice(INTS)
                elif r < 0.55:
                    ft = "bool"
                elif r < 0.8:
                    ft = Arr(rnd.choice(INTS), rnd.randint(1, 5))
                    if self.chance(0.1):
                        # note: offsets past the 12 bits of an rv32i
                        # immediate
                        ft = Arr(rnd.choice(INTS), rnd.randint(300, 1100))
                elif self.records:
                    sub = rnd.choice(self.records)
                    ft = sub if self.chance(0.6) else Arr(sub,
                                                          rnd.randint(1, 3))
                else:
                    ft = rnd.choice(INTS)
                fields.append((fname, ft))
            rec = Rec(self.name("r"), fields)
            self.records.append(rec)
            body = ", ".join(f"{f} {type_text(t)}" for f, t in fields)
            self.p.head.append(f"type {rec.name} {{ {body} }}")

    def gen_generics(self):
        rnd = self.rnd
        # 'type gN<c, A type> { a A, b i16[c], k i8 }' or with the type
        # parameter first, instances by aliases
        base = self.name("g")
        type_first = self.chance(0.5)

        def args(c, a):
            return f"<{a}, {c}>" if type_first else f"<{c}, {a}>"
        self.p.head.append(f"type {base}{args('c', 'A type')} "
                           "{ a A, b i16[c], k i8 }")
        self.generic_bases.append(base)
        derived = None
        if self.chance(0.5):
            derived = self.name("gd")
            self.p.head.append(f"type {derived}<c> = {base}{args('c', 'i8')}")
        aliases = []
        for _ in range(rnd.randint(1, 3)):
            c = rnd.randint(2, 5)
            name = self.name("ga")
            if derived and self.chance(0.4):
                a_t = "i8"
                self.p.head.append(f"type {name} = {derived}<{c}>")
                bases = [derived, base]
            else:
                # note: a type argument may be an instance of the same generic
                a_t = rnd.choice(list(INTS) + self.records[:1] + aliases)
                self.p.head.append(
                    f"type {name} = {base}{args(c, type_text(a_t))}")
                bases = [base]
            rec = Rec(name, [("a", a_t), ("b", Arr("i16", c)), ("k", "i8")],
                      bases, {"c": c})
            aliases.append(rec)
            self.records.append(rec)
        proto = Rec(base, [("b", UArr("i16")), ("k", "i8")], [base])
        proto.is_proto = True
        return base, proto

    def gen_globals(self):
        rnd = self.rnd
        if self.chance(0.4):
            name = self.name("G")
            t = Arr(rnd.choice(INTS), rnd.randint(600, 5000))
            self.p.dats[name] = (t, Zero(t, True))
            self.globals[name] = Var(name, t, True, True)
            self.p.head.append(f"dat {name} = {type_text(t)}")
        for _ in range(rnd.randint(0, 3)):
            name = self.name("K")
            v = rnd.randint(-50, 50)
            self.p.consts[name] = v
            self.p.head.append(f"let {name} = {v}")
        for _ in range(rnd.randint(1, 4)):
            name = self.name("G")
            r = rnd.random()
            if r < 0.3:
                t = rnd.choice(INTS)
                v = self.const_lit(t)
                value = Conv(t, v, "int") if t != "int" else v
                self.p.dats[name] = (t, value)
            elif r < 0.6:
                t = Arr(rnd.choice(INTS), rnd.randint(1, 5))
                elems = [self.const_lit(t.elem)
                         for _ in range(rnd.randint(0, t.n))]
                self.p.dats[name] = (t, Lit2(t, elems, True))
            elif r < 0.7:
                text = "".join(rnd.choice("abcxyz ") for _ in
                               range(rnd.randint(1, 6))) + "\\n"
                t = Arr("i8", len(text) - 1)
                self.p.dats[name] = (t, StrLit(text))
            else:
                t = rnd.choice(self.records)
                self.p.dats[name] = (t, self.const_record(t))
            self.globals[name] = Var(name, t, True, True)
            self.p.head.append(f"dat {name} = "
                               f"{self.p.dats[name][1].text()}")

    def const_lit(self, t):
        bits = 8 if t == "i8" else 16 if t == "i16" else 32
        hi = (1 << (bits - 1)) - 1
        v = self.rnd.choice([0, 1, -1, 2, 5, 100, -100, hi, -hi,
                             self.rnd.randint(-hi, hi)])
        if t == "i8":
            v = max(-127, min(127, v))
        return Lit(v)

    def const_record(self, t, named=True):
        elems = []
        for _, ft in t.fields[:self.rnd.randint(0, len(t.fields))]:
            elems.append(self.const_of(ft))
        return Lit2(t, elems, named)

    def const_of(self, ft):
        if is_int(ft):
            return self.const_lit(ft)
        if ft == "bool":
            return BLit(self.chance(0.5))
        if isinstance(ft, Arr):
            return Lit2(ft, [self.const_of(ft.elem) for _ in
                             range(self.rnd.randint(0, min(ft.n, 6)))], False)
        return self.const_record(ft, False)

    # paths

    def reach(self, t, pred, depth=3):
        if pred(t):
            return True
        if depth == 0:
            return False
        if isinstance(t, (Arr, UArr)):
            return self.reach(t.elem, pred, depth - 1)
        if isinstance(t, Rec):
            return any(self.reach(ft, pred, depth - 1) for _, ft in t.fields)
        return False

    def path(self, ctx, pred, writable=False, avoid=(), index_ok=True):
        cands = [v for v in ctx.visible() + list(self.globals.values())
                 if v.const_value is None
                 and (not writable or (v.writable and
                                       v.name not in ctx.readonly))
                 and v.name not in avoid and v.name not in ctx.hidden
                 and self.global_ok(ctx, v, writable)
                 and self.reach(v.typ, pred)]
        if not cands:
            return None
        for _ in range(8):
            v = self.rnd.choice(cands)
            p = self.walk(ctx, v, pred, index_ok)
            if p is not None:
                return p
        return None

    def global_ok(self, ctx, v, writable):
        if not v.is_global:
            return True
        if ctx.lookup(v.name) is not None:
            return False
        if ctx.fn is not None and ctx.fn.pure and writable:
            return False
        return True

    def walk(self, ctx, v, pred, index_ok):
        index_ok = index_ok and self.in_index == 0
        t = v.typ
        steps = []
        for _ in range(4):
            if pred(t) and (self.chance(0.6) or
                            not self.reach_deeper(t, pred)):
                return Path(v.name, steps, t)
            if isinstance(t, (Arr, UArr)):
                if not self.reach(t.elem, pred, 2):
                    return None
                idx = self.index(ctx, t, Path(v.name, list(steps), t),
                                 index_ok)
                if idx is None:
                    return None
                steps.append(("i", idx))
                t = t.elem
            elif isinstance(t, Rec):
                fs = [(f, ft) for f, ft in t.fields
                      if self.reach(ft, pred, 2)]
                if not fs:
                    return None
                f, ft = self.rnd.choice(fs)
                steps.append(("f", f))
                t = ft
            else:
                return None
        if pred(t):
            return Path(v.name, steps, t)
        return None

    def reach_deeper(self, t, pred):
        if isinstance(t, (Arr, UArr)):
            return self.reach(t.elem, pred, 2)
        if isinstance(t, Rec):
            return any(self.reach(ft, pred, 2) for _, ft in t.fields)
        return False

    def index(self, ctx, t, array_path, index_ok):
        rnd = self.rnd
        if index_ok and ctx.foo_n is not None and isinstance(t, Arr) and \
                t.n >= ctx.foo_n and rnd.random() < 0.3:
            # note: the 'i' of a 'foo' is below the length of its array
            k = rnd.randint(0, t.n - ctx.foo_n)
            return Bin("+", Read(Path("i", [], "int")), Lit(k))
        if isinstance(t, Arr) and (not index_ok or rnd.random() < 0.6):
            return Lit(rnd.randrange(t.n))
        if not index_ok:
            return None
        self.in_index += 1
        x = self.int_expr(ctx, "int", False, 0, nonconst=True, calls=False)
        self.in_index -= 1
        if x is None:
            return Lit(rnd.randrange(t.n)) if isinstance(t, Arr) else None
        if isinstance(t, Arr):
            if rnd.random() < 0.6:
                mask = 1
                while mask * 2 <= t.n:
                    mask *= 2
                return Bin("&", x, Lit(mask - 1))
            if rnd.random() < 0.9:
                x = Bin("&", x, Lit(0x7fff))
            return Bin("%", x, Lit(t.n))
        if rnd.random() < 0.9:
            x = Bin("&", x, Lit(0x7fff))
        return Bin("%", x, ALen(array_path))

    # integer expressions of type 't'

    def int_expr(self, ctx, t, lead, depth, nonconst=False, calls=True):
        for _ in range(10):
            e = self.int_expr_once(ctx, t, lead, depth, nonconst, calls)
            if e is not None and not (nonconst and e.const):
                return e
        return None

    def int_type_le(self, t, lead):
        if lead:
            return t
        return self.rnd.choice([u for u in INTS if RANK[u] <= RANK[t]])

    def lit_for(self, t):
        rnd = self.rnd
        hi = {"i8": 100, "i16": 3000, "i32": 100000, "int": 100000}[t]
        v = rnd.choice([0, 1, 2, 3, -1, 7, rnd.randint(-hi, hi),
                        rnd.randint(0, 20)])
        r = rnd.random()
        return Lit(v, radix=16 if r < 0.1 else 2 if r < 0.17 else 10)

    # the kinds of integer expression and their weights, inner nodes first
    EXPRS = (("bin", 40), ("un", 8), ("conv", 10), ("call", 14),
             ("bool", 3), ("lit", 20), ("path", 50))

    def int_expr_once(self, ctx, t, lead, depth, nonconst, calls):
        rnd = self.rnd
        kinds = [(k, w) for k, w in self.EXPRS if depth > 0 or k in
                 ("bool", "lit", "path")]
        kind = rnd.choices([k for k, _ in kinds], [w for _, w in kinds])[0]
        if kind == "bin":
            ops = ["+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>"]
            op = rnd.choice(ops + ["+", "-", "+", "&"])
            lhs = self.int_expr(ctx, t, lead, depth - 1, calls=calls)
            if lhs is None:
                return None
            bits = 8 if t == "i8" else 16 if t == "i16" else 32
            if op in ("<<", ">>") and rnd.random() < 0.7:
                rhs = Lit(rnd.randrange(bits))
            elif op in ("<<", ">>") and rnd.random() < 0.8:
                x = self.int_expr(ctx, t, False, 0, nonconst=True,
                                  calls=False)
                if x is None:
                    return None
                rhs = Bin("&", x, Lit(bits - 1))
            elif op in ("/", "%") and rnd.random() < 0.6:
                rhs = Lit(rnd.choice([1, 2, 3, 7, -1, -2, 10]))
            else:
                rhs = self.int_expr(ctx, t, False, depth - 1,
                                    nonconst=lhs.const or op in
                                    ("/", "%", "<<", ">>"), calls=calls)
            if rhs is None or (lhs.const and rhs.const):
                return None
            if op in ("/", "%") and not rhs.const and rnd.random() < 0.95:
                # note: an odd divisor is not zero
                rhs = Bin("|", rhs, Lit(1))
            return Bin(op, lhs, rhs)
        if kind == "un":
            x = self.int_expr(ctx, t, lead, depth - 1, calls=calls)
            if x is None or isinstance(x, Un) or (isinstance(x, Lit) and
                                                  x.v < 0):
                return None
            return Un(rnd.choice("-~"), x)
        if kind == "conv":
            ct = self.int_type_le(t, lead)
            inner_lead = rnd.choice(INTS)
            x = self.int_expr(ctx, inner_lead, True, depth - 1, calls=calls)
            if x is None:
                return None
            return Conv(ct, x, inner_lead)
        if kind == "call":
            if not calls:
                return None
            ct = self.int_type_le(t, lead)
            return self.call_expr(ctx, ct, depth)
        if kind == "bool":
            if t != "int":
                return None
            if rnd.random() < 0.5:
                b = self.path(ctx, lambda u: u == "bool")
                if b is not None:
                    return BoolToInt(Read(b))
            else:
                a = self.path(ctx, lambda u: isinstance(u, (Arr, UArr)),
                              index_ok=False)
                if a is not None and not nonconst:
                    return ALen(a)
            return None
        if kind == "lit":
            if (lead and t != "int") or nonconst:
                return None
            local = {v.name: v.const_value for v in ctx.visible()
                     if v.const_value is not None}
            consts = list(self.p.consts) + list(ctx.consts) + list(local)
            if consts and rnd.random() < 0.3:
                name = rnd.choice(consts)
                v = local.get(name, self.p.consts.get(
                    name, ctx.consts.get(name)))
                # note: the 'n' of a 'foo' over an unsized array is an
                #       'int' variable, not a constant
                if (name != "n" or t == "int") and \
                        (t != "i8" or -100 <= v <= 100):
                    return Const(name)
            return self.lit_for(t)
        want = t
        if lead:
            p = self.path(ctx, lambda u: u == want)
        else:
            p = self.path(ctx, lambda u: is_int(u) and RANK[u] <= RANK[want])
        if p is None:
            return None
        return Read(p)

    # bool expressions

    def bool_expr(self, ctx, depth):
        rnd = self.rnd
        r = rnd.random()
        if depth > 0 and r < 0.2:
            op = rnd.choice(["and", "or"])
            a = self.bool_expr(ctx, depth - 1)
            b = self.bool_expr(ctx, depth - 1)
            return Logic(op, a, b)
        if depth > 0 and r < 0.28:
            return Not(self.bool_expr(ctx, depth - 1))
        if r < 0.38:
            p = self.path(ctx, lambda u: u == "bool")
            if p is not None:
                return Read(p)
        if r < 0.45 and depth > 0:
            c = self.call_expr(ctx, "bool", depth)
            if c is not None:
                return c
        if r < 0.5:
            e = self.aeq(ctx)
            if e is not None:
                return e
        if r < 0.55:
            p = self.path(ctx, lambda u: isinstance(u, (Rec, Arr)) and
                          not isinstance(u, UArr))
            if p is not None:
                q = self.path(ctx, lambda u: u is p.typ or u == p.typ
                              if isinstance(u, Arr) else u is p.typ)
                if q is not None:
                    return Cmp(rnd.choice(["==", "!="]), Read(p), Read(q),
                               None, None)
        lt = rnd.choice(INTS)
        lhs = self.int_expr(ctx, lt, True, min(depth, 2), nonconst=True)
        if lhs is None:
            return BLit(rnd.random() < 0.5) if depth == 0 else \
                self.bool_expr(ctx, 0)
        if rnd.random() < 0.4:
            rhs = self.lit_for(lt)
            rt = lt
        else:
            rt = self.int_type_le(lt, False)
            rhs = self.int_expr(ctx, rt, True, min(depth, 2), nonconst=True)
            if rhs is None:
                rhs = self.lit_for(lt)
                rt = lt
        return Cmp(rnd.choice(["==", "!=", "<", "<=", ">", ">="]), lhs, rhs,
                   lt, rt)

    def aeq(self, ctx):
        a = self.path(ctx, lambda u: isinstance(u, (Arr, UArr)) and
                      is_int(u.elem))
        if a is None:
            return None
        b = self.path(ctx, lambda u: isinstance(u, (Arr, UArr)) and
                      u.elem == a.typ.elem)
        if b is None:
            return None
        a = self.maybe_start(a)
        b = self.maybe_start(b)
        cnt = self.count_expr(ctx, self.room(a), self.room(b))
        return AEq(a, b, cnt)

    def count_expr(self, ctx, *types):
        n = min(min(t.n, 8) if isinstance(t, Arr) else 1 for t in types)
        if self.chance(0.7):
            return Lit(self.rnd.randint(0, n))
        x = self.int_expr(ctx, "int", True, 0, nonconst=True, calls=False)
        if x is None:
            return Lit(0)
        if self.chance(0.8):
            return Bin("%", Bin("&", x, Lit(0x7fff)), Lit(n + 1))
        return Bin("&", x, Lit(3))

    # calls

    def callable_fns(self, ctx, rt):
        out = []
        for fn in self.p.functions:
            if fn.kind == "ctor":
                continue
            if ctx.fn is fn and not fn.recursive:
                continue
            if fn.result is None and rt is not None:
                continue
            if rt is not None and fn.result != rt and not (
                    rt == "T" and fn.result == "T"):
                continue
            if ctx.fn is not None and ctx.fn.pure and not fn.pure:
                continue
            out.append(fn)
        return out

    def call_expr(self, ctx, rt, depth):
        fns = [f for f in self.callable_fns(ctx, rt) if f.pure]
        if not fns:
            return None
        fn = self.rnd.choice(fns)
        return self.make_call(ctx, fn, depth)

    def make_call(self, ctx, fn, depth, avoid=()):
        # avoid: roots the arguments must not name (a destination)
        if ctx.in_foo_global and fn.globals_used:
            return None
        if ctx.fn is fn:
            return self.recursive_call(ctx, fn)
        recv = None
        blocked = set(avoid) | set(fn.globals_used)
        targs = None
        if fn.kind in ("method", "mmethod"):
            recv = self.path(ctx, lambda u: isinstance(u, Rec) and
                             fn.owner_base in u.bases,
                             writable=fn.kind == "mmethod", avoid=blocked)
            if recv is None:
                return None
            if fn.kind == "mmethod":
                blocked.add(recv.root)
        args = []
        roots = [] if recv is None else [recv.root]
        for name, ptype, mut in fn.params:
            if ptype == "T":
                p = self.path(ctx, lambda u: isinstance(u, Rec) and
                              fn.owner_base in u.bases, writable=mut,
                              avoid=blocked)
                if p is None:
                    return None
                if self.chance(0.5) and not p.typ.is_proto:
                    targs = [p.typ.name]
                a = Read(p)
            else:
                a = self.arg(ctx, ptype, mut, blocked, depth - 1)
            if a is None:
                return None
            if isinstance(a, Read):
                if mut:
                    if a.path.root in roots:
                        return None
                    blocked.add(a.path.root)
                roots.append(a.path.root)
            args.append(a)
        # a 'mut' argument shares no root with another argument
        for a, (name, ptype, mut) in zip(args, fn.params):
            if mut and roots.count(a.path.root) > 1:
                return None
        if recv is not None and fn.kind == "mmethod" and \
                roots.count(recv.root) > 1:
            return None
        return Call(fn, args, recv, targs)

    def recursive_call(self, ctx, fn):
        # 'rec(d - 1, x)': the depth parameter ends the recursion
        d = Bin("-", Read(Path("d", [], "i8")), Lit(1))
        x = self.arg(ctx, "int", False, set(), 0)
        if x is None:
            x = Lit(1)
        return Call(fn, [d, x])

    def arg(self, ctx, ptype, mut, blocked, depth):
        if mut:
            p = self.path(ctx, lambda u: u == ptype if not isinstance(
                ptype, UArr) else isinstance(u, Arr) and u.elem == ptype.elem,
                writable=True, avoid=blocked, index_ok=True)
            if p is None:
                return None
            return Read(p)
        if is_int(ptype):
            if self.const_args and self.chance(0.2):
                # a constant argument folds the inlined body
                return self.lit_for(ptype)
            if self.chance(0.3):
                p = self.path(ctx, lambda u: u == ptype, avoid=blocked)
                if p is not None:
                    return Read(p)
            e = self.int_expr(ctx, ptype, False, max(0, min(depth, 2)),
                              nonconst=True, calls=depth > 0)
            if e is None:
                return None
            if e.leaf and not self.leaf_type_is(e, ptype):
                return None
            if self.calls_in(e) & blocked:
                return None
            if self.roots_in(e) & blocked:
                return None
            return e
        if ptype == "bool":
            e = self.bool_expr(ctx, max(depth, 0))
            # note: a constant argument folds the inlined body
            if self.has_blit(e):
                return None
            if self.roots_in(e) & blocked:
                return None
            return e
        if isinstance(ptype, UArr):
            p = self.path(ctx, lambda u: isinstance(u, Arr) and
                          u.elem == ptype.elem, avoid=blocked)
            return Read(p) if p is not None else None
        p = self.path(ctx, lambda u: u is ptype, avoid=blocked)
        if p is not None and self.chance(0.7):
            return Read(p)
        if isinstance(ptype, Rec):
            return self.record_value(ctx, ptype, blocked, named=False)
        return None

    def leaf_type_is(self, e, t):
        if isinstance(e, Read):
            return e.path.typ == t
        if isinstance(e, Conv):
            return e.t == t
        if isinstance(e, Call):
            return e.fn.result == t
        if isinstance(e, (BoolToInt, ALen)):
            return t == "int"
        return True

    def roots_in(self, e):
        out = set()

        def visit(x):
            if isinstance(x, Read):
                out.add(x.path.root)
                for kind, i in x.path.steps:
                    if kind == "i":
                        visit(i)
            elif isinstance(x, ALen):
                out.add(x.path.root)
            elif isinstance(x, (Un, Not)):
                visit(x.x)
            elif isinstance(x, (Bin, Cmp, Logic)):
                visit(x.lhs)
                visit(x.rhs)
            elif isinstance(x, Conv):
                visit(x.x)
            elif isinstance(x, BoolToInt):
                visit(x.b)
            elif isinstance(x, Call):
                if x.recv is not None:
                    visit(Read(x.recv))
                for a in x.args:
                    visit(a)
                out.update(x.fn.globals_used)
            elif isinstance(x, AEq):
                visit(Read(x.a))
                visit(Read(x.b))
                visit(x.cnt)
            elif isinstance(x, Lit2):
                for a in x.elems:
                    visit(a)
        visit(e)
        return out

    def calls_in(self, e):
        # the globals that the calls in 'e' use
        out = set()

        def visit(x):
            if isinstance(x, Call):
                out.update(x.fn.globals_used)
                for a in x.args:
                    visit(a)
            elif isinstance(x, (Un, Not, Conv)):
                visit(x.x)
            elif isinstance(x, (Bin, Cmp, Logic)):
                visit(x.lhs)
                visit(x.rhs)
        visit(e)
        return out

    def has_call(self, e):
        found = []

        def visit(x):
            if isinstance(x, Call):
                found.append(x)
            elif isinstance(x, (Un, Not, Conv)):
                visit(x.x)
            elif isinstance(x, (Bin, Cmp, Logic)):
                visit(x.lhs)
                visit(x.rhs)
            elif isinstance(x, BoolToInt):
                visit(x.b)
            elif isinstance(x, Lit2):
                for a in x.elems:
                    visit(a)
            elif isinstance(x, AEq):
                visit(x.cnt)
        visit(e)
        return found

    def record_value(self, ctx, t, blocked, named):
        elems = []
        for _, ft in t.fields[:self.rnd.randint(0, len(t.fields))]:
            v = self.value_for(ctx, ft, blocked, 1)
            if v is None:
                return None
            elems.append(v)
        return Lit2(t, elems, named)

    def value_for(self, ctx, ft, blocked, depth):
        # a value for a field, an element or a destination of type 'ft'
        if is_int(ft):
            e = self.int_expr(ctx, ft, False, depth)
            if e is None or (e.leaf and not self.leaf_type_is(e, ft)) or \
                    self.roots_in(e) & blocked:
                return self.lit_for(ft)
            return e
        if ft == "bool":
            e = self.bool_expr(ctx, depth)
            return e if not self.roots_in(e) & blocked else BLit(True)
        if isinstance(ft, Arr):
            n = self.rnd.randint(0, min(ft.n, 6))
            elems = []
            for _ in range(n):
                elems.append(self.value_for(ctx, ft.elem, blocked, 0))
            return Lit2(ft, elems, False)
        v = self.record_value(ctx, ft, blocked, False)
        return v if v is not None else Zero(ft, False)

    # statements

    def stmts(self, ctx, budget, depth):
        out = []
        while budget > 0:
            s = self.stmt(ctx, depth)
            if s is None:
                continue
            out += s
            budget -= len(s)
        return out

    def declare(self, ctx, name, t, writable):
        ctx.scopes[-1][name] = Var(name, t, writable)

    # the kinds of statement and their weights
    KINDS = (("var", 18), ("assign", 20), ("if", 8), ("loop", 6), ("foo", 6),
             ("call", 8), ("show", 12), ("copy", 4), ("write", 2),
             ("block", 3), ("jump", 3), ("return", 2), ("exit", 0.1),
             ("read", 1))

    def stmt(self, ctx, depth):
        rnd = self.rnd
        kind = rnd.choices([k for k, _ in self.KINDS],
                           [w for _, w in self.KINDS])[0]
        nested = depth > 0
        if kind == "var":
            return self.var_decl(ctx)
        if kind == "assign":
            return self.assign(ctx)
        if kind == "if" and nested:
            return self.if_stmt(ctx, depth)
        if kind == "loop" and nested:
            return self.loop_stmt(ctx, depth)
        if kind == "foo" and nested:
            return self.foo_stmt(ctx, depth)
        if kind == "call":
            return self.call_stmt(ctx)
        if kind == "copy":
            return self.copy_stmt(ctx)
        if kind == "write":
            return self.write_stmt(ctx)
        if kind == "block" and nested:
            return [Block(self.shadowing_block(ctx, depth))]
        if kind == "jump" and ctx.loops > 0:
            cond = self.cond(ctx)
            if cond is None:
                return None
            return [If([(cond, [Simple(rnd.choice(["break", "continue"]))])],
                       None)]
        if kind == "return" and ctx.fn is not None and ctx.fn.kind != "ctor":
            cond = self.cond(ctx)
            if cond is None:
                return None
            return [If([(cond, [Simple("return")])], None)]
        if kind == "read" and ctx.fn is None and depth == 3 and \
                self.reads < 3:
            return self.read_stmt(ctx)
        if kind == "exit" and ctx.fn is None and depth == 3:
            cond = self.cond(ctx)
            if cond is None:
                return None
            return [If([(cond, [ExitS(rnd.randint(1, 100))])], None)]
        return self.show_stmt(ctx)

    def shadowing_block(self, ctx, depth):
        # a block that starts with a constant or a variable named like an
        # outer one, of another type
        rnd = self.rnd
        outer = [v for v in ctx.visible() if (v.name[0] in "vk" or
                                              v.name.startswith("s8")) and
                 v.name not in ctx.readonly]
        ctx.scopes.append({})
        head = []
        if rnd.random() < 0.6:
            name = rnd.choice(outer).name if outer and self.chance(0.5) \
                else self.name("k")
            value = rnd.randint(-100, 100)
            var = Var(name, "int", False)
            var.const_value = value
            ctx.scopes[-1][name] = var
            head.append(VarDecl(name, Lit(value), "int", let=True))
        if outer and rnd.random() < 0.5:
            name = rnd.choice(outer).name
            if name not in ctx.scopes[-1]:
                t = rnd.choice(INTS)
                # note: the initializer cannot read the name it declares
                was_hidden = name in ctx.hidden
                ctx.hidden.add(name)
                e = self.int_expr(ctx, t, True, 1)
                if not was_hidden:
                    ctx.hidden.discard(name)
                if e is not None and not (e.const and t != "int"):
                    head.append(VarDecl(name, e, t))
                    self.declare(ctx, name, t, True)
        body = head + self.stmts(ctx, 3, depth - 1)
        ctx.scopes.pop()
        return body

    def cond(self, ctx):
        for _ in range(20):
            e = self.bool_expr(ctx, 1)
            if not self.has_blit(e):
                return e
        return None

    def has_blit(self, e):
        if isinstance(e, BLit):
            return True
        if isinstance(e, Not):
            return self.has_blit(e.x)
        if isinstance(e, Logic):
            return self.has_blit(e.lhs) or self.has_blit(e.rhs)
        return False

    def block(self, ctx, budget, depth):
        ctx.scopes.append({})
        body = self.stmts(ctx, budget, depth)
        ctx.scopes.pop()
        return body

    def show_stmt(self, ctx):
        p = self.path(ctx, lambda u: is_int(u) or u == "bool")
        if p is None:
            return None
        if p.typ == "bool":
            return [Show(BoolToInt(Read(p)))]
        return [Show(Conv("int", Read(p), p.typ) if p.typ != "int"
                     else Read(p))]

    def var_decl(self, ctx):
        rnd = self.rnd
        name = self.name("v")
        r = rnd.random()
        let = rnd.random() < 0.15
        if r < 0.5:
            t = rnd.choice(INTS)
            if t == "i8":
                name = self.name("s8")
            if rnd.random() < 0.3 and t != "int":
                inner = rnd.choice(INTS)
                x = self.int_expr(ctx, inner, True, 2)
                if x is None:
                    return None
                e = Conv(t, x, inner)
            else:
                e = self.int_expr(ctx, t, True, 2)
                if e is None:
                    return None
                if t != "int" and isinstance(e, (Lit, Const)):
                    return None
            if e.const and let:
                return None
            self.declare(ctx, name, t, not let)
            return [VarDecl(name, e, t, let)]
        if r < 0.6:
            e = self.bool_expr(ctx, 2)
            # note: a constant 'bool' would decide a jump at the parse
            let = let and not self.has_blit(e)
            self.declare(ctx, name, "bool", not let)
            return [VarDecl(name, e, "bool", let)]
        if r < 0.65 and self.chance(0.3):
            text = "".join(rnd.choice("abcxyz ") for _ in
                           range(rnd.randint(1, 6))) + "\\n"
            t = Arr("i8", len(text) - 1)
            self.declare(ctx, name, t, not let)
            return [VarDecl(name, StrLit(text), t, let)]
        if r < 0.75:
            t = Arr(rnd.choice(INTS + tuple(self.records)),
                    rnd.randint(1, 5))
            if rnd.random() < 0.5:
                e = Zero(t, True)
            elif not is_int(t.elem):
                elems = [self.record_value(ctx, t.elem, set(), False) or
                         Zero(t.elem, False)
                         for _ in range(rnd.randint(1, t.n))]
                e = Lit2(t, elems, True)
            else:
                elems = [self.value_for(ctx, t.elem, set(), 0)
                         for _ in range(rnd.randint(1, min(t.n, 6)))]
                e = Lit2(t, elems, True)
            self.declare(ctx, name, t, not let)
            return [VarDecl(name, e, t, let)]
        if r < 0.85:
            p = self.path(ctx, lambda u: isinstance(u, (Rec, Arr)))
            if p is None:
                return None
            self.declare(ctx, name, p.typ, not let)
            return [VarDecl(name, Read(p), p.typ, let)]
        t = rnd.choice(self.records)
        r2 = rnd.random()
        if r2 < 0.3:
            e = Zero(t, True)
        elif r2 < 0.6:
            ctors = [f for f in self.p.functions if f.kind == "ctor" and
                     f.owner is t]
            if ctors and (ctx.fn is None or not ctx.fn.pure or
                          ctors[0].pure):
                e = self.make_call(ctx, rnd.choice(ctors), 1)
                if e is None:
                    return None
            else:
                e = self.record_value(ctx, t, set(), True)
        elif r2 < 0.8 and t.bases[-1] in self.generic_bases:
            mk = [f for f in self.p.functions if f.kind == "generic" and
                  f.result == "T" and f.owner_base in t.bases]
            if not mk:
                return None
            fn = mk[0]
            c = self.make_generic_make(ctx, fn, t, explicit=True)
            if c is None:
                return None
            e = c
        else:
            e = self.record_value(ctx, t, set(), True)
        if e is None:
            return None
        self.declare(ctx, name, t, not let)
        return [VarDecl(name, e, t, let)]

    def make_generic_make(self, ctx, fn, t, explicit):
        a = self.int_expr(ctx, "i8", False, 1, nonconst=True)
        if a is None or (a.leaf and not self.leaf_type_is(a, "i8")):
            return None
        c = Call(fn, [a], None, [t.name] if explicit else None)
        c.dest_type = t
        return c

    def assign(self, ctx):
        rnd = self.rnd
        p = self.path(ctx, lambda u: not isinstance(u, UArr), writable=True)
        if p is None:
            return None
        t = p.typ
        blocked = {p.root}
        if (is_int(t) or t == "bool") and self.chance(0.04):
            return [Assign(p, Zero(t, False))]
        if is_int(t):
            if rnd.random() < 0.25:
                inner = rnd.choice(INTS)
                x = self.int_expr(ctx, inner, True, 2)
                if x is None:
                    return None
                e = Conv(t, x, inner)
            else:
                e = self.int_expr(ctx, t, False, 2)
                if e is None or (e.leaf and isinstance(e, Read) and
                                 RANK[e.path.typ] > RANK[t]):
                    return None
            if self.calls_in(e) & blocked or self.call_args_touch(e, blocked):
                return None
            return [Assign(p, e)]
        if t == "bool":
            e = self.bool_expr(ctx, 2)
            if self.calls_in(e) & blocked or self.call_args_touch(e, blocked):
                return None
            return [Assign(p, e)]
        r = rnd.random()
        if r < 0.2 or (isinstance(t, Rec) and t.is_proto and r < 0.5):
            return [Assign(p, Zero(t, False))]
        if isinstance(t, Rec) and t.is_proto:
            q = self.path(ctx, lambda u: u is t, avoid=blocked)
            return [Assign(p, Read(q))] if q is not None else None
        if r < 0.5:
            q = self.path(ctx, lambda u: u is t or (isinstance(t, Arr) and
                                                     u == t), avoid=blocked)
            if q is not None:
                return [Assign(p, Read(q))]
        if isinstance(t, Rec):
            if r < 0.7:
                fns = [f for f in self.p.functions if f.result is t and
                       (ctx.fn is None or not ctx.fn.pure or f.pure)]
                ctors = [f for f in self.p.functions if f.kind == "ctor"
                         and f.owner is t and (ctx.fn is None or
                                               not ctx.fn.pure or f.pure)]
                fns += ctors
                if fns:
                    c = self.make_call(ctx, rnd.choice(fns), 1,
                                       avoid=blocked)
                    if c is not None and not (p.root in
                                              c.fn.globals_used):
                        return [Assign(p, c)]
            if r < 0.8 and t.bases[-1] in self.generic_bases:
                mk = [f for f in self.p.functions if f.kind == "generic" and
                      f.result == "T" and f.owner_base in t.bases]
                if mk and (ctx.fn is None or not ctx.fn.pure or mk[0].pure) \
                        and p.root not in mk[0].globals_used:
                    c = self.make_generic_make(ctx, mk[0], t,
                                               explicit=self.chance(0.3))
                    if c is not None and not self.roots_in(c) & blocked:
                        return [Assign(p, c)]
            v = self.record_value(ctx, t, blocked, False)
            if v is None or self.roots_in(v) & blocked:
                return None
            return [Assign(p, v)]
        v = self.value_for(ctx, t, blocked, 1)
        if self.roots_in(v) & blocked:
            return None
        return [Assign(p, v)]

    def call_args_touch(self, e, blocked):
        for c in self.has_call(e):
            for a in c.args:
                if self.roots_in(a) & blocked:
                    return True
            if c.recv is not None and c.recv.root in blocked:
                return True
        return False

    def if_stmt(self, ctx, depth):
        branches = []
        for _ in range(self.rnd.randint(1, 3)):
            cond = self.bool_expr(ctx, 2)
            branches.append((cond, self.block(ctx, 3, depth - 1)))
        other = self.block(ctx, 2, depth - 1) if self.chance(0.5) else None
        return [If(branches, other)]

    def loop_stmt(self, ctx, depth):
        c = self.name("c")
        limit = self.rnd.randint(0, 4)
        decl = VarDecl(c, Lit(0), "int")
        self.declare(ctx, c, "int", True)
        ctx.readonly.add(c)
        head = [If([(Cmp(">=", Read(Path(c, [], "int")), Lit(limit), "int",
                         "int"), [Simple("break")])], None),
                Assign(Path(c, [], "int"), Bin("+", Read(Path(c, [], "int")),
                                               Lit(1)))]
        ctx.loops += 1
        body = self.block(ctx, 4, depth - 1)
        ctx.loops -= 1
        return [decl, LoopS(head + body)]

    def foo_stmt(self, ctx, depth):
        # note: a 'foo' inside another hides its 'e', 'i' and 'n', the outer
        #       'e' is not the array of the inner one since the body names it
        p = self.path(ctx, lambda u: isinstance(u, (Arr, UArr)), avoid=("e",))
        if p is None:
            return None
        cnt = None
        if self.chance(0.4) or (isinstance(p.typ, Arr) and p.typ.n > 16):
            if isinstance(p.typ, Arr):
                cnt = self.count_expr(ctx, p.typ)
            else:
                cnt = Bin("%", self.int_expr(ctx, "int", True, 0,
                                             nonconst=True, calls=False)
                          or Lit(1), ALen(p)) if self.chance(0.5) else Lit(0)
        root = ctx.lookup(p.root) or self.globals.get(p.root)
        writable = root.writable and p.root not in ctx.readonly and \
            self.global_ok(ctx, root, True)
        ctx.scopes.append({"e": Var("e", p.typ.elem, writable),
                           "i": Var("i", "int", False)})
        outer_consts = ctx.consts
        ctx.consts = dict(ctx.consts)
        ctx.consts["n"] = p.typ.n if isinstance(p.typ, Arr) else 1
        ctx.hidden.add(p.root)
        was = ctx.in_foo_global
        ctx.in_foo_global = was or root.is_global
        outer_n = ctx.foo_n
        ctx.foo_n = p.typ.n if isinstance(p.typ, Arr) else None
        ctx.loops += 1
        body = self.block(ctx, 3, depth - 1)
        ctx.loops -= 1
        ctx.foo_n = outer_n
        ctx.in_foo_global = was
        ctx.hidden.discard(p.root)
        ctx.scopes.pop()
        ctx.consts = outer_consts
        return [Foo(p, cnt, body)]

    def call_stmt(self, ctx):
        fns = [f for f in self.callable_fns(ctx, None) if f.result is None
               or not f.pure]
        if not fns:
            return None
        fn = self.rnd.choice(fns)
        if fn.result == "T":
            return None
        if ctx.in_foo_global and not fn.pure:
            return None
        c = self.make_call(ctx, fn, 1)
        if c is None:
            return None
        if fn.result is None:
            return [CallS(c)]
        name = self.name("v")
        self.declare(ctx, name, fn.result, True)
        return [VarDecl(name, c, fn.result)]

    def copy_stmt(self, ctx):
        dst = self.path(ctx, lambda u: isinstance(u, (Arr, UArr)),
                        writable=True)
        if dst is None:
            return None
        src = self.path(ctx, lambda u: isinstance(u, (Arr, UArr)) and
                        u.elem == dst.typ.elem)
        if src is None:
            return None
        src = self.maybe_start(src)
        dst = self.maybe_start(dst)
        cnt = self.count_expr(ctx, self.room(src), self.room(dst))
        return [Copy(src, dst, cnt)]

    def room(self, p):
        # the elements from the start of an array path
        if p.steps and p.steps[-1][0] == "i" and \
                isinstance(p.steps[-1][1], Lit):
            k = p.steps[-1][1].v
            return Arr(p.typ, p.parent_type.n - k)
        return p.typ

    def maybe_start(self, p):
        if not isinstance(p.typ, Arr) or not self.chance(0.4):
            return p
        k = self.rnd.randrange(p.typ.n)
        q = Path(p.root, p.steps + [("i", Lit(k))], p.typ.elem)
        q.parent_type = p.typ
        return q

    def write_stmt(self, ctx):
        p = self.path(ctx, lambda u: isinstance(u, Arr) and u.elem == "i8"
                      and u.n >= 1)
        if p is None:
            return None
        n = min(p.typ.n, 12)
        if p.typ.n <= 16 and self.chance(0.3):
            # the whole array or a count from its start
            cnt = Lit(self.rnd.randint(0, n)) if self.chance(0.5) else None
            return [WriteS(p, cnt, None)]
        start = self.rnd.randint(0, n)
        cnt = self.rnd.randint(0, n - start)
        if self.chance(0.3):
            x = self.int_expr(ctx, "int", True, 0, nonconst=True, calls=False)
            if x is not None:
                # note: 'rdx' of x86_64 holds the count, a '%' there would
                # need it too
                mask = 1
                while mask * 2 <= n - start + 1:
                    mask *= 2
                return [WriteS(p, Bin("&", x, Lit(mask - 1)), Lit(start))]
        return [WriteS(p, Lit(cnt), Lit(start))]

    def read_stmt(self, ctx):
        # 'var gN = read(0, ...)' at the top of 'main', each runs once: the
        # first takes the line, the others get 0
        rnd = self.rnd
        need = len(self.p.stdin)
        out = []
        p = self.path(ctx, lambda u: isinstance(u, Arr) and u.elem == "i8"
                      and u.n >= need, writable=True)
        if p is None:
            t = Arr("i8", rnd.randint(need, 12))
            name = self.name("v")
            self.declare(ctx, name, t, True)
            out.append(VarDecl(name, Zero(t, True), t))
            p = Path(name, [], t)
        self.reads += 1
        r = rnd.random()
        if r < 0.3:
            e = Input(p, None, None)
        elif r < 0.6:
            e = Input(p, Lit(rnd.randint(need, p.typ.n)), None)
        else:
            start = rnd.randint(0, p.typ.n - need)
            e = Input(p, Lit(rnd.randint(need, p.typ.n - start)), Lit(start))
        name = self.name("g")
        self.declare(ctx, name, "int", True)
        return out + [VarDecl(name, e, "int")]

    # functions

    def gen_functions(self, base, proto):
        rnd = self.rnd
        plan = ["func"] * rnd.randint(2, 5) + ["method"] * rnd.randint(1, 3) \
            + ["mmethod"] * rnd.randint(1, 2) + ["ctor"] + \
            ["gmethod", "gmmethod", "gfunc", "gmake"]
        rnd.shuffle(plan)
        plan.insert(0, "recursive")
        for kind in plan:
            fn = self.gen_function(kind, base, proto)
            if fn is not None:
                fn.mixed = self.chance(0.5)
                self.p.functions.append(fn)

    def param_type(self):
        rnd = self.rnd
        r = rnd.random()
        if r < 0.55:
            return rnd.choice(INTS)
        if r < 0.65:
            return "bool"
        if r < 0.8:
            return UArr(rnd.choice(INTS), bare=self.chance(0.7))
        if r < 0.85:
            return UArr(rnd.choice(self.records))
        return rnd.choice(self.records)

    def gen_function(self, kind, base, proto):
        rnd = self.rnd
        fn = Fn(self.name("fn"), kind)
        ctx = Ctx(fn)
        if kind == "recursive":
            fn.recursive = True
            fn.noinline = True
            fn.params = [("d", "i8", False), ("x", "int", False)]
            fn.result = "int"
            self.declare(ctx, "d", "i8", False)
            self.declare(ctx, "x", "int", False)
            self.declare(ctx, "res", "int", True)
            first = Assign(Path("res", [], "int"),
                           Read(Path("x", [], "int")))
            stop = If([(Cmp("<=", Read(Path("d", [], "i8")), Lit(0), "i8",
                            "i8"), [Simple("return")])], None)
            body = self.stmts(ctx, rnd.randint(1, 4), 1)
            call = self.recursive_call(ctx, fn)
            tail = Assign(Path("res", [], "int"),
                          Bin(rnd.choice(["+", "-", "^"]),
                              Read(Path("res", [], "int")), call))
            fn.body = [first, stop] + body + [tail]
            fn.pure = True
            self.finish(fn)
            return fn
        if kind in ("method", "mmethod", "gmethod", "gmmethod"):
            if kind.startswith("g"):
                owner = proto
                fn.owner_base = base
                fn.kind = "method" if kind == "gmethod" else "mmethod"
            else:
                plain = [r for r in self.records if not r.consts]
                owner = rnd.choice(plain)
                fn.owner_base = owner.name
            fn.owner = owner
            fn.short = self.name("m")
            taken = {g.short for g in self.p.functions if g.owner is owner}
            free = [f for f, _ in owner.fields if f not in taken]
            if free and self.chance(0.3):
                # note: a method may have the name of a field
                fn.short = rnd.choice(free)
            fn.owner_text = owner.name
            self.declare(ctx, "self", owner, fn.kind == "mmethod")
            ctx.consts = dict(owner.consts)
            if kind.startswith("g"):
                ctx.consts = {"c": 2}
        elif kind == "ctor":
            plain = [r for r in self.records if not r.consts]
            owner = rnd.choice(plain)
            fn.owner = owner
            fn.owner_base = owner.name
            fn.short = self.name("mk")
            fn.owner_text = owner.name
        elif kind in ("gfunc", "gmake"):
            fn.kind = "generic"
            fn.owner_base = base
            fn.tparams = ["T"]
            # note: one of each per program
            fn.short = "fill" if kind == "gfunc" else "make"
            ctx.consts = {}
        for _ in range(rnd.randint(0, 3)):
            pname = self.name("p")
            ptype = self.param_type()
            mut = kind in ("func",) and rnd.random() < 0.3
            if mut and isinstance(ptype, str) and ptype == "bool":
                mut = False
            fn.params.append((pname, ptype, mut))
            self.declare(ctx, pname, ptype, mut)
            if mut:
                fn.pure = False
        if kind == "gfunc":
            mut = rnd.random() < 0.4
            fn.params.insert(0, ("x", "T", mut))
            self.declare(ctx, "x", proto, mut)
            if mut:
                fn.pure = False
        if kind == "gmake":
            fn.params = [("v", "i8", False)]
            ctx.scopes = [{}]
            self.declare(ctx, "v", "i8", False)
        if fn.kind == "mmethod":
            fn.pure = False
        if kind == "func" and rnd.random() < 0.25:
            fn.pure = False
        # the result
        body = []
        if kind == "ctor":
            for f, ft in owner.fields:
                v = self.value_for(ctx, ft, set(), 1)
                if not is_int(ft) and ft != "bool" and \
                        not isinstance(v, (Lit2, Zero)):
                    v = Zero(ft, False)
                body.append(Assign(Path("self", [("f", f)], ft), v))
            self.declare(ctx, "self", owner, True)
        elif kind == "gmake":
            fn.result = "T"
            self.declare(ctx, "res", proto, True)
            body.append(Assign(Path("res", [], proto), Zero(proto, False)))
            body.append(Assign(Path("res", [("f", "k")], "i8"),
                               Read(Path("v", [], "i8"))))
        elif rnd.random() < 0.8 or fn.pure:
            r = rnd.random()
            if r < 0.6:
                rt = rnd.choice(INTS)
            elif r < 0.75:
                rt = "bool"
            else:
                rt = rnd.choice([x for x in self.records if
                                 not x.consts])
            fn.result = rt
            v = self.value_for(ctx, rt, set(), 2)
            self.declare(ctx, "res", rt, True)
            if isinstance(rt, Rec) and not isinstance(v, (Lit2, Zero)):
                v = Zero(rt, False)
            body.append(Assign(Path("res", [], rt), v))
        ctx.fn = fn
        body += self.stmts(ctx, rnd.randint(1, 6), 2)
        fn.body = body
        self.finish(fn)
        return fn

    def finish(self, fn):
        # the globals a function uses, also through its calls
        used = set()

        def visit_expr(x):
            if isinstance(x, Read):
                if x.path.root in self.globals:
                    used.add(x.path.root)
                for kind, i in x.path.steps:
                    if kind == "i":
                        visit_expr(i)
            elif isinstance(x, ALen):
                visit_expr(Read(x.path))
            elif isinstance(x, (Un, Not, Conv)):
                visit_expr(x.x)
            elif isinstance(x, (Bin, Cmp, Logic)):
                visit_expr(x.lhs)
                visit_expr(x.rhs)
            elif isinstance(x, BoolToInt):
                visit_expr(x.b)
            elif isinstance(x, Call):
                if x.fn is not fn:
                    used.update(x.fn.globals_used)
                if x.recv is not None:
                    visit_expr(Read(x.recv))
                for a in x.args:
                    visit_expr(a)
            elif isinstance(x, AEq):
                visit_expr(Read(x.a))
                visit_expr(Read(x.b))
                visit_expr(x.cnt)
            elif isinstance(x, Lit2):
                for a in x.elems:
                    visit_expr(a)

        def visit(stmts):
            for s in stmts:
                if isinstance(s, VarDecl):
                    visit_expr(s.value)
                elif isinstance(s, Assign):
                    visit_expr(Read(s.path))
                    visit_expr(s.value)
                elif isinstance(s, If):
                    for c, b in s.branches:
                        visit_expr(c)
                        visit(b)
                    if s.other:
                        visit(s.other)
                elif isinstance(s, (LoopS, Block)):
                    visit(s.body)
                elif isinstance(s, Foo):
                    visit_expr(Read(s.path))
                    if s.cnt is not None:
                        visit_expr(s.cnt)
                    visit(s.body)
                elif isinstance(s, CallS):
                    visit_expr(s.call)
                elif isinstance(s, Show):
                    visit_expr(s.value)
                elif isinstance(s, WriteS):
                    visit_expr(Read(s.path))
                    for x in (s.cnt, s.start):
                        if x is not None:
                            visit_expr(x)
                elif isinstance(s, Copy):
                    visit_expr(Read(s.src))
                    visit_expr(Read(s.dst))
                    visit_expr(s.cnt)
        visit(fn.body)
        fn.globals_used = used

    def gen_main(self):
        fn = Fn("main", "main")
        fn.pure = False
        ctx = Ctx(None)
        fn.body = []
        if self.chance(0.4):
            t = Arr(self.rnd.choice(INTS), self.rnd.randint(500, 1500))
            self.declare(ctx, "big", t, True)
            fn.body.append(VarDecl("big", Zero(t, True), t))
        fn.body += self.stmts(ctx, self.rnd.randint(30, 70), 3)
        self.p.main = fn

    def generate(self):
        self.gen_types()
        base, proto = self.gen_generics()
        self.gen_globals()
        self.gen_functions(base, proto)
        self.gen_main()
        return self.p


def fn_text(fn, noinline):
    word = "noinline " if noinline or fn.noinline else ""
    params = []
    for name, ptype, mut in fn.params:
        t = "T" if ptype == "T" else type_text(ptype)
        params.append(f"{name}{' mut' if mut else ''} {t}".rstrip())
    tp = f"<{', '.join(p + ' type' for p in fn.tparams)}>" if fn.tparams \
        else ""
    if fn.kind == "ctor":
        head = f"func {word}{fn.owner_text}.{fn.short}"
        res = " self"
    elif fn.kind in ("method", "mmethod"):
        m = "mut " if fn.kind == "mmethod" else ""
        head = f"func {word}{m}{fn.owner_text}.{fn.short}"
        res = ""
    else:
        head = f"func {word}{fn.short}{tp}"
        res = ""
    if fn.result is not None and fn.kind != "ctor":
        res = " res " + ("T" if fn.result == "T" else type_text(fn.result))
    lines = [f"{head}({', '.join(params)}){res} {{"]
    lines += block_lines(fn.body, 1)
    lines.append("}")
    return lines


def program_text(p, noinline):
    lines = list(p.head)
    lines.append(SHOW)
    for fn in p.functions:
        # 'mixed': each function as the generator drew it
        lines += fn_text(fn, fn.mixed if noinline == "mixed" else noinline)
    lines.append("func main() {")
    lines += block_lines(p.main.body, 1)
    lines.append("}")
    return "\n".join(lines) + "\n"


# running


# the option lists of the build on one of the targets besides '--checks=noub'
OTHER_OPTIONS = (("--checks=noub", "--nopt"),
                 ("--checks=noub", "--versions=first"),
                 ("--checks=noub", "--versions=last"),
                 ("--checks=noub,line",),
                 ("--checks=noub", "--reproduce-source", "--report=registers"))
# note: the large arrays need more than the default 64 KiB of variables
VARS = "--vars=0x200000"
# the rejections of a constant out of range, which a constant argument of an
# inlined call can fold where a noinline call checks at run time (README 1.5)
FOLDED = ("divisor must not be zero", "shift count must be", "out of bounds",
          "past the end", "overflow", "is negative", "has no value")


def build(work, source, target, options, stdin):
    # what the program did: ('exit' or 'panic', code, stdout),
    # ('compiler-crash', stderr), ('compile', stderr) or how it did not end
    path = os.path.join(work, "f.baz")
    with open(path, "w") as f:
        f.write(fl.split_include(work, source))
    c, o, problem = fl.run_target(fl.COMPILER_ASAN, path, options, work,
                                  target, stdin=stdin)
    if c.timed_out:
        return ("compiler-hang", "")
    if c.returncode:
        sanitizer = any(t in c.stderr for t in fl.SANITIZER_TEXT)
        if sanitizer or c.returncode < 0 or \
                c.returncode in fl.SANITIZER_EXIT:
            return ("compiler-crash", c.stderr[-3000:])
        return ("compile", c.stderr[:800])
    if problem:
        return (problem[0], problem[1][:300])
    if o.kind not in ("exit", "panic"):
        return (o.kind, f"{o.code} {o.detail}")
    return (o.kind, o.code, o.stdout)


def judge(work, source, expected, variant, rnd, const_args, counts, stdin):
    # builds one variant of the program, returns the findings
    targets = fl.with_rotated(fl.TARGETS, source)
    runs = [(target, ("--checks=noub",)) for target in targets]
    runs.append((rnd.choice(targets), rnd.choice(OTHER_OPTIONS)))
    if expected[64][0] == "exit" and expected[32][0] == "exit":
        # a program without a panic ends the same without the checks
        runs.append((rnd.choice(targets), ()))
    found = []
    for target, options in runs:
        options = list(options) + [VARS]
        want = expected[64 if target == "x86_64" else 32]
        got = build(work, source, target, options, stdin)
        # a tool stopped by a limit says nothing about the code
        if got == want or got[0] == "skip":
            continue
        if got[0] == "compile" and limit_of_registers(got[1]):
            counts["out-of-registers"] = 1
            continue
        if got[0] == "compile" and variant != "noinline" and const_args \
                and folded_constant(got[1]):
            counts["folded-reject"] = 1
            continue
        if got[0] == "compile":
            message = got[1].strip().splitlines()[0] if got[1] else ""
            key = f"{target} {message.split(': ', 1)[-1]}"
            found.append(("whole-reject", key,
                          f"{variant} {' '.join(options)}: {message}"))
            # one rejection says enough about the program
            return found
        if got[0] == "compiler-crash":
            found.append((got[0], fl.signature(got[1]),
                          f"{target} {variant} {' '.join(options)}\n"
                          f"{got[1]}"))
            continue
        category = got[0] if got[0] in ("compiler-hang", "invalid-asm",
                                        "link-failure", "image-mismatch") \
            else "whole-mismatch"
        key = f"{target} {variant} {' '.join(options)} {got[0]}"
        found.append((category, key,
                      f"expected {short(want)}, got {short(got)}"))
    return found


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    try:
        gen = Gen(rnd)
        program = gen.generate()
    except RecursionError:
        return "", [], "gen-error"
    expected = {}
    for intw in (64, 32):
        try:
            expected[intw] = Model(program, intw).run()
        except TooLong:
            return "", [], "too-long"
        except RecursionError:
            return "", [], "too-deep"
    counts = {expected[64][0]: 1}
    work = tempfile.mkdtemp(prefix="fuzzw-")
    try:
        # note: 'fuzz-coverage.py' sees the first variant, the mixed one has
        #       both kinds of function
        for variant, noinline in (("mixed", "mixed"), ("inline", False),
                                  ("noinline", True)):
            source = program_text(program, noinline)
            found = judge(work, source, expected, variant, rnd,
                          gen.const_args, counts, program.stdin)
            # note: a finding keeps the text of the variant that found it
            if found:
                found = [(category, key, f"{detail}\ninput {program.stdin!r}")
                         for category, key, detail in found]
                return source, found, counts
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, [], counts


def folded_constant(text):
    first = text.strip().splitlines()[0] if text.strip() else ""
    return any(word in first for word in FOLDED)


def limit_of_registers(text):
    # a register that an operation fixes is a limit too (chapter 8 item 3)
    return fl.out_of_registers(text) or "is always used by the operation" \
        in text or "does not fit in the variable storage" in text


def short(o):
    if len(o) == 3:
        return (o[0], o[1], o[2][-120:])
    return o


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--one":
        rnd = random.Random(int(sys.argv[2]) * 1000003 + int(sys.argv[3]))
        program = Gen(rnd).generate()
        print(program_text(program, "--noinline" in sys.argv))
        print("input", program.stdin, file=sys.stderr)
        for intw in (64, 32):
            try:
                print(intw, Model(program, intw).run(), file=sys.stderr)
            except TooLong:
                print(intw, "too long", file=sys.stderr)
        return 0
    count, seed, jobs = fl.arguments(100)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
