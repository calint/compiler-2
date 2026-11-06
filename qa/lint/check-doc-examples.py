#!/usr/bin/env python3
# checks the examples of the documentation with the built '../../baz':
# 1. the fenced blocks of 'etc/readme/README.*.md' and 'etc/docs/README.md':
#    a block is baz when its first line starts with 'type', 'func', 'dat',
#    'var', 'let' or 'include'; the column of notes right of the code (after
#    three spaces) is cut; a body written '{ ... }' takes the declaration of
#    that name from an earlier block of the same chapter ('## '), a name the
#    block uses but does not declare likewise; a block without a declaration
#    at the left margin is a statement list, wrapped in 'func main() { ... }'
#    when it declares every name it uses (else skipped); a block of
#    declarations gets an empty 'main' when it has none; a block with a note
#    '=> "MESSAGE"' or 'rejected: "MESSAGE"' is split at its blank lines,
#    each such part compiled with the parts that have no note and the message
#    expected; a block followed by error output ('FILE:L:C: MESSAGE') expects
#    those messages and columns; other blocks are expected to compile
# 2. the quoted code of the prose marked "'CODE' is rejected [on TARGET]",
#    "'CODE' is accepted" or "'CODE' gives "MESSAGE"", wrapped as a statement
#    list, when it is a statement and every name it uses is a keyword, a
#    built-in or declared in it; code rejected on one target must compile on
#    the other
# 3. in 'etc/tutorial.baz' a commented-out line '#   CODE' followed by
#    '#   compile error ...' and a comment "'CODE' is a compile error" or
#    "'CODE' is an error" right below a line of code: the code put there must
#    fail at that line
# 4. the exit codes and messages of the failed checks that the README lists,
#    each with a probe program of this file
# 5. 'etc/tutorial.baz' and 'prog.baz' run with the options of 'run.sh' and
#    exit with 0, the tutorial also on rv32i
# the summary counts the checked and the skipped examples, '-v' lists each,
# a skipped one with its reason; the jobs run in parallel, about 1 s
# needs the tools of the README requirements: 'nasm', 'ld', 'llvm-mc',
# 'ld.lld', 'qemu-riscv32', 'qemu-system-riscv32'
# usage: qa/lint/check-doc-examples.py [-v]
# exits 1 and prints each example that does not behave as documented
import concurrent.futures
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BAZ = os.path.join(ROOT, 'baz')
DOCS = [os.path.join(ROOT, 'etc', 'readme', f'README.{n}.md') for n in range(1, 7)] + [
    os.path.join(ROOT, 'etc', 'docs', 'README.md'),
]
README_FIRST = DOCS[0]
TUTORIAL = os.path.join(ROOT, 'etc', 'tutorial.baz')
PROG = os.path.join(ROOT, 'prog.baz')

# the options 'run.sh' compiles with
RUN_OPTIONS = ['--checks=noub,line', '--report=registers']

DECLARATION = re.compile(r'(type|func|dat|var|let|include)\b')
TOP_LEVEL_ONLY = re.compile(r'(type|func|dat|include)\b')
# a note right of the code starts after three spaces
NOTE_GAP = re.compile(r'(?<=\S)   +(?=\S)')
NOTE_MESSAGE = re.compile(r'(?:=>|rejected:) "([^"]*)"')
ERROR_LINE = re.compile(r'^(\S+?):(\d+):(\d+): (.*)$')
ELIDED = re.compile(r'^(type|func) (\S+?)[<( ].*\{ \.\.\. \}$')

KEYWORDS = {
    'func', 'noinline', 'mut', 'type', 'dat', 'var', 'let', 'foo', 'loop',
    'if', 'else', 'continue', 'break', 'return', 'self', 'and', 'or', 'not',
    'true', 'false', 'include', 'res', 'bool', 'int', 'i8', 'i16', 'i32',
    'i64', 'array_copy', 'array_length', 'arrays_equal', 'read', 'write',
    'sector_read', 'sector_write', 'exit',
}

# a program of each failed check of the README list, its options and target
# note: 'value' hides its argument from the constant folding
VALUE = 'func noinline value(n) res { res = n }\n'
DEPTH = '''func noinline depth(n) res {
    if n == 0 {
        res = 0
        return
    }
    res = depth(n - 1) + 1
}
func main() { exit(depth(100000) - 100000) }
'''
PROBES = {
    'bounds': (VALUE + 'func main() {\n    var a = i8[4]\n    a[value(4)] = 1\n}\n',
               ['--checks=upper'], 'x86_64'),
    'frame overflow': (DEPTH, ['--vars=0x400', '--checks=frame'], 'x86_64'),
    'division': (VALUE + 'func main() {\n    var d = value(0)\n    exit(1 / d)\n}\n',
                 ['--checks=division'], 'x86_64'),
    'shift': (VALUE + 'func main() {\n    var s = value(70)\n    exit(1 << s)\n}\n',
              ['--checks=shift'], 'x86_64'),
    'overlap': ('func main() {\n    var a = i32[8]\n    array_copy(a, a[1], 3)\n}\n',
                ['--checks=overlap'], 'x86_64'),
    'stack overflow': (DEPTH, ['--stack=0x100', '--checks=stack'], 'rv32i-qemu'),
    'overflow': (VALUE + 'func main() {\n    var b = value(1)\n    exit(0x7fffffffffffffff + b)\n}\n',
                 ['--checks=overflow'], 'x86_64'),
    'sd card': ('func main() {\n    var block = i8[512]\n    sector_read(0, block)\n}\n',
                ['--sdcard=missing.img'], 'x86_64'),
}


class Report:
    def __init__(self):
        self.checked = []
        self.skipped = []
        self.failures = []
        self.jobs = []

    def skip(self, where, reason):
        self.skipped.append((where, reason))

    # 'run' returns an error text or None, it runs on a worker thread
    def check(self, where, run):
        self.jobs.append((where, run))


def relative(path):
    return os.path.relpath(path, ROOT)


# compiles 'source' in 'work' and returns the exit code, stdout and stderr
def compile_source(work, source, options, target='x86_64'):
    path = os.path.join(work, 'example.baz')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(source)
    done = subprocess.run([BAZ, f'--target={target}', *options, path], cwd=work,
                          capture_output=True, text=True, errors='replace', timeout=60)
    return done.returncode, done.stdout, done.stderr


# compiles, builds and runs 'source', returns the exit code and the output or
# an error text
def run_source(work, source, options, target='x86_64'):
    rc, out, err = compile_source(work, source, options, target)
    if rc != 0:
        return None, f'does not compile:\n{err.strip()}'
    asm = os.path.join(work, 'example.s')
    exe = os.path.join(work, 'example')
    with open(asm, 'w', encoding='utf-8') as f:
        f.write(out)
    if target == 'x86_64':
        steps = [['nasm', '-w+error', '-f', 'elf64', asm, '-o', exe + '.o'],
                 ['ld', '-s', '-T', os.path.join(ROOT, 'baz.ld'), '-o', exe, exe + '.o']]
        command = [exe]
    elif target == 'rv32i':
        steps = [['llvm-mc', '-triple=riscv32', '-mattr=-m,-a,-f,-d,-c', '-filetype=obj',
                  asm, '-o', exe + '.o'],
                 ['ld.lld', '-m', 'elf32lriscv', '-e', '_start', '-o', exe, exe + '.o']]
        command = ['qemu-riscv32', exe]
    else:
        assert target == 'rv32i-qemu'
        steps = []
        command = ['qemu-system-riscv32', '-machine', 'virt', '-bios', 'none', '-display',
                   'none', '-serial', 'stdio', '-monitor', 'none', '-kernel',
                   os.path.join(work, 'example-rv32i-qemu.bin')]
    for step in steps:
        done = subprocess.run(step, cwd=work, capture_output=True, text=True, timeout=60)
        if done.returncode != 0:
            return None, f'{step[0]} failed:\n{done.stderr.strip()}'
    done = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL, capture_output=True,
                          text=True, errors='replace', timeout=60)
    return done.returncode, done.stdout + done.stderr


# the messages of an error output with their columns, a message continues on
# the lines indented to it
def error_messages(lines):
    messages = []
    indent = None
    for line in lines:
        m = ERROR_LINE.match(line)
        if m:
            messages.append([int(m.group(3)), m.group(4)])
            indent = len(line) - len(m.group(4))
        elif messages and indent and line.startswith(' ' * indent) and line[indent] != ' ':
            messages[-1][1] += ' ' + line[indent:]
        else:
            indent = None
    return [(column, ' '.join(text.split())) for column, text in messages]


def same_message(expected, actual):
    expected = ' '.join(expected.split())
    if expected.endswith(' ...'):
        return actual.startswith(expected[:-4])
    return actual == expected


# the lines of a document with their places, the README parts as one text
# since a fence opened in one part is closed in the next
def document_lines(paths):
    lines = []
    for path in paths:
        with open(path, encoding='utf-8') as f:
            text = f.read()
        lines += [((relative(path), n), line) for n, line in enumerate(text.split('\n'), 1)]
    return lines


# the fenced blocks of a document: file and line of the fence, lines, the
# heading of its chapter
def fenced_blocks(lines):
    blocks = []
    section = ''
    i = 0
    while i < len(lines):
        if lines[i][1].startswith('## '):
            section = lines[i][1]
        m = re.match(r'^( *)```', lines[i][1])
        if not m:
            i += 1
            continue
        end = i + 1
        while not lines[end][1].strip().startswith('```'):
            end += 1
        indent = len(m.group(1))
        body = [l[indent:] for _, l in lines[i + 1:end]]
        blocks.append((lines[i][0], body, section))
        i = end + 1
    return blocks


# splits the lines of a block into code and the notes right of it
def split_notes(lines):
    code_lines = []
    notes = []
    note_column = None
    for line in lines:
        m = NOTE_GAP.search(line)
        if m and (note_column is None or m.end() < note_column):
            note_column = m.end()
    for line in lines:
        if note_column is not None and len(line) > note_column and line[:note_column].strip() == '' \
                and line[note_column] != ' ':
            # a note that continues the one above
            if notes:
                notes[-1] += ' ' + line.strip()
            continue
        m = NOTE_GAP.search(line)
        if m and note_column is not None and m.end() == note_column:
            code_lines.append(line[:m.start()])
            notes.append(line[m.end():].strip())
        else:
            code_lines.append(line.rstrip())
            notes.append('')
    return code_lines, notes


# the top-level declarations of a code text: name to text
def declarations(code_lines):
    found = {}
    current = None
    for line in code_lines:
        m = re.match(r'^(?:type|func|dat|var|let) (?:noinline )?(?:mut )?([A-Za-z_]\w*(?:\.\w+)?)', line)
        if m:
            current = m.group(1)
            found[current] = [line]
        elif current and line and (line[0] in ' \t}'):
            found[current].append(line)
        else:
            current = None
    return {name: '\n'.join(text) for name, text in found.items()}


def names_used(code):
    code = re.sub(r'"(?:[^"\\]|\\.)*"', '', code)
    code = re.sub(r"'(?:[^'\\]|\\.)*'", '', code)
    code = re.sub(r'#.*', '', code)
    # a field or method after '.' is not a name of its own
    code = re.sub(r'\.\s*[A-Za-z_]\w*', '', code)
    return set(re.findall(r'\b[A-Za-z_]\w*\b', code))


def as_program(code):
    if not any(TOP_LEVEL_ONLY.match(l) for l in code.split('\n')):
        body = '\n'.join('    ' + l if l else l for l in code.split('\n'))
        return 'func main() {\n' + body + '\n}\n'
    if not re.search(r'^func main\(', code, re.M):
        return code + '\nfunc main() {}\n'
    return code + '\n'


def expect_compile(source, target='x86_64'):
    def run(work):
        rc, _, err = compile_source(work, source, [], target)
        if rc != 0:
            return f'does not compile on {target}:\n{err.strip()}'
        return None
    return run


# 'expected' is a list of (column or None, message)
def expect_errors(source, expected, target='x86_64'):
    def run(work):
        rc, _, err = compile_source(work, source, [], target)
        if rc != 1:
            return f'compiles on {target}, expected an error'
        actual = error_messages(err.split('\n'))
        for (column, message), got in zip(expected, actual + [None] * len(expected)):
            if got is None or (column is not None and column != got[0]) \
                    or (message is not None and not same_message(message, got[1])):
                return f'expected {column}: "{message}" on {target}, got:\n{err.strip()}'
        return None
    return run


def check_block(report, place, lines, context):
    where = f'{place[0]}:{place[1]}'
    error_start = next((i for i, l in enumerate(lines) if ERROR_LINE.match(l)), len(lines))
    expected_errors = error_messages(lines[error_start:])
    code_lines, notes = split_notes(lines[:error_start])
    while code_lines and not code_lines[-1]:
        code_lines.pop()
    # a body written '{ ... }' is the one of an earlier block
    for i, line in enumerate(code_lines):
        m = ELIDED.match(line)
        if not m:
            continue
        if m.group(2) not in context:
            report.skip(where, f"elided and declared in no earlier block: '{m.group(2)}'")
            return
        code_lines[i:i + 1] = context[m.group(2)].split('\n')
        notes[i:i + 1] = [''] * len(context[m.group(2)].split('\n'))
    code = '\n'.join(code_lines)
    if '...' in code:
        report.skip(where, "elided code '...'")
        return
    own = declarations(code_lines)
    missing = names_used(code) - KEYWORDS - set(own) - {'main'}
    # the declarations of earlier blocks it needs, also through each other,
    # in the order they were declared
    needed = set()
    while True:
        more = {n for n in missing if n in context} - needed
        if not more:
            break
        needed |= more
        for name in more:
            missing |= names_used(context[name]) - KEYWORDS
    prelude = [text for name, text in context.items() if name in needed]
    statement_list = not any(TOP_LEVEL_ONLY.match(l) for l in code_lines)
    if statement_list:
        undeclared = {n for n in missing - set(context) if not declared_in(code, n)}
        if undeclared:
            report.skip(where, 'names not declared in it: ' + ', '.join(sorted(undeclared)))
            return
    if any(NOTE_MESSAGE.search(note) for note in notes):
        parts = paragraphs(code_lines, notes)
        plain = [p for p in parts if not any(p[2])]
        for first, part_code, part_notes in parts:
            # note: +1 because the block starts below its fence
            part_where = f'{place[0]}:{place[1] + 1 + first}'
            message = next((m.group(1) for n in part_notes for m in [NOTE_MESSAGE.search(n)] if m),
                           None)
            if message is None:
                if any(part_notes):
                    report.skip(part_where, 'a note without a quoted message')
                continue
            text = '\n'.join(prelude + [p[1] for p in plain] + [part_code])
            report.check(part_where, expect_errors(as_program(text), [(None, message)]))
        return
    source = as_program('\n'.join(prelude + [code]))
    if expected_errors:
        report.check(where, expect_errors(source, expected_errors))
        return
    report.check(where, expect_compile(source))
    for name, text in own.items():
        context.pop(name, None)
        context[name] = text


def declared_in(code, name):
    return re.search(rf'\b(?:var|let) {re.escape(name)}\b', code) is not None


# the parts of a block between blank lines: first line offset, code, notes
def paragraphs(code_lines, notes):
    parts = []
    start = None
    for i, line in enumerate(code_lines + ['']):
        if line and start is None:
            start = i
        elif not line and start is not None:
            parts.append((start, '\n'.join(code_lines[start:i]), notes[start:i]))
            start = None
    return parts


def check_doc_blocks(report):
    for paths in (DOCS[:-1], DOCS[-1:]):
        context = {}
        last_section = None
        for place, lines, section in fenced_blocks(document_lines(paths)):
            where = f'{place[0]}:{place[1]}'
            if section != last_section:
                # an earlier chapter declares its own names
                context = {}
                last_section = section
            first = next((l for l in lines if l.strip()), None)
            if first is None:
                report.skip(where, "empty, 'etc/readme/make.sh' fills it")
                continue
            if not DECLARATION.match(first.lstrip()):
                if ERROR_LINE.match(first):
                    report.skip(where, 'error output without its program')
                else:
                    report.skip(where, 'not baz code')
                continue
            check_block(report, place, lines, context)


# "'CODE' is rejected on rv32i" and the like in the prose
INLINE = re.compile(r"'([^']+)' (?:is (rejected|accepted)(?: on (x86_64|rv32i)\b)?|gives \"([^\"]*)\")")


# a declaration of a variable, an assignment or a call
STATEMENT = re.compile(r'(?:var|let) \w+ = |[\w.\[\]]+ = |\w+\(.*\)$')


def check_inline(report):
    for path in DOCS:
        with open(path, encoding='utf-8') as f:
            text = f.read()
        # the fenced blocks are checked on their own
        text = re.sub(r'```.*?```', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
        if path != DOCS[-1]:
            # the README quotes code with backticks
            text = text.replace('`', "'")
        offset = 0
        for paragraph in re.split(r'(\n\s*\n)', text):
            joined = ' '.join(paragraph.split())
            for m in INLINE.finditer(joined):
                code, verdict, target, message = m.groups()
                # the line where the code starts
                start = offset + max(paragraph.find("'" + code.split()[0]), 0)
                where = f'{relative(path)}:{text.count(chr(10), 0, start) + 1} {code!r}'
                undeclared = names_used(code) - KEYWORDS
                undeclared = {n for n in undeclared if not declared_in(code, n)}
                if undeclared:
                    report.skip(where, 'names not declared in it: ' + ', '.join(sorted(undeclared)))
                    continue
                if '...' in code:
                    report.skip(where, "elided code '...'")
                    continue
                if not STATEMENT.match(code):
                    report.skip(where, "not a statement, e.g. a type or an expression")
                    continue
                source = as_program(code)
                if verdict == 'accepted':
                    report.check(where, expect_compile(source))
                elif target is None:
                    report.check(where, expect_errors(source, [(None, message)]))
                else:
                    # the text tells a difference between the targets
                    other = 'rv32i' if target == 'x86_64' else 'x86_64'
                    report.check(where, expect_errors(source, [(None, message)], target))
                    report.check(f'{where} on {other}', expect_compile(source, other))
            offset += len(paragraph)


# "'CODE' is a compile error" in a comment right below code
TUTORIAL_INLINE = re.compile(r"'([^']+)' is (?:a compile error|an error)")


def check_tutorial_claims(report):
    with open(TUTORIAL, encoding='utf-8') as f:
        lines = f.read().split('\n')
    i = 0
    while i < len(lines):
        # a commented-out code block followed by 'compile error'
        if lines[i].startswith('#   ') and (i == 0 or not lines[i - 1].startswith('#   ')):
            end = i
            while end < len(lines) and lines[end].startswith('#   '):
                end += 1
            block = [l[4:] for l in lines[i:end]]
            reason = next((k for k, l in enumerate(block) if l.startswith('compile error')), None)
            where = f'{relative(TUTORIAL)}:{i + 1}'
            if reason is None:
                report.skip(where, "commented-out code not followed by 'compile error'")
            else:
                source = lines[:i] + block[:reason] + [''] * (end - i - reason) + lines[end:]
                report.check(where, expect_error_at('\n'.join(source), i + 1, i + reason))
            i = end
            continue
        if lines[i].lstrip().startswith('#') and not lines[i - 1].lstrip().startswith('#'):
            end = i
            while end < len(lines) and lines[end].lstrip().startswith('#'):
                end += 1
            comment = ' '.join(l.lstrip()[1:].strip() for l in lines[i:end])
            above = lines[i - 1]
            for m in TUTORIAL_INLINE.finditer(comment):
                where = f'{relative(TUTORIAL)}:{i + 1} {m.group(1)!r}'
                if not above.strip() or above.strip() == '}':
                    report.skip(where, 'not right below a line of code')
                    continue
                indent = above[:len(above) - len(above.lstrip())]
                source = lines[:i] + [indent + m.group(1)] + lines[i:]
                report.check(where, expect_error_at('\n'.join(source), i + 1, i + 1))
            i = end
            continue
        i += 1


# the first error is at a line from 'first' to 'last'
def expect_error_at(source, first, last):
    def run(work):
        rc, _, err = compile_source(work, source, [])
        if rc != 1:
            return 'compiles, expected an error'
        m = next((m for m in map(ERROR_LINE.match, err.split('\n')) if m), None)
        if m is None or not first <= int(m.group(2)) <= last:
            return f'expected an error at line {first}, got:\n{err.strip()}'
        return None
    return run


def check_panics(report):
    with open(README_FIRST, encoding='utf-8') as f:
        text = ' '.join(f.read().split())
    m = re.search(r'exits with the code of the check: (.*?) \*', text)
    assert m, "the list of exit codes is not found in 'README.1.md'"
    codes = [(name, int(code)) for name, code in re.findall(r'`([a-z ]+)` (\d+)', m.group(1))]
    codes += [(name, int(code)) for name, code in
              re.findall(r'prints `panic: ([a-z ]+)` and exits with (\d+)', m.group(1))]
    for name, code in codes:
        where = f'{relative(README_FIRST)} panic {name!r}'
        if name not in PROBES:
            report.failures.append(f'{where}: no probe program in this check')
            continue
        source, options, target = PROBES[name]
        report.check(where, expect_panic(source, options, target, name, code))
    # with 'line' the panic names the file and the line
    source, options, target = PROBES['bounds']
    report.check(f'{relative(README_FIRST)} panic with line',
                 expect_panic(source, ['--checks=upper,line'], target, 'bounds', 255,
                              ' at example.baz:4'))


def expect_panic(source, options, target, name, code, suffix=''):
    def run(work):
        rc, output = run_source(work, source, options, target)
        if rc is None:
            return output
        message = f'panic: {name}{suffix}'
        if rc != code or not re.search(rf'^{re.escape(message)}$', output, re.M):
            return f"expected '{message}' and exit code {code}, got {rc}:\n{output.strip()}"
        return None
    return run


def check_programs(report):
    with open(TUTORIAL, encoding='utf-8') as f:
        tutorial = f.read()
    with open(PROG, encoding='utf-8') as f:
        prog = f.read()
    for name, source, target in ((TUTORIAL, tutorial, 'x86_64'), (TUTORIAL, tutorial, 'rv32i'),
                                 (PROG, prog, 'x86_64')):
        report.check(f'{relative(name)} on {target}', expect_exit_zero(source, target))


def expect_exit_zero(source, target):
    def run(work):
        rc, output = run_source(work, source, RUN_OPTIONS, target)
        if rc != 0:
            return output if rc is None else f'exits with {rc}:\n{output.strip()}'
        return None
    return run


def run_job(job):
    where, run = job
    with tempfile.TemporaryDirectory() as work:
        return where, run(work)


def main():
    verbose = '-v' in sys.argv[1:]
    if not os.access(BAZ, os.X_OK):
        print("error: 'baz' is missing, build it with './make.sh build'", file=sys.stderr)
        return 1
    report = Report()
    check_doc_blocks(report)
    check_inline(report)
    check_tutorial_claims(report)
    check_panics(report)
    check_programs(report)
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        for where, error in pool.map(run_job, report.jobs):
            report.checked.append(where)
            if error:
                report.failures.append(f'{where}: {error}')
    for failure in report.failures:
        print(failure)
    if verbose:
        for where in report.checked:
            print(f'checked {where}')
        for where, reason in report.skipped:
            print(f'skipped {where}: {reason}')
    print(f'checked {len(report.checked)}, skipped {len(report.skipped)} (-v lists them)')
    return 1 if report.failures else 0


if __name__ == '__main__':
    sys.exit(main())
