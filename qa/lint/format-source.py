#!/usr/bin/env python3
# applies the source formatting rules to C++ files under 'src/': the layout
# of the classes and the blank lines of AGENTS.md
# usage: qa/lint/format-source.py [--apply] [files relative to the root...]
# without '--apply' it prints the files that would change
#
# the classes and the statements are found in the syntax tree of 'src/main.cpp'
# parsed by libclang, the text is rearranged by whole lines of the members,
# the blank lines are added to and removed from the gaps between statements

import os
import pathlib
import re
import sys
from dataclasses import dataclass

import clang.cindex as ci

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from libclang_tu import parse  # noqa: E402

K = ci.CursorKind

CLASS_KINDS = {K.CLASS_DECL, K.STRUCT_DECL, K.CLASS_TEMPLATE}
FUNCTION_KINDS = {
    K.CXX_METHOD,
    K.CONSTRUCTOR,
    K.DESTRUCTOR,
    K.FUNCTION_TEMPLATE,
    K.CONVERSION_FUNCTION,
}
ALIAS_KINDS = {
    K.TYPE_ALIAS_DECL,
    K.TYPE_ALIAS_TEMPLATE_DECL,
    K.TYPEDEF_DECL,
    K.ENUM_DECL,
}
RECORD_KINDS = {K.CLASS_DECL, K.STRUCT_DECL, K.CLASS_TEMPLATE, K.UNION_DECL}
REFERENCE_KINDS = {
    K.TYPE_REF,
    K.TEMPLATE_REF,
    K.DECL_REF_EXPR,
    K.MEMBER_REF_EXPR,
    K.MEMBER_REF,
}
ACCESS_LABEL = re.compile(r"^\s*(public|protected|private):$")
ACCESS_NAMES = {
    ci.AccessSpecifier.PUBLIC: "public",
    ci.AccessSpecifier.PROTECTED: "protected",
    ci.AccessSpecifier.PRIVATE: "private",
}


@dataclass
class member:
    cursor: ci.Cursor
    access: str
    # leading comments and the declaration, nested classes already formatted
    lines: list
    # a blank line separated it from the previous member in the source
    blank_before: bool


def is_blank(line):
    return line.strip() == ""


def strip_blank_edges(lines):
    start = 0
    while start < len(lines) and is_blank(lines[start]):
        start += 1
    end = len(lines)
    while end > start and is_blank(lines[end - 1]):
        end -= 1

    return lines[start:end]


#
# member kinds
#


def is_function(m):
    return m.cursor.kind in FUNCTION_KINDS


def is_type(m):
    return m.cursor.kind in ALIAS_KINDS or m.cursor.kind in RECORD_KINDS


def type_rank(m):
    # aliases and enums come before structs and classes
    return 0 if m.cursor.kind in ALIAS_KINDS else 1


def special_rank(m):
    # default constructor, copy, move, copy assignment, move assignment,
    # destructor, 'None' for other functions
    c = m.cursor
    if c.kind == K.DESTRUCTOR:
        return 5

    if c.kind == K.CONSTRUCTOR:
        if c.is_default_constructor():
            return 0
        if c.is_copy_constructor():
            return 1
        if c.is_move_constructor():
            return 2

        return None

    if c.kind != K.CXX_METHOD:
        return None
    if c.is_copy_assignment_operator_method():
        return 3
    if c.is_move_assignment_operator_method():
        return 4

    return None


def is_defaulted_special(m):
    # e.g. 'expr_any() = default;' or 'auto operator=(x&&) -> x& = delete;'
    if not is_function(m) or special_rank(m) is None:
        return False

    return m.cursor.is_default_method() or m.cursor.is_deleted_method()


def is_virtual_destructor(m):
    return m.cursor.kind == K.DESTRUCTOR and m.cursor.is_virtual_method()


def is_constructor(m, class_name):
    c = m.cursor
    if c.kind == K.FUNCTION_TEMPLATE:
        return c.spelling == class_name

    return c.kind == K.CONSTRUCTOR and not is_defaulted_special(m)


#
# joining members into lines
#


def spaced(members):
    # members staying in source order keep their source spacing
    lines = []
    for m in members:
        if lines and m.blank_before:
            lines.append("")
        lines += m.lines

    return lines


def separated(members):
    lines = []
    for m in members:
        if lines:
            lines.append("")
        lines += m.lines

    return lines


def tight(members):
    lines = []
    for m in members:
        lines += m.lines

    return lines


def join_groups(groups):
    lines = []
    for group in groups:
        if not group:
            continue

        if lines:
            lines.append("")
        lines += group

    return lines


def ordered_types(types, key):
    # sorted by 'key' but a member the others refer to is hoisted right before
    # its first use e.g. 'struct memory' before
    # 'using argument = std::variant<memory>'
    by_usr = {m.cursor.get_usr(): m for m in types}
    ordered = []
    done = set()

    def add(m):
        if id(m) in done:
            return

        done.add(id(m))
        for usr in referenced_usrs(m.cursor):
            if usr in by_usr and by_usr[usr] is not m:
                add(by_usr[usr])
        ordered.append(m)

    # the sort is stable so members of one key keep their source order
    for m in sorted(types, key=key):
        add(m)

    return ordered


def types_group(types):
    return separated(ordered_types(types, type_rank))


def top_lines(members):
    # types are set apart, fields keep their source spacing
    lines = []
    prev = None
    for m in members:
        if lines and (m.blank_before or is_type(m) or is_type(prev)):
            lines.append("")
        lines += m.lines
        prev = m

    return lines


#
# reading a class
#


def body_lines(cursor):
    # the line of the opening brace and the line of the closing brace
    for token in cursor.get_tokens():
        if token.spelling == "{":
            return token.location.line - 1, cursor.extent.end.line - 1

    return None


def class_keyword(cursor):
    # the keyword after the template head decides the default access
    depth = 0
    for token in cursor.get_tokens():
        if token.spelling == "<":
            depth += 1
        if token.spelling == ">":
            depth -= 1
        if depth == 0 and token.spelling in ("class", "struct"):
            return token.spelling

    return "class"


def is_class_definition(cursor):
    return cursor.kind in CLASS_KINDS and cursor.is_definition()


def member_cursors(cursor, open_n, close_n):
    # declarations sharing lines e.g. 'int a, b;' become one member
    groups = []
    for c in cursor.get_children():
        if c.kind == K.CXX_ACCESS_SPEC_DECL:
            continue

        start = c.extent.start.line - 1
        end = c.extent.end.line - 1

        # base classes, template parameters and attributes are in the head
        if start <= open_n or end >= close_n:
            continue

        if groups and start <= groups[-1][2]:
            groups[-1][2] = max(groups[-1][2], end)
            continue

        groups.append([c, start, end])

    return groups


def declaration_lines(cursor, lines, start, end):
    if not is_class_definition(cursor):
        return lines[start : end + 1]

    open_n, close_n = body_lines(cursor)

    return lines[start : open_n + 1] + format_body(cursor, lines) + [
        lines[close_n]
    ]


def trailing_note(gap):
    # the comment lines at the start of 'gap' when a blank line follows them
    count = 0
    while count < len(gap) and gap[count].strip().startswith("//"):
        count += 1

    if count == 0 or count == len(gap) or not is_blank(gap[count]):
        return []

    return gap[:count]


def class_members(cursor, lines, open_n, close_n):
    members = []
    prev_end = open_n
    for c, start, end in member_cursors(cursor, open_n, close_n):
        # the lines between members belong to the next one, except the labels
        # that are emitted anew
        gap = [l for l in lines[prev_end + 1 : start] if not ACCESS_LABEL.match(l)]
        gap = without_markers(gap)

        # a comment right below a member, set apart from the next one by a
        # blank line, is a note of that member and moves with it
        note = trailing_note(gap)
        if members and note:
            members[-1].lines += note
            gap = gap[len(note) :]

        # a blank line after a detached comment stays
        first = 0
        while first < len(gap) and is_blank(gap[first]):
            first += 1
        leading = gap[first:]
        blank_before = first > 0

        members.append(
            member(
                cursor=c,
                access=ACCESS_NAMES[c.access_specifier],
                lines=leading + declaration_lines(c, lines, start, end),
                blank_before=blank_before,
            )
        )
        prev_end = end

    # comments after the last member stay after it
    trailing = [
        l for l in lines[prev_end + 1 : close_n] if not ACCESS_LABEL.match(l)
    ]
    trailing = strip_blank_edges(trailing)
    if members and trailing:
        members[-1].lines += [""] + trailing

    return members


#
# the class layout rule
#


def sections_by_access(members):
    # consecutive members of one access form a section
    sections = []
    for m in members:
        if sections and sections[-1][0] == m.access:
            sections[-1][1].append(m)
            continue

        sections.append([m.access, [m]])

    return sections


def referenced_usrs(cursor):
    # types, constants and fields that a member names
    for c in cursor.walk_preorder():
        if c.kind not in REFERENCE_KINDS:
            continue

        if c.referenced is not None:
            yield c.referenced.get_usr()


def needed_accesses(private_data, members):
    # the accesses of non-private types that the private data refers to,
    # directly or through other such types
    types = {
        m.cursor.get_usr(): m
        for m in members
        if is_type(m) and m.access != "private"
    }

    accesses = set()
    seen = set()
    pending = [m.cursor for m in private_data]
    while pending:
        for usr in referenced_usrs(pending.pop()):
            if usr in seen or usr not in types:
                continue

            seen.add(usr)
            accesses.add(types[usr].access)
            pending.append(types[usr].cursor)

    return accesses


def take(members, select):
    kept = [m for m in members if not select(m)]
    taken = [m for m in members if select(m)]

    return kept, taken


def by_name(functions):
    # the sort is stable so overloads keep their source order
    return sorted(functions, key=lambda m: m.cursor.spelling)


def is_lifecycle(m, class_name):
    if is_constructor(m, class_name) or is_defaulted_special(m):
        return True

    return m.cursor.kind == K.DESTRUCTOR


def lifecycle_group(members, class_name):
    # constructors, then the defaulted and deleted special members without
    # blank lines, then the destructors set apart
    members, constructors = take(members, lambda m: is_constructor(m, class_name))
    members, specials = take(members, is_defaulted_special)
    specials.sort(key=special_rank)
    specials, virtual_destructors = take(specials, is_virtual_destructor)

    return join_groups(
        [
            separated(constructors),
            tight(specials),
            tight(virtual_destructors),
            separated(members),
        ]
    )


# statements read best as how they are written and then compiled
STATEMENT_BASE = "statement"
STATEMENT_METHODS = ["source_to", "compile"]


def is_override(m):
    # 'override' and 'final' are attributes of the method in the syntax tree
    if m.cursor.kind != K.CXX_METHOD:
        return False

    return any(
        c.kind in (K.CXX_OVERRIDE_ATTR, K.CXX_FINAL_ATTR)
        for c in m.cursor.get_children()
    )


# emitted by 'method_groups' and dropped when read so they are not repeated
OVERRIDES_TITLE = "// overridden methods"
VIRTUALS_TITLE = "// virtual methods"
METHODS_TITLE = "// class methods"
STATICS_TITLE = "// statics"
MARKER_TITLES = (OVERRIDES_TITLE, VIRTUALS_TITLE, METHODS_TITLE, STATICS_TITLE)


def is_virtual(m):
    return m.cursor.kind == K.CXX_METHOD and m.cursor.is_virtual_method()


def without_markers(gap):
    # a title and the '//' lines framing it
    drop = set()
    for i, line in enumerate(gap):
        if line.strip() not in MARKER_TITLES:
            continue

        drop.add(i)
        for j in (i - 1, i + 1):
            if 0 <= j < len(gap) and gap[j].strip() == "//":
                drop.add(j)

    return [l for i, l in enumerate(gap) if i not in drop]


def marked(title, members, indent):
    return [indent + "//", indent + title, indent + "//", ""] + separated(
        members
    )


def method_groups(overrides, virtuals, methods, indent):
    # the markers tell the base interface, the interface for subclasses and
    # the class's own methods apart, a section with only its own methods needs
    # none, static methods need no object and close the section marked
    methods, statics = take(methods, lambda m: m.cursor.is_static_method())
    groups = [separated(by_name(methods))]
    if overrides or virtuals:
        groups = []
        for title, group in (
            (OVERRIDES_TITLE, overrides),
            (VIRTUALS_TITLE, virtuals),
            (METHODS_TITLE, by_name(methods)),
        ):
            if group:
                groups.append(marked(title, group, indent))

    if statics:
        groups.append(marked(STATICS_TITLE, by_name(statics), indent))

    return groups


def format_section(members, lifecycle, class_name, leading_methods, indent):
    members, types = take(members, is_type)
    leading = []
    for name in leading_methods:
        members, found = take(
            members, lambda m, n=name: is_function(m) and m.cursor.spelling == n
        )
        leading += found

    # overrides implement the base interface, then the virtual methods
    # subclasses may override, then the own methods, data and other members
    # keep their source order before them
    members, overrides = take(members, is_override)
    members, virtuals = take(members, is_virtual)
    members, methods = take(members, is_function)

    return join_groups(
        [
            types_group(types),
            lifecycle_group(lifecycle, class_name),
            spaced(members),
            *method_groups(
                leading + by_name(overrides),
                by_name(virtuals),
                methods,
                indent,
            ),
        ]
    )


def format_body(cursor, lines):
    open_n, close_n = body_lines(cursor)
    members = class_members(cursor, lines, open_n, close_n)
    if not members:
        return lines[open_n + 1 : close_n]

    class_name = cursor.spelling
    default_access = "private" if class_keyword(cursor) == "class" else "public"
    label_indent = re.match(r"\s*", lines[close_n]).group(0) + "  "
    member_indent = label_indent + "  "
    leading_methods = []
    if derives_from(cursor, STATEMENT_BASE):
        leading_methods = STATEMENT_METHODS

    for m in members:
        m.access = least_access(m)

    # private data and types stay in source order at the top, private
    # functions go to the bottom sorted by name after the constructors and
    # destructors
    private = [m for m in members if m.access == "private"]
    _, private_data = take(private, lambda m: not is_function(m))
    _, private_functions = take(private, is_function)
    private_functions, private_lifecycle = take(
        private_functions, lambda m: is_lifecycle(m, class_name)
    )

    others = [m for m in members if m.access != "private"]

    # the needed types of other accesses come first, then the private types
    # and fields in source order, each after what it refers to
    needed = needed_accesses(private_data, members)
    top_types = []
    for access in ("public", "protected"):
        if access not in needed:
            continue

        others, types = take(
            others, lambda m, a=access: m.access == a and is_type(m)
        )
        top_types += sorted(types, key=type_rank)

    top = ordered_types(top_types + private_data, lambda m: 0)
    body_sections = [[a, top_lines(t)] for a, t in sections_by_access(top)]

    # the constructors and destructors of all public sections open the first
    # one, which keeps its place even when it held only those
    sections = sections_by_access(others)
    public_lifecycle = [
        m for m in others if m.access == "public" and is_lifecycle(m, class_name)
    ]
    public_lifecycle_ids = {id(m) for m in public_lifecycle}

    first_public = True
    for access, section_members in sections:
        section_members = [
            m for m in section_members if id(m) not in public_lifecycle_ids
        ]
        if access == "public" and first_public:
            first_public = False
            content = format_section(
                section_members,
                public_lifecycle,
                class_name,
                leading_methods,
                member_indent,
            )
            body_sections.append([access, content])
            continue

        section_members, lifecycle = take(
            section_members, lambda m: is_lifecycle(m, class_name)
        )
        content = format_section(
            section_members, lifecycle, class_name, leading_methods, member_indent
        )
        body_sections.append([access, content])

    private_functions, private_overrides = take(private_functions, is_override)
    private_functions, private_virtuals = take(private_functions, is_virtual)
    private_content = join_groups(
        [
            lifecycle_group(private_lifecycle, class_name),
            *method_groups(
                by_name(private_overrides),
                by_name(private_virtuals),
                private_functions,
                member_indent,
            ),
        ]
    )
    body_sections.append(["private", private_content])

    return emit_body(body_sections, default_access, label_indent)


def emit_body(sections, default_access, label_indent):
    # adjacent sections of one access become one
    merged = []
    for access, content in sections:
        if not content:
            continue

        if merged and merged[-1][0] == access:
            merged[-1][1] = join_groups([merged[-1][1], content])
            continue

        merged.append([access, content])

    body = []
    for i, (access, content) in enumerate(merged):
        # the first section needs no label when it has the default access
        if i == 0 and access == default_access:
            body += content
            continue

        if body:
            body.append("")
        body.append(f"{label_indent}{access}:")
        body += content

    return body


#
# the least access of nested types
#

ACCESS_ORDER = ["private", "protected", "public"]
SCOPE_KINDS = CLASS_KINDS | FUNCTION_KINDS | {K.FUNCTION_DECL}

# the least access each nested type needs, by 'usr', filled by
# 'analyze_access', types named nowhere else need only private
required_access = {}

# the direct bases and the names of the classes by 'usr', filled by
# 'analyze_access'
class_bases = {}
class_names = {}


def derives_from(cursor, base_name):
    usr = cursor.get_usr()
    base_usrs = [u for u, n in class_names.items() if n == base_name]

    return any(
        b != usr and is_derived(usr, b, class_bases) for b in base_usrs
    )


def enclosing_classes(cursor):
    # a member function defined outside its class still has its access
    usrs = set()
    while cursor is not None and cursor.kind != K.TRANSLATION_UNIT:
        if cursor.kind in CLASS_KINDS:
            usrs.add(cursor.get_usr())
        cursor = cursor.semantic_parent

    return frozenset(usrs)


def nested_type(reference):
    # the nested type a reference names, 'None' for other references
    target = reference.referenced
    if target is None:
        return None

    # naming an enumerator needs the access of its enum
    if target.kind == K.ENUM_CONSTANT_DECL:
        target = target.semantic_parent

    if target.kind not in ALIAS_KINDS and target.kind not in RECORD_KINDS:
        return None

    owner = target.semantic_parent
    if owner is None or owner.kind not in CLASS_KINDS:
        return None

    return target


def is_derived(usr, base_usr, bases):
    pending = [usr]
    seen = set()
    while pending:
        cur = pending.pop()
        if cur == base_usr:
            return True

        if cur in seen:
            continue

        seen.add(cur)
        pending += bases.get(cur, ())

    return False


def analyze_access(tu, root):
    # each nested type needs private when only its class names it, protected
    # when a derived class does and public when other code does
    bases = class_bases
    references = []
    pending = [
        (c, frozenset())
        for c in tu.cursor.get_children()
        if c.location.file is not None
        and pathlib.Path(c.location.file.name).resolve().is_relative_to(root)
    ]
    while pending:
        cursor, classes = pending.pop()
        if cursor.kind in SCOPE_KINDS:
            classes = enclosing_classes(cursor)

        if cursor.kind in CLASS_KINDS and cursor.is_definition():
            class_names[cursor.get_usr()] = cursor.spelling
            bases[cursor.get_usr()] = {
                c.referenced.get_usr()
                for c in cursor.get_children()
                if c.kind == K.CXX_BASE_SPECIFIER and c.referenced
            }

        if cursor.kind in REFERENCE_KINDS:
            target = nested_type(cursor)
            if target is not None:
                references.append((target, classes))

        pending += [(c, classes) for c in cursor.get_children()]

    for target, classes in references:
        usr = target.get_usr()
        owner = target.semantic_parent.get_usr()
        access = "public"
        if usr in classes or owner in classes:
            access = "private"
        elif any(is_derived(c, owner, bases) for c in classes):
            access = "protected"

        cur = required_access.get(usr, "private")
        required_access[usr] = max(cur, access, key=ACCESS_ORDER.index)


def least_access(m):
    # access is only lowered, a type named where it cannot be would not have
    # compiled
    if not is_type(m):
        return m.access

    needed = required_access.get(m.cursor.get_usr(), "private")

    return min(m.access, needed, key=ACCESS_ORDER.index)


#
# files
#


def class_cursors(cursor, path):
    # classes of the file at namespace scope
    for c in cursor.get_children():
        if c.location.file is None:
            continue
        if pathlib.Path(c.location.file.name).resolve() != path:
            continue

        if c.kind == K.NAMESPACE:
            yield from class_cursors(c, path)
            continue

        if is_class_definition(c):
            yield c


def format_file(tu, path):
    lines = path.read_text().split("\n")
    classes = sorted(
        class_cursors(tu.cursor, path.resolve()),
        key=lambda c: c.extent.start.line,
    )

    out = []
    n = 0
    for c in classes:
        open_n, close_n = body_lines(c)
        out += lines[n : open_n + 1]
        out += format_body(c, lines)
        n = close_n

    out += lines[n:]

    return "\n".join(out)


#
# the blank line rules of AGENTS.md
#
# a rule looks at the statements of one block and asks for a blank line or for
# none in the gap before a statement, 'wants' of index 0 is the gap after the
# '{' of the block, a later one the gap after the statement before. the want
# with the highest priority wins, a blank line is asked for by default. a new
# rule is a function 'rule_x(block, want)' added to 'BLANK_LINE_RULES'
#

BLANK = "blank"
TIGHT = "tight"

# a rule that only keeps statements together gives way to the others
PRIORITY = {BLANK: 2, TIGHT: 1}

CONTROL_KINDS = {
    K.IF_STMT,
    K.FOR_STMT,
    K.CXX_FOR_RANGE_STMT,
    K.WHILE_STMT,
    K.DO_STMT,
    K.SWITCH_STMT,
    K.CXX_TRY_STMT,
}
# blocks, labels and empty statements do not ask for blank lines themselves
PASSIVE_KINDS = {K.COMPOUND_STMT, K.NULL_STMT, K.LABEL_STMT}
CASE_KINDS = {K.CASE_STMT, K.DEFAULT_STMT}
DEFINITION_KINDS = FUNCTION_KINDS | {K.FUNCTION_DECL}

ASSERT = re.compile(r"\s*assert\(")


@dataclass
class stmt:
    kind: object
    # the first line, of the case label when it has one, the line the
    # statement itself starts at and its last line, all 0-based
    first: int
    start: int
    last: int
    # the first statement after a case label needs no blank line before it
    after_label: bool
    is_assert: bool

    @property
    def is_multiline(self):
        return self.start != self.last

    @property
    def is_control(self):
        return self.kind in CONTROL_KINDS

    @property
    def is_plain(self):
        # a statement that a rule treats as one line of code
        return not (
            self.is_control
            or self.is_assert
            or self.kind in PASSIVE_KINDS | {K.RETURN_STMT}
        )


@dataclass
class block:
    lines: list
    # the line of the '{'
    open_line: int
    stmts: list
    # the definition this block is the body of, none for other blocks
    function: object = None

    def has_multiline_signature(self):
        if self.function is None:
            return False

        start = self.function.extent.start.line - 1
        while self.lines[start].lstrip().startswith("template"):
            start += 1

        # an attribute on a line of its own is part of the declaration
        while start > 0 and self.lines[start - 1].lstrip().startswith("[["):
            start -= 1

        return start != self.open_line


def statement_of(child, lines):
    after_label = False
    effective = child
    while effective.kind in CASE_KINDS:
        after_label = True
        effective = list(effective.get_children())[-1]

    first = child.extent.start.line - 1
    start = effective.extent.start.line - 1
    last = effective.extent.end.line - 1
    is_assert = ASSERT.match(lines[start]) is not None

    return stmt(effective.kind, first, start, last, after_label, is_assert)


def source_path(cursor, resolved):
    # the file of a cursor, 'resolved' remembers the paths by name because
    # resolving one is a file system call
    file = cursor.location.file
    if file is None:
        return None

    if file.name not in resolved:
        resolved[file.name] = pathlib.Path(file.name).resolve()

    return resolved[file.name]


def blocks_by_file(tu, paths, texts):
    # the blocks of each file in one walk, the headers of the library and the
    # syntax of other files are not entered
    wanted = {p.resolve() for p in paths}
    resolved = {}
    bodies = {}
    seen = set()
    found = {p: [] for p in wanted}
    for top in tu.cursor.get_children():
        path = source_path(top, resolved)
        if path not in wanted:
            continue

        lines = texts[path]
        for cursor in top.walk_preorder():
            if cursor.kind in DEFINITION_KINDS:
                for child in cursor.get_children():
                    if child.kind == K.COMPOUND_STMT:
                        bodies[(path, child.extent.start.line - 1)] = cursor

                continue

            if cursor.kind != K.COMPOUND_STMT:
                continue

            open_line = cursor.extent.start.line - 1
            if source_path(cursor, resolved) != path or (path, open_line) in seen:
                continue

            seen.add((path, open_line))
            stmts = [statement_of(c, lines) for c in cursor.get_children()]
            found[path].append(
                block(lines, open_line, stmts, bodies.get((path, open_line)))
            )

    return found


def rule_signature(b, want):
    # a function whose declaration spans several lines has a blank line after
    # its '{'
    if b.stmts and b.has_multiline_signature():
        want(0, BLANK)


def rule_multiline(b, want):
    # a statement that spans several lines is set apart, a multiline 'if' on
    # both sides and other control statements below
    for i, s in enumerate(b.stmts):
        if not s.is_multiline or s.kind in PASSIVE_KINDS:
            continue

        # the first statement of a block needs no blank line above it
        if i > 0 and (not s.is_control or s.kind == K.IF_STMT):
            if not s.after_label:
                want(i, BLANK)

        want(i + 1, BLANK)


def rule_assert_groups(b, want):
    # consecutive asserts form a group, set apart on both sides
    i = 0
    while i < len(b.stmts):
        if not b.stmts[i].is_assert:
            i += 1
            continue

        end = i
        while end + 1 < len(b.stmts) and b.stmts[end + 1].is_assert:
            end += 1

        if i > 0 and not b.stmts[i].after_label:
            want(i, BLANK)

        want(end + 1, BLANK)
        i = end + 1


def rule_tight_return(b, want):
    # a return after the only statement of a block, or of a case, stays
    # with it
    for i, s in enumerate(b.stmts):
        if i == 0 or s.kind != K.RETURN_STMT or s.is_multiline:
            continue

        before = b.stmts[i - 1]
        if not before.is_plain or before.is_multiline:
            continue

        # a note below the statement keeps the gap
        gap = range(before.last + 1, s.first)
        if any(not is_blank(b.lines[n]) for n in gap):
            continue

        # a comment above the statement keeps the gap
        opening = b.open_line if i == 1 else before.first
        between = range(opening + 1, before.start if i != 1 else before.first)
        if any(not is_blank(b.lines[n]) for n in between):
            continue

        if i == 1 or before.after_label:
            want(i, TIGHT)


BLANK_LINE_RULES = [
    rule_signature,
    rule_multiline,
    rule_assert_groups,
    rule_tight_return,
]


def block_gaps(b):
    wants = {}

    def want(index, state):
        # a gap after the last statement is the end of the block
        if index >= len(b.stmts):
            return

        best = wants.get(index)
        if best is None or PRIORITY[state] > PRIORITY[best]:
            wants[index] = state

    for rule in BLANK_LINE_RULES:
        rule(b, want)

    return wants


def blank_line_edits(blocks, lines):
    # the lines to insert a blank line before and the blank lines to remove
    inserts = set()
    removes = set()
    for b in blocks:
        for index, state in block_gaps(b).items():
            above = b.open_line if index == 0 else b.stmts[index - 1].last
            region = range(above + 1, b.stmts[index].first)
            blanks = [n for n in region if is_blank(lines[n])]
            if state == TIGHT:
                removes.update(blanks)
            elif not blanks:
                inserts.add(above + 1)

    return inserts, removes


def apply_blank_lines(lines, inserts, removes):
    out = []
    for n, line in enumerate(lines):
        if n in inserts:
            out.append("")

        if n not in removes:
            out.append(line)

    return out


def main():
    args = sys.argv[1:]
    apply = "--apply" in args
    files = [a for a in args if a != "--apply"]

    # paths are relative to the repository root
    os.chdir(pathlib.Path(__file__).resolve().parent.parent.parent)

    paths = [pathlib.Path(f) for f in files]
    if not paths:
        paths = sorted(pathlib.Path("src").glob("*.[ch]pp"))

    root = pathlib.Path("src").resolve()
    tu = parse()
    analyze_access(tu, root)
    parsed = {
        pathlib.Path(i.include.name).resolve() for i in tu.get_includes()
    }
    parsed.add(pathlib.Path("src/main.cpp").resolve())

    paths = [p for p in paths if p.resolve() in parsed]

    # the class layout moves lines, so the blank lines are looked at in the
    # source as it is after it
    laid_out = 0
    for path in paths:
        text = path.read_text()
        formatted = format_file(tu, path)
        if formatted == text:
            continue

        laid_out += 1
        if apply:
            path.write_text(formatted)
            print(f"laid out: {path}")
            continue

        # a diff of moved sections is hard to read, 'git diff' after applying
        # shows them better
        print(f"would lay out: {path}")

    if apply and laid_out:
        tu = parse()

    texts = {p.resolve(): p.read_text().split("\n") for p in paths}
    blocks = blocks_by_file(tu, paths, texts)

    changes = 0
    for path in paths:
        lines = texts[path.resolve()]
        inserts, removes = blank_line_edits(blocks[path.resolve()], lines)
        if not inserts and not removes:
            continue

        changes += len(inserts) + len(removes)
        if apply:
            path.write_text("\n".join(apply_blank_lines(lines, inserts, removes)))
            print(f"blank lines: {path}")
            continue

        print(f"would change blank lines: {path} (+{len(inserts)} -{len(removes)})")

    verb = "done" if apply else "pending"
    print(f"{laid_out} file(s) laid out, {changes} blank line change(s) {verb}")


main()
