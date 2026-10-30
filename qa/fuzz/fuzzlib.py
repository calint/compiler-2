# helpers of the fuzzers: running the compiler, the assembler, the programs and
# the emulator with limits, and keeping the findings
import hashlib
import os
import re
import resource
import signal
import subprocess
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
COMPILER = os.path.join(ROOT, "baz")
COMPILER_ASAN = os.path.join(HERE, "build", "baz-asan")
EMULATOR = os.path.join(ROOT, "fpga-emulator", "osqa")
# FUZZ_FINDINGS keeps the findings of a run apart from earlier ones
FINDINGS = os.environ.get("FUZZ_FINDINGS", os.path.join(HERE, "findings"))
TESTS = os.path.join(ROOT, "qa", "coverage", "tests")

COMPILE_TIMEOUT = 120
# a compile that takes longer is a finding, 'compiler-slow'
SLOW_COMPILE = 60
RUN_TIMEOUT = 5
OUTPUT_LIMIT = 1 << 20
MEMORY_LIMIT = 4 << 30

# a sanitizer report ends the compiler with these exit codes
SANITIZER_EXIT = {98, 99}
ENVIRONMENT = dict(
    os.environ,
    ASAN_OPTIONS="exitcode=99:detect_leaks=0:abort_on_error=0",
    UBSAN_OPTIONS="halt_on_error=1:exitcode=98:print_stacktrace=1",
)
# the exit code of each panic, the same on every target; a panic prints a
# newline and 'panic: NAME', on x86_64 on standard error, on rv32i-fpga on the
# uart after the output of the program, with ' at FILE:LINE' under
# '--checks=line'
PANIC_CODE = {"overflow": 249, "stack overflow": 250, "overlap": 251,
              "shift": 252, "division": 253, "frame overflow": 254,
              "bounds": 255}
# the exit code of 'panic: sd card', the outcome 'sdcard'
SDCARD_EXIT = 248
# the last line of the emulator when it stops at an illegal instruction or
# address, on its standard output after the program's; it exits with the status
# modulo 256
CPU_ERROR = re.compile(rb"CPU error: (\d+)\n\Z")
# the last line of a panic on rv32i-fpga, see 'PANIC_CODE'
FPGA_PANIC = re.compile(rb"\npanic: ([a-z ]+?)(?: at [^\n]*)?\n\Z")
# the emulator stops at the store of a sector number past the end of the image,
# 13xx is a 'sw' and 4 the write and 5 the read sector address; x86_64 prints
# 'panic: sd card' instead, both are the outcome 'sdcard'
SDCARD_PAST_END_ERRORS = (1304, 1305)
# the model of the signed integers of the language, shared by the fuzzers
# that compute what a program prints: the width of each type, its range, the
# value an operation wraps to, the quotient that truncates toward zero, and a
# hash of the values a program prints
BITS = {"i8": 8, "i16": 16, "i32": 32, "i64": 64}


def signed_limits(bits):
    return -(1 << (bits - 1)), (1 << (bits - 1)) - 1


def limits(t):
    return signed_limits(BITS[t])


def fits(t, value):
    low, high = limits(t)
    return low <= value <= high


def wrap(t, value):
    bits = BITS[t]
    value &= (1 << bits) - 1
    return value - (1 << bits) if value >> (bits - 1) else value


def trunc_div(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def mix(acc, value):
    return ((acc ^ (value & 0xffff)) * 31) & 0xffff


# the code of the outcome 'sdcard', no exit code of a program
SDCARD_CODE = -1
CRASH_SIGNALS = (signal.SIGSEGV, signal.SIGFPE, signal.SIGILL, signal.SIGBUS,
                 signal.SIGABRT)


def group(text):
    # 'text' as an operand: parentheses only around an operation, a single
    # value in parentheses is rejected, e.g. '(x)'; a value with a unary
    # operator keeps them, e.g. '(-3)'
    depth = 0
    for ch in text:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == " " and depth == 0:
            return f"({text})"
    if text[:1] in "-~" or text.startswith("not "):
        return f"({text})"
    return text


class Result:
    def __init__(self, returncode, stdout, stderr, timed_out=False):
        self.returncode = returncode
        self.stdout = stdout
        self.stderr = stderr
        self.timed_out = timed_out

    @property
    def signal(self):
        return -self.returncode if self.returncode < 0 else 0


def _limits(seconds, memory, file_size=OUTPUT_LIMIT):
    def apply():
        resource.setrlimit(resource.RLIMIT_CPU, (seconds, seconds))
        resource.setrlimit(resource.RLIMIT_FSIZE, (file_size, file_size))
        if memory:
            resource.setrlimit(resource.RLIMIT_AS, (memory, memory))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    return apply


def run(command, timeout, cwd=None, memory=0, env=None,
        file_size=OUTPUT_LIMIT, stdin_bytes=None):
    # output goes to files so that a program that prints without end cannot
    # fill the memory, the size of a file is limited
    with tempfile.TemporaryFile() as out, tempfile.TemporaryFile() as err, \
            tempfile.TemporaryFile() as given:
        if stdin_bytes is not None:
            given.write(stdin_bytes)
            given.seek(0)
        process = subprocess.Popen(
            command, stdin=given if stdin_bytes is not None
            else subprocess.DEVNULL, stdout=out, stderr=err,
            cwd=cwd, env=env or ENVIRONMENT,
            preexec_fn=_limits(timeout + 2, memory, file_size))
        timed_out = False
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            timed_out = True
        out.seek(0)
        err.seek(0)
        return Result(process.returncode,
                      out.read(OUTPUT_LIMIT),
                      err.read(OUTPUT_LIMIT).decode("utf-8", "replace"),
                      timed_out)


# the compilers found as new as the sources, see 'assert_fresh'
_fresh = set()


def assert_fresh(compiler):
    # a compiler older than a file of 'src/' would test the code before the
    # change, e.g. 'build/baz-asan' not built again by 'build-asan.sh'
    if compiler in _fresh:
        return
    if not os.path.exists(compiler):
        raise SystemExit(f"{compiler} is missing, build it ('make.sh' or "
                         "'build-asan.sh')")
    src = os.path.join(ROOT, "src")
    newest = max(os.path.getmtime(os.path.join(src, name))
                 for name in os.listdir(src))
    if os.path.getmtime(compiler) < newest:
        raise SystemExit(f"{compiler} is older than 'src/', build it again "
                         "('make.sh' or 'build-asan.sh')")
    _fresh.add(compiler)


def compile_source(compiler, path, options, assembly_path=None, cwd=None):
    # the assembler listing is written to 'assembly_path' when given
    assert_fresh(compiler)
    begin = time.time()
    result = _compile_source(compiler, path, options, assembly_path, cwd)
    seconds = time.time() - begin
    if seconds > SLOW_COMPILE and not result.timed_out:
        note_slow(compiler, path, options, seconds)
    return result


def note_if_slow(begin, compiler, options, path):
    seconds = time.time() - begin
    if seconds > SLOW_COMPILE:
        note_slow(compiler, path, options, seconds)


def run_compiler(compiler, options, path, cwd=None):
    # runs the compiler on 'path' with all the limits for a compile and notes
    # a slow one
    assert_fresh(compiler)
    begin = time.time()
    result = run([compiler] + options + [path], COMPILE_TIMEOUT, cwd=cwd,
                 file_size=1 << 30)
    if not result.timed_out:
        note_if_slow(begin, compiler, options, path)
    return result


def note_slow(compiler, path, options, seconds):
    # keeps the program of a compile that took more than 'SLOW_COMPILE'
    # seconds, one finding for each program and option list
    try:
        with open(path, "rb") as f:
            source = joined_include(path, f.read().decode("latin-1"))
    except OSError:
        return
    key = f"{' '.join(options)} {hashlib.sha1(source.encode()).hexdigest()}"
    detail = f"{os.path.basename(compiler)} {' '.join(options)} took " \
        f"{seconds:.0f}s"
    if Findings().add("compiler-slow", key, source, detail):
        print(f"NEW compiler-slow: {detail}", flush=True)


def _compile_source(compiler, path, options, assembly_path, cwd):
    command = [compiler] + options + [path]
    if assembly_path is None:
        return run(command, COMPILE_TIMEOUT, cwd=cwd)
    with open(assembly_path, "wb") as out:
        with tempfile.TemporaryFile() as err:
            process = subprocess.Popen(
                command, stdin=subprocess.DEVNULL, stdout=out, stderr=err,
                cwd=cwd, env=ENVIRONMENT,
                preexec_fn=_limits(COMPILE_TIMEOUT + 5, 0, 1 << 30))
            timed_out = False
            try:
                process.wait(timeout=COMPILE_TIMEOUT)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                timed_out = True
            err.seek(0)
            return Result(process.returncode, b"",
                          err.read(OUTPUT_LIMIT).decode("utf-8", "replace"),
                          timed_out)


def assemble_x86(assembly, work):
    obj = os.path.join(work, "f.o")
    exe = os.path.join(work, "f.x")
    a = run(["nasm", "-f", "elf64", assembly, "-o", obj], 60,
            file_size=1 << 30)
    # a tool that is stopped by a limit says nothing about the code
    if a.timed_out or a.signal:
        return None, ("skip", "")
    if a.returncode:
        return None, ("invalid-asm", a.stderr)
    link = run(["ld", "-s", "-T", os.path.join(ROOT, "baz.ld"), obj, "-o", exe], 60,
               file_size=1 << 30)
    if link.timed_out or link.signal:
        return None, ("skip", "")
    if link.returncode:
        return None, ("link-failure", link.stderr)
    return exe, None


class Outcome:
    # what a program did: the normalized exit code (a panic as the code of
    # rv32i-fpga), the bytes it wrote and how it ended
    def __init__(self, kind, code, stdout, detail=""):
        self.kind = kind
        self.code = code
        self.stdout = stdout
        self.detail = detail

    def same(self, other):
        return self.code == other.code and self.stdout == other.stdout

    def finished(self):
        return self.kind in ("exit", "panic", "sdcard")

    def __repr__(self):
        return f"{self.kind} {self.code} {len(self.stdout)}B {self.detail}"


def outcome_x86(result):
    if result.timed_out:
        return Outcome("timeout", None, b"")
    if result.signal == signal.SIGXFSZ:
        return Outcome("output-limit", None, b"")
    if result.signal in (signal.SIGKILL, signal.SIGXCPU):
        return Outcome("timeout", None, b"")
    if result.signal:
        return Outcome("signal", result.signal, result.stdout,
                       signal.Signals(result.signal).name)
    if result.returncode == SDCARD_EXIT and "panic: sd card" in result.stderr:
        return Outcome("sdcard", SDCARD_CODE, result.stdout, "sd card")
    for message, code in PANIC_CODE.items():
        if result.returncode == code and f"panic: {message}" in result.stderr:
            return Outcome("panic", code, result.stdout, message)
    return Outcome("exit", result.returncode, result.stdout)


def outcome_fpga(result):
    if result.timed_out:
        return Outcome("timeout", None, b"")
    if result.signal == signal.SIGXFSZ:
        return Outcome("output-limit", None, b"")
    if result.signal in (signal.SIGKILL, signal.SIGXCPU):
        return Outcome("timeout", None, b"")
    if result.signal:
        return Outcome("emulator-signal", result.signal, result.stdout,
                       signal.Signals(result.signal).name)
    cpu_error = CPU_ERROR.search(result.stdout)
    if cpu_error and int(cpu_error.group(1)) in SDCARD_PAST_END_ERRORS:
        return Outcome("sdcard", SDCARD_CODE, result.stdout[:cpu_error.start()],
                       "sd card")
    if cpu_error:
        return Outcome("cpu-error", int(cpu_error.group(1)),
                       result.stdout[:cpu_error.start()],
                       cpu_error.group(0).decode().strip())
    panic = FPGA_PANIC.search(result.stdout)
    if panic and PANIC_CODE.get(panic.group(1).decode()) == result.returncode:
        return Outcome("panic", result.returncode,
                       result.stdout[:panic.start()], panic.group(1).decode())
    return Outcome("exit", result.returncode, result.stdout)


def run_fpga(compiler, path, options, work, name="f.bin", stdin=b""):
    # compile for rv32i-fpga and run in the emulator: (compile result, outcome)
    # the uart takes the input followed by end of transmission bytes, each read at
    # the end of the input takes one
    image = os.path.join(work, name)
    c = compile_source(compiler, path,
                       ["--target=rv32i-fpga", "--bin=" + image] + options,
                       os.devnull)
    if c.returncode or c.timed_out:
        return c, None
    r = run([EMULATOR, image, os.devnull], RUN_TIMEOUT,
            stdin_bytes=stdin + b"\x04" * 64)
    return c, outcome_fpga(r)


def run_x86(compiler, path, options, work, name="f", stdin=b""):
    assembly = os.path.join(work, name + ".s")
    c = compile_source(compiler, path, ["--target=x86_64"] + options,
                       assembly)
    if c.returncode or c.timed_out:
        return c, None, None
    exe, problem = assemble_x86(assembly, work)
    if problem:
        return c, None, problem
    r = run([exe], RUN_TIMEOUT, stdin_bytes=stdin)
    return c, outcome_x86(r), None


# diagnostics are 'file:line:column: message'
DIAGNOSTIC = re.compile(r"^.+:\d+:\d+: \S", re.M)
SANITIZER_TEXT = ("AddressSanitizer", "runtime error:", "LeakSanitizer",
                  "terminate called", "Assertion", "stack-overflow",
                  "SUMMARY:", "libc++abi", "std::bad_alloc")


def signature(text):
    # a short stable key of a crash: the sanitizer's reason and the first
    # frames in the compiler
    reason = re.search(r"(ERROR: AddressSanitizer: [\w-]+|runtime error: "
                       r"[^\n]*?(?= [a-z]*$|$)|Assertion [^\n]*|terminate "
                       r"called[^\n]*)", text)
    frames = re.findall(r"#\d+ 0x[0-9a-f]+ in ([^\n(]+?) [^\n]*?/src/([\w.]+:\d+)",
                        text)
    parts = [reason.group(1) if reason else "crash"]
    parts += [f"{func.split('(')[0][:60]}@{where}" for func, where in frames[:3]]
    # the runtime errors name the place in the first line
    place = re.search(r"/src/([\w.]+:\d+):\d+: runtime error", text)
    if place:
        parts.append(place.group(1))
    return " | ".join(parts)


class Findings:
    # keeps the first example of each finding in 'findings/'
    def __init__(self):
        os.makedirs(FINDINGS, exist_ok=True)
        self.seen = set()
        for name in os.listdir(FINDINGS):
            self.seen.add(name.rsplit(".", 1)[0])

    def add(self, category, key, source, detail):
        digest = hashlib.sha1(f"{category}|{key}".encode()).hexdigest()[:10]
        name = f"{category}-{digest}"
        if name in self.seen:
            return False
        self.seen.add(name)
        with open(os.path.join(FINDINGS, name + ".baz"), "wb") as f:
            f.write(source.encode("latin-1", "replace"))
        with open(os.path.join(FINDINGS, name + ".txt"), "w") as f:
            f.write(f"{category}\n{key}\n\n{detail}\n")
        return True


# the targets and the option lists that the model fuzzers build every program
# with
TARGETS = ("x86_64", "rv32i-fpga")
OPTION_LISTS = (("--checks=noub",), ("--checks=noub", "--nopt"),
                ("--checks=noub,line",))


# the first line of a program that 'split_include' split
PART_INCLUDE = 'include "lib/part.baz"\n'


def split_include(work, source):
    # about every third program has its definitions in an included file in a
    # directory, 'main' stays in the file that is built, the program is the
    # same: returns the text of the file to build
    marker = "\nfunc main()"
    at = source.find(marker)
    if at < 0 or int(hashlib.sha1(source.encode("latin-1", "replace"))
                     .hexdigest(), 16) % 3:
        return source
    os.makedirs(os.path.join(work, "lib"), exist_ok=True)
    with open(os.path.join(work, "lib", "part.baz"), "wb") as f:
        f.write(source[:at + 1].encode("latin-1", "replace"))
    return PART_INCLUDE + source[at + 1:]


def joined_include(path, source):
    # the whole program of a file that 'split_include' wrote, so that a
    # finding compiles without its directory
    if not source.startswith(PART_INCLUDE):
        return source
    with open(os.path.join(os.path.dirname(path), "lib", "part.baz"),
              "rb") as f:
        part = f.read().decode("latin-1")
    return part + source[len(PART_INCLUDE):]


def build_and_run(work, source, target, options, stdin=b"",
                  compiler=COMPILER_ASAN):
    # what a program did: ('compile', stderr), ('exit', code, stdout) or
    # (kind, detail) when it did not end by itself or the assembler failed
    path = os.path.join(work, "f.baz")
    with open(path, "w") as f:
        f.write(split_include(work, source))
    options = list(options)
    if target == "x86_64":
        c, o, problem = run_x86(compiler, path, options, work, stdin=stdin)
    else:
        c, o = run_fpga(compiler, path, options, work, stdin=stdin)
        problem = None
    if c.timed_out:
        return ("compile-hang", "")
    if c.returncode:
        return ("compile", c.stderr[:600])
    if problem:
        return (problem[0], problem[1][:300])
    if o.kind != "exit":
        return (o.kind, f"{o.code} {o.detail}")
    return ("exit", o.code, o.stdout)


def judge(work, source, expected, stdin=b"", targets=TARGETS,
          option_lists=OPTION_LISTS, prefix="model"):
    # builds the program for every target and option list, returns the
    # findings (category, key, detail) of the builds that did not end with
    # the 'expected' ('exit', code, stdout)
    found = []
    for target in targets:
        for options in option_lists:
            got = build_and_run(work, source, target, options, stdin)
            if got == expected:
                continue
            key = f"{target} {' '.join(options)} {got[0]}"
            if got[0] == "compile":
                category = f"{prefix}-reject"
                message = got[1].strip().splitlines()[0] if got[1] else ""
                key = f"{target} {message.split(': ', 1)[-1]}"
            elif got[0] == "compile-hang":
                category = "compiler-hang"
            elif got[0] in ("invalid-asm", "link-failure"):
                category = got[0]
            else:
                category = f"{prefix}-mismatch"
            found.append((category, key, f"expected {expected}, got {got}"))
    return found


def run_fuzzer(fuzz_one, count, seed, jobs, label="programs", heading="",
               task_args=(), initializer=None, chunksize=2):
    # runs 'fuzz_one((seed, index, *task_args))' in processes, each returns
    # (source, findings) or (source, findings, counts), 'counts' a dict summed
    # over the run or one key counted once, e.g. "exit"; prints the first
    # example of each finding and the counts, returns 1 when a finding is new
    import concurrent.futures
    import time
    findings = Findings()
    print(f"seed {seed}, {count} {label}, {jobs} jobs{heading}")
    new = 0
    counts = {}
    categories = {}
    begin = time.time()
    tasks = [(seed, i, *task_args) for i in range(count)]
    with concurrent.futures.ProcessPoolExecutor(
            jobs, initializer=initializer) as pool:
        for n, result in enumerate(
                pool.map(fuzz_one, tasks, chunksize=chunksize), 1):
            source, found = result[0], result[1]
            extra = result[2] if len(result) > 2 else {}
            for key, value in ({extra: 1} if isinstance(extra, str)
                               else extra).items():
                counts[key] = counts.get(key, 0) + value
            for category, key, detail in found:
                categories[category] = categories.get(category, 0) + 1
                if findings.add(category, key, source, detail):
                    new += 1
                    print(f"NEW {category}: {key[:150]}\n    {detail[:300]}",
                          flush=True)
            if n % 200 == 0:
                print(f"  {n}/{count} {time.time() - begin:.0f}s {counts}",
                      flush=True)
    total = sum(categories.values())
    print(f"{count} {label}, {total} mismatches, {counts}, "
          f"{time.time() - begin:.0f}s")
    print("findings by category (all occurrences):",
          dict(sorted(categories.items())))
    print(f"{new} new findings in {FINDINGS}")
    return 1 if new else 0


def arguments(default_count):
    # count, seed and jobs of the command line
    import random
    import sys
    count = int(sys.argv[1]) if len(sys.argv) > 1 else default_count
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(1 << 30)
    jobs = int(sys.argv[3]) if len(sys.argv) > 3 else \
        max(1, (os.cpu_count() or 2) - 2)
    return count, seed, jobs
