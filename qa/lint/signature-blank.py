import pathlib
import re
import sys

# a function whose declaration spans several lines gets a blank line after
# its opening brace, unless the body is empty ('}' follows)
# usage (from the workspace root): python3 qa/lint/signature-blank.py [apply]

apply = len(sys.argv) > 1 and sys.argv[1] == "apply"
counts = {}

# control statements, types and lambdas also open blocks after a multiline
# header but are not function signatures
not_function = re.compile(
    r"\s*(if|else|for|while|switch|do|try|catch|return|case|class|struct|"
    r"enum|union|namespace)\b"
)
lambda_intro = re.compile(r"\[[^\[\]]*\]\s*(\(|->|\{|mutable)")
# a parameter list, trailing return type or member initializer before '{'
declaration_end = re.compile(
    r"(\)|->[^{};]*|\})(\s*(const|override|final|noexcept))*\s*\{$"
)


def is_statement_boundary(text):
    stripped = text.strip()
    if stripped == "" or stripped.startswith(("//", "#")):
        return True

    # a template header line is not part of the declaration's line count
    if stripped.startswith("template"):
        return True

    return stripped.endswith((";", "{", "}", ":"))


for path in sorted(pathlib.Path("src").glob("*.[ch]pp")):
    lines = path.read_text().split("\n")
    add = set()
    for n, line in enumerate(lines):
        if not line.rstrip().endswith(" {"):
            continue
        start = n
        while start > 0 and not is_statement_boundary(lines[start - 1]):
            start -= 1
        # a one-line header is not a multiline signature
        if start == n:
            continue
        header = " ".join(lines[i].strip() for i in range(start, n + 1))
        if not_function.match(header) or lambda_intro.search(header):
            continue
        # a brace initializer argument such as 'f(a, {' is not a body
        if line.strip() == "{" or not declaration_end.search(header):
            continue
        # a declaration has a parameter list, an initializer 'x{' does not
        if "(" not in header or "=" in header.split("(")[0]:
            continue
        below = lines[n + 1].strip() if n + 1 < len(lines) else ""
        if below == "" or below.startswith("}"):
            continue
        counts["add"] = counts.get("add", 0) + 1
        if not apply:
            print(f"{path}:{n + 1}: add")
        add.add(n + 1)
    if apply and add:
        kept = []
        for i, l in enumerate(lines):
            if i in add:
                kept.append("")
            kept.append(l)
        path.write_text("\n".join(kept))

print(counts)
