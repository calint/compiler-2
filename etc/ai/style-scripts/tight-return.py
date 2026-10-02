import pathlib
import re
import sys

# blank line before a single-line 'return':
# - removed when the block holds only one single-line statement before it
# - added when the statement before it spans several lines
# usage (from the workspace root): python3 tight-return.py [apply]

apply = len(sys.argv) > 1 and sys.argv[1] == "apply"
counts = {}


def is_block_open(text):
    # a brace initializer 'x{' continues a statement, a block opens with ' {'
    if text.strip() == "{" or text.endswith(" {"):
        return True

    return re.match(r"\s*(case\b.*|default):$", text) is not None


def is_statement_start(lines, p):
    above = lines[p - 1].rstrip()
    if above == "" or above.lstrip().startswith("//"):
        return True
    if above.endswith((";", "}")):
        return True

    return is_block_open(above)


for path in sorted(pathlib.Path("src").glob("*.[ch]pp")):
    lines = path.read_text().split("\n")
    remove = set()
    add = set()
    for n, line in enumerate(lines):
        if not re.match(r"\s*return\b", line):
            continue
        # a multiline return keeps its separation as a multiline statement
        if not line.rstrip().endswith(";"):
            continue
        p = n - 1
        while p >= 0 and lines[p].strip() == "":
            p -= 1
        prev = lines[p].rstrip()
        if not prev.endswith(";") or prev.lstrip().startswith(("//", "return")):
            continue
        # an assert group keeps its blank line (assert-groups.py)
        if prev.lstrip().startswith("assert("):
            continue
        if not is_statement_start(lines, p):
            if p == n - 1:
                counts["add"] = counts.get("add", 0) + 1
                print(f"{path}:{n + 1}: add")
                add.add(n)
            continue
        if p == n - 1:
            continue
        above = lines[p - 1].rstrip()
        if is_block_open(above):
            kind = "brace"
        elif above == "" and p >= 2 and is_block_open(lines[p - 2].rstrip()):
            # blank after the brace of a multiline signature
            kind = "signature"
        else:
            continue
        counts[kind] = counts.get(kind, 0) + 1
        remove.update(range(p + 1, n))
    if apply and (remove or add):
        kept = []
        for i, l in enumerate(lines):
            if i in remove:
                continue
            if i in add:
                kept.append("")
            kept.append(l)
        path.write_text("\n".join(kept))

print(counts)
