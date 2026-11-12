#!/usr/bin/env python3
# model based fuzzer of the forms the compiler can build one program in: each
# function inlined, 'noinline' or as drawn, each '--versions' value and
# '--nopt', with the checks of 'noub' failing inside those bodies
#
# builds programs of functions with 'mut' and read-only array and scalar
# parameters of i8, i16 and i32, a result, an early 'return', 'loop' with
# 'break' and 'continue', 'foo' with and without a count, 'array_copy',
# 'exit', calls nested in arguments and indexes, elements as 'mut' arguments,
# 'dat' globals and arrays larger than the immediate offset of rv32i. the
# values are drawn near the edges of their type, so most programs end in a
# panic, which may come from an inlined or a 'noinline' body. the generator
# leaves out the calls that the rules of shared storage and of evaluation
# order reject (README 4.2), conservatively
#
# python runs the same program left to right with the checks of 'noub' and
# gives the printed values and the exit code or the first panic. the program
# is written with every function inlined, with every function 'noinline' and
# with each as drawn; each variant is built with every option list on x86_64
# and rv32i-fpga and with the first on one of rv32i and rv32i-qemu
# ('fuzzlib.with_rotated'):
#
#   versions-mismatch  a build ends differently from the model
#   versions-reject    a rejection that the generator does not expect, or
#                      one that not every build of the program makes: a
#                      constant out of range is expected, and a shared
#                      storage the generator did not see when every variant
#                      that is not rejected for a constant makes it
#   compiler-crash, compiler-hang, invalid-asm, link-failure, image-mismatch
#                      see fuzz-compiler.py
#
# usage: fuzz-versions.py [count] [seed] [jobs]
import os
import random
import sys
import tempfile

import fuzzlib as fl

TYPES = ("i8", "i16", "i32")
WIDTH = {"i8": 8, "i16": 16, "i32": 32}
# values near the edges of each type, where the checks fail; e.g. 46341
# squared overflows i32 and 46340 squared does not
EDGES = {"i8": (127, -128, 126, -127, 64, -1, 100, 50, -64),
         "i16": (32767, -32768, 32766, 16384, -1, 255, 200, 181, -256),
         "i32": (2147483647, -2147483648, 2147483646, 65536, 46341, -1, 31,
                 46340, -65536)}
OPTION_LISTS = (("--checks=noub",), ("--checks=noub", "--nopt"),
                ("--checks=noub", "--versions=nested"),
                ("--checks=noub", "--versions=first"),
                ("--checks=noub", "--versions=last"))
# the rejections of a constant out of range: of a constant expression in any
# variant, and of a constant argument folded into an inlined body where a
# 'noinline' call checks at run time (README 1.5)
FOLDED = ("divisor must not be zero", "shift count must be", "out of bounds",
          "past the end", "overflow", "is negative")
# the rejections of shared storage and of evaluation order, the same with and
# without inlining (README 4.2)
SHARED = ("may share storage", "may be written by the call")
# the name of a panic by its exit code, for the counts of a run
PANIC_NAME = {code: name for name, code in fl.PANIC_CODE.items()}
# a statement of the model past this count ends the program as too long
MODEL_STEPS = 100000


class Panic(Exception):
    def __init__(self, name):
        super().__init__(name)
        self.name = name


class TooLong(Exception):
    pass


class Return(Exception):
    pass


class Exit(Exception):
    def __init__(self, code):
        super().__init__(code)
        self.code = code


class Break(Exception):
    pass


class Continue(Exception):
    pass


def literal_text(v):
    return str(v) if v >= 0 else f"({v})"


# the program is a tree of tuples:
#
# expressions: ('var', name) ('neg', name) ('un', '-' or '~', e)
# ('elem', array, index) ('call', function, args) ('conv', type, e)
# ('tlit', type, value) a typed literal, ('lit', value) an untyped one,
# ('bin', op, lhs, rhs); the first element decides the type of a 'bin'
#
# indexes: ('ix', kind, name, c): 'c' the constant 'c', 'v' the variable
# 'name', 'and' 'name & 1', 'add' 'name + c', 'call' the call 'name' & 1
#
# arguments: ('arg_arr', array) ('arg_var', name) a 'mut' scalar,
# ('arg_elem', array, index) a 'mut' element, ('arg_val', e) a read-only value
#
# conditions: ('cmp', op, lhs, rhs) ('and' or 'or', c, c)
#
# statements: ('assign', destination, e) with destination a 'var' or 'elem',
# ('var', name, type, e) ('pad', name, size) an unused 'i8' array,
# ('if', c, then, else or None) ('loop', counter, start, bound, body)
# ('foo', array, count index or None, body) ('jump', c, 'break' or
# 'continue') ('ret', c) ('exit', c, e) ('callst', call)
# ('copy', src, index, dst, index, count index) ('pr', e) prints 'i32(e)'


class Function:
    def __init__(self, name, params, res):
        self.name = name
        # (name, 'arr' or 'sc', type, mut)
        self.params = params
        self.res = res
        self.body = []
        # the globals the body reaches, also through its calls
        self.globals = set()


class Scope:
    # what the statements of a block can use: scalars (name, type, writable),
    # arrays (name, type, length or None when unsized, writable), the loop
    # counters and the array of the 'foo' they are in, whose element is 'e'
    def __init__(self):
        self.scalars = []
        self.arrays = []
        self.counters = []
        self.foo_array = None

    def copy(self):
        scope = Scope()
        scope.scalars = list(self.scalars)
        scope.arrays = list(self.arrays)
        scope.counters = list(self.counters)
        scope.foo_array = self.foo_array
        return scope

    def types(self):
        return [t for _, t, _ in self.scalars] + [a[1] for a in self.arrays]

    def root(self, name):
        # the variable whose storage 'name' is
        return self.foo_array if name == "e" else name


class Effects:
    # the reads and writes of one statement in the order of evaluation, to
    # leave out what the compiler rejects (README 4.2): a call that writes
    # what an earlier operand or the destination read, a call whose
    # arguments, globals and result destination name the same storage where
    # one of them is written; conservative, every global a callee reaches
    # counts as written and the destination as the result of every call
    def __init__(self, functions, scope, destination=None):
        self.functions = functions
        self.scope = scope
        self.destination = destination
        self.read = set()
        self.valid = True
        if destination:
            self.read.add(destination)

    def expr(self, e):
        kind = e[0]
        if kind in ("var", "neg"):
            self.read.add(self.scope.root(e[1]))
        elif kind == "elem":
            self.index(e[2])
            self.read.add(self.scope.root(e[1]))
        elif kind in ("un", "conv"):
            self.expr(e[2])
        elif kind == "bin":
            self.expr(e[2])
            self.expr(e[3])
        elif kind == "call":
            self.call(e)

    def condition(self, c):
        if c[0] == "cmp":
            self.expr(c[2])
            self.expr(c[3])
        else:
            self.condition(c[1])
            self.condition(c[2])

    def index(self, ix):
        _, kind, name, _ = ix
        if kind == "call":
            self.call(name)
        elif kind != "c":
            self.read.add(self.scope.root(name))

    def call(self, call):
        function = self.functions[call[1]]
        read_before = set(self.read)
        # (root, written) of what the call passes and reaches
        storage = [(g, True) for g in function.globals]
        if self.destination:
            storage.append((self.destination, True))
        for (_, _, _, mut), arg in zip(function.params, call[2]):
            if arg[0] == "arg_val":
                self.expr(arg[1])
                if arg[1][0] in ("var", "elem"):
                    storage.append((self.scope.root(arg[1][1]), False))
                continue
            if arg[0] == "arg_elem":
                self.index(arg[2])
            root = self.scope.root(arg[1])
            self.read.add(root)
            storage.append((root, mut))
        for i, (root, written) in enumerate(storage):
            for other, other_written in storage[i + 1:]:
                if root == other and (written or other_written):
                    self.valid = False
        written = {root for root, w in storage if w}
        if written & read_before:
            self.valid = False


class Program:
    def __init__(self):
        # ('sc', name, type, value) or ('arr', name, type, values)
        self.global_decls = []
        self.functions = []
        # as 'global_decls'
        self.main_locals = []
        self.main_body = []
        # the functions that the variant as drawn makes 'noinline'
        self.drawn_noinline = set()


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        # how often a value is at an edge of its type, an index is a variable
        # not masked to 0 or 1, the depth and the length of an expression
        self.edge = rnd.choice([0.05, 0.1, 0.2, 0.3])
        self.raw = rnd.choice([0.0, 0.05, 0.15])
        self.depth = rnd.choice([1, 2, 2, 3, 4])
        self.wide = rnd.choice([0, 0, 1, 2])
        self.program = Program()
        self.by_name = {}
        self.global_scope = Scope()
        self.current = None
        self.names = 0
        self.in_index = False

    def fresh(self, prefix):
        self.names += 1
        return f"{prefix}{self.names}"

    def value(self, t):
        rnd = self.rnd
        low, high = fl.limits(t)
        v = rnd.choice(EDGES[t]) if rnd.random() < self.edge \
            else rnd.randint(-3, 9)
        return max(low, min(high, v))

    def available_types(self, scope):
        return scope.types() or list(TYPES)

    def narrow(self, scope, t):
        # a type no wider than 't' that leads a value assigned at 't'
        if self.rnd.random() < 0.7:
            return t
        return self.rnd.choice([u for u in self.available_types(scope)
                                if WIDTH[u] <= WIDTH[t]] or [t])

    def callable(self):
        return [f for f in self.program.functions if f is not self.current]

    def allowed(self, scope, statement):
        # whether the statement keeps the rules of shared storage and of
        # evaluation order (see 'Effects')
        kind = statement[0]
        destination = statement[1] if kind == "assign" else None
        effects = Effects(self.by_name, scope,
                          scope.root(destination[1]) if destination else None)
        if kind == "assign":
            if destination[0] == "elem":
                effects.index(destination[2])
            effects.expr(statement[2])
        elif kind in ("var", "pr"):
            effects.expr(statement[-1])
        elif kind in ("if", "jump", "ret"):
            effects.condition(statement[1])
        elif kind == "exit":
            effects.condition(statement[1])
            effects.expr(statement[2])
        elif kind == "callst":
            effects.call(statement[1])
        elif kind == "foo":
            effects.read.add(statement[1])
            if statement[2]:
                effects.index(statement[2])
        elif kind == "copy":
            _, src, src_index, dst, dst_index, count = statement
            effects.read |= {scope.root(src), scope.root(dst)}
            effects.index(src_index)
            effects.index(dst_index)
            effects.index(count)
        return effects.valid

    # expressions

    def index(self, scope, length):
        rnd = self.rnd
        c = rnd.random()
        variables = [s for s, _, _ in scope.scalars]
        if scope.counters and c < 0.45:
            j = rnd.choice(scope.counters)
            k = rnd.choice([-1] * 4 + [0, 1])
            return ("ix", "add", j, k) if k else ("ix", "v", j, 0)
        if variables and c < 0.8:
            v = rnd.choice(variables)
            return ("ix", "v", v, 0) if rnd.random() < self.raw else \
                ("ix", "and", v, 0)
        calls = [f for f in self.callable() if f.res]
        if calls and c < 0.9 and not self.in_index:
            self.in_index = True
            call = self.call(scope, rnd.choice(calls), 0)
            self.in_index = False
            if call:
                return ("ix", "call", call, 0)
        n = length if length else 1
        return ("ix", "c", None, rnd.randint(0, n - 1))

    def first(self, scope, t, depth):
        # the first element of an expression of type 't'
        rnd = self.rnd
        scalars = [s for s, u, _ in scope.scalars if u == t]
        arrays = [a for a in scope.arrays if a[1] == t]
        calls = [f for f in self.callable() if f.res == t]
        kinds = []
        if scalars:
            kinds += ["sc"] * 4 + ["neg"]
        if arrays:
            kinds += ["ar"] * 4
        if calls and depth > 0:
            kinds += ["call"] * 3
        if not kinds or depth > 0:
            kinds += ["conv"]
        kind = rnd.choice(kinds)
        if rnd.random() < 0.08:
            inner = self.first(scope, t, depth)
            if inner[0] != "tlit":
                return ("un", rnd.choice(["-", "-", "~"]), inner)
        if kind == "sc":
            return ("var", rnd.choice(scalars))
        if kind == "neg":
            return ("neg", rnd.choice(scalars))
        if kind == "ar":
            a = rnd.choice(arrays)
            return ("elem", a[0], self.index(scope, a[2]))
        if kind == "call":
            call = self.call(scope, rnd.choice(calls), depth - 1)
            if call:
                return call
        u = rnd.choice(self.available_types(scope))
        if depth > 0 and u in scope.types():
            return ("conv", t, self.expr(scope, u, depth - 1))
        return ("tlit", t, self.value(t))

    def operand(self, scope, t, depth):
        # a later operand of an expression of type 't'
        rnd = self.rnd
        narrower = [u for u in TYPES if WIDTH[u] <= WIDTH[t]]
        c = rnd.random()
        if c < 0.3:
            # note: not zero, a constant divisor of zero is rejected
            return ("lit", self.value(t) or 3)
        u = rnd.choice([x for x in self.available_types(scope)
                        if x in narrower] or narrower)
        if c < 0.8 or depth <= 0:
            return self.first(scope, u, depth)
        return self.expr(scope, u, depth - 1)

    def shift_count(self, scope, t):
        rnd = self.rnd
        scalars = [s for s, u, _ in scope.scalars if WIDTH[u] <= WIDTH[t]]
        if scalars and rnd.random() < 0.6:
            count = ("var", rnd.choice(scalars))
            if rnd.random() < 0.7:
                count = ("bin", "&", count, ("lit", WIDTH[t] // 2 - 1))
            return count
        return ("lit", rnd.randint(0, WIDTH[t] - 1))

    def expr(self, scope, t, depth):
        rnd = self.rnd
        e = self.first(scope, t, depth)
        for _ in range(rnd.choice([0, 0, 1, 1, 2] + [3, 4] * self.wide)):
            op = rnd.choice(["+", "-", "*", "+", "-", "*", "/", "%", "&",
                             "|", "^", "<<", ">>", "&", "|", "+"])
            if op in ("<<", ">>"):
                e = ("bin", op, e, self.shift_count(scope, t))
            else:
                e = ("bin", op, e, self.operand(scope, t, depth - 1))
        return e

    def condition(self, scope, depth):
        rnd = self.rnd
        t = rnd.choice(self.available_types(scope))
        c = ("cmp", rnd.choice(["<", ">", "==", "!=", "<=", ">="]),
             self.expr(scope, t, depth), self.operand(scope, t, 0))
        if rnd.random() < 0.2:
            c = (rnd.choice(["and", "or"]), c, self.condition(scope, 0))
        return c

    def call(self, scope, f, depth):
        # a call of 'f' or None when the scope has no argument for a 'mut'
        # parameter
        rnd = self.rnd
        args = []
        for _, kind, t, mut in f.params:
            if kind == "arr":
                arrays = [a for a in scope.arrays if a[1] == t and
                          (a[3] or not mut)]
                if not arrays:
                    return None
                args.append(("arg_arr", rnd.choice(arrays)[0]))
            elif mut:
                scalars = [s for s, u, w in scope.scalars if u == t and w]
                arrays = [a for a in scope.arrays if a[1] == t and a[3]]
                if arrays and (not scalars or rnd.random() < 0.5):
                    a = rnd.choice(arrays)
                    args.append(("arg_elem", a[0], self.index(scope, a[2])))
                elif scalars:
                    args.append(("arg_var", rnd.choice(scalars)))
                else:
                    return None
            elif rnd.random() < 0.25:
                args.append(("arg_val", ("tlit", t, self.value(t))))
            else:
                # note: the first element of an argument has the type of the
                #       parameter, a narrower one is rejected
                args.append(("arg_val", self.expr(scope, t, depth)))
        return ("call", f.name, args)

    # statements

    def destinations(self, scope):
        found = [(("var", s), t) for s, t, w in scope.scalars if w]
        for a in scope.arrays:
            if a[3]:
                found.append((("elem", a[0], self.index(scope, a[2])), a[1]))
        return found

    def statements(self, scope, depth, in_loop, can_return, count):
        found = []
        for _ in range(count):
            s = self.statement(scope, depth, in_loop, can_return)
            if s and self.allowed(scope, s):
                found.append(s)
                if s[0] == "var":
                    scope.scalars.append((s[1], s[2], True))
        return found

    def statement(self, scope, depth, in_loop, can_return):
        rnd = self.rnd
        c = rnd.random()
        destinations = self.destinations(scope)
        if c < 0.35 and destinations:
            destination, t = rnd.choice(destinations)
            return ("assign", destination,
                    self.expr(scope, self.narrow(scope, t), self.depth))
        if c < 0.45:
            t = rnd.choice(self.available_types(scope))
            return ("var", self.fresh("t"), t, self.expr(scope, t, self.depth))
        if c < 0.55 and depth > 0:
            condition = self.condition(scope, 1)
            then = self.statements(scope.copy(), depth - 1, in_loop,
                                   can_return, rnd.randint(1, 2))
            otherwise = self.statements(scope.copy(), depth - 1, in_loop,
                                        can_return, rnd.randint(1, 2)) \
                if rnd.random() < 0.4 else None
            return ("if", condition, then, otherwise)
        if c < 0.68 and depth > 0:
            j = self.fresh("j")
            start = rnd.choice([0] * 6 + [1, -1])
            inner = scope.copy()
            inner.counters.append(j)
            inner.scalars.append((j, "i32", False))
            body = self.statements(inner, depth - 1, True, can_return,
                                   rnd.randint(1, 3))
            return ("loop", j, start, rnd.randint(1, 3), body)
        if c < 0.72 and depth > 0 and not scope.foo_array and scope.arrays:
            a = rnd.choice(scope.arrays)
            inner = scope.copy()
            inner.foo_array = a[0]
            inner.counters.append("i")
            inner.scalars.append(("e", a[1], a[3]))
            count = self.index(scope, 3) if rnd.random() < 0.4 else None
            body = self.statements(inner, depth - 1, True, can_return,
                                   rnd.randint(1, 3))
            return ("foo", a[0], count, body)
        if c < 0.76 and in_loop:
            return ("jump", self.condition(scope, 1),
                    rnd.choice(["break", "continue"]))
        if c < 0.79 and rnd.random() < 0.25:
            # note: the argument of 'exit' has the default type, whose width
            #       depends on the target, so it is one value of 'i32'
            i32s = [v for v, t, _ in scope.scalars if t == "i32"]
            return ("exit", self.condition(scope, 1),
                    ("var", rnd.choice(i32s)) if i32s else
                    ("tlit", "i32", rnd.randint(0, 255)))
        if c < 0.82 and can_return:
            return ("ret", self.condition(scope, 1))
        if c < 0.9:
            functions = [f for f in self.callable() if f.res is None]
            if not functions:
                return None
            call = self.call(scope, rnd.choice(functions), 1)
            return ("callst", call) if call else None
        writable = [a for a in scope.arrays if a[3]]
        if not writable:
            return None
        dst = rnd.choice(writable)
        src = rnd.choice([a for a in scope.arrays if a[1] == dst[1]])
        return ("copy", src[0], self.index(scope, src[2]), dst[0],
                self.index(scope, dst[2]), self.index(scope, 3))

    # definitions

    def globals(self):
        rnd = self.rnd
        for _ in range(rnd.randint(0, 3)):
            t = rnd.choice(TYPES)
            if rnd.random() < 0.5:
                g = self.fresh("g")
                self.program.global_decls.append(("sc", g, t, self.value(t)))
                self.global_scope.scalars.append((g, t, True))
            else:
                g = self.fresh("ga")
                n = rnd.randint(2, 4)
                self.program.global_decls.append(
                    ("arr", g, t, [self.value(t) for _ in range(n)]))
                self.global_scope.arrays.append((g, t, n, True))

    def reached_globals(self, node):
        # the globals that a part of the tree names or reaches through calls
        names = {decl[1] for decl in self.program.global_decls}
        found = set()

        def walk(part):
            if isinstance(part, str):
                if part in names:
                    found.add(part)
            elif isinstance(part, (tuple, list)):
                if part and part[0] == "call":
                    found.update(self.by_name[part[1]].globals)
                for x in part:
                    walk(x)

        walk(node)
        return found

    def function(self, number):
        rnd = self.rnd
        params = []
        for p in range(rnd.randint(1, 4)):
            params.append((f"f{number}p{p}", rnd.choice(["arr", "sc", "sc"]),
                           rnd.choice(TYPES), rnd.random() < 0.5))
        f = Function(f"f{number}", params, rnd.choice(TYPES + (None,)))
        self.current = f
        scope = Scope()
        if rnd.random() < 0.6:
            scope.scalars += self.global_scope.scalars
            scope.arrays += self.global_scope.arrays
        for name, kind, t, mut in params:
            if kind == "arr":
                scope.arrays.append((name, t, None, mut))
            else:
                scope.scalars.append((name, t, mut))
        if rnd.random() < 0.2:
            # note: a frame larger than the 12-bit offset of rv32i
            f.body.append(("pad", f"fpad{number}", rnd.randint(2100, 9000)))
        if f.res:
            f.body.append(self.first_result(scope, f.res))
            scope.scalars.append(("res", f.res, True))
        f.body += self.statements(scope, 2, False, True, rnd.randint(2, 5))
        f.globals = self.reached_globals(f.body)
        self.current = None
        self.program.functions.append(f)
        self.by_name[f.name] = f

    def first_result(self, scope, t):
        # the assignment of 'res' that starts a body, every path assigns it
        for _ in range(3):
            statement = ("assign", ("var", "res"), self.expr(scope, t, 1))
            if self.allowed(scope, statement):
                return statement
        return ("assign", ("var", "res"), ("tlit", t, self.value(t)))

    def main_call(self, scope, body):
        # a call of a drawn function: its result assigned or printed, the
        # call alone or in a loop
        rnd = self.rnd
        f = rnd.choice(self.program.functions)
        destinations = [d for d in self.destinations(scope) if d[1] == f.res]
        destination = rnd.choice(destinations)[0] \
            if destinations and rnd.random() < 0.5 else None
        call = self.call(scope, f, self.depth)
        if call is None:
            return
        if destination:
            statement = ("assign", destination, call)
        elif f.res:
            statement = ("var", self.fresh("r"), f.res, call)
        else:
            statement = ("callst", call)
        if not self.allowed(scope, statement):
            return
        if rnd.random() < 0.3 and statement[0] != "var":
            j = self.fresh("j")
            inner = scope.copy()
            inner.counters.append(j)
            inner.scalars.append((j, "i32", False))
            loop_body = [statement]
            jump = ("jump", self.condition(inner, 1),
                    rnd.choice(["break", "continue"]))
            if rnd.random() < 0.6 and self.allowed(inner, jump):
                loop_body.insert(rnd.randint(0, 1), jump)
            loop_body.append(("pr", ("var", rnd.choice(scope.scalars)[0])))
            body.append(("loop", j, 0, rnd.randint(1, 3), loop_body))
        else:
            body.append(statement)
            if statement[0] == "var":
                scope.scalars.append((statement[1], f.res, True))
                body.append(("pr", ("var", statement[1])))
        body.append(("pr", ("var", rnd.choice(scope.scalars)[0])))

    def main(self):
        rnd = self.rnd
        scope = Scope()
        scope.scalars += self.global_scope.scalars
        scope.arrays += self.global_scope.arrays
        if rnd.random() < 0.3:
            # note: variables past the 12-bit offset of rv32i
            self.program.main_locals.append(
                ("arr", "pad", "i8", [0] * rnd.randint(2100, 9000)))
        for t in TYPES:
            for _ in range(rnd.randint(1, 2)):
                a = self.fresh("a")
                n = rnd.randint(2, 5)
                values = [self.value(t) if rnd.random() < 0.7 else 0
                          for _ in range(n)]
                self.program.main_locals.append(("arr", a, t, values))
                scope.arrays.append((a, t, n, True))
            for _ in range(rnd.randint(1, 2)):
                v = self.fresh("v")
                self.program.main_locals.append(("sc", v, t, self.value(t)))
                scope.scalars.append((v, t, True))
        body = self.program.main_body
        for _ in range(rnd.randint(3, 6)):
            self.main_call(scope, body)
        # the values the program ends with
        for name, _, _ in scope.scalars:
            body.append(("pr", ("var", name)))
        for name, _, length, _ in scope.arrays:
            for i in range(length):
                body.append(("pr", ("elem", name, ("ix", "c", None, i))))

    def generate(self):
        self.globals()
        for number in range(self.rnd.randint(2, 4)):
            self.function(number)
        self.main()
        self.program.drawn_noinline = {f.name for f in self.program.functions
                                       if self.rnd.random() < 0.5}
        return self.program


# the text of a program


def index_text(ix):
    _, kind, name, c = ix
    if kind == "c":
        return str(c)
    if kind == "v":
        return name
    if kind == "and":
        return f"{name} & 1"
    if kind == "call":
        return f"{expr_text(name)} & 1"
    return f"{name} + {c}" if c > 0 else f"{name} - {-c}"


def expr_text(e):
    kind = e[0]
    if kind == "var":
        return e[1]
    if kind == "neg":
        return f"(-{e[1]})"
    if kind == "un":
        return f"({e[1]}{expr_text(e[2])})"
    if kind == "elem":
        return f"{e[1]}[{index_text(e[2])}]"
    if kind == "call":
        return f"{e[1]}({', '.join(arg_text(a) for a in e[2])})"
    if kind == "conv":
        return f"{e[1]}({expr_text(e[2])})"
    if kind == "tlit":
        return f"{e[1]}({literal_text(e[2])})"
    if kind == "lit":
        return literal_text(e[1])
    assert kind == "bin"
    return f"{fl.group(expr_text(e[2]))} {e[1]} {fl.group(expr_text(e[3]))}"


def condition_text(c):
    if c[0] == "cmp":
        return f"{fl.group(expr_text(c[2]))} {c[1]} " \
            f"{fl.group(expr_text(c[3]))}"
    return f"({condition_text(c[1])}) {c[0]} ({condition_text(c[2])})"


def arg_text(a):
    if a[0] in ("arg_arr", "arg_var"):
        return a[1]
    if a[0] == "arg_elem":
        return f"{a[1]}[{index_text(a[2])}]"
    return expr_text(a[1])


def statements_text(statements, indent, lines):
    sp = "    " * indent
    for s in statements:
        kind = s[0]
        if kind == "assign":
            lines.append(f"{sp}{expr_text(s[1])} = {expr_text(s[2])}")
        elif kind == "pad":
            lines.append(f"{sp}var {s[1]} = i8[{s[2]}]")
        elif kind == "var":
            lines.append(f"{sp}var {s[1]} = {expr_text(s[3])}")
        elif kind == "if":
            lines.append(f"{sp}if {condition_text(s[1])} {{")
            statements_text(s[2], indent + 1, lines)
            if s[3] is not None:
                lines.append(f"{sp}}} else {{")
                statements_text(s[3], indent + 1, lines)
            lines.append(f"{sp}}}")
        elif kind == "loop":
            _, j, start, bound, body = s
            lines.append(f"{sp}var {j} = i32({literal_text(start)})")
            lines.append(f"{sp}loop {{")
            lines.append(f"{sp}    if {j} >= {bound} break")
            lines.append(f"{sp}    {j} = {j} + 1")
            statements_text(body, indent + 1, lines)
            lines.append(f"{sp}}}")
        elif kind == "foo":
            count = f", {index_text(s[2])}" if s[2] else ""
            lines.append(f"{sp}foo {s[1]}{count} {{")
            statements_text(s[3], indent + 1, lines)
            lines.append(f"{sp}}}")
        elif kind == "jump":
            lines.append(f"{sp}if {condition_text(s[1])} {s[2]}")
        elif kind == "exit":
            lines.append(f"{sp}if {condition_text(s[1])} {{")
            lines.append(f"{sp}    exit({expr_text(s[2])})")
            lines.append(f"{sp}}}")
        elif kind == "ret":
            lines.append(f"{sp}if {condition_text(s[1])} return")
        elif kind == "callst":
            lines.append(f"{sp}{expr_text(s[1])}")
        elif kind == "copy":
            _, src, src_index, dst, dst_index, count = s
            lines.append(f"{sp}array_copy({src}[{index_text(src_index)}], "
                         f"{dst}[{index_text(dst_index)}], "
                         f"{index_text(count)})")
        else:
            assert kind == "pr"
            lines.append(f"{sp}pr(i32({expr_text(s[1])}))")


# prints a value as 8 letters 'a' to 'p', one per 4 bits from the top
PR = """func noinline pr(v i32) {
    var b = i8[9]
    var k = i32(0)
    loop {
        if k == 8 break
        var d = (v >> (28 - k * 4)) & 15
        b[k] = i8('a' + d)
        k = k + 1
    }
    b[8] = '\\n'
    write(1, b)
}
"""


def program_text(program, noinline):
    # 'noinline' the names of the functions written 'noinline'
    lines = []
    for kind, name, t, v in program.global_decls:
        if kind == "sc":
            lines.append(f"dat {name} = {t}({literal_text(v)})")
        else:
            values = ", ".join(literal_text(x) for x in v)
            lines.append(f"dat {name} = {t}[{len(v)}]{{{values}}}")
    lines.append(PR)
    for f in program.functions:
        params = []
        for name, kind, t, mut in f.params:
            word = "mut " if mut else ""
            params.append(f"{name} {word}{t}[]" if kind == "arr"
                          else f"{name} {word}{t}")
        res = f" res {f.res}" if f.res else ""
        word = "noinline " if f.name in noinline else ""
        lines.append(f"func {word}{f.name}({', '.join(params)}){res} {{")
        statements_text(f.body, 1, lines)
        lines.append("}\n")
    lines.append("func main() {")
    for kind, name, t, v in program.main_locals:
        if kind == "sc":
            lines.append(f"    var {name} = {t}({literal_text(v)})")
            continue
        lines.append(f"    var {name} = {t}[{len(v)}]")
        for i, x in enumerate(v):
            if x:
                lines.append(f"    {name}[{i}] = {literal_text(x)}")
    statements_text(program.main_body, 1, lines)
    lines.append("}")
    return "\n".join(lines) + "\n"


# the model


class Cell:
    # a scalar: a variable or an element, at 'pos' of the list 'store'
    def __init__(self, store, pos, t):
        self.store = store
        self.pos = pos
        self.t = t

    def get(self):
        return self.store[self.pos]

    def set(self, v):
        self.store[self.pos] = v


class Array:
    def __init__(self, values, t):
        self.values = values
        self.t = t


def storage(kind, value, t):
    return Cell([value], 0, t) if kind == "sc" else Array(list(value), t)


class Model:
    def __init__(self, program):
        self.program = program
        self.functions = {f.name: f for f in program.functions}
        self.out = []
        self.steps = 0
        self.globals = {name: storage(kind, v, t)
                        for kind, name, t, v in program.global_decls}

    def look(self, scope, name):
        return scope[name] if name in scope else self.globals[name]

    def index(self, scope, ix):
        _, kind, name, c = ix
        if kind == "c":
            return c
        if kind == "call":
            return self.call(scope, name) & 1
        v = self.look(scope, name).get()
        if kind == "v":
            return v
        if kind == "and":
            return v & 1
        return v + c

    def elem(self, scope, name, ix):
        a = self.look(scope, name)
        i = self.index(scope, ix)
        if i < 0 or i >= len(a.values):
            raise Panic("bounds")
        return Cell(a.values, i, a.t)

    def type_of(self, scope, e):
        kind = e[0]
        if kind in ("var", "neg", "elem"):
            return self.look(scope, e[1]).t
        if kind in ("un", "bin"):
            return self.type_of(scope, e[2])
        if kind == "call":
            return self.functions[e[1]].res
        if kind in ("conv", "tlit"):
            return e[1]
        assert kind == "lit"
        return None

    def fit(self, t, v, wrap):
        if wrap:
            return fl.wrap(t, v)
        if not fl.fits(t, v):
            raise Panic("overflow")
        return v

    def value(self, scope, e, w=None, wrap=False):
        # 'w' the type the expression is computed at, 'wrap' inside a
        # conversion, whose arithmetic wraps
        kind = e[0]
        if kind == "var":
            return self.look(scope, e[1]).get()
        if kind == "neg":
            cell = self.look(scope, e[1])
            return self.fit(w or cell.t, -cell.get(), wrap)
        if kind == "elem":
            return self.elem(scope, e[1], e[2]).get()
        if kind == "un":
            w = w or self.type_of(scope, e[2])
            v = self.value(scope, e[2], w, wrap)
            return self.fit(w, -v, wrap) if e[1] == "-" else ~v
        if kind == "call":
            return self.call(scope, e)
        if kind == "conv":
            u = self.type_of(scope, e[2])
            inner = e[1] if WIDTH[e[1]] > WIDTH[u] else u
            return fl.wrap(e[1], self.value(scope, e[2], inner, True))
        if kind in ("tlit", "lit"):
            return e[-1]
        assert kind == "bin"
        w = w or self.type_of(scope, e[2])
        a = self.value(scope, e[2], w, wrap)
        b = self.value(scope, e[3], w, wrap)
        return self.operation(e[1], a, b, w, wrap)

    def operation(self, op, a, b, t, wrap):
        if op == "+":
            return self.fit(t, a + b, wrap)
        if op == "-":
            return self.fit(t, a - b, wrap)
        if op == "*":
            return self.fit(t, a * b, wrap)
        if op in ("/", "%"):
            low, _ = fl.limits(t)
            if b == 0 or (a == low and b == -1):
                raise Panic("division")
            q = fl.trunc_div(a, b)
            return q if op == "/" else a - q * b
        if op == "&":
            return a & b
        if op == "|":
            return a | b
        if op == "^":
            return a ^ b
        if b < 0 or b >= WIDTH[t]:
            raise Panic("shift")
        if op == "<<":
            return fl.wrap(t, a << b)
        return a >> b

    def condition(self, scope, c):
        if c[0] == "cmp":
            a = self.value(scope, c[2])
            b = self.value(scope, c[3])
            return {"<": a < b, ">": a > b, "==": a == b, "!=": a != b,
                    "<=": a <= b, ">=": a >= b}[c[1]]
        if c[0] == "and":
            return self.condition(scope, c[1]) and \
                self.condition(scope, c[2])
        return self.condition(scope, c[1]) or self.condition(scope, c[2])

    def call(self, scope, e):
        # the arguments left to right, an array and a 'mut' scalar by
        # reference
        f = self.functions[e[1]]
        inner = {}
        for (name, _, t, _), a in zip(f.params, e[2]):
            if a[0] in ("arg_arr", "arg_var"):
                inner[name] = self.look(scope, a[1])
            elif a[0] == "arg_elem":
                inner[name] = self.elem(scope, a[1], a[2])
            else:
                inner[name] = Cell([self.value(scope, a[1], t)], 0, t)
        if f.res:
            inner["res"] = Cell([0], 0, f.res)
        try:
            self.run(inner, f.body)
        except Return:
            pass
        return inner["res"].get() if f.res else None

    def loop_body(self, scope, body):
        # whether the loop goes on after 'body'
        try:
            self.run(scope, body)
        except Break:
            return False
        except Continue:
            pass
        return True

    def run(self, scope, statements):
        for s in statements:
            self.steps += 1
            if self.steps > MODEL_STEPS:
                raise TooLong()
            self.statement(scope, s)

    def statement(self, scope, s):
        kind = s[0]
        if kind == "assign":
            # note: the index of the destination first, then the value
            destination = s[1]
            cell = self.look(scope, destination[1]) \
                if destination[0] == "var" else \
                self.elem(scope, destination[1], destination[2])
            cell.set(self.value(scope, s[2], cell.t))
        elif kind == "var":
            scope[s[1]] = Cell([self.value(scope, s[3])], 0, s[2])
        elif kind == "if":
            if self.condition(scope, s[1]):
                self.run(scope, s[2])
            elif s[3] is not None:
                self.run(scope, s[3])
        elif kind == "loop":
            _, j, start, bound, body = s
            scope[j] = Cell([start], 0, "i32")
            while scope[j].get() < bound:
                scope[j].set(scope[j].get() + 1)
                if not self.loop_body(scope, body):
                    break
        elif kind == "foo":
            a = self.look(scope, s[1])
            n = len(a.values)
            if s[2] is not None:
                n = self.index(scope, s[2])
                if n < 0 or n > len(a.values):
                    raise Panic("bounds")
            for i in range(n):
                scope["e"] = Cell(a.values, i, a.t)
                scope["i"] = Cell([i], 0, None)
                if not self.loop_body(scope, s[3]):
                    break
            scope.pop("e", None)
            scope.pop("i", None)
        elif kind == "jump":
            if self.condition(scope, s[1]):
                raise Break() if s[2] == "break" else Continue()
        elif kind == "exit":
            if self.condition(scope, s[1]):
                raise Exit(self.value(scope, s[2]) & 255)
        elif kind == "ret":
            if self.condition(scope, s[1]):
                raise Return()
        elif kind == "callst":
            self.call(scope, s[1])
        elif kind == "copy":
            self.copy(scope, s)
        elif kind == "pr":
            v = fl.wrap("i32", self.value(scope, s[1]))
            self.out.append("".join(chr(ord("a") + ((v >> (28 - k * 4)) & 15))
                                    for k in range(8)) + "\n")
        else:
            assert kind == "pad"

    def copy(self, scope, s):
        _, src, src_index, dst, dst_index, count = s
        a = self.look(scope, src)
        i = self.index(scope, src_index)
        if i < 0 or i > len(a.values):
            raise Panic("bounds")
        b = self.look(scope, dst)
        j = self.index(scope, dst_index)
        if j < 0 or j > len(b.values):
            raise Panic("bounds")
        n = self.index(scope, count)
        if n < 0 or i + n > len(a.values) or j + n > len(b.values):
            raise Panic("bounds")
        if a is b and i < j < i + n:
            raise Panic("overlap")
        b.values[j:j + n] = a.values[i:i + n]

    def outcome(self):
        # ('exit' or 'panic', code, output)
        scope = {name: storage(kind, v, t)
                 for kind, name, t, v in self.program.main_locals}
        try:
            self.run(scope, self.program.main_body)
        except Panic as p:
            return ("panic", fl.PANIC_CODE[p.name],
                    "".join(self.out).encode())
        except Exit as x:
            return ("exit", x.code, "".join(self.out).encode())
        return ("exit", 0, "".join(self.out).encode())


# running


def build(work, source, target, options):
    # what the program did: ('exit' or 'panic', code, stdout),
    # ('compiler-crash', stderr), ('compile', stderr) or how it did not end
    path = os.path.join(work, "f.baz")
    with open(path, "w") as f:
        f.write(fl.split_include(work, source))
    c, o, problem = fl.run_target(fl.COMPILER_ASAN, path, list(options), work,
                                  target)
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


def limit_of_registers(text):
    # a register that an operation fixes is a limit too (chapter 8 item 3)
    return fl.out_of_registers(text) or "is always used by the operation" \
        in text or "does not fit in the variable storage" in text


def rejection(text):
    # the message of a rejection without its position
    first = text.strip().splitlines()[0] if text.strip() else ""
    return first.split(": ", 1)[-1]


def folded(message):
    return any(word in message for word in FOLDED)


def short(o):
    return (o[0], o[1], o[2][-120:]) if len(o) == 3 else o


def judge(work, source, variant, expected, counts):
    # builds one variant of the program: (the findings, 'accepted' or the
    # message of the rejection)
    targets = fl.with_rotated(fl.TARGETS, source)
    found = []
    verdicts = set()
    for target in targets:
        option_lists = OPTION_LISTS if target in fl.TARGETS \
            else OPTION_LISTS[:1]
        for options in option_lists:
            got = build(work, source, target, options)
            where = f"{target} {variant} {' '.join(options)}"
            # a tool stopped by a limit says nothing about the code
            if got[0] == "skip":
                continue
            if got[0] == "compile" and limit_of_registers(got[1]):
                counts["out-of-registers"] = counts.get(
                    "out-of-registers", 0) + 1
                continue
            if got[0] == "compile":
                message = rejection(got[1])
                verdicts.add(message)
                if not folded(message) and \
                        not any(word in message for word in SHARED):
                    found.append(("versions-reject", f"{target} {message}",
                                  f"{where}: {message}"))
                continue
            verdicts.add("accepted")
            if got == expected:
                continue
            if got[0] == "compiler-crash":
                found.append((got[0], fl.signature(got[1]),
                              f"{where}\n{got[1]}"))
                continue
            category = got[0] if got[0] in ("compiler-hang", "invalid-asm",
                                            "link-failure", "image-mismatch") \
                else "versions-mismatch"
            found.append((category, f"{where} {got[0]}",
                          f"expected {short(expected)}, got {short(got)}"))
    if len(verdicts) > 1:
        found.append(("versions-reject", f"{variant} accepted by some builds",
                      f"{variant}: {sorted(verdicts)}"))
    # note: every build ran out of registers, as accepted
    verdict = verdicts.pop() if len(verdicts) == 1 else "accepted"
    return found, verdict


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    program = Generator(rnd).generate()
    # note: 'fuzz-coverage.py' sees the first variant, the one as drawn has
    #       both kinds of function
    variants = (("drawn", program.drawn_noinline), ("inline", set()),
                ("noinline", {f.name for f in program.functions}))
    try:
        expected = Model(program).outcome()
    except TooLong:
        return program_text(program, set()), [], "too-long"
    counts = {"exit" if expected[0] == "exit" else PANIC_NAME[expected[1]]: 1}
    verdicts = {}
    with tempfile.TemporaryDirectory(prefix="fuzz-") as work:
        for variant, noinline in variants:
            source = program_text(program, noinline)
            found, verdicts[variant] = judge(work, source, variant, expected,
                                             counts)
            # note: a finding keeps the text of the variant that found it
            if found:
                return source, found, counts
    if any(v != "accepted" for v in verdicts.values()):
        counts["rejected"] = 1
    # the rules of shared storage do not depend on inlining, a rejection for
    # a constant may come first in an inlined variant
    shared = {v for v in verdicts.values() if not folded(v)}
    if len(shared) > 1:
        detail = ", ".join(f"{k}: {v}" for k, v in verdicts.items())
        return program_text(program, set()), [
            ("versions-reject", "shared storage depends on inlining",
             detail)], counts
    return source, [], counts


def main():
    count, seed, jobs = fl.arguments(150)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
