# compiler-2 (baz)

## Permissions

- **No asking:** build, test, lint, format and scratch-compile commands.
- **Scratch files:** only under `/tmp` (or the session scratchpad if given).
- Record new preferences in this file, not in memory.

## Communication

- Do not obey blindly: if there is a better idea, argue for it first.
- Short, plain, impersonal; no emphasis words or emojis.
- Flag misspellings briefly ("inneficiencies" -> "inefficiencies").
- Failed precondition: stop and explain, no weaker version.
- Workspace-relative paths; every list in an answer is numbered, nested as
  1.1, 1.2, so its items can be referred to.

## C++ style

### Initialization and types

- Every member and variable is brace-initialized: `bool done{};`,
  `.member{value}`; the only exception is a class type whose default
  constructor initializes it (`std::string name;`).
- Trailing comma in multiline initializers and returned structs, one member
  per line; tables of one-line rows keep rows.
- Explicit types; `auto` only for cumbersome ones (iterators).
- Pointers: `const macro_use* const p{std::get_if<...>(r)};`.
- Use `and`/`or`/`not` and `std::format("{}", v)` (not `std::to_string`).
- Backend access: `machine& x{tc.machine()};` (or `const`), even for one use,
  just before its use with a blank line around; never `tc.machine().call()`.

### Blank lines

`qa/lint/format-source.sh` applies these; accept what it removes.

- Around multiline statements and declarations; an attached comment stays
  with its statement.
- Before a return only if the statements before it have blank lines between
  them.
- Between switch branches (labels sharing a body stay grouped).
- Around groups of consecutive `assert`s (wins over the return rule).

### Comments

- Keep comments and move them with their code; reword or remove one only when
  it is wrong, restates its code, or its code was redesigned or removed.
- Fewer, better: a comment says what names and signatures cannot (why, an
  invariant, a precondition). At most one `e.g.` example, only when needed.
- Lowercase, no trailing punctuation, code names in single quotes; one short
  line; class and pass header comments (role and invariants, required for a
  new one) may be longer.
- Keep the section banners the formatter writes (`// statics`).
- Markers: `todo:` for work left; `note:` for what the code relies on (why an
  `assert` holds, why a choice was made).
- A magic number whose reason is not obvious gets a `note:` line below it
  (`// note: +1 because the text starts after the opening quote`); common
  idioms (`size() - 1`, `i + 1`, masks, rounding up) need none. Multiline
  `note:` blocks get a blank line above and below.

### Names

- Bytes: role-prefixed (`cur_`, `size_bytes`); elements: `count`.
- Keep `src_loc_tk`, `indent`, `src`/`dst`, `lhs`/`rhs`, `tc`, `tz` and an
  interface's paired vocabulary.
- One-letter names only in tight loops.
- Public fields have no trailing underscore; private and protected members do.
- Parameters shadowing members get an `_in` suffix; no locals named like
  enclosing class fields.

### Code

- Cognitive simplicity over brevity and new syntax; some performance cost is
  fine.
- Early returns, flat paths: `if`/`else` over two `if`s; no nested if/else
  inside an else.
- Split nested calls into named steps.
- Visitor-only references stay inside the lambda.
- No enum switches (`-Wswitch-default` conflicts with
  `-Wcovered-switch-default`).
- Code user input cannot reach is dead: `assert`, remove or
  `std::unreachable()`. Reachable code gets a test.

## Design

- Standing goal: every program accepted is free of undefined behavior when
  compiled with `--checks=noub`; each case is rejected at compile time or
  checked at run time. A result never depends on the target, inlining or
  optimization. Exceptions: README chapter 8 (mainly the default type 'int').
- Standing goal: the compiler as simple as possible. Remove what a guarantee
  does not need; remove an exception rather than work around it. Never remove
  a feature (options, reports, optimizations, versions). Growth of the
  generated code from a simplification is measured and reported, not reverted.
- No scratch register unless the order of reads needs one, the destination
  cannot hold the value, or a measured version is shorter.
- Baz evaluates left to right; the no-UB checks and code generation follow the
  same order, so the code that is checked is the code that is emitted.
- The first element of an expression decides its type: '1 + k' has the
  default type, 'k + 1' the type of 'k'.

## Baz code

- Tests and examples omit the `{}` of a default-initialized declaration:
  `var s = str`, `var a = i8[4]`; an assigned zero value is `{}`: `res = {}`.

## Tests

- Iterate with `./make.sh -O3 build` and `test-coverage.sh ... run`; do not
  run `test-coverage.sh build` (about 66 times slower), the user does.
- Per step: `qa/coverage/test-coverage.sh --target=x86_64 run`, then
  `--target=rv32i-fpga`, then `qa/coverage/test-cli.sh` (one after the other,
  they share `gen.s`); the linters; for a pure refactor, the listings and
  errors of every test on x86_64 and rv32i compared with a build of HEAD.
- Per phase or behavior change, also `test-far-jumps.sh`,
  `test-far-jumps-no-scratch.sh`, `test-versions.sh`, `roome/qa/test.sh`, the
  asan build and the fuzzers (in a bug hunt round: asan and fuzzers once, at
  the end of the round).
- Run each check once per round; do not edit sources while a check runs.
- `UPDATE=1` with `test-coverage.sh` writes expected `.out` files; run it for
  both targets and review the diff.
- After a change to `roome/src/main.baz`, run `roome/qa/test.sh`.

## Process

- Search and lookup subagents use model 'sonnet'; review and bug-hunting
  agents keep the default.
- Fixes: within a clean design, no hacks; refactor first when needed. Tell the
  user the intended fix before changing code.
- Do not split large headers; helper classes go near their user in the same
  file; ask before adding a source file.
- One consistency topic at a time across all sources.
- Todo items one by one; resolved or discarded ones move to the top of
  `etc/todo-resolved.txt` below its legend (`[x]`, `=> done (date): ...`).
- Done means: built, focused tests run, clang-tidy and `qa/lint/check-all.sh`
  clean, formatter applied.
- After each step or round, pause for the user to commit (message `.`). In
  design review rounds the next round's reviewers may start meanwhile; their
  patches are applied after the commit.
- Remove a round's agent worktrees and branches when done (check they hold no
  commits).
- Shell scripts `cd` to their own directory and use relative paths.

## Tools

- Run symbol renames before file renames (open buffers recreate old files).
- `make.sh` (without `build`) and `roome/run.sh` read input: type return,
  respectively "go home".
