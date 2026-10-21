#!/usr/bin/env python3
# fuzzer of the programs that the language rejects, after the compile errors
# that 'etc/tutorial.baz' names: writing a 'let', a constant, a read-only
# argument or an element of a read-only array, narrowing without a conversion,
# a result that is not assigned on every path, a 'dat' that is not made of
# constants, a call whose result and arguments share storage, a record that
# reads a field after it was overwritten, a constant index out of bounds, a
# wider element, 'mut' given a read-only value, an unknown name, a wrong number
# of arguments, a repeated name, 'break' outside a loop, 'and' mixed with 'or',
# a shift count and an overflow that are constant, arrays of other sizes, an
# integer as a condition, a 'bool' compared with an integer, a name in the body
# of a function that repeats a parameter
#
# every rule makes a program that breaks it and a twin that does not. the
# program must be rejected on every target with a position (file:line:column)
# on the line of the mistake, the twin must be accepted
#
#   reject-accepted   the program with the mistake is accepted
#   reject-line       the position is not on the line of the mistake
#   reject-crash      the compiler crashes or hangs on it
#   reject-over       the twin is rejected
#
# usage: fuzz-reject.py [count] [seed] [jobs]
import os
import random
import re
import shutil
import sys
import tempfile

import fuzzlib as fl

INT_TYPES = ["i8", "i16", "i32"]


class Program:
    # lines of a program, the lines of the mistake are marked
    def __init__(self):
        self.lines = []
        self.error_lines = set()

    def add(self, text, error=False):
        for part in text.split("\n"):
            self.lines.append(part)
            if error:
                self.error_lines.add(len(self.lines))

    def text(self):
        return "\n".join(self.lines) + "\n"


class Case:
    def __init__(self, name, options=()):
        self.name = name
        self.options = list(options)
        self.bad = Program()
        self.good = Program()

    def both(self, header=None, body=None):
        for p in (self.bad, self.good):
            if header:
                p.add(header)
            if body:
                p.add(body)


def insert(program, at, text):
    # lines are inserted before the line at index 'at' (0 based) and the
    # lines of the mistake move down
    new_lines = text.split("\n")
    program.lines[at:at] = new_lines
    program.error_lines = {
        n + len(new_lines) if n > at else n for n in program.error_lines
    }


def decorate(program, rnd, allow_loop):
    # comments and blank lines at the top, the body of 'main' in blocks
    for _ in range(rnd.randint(0, 3)):
        insert(program, 0, rnd.choice(["# padding", "", "# note: 1 + 2"]))

    if "func main() {" not in program.lines:
        return

    start = program.lines.index("func main() {")
    end = len(program.lines) - 1

    wrappers = [("{", "}"), ("if true {", "}"), ("if 1 == 1 {", "} else {\n}")]

    if allow_loop:
        wrappers.append(("loop {", "break\n}"))

    for _ in range(rnd.randint(0, 2)):
        opener, closer = rnd.choice(wrappers)
        insert(program, end, closer)
        insert(program, start + 1, opener)
        end += len(closer.split("\n")) + len(opener.split("\n"))


def rule_assign_let(rnd, c):
    v = rnd.randint(1, 50)
    for p, text in ((c.bad, "b = 0"), (c.good, "var c = b")):
        p.add("func main() {")
        p.add(f"    var a = {v}")
        p.add("    let b = a + 1")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_assign_constant(rnd, c):
    v = rnd.randint(1, 50)
    for p, text in ((c.bad, f"k = {v + 1}"), (c.good, "var y = k")):
        p.add(f"let k = {v}")
        p.add("func main() {")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_narrowing(rnd, c):
    # note: 'i32' is the default type on rv32i, nothing narrows there
    t = rnd.choice(["i8", "i16"])
    for p, text in ((c.bad, "small = wide"), (c.good, f"small = {t}(wide)")):
        p.add("func main() {")
        p.add(f"    var small = {t}(1)")
        p.add("    var wide = int(small)")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_narrowing_expression(rnd, c):
    t = rnd.choice(["i8", "i16"])
    for p, text in ((c.bad, "sum = wide"), (c.good, "total = wide")):
        p.add("func main() {")
        p.add(f"    var small = {t}(3)")
        p.add("    var wide = int(small)")
        p.add("    var sum = small + 1")
        p.add("    var total = 1 + small")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_let_array(rnd, c):
    n = rnd.randint(2, 5)
    items = ", ".join(str(rnd.randint(0, 9)) for _ in range(n))
    i = rnd.randrange(n)
    for p, text in ((c.bad, f"table[{i}] = 0"), (c.good, f"var x = table[{i}]")):
        p.add("func main() {")
        p.add(f"    let table = []{{{items}}}")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_let_instance(rnd, c):
    for p, text in ((c.bad, "p.x = 3"), (c.good, "var q = p.x")):
        p.add("type point {x, y}")
        p.add("func main() {")
        p.add("    let p = point{1, 2}")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_argument_write(rnd, c):
    for p, params in ((c.bad, "a"), (c.good, "a mut")):
        p.add(f"func f({params}) {{")
        p.add("    a = 0", p is c.bad)
        p.add("}")
        p.add("func main() {")
        p.add("    var x = 1")
        p.add("    f(x)")
        p.add("    exit(0)")
        p.add("}")


def rule_array_argument_write(rnd, c):
    for p, params in ((c.bad, "arr[]"), (c.good, "arr mut []")):
        p.add(f"func f({params}) {{")
        p.add("    arr[0] = 1", p is c.bad)
        p.add("}")
        p.add("func main() {")
        p.add("    var a = [3]")
        p.add("    f(a)")
        p.add("    exit(0)")
        p.add("}")


def rule_element_write(rnd, c):
    for p, params in ((c.bad, "arr[]"), (c.good, "arr mut []")):
        p.add(f"func f({params}) {{")
        p.add("    foo arr {")
        p.add("        e = 0", p is c.bad)
        p.add("    }")
        p.add("}")
        p.add("func main() {")
        p.add("    var a = [3]")
        p.add("    f(a)")
        p.add("    exit(0)")
        p.add("}")


def rule_result_unassigned(rnd, c):
    # the position is somewhere in the function
    for p, last in ((c.bad, "    res = 1"), (c.good, "    res = 1")):
        p.add("func f(v) res {")
        if p is c.bad:
            p.add("    if v == 0 return", True)
            p.add(last, True)
            p.error_lines.update({1, 2, 3, 4})
        else:
            p.add("    res = 0")
            p.add("    if v == 0 return")
            p.add(last)
        p.add("}")
        p.add("func main() {")
        p.add("    var x = f(1)")
        p.add("    exit(0)")
        p.add("}")


def rule_dat_variable(rnd, c):
    for p, init in ((c.bad, "calls"), (c.good, "7")):
        p.add("dat calls = 0")
        p.add(f"dat twice = {init}", p is c.bad)
        p.add("func main() {")
        p.add("    exit(0)")
        p.add("}")


def rule_dat_after_var(rnd, c):
    for p in (c.bad, c.good):
        if p is c.bad:
            p.add("var g = 1")
            p.add("dat d = 2", True)
        else:
            p.add("dat d = 2")
            p.add("var g = 1")
        p.add("func main() {")
        p.add("    exit(0)")
        p.add("}")


def rule_alias_result(rnd, c):
    for p, text in ((c.bad, "x = add_to_zero(x)"),
                    (c.good, "var y = add_to_zero(x)")):
        p.add("func add_to_zero(v) res {")
        p.add("    res = 0")
        p.add("    res = res + v")
        p.add("}")
        p.add("func main() {")
        p.add("    var x = 5")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_record_overwrite(rnd, c):
    for p, text in ((c.bad, "p = {p.y, p.x}"), (c.good, "p = {2, 1}")):
        p.add("type point {x, y}")
        p.add("func main() {")
        p.add("    var p = point{1, 2}")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_constant_index(rnd, c):
    n = rnd.randint(2, 6)
    t = rnd.choice(["i8", "i16", "i32"])
    bad = rnd.choice([n, n + 1, -1])
    for p, i in ((c.bad, bad), (c.good, n - 1)):
        p.add("func main() {")
        p.add(f"    var arr = {t}[{n}]")
        p.add(f"    arr[{i}] = 1", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_wider_element(rnd, c):
    for p, text in ((c.bad, "var s = small + wide"),
                    (c.good, "var s = small + i8(wide)")):
        p.add("func main() {")
        p.add("    var small = i8(1)")
        p.add("    var wide = 5")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_mut_let(rnd, c):
    for p, text in ((c.bad, "inc(a)"), (c.good, "inc(b)")):
        p.add("func inc(v mut) { v = v + 1 }")
        p.add("func main() {")
        p.add("    var b = 1")
        p.add("    let a = b + 1")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_mut_literal(rnd, c):
    for p, text in ((c.bad, "inc(5)"), (c.good, "inc(b)")):
        p.add("func inc(v mut) { v = v + 1 }")
        p.add("func main() {")
        p.add("    var b = 1")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_unknown_name(rnd, c):
    for p, text in ((c.bad, "var y = nothing + 1"), (c.good, "var y = z + 1")):
        p.add("func main() {")
        p.add("    var z = 1")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_argument_count(rnd, c):
    for p, text in ((c.bad, "var y = f2(1)"), (c.good, "var y = f2(1, 2)")):
        p.add("func f2(a, b) res { res = a + b }")
        p.add("func main() {")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_repeated_name(rnd, c):
    for p in (c.bad, c.good):
        p.add("func main() {")
        p.add("    var a = 1")
        if p is c.bad:
            p.add("    var a = 2", True)
        else:
            p.add("    {")
            p.add("        var a = 2")
            p.add("    }")
        p.add("    exit(0)")
        p.add("}")


def rule_integer_condition(rnd, c):
    v = rnd.randint(1, 50)
    for p, text in ((c.bad, "if a exit(1)"), (c.good, "if a != 0 exit(1)")):
        p.add("func main() {")
        p.add(f"    var a = {v}")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_bool_compared(rnd, c):
    v = rnd.randint(0, 1)
    for p, text in ((c.bad, f"var c = b == {v}"),
                    (c.good, f"var c = int(b) == {v}")):
        p.add("func main() {")
        p.add("    var b = true")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_hidden_parameter(rnd, c):
    v = rnd.randint(1, 50)
    for p, text in ((c.bad, f"var x = {v}"), (c.good, f"var y = {v}")):
        p.add("func f(x) res {")
        p.add(f"    {text}", p is c.bad)
        p.add("    res = x")
        p.add("}")
        p.add("func main() {")
        p.add("    exit(f(1))")
        p.add("}")


def rule_break_outside(rnd, c):
    for p in (c.bad, c.good):
        p.add("func main() {")
        p.add("    var a = 1")
        if p is c.bad:
            p.add("    if a == 1 break", True)
        else:
            p.add("    loop {")
            p.add("        if a == 1 break")
            p.add("    }")
        p.add("    exit(0)")
        p.add("}")


def rule_and_or(rnd, c):
    for p, text in ((c.bad, "if a == 1 and b == 2 or a == 3 exit(1)"),
                    (c.good, "if (a == 1 and b == 2) or a == 3 exit(1)")):
        p.add("func main() {")
        p.add("    var a = 1")
        p.add("    var b = 2")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_unknown_method(rnd, c):
    for p, text in ((c.bad, "p.nothing()"), (c.good, "p.sum()")):
        p.add("type point {x, y}")
        p.add("func point.sum() res { res = self.x + self.y }")
        p.add("func main() {")
        p.add("    var p = point{1, 2}")
        p.add(f"    var q = {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_unknown_field(rnd, c):
    for p, text in ((c.bad, "p.z"), (c.good, "p.y")):
        p.add("type point {x, y}")
        p.add("func main() {")
        p.add("    var p = point{1, 2}")
        p.add(f"    var q = {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_shift_count(rnd, c):
    for p, count in ((c.bad, 8), (c.good, 7)):
        p.add("func main() {")
        p.add("    var s = i8(1)")
        p.add(f"    var t = s << {count}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_constant_overflow(rnd, c):
    for p, v in ((c.bad, 100), (c.good, 27)):
        p.add("func main() {")
        p.add(f"    var s = i8(100) + {v}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_array_sizes(rnd, c):
    for p, text in ((c.bad, "a = b"), (c.good, "var c = a")):
        p.add("func main() {")
        p.add("    var a = i32[4]")
        p.add("    var b = i32[3]")
        p.add(f"    {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_unknown_type(rnd, c):
    for p, name in ((c.bad, "nothing"), (c.good, "small")):
        p.add("type buffer<capacity> {")
        p.add("    items i8[capacity],")
        p.add("}")
        p.add("type small = buffer<4>")
        p.add("func main() {")
        p.add(f"    var s = {name}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


def rule_wrong_record_field_count(rnd, c):
    for p, text in ((c.bad, "point{1, 2, 3}"), (c.good, "point{1, 2}")):
        p.add("type point {x, y}")
        p.add("func main() {")
        p.add(f"    var p = {text}", p is c.bad)
        p.add("    exit(0)")
        p.add("}")


RULES = [
    (rule_assign_let, ()), (rule_assign_constant, ()), (rule_narrowing, ()),
    (rule_narrowing_expression, ()), (rule_let_array, ()),
    (rule_let_instance, ()), (rule_argument_write, ()),
    (rule_array_argument_write, ()), (rule_element_write, ()),
    (rule_result_unassigned, ()), (rule_dat_variable, ()),
    (rule_dat_after_var, ()), (rule_alias_result, ()),
    (rule_record_overwrite, ()), (rule_constant_index, ()),
    (rule_wider_element, ()), (rule_mut_let, ()), (rule_mut_literal, ()),
    (rule_unknown_name, ()), (rule_argument_count, ()),
    (rule_repeated_name, ()), (rule_break_outside, ()), (rule_and_or, ()),
    (rule_unknown_method, ()),
    (rule_unknown_field, ()), (rule_shift_count, ("--checks=shift",)),
    (rule_constant_overflow, ("--checks=overflow",)),
    (rule_array_sizes, ()), (rule_unknown_type, ()),
    (rule_wrong_record_field_count, ()), (rule_integer_condition, ()),
    (rule_bool_compared, ()), (rule_hidden_parameter, ()),
]

POSITION = re.compile(r"^.+?:(\d+):(\d+): \S", re.M)


def compile_only(path, target, options):
    return fl.compile_source(fl.COMPILER_ASAN, path,
                             ["--target=" + target] + list(options),
                             os.devnull)


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    rule, options = RULES[index % len(RULES)]
    case = Case(rule.__name__, options)
    rule(rnd, case)
    # note: the mistake of a 'break' is only one outside of a loop
    for p in (case.bad, case.good):
        decorate(p, rnd, rule is not rule_break_outside)
    work = tempfile.mkdtemp(prefix="fuzzr-")
    found = []
    try:
        bad_path = os.path.join(work, "bad.baz")
        good_path = os.path.join(work, "good.baz")
        with open(bad_path, "w") as f:
            f.write(case.bad.text())
        with open(good_path, "w") as f:
            f.write(case.good.text())
        for target in fl.TARGETS:
            r = compile_only(bad_path, target, case.options)
            if r.timed_out:
                found.append(("reject-crash", f"{case.name} hang", ""))
                continue
            if r.signal or fl.known_crash(r.stderr) or any(
                    t in r.stderr for t in fl.SANITIZER_TEXT):
                found.append(("reject-crash", f"{case.name} {target}",
                              r.stderr[:400]))
                continue
            if r.returncode == 0:
                found.append(("reject-accepted", f"{case.name} {target}",
                              "accepted"))
                continue
            # note: an error inside a function that is inlined also names the
            #       call, 'called from', so any of the positions may be it
            lines = {int(m.group(1)) for m in POSITION.finditer(r.stderr)}
            if not lines:
                found.append(("reject-line", f"{case.name} no position",
                              r.stderr[:300]))
            elif not lines & case.bad.error_lines:
                found.append((
                    "reject-line", f"{case.name} {target}",
                    f"lines {sorted(lines)} not in "
                    f"{sorted(case.bad.error_lines)}: {r.stderr[:300]}"))
            g = compile_only(good_path, target, case.options)
            if g.returncode:
                found.append(("reject-over", f"{case.name} {target}",
                              g.stderr[:300]))
        source = case.bad.text() + "\n# twin\n" + "\n".join(
            "# " + line for line in case.good.lines)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return source, found


def main():
    count, seed, jobs = fl.arguments(len(RULES) * 3)
    return fl.run_fuzzer(fuzz_one, count, seed, jobs)


if __name__ == "__main__":
    sys.exit(main())
