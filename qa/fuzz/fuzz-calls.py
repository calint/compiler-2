#!/usr/bin/env python3
# model based fuzzer of calls in every context, after the section 'functions'
# of 'etc/tutorial.baz'
#
# a function result is the destination of the statements of an inlined body, a
# register when the call is an argument, an index or a condition, a variable
# when it initializes one. the statements read and write the result in many
# shapes ('res = (p + res) * 3', 'res = 3 - res', 'res = p * res + p'), and
# the checks, the optimizer and the target change the code of each. this
# builds functions with such bodies, 'noinline' ones, ones that call earlier
# ones, and calls them as the value of a variable, an assignment, an argument,
# an index, a shift count, a divisor, an operand of a comparison, a condition,
# the count of 'array_copy', the argument of another call and of 'exit'.
# python runs the same calls, the program must end with the same exit code:
#
#   calls-mismatch   a build ends differently from the model
#   calls-reject     a generated program is rejected
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-calls.py [count] [seed] [jobs]
import random
import shutil
import sys
import tempfile

import fuzzlib as fl

OPS = ["+", "-", "*", "&", "|", "^"]


class Function:
    def __init__(self, name, body, noinline):
        self.name = name
        self.body = body
        self.noinline = noinline


def apply(op, a, b):
    return {"+": a + b, "-": a - b, "*": a * b, "&": a & b, "|": a | b,
            "^": a ^ b}[op]


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.functions = []
        self.lines = []
        self.acc = 0
        self.names = 0

    def fresh(self, prefix):
        self.names += 1
        return f"{prefix}{self.names}"

    def emit(self, text):
        self.lines.append("    " + text)

    # expressions over p, q and res: ('lit', n) ('var', name) ('bin', op, a, b)
    # ('call', function, [args])

    def expr(self, names, depth, calls=False):
        rnd = self.rnd
        r = rnd.random()
        if depth <= 0 or r < 0.3:
            if rnd.random() < 0.7:
                return ("var", rnd.choice(names))
            return ("lit", rnd.randint(0, 9))
        if calls and self.functions and r < 0.45:
            f = rnd.choice(self.functions)
            return ("call", f, [self.expr(names, depth - 1) for _ in range(2)])
        op = rnd.choice(OPS)
        lhs = self.expr(names, depth - 1, calls)
        rhs = self.expr(names, depth - 1, calls)
        if op == "*":
            # note: masked so that a product of products fits in 32 bits
            lhs = ("bin", "&", lhs, ("lit", 255))
            rhs = ("bin", "&", rhs, ("lit", 255))
        return ("bin", op, lhs, rhs)

    def show(self, e):
        if e[0] == "lit":
            return str(e[1])
        if e[0] == "var":
            return e[1]
        if e[0] == "call":
            return f"{e[1].name}({', '.join(self.show(a) for a in e[2])})"
        return f"({self.show(e[2])} {e[1]} {self.show(e[3])})"

    def eval(self, e, env):
        if e[0] == "lit":
            return e[1]
        if e[0] == "var":
            return env[e[1]]
        if e[0] == "call":
            return self.call(e[1], [self.eval(a, env) for a in e[2]])
        return apply(e[1], self.eval(e[2], env), self.eval(e[3], env))

    def call(self, f, args):
        env = {"p": args[0], "q": args[1], "res": 0}
        for target, expr in f.body:
            env[target] = self.eval(expr, env)
        return env["res"]

    # the values stay small: the operands are masked where they could grow

    def make_function(self, index):
        rnd = self.rnd
        body = []
        for _ in range(rnd.randint(1, 4)):
            # the shapes that read the result: constant first, result last,
            # result twice, a parenthesis around it
            shape = rnd.randrange(6)
            c = rnd.randint(1, 9)
            if shape == 0:
                e = ("bin", "*", ("bin", "+", ("var", "p"), ("var", "res")),
                     ("lit", c))
            elif shape == 1:
                e = ("bin", "-", ("lit", c), ("var", "res"))
            elif shape == 2:
                e = ("bin", "+", ("bin", "*", ("var", "p"), ("var", "res")),
                     ("var", "q"))
            elif shape == 3:
                e = ("bin", "^", ("var", "res"), ("bin", "&", ("var", "p"),
                                                  ("lit", c)))
            elif shape == 4:
                e = ("bin", "+", ("var", "res"), ("var", "res"))
            else:
                e = self.expr(["p", "q", "res"], 3, calls=True)
            # note: masked so that a chain of shapes cannot overflow
            e = ("bin", "&", e, ("lit", 1023))
            body.append(("res", e))
        f = Function(f"f{index}", body, rnd.random() < 0.3)
        self.functions.append(f)
        word = "noinline " if f.noinline else ""
        head = [f"func {word}{f.name}(p, q) res {{", "    res = 0"]
        for target, e in body:
            head.append(f"    {target} = {self.show(e)}")
        head.append("}")
        return head

    def mix(self, text, value):
        self.emit(f"acc = (acc * 31 + ({text})) & 0xffff")
        self.acc = (self.acc * 31 + value) & 0xffff

    def program(self):
        rnd = self.rnd
        head = []
        for i in range(rnd.randint(1, 3)):
            head += self.make_function(i)
        self.emit("var acc = 0")
        self.emit("var a = 3")
        self.emit("var b = 5")
        self.emit("var arr = []{1, 2, 4, 8}")
        self.emit("var dst = [4]")
        env = {"a": 3, "b": 5}
        arr = [1, 2, 4, 8]
        for _ in range(rnd.randint(4, 10)):
            f = rnd.choice(self.functions)
            args = [rnd.choice([("var", "a"), ("var", "b"), ("lit", rnd.randint(0, 9))])
                    for _ in range(2)]
            call = ("call", f, args)
            text = self.show(call)
            v = self.eval(call, env)
            context = rnd.choice(["var", "assign", "mix", "index", "shift",
                                  "divisor", "compare", "condition", "nested",
                                  "copy", "operand"])
            if context == "var":
                name = self.fresh("v")
                self.emit(f"var {name} = {text}")
                self.mix(name, v)
            elif context == "assign":
                name = self.fresh("v")
                self.emit(f"var {name} = 0")
                self.emit(f"{name} = {text} + 1")
                self.mix(name, v + 1)
            elif context == "mix":
                self.mix(text, v)
            elif context == "index":
                self.mix(f"arr[{text} & 3]", arr[v & 3])
            elif context == "shift":
                self.mix(f"i32(1) << ({text} & 7)", 1 << (v & 7))
            elif context == "divisor":
                self.mix(f"1000 / ({text} + 1)", 1000 // (v + 1))
            elif context == "compare":
                c = rnd.randint(0, 600)
                name = self.fresh("q")
                self.emit(f"var {name} = {text} > {c}")
                self.emit(f"if {name} acc = acc + 1")
                if v > c:
                    self.acc = self.acc + 1
            elif context == "condition":
                c = rnd.randint(0, 600)
                self.emit(f"if {text} < {c} acc = (acc * 3 + 1) & 0xffff")
                if v < c:
                    self.acc = (self.acc * 3 + 1) & 0xffff
            elif context == "nested":
                g = rnd.choice(self.functions)
                inner = ("call", g, [args[1], args[0]])
                outer = ("call", f, [inner, args[0]])
                self.mix(self.show(outer), self.eval(outer, env))
            elif context == "copy":
                n = v & 3
                self.emit(f"array_copy(arr, dst, {text} & 3)")
                self.mix("dst[0] + dst[1] + dst[2] + dst[3]",
                         sum(arr[:n]))
                self.emit("dst = [4]")
            else:
                self.mix(f"({text} + a) * 2 - ({text} ^ b)",
                         (v + 3) * 2 - (v ^ 5))
        self.emit("exit(acc & 127)")
        source = "\n".join(head) + "\nfunc main() {\n" + \
            "\n".join(self.lines) + "\n}\n"
        return source, ("exit", self.acc & 127, b"")


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    source, expected = Generator(rnd).program()
    work = tempfile.mkdtemp(prefix="fuzzc-")
    try:
        # note: the optimizer, the checks and the target change the code of a call
        found = fl.judge(
            work, source, expected, prefix="calls",
            option_lists=(("--checks=noub",), ("--checks=noub", "--nopt"),
                          ("--checks=overflow",), ("--checks=-alias",),
                          ("--checks=noub,line", "--nopt")))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
