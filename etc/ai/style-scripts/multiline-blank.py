import pathlib
import sys

import clang.cindex as ci

# a statement that spans several lines has a blank line before and after,
# except next to the braces of its block, and a comment above it stays with it
# control statements are not checked, only declarations and expressions
# usage (from the workspace root): python3 multiline-blank.py [apply]

sys.path.insert(0, "qa/lint")
from libclang_tu import is_in, parse  # noqa: E402

K = ci.CursorKind
apply = len(sys.argv) > 1 and sys.argv[1] == "apply"
root = pathlib.Path("src").resolve()

CONTROL = {
    K.IF_STMT,
    K.FOR_STMT,
    K.CXX_FOR_RANGE_STMT,
    K.WHILE_STMT,
    K.DO_STMT,
    K.SWITCH_STMT,
    K.COMPOUND_STMT,
    K.CXX_TRY_STMT,
    K.CASE_STMT,
    K.DEFAULT_STMT,
    K.NULL_STMT,
    K.LABEL_STMT,
}

files = {}


def lines_of(name):
    if name not in files:
        files[name] = pathlib.Path(name).read_text().split("\n")

    return files[name]


def is_comment(text):
    return text.strip().startswith("//")


def is_label(text):
    stripped = text.strip()

    return stripped.startswith(("case ", "default:")) and stripped.endswith(":")


def needs_blank_before(lines, first):
    index = first
    while index > 0 and is_comment(lines[index - 1]):
        index -= 1

    if index == 0:
        return None

    above = lines[index - 1]
    if above.strip() == "" or above.endswith("{") or is_label(above):
        return None

    return index


def needs_blank_after(lines, last):
    index = last + 1
    if index >= len(lines):
        return None

    nxt = lines[index]
    if nxt.strip() == "" or nxt.strip().startswith("}") or is_label(nxt):
        return None

    # comments below a statement stay with it when a blank follows them
    probe = index
    while probe < len(lines) and is_comment(lines[probe]):
        probe += 1

    if probe > index and (
        probe >= len(lines)
        or lines[probe].strip() == ""
        or lines[probe].strip().startswith("}")
    ):
        return None

    return index


inserts = {}
seen = set()

for cursor in parse().cursor.walk_preorder():
    if cursor.kind != K.COMPOUND_STMT or not is_in(cursor, root):
        continue

    for child in cursor.get_children():
        if child.kind in CONTROL:
            continue

        start = child.extent.start
        end = child.extent.end
        if start.file is None or start.line == end.line:
            continue

        key = (start.file.name, start.line)
        if key in seen:
            continue
        seen.add(key)

        lines = lines_of(start.file.name)
        before = needs_blank_before(lines, start.line - 1)
        after = needs_blank_after(lines, end.line - 1)
        for at in (before, after):
            if at is not None:
                inserts.setdefault(start.file.name, set()).add(at)

total = 0
for name, positions in sorted(inserts.items()):
    total += len(positions)
    print(f"{len(positions):4} {name}")

    if apply:
        lines = lines_of(name)
        for at in sorted(positions, reverse=True):
            lines.insert(at, "")
        pathlib.Path(name).write_text("\n".join(lines))

print(f"{total} blank lines {'added' if apply else 'missing'}")
