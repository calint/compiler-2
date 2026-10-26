#!/usr/bin/env python3
# model based fuzzer of user types, methods and constructors, after the
# sections 'user types', 'methods', 'constructors' and 'arrays in user types'
# of 'etc/tutorial.baz'
#
# builds programs of records of i32 fields with nested records and arrays, with
# 'mut' methods that write 'self', methods that only read it (also on a 'let'
# receiver), methods named like a field, constructors that build 'self', 'noinline'
# functions that take a record, functions that return a record, record
# literals as arguments, and calls on variables, fields, array elements and
# fields of array elements. python runs the same operations, the program mixes
# what it reads into a checksum and ends with it as the exit code:
#
#   methods-mismatch   a build ends differently from the model
#   methods-reject     a generated program is rejected
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-methods.py [count] [seed] [jobs]
import random
import shutil
import sys
import tempfile

import fuzzlib as fl
from fuzzlib import mix


def wrap32(v):
    v &= 0xffffffff
    return v - (1 << 32) if v >> 31 else v


class Record:
    def __init__(self, name, fields):
        # fields: (name, kind, detail): kind 'int', 'rec' (detail the Record)
        # or 'arr' (detail the count)
        self.name = name
        self.fields = fields

    def zero(self):
        out = {}
        for n, kind, detail in self.fields:
            if kind == "int":
                out[n] = 0
            elif kind == "rec":
                out[n] = detail.zero()
            else:
                out[n] = [0] * detail
        return out

    def leaves(self, value, path):
        for n, kind, detail in self.fields:
            if kind == "int":
                yield f"{path}.{n}", (value, n, None)
            elif kind == "rec":
                yield from detail.leaves(value[n], f"{path}.{n}")
            else:
                for i in range(detail):
                    yield f"{path}.{n}[{i}]", (value[n], i, None)

    def total(self, value):
        t = 0
        for n, kind, detail in self.fields:
            if kind == "int":
                t += value[n]
            elif kind == "rec":
                t += detail.total(value[n])
            else:
                t += sum(value[n])
        return wrap32(t)


def copy(v):
    if isinstance(v, dict):
        return {k: copy(x) for k, x in v.items()}
    if isinstance(v, list):
        return [copy(x) for x in v]
    return v


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.records = []
        self.lines = []
        self.acc = 0
        self.names = 0

    def name(self, p):
        self.names += 1
        return f"{p}{self.names}"

    def emit(self, text):
        self.lines.append("    " + text)

    def mix_value(self, text, value):
        self.emit(f"mix(acc, {text})")
        self.acc = mix(self.acc, value)

    def make_types(self, head):
        rnd = self.rnd
        for i in range(rnd.randint(1, 3)):
            fields = []
            for j in range(rnd.randint(1, 3)):
                r = rnd.random()
                if r < 0.25 and self.records:
                    fields.append((f"f{j}", "rec", rnd.choice(self.records)))
                elif r < 0.45:
                    fields.append((f"f{j}", "arr", rnd.randint(1, 3)))
                else:
                    fields.append((f"f{j}", "int", None))
            rec = Record(f"t{i}", fields)
            self.records.append(rec)
            parts = []
            for n, kind, detail in fields:
                if kind == "int":
                    parts.append(f"{n} i32")
                elif kind == "rec":
                    parts.append(f"{n} {detail.name}")
                else:
                    parts.append(f"{n} i32[{detail}]")
            head.append(f"type {rec.name} {{ {', '.join(parts)} }}")

    def methods(self, head, rec):
        # a method that writes self, one that reads it, one named like a
        # field, a constructor, a function that takes and one that returns it
        name = rec.name
        leaves = list(rec.leaves(rec.zero(), "self"))
        first = leaves[0][0]
        head.append(f"func mut {name}.bump(d i32) {{")
        head.append(f"    {first} = {first} + d")
        head.append("}")
        head.append(f"func {name}.total() res i32 {{")
        head.append("    res = " + " + ".join(p for p, _ in leaves))
        head.append("}")
        field = rec.fields[0]
        if field[1] == "int":
            head.append(f"func mut {name}.{field[0]}(v i32) {{")
            head.append(f"    self.{field[0]} = v")
            head.append("}")
        params = [f"a{i} i32" for i, _ in enumerate(leaves)]
        head.append(f"func {name}.build({', '.join(params)}) self {{")
        for i, (p, _) in enumerate(leaves):
            head.append(f"    {p} = a{i}")
        head.append("}")
        head.append(f"func noinline take_{name}(p {name}) res i32 {{")
        head.append("    res = " + " + ".join(
            p.replace("self", "p") for p, _ in leaves))
        head.append("}")
        head.append(f"func give_{name}(v i32) res {name} {{")
        head.append(f"    res = {name}.build({', '.join(['v'] * len(leaves))})")
        head.append("}")

    def program(self):
        rnd = self.rnd
        head = []
        self.make_types(head)
        for rec in self.records:
            self.methods(head, rec)
        head.append("func mix(acc mut i32, v i32) "
                    "{ acc = ((acc ^ (v & 0xffff)) * 31) & 0xffff }")
        self.emit("var acc = i32(0)")
        for _ in range(rnd.randint(2, 4)):
            rec = rnd.choice(self.records)
            self.object(rec)
        self.emit("exit(acc & 127)")
        source = "\n".join(head) + "\nfunc main() {\n" + \
            "\n".join(self.lines) + "\n}\n"
        return source, ("exit", self.acc & 127, b"")

    def leaf_count(self, rec):
        return len(list(rec.leaves(rec.zero(), "x")))

    def set_value(self, rec, value, path, new):
        # path like '.f0[1].f2'
        node = value
        steps = []
        i = 0
        while i < len(path):
            if path[i] == ".":
                j = i + 1
                while j < len(path) and path[j] not in ".[":
                    j += 1
                steps.append(path[i + 1:j])
                i = j
            else:
                j = path.index("]", i)
                steps.append(int(path[i + 1:j]))
                i = j + 1
        for s in steps[:-1]:
            node = node[s]
        node[steps[-1]] = new

    def get_value(self, value, path):
        node = value
        i = 0
        while i < len(path):
            if path[i] == ".":
                j = i + 1
                while j < len(path) and path[j] not in ".[":
                    j += 1
                node = node[path[i + 1:j]]
                i = j
            else:
                j = path.index("]", i)
                node = node[int(path[i + 1:j])]
                i = j + 1
        return node

    def object(self, rec):
        rnd = self.rnd
        var = self.name("o")
        value = rec.zero()
        count = self.leaf_count(rec)
        container = rnd.choice(["var", "array", "let"])
        if container == "array":
            n = rnd.randint(2, 3)
            self.emit(f"var {var} = {rec.name}[{n}]")
            values = [rec.zero() for _ in range(n)]
            ix = rnd.randrange(n)
            target = f"{var}[{ix}]"
            value = values[ix]
        elif container == "let":
            args = [rnd.randint(-50, 50) for _ in range(count)]
            self.emit(f"let {var} = {rec.name}.build({', '.join(map(str, args))})")
            value = rec.zero()
            for (p, _), a in zip(rec.leaves(value, ""), args):
                self.set_value(rec, value, p, a)
            target = var
            self.mix_value(f"{var}.total()", rec.total(value))
            self.mix_value(f"take_{rec.name}({var})", rec.total(value))
            return
        else:
            self.emit(f"var {var} = {rec.name}")
            target = var
        for _ in range(rnd.randint(2, 6)):
            kind = rnd.choice(["bump", "build", "total", "take", "give",
                               "field", "leaf", "copy", "literal"])
            leaves = list(rec.leaves(value, ""))
            if kind == "bump":
                d = rnd.randint(-20, 20)
                self.emit(f"{target}.bump({d})")
                p = leaves[0][0]
                self.set_value(rec, value, p,
                               wrap32(self.get_value(value, p) + d))
            elif kind == "build":
                args = [rnd.randint(-50, 50) for _ in leaves]
                self.emit(f"{target} = {rec.name}.build({', '.join(map(str, args))})")
                for (p, _), a in zip(leaves, args):
                    self.set_value(rec, value, p, a)
            elif kind == "total":
                self.mix_value(f"{target}.total()", rec.total(value))
            elif kind == "take":
                self.mix_value(f"take_{rec.name}({target})", rec.total(value))
            elif kind == "give":
                v = rnd.randint(-50, 50)
                self.emit(f"{target} = give_{rec.name}({v})")
                for p, _ in leaves:
                    self.set_value(rec, value, p, v)
            elif kind == "field":
                field = rec.fields[0]
                if field[1] == "int":
                    v = rnd.randint(-50, 50)
                    self.emit(f"{target}.{field[0]}({v})")
                    value[field[0]] = v
                    self.mix_value(f"{target}.{field[0]}", v)
            elif kind == "leaf":
                p, _ = rnd.choice(leaves)
                v = rnd.randint(-50, 50)
                self.emit(f"{target}{p} = {v}")
                self.set_value(rec, value, p, v)
                self.mix_value(f"{target}{p}", v)
            elif kind == "copy":
                other = self.name("c")
                self.emit(f"var {other} = {target}")
                self.mix_value(f"{other}.total()", rec.total(value))
                self.mix_value(f"take_{rec.name}({other})", rec.total(value))
                eq = self.name("q")
                self.emit(f"var {eq} = {other} == {target}")
                self.emit(f"if not {eq} exit(1)")
            else:
                args = [rnd.randint(-50, 50) for _ in leaves]
                text = ", ".join(map(str, args))
                self.mix_value(f"take_{rec.name}({rec.name}.build({text}))",
                               wrap32(sum(args)))
        self.mix_value(f"{target}.total()", rec.total(value))


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    source, expected = Generator(rnd).program()
    work = tempfile.mkdtemp(prefix="fuzzm-")
    try:
        found = fl.judge(work, source, expected, prefix="methods")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
