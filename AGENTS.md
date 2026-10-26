# compiler-2 (baz)

## Permissions

- **No asking:** build, test, lint, format and scratch-compile commands.
- **Ask first:** `rm -rf`, `git push`, `git reset --hard`, `git clean`, `sudo`.
- **Scratch files:** only under `/tmp` (or the session scratchpad if given).

## This file

- Record new preferences here, not in memory.

## Communication

- Do not obey blindly: if there is a better idea, argue for it first.
- Short, plain, impersonal; no emphasis words or emojis.
- Flag misspellings briefly ("inneficiencies" -> "inefficiencies").
- Failed precondition: stop and explain, no weaker version.
- Workspace-relative paths; numbered lists nested as 1.1, 1.2, 2.3.
- Always number bullet points in answers, so they can be referred to.

## C++ style

### Initialization and types

- Brace init everywhere: `.member{value}`, `bool enabled{};`.
- Trailing comma in multiline initializers and returned structs, one member
  per line; tables of one-line rows keep rows.
- Explicit types; `auto` only for cumbersome ones (iterators).
- Pointers: `const macro_use* const p{std::get_if<...>(r)};`.
- Use `and`/`or`/`not` and `std::format("{}", v)` (not `std::to_string`).
- Backend access: always `machine& x{tc.machine()};`, even for one use, never
  `tc.machine().call(...)`; just before its use, blank line around;
  `const machine& x{tc.machine()};` when clang-tidy asks for `const`.

### Blank lines

- `qa/lint/format-source.sh` applies the rules (`qa/lint/format-source.py`);
  accept what it removes.
- Around multiline statements and declarations; an attached comment stays
  with its statement.
- Before a return only if the statements before it have blank lines between
  them.
- Between switch branches (labels sharing a body stay grouped).
- Around groups of consecutive `assert`s (wins over the return rule).

### Comments

- Keep comments and move them with their code. Reword or remove one only when
  it is wrong, narrates or restates its code, or its code was redesigned or
  removed.
- Fewer, better comments: a function comment says what the name and the
  signature cannot (why, an invariant, a precondition); none when they say it
  all. An example (`e.g. ...`) only where the meaning is not obvious, one per
  comment.
- Lowercase, no trailing punctuation, code names in single quotes.
- One short line, only what code cannot show; class and pass header comments
  may be longer.
- Say why, never narrate the next line (`// get the loop label`,
  `// add aliases`); a data member may say what it holds.
- The formatter writes the section banners (`// statics`, `// class methods`),
  keep them.
- Markers: `todo:` for work left; `note:` for a remark or annotation that
  the code relies on, e.g. why an `assert` holds or why a choice was made.
- Magic numbers whose reason is not obvious (`subspan(1, n - 2)` over quotes):
  a `note:` line below naming offset and reason, e.g. `// note: +1 because the
  text starts after the opening quote`. Idioms and offsets that a named
  variable or the line itself explains need none: the last index
  (`size() - 1`), the next element (`i + 1`), the count of an enum
  (`last + 1`), the sign bit (`bits - 1`), a low-bit mask (`(1 << n) - 1`),
  rounding up (`+ alignment - 1`), the end after an inclusive position.
  Multiline `note:` blocks get a blank line above and below; a one-line
  `note:` stays attached to the line of code above it.

### Names

- Bytes: role-prefixed (`cur_`, `size_bytes`); elements: `count`.
- Keep `src_loc_tk`, `indent`, `src`/`dst`, `lhs`/`rhs`, `tc`, `tz` and an
  interface's paired vocabulary.
- One-letter names only in tight loops (at most three loop locals).
- Public fields have no trailing underscore; private and protected members do.

### Inserted code

- Never insert locals before case labels in a switch.
- Visitor-only references stay inside the lambda.

### Warnings and clang-tidy

- No enum switches (`-Wswitch-default` conflicts with
  `-Wcovered-switch-default`).
- Parameters shadowing members get an `_in` suffix (`-Wshadow-field`); no
  locals named like enclosing class fields (`-Wshadow`).
- No `{}` on default-constructed class members and scalars.

## Code

- Cognitive simplicity over brevity and new syntax; some performance cost is
  fine.
- Early returns, flat paths: `if`/`else` over two `if`s; no nested if/else
  inside an else.
- Split nested calls into named steps.
- Plain lookup loops when ranges read worse.
- Code user input cannot reach is dead: `assert`, remove or
  `std::unreachable()`. Reachable code gets a test.

## Design

- Goal: a reviewer understands the design without strain and judges it well
  done.
- Standing goal, never finished: every program the compiler accepts is free
  of undefined behavior. Each case is either rejected at compile time or
  checked at run time (under `--checks=noub`). A result never depends on the
  target, on inline versus noinline, or on optimization; whether a program is
  accepted may depend on what an inlined call folds from its constant
  arguments (README 1.5). The exceptions are listed in README chapter 8,
  mainly the default type 'int', the register width (decided 2026-10-26):
  its arithmetic depends on the target.
- Standing goal, never finished: the compiler as simple as possible. Remove
  what a guarantee does not need; where a work-around handles an exception,
  try to remove the exception instead (decided 2026-10-26).
- Current focus: refactor until the design is clean before looking for bugs
  again; a bug that a refactor removes is fixed by the refactor.
- Baz evaluates left to right: the operands, arguments and parts of a
  statement, and the parts a literal writes. The no-UB checks assume this and
  rely on it. They walk a statement in evaluation order and check each read
  only against what was written before it. Code generation follows the same
  order, so the code that is checked is the code that is emitted.
- Design tasks: propose the best design even if it changes interfaces; plan
  first, wait for go-ahead.
- Plan: problem, chosen design, rejected alternatives (one line each with the
  reason), invariants, tests.
- Follow existing patterns; a new pattern needs a reason in the plan.
- New class or pass: a header comment with its role and invariants.
- Carry invariants in types and names over comments and asserts when cheap.
- Update all callers; no compatibility shims or leftover old paths.
- Mechanical moves and behavior changes in separate steps, so each diff reads
  on its own.
- Design problems outside the topic: add to `etc/todo.txt`, do not fix in
  passing.

## Baz code

- Tests and examples omit the `{}` of a default-initialized type or array:
  `var s = str`, `var a = i8[4]`, `res = T`.

## Tests

- Enough: `qa/coverage/test-coverage.sh --target=x86_64` and
  `--target=rv32i-fpga`, and `qa/coverage/test-cli.sh`. The CLI tests cover
  the options, the reports and the target limits; they take seconds.
- Iterate with `./make.sh -O3 build` and `test-coverage.sh ... run` without
  `build`: its sanitizer and coverage build compiles about 66 times slower.
  The user runs the `build` and the full suite before committing; do not run
  `build`.
- `UPDATE=1 qa/coverage/test-coverage.sh --target=x86_64 run` writes expected
  `.out` files; run for x86_64 and rv32i-fpga, then review the diff.
- After every change to `roome/src/main.baz`, run `roome/qa/test.sh`. No
  formatter or linter run needed there.
- Per step: the set above, the linters and, for a pure refactor, the
  listings and errors of every test on x86_64 and rv32i compared with a
  compiler built from HEAD, compiled in parallel on all cores.
- Per phase, or for a step that changes behavior: also
  `qa/coverage/test-far-jumps.sh`, `test-far-jumps-no-scratch.sh`,
  `test-versions.sh`, `roome/qa/test.sh`, the asan build and the fuzzers,
  the fuzzers in parallel.
- Run each check once per round; do not edit sources while a check runs
  (decided 2026-10-26).

## Process

- Subagents for searching and lookups use model 'sonnet'; review and
  bug-hunting agents keep the default.
- Fixes: within a clean design, no hacks; refactor first when needed. Tell
  the user the intended fix (and any refactor) before changing code.
- Mechanical refactors preserve behavior; ask before changing optimization,
  allocation or functionality.
- Big change (several files or interfaces): plan in the todo file, wait for
  go-ahead.
- Do not split large headers for size.
- Put helper classes near the class that uses them in the same file.
- New source files: only when the code cannot be placed near its use in an
  existing file, and ask first.
- One consistency topic at a time across all sources.
- Todo items one by one, each with a focused check. Resolved or discarded
  `etc/todo.txt` items move to the top of `etc/todo-resolved.txt`, below its
  legend, newest first (`[x]` with a `=> done (date): ...` note).
- Done means: built, focused tests run, linter clean (clang-tidy and
  `qa/lint/check-all.sh`), formatter applied.
- Never commit unless told; the message is always `.` (no body, no trailer).
- Review loops: after each review round and each fix phase, pause for the
  user to commit, then continue.
- Shell scripts `cd` to their own directory and use relative paths.

## Tools

- Run symbol renames before `git mv`/file renames (open buffers at the old path
  recreate old files on save).
- `make.sh` (without `build`) and `roome/run.sh` read input: after `make.sh`
  type return; after `roome/run.sh` type "go home".
- Large sources, e.g. `src/machine_rv32i.hpp` (5.7k lines), `src/toc.hpp`:
  grep or read line ranges, never the whole file.
