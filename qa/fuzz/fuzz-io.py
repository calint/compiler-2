#!/usr/bin/env python3
# model based fuzzer of 'read' and 'write', after the section 'strings and
# data' of 'etc/tutorial.baz'
#
# builds programs that write a whole array, a count of elements and a count
# from a start index of strings, 'dat', variables and array fields, and that
# read lines of the input into an array at a start index with a count. a read
# returns the number of elements it read and 0 at the end of the input. python
# runs the same operations: the program must write the bytes of the model and
# end with the checksum of the numbers the reads returned. the input is one
# line because the uart of rv32i-fpga gives one line to a read while a file
# gives as many bytes as fit
#
# every other program calls them with descriptors that are served and not
# (-1, 3, 256, 2^31 - 1, -2^31; 2^31 and 2^32 + 1 in an x86_64 program, which
# has 'i64'), counts and starts at the edges of the range and past them (a
# 'bounds' panic, those programs only with the checks), each a constant, a
# variable, a field, an element, a conversion, a served value or the result
# of an inlined or 'noinline' call of any width, alone or in a list, the calls
# also inside inlined and 'noinline' functions; built with and without the
# checks, a write to 2 shows on the uart
#
#   io-mismatch   a build ends differently from the model
#   io-reject     a generated program is rejected
#   compiler-hang, invalid-asm, link-failure  see fuzz-compiler.py
#
# usage: fuzz-io.py [count] [seed] [jobs]
import random
import sys

import fuzzlib as fl

LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJ0123456789 .,:;-_"


def string_literal(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


class Generator:
    def __init__(self, rnd):
        self.rnd = rnd
        self.lines = []
        self.out = bytearray()
        self.code = 0
        self.stdin = b""

    def emit(self, text):
        self.lines.append("    " + text)

    def text(self, low=0, high=12):
        return "".join(self.rnd.choice(LETTERS)
                       for _ in range(self.rnd.randint(low, high)))

    def write_slice(self, name, data, size):
        # a whole array, a count, a count from a start index
        rnd = self.rnd
        kind = rnd.choice(["all", "count", "range"])
        if kind == "all":
            self.emit(f"write(1, {name})")
            self.out += bytes(data)
        elif kind == "count":
            n = rnd.randint(0, size)
            self.emit(f"write(1, {name}, {n})")
            self.out += bytes(data[:n])
        else:
            n = rnd.randint(0, size)
            start = rnd.randint(0, size - n)
            self.emit(f"write(1, {name}, {n}, {start})")
            self.out += bytes(data[start:start + n])

    def program(self):
        rnd = self.rnd
        head = []
        arrays = []

        # 'dat' strings and variables initialized from strings
        for i in range(rnd.randint(1, 2)):
            text = self.text(1, 14)
            head.append(f"dat d{i} = {string_literal(text)}")
            arrays.append((f"d{i}", bytearray(text.encode()), len(text)))
        head.append("type holder { n, data i8[24] }")

        for i in range(rnd.randint(1, 3)):
            text = self.text(1, 14)
            self.emit(f"var s{i} = {string_literal(text)}")
            arrays.append((f"s{i}", bytearray(text.encode()), len(text)))

        # a field of an instance holds an array too
        text = self.text(1, 20)
        self.emit(f"var h = holder{{3, {string_literal(text)}}}")
        field = bytearray(text.encode()) + bytearray(24 - len(text))
        arrays.append(("h.data", field, 24))

        # one line of the input, the first read takes it and the others are at the
        # end of the input
        lines = [self.text(0, 10)]
        self.stdin = "".join(line + "\n" for line in lines).encode()
        buffers = []
        for i in range(rnd.randint(1, 2)):
            size = rnd.randint(16, 24)
            self.emit(f"var r{i} = i8[{size}]")
            buffers.append([f"r{i}", bytearray(size), size])

        pending = list(lines)
        code = 0
        for _ in range(rnd.randint(2, 8)):
            if rnd.random() < 0.55 and buffers:
                b = rnd.choice(buffers)
                name, data, size = b
                count = rnd.randint(12, size)
                start = rnd.randint(0, size - count)
                call = f"read(0, {name}, {count}, {start})"
                if start == 0 and rnd.random() < 0.3 and count == size:
                    call = f"read(0, {name})"
                    count = size
                got_var = f"g{len(self.lines)}"
                self.emit(f"var {got_var} = {call}")
                if pending:
                    line = pending.pop(0).encode() + b"\n"
                    got = len(line)
                    data[start:start + got] = line
                else:
                    got = 0
                code = (code * 7 + got) & 0xffff
                self.emit(f"acc = (acc * 7 + {got_var}) & 0xffff")
                self.write_slice(name, data, size)
            else:
                name, data, size = rnd.choice(arrays + [tuple(b) for b in buffers])
                self.write_slice(name, data, size)
        self.code = code
        self.lines.insert(0, "    var acc = 0")
        self.emit("exit(acc & 127)")
        source = "\n".join(head) + "\nfunc main() {\n" + \
            "\n".join(self.lines) + "\n}\n"
        return source, ("exit", self.code & 127, bytes(self.out))


# the descriptors 'read' serves and 'write' serves, any other returns this
SERVED_READ = (0,)
SERVED_WRITE = (1, 2)
BAD_DESCRIPTOR = -9
# descriptors that are not served, 'WIDE_BAD' only on x86_64 where 'int' and
# 'i64' hold them
BAD = (-1, 3, 255, 256, -9, 127, -128, 65535, 65536, (1 << 31) - 1,
       -(1 << 31))
WIDE_BAD = (1 << 31, (1 << 32) + 1, 1 << 32, (1 << 32) + 2, -(1 << 32),
            (1 << 63) - 1, -(1 << 63))
# the exit codes of the panics a program may end with, see 'fl.PANIC_CODE'
BOUNDS = ("panic", f"{fl.PANIC_CODE['bounds']} bounds")
OVERFLOW = ("panic", f"{fl.PANIC_CODE['overflow']} overflow")


def literal(value):
    # note: the minimum of 'i64' is no literal, its negation is out of range
    if value == fl.limits("i64")[0]:
        return f"{value + 1} - 1"
    return str(value)


def le_bytes(t, values):
    size = fl.BITS[t] // 8
    return bytearray(b"".join((v & ((1 << (8 * size)) - 1))
                              .to_bytes(size, "little") for v in values))


class DescriptorGenerator:
    # 'read' and 'write' whose descriptor, count and start are constants,
    # variables, fields, elements, 'let', conversions, served values and
    # results of inlined and 'noinline' calls of every width, nested in
    # lists, the calls also inside functions; descriptors served and not,
    # counts and starts at the edges of the range and past them
    #
    # an argument led by a type of 32 bits or less is computed at 'i32', one
    # led by 'i64' or a literal at the default type: a portable program keeps
    # every value in 'i32', a wide one runs on x86_64 only
    def __init__(self, rnd):
        self.rnd = rnd
        self.wide = rnd.random() < 0.3
        self.types = ("i8", "i16", "i32", "i64") if self.wide else \
            ("i8", "i16", "i32")
        self.lines = []
        self.out = bytearray()
        # the standard error, which the uart shows with the output
        self.uart_out = bytearray()
        self.served = []
        self.functions = {}
        self.names = 0
        self.pending = b""
        self.ending = None
        self.head = []

    def name(self, prefix):
        self.names += 1
        return f"{prefix}{self.names}"

    def emit(self, text):
        self.lines.append("    " + text)

    def print(self, data, descriptor):
        if descriptor == 1:
            self.out += data
        self.uart_out += data

    def default_limit(self):
        # the range all targets compute a list of the default type in
        return fl.limits("i64") if self.wide else fl.limits("i32")

    def fits_default(self, value):
        low, high = self.default_limit()
        return low <= value <= high

    def function(self, key, text):
        if key not in self.functions:
            self.functions[key] = text
        return key[0]

    def interesting(self, t):
        low, high = fl.limits(t)
        values = [0, 1, 2, -1, 3, low, high, high - 1, low + 1, 7, -9]
        return self.rnd.choice(values + [self.rnd.randint(low, high)])

    def operand(self, t, value, runtime=False):
        # text of type 't' that is 'value', 'runtime' when the compiler must
        # not know it
        rnd = self.rnd
        low, high = fl.limits(t)
        assert low <= value <= high
        kinds = ["var", "field", "element", "noinline", "say", "widen"]
        if not runtime:
            kinds += ["literal", "wrapped", "let", "inline"]
        kind = rnd.choice(kinds)
        narrower = [u for u in self.types
                    if fl.BITS[u] < fl.BITS[t] and fl.fits(u, value)]
        if kind == "literal":
            return f"{t}({literal(value)})"
        if kind == "wrapped":
            bits = fl.BITS[t]
            wrapped = value + rnd.choice((-1, 1)) * (1 << bits)
            # note: a literal wraps inside a conversion to its type
            if not self.fits_default(wrapped) and bits == 64:
                return f"{t}({literal(value)})"
            return f"{t}({wrapped})"
        if kind == "let":
            name = self.name("k")
            self.emit(f"let {name} = {t}({literal(value)})")
            return name
        # a new record or array for each operand, so the operands of one
        # call do not share one
        if kind == "field":
            name = self.name("r")
            self.emit(f"var {name} = rec{{}}")
            self.emit(f"{name}.f{fl.BITS[t]} = {t}({literal(value)})")
            return f"{name}.f{fl.BITS[t]}"
        if kind == "element":
            index = rnd.randint(0, 3)
            name = self.name("a")
            self.emit(f"var {name} = {t}[4]")
            self.emit(f"var {name}i = {index}")
            self.emit(f"{name}[{name}i] = {t}({literal(value)})")
            return f"{name}[{name}i]"
        if kind == "inline" and value - 1 >= low:
            f = self.function((f"inc_{t}",), f"func inc_{t}(v {t}) res {t} "
                              f"{{\n    res = v + 1\n}}\n")
            return f"{f}({self.typed(t, value - 1)})"
        if kind == "noinline" and value + 1 <= high:
            f = self.function((f"dec_{t}",), f"func noinline dec_{t}(v {t}) "
                              f"res {t} {{\n    res = v - 1\n}}\n")
            return f"{f}({self.typed(t, value + 1, runtime)})"
        if kind == "say":
            f = self.function((f"say_{t}",), f"func noinline say_{t}(v {t}) "
                              f"res {t} {{\n    write(1, mark)\n"
                              f"    res = v\n}}\n")
            text = f"{f}({self.typed(t, value, runtime)})"
            self.print(b"#", 1)
            return text
        if kind == "widen" and narrower:
            return f"{t}({self.operand(rnd.choice(narrower), value, runtime)})"
        name = self.name("x")
        self.emit(f"var {name} = {t}({literal(value)})")
        return name

    def typed(self, t, value, runtime=False):
        # an argument of a parameter of type 't', computed at 't'
        rnd = self.rnd
        low, high = fl.limits(t)
        step = rnd.randint(-3, 3)
        if rnd.random() < 0.3 and low <= value - step <= high and step:
            text = self.operand(t, value - step, runtime)
            return f"{fl.group(text)} + {fl.group(str(step))}"
        return self.operand(t, value, runtime)

    def listed(self, value, is_descriptor, runtime=False):
        # an argument of 'read' or 'write' that is 'value', a list led by an
        # operand of a random type, a served value or a literal; None when
        # the list would leave its width, before any text is made
        rnd = self.rnd
        if not runtime and rnd.random() < 0.1:
            return fl.group(literal(value))
        served = None
        converted = False
        if self.served and rnd.random() < 0.2:
            served, lead_value = rnd.choice(self.served)
            width = "default"
        else:
            t = rnd.choice(self.types)
            if is_descriptor and not fl.fits("i32", value) and self.wide:
                t = "i64"
            width = "default" if not is_descriptor else \
                "i64" if t == "i64" else "i32"
            low, high = fl.limits(t)
            if self.served and rnd.random() < 0.15:
                # a served value converted to the type of the list
                served, got = rnd.choice(self.served)
                lead_value = fl.wrap(t, got)
                converted = True
            elif low <= value <= high and rnd.random() < 0.5:
                lead_value = value
            else:
                lead_value = self.interesting(t)
        low, high = self.default_limit() if width == "default" else \
            fl.limits(width)
        op = rnd.choice(["+", "-", "^"])
        rest = {"+": value - lead_value, "-": lead_value - value,
                "^": lead_value ^ value}[op]
        alone = lead_value == value and rnd.random() < 0.5
        if not alone and not low <= rest <= high:
            # note: a list that leaves its width is rejected or overflows
            return None
        if served and not converted:
            lead = served
        elif converted:
            lead = f"{t}({served})"
        else:
            lead = self.operand(t, lead_value, runtime)
        if alone:
            return lead
        return f"{fl.group(lead)} {op} {fl.group(self.element(rest, width))}"

    def element(self, value, width):
        # an element after the first of a list computed at 'width', a type of
        # it or narrower, else a literal
        rnd = self.rnd
        allowed = [t for t in self.types
                   if fl.fits(t, value) and (width == "default" or
                                             fl.BITS[t] <= fl.BITS[width])]
        if allowed and rnd.random() < 0.6:
            return self.operand(rnd.choice(allowed), value)
        return literal(value)

    def descriptor(self, served):
        rnd = self.rnd
        if rnd.random() < 0.6:
            return rnd.choice(served)
        bad = BAD + WIDE_BAD if self.wide else BAD
        return rnd.choice(bad + SERVED_READ + SERVED_WRITE)

    def buffer(self):
        # name, element type, the bytes of the model and the length; the
        # buffers are made new for each call so the model is plain
        rnd = self.rnd
        kind = rnd.choice(["string", "zeros", "list", "field", "dat"])
        if kind in ("string", "dat"):
            text = self.text(1, 14)
            name = self.name("s" if kind == "string" else "d")
            if kind == "dat":
                self.head.append(f"dat {name} = {string_literal(text)}")
            else:
                self.emit(f"var {name} = {string_literal(text)}")
            return name, "i8", bytearray(text.encode()), len(text)
        if kind == "field":
            text = self.text(0, 12)
            self.emit(f"h.data = {string_literal(text)}")
            data = bytearray(text.encode()) + bytearray(12 - len(text))
            return "h.data", "i8", data, 12
        t = rnd.choice(self.types)
        length = rnd.randint(1, 6)
        name = self.name("b")
        if kind == "zeros":
            self.emit(f"var {name} = {t}[{length}]")
            return name, t, bytearray(length * fl.BITS[t] // 8), length
        values = [self.interesting(t) for _ in range(length)]
        self.emit(f"var {name} = {t}[]{{{', '.join(map(literal, values))}}}")
        return name, t, le_bytes(t, values), length

    def text(self, low, high):
        return "".join(self.rnd.choice(LETTERS)
                       for _ in range(self.rnd.randint(low, high)))

    def range_of(self, length, size, reading):
        # count and start in range, 'reading' the input needs room for the
        # line in bytes
        rnd = self.rnd
        need = -(-len(self.pending) // size) if reading and self.pending \
            else 0
        if need > length:
            return 0, rnd.randint(0, length)
        count = rnd.choice([length, need, rnd.randint(need, length)])
        start = rnd.choice([0, length - count, rnd.randint(0, length - count)])
        return count, start

    def bad_range(self, length, size):
        # a count and a start past the range, also ones whose sum or byte
        # count overflows a register
        rnd = self.rnd
        high = (1 << 63) - 1 if self.wide else (1 << 31) - 1
        cases = [(length + 1, 0), (1, length), (0, length + 1), (-1, 0),
                 (0, -1), (high, 1), (1, high), (high, high),
                 (-high - 1, length), (length, -high - 1),
                 ((1 << 31) // size, 0), ((1 << 32) // size, 0),
                 ((1 << 32) // size + 1, 0)]
        if self.wide:
            cases += [((1 << 32) + 1, 0), (1, 1 << 32), (length, 1 << 32),
                      ((1 << 64) // size // 2, 0), ((1 << 64) // size // 4, 1)]
        count, start = rnd.choice(cases)
        if self.fits_default(count) and self.fits_default(start):
            return count, start
        return length + 1, 0

    def call(self, reading):
        # one 'read' or 'write', or None when the program ends in a panic
        rnd = self.rnd
        name, t, data, length = self.buffer()
        size = fl.BITS[t] // 8
        served = SERVED_READ if reading else SERVED_WRITE
        if rnd.random() < 0.03:
            return self.overflow(reading, name), None
        descriptor = self.descriptor(served)
        is_served = descriptor in served
        ends = rnd.random() < 0.05
        if ends:
            count, start = self.bad_range(length, size)
        else:
            count, start = self.range_of(length, size,
                                         reading and is_served)
        form = rnd.choice(["direct"] * 3 + ["inline", "noinline"])
        if form != "direct" and fl.fits("i32", count) and \
                fl.fits("i32", start):
            return self.wrapped(form, reading, name, t, descriptor, count,
                                start, (data, length, ends, is_served))
        d = self.listed(descriptor, True)
        if d is None:
            d = self.operand("i64" if not fl.fits("i32", descriptor)
                             else "i32", descriptor)
        arguments = [d, name]
        shape = rnd.random()
        whole = count == length and start == 0
        if not (whole and shape < 0.2):
            arguments.append(self.count_text(count, ends))
            if start or shape < 0.7 or ends:
                arguments.append(self.count_text(start, ends))
        text = f"{'read' if reading else 'write'}({', '.join(arguments)})"
        return self.transfer(text, reading, descriptor, data, size, count,
                             start, ends, is_served)

    def overflow(self, reading, name):
        # a descriptor list that leaves its width, 'i32' unless led by 'i64',
        # panics with the checks
        t = self.rnd.choice(self.types)
        width = "i64" if t == "i64" else "i32"
        lead_value = fl.limits(t)[1]
        rest = fl.limits(width)[1] - lead_value + 1
        lead = self.operand(t, lead_value, runtime=True)
        self.ending = OVERFLOW
        return f"{'read' if reading else 'write'}({fl.group(lead)} + " \
            f"{fl.group(self.element(rest, width))}, {name})"

    def count_text(self, value, ends):
        text = self.listed(value, False, runtime=ends)
        if text is None:
            return self.operand("i64" if not fl.fits("i32", value)
                                else "i32", value, runtime=ends)
        return text

    def wrapped(self, form, reading, name, t, descriptor, count, start,
                rest):
        # the call inside a function that takes the descriptor, the buffer,
        # the count and the start
        data, length, ends, is_served = rest
        fd_types = [u for u in self.types if fl.fits(u, descriptor)]
        fd_type = self.rnd.choice(fd_types)
        verb = "read" if reading else "write"
        noinline = "noinline " if form == "noinline" else ""
        key = (f"{verb}_{form}_{fd_type}_{t}",)
        buffer = f"buf mut {t}[]" if reading else f"buf {t}[]"
        f = self.function(key, f"func {noinline}{key[0]}(fd {fd_type}, "
                          f"{buffer}, n i32, s i32) res {{\n"
                          f"    res = {verb}(fd, buf, n, s)\n}}\n")
        text = f"{f}({self.typed(fd_type, descriptor)}, {name}, " \
            f"{self.typed('i32', count, ends)}, " \
            f"{self.typed('i32', start, ends)})"
        size = fl.BITS[t] // 8
        return self.transfer(text, reading, descriptor, data, size, count,
                             start, ends, is_served)

    def transfer(self, text, reading, descriptor, data, size, count, start,
                 ends, is_served):
        # the model of the call: what it returns and prints, None at a panic
        if ends:
            self.ending = BOUNDS
            return text, None
        if not is_served:
            return text, BAD_DESCRIPTOR
        begin, end = start * size, (start + count) * size
        if not reading:
            self.print(bytes(data[begin:end]), descriptor)
            return text, end - begin
        if count == 0 or not self.pending:
            return text, 0
        got = len(self.pending)
        data[begin:begin + got] = self.pending
        self.pending = b""
        return text, got

    def program(self):
        rnd = self.rnd
        self.head = ['dat mark = "#"']
        fields = ", ".join(f"f{fl.BITS[t]} {t}" for t in self.types)
        self.head.append(f"type rec {{ {fields}, data i8[12] }}")
        self.emit("var acc = 0")
        self.emit("var h = rec{}")
        line = self.text(0, 10).encode() + b"\n"
        self.pending = line
        code = 0
        for _ in range(rnd.randint(2, 7)):
            text, got = self.call(rnd.random() < 0.4)
            if got is None:
                self.emit(f"var end = {text}")
                break
            name = self.name("g")
            shape = rnd.random()
            if shape < 0.15:
                self.emit(f"var {name} = 3 + {text}")
                got += 3
            elif shape < 0.3:
                self.emit(f"if {text} == {got} {{")
                self.emit("    acc = acc + 1")
                self.emit("}")
                code = (code + 1) & 0xffff
                continue
            else:
                self.emit(f"var {name} = {text}")
            self.served.append((name, got))
            self.emit(f"acc = (acc * 7 + {name}) & 0xffff")
            code = (code * 7 + got) & 0xffff
        self.emit("exit(acc & 127)")
        source = "\n".join(self.head) + "\n" + \
            "".join(self.functions.values()) + "func main() {\n" + \
            "\n".join(self.lines) + "\n}\n"
        if self.ending:
            return source, self.ending, self.ending, line
        return source, ("exit", code & 127, bytes(self.out)), \
            ("exit", code & 127, bytes(self.uart_out)), line


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    if index % 2:
        return fuzz_descriptors(rnd)
    gen = Generator(rnd)
    source, expected = gen.program()
    found = fl.judge(source, expected, stdin=gen.stdin, prefix="io")
    return source, found, "slices"


def fuzz_descriptors(rnd):
    # with and without the checks; a program that ends in a panic only with
    # them, without them its range is not checked
    gen = DescriptorGenerator(rnd)
    source, expected, uart_expected, stdin = gen.program()
    targets = ("x86_64",) if gen.wide else fl.TARGETS
    lists = fl.OPTION_LISTS if gen.ending else fl.OPTION_LISTS + ((),)
    found = fl.judge(source, expected, stdin=stdin, targets=targets,
                     option_lists=lists, prefix="io",
                     uart_expected=uart_expected)
    return source, found, "wide" if gen.wide else "descriptors"


def main():
    count, seed, jobs = fl.arguments(300)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
