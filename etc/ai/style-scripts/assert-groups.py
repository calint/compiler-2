import pathlib
import re
import sys

# a group of consecutive 'assert' statements (single or multiline) is
# separated by a blank line before and after it, except at the top of a
# block (before) and at its end (after, clang-format removes those)
# comment lines directly above a group belong to it
# usage (from the workspace root): python3 assert-groups.py [apply]

apply = len(sys.argv) > 1 and sys.argv[1] == "apply"
assert_re = re.compile(r"\s*assert\(")
counts = {}


def is_block_open(text):
    # a brace initializer 'x{' continues a statement, a block opens with ' {'
    if text.strip() == "{" or text.endswith(" {"):
        return True

    return re.match(r"\s*(case\b.*|default):$", text) is not None


def statement_end(lines, i):
    while not lines[i].rstrip().endswith(";"):
        i += 1

    return i


def report(path, n, kind):
    counts[kind] = counts.get(kind, 0) + 1
    if not apply:
        print(f"{path}:{n + 1}: {kind}")


for path in sorted(pathlib.Path("src").glob("*.[ch]pp")):
    lines = path.read_text().split("\n")
    blank_before = set()
    n = 0
    while n < len(lines):
        if not assert_re.match(lines[n]):
            n += 1
            continue
        start = n
        end = statement_end(lines, n)
        while end + 1 < len(lines) and assert_re.match(lines[end + 1]):
            end = statement_end(lines, end + 1)
        n = end + 1

        first = start
        while first > 0 and lines[first - 1].lstrip().startswith("//"):
            first -= 1
        above = lines[first - 1].rstrip()
        if above != "" and not is_block_open(above):
            report(path, first, "before")
            blank_before.add(first)

        below = lines[end + 1].strip() if end + 1 < len(lines) else ""
        if below != "" and not below.startswith("}"):
            report(path, end + 1, "after")
            blank_before.add(end + 1)

    if apply and blank_before:
        kept = []
        for i, line in enumerate(lines):
            if i in blank_before:
                kept.append("")
            kept.append(line)
        path.write_text("\n".join(kept))

print(counts)
