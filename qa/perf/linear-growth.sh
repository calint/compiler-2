#!/bin/sh
# checks that compile time grows linearly in the inputs that once grew
# faster: a long expression on one line, many statements on one line, a path
# of constant indexes, a path of fields, the result of a chain of nested
# types, a chain of noinline functions, the fields of one type and the dats
# (each compiled at a size and at 4 times it, the best of 3 runs; a ratio
# above 8 fails, linear is about 4 and quadratic about 16)
#
# usage: linear-growth.sh
set -eu

cd "$(dirname "$0")"

[ -x ../../baz ] || ../../make.sh build

exec python3 -I - <<'EOF'
import os
import subprocess
import sys
import tempfile
import time

BAZ = os.path.abspath("../../baz")
RUNS = 3
FACTOR = 4
LIMIT = 8.0


def long_expression(n):
    terms = " + ".join(["x"] * n)
    return f"func main() {{\nvar x = 1\nvar y = {terms}\nexit(i8(y & 1))\n}}\n"


def long_line(n):
    statements = " ".join(["x = x + 1"] * n)
    return f"func main() {{\nvar x = 0\n{statements}\nexit(i8(x & 1))\n}}\n"


def constant_index_path(n):
    types = ["type t0 {a, b i8}"]
    types += [f"type t{i} {{a t{i - 1}[1], b i8}}" for i in range(1, n)]
    path = "".join([".a[0]"] * (n - 1))
    return "\n".join(types) + \
        f"\nfunc main() {{\nvar p = t{n - 1}\np{path}.a = 1\nexit(0)\n}}\n"


def field_path(n):
    types = ["type t0 {a, b}"]
    types += [f"type t{i} {{a t{i - 1}, b}}" for i in range(1, n)]
    path = "".join([".a"] * n)
    return "\n".join(types) + \
        f"\nfunc main() {{\nvar p = t{n - 1}\nexit(i8(p{path}))\n}}\n"


# every type adds a gap of padding, zeroed for the result of the call
def nested_result(n):
    types = ["type t0 {a i8, b i32}"]
    types += [f"type t{i} {{c i8, a t{i - 1}}}" for i in range(1, n)]
    return "\n".join(types) + \
        f"\nfunc noinline make() res t{n - 1} {{\nres = {{}}\n}}\n" \
        "func main() {\nvar p = make()\nexit(p.c)\n}\n"


# every function is a body of its own, found by its function when called
def noinline_chain(n):
    funcs = ["func noinline f0() {}"]
    funcs += [f"func noinline f{i}() {{ f{i - 1}() }}" for i in range(1, n)]
    return "\n".join(funcs) + \
        f"\nfunc main() {{\nf{n - 1}()\nexit(0)\n}}\n"


# every field is looked up by its name, at its definition and in a path
def fields_of_one_type(n):
    fields = ", ".join(f"f{i} i8" for i in range(n))
    writes = "".join(f"p.f{i} = 1\n" for i in range(0, n, 16))
    return f"type t {{ {fields} }}\n" \
        f"func main() {{\nvar p = t\n{writes}exit(p.f{n - 1})\n}}\n"


# every dat is placed by its name
def dats(n):
    lines = "".join(f"dat g{i} = {i % 100}\n" for i in range(n))
    return f"{lines}func main() {{\nexit(i8(g{n - 1}))\n}}\n"


# name, generator, base size, options
CASES = [
    ("long expression on one line", long_expression, 10000, []),
    ("statements on one line", long_line, 5000, []),
    ("path of constant indexes", constant_index_path, 1600,
     ["--vars=0x10000000"]),
    ("path of fields", field_path, 20000, ["--vars=0x10000000"]),
    ("result of nested types", nested_result, 10000, ["--vars=0x10000000"]),
    ("chain of noinline functions", noinline_chain, 10000, []),
    ("fields of one type", fields_of_one_type, 16000, ["--vars=0x10000000"]),
    ("dats", dats, 16000, ["--vars=0x10000000"]),
]


def best_time(source, options, work):
    path = os.path.join(work, "prog.baz")
    with open(path, "w") as f:
        f.write(source)
    best = None
    for _ in range(RUNS):
        start = time.perf_counter()
        result = subprocess.run([BAZ, *options, path],
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.PIPE, text=True)
        elapsed = time.perf_counter() - start
        if result.returncode != 0:
            sys.exit(f"compile failed: {result.stderr.strip()}")
        best = elapsed if best is None else min(best, elapsed)
    return best


failed = False
with tempfile.TemporaryDirectory() as work:
    for name, generate, size, options in CASES:
        small = best_time(generate(size), options, work)
        large = best_time(generate(size * FACTOR), options, work)
        ratio = large / small
        verdict = "ok" if ratio <= LIMIT else "FAILED"
        print(f"{name}: {size} {small:.3f} s, {size * FACTOR} {large:.3f} s, "
              f"ratio {ratio:.1f} {verdict}")
        failed = failed or ratio > LIMIT

sys.exit(1 if failed else 0)
EOF
