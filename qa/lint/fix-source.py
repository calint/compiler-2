#!/usr/bin/env python3
# adds to the C++ files under 'src/' what clang-tidy does not fix:
#   '[[nodiscard]]' on every function that returns a value, except the const
#   methods 'modernize-use-nodiscard' covers
#   'const' on the by-value parameters a function body does not change
# usage: qa/lint/fix-source.py [--apply]
# without '--apply' it prints what would be added
#
# the additions are checked by parsing the changed source, an addition that
# breaks it is dropped, a dropped '[[nodiscard]]' prints the call that
# discards the value

import os
import pathlib
import sys
from dataclasses import dataclass

import clang.cindex as ci

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from libclang_tu import is_in, parse  # noqa: E402

K = ci.CursorKind
T = ci.TypeKind

FUNCTION_KINDS = {K.FUNCTION_DECL, K.CXX_METHOD, K.FUNCTION_TEMPLATE}
BODY_KINDS = FUNCTION_KINDS | {
    K.CONSTRUCTOR,
    K.DESTRUCTOR,
    K.CONVERSION_FUNCTION,
    K.LAMBDA_EXPR,
}
TEMPLATE_KINDS = {
    K.CLASS_TEMPLATE,
    K.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION,
    K.FUNCTION_TEMPLATE,
}
REFERENCE_TYPES = {T.LVALUEREFERENCE, T.RVALUEREFERENCE}
ARRAY_TYPES = {
    T.CONSTANTARRAY,
    T.INCOMPLETEARRAY,
    T.VARIABLEARRAY,
    T.DEPENDENTSIZEDARRAY,
}
# a moved or forwarded parameter would be copied when const
MOVE_NAMES = {"move", "forward"}
NODISCARD = "[[nodiscard]] "
CONST = "const "


@dataclass
class addition:
    file: str
    line: int
    column: int
    text: str
    # the function for '[[nodiscard]]', the parameter for 'const'
    usr: str
    name: str
    # the lines of the function a 'const' parameter belongs to
    first_line: int
    last_line: int


#
# finding the additions
#


def after_template_head(tokens):
    # '[[nodiscard]]' goes after 'template <...>'
    if not tokens or tokens[0].spelling != "template":
        return tokens

    depth = 0
    for i, token in enumerate(tokens):
        if token.spelling == "<":
            depth += 1
        if token.spelling == ">":
            depth -= 1
            if depth == 0:
                return tokens[i + 1 :]

    return tokens


def after_attributes(tokens):
    # 'const' goes after e.g. '[[maybe_unused]]'
    depth = 0
    for i, token in enumerate(tokens):
        if token.spelling == "[":
            depth += 1
            continue
        if token.spelling == "]":
            depth -= 1
            continue
        if depth == 0:
            return tokens[i:]

    return []


def returns_value(cursor):
    result = cursor.result_type
    if result.kind in (T.VOID, T.INVALID):
        return False

    # a return type deduced in a template is not known yet
    return result.spelling not in ("void", "auto", "decltype(auto)")


def has_mutable_fields(record):
    # like clang's 'hasMutableFields' the bases and member objects count too
    for c in record.get_children():
        if c.kind == K.FIELD_DECL and c.is_mutable_field():
            return True

        if c.kind == K.CXX_BASE_SPECIFIER:
            base = c.type.get_canonical().get_declaration()
            if has_mutable_fields(base):
                return True

        if c.kind == K.FIELD_DECL and c.type.get_canonical().kind == T.RECORD:
            member = c.type.get_canonical().get_declaration()
            if has_mutable_fields(member):
                return True

    return False


def is_std_function(t):
    if t.kind in REFERENCE_TYPES:
        t = t.get_pointee()

    declaration = t.get_canonical().get_declaration()
    if declaration.spelling != "function":
        return False

    # libc++ puts it in an inline namespace inside 'std'
    parent = declaration.semantic_parent
    while parent is not None and parent.kind == K.NAMESPACE:
        if parent.spelling == "std":
            return True

        parent = parent.semantic_parent

    return False


def tidy_adds_nodiscard(cursor, tokens_before_name):
    # mirrors clang-tidy 'modernize-use-nodiscard' which 'clang-tidy.sh fix'
    # applies, a case not certainly covered is left to this script
    if cursor.kind != K.CXX_METHOD or not cursor.is_const_method():
        return False

    # libclang does not tell which types in a template are dependent
    parent = cursor.semantic_parent
    if parent is None or parent.kind not in (K.CLASS_DECL, K.STRUCT_DECL):
        return False

    ancestor = parent
    while ancestor is not None and ancestor.kind != K.TRANSLATION_UNIT:
        if ancestor.kind in TEMPLATE_KINDS:
            return False

        ancestor = ancestor.semantic_parent

    if any(t.spelling == "noreturn" for t in tokens_before_name):
        return False

    if cursor.type.is_function_variadic() or has_mutable_fields(parent):
        return False

    for parameter in cursor.get_arguments():
        t = parameter.type
        if t.kind == T.POINTER or is_std_function(t):
            return False

        if t.kind in REFERENCE_TYPES and not t.get_pointee().is_const_qualified():
            return False

    return True


def nodiscard_addition(cursor):
    if cursor.kind not in FUNCTION_KINDS or cursor.canonical != cursor:
        return None

    # operators such as '=' and '<<' are used for their effect
    if cursor.spelling.startswith("operator") or cursor.spelling == "main":
        return None

    if cursor.lexical_parent and cursor.lexical_parent.kind == K.FRIEND_DECL:
        return None

    if not returns_value(cursor):
        return None

    tokens = after_template_head(list(cursor.get_tokens()))
    names = [i for i, t in enumerate(tokens) if t.spelling == cursor.spelling]
    if not tokens or not names:
        return None

    if any(t.spelling == "nodiscard" for t in tokens[: names[0]]):
        return None

    if tidy_adds_nodiscard(cursor, tokens[: names[0]]):
        return None

    location = tokens[0].location

    return addition(
        file=location.file.name,
        line=location.line,
        column=location.column,
        text=NODISCARD,
        usr=cursor.get_usr(),
        name=cursor.spelling,
        first_line=cursor.extent.start.line,
        last_line=cursor.extent.end.line,
    )


def direct_reference(cursor):
    # the declaration an expression names, through implicit conversions
    while cursor.kind == K.UNEXPOSED_EXPR:
        children = list(cursor.get_children())
        if len(children) != 1:
            return None

        cursor = children[0]

    if cursor.kind != K.DECL_REF_EXPR or cursor.referenced is None:
        return None

    return cursor.referenced.get_usr()


def moved_usrs(body):
    # 'std::move(p)' and 'std::forward(p)' move explicitly, 'return p' and
    # 'throw p' move implicitly
    usrs = set()
    for c in body.walk_preorder():
        if c.kind in (K.RETURN_STMT, K.CXX_THROW_EXPR):
            for child in c.get_children():
                usrs.add(direct_reference(child))
            continue

        if c.kind != K.CALL_EXPR or c.spelling not in MOVE_NAMES:
            continue

        for d in c.walk_preorder():
            if d.kind == K.DECL_REF_EXPR and d.referenced is not None:
                usrs.add(d.referenced.get_usr())

    return usrs


def const_addition(function, parameter, moved):
    t = parameter.type
    if not parameter.spelling or t.is_const_qualified():
        return None

    if t.kind in REFERENCE_TYPES or t.kind in ARRAY_TYPES:
        return None

    if parameter.get_usr() in moved:
        return None

    tokens = after_attributes(list(parameter.get_tokens()))
    if not tokens or any(tok.spelling == "..." for tok in tokens):
        return None

    # 'T* p' becomes 'T* const p' so the pointer itself is const
    token = tokens[0]
    if t.kind == T.POINTER:
        named = [tok for tok in tokens if tok.spelling == parameter.spelling]
        if not named:
            return None

        token = named[-1]

    return addition(
        file=token.location.file.name,
        line=token.location.line,
        column=token.location.column,
        text=CONST,
        usr=parameter.get_usr(),
        name=parameter.spelling,
        first_line=function.extent.start.line,
        last_line=function.extent.end.line,
    )


def const_additions(function):
    bodies = [c for c in function.get_children() if c.kind == K.COMPOUND_STMT]
    if not bodies:
        return []

    # member initializers are outside the body e.g. 'x_{std::move(x)}'
    moved = moved_usrs(function)
    additions = []
    for parameter in function.get_children():
        if parameter.kind != K.PARM_DECL:
            continue

        found = const_addition(function, parameter, moved)
        if found is not None:
            additions.append(found)

    return additions


def find_additions(tu, root):
    additions = []
    pending = [c for c in tu.cursor.get_children() if is_in(c, root)]
    while pending:
        cursor = pending.pop()
        found = nodiscard_addition(cursor)
        if found is not None:
            additions.append(found)

        if cursor.kind in BODY_KINDS:
            additions += const_additions(cursor)

        pending += list(cursor.get_children())

    return additions


#
# checking the additions
#


def apply_additions(originals, additions):
    texts = dict(originals)
    by_file = {}
    for a in additions:
        by_file.setdefault(a.file, []).append(a)

    for file, file_additions in by_file.items():
        data = originals[file]
        line_starts = [0]
        for i, byte in enumerate(data):
            if byte == ord("\n"):
                line_starts.append(i + 1)

        # later additions first so earlier offsets stay valid
        for a in sorted(
            file_additions, key=lambda a: (a.line, a.column), reverse=True
        ):
            offset = line_starts[a.line - 1] + a.column - 1
            data = data[:offset] + a.text.encode() + data[offset:]

        texts[file] = data

    return texts


def discarded_function(tu, diagnostic):
    # the function whose value a call discards
    cursor = ci.Cursor.from_location(tu, diagnostic.location)
    if cursor is None or cursor.referenced is None:
        return None

    return cursor.referenced.canonical.get_usr()


def broken_consts(group, consts):
    # the parameters of the function an error is in, the ones it names first,
    # an error in a library template is found through the notes leading to
    # the source e.g. 'in instantiation ... requested here'
    locations = [d.location for d in group]
    candidates = [
        a
        for a in consts
        for location in locations
        if location.file is not None
        and a.file == location.file.name
        and a.first_line <= location.line <= a.last_line
    ]
    texts = [d.spelling for d in group]
    named = [a for a in candidates if any(f"'{a.name}'" in t for t in texts)]

    return named or candidates


def error_groups(diagnostics):
    # an error with its notes, which follow it or are its children
    groups = []
    for d in diagnostics:
        if d.severity == ci.Diagnostic.Note and groups:
            groups[-1].append(d)
            continue

        groups.append([d, *d.children])

    return [g for g in groups if g[0].severity >= ci.Diagnostic.Error]


def check_additions(originals, additions):
    # returns the additions that keep the source compiling and the calls
    # discarding a value
    discards = []
    while True:
        texts = apply_additions(originals, additions)
        unsaved = [(file, texts[file].decode()) for file in texts]
        tu = parse(unsaved)

        consts = [a for a in additions if a.text == CONST]
        drop = set()
        for d in tu.diagnostics:
            if d.option == "-Wunused-result":
                usr = discarded_function(tu, d)
                discards.append((d, usr))
                drop.add(usr)

        for group in error_groups(tu.diagnostics):
            broken = broken_consts(group, consts)
            if not broken:
                for d in group:
                    print(f"unexpected error: {d}", file=sys.stderr)
                sys.exit(1)

            drop.update(a.usr for a in broken)

        if not drop:
            return additions, discards

        additions = [a for a in additions if a.usr not in drop]


def main():
    apply = "--apply" in sys.argv[1:]

    # paths are relative to the repository root
    os.chdir(pathlib.Path(__file__).resolve().parent.parent.parent)
    root = pathlib.Path("src").resolve()

    tu = parse()
    found = find_additions(tu, root)

    # a function template and its declarations are found once each
    unique = {(a.file, a.line, a.column, a.text): a for a in found}
    originals = {a.file: pathlib.Path(a.file).read_bytes() for a in found}
    additions, discards = check_additions(originals, list(unique.values()))

    for d, _ in discards:
        location = d.location
        print(
            f"{location.file.name}:{location.line}: discards the value, "
            "no '[[nodiscard]]' added"
        )

    nodiscards = [a for a in additions if a.text == NODISCARD]
    consts = [a for a in additions if a.text == CONST]
    for a in sorted(additions, key=lambda a: (a.file, a.line, a.column)):
        print(f"{a.file}:{a.line}: {a.text.strip()} {a.name}")

    print(f"{len(nodiscards)} '[[nodiscard]]' and {len(consts)} 'const' added")
    if not apply:
        return

    texts = apply_additions(originals, additions)
    for file, data in texts.items():
        if data != originals[file]:
            pathlib.Path(file).write_bytes(data)


main()
