#!/usr/bin/env python3
# model based fuzzer of control flow, scopes and functions, after the sections
# 'control flow', 'scopes' and 'functions' of 'etc/tutorial.baz'
#
# builds programs of 'if' / 'else if' / 'else' (with and without braces),
# 'loop' with 'break' and 'continue', nested blocks whose 'var' and 'let'
# shadow the outer ones, 'exit' from inside loops and functions, functions
# with a named result, early 'return', read-only arguments and 'noinline'
# functions. python runs the same program, the program mixes the values it
# computes into a checksum and ends with it as the exit code:
#
#   flow-mismatch   a build ends differently from the model
#   flow-reject     a generated program is rejected
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-flow.py [count] [seed] [jobs]
import random
import shutil
import sys
import tempfile

import fuzzlib as fl

COMPARES = ["<", "<=", "==", "!=", ">", ">="]
BITOPS = ["&", "|", "^"]


class Exit(Exception):
    def __init__(self, code):
        self.code = code


class Return(Exception):
    pass


class Break(Exception):
    pass


class Continue(Exception):
    pass


class Scope:
    def __init__(self, parent=None):
        self.parent = parent
        self.values = {}

    def find(self, name):
        scope = self
        while scope is not None:
            if name in scope.values:
                return scope
            scope = scope.parent
        raise KeyError(name)

    def get(self, name):
        return self.find(name).values[name]

    def set(self, name, value):
        self.find(name).values[name] = value


class Function:
    def __init__(self, name, params, body, noinline):
        self.name = name
        self.params = params
        self.body = body
        self.noinline = noinline


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.functions = []
        self.counter = 0
        self.globals = Scope()
        self.globals.values["h"] = 0
        self.read_only = {"h"}

    def fresh(self, prefix):
        self.counter += 1
        return f"{prefix}{self.counter}"

    # expressions: ('lit', n) ('var', name) ('bin', op, a, b) ('mul', a, b)
    # ('call', function, [args]); conditions: ('cmp', op, a, b) ('not', c)
    # ('and', a, b) ('or', a, b)

    def expr(self, names, depth, allow_call=True):
        rnd = self.rnd
        r = rnd.random()
        if depth <= 0 or r < 0.3:
            if names and rnd.random() < 0.7:
                return ("var", rnd.choice(names))
            return ("lit", rnd.randint(0, 9))
        if r < 0.45 and allow_call and self.functions:
            f = rnd.choice(self.functions)
            return ("call", f, [self.expr(names, depth - 1, False)
                                for _ in f.params])
        if r < 0.6:
            return ("mul", self.expr(names, depth - 1, allow_call),
                    self.expr(names, depth - 1, allow_call))
        return ("bin", rnd.choice(["+", "-"] + BITOPS),
                self.expr(names, depth - 1, allow_call),
                self.expr(names, depth - 1, allow_call))

    def cond(self, names, depth, allow_call=True):
        rnd = self.rnd
        r = rnd.random()
        if depth <= 0 or r < 0.5:
            return ("cmp", rnd.choice(COMPARES),
                    self.expr(names, 2, allow_call), self.expr(names, 1, False))
        if r < 0.65:
            return ("not", self.cond(names, depth - 1, allow_call))
        return (rnd.choice(["and", "or"]),
                self.cond(names, depth - 1, allow_call),
                self.cond(names, depth - 1, allow_call))

    def render(self, e):
        kind = e[0]
        if kind == "lit":
            return str(e[1])
        if kind == "var":
            return e[1]
        if kind == "bin":
            return f"(({self.render(e[2])} {e[1]} {self.render(e[3])}) & 1023)"
        if kind == "mul":
            return (f"(({self.render(e[1])} & 31) * "
                    f"({self.render(e[2])} & 31))")
        if kind == "call":
            return f"{e[1].name}({', '.join(self.render(a) for a in e[2])})"
        if kind == "cmp":
            return f"({self.render(e[2])} {e[1]} {self.render(e[3])})"
        if kind == "not":
            return f"not {self.group(e[1])}"
        return f"{self.group(e[1])} {e[0]} {self.group(e[2])}"

    def group(self, e):
        # 'and' and 'or' are never mixed without parentheses
        text = self.render(e)
        return f"({text})" if e[0] in ("and", "or") else text

    def eval(self, e, scope):
        kind = e[0]
        if kind == "lit":
            return e[1]
        if kind == "var":
            return scope.get(e[1])
        if kind == "bin":
            a = self.eval(e[2], scope)
            b = self.eval(e[3], scope)
            r = {"+": a + b, "-": a - b, "&": a & b, "|": a | b,
                 "^": a ^ b}[e[1]]
            return r & 1023
        if kind == "mul":
            return (self.eval(e[1], scope) & 31) * (self.eval(e[2], scope) & 31)
        if kind == "call":
            return self.call(e[1], [self.eval(a, scope) for a in e[2]])
        if kind == "cmp":
            a = self.eval(e[2], scope)
            b = self.eval(e[3], scope)
            return {"<": a < b, "<=": a <= b, "==": a == b, "!=": a != b,
                    ">": a > b, ">=": a >= b}[e[1]]
        if kind == "not":
            return not self.eval(e[1], scope)
        if kind == "and":
            return bool(self.eval(e[1], scope) and self.eval(e[2], scope))
        return bool(self.eval(e[1], scope) or self.eval(e[2], scope))

    def call(self, f, args):
        scope = Scope(self.globals)
        for p, a in zip(f.params, args):
            scope.values[p] = a
        scope.values["res"] = 0
        try:
            self.run_block(f.body, scope)
        except Return:
            pass
        return scope.values["res"]

    # statements: ('assign', name, expr) ('observe', expr) ('res', expr)
    # ('if', [(cond, body)...], else_body) ('loop', counter, limit, body)
    # ('block', body) ('var', name, expr) ('let', name, expr)
    # ('callvar', name, call) ('break', cond) ('continue', cond)
    # ('return', cond) ('exit', cond, code)

    def block(self, names, writable, depth, in_loop, in_function):
        names = list(names)
        writable = list(writable)
        body = []
        for _ in range(self.rnd.randint(1, 4)):
            s = self.statement(names, writable, depth, in_loop, in_function)
            body.append(s)
            if s[0] in ("var", "callvar"):
                names.append(s[1])
                writable.append(s[1])
            elif s[0] == "let":
                names.append(s[1])
        return body

    def statement(self, names, writable, depth, in_loop, in_function):
        rnd = self.rnd
        r = rnd.random()
        if r < 0.18 and writable:
            # a call whose argument is the destination is rejected as aliasing
            return ("assign", rnd.choice(writable), self.expr(names, 2, False))
        if r < 0.32:
            if in_function:
                # note: a function leaves the checksum alone, the order in
                #       which the calls of an expression run is not defined
                return ("res", ("bin", "^", ("var", "res"),
                                self.expr(names, 2, False)))
            return ("observe", self.expr(names, 2))
        if r < 0.36 and in_function:
            return ("res", self.expr(names, 2, False))
        if r < 0.5 and depth > 0:
            branches = [(self.cond(names, 2),
                         self.block(names, writable, depth - 1, in_loop,
                                    in_function))
                        for _ in range(rnd.randint(1, 3))]
            other = None
            if rnd.random() < 0.5:
                other = self.block(names, writable, depth - 1, in_loop,
                                   in_function)
            return ("if", branches, other)
        if r < 0.62 and depth > 0:
            counter = self.fresh("c")
            body = self.block(names + [counter], writable, depth - 1, True,
                              in_function)
            return ("loop", counter, rnd.randint(1, 6), body)
        if r < 0.68 and depth > 0:
            return ("block", self.block(names, writable, depth - 1, in_loop,
                                        in_function))
        if r < 0.76:
            return ("var", self.fresh("w"), self.expr(names, 2, False))
        if r < 0.82:
            return ("let", self.fresh("k"), self.expr(names, 2, False))
        if r < 0.88 and in_loop:
            return (rnd.choice(["break", "continue"]),
                    self.cond(names, 1, False))
        if r < 0.92 and in_function:
            return ("return", self.cond(names, 1, False))
        if r < 0.94:
            return ("exit", self.cond(names, 1, False), rnd.randint(1, 100))
        if self.functions:
            f = rnd.choice(self.functions)
            call = ("call", f, [self.expr(names, 1, False) for _ in f.params])
            return ("callvar", self.fresh("t"), call)
        if in_function:
            return ("res", self.expr(names, 1, False))
        return ("observe", self.expr(names, 1))

    # rendering and running statements

    def show_block(self, body, indent):
        lines = []
        for s in body:
            lines += self.show(s, indent)
        return lines

    def show(self, s, indent):
        pad = "    " * indent
        kind = s[0]
        if kind == "assign":
            return [f"{pad}{s[1]} = {self.render(s[2])}"]
        if kind == "observe":
            return [f"{pad}h = ((h ^ {self.render(s[1])}) * 31) & 0xffff"]
        if kind == "res":
            return [f"{pad}res = {self.render(s[1])}"]
        if kind in ("var", "callvar"):
            return [f"{pad}var {s[1]} = {self.render(s[2])}"]
        if kind == "let":
            return [f"{pad}let {s[1]} = {self.render(s[2])}"]
        if kind == "block":
            return [pad + "{"] + self.show_block(s[1], indent + 1) + [pad + "}"]
        if kind == "if":
            lines = []
            for i, (c, body) in enumerate(s[1]):
                lead = "if" if i == 0 else "} else if"
                lines.append(f"{pad}{lead} {self.render(c)} {{")
                lines += self.show_block(body, indent + 1)
            if s[2] is not None:
                lines.append(f"{pad}}} else {{")
                lines += self.show_block(s[2], indent + 1)
            lines.append(pad + "}")
            return lines
        if kind == "loop":
            lines = [f"{pad}var {s[1]} = 0", pad + "loop {",
                     f"{pad}    {s[1]} = {s[1]} + 1",
                     f"{pad}    if {s[1]} > {s[2]} break"]
            lines += self.show_block(s[3], indent + 1)
            lines.append(pad + "}")
            return lines
        if kind in ("break", "continue"):
            return [f"{pad}if {self.render(s[1])} {kind}"]
        if kind == "return":
            return [f"{pad}if {self.render(s[1])} return"]
        return [f"{pad}if {self.render(s[1])} exit({s[2]})"]

    def run_block(self, body, scope):
        inner = Scope(scope)
        for s in body:
            self.run(s, inner)

    def run(self, s, scope):
        kind = s[0]
        if kind == "assign":
            scope.set(s[1], self.eval(s[2], scope))
        elif kind == "observe":
            v = self.eval(s[1], scope)
            scope.set("h", ((scope.get("h") ^ v) * 31) & 0xffff)
        elif kind == "res":
            scope.set("res", self.eval(s[1], scope))
        elif kind in ("var", "let", "callvar"):
            scope.values[s[1]] = self.eval(s[2], scope)
        elif kind == "block":
            self.run_block(s[1], scope)
        elif kind == "if":
            for c, body in s[1]:
                if self.eval(c, scope):
                    self.run_block(body, scope)
                    return
            if s[2] is not None:
                self.run_block(s[2], scope)
        elif kind == "loop":
            inner = Scope(scope)
            inner.values[s[1]] = 0
            while True:
                inner.values[s[1]] += 1
                if inner.values[s[1]] > s[2]:
                    break
                try:
                    self.run_block(s[3], inner)
                except Break:
                    break
                except Continue:
                    continue
        elif kind == "break":
            if self.eval(s[1], scope):
                raise Break()
        elif kind == "continue":
            if self.eval(s[1], scope):
                raise Continue()
        elif kind == "return":
            if self.eval(s[1], scope):
                raise Return()
        elif self.eval(s[1], scope):
            raise Exit(s[2])

    # program

    def program(self, limit=None):
        rnd = self.rnd
        lines = ["dat h = 0"]
        for i in range(rnd.randint(0, 3)):
            params = [f"p{j}" for j in range(rnd.randint(1, 3))]
            body = self.block(params + ["res"], [], 2, False, True)
            f = Function(f"f{i}", params, body, rnd.random() < 0.4)
            self.functions.append(f)
            word = "noinline " if f.noinline else ""
            lines.append(f"func {word}{f.name}({', '.join(params)}) res {{")
            lines.append("    res = 0")
            lines += self.show_block(body, 1)
            lines.append("}")
        names = ["v0", "v1", "v2"]
        body = self.block(names, names, 3, False, False)

        # the first statements of main only, to find the one that goes wrong
        if limit is not None:
            body = body[:limit]

        lines.append("func main() {")
        lines += ["    var v0 = 3", "    var v1 = 5", "    var v2 = 7"]
        lines += self.show_block(body, 1)
        lines.append("    exit((h ^ v0 ^ v1 ^ v2) & 127)")
        lines.append("}")
        scope = Scope(self.globals)
        scope.values.update({"v0": 3, "v1": 5, "v2": 7})
        try:
            self.run_block(body, scope)
            code = (self.globals.values["h"] ^ scope.values["v0"]
                    ^ scope.values["v1"] ^ scope.values["v2"]) & 127
        except Exit as e:
            code = e.code
        return "\n".join(lines) + "\n", ("exit", code, b"")


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    source, expected = Generator(rnd).program()
    work = tempfile.mkdtemp(prefix="fuzzf-")
    try:
        found = fl.judge(work, source, expected, prefix="flow")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def bisect(seed, index):
    # the least number of statements of main after which the builds and the
    # model differ, for the program 'index' of 'seed'
    work = tempfile.mkdtemp(prefix="fuzzf-")
    try:
        for limit in range(1, 40):
            rnd = random.Random(seed * 1000003 + index)
            source, expected = Generator(rnd).program(limit)
            found = fl.judge(work, source, expected, prefix="flow",
                             option_lists=(("--checks=noub",),))
            if found:
                print(f"{limit} statements of main:\n{source}\n{found[0][2]}")
                return 1
        print("no difference")
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    # 'fuzz-flow.py bisect SEED INDEX' finds the statement of main that goes
    # wrong in a program that was reported
    if len(sys.argv) == 4 and sys.argv[1] == "bisect":
        return bisect(int(sys.argv[2]), int(sys.argv[3]))

    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
