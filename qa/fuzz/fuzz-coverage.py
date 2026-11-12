#!/usr/bin/env python3
# which constructs of 'constructs.py' the generators of the fuzzers emit
#
# the generators run without the compiler: the programs they would build are
# collected and searched for the pattern of each construct. a construct that no
# fuzzer emits is reported and ends the run with 1. the fixed seed makes the
# result repeatable
#
# usage: fuzz-coverage.py [programs per fuzzer] [seed] [-v]
import concurrent.futures
import importlib.util
import os
import random
import sys

import fuzzlib as fl
import constructs as cs

HERE = os.path.dirname(os.path.abspath(__file__))
# fuzzers that build programs through 'fuzz_one', 'fuzz-compiler.py' and
# 'fuzz-mutate.py' mutate the tests, 'fuzz-roome.py' the session of roome and
# 'fuzz-limits.py' builds the programs of 'fuzz-programs.py': none is a
# generator
FUZZERS = ("fuzz-exprs", "fuzz-data", "fuzz-flow", "fuzz-bools",
           "fuzz-calls", "fuzz-methods", "fuzz-generics", "fuzz-reject",
           "fuzz-io", "fuzz-programs", "fuzz-whole", "fuzz-versions",
           "fuzz-frames")


def load(name):
    spec = importlib.util.spec_from_file_location(
        name.replace("-", "_"), os.path.join(HERE, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Recorder:
    # replaces the compiler: keeps the text of every file it is asked to build
    def __init__(self):
        self.sources = []

    def compile_source(self, compiler, path, options, assembly_path=None,
                       cwd=None):
        try:
            with open(path, "rb") as f:
                text = f.read().decode("latin-1")
            # the definitions of 'fuzzlib.split_include', only when the file
            # includes them: a part left by an earlier program is not its own
            if text.startswith(fl.PART_INCLUDE):
                part = os.path.join(os.path.dirname(path), "lib", "part.baz")
                with open(part, "rb") as f:
                    text = f.read().decode("latin-1") + text
            self.sources.append(text)
        except OSError:
            pass
        return fl.Result(1, b"", "recorded", False)


def collect_fuzz_one(name, count, seed, recorder):
    module = load(name)
    for index in range(count):
        # 'fuzz-programs.py' also takes the compiler to use
        args = (seed, index, "asan") if name == "fuzz-programs" \
            else (seed, index)
        # an error of a generator ends the run
        source = module.fuzz_one(args)[0]
        if isinstance(source, str):
            recorder.sources.append(source)


def collect_ub(count, seed, recorder):
    module = load("fuzz-ub")
    for n in range(count):
        rnd = random.Random(seed * 100003 + n)
        program = module.SCENARIOS[n % len(module.SCENARIOS)](rnd)
        # the outcomes of the model, as in 'fuzz-ub.py'
        try:
            program.evaluate()
        except (module.Panic, module.Rejected, module.Skip):
            pass
        recorder.sources.append(program.source())


def collect(job):
    name, count, seed = job
    recorder = Recorder()
    fl.compile_source = recorder.compile_source
    if name == "fuzz-ub":
        collect_ub(count, seed, recorder)
    else:
        collect_fuzz_one(name, count, seed, recorder)
    return name, recorder.sources


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    verbose = "-v" in sys.argv
    count = int(args[0]) if args else 100
    seed = int(args[1]) if len(args) > 1 else 1
    names = FUZZERS + ("fuzz-ub",)
    corpora = {}
    with concurrent.futures.ProcessPoolExecutor(len(names)) as pool:
        for name, sources in pool.map(collect, [(n, count, seed)
                                                for n in names]):
            corpora[name] = sources
            print(f"{name}: {len(sources)} programs")
    emitted = {}
    for construct in cs.CONSTRUCTS:
        if not construct.gate:
            continue
        emitted[construct.name] = [
            name for name, sources in corpora.items()
            if any(construct.found_in(s) for s in sources)]
    missing = [name for name, fuzzers in emitted.items() if not fuzzers]
    for name, fuzzers in emitted.items():
        if verbose or not fuzzers:
            print(f"{name:34} {', '.join(fuzzers) or 'NOT EMITTED'}")
    print(f"{len(emitted) - len(missing)}/{len(emitted)} constructs emitted")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
