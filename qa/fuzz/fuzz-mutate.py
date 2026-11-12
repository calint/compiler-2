#!/usr/bin/env python3
# structure-aware mutation fuzzer of the compiler
#
# mutates the sources of 'qa/coverage/tests' by their structure, mostly those
# the tests expect to be accepted: types and array sizes, numbers at the edges
# of the types, 'mut' and 'noinline' added and removed, keywords and operators
# swapped for their kin, statements deleted, repeated, moved, swapped, wrapped
# in 'if', 'loop', 'foo' and blocks or taken from another test, blocks
# deleted, repeated, unwrapped and nested, definitions deleted, repeated,
# swapped, renamed or taken from another test, arguments of calls deleted,
# added, swapped and wrapped. 'fuzz-compiler.py' mutates tokens and runs the
# programs, this fuzzer keeps more mutants valid and only compiles them, with
# the compiler built with AddressSanitizer and UndefinedBehaviorSanitizer
# ('build-asan.sh'), on the four targets in a random order with a random
# option list (checks, '--nopt', '--versions', '--report',
# '--reproduce-source'); after a rejection it goes on to the next target
# about every third time. findings, the first example of each is kept in
# 'findings/':
#
#   compiler-crash    sanitizer report, abort, signal, internal error or an
#                     unexpected exit code
#   compiler-hang     no result in time
#   bad-diagnostic    a rejected source without a 'file:line:column: message'
#
# usage: fuzz-mutate.py [count] [seed] [jobs]
import os
import random
import re
import sys
import tempfile

import fuzzlib as fl

ALL_TARGETS = ["x86_64", "rv32i", "rv32i-qemu", "rv32i-fpga"]
TYPES = ["i8", "i16", "i32", "i64", "bool"]
# the types that replace one, also arrays of odd sizes and a user type
NEW_TYPES = TYPES + ["i8[2]", "i64[1]", "bool[3]", "i8[0]", "i16[65536]",
                     "i32[2][2]", "str"]
NEW_SIZES = ["[0]", "[1]", "[2]", "[3]", "[255]", "[256]", "[65536]",
             "[2147483648]", "[4294967296]", "[1][1]", "", "[2][0]", "[i]",
             "[-1]", "[1 + 1]"]
# the numbers at the edges of the types and of the shift counts
EDGE_NUMBERS = ["0", "1", "-1", "127", "128", "-128", "-129", "255", "256",
                "32767", "32768", "-32768", "-32769", "65535", "2147483647",
                "2147483648", "-2147483648", "-2147483649", "4294967295",
                "9223372036854775807", "-9223372036854775808",
                "9223372036854775808", "0x7f", "0x80", "0xff", "0x7fffffff",
                "0x80000000", "0xffffffff", "0x8000000000000000", "0b1", "63",
                "64", "31", "32", "7", "8", "15", "16", "-0", "- -1"]
# a word and a word of the same kind that may take its place
KIN_WORDS = [("var", "let"), ("let", "var"), ("var", "dat"), ("dat", "var"),
             ("let", "dat"), ("loop", "if true"), ("break", "continue"),
             ("continue", "break"), ("return", "break"), ("break", "return"),
             ("and", "or"), ("==", "!="), ("true", "false"), ("res", "self"),
             ("self", "res"), ("if", "loop"), ("else", ""), ("exit", "write"),
             ("array_copy", "arrays_equal"), ("arrays_equal", "array_copy"),
             ("read", "write"), ("write", "read"), ("<<", ">>"), ("/", "%"),
             ("-", "+")]
# the words that a use of a name does not replace
DEFINING_WORDS = {"func", "var", "let", "dat", "type", "if", "else", "loop",
                  "mut", "noinline", "include"}
WORD = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")
NUMBER = re.compile(
    r"(?<![A-Za-z_0-9.])-?(0x[0-9a-fA-F_]+|0b[01_]+|\d[\d_]*)\b")
# the case of a test in 'cases.sh', e.g. 'SRC=0001 && EXP=58 && RUN'
CASE = re.compile(r"^\s*SRC=(\w+) .*?(\w+)\s*$", re.M)

# (name, text) of the tests and of those the tests expect to be accepted
SOURCES = []
ACCEPTED = []


def load_sources():
    # also the initializer of the processes, which may start without them
    if SOURCES:
        return
    SOURCES.extend(fl.test_sources())
    with open(os.path.join(fl.ROOT, "qa", "coverage", "cases.sh")) as f:
        cases = f.read()
    # note: a file without a case is a part that a test includes
    accepted = {f"{name}.baz" for name, helper in CASE.findall(cases)
                if helper != "COMPERR"}
    ACCEPTED.extend(s for s in SOURCES if s[0] in accepted)


def definitions(text):
    # the (start, end) ranges of the definitions at the top level: from the
    # start of the line of a '{' at depth 0 to its '}'
    ranges = []
    depth = 0
    start = None
    quote = None
    i = 0
    while i < len(text):
        c = text[i]
        if quote:
            if c == "\\":
                i += 2
                continue
            if c == quote:
                quote = None
        elif c in "\"'":
            quote = c
        elif c == "#":
            end = text.find("\n", i)
            i = len(text) if end < 0 else end
            continue
        elif c == "{":
            if depth == 0:
                start = text.rfind("\n", 0, i) + 1
            depth += 1
        elif c == "}":
            depth -= 1
            if depth < 0:
                return []
            if depth == 0 and start is not None:
                ranges.append((start, i + 1))
                start = None
        i += 1
    return ranges


def statement_lines(lines):
    # the indexes of the indented lines that hold one whole statement
    found = []
    for k, line in enumerate(lines):
        s = line.strip()
        if not line.startswith((" ", "\t")) or not s or s.startswith("#"):
            continue
        if s.count("{") != s.count("}") or s.count("(") != s.count(")"):
            continue
        if s == "{" or s.startswith("}"):
            continue
        found.append(k)
    return found


def body_blocks(lines):
    # the (open, close) line indexes of the blocks of several lines in a body
    found = []
    opened = []
    for k, line in enumerate(lines):
        s = line.strip()
        if s.startswith("#"):
            continue
        balance = s.count("{") - s.count("}")
        if balance > 0 and s.endswith("{"):
            opened.append(k)
        elif balance < 0 and opened:
            first = opened.pop()
            if lines[first].startswith((" ", "\t")):
                found.append((first, k))
    return found


def replace_match(text, match, new):
    return text[:match.start()] + new + text[match.end():]


def mutate_type(rnd, text):
    matches = list(re.finditer(r"\b(i8|i16|i32|i64|bool)\b", text))
    if not matches:
        return text
    return replace_match(text, rnd.choice(matches), rnd.choice(NEW_TYPES))


def mutate_size(rnd, text):
    matches = list(re.finditer(r"\[(\d+)\]", text))
    if not matches:
        return text
    return replace_match(text, rnd.choice(matches), rnd.choice(NEW_SIZES))


def mutate_number(rnd, text):
    matches = list(NUMBER.finditer(text))
    if not matches:
        return text
    return replace_match(text, rnd.choice(matches), rnd.choice(EDGE_NUMBERS))


def mutate_mut(rnd, text):
    # removes or adds 'mut' on a parameter, else 'noinline' on a function
    choice = rnd.random()
    if choice < 0.35:
        matches = list(re.finditer(r"\bmut ", text))
        if matches:
            return replace_match(text, rnd.choice(matches), "")
    if choice < 0.7:
        # a parameter, e.g. 'p i32' after '(' or ','
        matches = list(re.finditer(r"[(,]\s*([A-Za-z_]\w*)\s+(?=[A-Za-z_])",
                                   text))
        if matches:
            at = rnd.choice(matches).start(1)
            return text[:at] + "mut " + text[at:]
    funcs = list(re.finditer(r"^func ", text, re.M))
    noinlines = list(re.finditer(r"\bnoinline ", text))
    if noinlines and rnd.random() < 0.5:
        return replace_match(text, rnd.choice(noinlines), "")
    if funcs:
        at = rnd.choice(funcs).end()
        return text[:at] + "noinline " + text[at:]
    return text


def mutate_kin_word(rnd, text):
    old, new = rnd.choice(KIN_WORDS)
    matches = list(re.finditer(r"(?<!\w)" + re.escape(old) + r"(?!\w)", text))
    if not matches:
        return text
    return replace_match(text, rnd.choice(matches), new)


def mutate_statement(rnd, text, other):
    lines = text.split("\n")
    statements = statement_lines(lines)
    if not statements:
        return text
    k = rnd.choice(statements)
    indent = re.match(r"\s*", lines[k]).group(0)
    statement = lines[k].strip()
    kind = rnd.randrange(11)
    if kind == 0:
        del lines[k]
    elif kind == 1:
        lines.insert(k, lines[k])
    elif kind == 2:
        lines[k] = f"{indent}if true {{ {statement} }}"
    elif kind == 3:
        lines[k] = f"{indent}loop {{ {statement} break }}"
    elif kind == 4:
        lines[k] = f"{indent}{{ {statement} }}"
    elif kind == 5:
        # to another place of a statement, maybe in another function
        moved = lines.pop(k)
        places = statement_lines(lines)
        lines.insert(rnd.choice(places) if places else k, moved)
    elif kind == 6:
        other_lines = other.split("\n")
        others = statement_lines(other_lines)
        if others:
            lines.insert(k, indent + other_lines[rnd.choice(others)].strip())
    elif kind == 7:
        lines[k] = (f"{indent}loop {{ if true {{ {statement} continue }} "
                    "break }")
    elif kind == 8:
        # over a name of the source, an array when the mutant is accepted
        array = rnd.choice(WORD.findall(text))
        lines[k] = f"{indent}foo {array} {{ {statement} }}"
    elif kind == 9:
        lines[k] = (f"{indent}if false {{ }} else if true {{ {statement} }} "
                    "else { exit(3) }")
    else:
        other_k = rnd.choice(statements)
        lines[k], lines[other_k] = lines[other_k], lines[k]
    return "\n".join(lines)


def mutate_block(rnd, text):
    lines = text.split("\n")
    blocks = body_blocks(lines)
    if not blocks:
        return text
    first, last = rnd.choice(blocks)
    kind = rnd.randrange(5)
    if kind == 0:
        del lines[first:last + 1]
    elif kind == 1:
        lines[last + 1:last + 1] = lines[first:last + 1]
    elif kind == 2:
        # keeps the body without its braces
        del lines[last]
        del lines[first]
    elif kind == 3:
        lines.insert(last + 1, "break }")
        lines.insert(first, "loop {")
    else:
        depth = rnd.choice([2, 8, 40])
        lines.insert(last + 1, "}" * depth)
        lines.insert(first, "if true { " * depth)
    return "\n".join(lines)


def mutate_definition(rnd, text, other):
    ranges = definitions(text)
    other_ranges = definitions(other)
    kind = rnd.randrange(5)
    if kind == 0 and ranges:
        start, end = rnd.choice(ranges)
        return text[:start] + text[end:]
    if kind == 1 and ranges:
        start, end = rnd.choice(ranges)
        return text[:end] + "\n" + text[start:end] + text[end:]
    if kind == 2 and other_ranges:
        start, end = rnd.choice(other_ranges)
        at = rnd.choice([0, len(text)] + [s for s, _ in ranges])
        return text[:at] + other[start:end] + "\n" + text[at:]
    if kind == 3 and len(ranges) > 1:
        (start1, end1), (start2, end2) = sorted(rnd.sample(ranges, 2))
        return (text[:start1] + text[start2:end2] + text[end1:start2]
                + text[start1:end1] + text[end2:])
    if not ranges:
        return text
    # renames a definition to the name of another one or of a built-in
    start, end = rnd.choice(ranges)
    name = re.match(r"\s*(?:func|type|noinline|\s)*\s*([A-Za-z_]\w*)",
                    text[start:end])
    names = re.findall(r"^(?:func\s+(?:noinline\s+)?|type\s+)([A-Za-z_]\w*)",
                       text, re.M)
    if not name or not names:
        return text
    new = rnd.choice(names + ["main", "res", "self", "e", "str", "i64"])
    return text[:start + name.start(1)] + new + text[start + name.end(1):]


def mutate_name(rnd, text):
    # a use of a name becomes another name of the source
    uses = [m for m in WORD.finditer(text) if m.group(0) not in DEFINING_WORDS]
    if not uses:
        return text
    names = sorted(set(WORD.findall(text)))
    return replace_match(text, rnd.choice(uses), rnd.choice(names))


def mutate_arguments(rnd, text):
    calls = list(re.finditer(r"([A-Za-z_]\w*)\(([^()\n]*)\)", text))
    if not calls:
        return text
    call = rnd.choice(calls)
    callee = call.group(1)
    args = call.group(2).split(",") if call.group(2).strip() else []
    kind = rnd.randrange(5)
    if kind == 0 and args:
        del args[rnd.randrange(len(args))]
    elif kind == 1:
        added = rnd.choice(args + [" 1", " true", " x", " res"]) if args \
            else "1"
        args.insert(rnd.randrange(len(args) + 1), added)
    elif kind == 2 and len(args) > 1:
        i, j = rnd.sample(range(len(args)), 2)
        args[i], args[j] = args[j], args[i]
    elif kind == 3 and args:
        i = rnd.randrange(len(args))
        args[i] = f" {callee}({args[i].strip()})"
    elif kind == 4 and args:
        i = rnd.randrange(len(args))
        arg = args[i].strip()
        args[i] = rnd.choice([" {}", f" ({arg})", f" {arg}[0]", f" {arg}.x",
                              f" i8({arg})", f" -{arg}", f" not {arg}"])
    return text[:call.start(2)] + ",".join(args) + text[call.end(2):]


# the mutations by name, each as often as it is listed
MUTATIONS = ["type"] * 3 + ["size"] * 2 + ["number"] * 3 + ["mut"] * 3 + \
    ["kin-word"] * 3 + ["statement"] * 5 + ["block"] * 2 + \
    ["definition"] * 3 + ["name"] * 2 + ["arguments"] * 3


def mutate(rnd, text, other):
    # 'other' is the test that statements and definitions are taken from
    for _ in range(rnd.choice([1, 1, 2, 2, 3, 4, 6])):
        kind = rnd.choice(MUTATIONS)
        if kind == "type":
            text = mutate_type(rnd, text)
        elif kind == "size":
            text = mutate_size(rnd, text)
        elif kind == "number":
            text = mutate_number(rnd, text)
        elif kind == "mut":
            text = mutate_mut(rnd, text)
        elif kind == "kin-word":
            text = mutate_kin_word(rnd, text)
        elif kind == "statement":
            text = mutate_statement(rnd, text, other)
        elif kind == "block":
            text = mutate_block(rnd, text)
        elif kind == "definition":
            text = mutate_definition(rnd, text, other)
        elif kind == "name":
            text = mutate_name(rnd, text)
        else:
            text = mutate_arguments(rnd, text)
    return text


def random_options(rnd):
    options = [rnd.choice(["--checks=noub", "--checks=noub",
                           "--checks=noub,line", "--checks=",
                           "--checks=upper,overflow", "--checks=frame,stack",
                           "--checks=shift,division",
                           "--checks=overlap,lower"])]
    if rnd.random() < 0.3:
        options.append("--nopt")
    if rnd.random() < 0.3:
        options.append("--versions=" + rnd.choice(["smaller", "nested",
                                                   "first", "last"]))
    if rnd.random() < 0.15:
        options.append("--report=registers")
    if rnd.random() < 0.15:
        options.append("--reproduce-source")
    return options


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    name, text = rnd.choice(ACCEPTED if rnd.random() < 0.8 else SOURCES)
    _, other = rnd.choice(SOURCES)
    mutant = mutate(rnd, text, other)
    options = random_options(rnd)
    targets = ALL_TARGETS[:]
    rnd.shuffle(targets)
    findings = []
    stats = {"accepted": 0, "rejected": 0}
    with tempfile.TemporaryDirectory(prefix="fuzzm-") as work:
        # the tests include files of 'tests/include'
        os.symlink(os.path.join(fl.TESTS, "include"),
                   os.path.join(work, "include"))
        path = os.path.join(work, "f.baz")
        with open(path, "wb") as f:
            f.write(mutant.encode("latin-1", "replace"))
        for target in targets:
            command = ["--target=" + target] + options
            # note: the image of rv32i holds the 'dat' arrays, one of 2 GiB
            #       is past the limit of the file size of a run
            if target != "x86_64":
                command.append("--bin=" + os.devnull)
            c = fl.compile_source(fl.COMPILER_ASAN, path, command, os.devnull,
                                  cwd=work)
            finding = fl.classify_compile(c)
            if finding:
                findings.append(finding + (f"from {name}, "
                                           f"{' '.join(command)}\n"
                                           f"{c.stderr[-3000:]}",))
                break
            if c.returncode == 0:
                stats["accepted"] += 1
                continue
            stats["rejected"] += 1
            # the other targets reject it too in most cases
            if rnd.random() < 0.7:
                break
    return mutant, findings, stats


def main():
    count, seed, jobs = fl.arguments(500)
    load_sources()
    return fl.run_fuzzer(fuzz_one, count, seed, jobs,
                         heading=f", {len(SOURCES)} tests, "
                         f"{len(ACCEPTED)} accepted",
                         initializer=load_sources, chunksize=8)


if __name__ == "__main__":
    sys.exit(main())
