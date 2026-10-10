#!/usr/bin/env python3
# session fuzzer of the example application 'roome/src/main.baz'
#
# builds roome for x86_64 and rv32i-fpga with '--checks=noub,line', mutates the
# sessions of 'roome/qa/roome.in' (commands, arguments, line editing escapes,
# very long lines, NUL and non-ASCII bytes) and runs them on both:
#
#   roome-panic    a check of 'noub' fired
#   roome-signal   a signal ended the program
#   roome-cpu      the emulator stopped on an illegal instruction or address
#   roome-diff     the two targets wrote different bytes or ended differently
#   roome-timeout  no end in time
#
# usage: fuzz-roome.py [count] [seed] [jobs]
import concurrent.futures
import os
import random
import re
import shutil
import sys
import tempfile
import time

import fuzzlib as fl

ROOME = os.path.join(fl.ROOT, "roome", "src", "main.baz")
SESSION = os.path.join(fl.ROOT, "roome", "qa", "roome.in")
BUILD = os.path.join(fl.HERE, "build")
X86 = os.path.join(BUILD, "roome-x86")
IMAGE = os.path.join(BUILD, "roome-fpga.bin")
SDCARD = os.path.join(BUILD, "sdcard")
NASTY = [b"\x00", b"\x1b[A", b"\x1b[D", b"\x1b[3~", b"\x7f", b"\x08", b"\xff",
         b"\xc3\xa9", b"\t", b"%s", b"\\", b"\"", b"'"]


def build():
    os.makedirs(BUILD, exist_ok=True)
    options = ["--checks=noub,line", "--vars=0x20000"]
    asm = X86 + ".s"
    c = fl.compile_source(fl.COMPILER, ROOME, ["--target=x86_64"] + options, asm)
    if c.returncode:
        sys.exit("roome does not compile: " + c.stderr)
    obj = X86 + ".o"
    if fl.run(["nasm", "-f", "elf64", asm, "-o", obj], 120,
              file_size=1 << 30).returncode:
        sys.exit("nasm failed")
    if fl.run(["ld", "-s", "-T", os.path.join(fl.ROOT, "baz.ld"), obj, "-o", X86],
              60).returncode:
        sys.exit("ld failed")
    c = fl.compile_source(fl.COMPILER, ROOME,
                          ["--target=rv32i-fpga", "--bin=" + IMAGE] + options,
                          os.devnull)
    if c.returncode:
        sys.exit("roome does not compile for rv32i-fpga: " + c.stderr)
    open(SDCARD, "wb").close()


def words_of(lines):
    words = set()
    for line in lines:
        words.update(re.findall(rb"[A-Za-z]+", line))
    return sorted(words)


def mutate_session(rnd, lines, words):
    lines = list(lines)
    for _ in range(rnd.choice([1, 2, 3, 5, 10, 30])):
        kind = rnd.choice(["delete", "duplicate", "swap", "insert-word",
                           "nasty", "long", "flip", "truncate", "empty",
                           "word-swap", "numbers", "repeat"])
        if not lines:
            lines.append(b"")
        i = rnd.randrange(len(lines))
        line = lines[i]
        if kind == "delete":
            del lines[i]
        elif kind == "duplicate":
            lines.insert(i, line)
        elif kind == "swap":
            j = rnd.randrange(len(lines))
            lines[i], lines[j] = lines[j], lines[i]
        elif kind == "insert-word":
            lines.insert(i, rnd.choice(words) + b" " + rnd.choice(words))
        elif kind == "nasty":
            k = rnd.randrange(len(line) + 1)
            lines[i] = line[:k] + rnd.choice(NASTY) + line[k:]
        elif kind == "long":
            lines[i] = line + b" " + rnd.choice(words + [b"x"]) * rnd.choice(
                [10, 100, 1000, 5000])
        elif kind == "flip":
            if line:
                k = rnd.randrange(len(line))
                lines[i] = line[:k] + bytes([rnd.randrange(256)]) + line[k + 1:]
        elif kind == "truncate":
            lines[i] = line[:rnd.randrange(len(line) + 1)]
        elif kind == "empty":
            lines[i] = b""
        elif kind == "word-swap":
            lines[i] = rnd.choice(words) + b" " + line.split(b" ", 1)[-1]
        elif kind == "numbers":
            lines.insert(i, rnd.choice(words) + b" " + rnd.choice(
                [b"0", b"-1", b"255", b"256", b"65536", b"2147483648",
                 b"99999999999999999999"]))
        else:
            lines[i:i] = [line] * rnd.choice([2, 10, 100, 500])
    return b"\n".join(lines) + b"\ngo home\n"


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    with open(SESSION, "rb") as f:
        base = f.read().split(b"\n")
    words = words_of(base)
    # a part of the session, mutated
    start = rnd.randrange(len(base))
    lines = base[start:start + rnd.choice([5, 20, 60, len(base)])]
    if rnd.random() < 0.5:
        lines = base
    # note: ctrl-d is the end of input of the uart, carriage return is read as
    # newline and backspace as delete, so none is mutated into the session and
    # the fpga run gets a ctrl-d at the end instead
    data = mutate_session(rnd, lines, words)
    data = data.replace(b"\x04", b"\x05").replace(b"\r", b"\x05")
    data = data.replace(b"\x08", b"\x05")
    # the emulator needs about 0.2 seconds per megabyte
    data = data[:1 << 20] + b"\ngo home\n"
    found = []
    r86 = fl.run([X86], 10, stdin_bytes=data)
    o86 = fl.outcome_x86(r86)
    rf = fl.run([fl.EMULATOR, IMAGE, SDCARD], 10, stdin_bytes=data + b"\x04")
    ofpga = fl.outcome_fpga(rf)
    text = data.decode("latin-1")
    for target, o in (("x86", o86), ("fpga", ofpga)):
        if o.kind == "panic":
            found.append(("roome-panic", f"{target} {o.code} {o.detail}",
                          r86.stderr[-300:] if target == "x86" else ""))
        elif o.kind in ("signal", "emulator-signal"):
            found.append(("roome-signal", f"{target} {o.detail}", ""))
        elif o.kind == "cpu-error":
            found.append(("roome-cpu", f"{target} {o.detail}"[:60], ""))
        elif o.kind == "timeout":
            found.append(("roome-timeout", target, ""))
    if o86.finished() and ofpga.finished() and not o86.same(ofpga):
        found.append(("roome-diff", f"{o86.kind}/{o86.code} vs "
                      f"{ofpga.kind}/{ofpga.code} output "
                      f"{o86.stdout == ofpga.stdout}",
                      f"x86 {o86}\nfpga {ofpga}"))
    return text, found


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 500
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(1 << 30)
    jobs = int(sys.argv[3]) if len(sys.argv) > 3 else max(1, (os.cpu_count() or 2) - 2)
    build()
    findings = fl.Findings()
    print(f"seed {seed}, {count} sessions, {jobs} jobs")
    categories = {}
    new = 0
    begin = time.time()
    with concurrent.futures.ProcessPoolExecutor(jobs) as pool:
        for n, (text, found) in enumerate(
                pool.map(fuzz_one, [(seed, i) for i in range(count)],
                         chunksize=4), 1):
            for category, key, detail in found:
                categories[category] = categories.get(category, 0) + 1
                if findings.add(category, key, text, detail):
                    new += 1
                    print(f"NEW {category}: {key[:150]}", flush=True)
            if n % 200 == 0:
                print(f"  {n}/{count} {time.time() - begin:.0f}s", flush=True)
    print(f"{count} sessions, {time.time() - begin:.0f}s")
    print("findings by category (all occurrences):", dict(sorted(categories.items())))
    print(f"{new} new findings in {fl.FINDINGS}")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
