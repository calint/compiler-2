#!/usr/bin/env python3
# model based fuzzer of data and user types, after the sections 'constants and
# variables', 'literals', 'arrays', 'strings and data', 'user types' and
# 'arrays in user types' of 'etc/tutorial.baz'
#
# builds programs of user types (scalars of every width, 'bool', nested
# records, arrays) and checks 'dat' and 'var' initializers (full, partial that
# is zero filled, strings, characters), 'let' constants of literals and of
# expressions of constants, read-only variables, shadowing in blocks, copies,
# '==' and '!=' of records and arrays, 'foo', 'array_copy', 'arrays_equal' and
# 'write' of a part of an array. each program mixes every value it reads into a
# checksum, python computes the same checksum and the bytes written. the
# program must end with the modeled exit code and output on every target, with
# and without '--nopt'
#
#   data-mismatch   a build ends differently from the model
#   data-reject     a generated program is rejected (the generator or the
#                   compiler is wrong)
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-data.py [count] [seed] [jobs]
import random
import shutil
import sys
import tempfile

import fuzzlib as fl

# "int" is the default type, 64 bits on x86_64 and 32 on rv32i: the model
# keeps its values inside 32 bits
SCALARS = {"i8": 8, "i16": 16, "i32": 32, "int": 32}
CHARS = "abcxyz AZ09_~!#%&()*+,-./:;<=>?@[]^`{|}"
ESCAPES = {"\\n": 10, "\\t": 9, "\\0": 0, "\\\\": 92, "\\'": 39, "\\x41": 65,
           "\\x7f": 127, "\\x00": 0}
PRECEDENCE = {"+": 1, "-": 1, "*": 2, "|": 3, "&": 4, "^": 5, "<<": 6,
              ">>": 6}


def mix(acc, value):
    return ((acc ^ (value & 0xffff)) * 31) & 0xffff


def limits(t):
    bits = SCALARS[t]
    return -(1 << (bits - 1)), (1 << (bits - 1)) - 1


class Scalar:
    def __init__(self, t):
        self.t = t

    def zero(self):
        return 0

    def decl(self):
        return self.t


class Bool:
    t = "bool"

    def zero(self):
        return False

    def decl(self):
        return "bool"


class Array:
    def __init__(self, elem, count):
        self.elem = elem
        self.count = count

    def zero(self):
        return [self.elem.zero() for _ in range(self.count)]

    def decl(self):
        return f"{self.elem.decl()}[{self.count}]"


class Record:
    def __init__(self, name, fields):
        self.name = name
        self.fields = fields

    def zero(self):
        return {n: t.zero() for n, t in self.fields}

    def decl(self):
        return self.name


def evaluate(values, ops):
    # the precedence of the language, from lowest '+ -', '*', '|', '&', '^',
    # '<< >>'
    values = list(values)
    ops = list(ops)
    for level in range(6, 0, -1):
        i = 0
        while i < len(ops):
            if PRECEDENCE[ops[i]] == level:
                a, b, op = values[i], values[i + 1], ops[i]
                if op in ("<<", ">>") and not 0 <= b <= 3:
                    raise ValueError("shift count")
                r = {"+": a + b, "-": a - b, "*": a * b, "|": a | b,
                     "&": a & b, "^": a ^ b, "<<": a << b, ">>": a >> b}[op]
                if abs(r) > 1 << 28:
                    raise ValueError("large")
                values[i:i + 2] = [r]
                del ops[i]
            else:
                i += 1
    return values[0]


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.types = []
        self.constants = {}
        self.out = bytearray()
        self.acc = 0
        self.names = 0
        self.lines = []
        self.indent = 1

    def name(self, prefix):
        self.names += 1
        return f"{prefix}{self.names}"

    def emit(self, text):
        self.lines.append("    " * self.indent + text)

    # constants

    def constant_operand(self):
        rnd = self.rnd
        r = rnd.random()
        if r < 0.3 and self.constants:
            n = rnd.choice(list(self.constants))
            return self.constants[n], n
        if r < 0.4:
            c = rnd.choice(CHARS)
            return ord(c), f"'{c}'"
        v = rnd.randint(0, 40)
        return v, rnd.choice([str(v), hex(v), bin(v)])

    def constant_expression(self):
        while True:
            try:
                return self.constant_expression_once()
            except ValueError:
                continue

    def constant_expression_once(self):
        # every result stays small so that it fits every width
        rnd = self.rnd
        count = rnd.randint(1, 4)
        values = []
        texts = []
        ops = []
        for i in range(count):
            v, t = self.constant_operand()
            if rnd.random() < 0.2:
                v, t = -v, f"-{t}"
            elif rnd.random() < 0.1:
                v, t = ~v, f"~{t}"
            values.append(v)
            texts.append(t)
            if i + 1 < count:
                ops.append(rnd.choice(list(PRECEDENCE)))
        for i, op in enumerate(ops):
            if op in ("<<", ">>"):
                c = rnd.randint(0, 3)
                values[i + 1], texts[i + 1] = c, str(c)
        if count > 1 and rnd.random() < 0.3:
            lo = rnd.randint(0, count - 2)
            hi = rnd.randint(lo + 1, count - 1)
            inner = evaluate(values[lo:hi + 1], ops[lo:hi])
            text = "(" + self.join(texts[lo:hi + 1], ops[lo:hi]) + ")"
            values[lo:hi + 1] = [inner]
            texts[lo:hi + 1] = [text]
            del ops[lo:hi]
        value = evaluate(values, ops)
        if abs(value) > 20000:
            raise ValueError("large")
        return value, self.join(texts, ops)

    @staticmethod
    def join(texts, ops):
        out = texts[0]
        for op, t in zip(ops, texts[1:]):
            out += f" {op} {t}"
        return out

    # types

    def scalar_type(self):
        if self.rnd.random() < 0.15:
            return Bool()
        t = self.rnd.choice(list(SCALARS) + ["i8", "i16", "i32"])
        return Scalar(t)

    def field_type(self):
        r = self.rnd.random()
        if r < 0.25 and self.types:
            return self.rnd.choice(self.types)
        if r < 0.5:
            elem = self.scalar_type()
            if elem.t == "bool" and self.rnd.random() < 0.5:
                elem = Scalar("i8")
            return Array(elem, self.rnd.randint(1, 5))
        return self.scalar_type()

    def make_types(self):
        for i in range(self.rnd.randint(1, 4)):
            fields = [(f"f{j}", self.field_type())
                      for j in range(self.rnd.randint(1, 4))]
            self.types.append(Record(f"t{i}", fields))

    # values and their source text

    def literal(self, t):
        rnd = self.rnd
        if t.t == "bool":
            v = rnd.random() < 0.5
            return v, "true" if v else "false"
        low, high = limits(t.t)
        r = rnd.random()
        if r < 0.15 and high >= 126:
            c = rnd.choice(CHARS)
            return ord(c), f"'{c}'"
        if r < 0.22:
            e = rnd.choice(list(ESCAPES))
            return ESCAPES[e], f"'{e}'"
        fitting = [(n, v) for n, v in self.constants.items()
                   if low <= v <= high]
        if r < 0.35 and fitting:
            n, v = rnd.choice(fitting)
            return v, n
        v = rnd.choice([low, high, 0, 1, -1, rnd.randint(low, high),
                        rnd.randint(low, high), rnd.randint(-100, 100)])
        v = max(low, min(high, v))
        style = rnd.random()
        if v < 0:
            if style < 0.2 and -v <= high:
                return v, f"-{rnd.choice([hex(-v), bin(-v)])}"
            return v, str(v)
        if style < 0.2:
            return v, hex(v)
        if style < 0.3:
            return v, bin(v)
        return v, str(v)

    def initializer(self, t):
        # (value, text) of a full or partial literal of the type
        rnd = self.rnd
        if isinstance(t, (Scalar, Bool)):
            return self.literal(t)
        if isinstance(t, Array):
            given = rnd.randint(0, t.count)
            if isinstance(t.elem, Scalar) and t.elem.t == "i8" and \
                    rnd.random() < 0.4:
                text = "".join(rnd.choice("abcdefgh") for _ in range(given))
                value = [ord(c) for c in text] + [0] * (t.count - given)
                return value, f'"{text}"' if given else "{}"
            values = []
            texts = []
            for _ in range(given):
                v, s = self.initializer(t.elem)
                values.append(v)
                texts.append(s)
            values += [t.elem.zero() for _ in range(t.count - given)]
            return values, "{" + ", ".join(texts) + "}"
        value = {}
        texts = []
        given = rnd.randint(0, len(t.fields))
        for i, (n, ft) in enumerate(t.fields):
            if i < given:
                v, s = self.initializer(ft)
                texts.append(s)
            else:
                v = ft.zero()
            value[n] = v
        return value, "{" + ", ".join(texts) + "}"

    def typed_initializer(self, t):
        value, text = self.initializer(t)
        if isinstance(t, Record):
            return value, t.name + text
        return value, f"{t.elem.decl()}[{t.count}]{text}"

    # access paths: leaves in program order

    def leaves(self, t, value, path):
        if isinstance(t, (Scalar, Bool)):
            yield t, path, value
        elif isinstance(t, Array):
            for i, v in enumerate(value):
                yield from self.leaves(t.elem, v, f"{path}[{i}]")
        else:
            for n, ft in t.fields:
                yield from self.leaves(ft, value[n], f"{path}.{n}")

    def mix_leaf(self, t, path, value):
        if t.t == "bool":
            self.emit(f"if {path} mix(acc, 1) else mix(acc, 0)")
            self.acc = mix(self.acc, 1 if value else 0)
        else:
            self.emit(f"mix(acc, i32({path}))")
            self.acc = mix(self.acc, value)

    def mix_all(self, t, value, path):
        for lt, lp, lv in self.leaves(t, value, path):
            self.mix_leaf(lt, lp, lv)

    @staticmethod
    def set_path(value, path, new):
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
        node = value
        for s in steps[:-1]:
            node = node[s]
        node[steps[-1]] = new

    @staticmethod
    def copy(value):
        if isinstance(value, dict):
            return {k: Generator.copy(v) for k, v in value.items()}
        if isinstance(value, list):
            return [Generator.copy(v) for v in value]
        return value

    # statements

    def mutate(self, t, value, name):
        value = self.copy(value)
        leaves = list(self.leaves(t, value, name))
        for _ in range(self.rnd.randint(1, 4)):
            lt, lp, _ = self.rnd.choice(leaves)
            v, s = self.literal(lt)
            self.emit(f"{lp} = {s}")
            self.set_path(value, lp[len(name):], v)
            leaves = list(self.leaves(t, value, name))
        return value

    def check_object(self, t, value, name):
        self.mix_all(t, value, name)
        # a copy equals the original until a leaf changes
        copy = self.name("c")
        self.emit(f"var {copy} = {name}")
        self.emit(f"if {copy} != {name} exit(1)")
        self.emit(f"if not ({copy} == {name}) exit(2)")
        new = self.mutate(t, value, name)
        equal = new == value
        # note: a comparison is not an operand of '==', it is stored first
        eq = self.name("q")
        ne = self.name("q")
        self.emit(f"var {eq} = {copy} == {name}")
        self.emit(f"var {ne} = {copy} != {name}")
        self.emit(f"if {eq} != {str(equal).lower()} exit(3)")
        self.emit(f"if {ne} == {str(equal).lower()} exit(4)")
        self.mix_all(t, new, name)
        if self.rnd.random() < 0.7:
            self.emit(f"{name} = {copy}")
            self.mix_all(t, value, name)
            self.emit(f"if {copy} != {name} exit(5)")
            return value
        return new

    def array_ops(self, t, value, name):
        # scalar arrays: 'foo', 'array_copy', 'arrays_equal', 'write'
        rnd = self.rnd
        elem = t.elem
        if not isinstance(elem, Scalar) or t.count < 2:
            return value
        value = self.copy(value)
        kind = rnd.choice(["foo", "copy", "write"])
        if kind == "foo":
            step = rnd.randint(0, 3)
            # a count visits only the first elements, 'n' stays the size
            visited = t.count
            head = f"foo {name}"
            if rnd.random() < 0.4:
                visited = rnd.randint(0, t.count)
                if rnd.random() < 0.5:
                    self.emit(f"var {name.replace('.', '_').replace('[', '_').replace(']', '')}_c = {visited}")
                    head += f", {name.replace('.', '_').replace('[', '_').replace(']', '')}_c"
                else:
                    head += f", {visited}"
            self.emit(f"{head} {{")
            self.emit(f"    mix(acc, i32(int(e) + {step} + i - n))")
            self.emit("}")
            for i, v in enumerate(value[:visited]):
                self.acc = mix(self.acc, v + step + i - t.count)
        elif kind == "copy":
            n = rnd.randint(1, t.count)
            src = rnd.randint(0, t.count - n)
            dst = rnd.randint(0, t.count - n)
            # a destination inside the source is rejected by 'overlap' when
            # it starts above the source
            if src < dst < src + n:
                dst = src
            tmp = self.name("a")
            self.emit(f"var {tmp} = {name}")
            self.emit(f"array_copy({name}[{src}], {tmp}[{dst}], {n})")
            result = list(value)
            result[dst:dst + n] = value[src:src + n]
            self.emit(f"if arrays_equal({tmp}[{dst}], {name}[{src}], {n}) "
                      f"mix(acc, 1) else mix(acc, 2)")
            same = result[dst:dst + n] == value[src:src + n]
            self.acc = mix(self.acc, 1 if same else 2)
            for i, v in enumerate(result):
                self.emit(f"mix(acc, i32({tmp}[{i}]))")
                self.acc = mix(self.acc, v)
        elif elem.t == "i8":
            n = rnd.randint(0, t.count)
            start = rnd.randint(0, t.count - n)
            self.emit(f"write(1, {name}, {n}, {start})")
            self.out += bytes(v & 255 for v in value[start:start + n])
        return value

    def block_scope(self):
        # constants and variables shadow outer declarations
        rnd = self.rnd
        name = self.name("k")
        outer = rnd.randint(-100, 100)
        inner = rnd.randint(-100, 100)
        self.emit(f"let {name} = {outer}")
        self.emit("{")
        self.emit(f"    let {name} = {inner}")
        self.emit(f"    mix(acc, {name})")
        self.emit("}")
        self.emit(f"mix(acc, {name})")
        self.acc = mix(mix(self.acc, inner), outer)
        # a read-only variable from an expression of a variable
        a = rnd.randint(-50, 50)
        b = rnd.randint(-50, 50)
        base = self.name("b")
        self.emit(f"var {base} = {outer}")
        ro = self.name("r")
        self.emit(f"let {ro} = {base} + {a} * {b}")
        self.emit(f"mix(acc, i32({ro}))")
        self.acc = mix(self.acc, outer + a * b)

    def walk(self, t, value, path):
        if isinstance(t, Array):
            if isinstance(t.elem, Scalar):
                value[:] = self.array_ops(t, value, path)
            else:
                for i, v in enumerate(value):
                    self.walk(t.elem, v, f"{path}[{i}]")
        elif isinstance(t, Record):
            for n, ft in t.fields:
                self.walk(ft, value[n], f"{path}.{n}")

    def field_declaration(self, name, t):
        # an array of the default type may leave out the type name
        if isinstance(t, Array) and isinstance(t.elem, Scalar) \
                and t.elem.t == "int" and self.rnd.random() < 0.5:
            return f"{name}[{t.count}]"
        return f"{name} {t.decl()}"

    # program

    def program(self):
        rnd = self.rnd
        self.make_types()
        head = []
        for i in range(rnd.randint(0, 4)):
            n = f"C{i}"
            v, text = self.constant_expression()
            head.append(f"let {n} = {text}")
            self.constants[n] = v
        for t in self.types:
            fields = ", ".join(self.field_declaration(n, ft)
                               for n, ft in t.fields)
            head.append(f"type {t.name} {{ {fields} }}")
        objects = []
        for _ in range(rnd.randint(1, 3)):
            t = rnd.choice(self.types)
            if rnd.random() < 0.3:
                t = Array(t, rnd.randint(1, 3))
            name = self.name("d")
            value, text = self.typed_initializer(t)
            head.append(f"dat {name} = {text}")
            objects.append((name, t, value))
        head.append("func mix(acc mut i32, v i32) "
                    "{ acc = ((acc ^ (v & 0xffff)) * 31) & 0xffff }")
        self.emit("var acc = i32(0)")
        for _ in range(rnd.randint(1, 3)):
            t = rnd.choice(self.types)
            name = self.name("v")
            value, text = self.typed_initializer(t)
            self.emit(f"var {name} = {text}")
            objects.append((name, t, value))
        for name, t, value in objects:
            value = self.check_object(t, value, name)
            self.walk(t, value, name)
            if rnd.random() < 0.5:
                self.block_scope()
        self.emit("exit(acc & 127)")
        source = "\n".join(head) + "\nfunc main() {\n" + \
            "\n".join(self.lines) + "\n}\n"
        return source, ("exit", self.acc & 127, bytes(self.out))


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    source, expected = Generator(rnd).program()
    work = tempfile.mkdtemp(prefix="fuzzd-")
    try:
        found = fl.judge(work, source, expected, prefix="data")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
