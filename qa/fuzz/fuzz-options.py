#!/usr/bin/env python3
# fuzzer of the command line of the compiler
#
# combines options with valid, malformed and extreme values ('--target',
# '--vars', '--stack', '--memory', '--checks', '--report', '--bin', '--nopt',
# '--reproduce-source', unknown options, missing and odd files) and compiles
# small programs with the sanitizer build:
#
#   option-crash      sanitizer report, abort, signal or an unexpected exit code
#   option-hang       no result in time
#   option-message    a rejection without a message
#   option-wrong      an accepted combination whose program does not end with
#                     the expected exit code
#   option-cpu        an accepted rv32i-fpga image stops the emulator
#
# usage: fuzz-options.py [count] [seed] [jobs]
import concurrent.futures
import os
import random
import shutil
import sys
import tempfile
import time

import fuzzlib as fl

PROGRAMS = {
    "exit": ("func main() { exit(7) }\n", 7),
    "recursion": ("""func noinline sum(n i32) res i32 {
    res = 0
    if n == 0 return
    var r = sum(n - 1)
    res = r + n
}
func main() {
    var s = sum(50)
    exit(s & 127)
}
""", 1275 & 127),
    "array": ("""dat table = i32[]{1, 2, 3, 4, 5, 6, 7, 8}
func main() {
    var a = i32[8]
    array_copy(table, a, 8)
    var total = i32(0)
    foo a {
        total = total + e
    }
    var idx = i32(3)
    exit(total + a[idx])
}
""", 36 + 4),
}
TARGETS = ["x86_64", "rv32i", "rv32i-qemu", "rv32i-fpga", "", "bogus", "X86_64",
           "x86_64 ", "rv32"]
SIZES = ["0", "16", "4096", "65536", "0x10000", "0x1000", "0x20000", "1048576",
         "0x100000", "8388608", "0x800000", "17", "15", "-16", "0x", "abc",
         "1e3", "", "99999999999999999999", "0xffffffff", "4294967296",
         "0x7fffffffffffffff", "2147483648", "1_000", " 16", "16 ", "+16",
         "0b101", "0o20", "4095", "4097", "0xfffffff0"]
CHECK_NAMES = ["upper", "lower", "line", "frame", "alias", "division", "shift",
               "overlap", "stack", "overflow", "noub"]
REPORTS = ["registers", "bogus", "", "registers,registers", "Registers"]
FLAGS = ["--nopt", "--reproduce-source", "--help", "-h", "--unknown",
         "-x", "--", "-", "--target", "--checks", "--vars", "--bin",
         "--nopt=1", "--checks=", "--report=", "--stack=", "--memory="]


def checks_value(rnd):
    parts = []
    for _ in range(rnd.choice([0, 1, 2, 3, 5, 12])):
        name = rnd.choice(CHECK_NAMES + ["bogus", "", "NOUB", "noub,noub",
                                         "upper lower"])
        prefix = rnd.choice(["", "", "", "-", "+", "--", "!"])
        parts.append(prefix + name)
    return ",".join(parts) + rnd.choice(["", "", ",", ",,"])


VALID_SIZES = ["16", "4096", "65536", "0x10000", "0x20000", "0x40000",
               "1048576", "0x100000", "0x1000"]
VALID_MEMORY = ["4096", "65536", "0x100000", "0x200000", "8388608",
                "0x800000"]


def make_valid_options(rnd):
    # every value is well formed, the combination may still not fit
    options = ["--target=" + rnd.choice(TARGETS[:4])]
    if rnd.random() < 0.5:
        options.append("--vars=" + rnd.choice(VALID_SIZES))
    if rnd.random() < 0.4:
        options.append("--stack=" + rnd.choice(VALID_SIZES))
    if rnd.random() < 0.4:
        options.append("--memory=" + rnd.choice(VALID_MEMORY))
    if rnd.random() < 0.9:
        names = []
        for _ in range(rnd.randint(0, 6)):
            names.append(rnd.choice(["", "", "-", "+"]) + rnd.choice(CHECK_NAMES))
        options.append("--checks=" + ",".join(names))
    if rnd.random() < 0.15:
        options.append("--report=registers")
    if rnd.random() < 0.15:
        options.append("--nopt")
    return options


def make_options(rnd):
    if rnd.random() < 0.6:
        return make_valid_options(rnd)
    options = []
    if rnd.random() < 0.85:
        options.append("--target=" + (rnd.choice(TARGETS) if rnd.random() < 0.15
                                      else rnd.choice(TARGETS[:4])))
    for name in ("--vars", "--stack", "--memory"):
        if rnd.random() < 0.4:
            options.append(f"{name}={rnd.choice(SIZES)}")
    for _ in range(rnd.choice([0, 1, 1, 2])):
        options.append("--checks=" + checks_value(rnd))
    if rnd.random() < 0.2:
        options.append("--report=" + rnd.choice(REPORTS))
    for _ in range(rnd.choice([0, 0, 0, 1, 2])):
        options.append(rnd.choice(FLAGS))
    rnd.shuffle(options)
    return options


def fuzz_one(args):
    seed, index = args
    rnd = random.Random(seed * 1000003 + index)
    work = tempfile.mkdtemp(prefix="fuzzo-")
    found = []
    try:
        name = rnd.choice(list(PROGRAMS))
        source, expected = PROGRAMS[name]
        path = os.path.join(work, "f.baz")
        with open(path, "w") as f:
            f.write(source)
        options = make_options(rnd)
        file_arguments = rnd.choice([[path], [path], [path], [], [path, path],
                                     [work], [os.path.join(work, "missing.baz")]])
        description = " ".join(options) + " " + name
        target = next((o.split("=", 1)[1] for o in options
                       if o.startswith("--target=")), "x86_64")
        fpga = target == "rv32i-fpga"
        image = os.path.join(work, "image.bin")
        if "rv32i" in target and not any(o.startswith("--bin") for o in options):
            options.append("--bin=" + image)
        assembly = os.path.join(work, "f.s")
        c = fl.compile_source(fl.COMPILER_ASAN, None, options + file_arguments,
                              assembly, cwd=work) if False else None
        command = [fl.COMPILER_ASAN] + options + file_arguments
        result = fl.run(command, fl.COMPILE_TIMEOUT, cwd=work, file_size=1 << 30)
        sanitizer = any(t in result.stderr for t in fl.SANITIZER_TEXT)
        if result.timed_out:
            found.append(("option-hang", description[:100], result.stderr[-500:]))
        elif result.returncode in fl.SANITIZER_EXIT or result.returncode < 0 \
                or sanitizer or result.returncode not in (0, 1):
            found.append(("option-crash", fl.signature(result.stderr) if sanitizer
                          else f"exit {result.returncode}", description
                          + "\n" + result.stderr[-2500:]))
        elif result.returncode == 1 and not result.stderr.strip():
            found.append(("option-message", description[:100], ""))
        elif result.returncode == 0 and file_arguments == [path] and \
                not any(o in ("--help", "-h", "--reproduce-source") for o in options):
            run_accepted(work, path, options, target, fpga, expected, name,
                         description, found, result, image)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return description + "\n" + source, found


def run_accepted(work, path, options, target, fpga, expected, name, description,
                 found, result, image):
    checks = ",".join(o for o in options if o.startswith("--checks="))
    if target == "x86_64":
        assembly = os.path.join(work, "f.s")
        with open(assembly, "wb") as f:
            f.write(result.stdout)
        # the listing went to stdout of the run above, compile again to a file
        c = fl.compile_source(fl.COMPILER_ASAN, path, options, assembly, cwd=work)
        exe, problem = fl.assemble_x86(assembly, work)
        if problem and problem[0] != "skip":
            found.append((problem[0], description[:100], problem[1][-800:]))
        if exe:
            o = fl.outcome_x86(fl.run([exe], fl.RUN_TIMEOUT))
            judge(o, expected, name, options, description, found)
    elif fpga and os.path.exists(image):
        r = fl.run([fl.EMULATOR, image, os.devnull], fl.RUN_TIMEOUT)
        o = fl.outcome_fpga(r)
        if o.kind == "cpu-error":
            found.append(("option-cpu", description[:100], str(o)))
        else:
            judge(o, expected, name, options, description, found)


def small_memory(options):
    # the frame and stack checks may fire when the sizes are small
    for o in options:
        if o.split("=")[0] in ("--vars", "--stack", "--memory"):
            return True
    return False


def judge(outcome, expected, name, options, description, found):
    if not outcome.finished():
        return
    if outcome.kind == "exit" and outcome.code == expected:
        return
    if outcome.kind == "panic" and outcome.code in (250, 254) and \
            small_memory(options):
        return
    found.append(("option-wrong", f"{name} {outcome.kind} {outcome.code}"
                  f" {[o for o in options if o.startswith('--target')]}",
                  description + "\n" + str(outcome)))


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(1 << 30)
    jobs = int(sys.argv[3]) if len(sys.argv) > 3 else max(1, (os.cpu_count() or 2) - 2)
    if not os.path.exists(fl.COMPILER_ASAN):
        sys.exit("build the compiler first: build-asan.sh")
    findings = fl.Findings()
    print(f"seed {seed}, {count} runs, {jobs} jobs")
    categories = {}
    new = 0
    begin = time.time()
    with concurrent.futures.ProcessPoolExecutor(jobs) as pool:
        for n, (text, found) in enumerate(
                pool.map(fuzz_one, [(seed, i) for i in range(count)],
                         chunksize=8), 1):
            for category, key, detail in found:
                categories[category] = categories.get(category, 0) + 1
                if findings.add(category, key, text, detail):
                    new += 1
                    print(f"NEW {category}: {key[:150]}", flush=True)
            if n % 500 == 0:
                print(f"  {n}/{count} {time.time() - begin:.0f}s", flush=True)
    print(f"{count} runs, {time.time() - begin:.0f}s")
    print("findings by category (all occurrences):", dict(sorted(categories.items())))
    print(f"{new} new findings in {fl.FINDINGS}")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
