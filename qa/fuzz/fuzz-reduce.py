#!/usr/bin/env python3
# reduces a source that makes the compiler misbehave to a small one
#
# keeps removing lines, then tokens, while the compiler still prints the text
# that is looked for, or ends with the same exit code when no text is given
#
# usage: fuzz-reduce.py FILE.baz [--match=REGEX] [--exit=CODE]
#                       [--options="--target=rv32i-fpga --checks=noub"]
#                       [--accepted-by="--target=x86_64 --checks=noub"]
# '--accepted-by' keeps the source valid: the compiler must also accept it
# with those options
#                       [--compiler=asan|plain] [--out=FILE]
# the reduced source is written to 'FILE.min.baz' (or --out)
import re
import shlex
import sys

import fuzzlib as fl


def parse():
    options = {"match": None, "exit": None, "options": "", "compiler": "asan",
               "out": None, "accepted-by": None}
    files = []
    for arg in sys.argv[1:]:
        if arg.startswith("--") and "=" in arg:
            key, value = arg[2:].split("=", 1)
            options[key] = value
        else:
            files.append(arg)
    if len(files) != 1:
        sys.exit(__doc__)
    return files[0], options


def main():
    path, options = parse()
    compiler = fl.COMPILER_ASAN if options["compiler"] == "asan" else fl.COMPILER
    flags = shlex.split(options["options"])
    if not any(f.startswith("--bin") for f in flags) and \
            any("rv32i" in f for f in flags):
        flags.append("--bin=/dev/null")
    with open(path, "rb") as f:
        text = f.read().decode("latin-1")
    work = path + ".reduce.baz"

    accepted_flags = shlex.split(options["accepted-by"] or "")

    def interesting(candidate):
        with open(work, "wb") as f:
            f.write(candidate.encode("latin-1"))
        if options["accepted-by"] is not None:
            check = fl.run([fl.COMPILER] + accepted_flags + [work],
                           fl.COMPILE_TIMEOUT, file_size=1 << 30)
            if check.returncode != 0:
                return False
        result = fl.run([compiler] + flags + [work], fl.COMPILE_TIMEOUT)
        output = result.stderr
        if options["match"]:
            return re.search(options["match"], output) is not None
        if options["exit"] is not None:
            return result.returncode == int(options["exit"])
        return result.returncode != 0 and "error" not in output

    if not interesting(text):
        sys.exit("the source does not show the behavior")
    # lines, then tokens, in chunks that halve until single
    for unit in ("lines", "tokens"):
        pieces = text.split("\n") if unit == "lines" else \
            re.findall(r"\s+|\w+|.", text, re.S)
        joiner = "\n" if unit == "lines" else ""
        chunk = max(1, len(pieces) // 2)
        while chunk >= 1:
            i = 0
            changed = False
            while i < len(pieces):
                candidate = pieces[:i] + pieces[i + chunk:]
                if interesting(joiner.join(candidate)):
                    pieces = candidate
                    changed = True
                else:
                    i += chunk
            if not changed or chunk == 1:
                if chunk == 1 and changed:
                    continue
                chunk //= 2
        text = joiner.join(pieces)
    out = options["out"] or re.sub(r"\.baz$", "", path) + ".min.baz"
    with open(out, "wb") as f:
        f.write(text.encode("latin-1"))
    print(f"{len(text)} bytes in {out}")


if __name__ == "__main__":
    main()
