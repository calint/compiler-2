# helpers of the fuzzers: running the compiler, the assembler, the programs and
# the emulator with limits, and keeping the findings
import hashlib
import os
import re
import resource
import signal
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
COMPILER = os.path.join(ROOT, "baz")
COMPILER_ASAN = os.path.join(HERE, "build", "baz-asan")
EMULATOR = os.path.join(ROOT, "fpga-emulator", "osqa")
# FUZZ_FINDINGS keeps the findings of a run apart from earlier ones
FINDINGS = os.environ.get("FUZZ_FINDINGS", os.path.join(HERE, "findings"))
TESTS = os.path.join(ROOT, "qa", "coverage", "tests")

COMPILE_TIMEOUT = 120
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
# exit codes of the panics of rv32i-fpga and the message of x86_64
PANIC_CODE = {"overflow": 249, "stack overflow": 250, "overlap": 251,
              "shift": 252, "division": 253, "frame overflow": 254,
              "bounds": 255}
CRASH_SIGNALS = (signal.SIGSEGV, signal.SIGFPE, signal.SIGILL, signal.SIGBUS,
                 signal.SIGABRT)


# crashes that are reported and known, the fuzzers skip them to find others
KNOWN_CRASHES = ("assert_folded", "checked_number")


def known_crash(text):
    return any(k in text for k in KNOWN_CRASHES)


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


def compile_source(compiler, path, options, assembly_path=None, cwd=None):
    # the assembler listing is written to 'assembly_path' when given
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
        return self.kind in ("exit", "panic")

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
    if result.returncode == 255:
        for message, code in PANIC_CODE.items():
            if f"panic: {message}" in result.stderr:
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
    if "CPU error" in result.stderr:
        return Outcome("cpu-error", result.returncode, result.stdout,
                       result.stderr.strip())
    if 249 <= result.returncode <= 254:
        return Outcome("panic", result.returncode, result.stdout)
    return Outcome("exit", result.returncode, result.stdout)


def run_fpga(compiler, path, options, work, name="f.bin"):
    # compile for rv32i-fpga and run in the emulator: (compile result, outcome)
    image = os.path.join(work, name)
    c = compile_source(compiler, path,
                       ["--target=rv32i-fpga", "--bin=" + image] + options,
                       os.devnull)
    if c.returncode or c.timed_out:
        return c, None
    r = run([EMULATOR, image, os.devnull], RUN_TIMEOUT)
    return c, outcome_fpga(r)


def run_x86(compiler, path, options, work, name="f"):
    assembly = os.path.join(work, name + ".s")
    c = compile_source(compiler, path, ["--target=x86_64"] + options,
                       assembly)
    if c.returncode or c.timed_out:
        return c, None, None
    exe, problem = assemble_x86(assembly, work)
    if problem:
        return c, None, problem
    r = run([exe], RUN_TIMEOUT)
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
