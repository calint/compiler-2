#!/usr/bin/env python3
# end to end fuzz of '--checks=alias': random programs whose destination is the
# element 'e' of a 'foo' or a 'mut' parameter, the value reads a variable; the
# verdict of the compiler ("may share storage" or not) against a brute force of
# the bytes, every value of every run-time index is tried
#
# usage: alias-foo-fuzz.py [programs] [seed] [compiler]
# exits 1 on a missed overlap (accepted, bytes can be the same), on a false
# positive (rejected, bytes never are) or on an error that is not an alias error
#
# note: a read under the name of the destination ('e.f = e.g') is handled where
#       the value is compiled, not by this check, so it must be accepted

import os
import random
import subprocess
import sys
import tempfile

SCALARS = [("i8", 1), ("i16", 2), ("i32", 4), ("i64", 8)]


class Type:
    """a scalar, a struct or an array, with the layout of the compiler"""

    def __init__(self, kind, name=None, size=0, align=1, fields=None, elem=None,
                 length=0):
        self.kind = kind
        self.name = name
        self.size = size
        self.align = align
        self.fields = fields or []  # (name, offset, type)
        self.elem = elem
        self.length = length

    @staticmethod
    def scalar(rng):
        name, size = rng.choice(SCALARS)
        return Type("scalar", name=name, size=size, align=size)

    @staticmethod
    def array(elem, length):
        return Type("array", size=elem.size * length, align=elem.align, elem=elem,
                    length=length)

    @staticmethod
    def struct(name, fields):
        align = max(f[2].align for f in fields)
        end = max(off + t.size for _, off, t in fields)
        size = (end + align - 1) // align * align
        return Type("struct", name=name, size=size, align=align, fields=fields)

    def decl(self):
        if self.kind in ("scalar", "struct"):
            return self.name
        return f"{self.elem.decl()}[{self.length}]"


def make_types(rng):
    types = []
    for ti in range(rng.randint(1, 4)):
        fields = []
        offset = 0
        for fi in range(rng.randint(1, 4)):
            base = rng.choice(types) if types and rng.random() < 0.5 else Type.scalar(rng)
            ft = Type.array(base, rng.randint(1, 3)) if rng.random() < 0.4 else base
            offset = (offset + ft.align - 1) // ft.align * ft.align
            fields.append((f"f{fi}", offset, ft))
            offset += ft.size
        types.append(Type.struct(f"t{ti}", fields))
    return types


class Program:
    def __init__(self, rng):
        self.rng = rng
        self.run_time_vars = 0

    def new_var(self):
        name = f"k{self.run_time_vars}"
        self.run_time_vars += 1
        return name

    def step_into_array(self, cur, text, steps):
        """an index of the array 'cur': constant or a run-time variable"""
        if self.rng.random() < 0.5:
            k = self.rng.randrange(cur.length)
            text += f"[{k}]"
            steps.append(("fixed", k, cur.elem.size))
        else:
            text += f"[{self.new_var()}]"
            steps.append(("run", cur.length, cur.elem.size))
        return text

    def walk(self, tp, text, want=None):
        """random walk to a scalar, of the type 'want' when given; (text, steps, leaf)"""
        steps = []
        cur = tp
        while cur.kind != "scalar":
            if cur.kind == "struct":
                name, off, ft = self.rng.choice(cur.fields)
                text += "." + name
                steps.append(("field", off))
                cur = ft
            else:
                text = self.step_into_array(cur, text, steps)
                cur = cur.elem
        if want is not None and cur.name != want:
            return None
        return text, steps, cur

    def walk_to(self, tp, text, want):
        for _ in range(200):
            saved = self.run_time_vars
            found = self.walk(tp, text, want)
            if found:
                return found
            self.run_time_vars = saved
        return None

    def walk_to_array(self, root):
        """a path from the variable to an array: (text, steps, array)"""
        for _ in range(100):
            text, steps, cur = "r", [], root
            while cur.kind != "scalar":
                if cur.kind == "array" and self.rng.random() < 0.6:
                    return text, steps, cur
                if cur.kind == "struct":
                    name, off, ft = self.rng.choice(cur.fields)
                    text += "." + name
                    steps.append(("field", off))
                    cur = ft
                else:
                    text = self.step_into_array(cur, text, steps)
                    cur = cur.elem
        return None


def starts(steps):
    """every byte offset the access can start at, for every value of its indexes"""
    out = [0]
    for st in steps:
        if st[0] == "field":
            out = [o + st[1] for o in out]
        elif st[0] == "fixed":
            out = [o + st[1] * st[2] for o in out]
        else:
            out = [o + k * st[2] for o in out for k in range(st[1])]
    return out


def can_be_same_bytes(a_steps, b_steps, size):
    a, b = starts(a_steps), starts(b_steps)
    return any(x < y + size and y < x + size for x in a for y in b)


def type_declarations(types):
    out = []
    for t in types:
        out.append(f"type {t.name} {{")
        for name, _, ft in t.fields:
            out.append(f"    {name} {ft.decl()},")
        out.append("}\n")
    return out


def make_root(rng, types):
    root_t = types[-1]
    if rng.random() < 0.4:
        return Type.array(root_t, rng.randint(1, 3))
    return root_t


def gen_foo(rng):
    types = make_types(rng)
    root = make_root(rng, types)
    p = Program(rng)
    found = p.walk_to_array(root)
    if not found:
        return None
    foo_text, foo_steps, arr = found
    elem = arr.elem
    if elem.kind == "scalar":
        return None  # the element of a scalar array has no path
    dst = p.walk(elem, "e")
    dst_text, dst_steps, leaf = dst
    # e is any element of the array
    e_steps = foo_steps + [("run", arr.length, elem.size)]
    if rng.random() < 0.3:
        rd = p.walk_to(elem, "e", leaf.name)
        rd_base = e_steps
    else:
        rd = p.walk_to(root, "r", leaf.name)
        rd_base = []
    if not rd:
        return None
    rd_text, rd_steps, _ = rd
    truth = can_be_same_bytes(e_steps + dst_steps, rd_base + rd_steps, leaf.size)
    if rd_text.startswith("e"):
        truth = False
    src = type_declarations(types)
    src.append(f"var r = {root.decl()}\n")
    for i in range(p.run_time_vars):
        src.append(f"var k{i} = 0")
    src.append("\nfunc main() {")
    src.append(f"    foo {foo_text} {{")
    src.append(f"        {dst_text} = {rd_text}")
    src.append("    }")
    src.append("}")
    return "\n".join(src) + "\n", truth


def gen_param(rng):
    types = make_types(rng)
    root = make_root(rng, types)
    p = Program(rng)
    dst = p.walk(root, "r")
    dst_text, dst_steps, leaf = dst
    rd = p.walk_to(root, "r", leaf.name)
    if not rd:
        return None
    rd_text, rd_steps, _ = rd
    truth = can_be_same_bytes(dst_steps, rd_steps, leaf.size)
    modifier = "" if rng.random() < 0.5 else "noinline "
    src = type_declarations(types)
    src.append(f"var r = {root.decl()}\n")
    for i in range(p.run_time_vars):
        src.append(f"var k{i} = 0")
    src.append(f"\nfunc {modifier}put(x mut {leaf.name}) {{")
    src.append(f"    x = {rd_text}")
    src.append("}\n")
    src.append("func main() {")
    src.append(f"    put({dst_text})")
    src.append("}")
    return "\n".join(src) + "\n", truth


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 5000
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    compiler = sys.argv[3] if len(sys.argv) > 3 else "../../baz"
    rng = random.Random(seed)
    stats = {"foo": {}, "param": {}}
    failures = 0
    with tempfile.TemporaryDirectory(dir="/tmp") as tmp:
        path = os.path.join(tmp, "p.baz")
        done = 0
        while done < count:
            scenario = "foo" if rng.random() < 0.5 else "param"
            made = (gen_foo if scenario == "foo" else gen_param)(rng)
            if made is None:
                continue
            src, truth = made
            with open(path, "w") as f:
                f.write(src)
            run = subprocess.run([compiler, "--checks=alias", path],
                                 capture_output=True, text=True, check=False)
            rejected = "may share storage" in run.stderr
            if run.returncode != 0 and not rejected:
                verdict = "other error"
            elif truth and not rejected:
                verdict = "missed overlap"
            elif not truth and rejected:
                verdict = "false positive"
            else:
                verdict = "reject" if truth else "accept"
            stats[scenario][verdict] = stats[scenario].get(verdict, 0) + 1
            if verdict in ("other error", "missed overlap", "false positive"):
                failures += 1
                if failures <= 5:
                    print(f"{scenario}: {verdict}\n{run.stderr.strip()[:300]}\n{src}")
            done += 1
    for scenario, counts in stats.items():
        print(f"{scenario}: {dict(sorted(counts.items()))}")
    return 1 if failures else 0


sys.exit(main())
