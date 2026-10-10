#!/usr/bin/env python3
# mutation fuzzer of the compiler and of the programs it accepts
#
# mutates the sources of 'qa/coverage/tests' (tokens, lines, numbers, types,
# operators, splices, deep nesting, huge literals, broken bytes) and feeds them
# to the compiler built with AddressSanitizer and UndefinedBehaviorSanitizer
# ('build-asan.sh'). findings, the first example of each is kept in
# 'findings/':
#
#   compiler-crash    sanitizer report, abort, signal or an unexpected exit code
#   compiler-hang     no result in time
#   bad-diagnostic    a rejected source without a 'file:line:column: message'
#   empty-output      an accepted source without code
#   invalid-asm       the assembler rejects the code of x86_64
#   link-failure      the linker rejects the object of x86_64
#   ub-signal         a program compiled with the checks of 'noub' ends with
#                     SIGSEGV, SIGFPE, SIGILL, SIGBUS or SIGABRT
#   cpu-error         a program compiled with 'noub' stops the emulator
#   diff-target       x86_64 and rv32i-fpga end differently (code or output)
#   diff-nopt         '--nopt' ends differently than the optimized program
#   diff-checks       a program that ends normally under 'noub' ends
#                     differently without checks
#   diff-line         '--checks=noub,line' ends differently than 'noub'
#   accept-diff       one target accepts what the other rejects
#   reproduce         '--reproduce-source' fails or writes a different source
#
# usage: fuzz-compiler.py [count] [seed] [jobs]
import concurrent.futures
import os
import random
import re
import shutil
import signal
import sys
import tempfile
import time

import fuzzlib as fl

sys.set_int_max_str_digits(0)

SEEDS = []
VOCABULARY = []
TOKEN = re.compile(r'\s+|[A-Za-z_][A-Za-z_0-9]*|0x[0-9a-fA-F]+|\d+|"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|#[^\n]*|.',
                   re.S)
OPERATORS = ["+", "-", "*", "/", "%", "<<", ">>", "&", "|", "^", "==", "!=",
             "<", ">", "<=", ">=", "and", "or", "not", "=", "~"]
TYPES = ["i8", "i16", "i32", "i64", "bool"]
KEYWORDS = ["func", "noinline", "mut", "type", "dat", "var", "let", "foo",
            "loop", "if", "else", "continue", "break", "return", "self", "and",
            "or", "not", "true", "false", "include", "array_copy",
            "array_length", "arrays_equal", "read", "write", "exit"]
NUMBERS = ["0", "1", "-1", "2", "3", "7", "8", "15", "16", "31", "32", "63",
           "64", "127", "128", "255", "256", "32767", "32768", "65535",
           "65536", "2147483647", "2147483648", "-2147483648", "4294967295",
           "4294967296", "9223372036854775807", "9223372036854775808",
           "99999999999999999999", "0x7fffffff", "0x80000000", "0xffffffff",
           "0x7fffffffffffffff", "0xffffffffffffffff", "0x", "00", "1_0"]
JUNK = ["\x00", "\r", "\t", "\x7f", "\xff", "\xc3\xa9", "\x1b", "\\", "`", "@",
        "$", "?", "\"", "'", "{", "}", "(", ")", "[", "]", ",", ";", ":", "."]
STRESS = ["deep-parens", "deep-brackets", "deep-unary", "long-chain",
          "long-identifier", "many-vars", "huge-array", "deep-blocks",
          "many-args", "deep-calls", "many-fields", "long-line",
          "huge-literal", "many-functions", "many-includes"]


def load_seeds():
    for name in sorted(os.listdir(fl.TESTS)):
        if not name.endswith(".baz"):
            continue
        path = os.path.join(fl.TESTS, name)
        # the huge tests only slow the fuzzing down
        if os.path.getsize(path) > 25000:
            continue
        with open(path, "rb") as f:
            text = f.read().decode("latin-1")
        # the tests of far jumps take half a minute to compile
        if "MiB" in text or "GiB" in text or "step4096" in text:
            continue
        SEEDS.append(text)
    words = set()
    for text in SEEDS:
        words.update(re.findall(r"[A-Za-z_][A-Za-z_0-9]*", text))
    VOCABULARY.extend(sorted(words))


def tokens_of(text):
    return TOKEN.findall(text)


def non_space(tokens):
    return [i for i, t in enumerate(tokens) if not t.isspace()]


def mutate_tokens(rnd, text):
    tokens = tokens_of(text)
    positions = non_space(tokens)
    if not positions:
        return text
    # the mutations that keep a program valid come more often, the others
    # find the parser's and the checker's mistakes
    kind = rnd.choice(["replace-number"] * 6 + ["replace-operator"] * 4
                      + ["number-edit"] * 5 + ["replace-type"] * 3
                      + ["negate", "swap-lines", "delete-line",
                         "duplicate-line", "rename", "replace-word"] * 2
                      + ["delete", "duplicate", "swap", "insert", "junk",
                         "truncate", "splice", "brace", "stress"])
    i = rnd.choice(positions)
    if kind == "delete":
        del tokens[i]
    elif kind == "duplicate":
        tokens.insert(i, tokens[i])
    elif kind == "swap":
        j = rnd.choice(positions)
        tokens[i], tokens[j] = tokens[j], tokens[i]
    elif kind == "replace-word":
        tokens[i] = rnd.choice(VOCABULARY + KEYWORDS)
    elif kind == "replace-number":
        numbers = [k for k in positions if tokens[k][:1].isdigit()]
        if numbers:
            tokens[rnd.choice(numbers)] = rnd.choice(NUMBERS)
    elif kind == "replace-operator":
        ops = [k for k in positions if tokens[k] in OPERATORS]
        if ops:
            tokens[rnd.choice(ops)] = rnd.choice(OPERATORS)
    elif kind == "replace-type":
        types = [k for k in positions if tokens[k] in TYPES]
        if types:
            tokens[rnd.choice(types)] = rnd.choice(TYPES)
    elif kind == "insert":
        tokens.insert(i, " " + rnd.choice(VOCABULARY + KEYWORDS + OPERATORS
                                          + NUMBERS) + " ")
    elif kind == "junk":
        tokens.insert(i, rnd.choice(JUNK))
    elif kind == "number-edit":
        numbers = [k for k in positions if tokens[k].isdigit()]
        if numbers:
            k = rnd.choice(numbers)
            if len(tokens[k]) < 30:
                tokens[k] = str(int(tokens[k]) + rnd.choice([-2, -1, 1, 2, 100]))
    elif kind == "brace":
        pairs = [k for k in positions if tokens[k] in "(){}[]"]
        if pairs:
            k = rnd.choice(pairs)
            if rnd.random() < 0.5:
                del tokens[k]
            else:
                tokens[k] = rnd.choice("(){}[]")
    elif kind == "rename":
        words = [k for k in positions if re.fullmatch(r"[A-Za-z_]\w*", tokens[k])]
        if len(words) > 2:
            a, b = rnd.sample(words, 2)
            old = tokens[a]
            for k in words:
                if tokens[k] == old and rnd.random() < 0.5:
                    tokens[k] = tokens[b]
    elif kind == "negate":
        tokens.insert(i, rnd.choice(["-", "not ", "~", "- -", "not not "]))
    text = "".join(tokens)
    lines = text.split("\n")
    if kind == "swap-lines" and len(lines) > 2:
        a, b = rnd.randrange(len(lines)), rnd.randrange(len(lines))
        lines[a], lines[b] = lines[b], lines[a]
    elif kind == "delete-line" and len(lines) > 2:
        del lines[rnd.randrange(len(lines))]
    elif kind == "duplicate-line" and len(lines) > 2:
        k = rnd.randrange(len(lines))
        lines.insert(k, lines[k])
    elif kind == "truncate":
        return text[:rnd.randrange(len(text) + 1)]
    elif kind == "splice":
        other = rnd.choice(SEEDS).split("\n")
        a = rnd.randrange(len(other))
        chunk = other[a:a + rnd.randint(1, 12)]
        k = rnd.randrange(len(lines) + 1)
        lines[k:k] = chunk
    elif kind == "stress":
        return stress(rnd, text)
    return "\n".join(lines)


def stress(rnd, text):
    # sizes and depths that break recursive and quadratic code
    kind = rnd.choice(STRESS)
    n = rnd.choice([100, 1000, 5000, 20000])
    if kind == "deep-parens":
        body = "(" * n + "1" + ")" * n
        return f"func main() {{\n    var a = {body}\n    exit(a)\n}}\n"
    if kind == "deep-brackets":
        return ("func main() {\n    var a = i8" + "[2]" * n
                + "\n    exit(0)\n}\n")
    if kind == "deep-unary":
        op = rnd.choice(["-", "not ", "~"])
        return ("func main() {\n    var a = 1\n    a = " + op * n
                + "a\n    exit(0)\n}\n")
    if kind == "long-chain":
        op = rnd.choice(["+", "*", "-", "&", "|", "and", "<<"])
        return ("func main() {\n    var a = 1\n    a = 1 " +
                f" {op} 1" * n + "\n    exit(0)\n}\n")
    if kind == "long-identifier":
        name = "x" * n
        return f"func main() {{\n    var {name} = 1\n    exit({name})\n}}\n"
    if kind == "many-vars":
        lines = "".join(f"    var v{i} = {i % 100}\n" for i in range(n // 5))
        return f"func main() {{\n{lines}    exit(v0)\n}}\n"
    if kind == "huge-array":
        size = rnd.choice(["1000000000", "4294967296", "9223372036854775807",
                           "100000", "2147483647", "65536"])
        shape = rnd.choice([f"i8[{size}]", f"i32[{size}]", f"i8[{size}][{size}]"])
        return f"func main() {{\n    var a = {shape}\n    exit(0)\n}}\n"
    if kind == "deep-blocks":
        return ("func main() {\n" + "if true {\n" * n + "exit(0)\n" + "}\n" * n
                + "}\n")
    if kind == "many-args":
        params = ", ".join(f"p{i} i32" for i in range(n // 20))
        args = ", ".join("1" for _ in range(n // 20))
        return (f"func f({params}) res i32 {{ res = p0 }}\n"
                f"func main() {{ exit(f({args})) }}\n")
    if kind == "deep-calls":
        funcs = "func f0() res i32 { res = 1 }\n"
        funcs += "".join(f"func f{i}() res i32 {{ res = f{i - 1}() + 1 }}\n"
                         for i in range(1, n // 20))
        return funcs + f"func main() {{ exit(f{n // 20 - 1}()) }}\n"
    if kind == "many-fields":
        fields = ", ".join(f"f{i} i8" for i in range(n // 10))
        return (f"type t {{ {fields} }}\n"
                "func main() { var a = t exit(a.f0) }\n")
    if kind == "long-line":
        return text.replace("\n", " ")
    if kind == "huge-literal":
        digits = "9" * n
        return f"func main() {{\n    var a = {digits}\n    exit(0)\n}}\n"
    if kind == "many-functions":
        funcs = "".join(f"func f{i}() {{ }}\n" for i in range(n // 5))
        return funcs + "func main() { f0() }\n"
    includes = "".join('include "include/base.baz"\n' for _ in range(n // 100))
    return includes + text


def mutate(rnd, seed_text):
    text = rnd.choice(SEEDS) if seed_text is None else seed_text
    for _ in range(rnd.choice([1, 1, 1, 1, 1, 2, 2, 3, 5, 10])):
        text = mutate_tokens(rnd, text)
    return text


def write_source(path, text):
    with open(path, "wb") as f:
        f.write(text.encode("latin-1", "replace"))


def classify_compile(c, source):
    # a finding from the result of the compiler, or None
    if c.timed_out:
        return ("compiler-hang", "timeout")
    text = c.stderr
    if fl.known_crash(text):
        return None
    sanitizer = any(t in text for t in fl.SANITIZER_TEXT)
    if c.returncode in fl.SANITIZER_EXIT or c.returncode < 0 or sanitizer:
        return ("compiler-crash", fl.signature(text) if sanitizer
                else f"exit {c.returncode} {text[:80]}")
    if c.returncode == 1:
        if not fl.DIAGNOSTIC.search(text) and not NO_LOCATION.search(text):
            return ("bad-diagnostic", re.sub(r"\d+", "N", text[:100]))
        return None
    if c.returncode != 0:
        return ("compiler-crash", f"exit code {c.returncode}")
    return None


def first_diagnostic(text):
    match = fl.DIAGNOSTIC.search(text)
    return text[match.start():].split("\n")[0] if match else text.strip()[:100]


# a target limit or a difference of the default integer type is not a finding
EXPECTED_DIFFERENCE = re.compile(
    r"overflows the type|scratch registers|cannot allocate register|reduce expression|does not end at compile time|RV32I|'i64'|i64|out of range|too large|too many|does not fit|"
    r"exceeds|capacity|memory|beyond")
# the diagnostics that have no place in the source
NO_LOCATION = re.compile(r"function 'main' not found")


def checks_options(rnd):
    return rnd.choice([["--checks=noub"], ["--checks=noub"],
                       ["--checks=noub,line"], [],
                       ["--checks=upper,lower,frame"]])


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    text = mutate(rnd, None)
    work = tempfile.mkdtemp(prefix="fuzzc-")
    findings = []
    stats = {"accepted": 0, "rejected": 0, "compared": 0}
    try:
        path = os.path.join(work, "f.baz")
        write_source(path, text)
        options = checks_options(rnd)
        noub = "--checks=noub" in options or "--checks=noub,line" in options
        asan = fl.COMPILER_ASAN

        c, x86, problem = fl.run_x86(asan, path, options, work)
        finding = classify_compile(c, text)
        if finding:
            findings.append(finding + (c.stderr[-3000:],))
            return text, findings, stats
        if c.returncode:
            stats["rejected"] += 1
            # the other target must give a diagnostic too
            c2, _ = fl.run_fpga(asan, path, options, work)
            finding = classify_compile(c2, text)
            if finding:
                findings.append(finding + (c2.stderr[-3000:],))
            elif not c2.returncode:
                message = first_diagnostic(c.stderr).split(": ", 1)[-1]
                if not EXPECTED_DIFFERENCE.search(c.stderr):
                    findings.append(("accept-diff", "rv32i accepts, x86 rejects: "
                                     + re.sub(r"\d+", "N", message[:90]),
                                     c.stderr[-500:]))
            return text, findings, stats
        stats["accepted"] += 1
        if problem and problem[0] == "skip":
            return text, findings, stats
        if problem:
            findings.append((problem[0], problem[1][:80], problem[1][-1500:]))
            return text, findings, stats
        # the code of the accepted source
        with open(os.path.join(work, "f.s"), "rb") as f:
            if not f.read(1):
                findings.append(("empty-output", "x86_64", ""))
        if noub and x86.kind == "signal" and x86.code in [int(s) for s in fl.CRASH_SIGNALS]:
            recursion = "noinline" in text
            findings.append(("ub-signal", f"{x86.detail} recursion={recursion}",
                             str(x86)))

        c2, rv = fl.run_fpga(asan, path, options, work)
        finding = classify_compile(c2, text)
        if finding:
            findings.append(finding + (c2.stderr[-3000:],))
        elif c2.returncode:
            message = first_diagnostic(c2.stderr).split(": ", 1)[-1]
            if not EXPECTED_DIFFERENCE.search(c2.stderr):
                findings.append(("accept-diff", "x86 accepts, rv32i rejects: "
                                 + re.sub(r"[\d']+", "N", message[:90]),
                                 c2.stderr[-500:]))
        else:
            if noub and rv.kind in ("cpu-error", "emulator-signal"):
                findings.append(("cpu-error", rv.detail[:60], str(rv)))
            if noub and x86.finished() and rv.finished():
                stats["compared"] += 1
                same = x86.code == rv.code and (
                    x86.stdout == rv.stdout or FOREIGN_WRITE.search(text))
                # the default integer is 32 bits on rv32i, 64 on x86_64
                width = rv.kind == "panic" and rv.code == 249 and x86.kind == "exit"
                if not same and not stack_difference(x86, rv) and not width:
                    findings.append(("diff-target",
                                     f"{bucket(x86)} vs {bucket(rv)} "
                                     f"output {x86.stdout == rv.stdout}",
                                     f"x86 {x86}\nfpga {rv}"))

        if x86.finished():
            c3, other, p3 = fl.run_x86(asan, path, options + ["--nopt"], work, "n")
            if not c3.returncode and not p3 and other.finished():
                if not x86.same(other):
                    findings.append(("diff-nopt", f"{bucket(x86)} vs {bucket(other)}",
                                     f"opt {x86}\nnopt {other}"))
            elif c3.returncode and not c3.timed_out and c3.signal not in (
                    signal.SIGKILL, signal.SIGXCPU):
                findings.append(("diff-nopt", "--nopt rejects", c3.stderr[-500:]))
            if noub and not (249 <= x86.code <= 255):
                c4, other, p4 = fl.run_x86(asan, path, [], work, "u")
                if not c4.returncode and not p4 and other.finished():
                    if not x86.same(other):
                        findings.append(("diff-checks", f"{bucket(x86)} vs {bucket(other)}",
                                         f"noub {x86}\nnone {other}"))
            if noub and "--checks=noub" in options:
                c5, other, p5 = fl.run_x86(asan, path, ["--checks=noub,line"], work, "l")
                if not c5.returncode and not p5 and other.finished():
                    if not x86.same(other):
                        findings.append(("diff-line", f"{bucket(x86)} vs {bucket(other)}",
                                         f"noub {x86}\nline {other}"))

        r = fl.run([asan, "--reproduce-source"] + options + [path],
                   fl.COMPILE_TIMEOUT, cwd=work, file_size=1 << 30)
        if r.timed_out or r.signal in (signal.SIGKILL, signal.SIGXCPU):
            pass
        elif r.returncode:
            findings.append(("reproduce", f"exit {r.returncode}", r.stderr[-1500:]))
        else:
            diff = os.path.join(work, "diff.baz")
            if not os.path.exists(diff):
                findings.append(("reproduce", "no diff.baz", ""))
            else:
                with open(diff, "rb") as a, open(path, "rb") as b:
                    if a.read() != b.read():
                        findings.append(("reproduce", "different source", ""))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return text, findings, stats


def bucket(outcome):
    # panics keep their code, normal exits are told apart from them only
    return f"panic {outcome.code}" if outcome.kind == "panic" else \
        f"{outcome.kind}"


# the uart of rv32i-fpga takes every file descriptor, a write to another one
# than 1 shows only there
FOREIGN_WRITE = re.compile(r"write\(\s*(?!1\s*,)")


def stack_difference(x86, rv):
    # recursion that is too deep ends differently on the targets
    stack = (250, 254)
    return (x86.kind == "signal" or rv.code in stack or x86.code in stack)


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 500
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(1 << 30)
    jobs = int(sys.argv[3]) if len(sys.argv) > 3 else max(1, (os.cpu_count() or 2) - 2)
    if not os.path.exists(fl.COMPILER_ASAN):
        sys.exit("build the compiler first: build-asan.sh")
    load_seeds()
    findings = fl.Findings()
    print(f"seed {seed}, {count} programs, {jobs} jobs, {len(SEEDS)} seeds")
    totals = {"accepted": 0, "rejected": 0, "compared": 0}
    categories = {}
    new = 0
    begin = time.time()
    with concurrent.futures.ProcessPoolExecutor(jobs, initializer=load_seeds) as pool:
        for n, (text, found, stats) in enumerate(
                pool.map(fuzz_one, [(seed, i) for i in range(count)],
                         chunksize=4), 1):
            for k in totals:
                totals[k] += stats[k]
            for category, key, detail in found:
                categories[category] = categories.get(category, 0) + 1
                if findings.add(category, key, text, detail):
                    new += 1
                    print(f"NEW {category}: {key[:150]}", flush=True)
            if n % 200 == 0:
                print(f"  {n}/{count} {time.time() - begin:.0f}s "
                      f"accepted {totals['accepted']} rejected "
                      f"{totals['rejected']} compared {totals['compared']}",
                      flush=True)
    print(f"{count} programs: {totals['accepted']} accepted, "
          f"{totals['rejected']} rejected, {totals['compared']} compared "
          f"on both targets, {time.time() - begin:.0f}s")
    print("findings by category (all occurrences):",
          dict(sorted(categories.items())))
    print(f"{new} new findings in {fl.FINDINGS}")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
