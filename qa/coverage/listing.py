#!/usr/bin/env python3
# extracts from a listing of the compiler the part that a 'LISTING KIND' case
# of 'cases.sh' compares with 'NNN.TARGET.KIND': the code of the source lines
# that a kind selects, found through the located comments of the listing,
# e.g. '; [0589.baz:12:5] x = 3 + 4' on x86_64 or '# [...]' on rv32i
#
#   folds     the instructions of each line marked '# folds', so constant
#             elements stay folded at compile time
#   stores    the instructions of each line with a string literal and the
#             read-only string constants, so short strings stay stored with
#             immediates
#   widths    the access width rv32i picks for each zero, copy and compare
#             that is not unrolled, so one that loses a proven alignment is
#             noticed
#   compares  the x86_64 compares of '==' and '!=' on lines marked
#             '# compares', so a known size keeps 'repe cmpsq' for more than
#             2 qwords and single compares for the rest
#   types     the type deduced for each untyped variable marked '# is TYPE',
#             'default' the default type of the target and an array its
#             element type; a type other than the marked one is printed as
#             "expected TYPE, got TYPE" and fails
#
# usage: listing.py KIND SOURCE TARGET <LISTING
#   TARGET  x86_64 or rv32i, the target of the compile
import re
import sys

# the default type of each target, see 'types'
DEFAULT_TYPE = {"x86_64": "i64", "rv32i": "i32"}


def located_line(comment, line):
    # the source line a located comment names, e.g. 12 of '; [f.baz:12:5]'
    m = re.match(r"\s*" + re.escape(comment) + r"\s*\[[^:]+:([0-9]+):", line)
    return int(m.group(1)) if m else None


def grouped(selected_lines, comment, listing, ends_at_indented_label):
    # the instructions under the located comments of each selected source
    # line; a label, directive or data at the first column ends a group, also
    # an indented label such as an inline function end when
    # 'ends_at_indented_label', the code after it belongs to the caller line
    code = {}
    group = 0
    for line in listing:
        n = located_line(comment, line)
        if n is not None:
            group = n
            continue
        if re.match(r"\S", line):
            group = 0
            continue
        if ends_at_indented_label and re.fullmatch(r"\s*\S+:\s*", line):
            group = 0
            continue
        if (group in selected_lines and
                not re.match(r"\s*" + re.escape(comment), line) and
                re.search(r"\S", line)):
            code[group] = code.get(group, "") + line + "\n"
    return code


def groups_text(source, selected_lines, code):
    return "".join(f"=== {n}: {source[n - 1]}\n{code.get(n, '')}"
                   for n in selected_lines)


def comment_of(target):
    return "#" if target == "rv32i" else ";"


def folds(source, listing, target):
    selected = [n for n, line in enumerate(source, 1)
                if line.endswith("# folds")]
    code = grouped(set(selected), comment_of(target), listing, True)
    return groups_text(source, selected, code)


def stores(source, listing, target):
    comment = comment_of(target)
    selected = [n for n, line in enumerate(source, 1)
                if '"' in line and not re.match(r"\s*#", line)]
    code = {}
    constants = ""
    group = 0
    previous = ""
    after_label = False
    for line in listing:
        n = located_line(comment, line)
        if n is not None:
            group = n
            continue
        if re.match(r"\S", line):
            # a string constant with the alignment before it
            if re.fullmatch(r"str\.[0-9.]+:", line):
                if re.match(r"\s*\.balign", previous):
                    constants += previous + "\n"
                constants += line + "\n"
                after_label = True
            else:
                if after_label:
                    constants += line + "\n"
                after_label = False
            previous = line
            group = 0
            continue
        previous = line
        after_label = False
        if (group in selected and not re.match(r"\s*" + re.escape(comment),
                                               line) and re.search(r"\S", line)):
            code[group] = code.get(group, "") + line + "\n"
    return (groups_text(source, selected, code) +
            f"=== read-only constants\n{constants}")


def widths(source, listing, target):
    # the source line of each width decision, the width and the head
    stripped = [line.lstrip() for line in source]
    text = ""
    for line in listing:
        if not re.search(r"# \[[^:]+:[0-9]+:[0-9]+\] ", line):
            continue
        m = re.search(r"[124]-byte accesses( after a [0-9]+ B head)?", line)
        if not m:
            continue
        n = int(re.match(r"[^[]*\[[^:]*:([0-9]+)", line).group(1))
        source_text = stripped[n - 1] if 0 < n <= len(stripped) else ""
        text += f"{n}: {source_text}: {m.group(0)}\n"
    return text


def compares(source, listing, target):
    # from the comment of the comparison to the instruction that stores the
    # result; label numbers depend on earlier code, they are left out
    selected = [n for n, line in enumerate(source, 1)
                if line.endswith("# compares")]
    code = {}
    group = 0
    for line in listing:
        m = re.match(r"\s*;\s*\[[^:]+:([0-9]+):[0-9]+\] \? .*(==|!=) ", line)
        if m:
            n = int(m.group(1))
            group = n if n in selected else 0
            continue
        if group == 0 or re.match(r"\s*;", line):
            continue
        text = re.sub(r"\.Lbaz_equal\.[0-9]+", ".Lbaz_equal.N", line.lstrip())
        if re.match(r"(mov rcx,|repe cmps|cmps|jne \.Lbaz_equal|\.Lbaz_equal)",
                    text):
            code[group] = code.get(group, "") + text + "\n"
        if text.startswith("set"):
            code[group] = code.get(group, "") + text.split(" ")[0] + "\n"
            group = 0
    return groups_text(source, selected, code)


def types(source, listing, target):
    marked = {}
    for n, line in enumerate(source, 1):
        if not re.search(r"var [a-z_0-9]+[\[ ].*# is [a-z_0-9]+$", line):
            continue
        name = re.sub(r"[\[ ].*", "", re.sub(r"^.*var ", "", line))
        tp = re.sub(r"^.*# is ", "", line)
        marked[n] = (name, DEFAULT_TYPE[target] if tp == "default" else tp)
    found = {}
    for line in listing:
        m = re.match(r"\s*[;#]\s*\[[^:]+:([0-9]+):[0-9]+\] ([a-z_0-9]+): "
                     r"([a-z_0-9]+)(\[[0-9]+\])? \(", line)
        if not m:
            continue
        n = int(m.group(1))
        if n in marked and m.group(2) == marked[n][0] and n not in found:
            found[n] = m.group(3)
    text = ""
    failed = False
    for n, (name, tp) in marked.items():
        got = found.get(n)
        if got == tp:
            text += f"{n}: {name}: {tp}\n"
            continue
        failed = True
        text += (f"{n}: {name}: expected {tp}, got "
                 f"{got or 'no variable comment'}\n")
    return text, failed


KINDS = {"folds": folds, "stores": stores, "widths": widths,
         "compares": compares, "types": types}


def main():
    kind, source_path, target = sys.argv[1:4]
    with open(source_path, encoding="latin-1") as f:
        source = f.read().split("\n")
    listing = sys.stdin.buffer.read().decode("latin-1").split("\n")
    result = KINDS[kind](source, listing, target)
    text, failed = result if isinstance(result, tuple) else (result, False)
    sys.stdout.buffer.write(text.encode("latin-1"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
