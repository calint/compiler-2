# compiler-2 (baz)

Working agreements for AI sessions in this repository.

## Permissions

- **No asking:** build, test, lint, format and scratch-compile commands.
- **Ask first:** `rm -rf`, `git push`, `git reset --hard`, `git clean`, `sudo`.
- **Scratch files:** only under `/tmp` (or the session scratchpad if given).

## This file

- Read and follow it every session; record new preferences here, not in
  memory.
- Keep it human-readable: headings, short bullets, blank lines between
  sections.

## Communication

- Do not obey blindly: if there is a better idea, argue for it first.
- Short, plain, impersonal; no emphasis words or emojis.
- Flag misspellings briefly ("inneficiencies" -> "inefficiencies"); spell
  names and files correctly.
- Report what was and was not verified, and why.
- Failed precondition: stop and explain, no weaker version.
- Workspace-relative paths; numbered lists, not bulleted, nested as 1.1,
  1.2, 2.3 so items can be referenced.

## C++ style

### Initialization and types

- Brace init everywhere: `.member{value}`, `bool enabled{};`.
- Trailing comma in multiline initializers, designated initializers and
  returned structs (one member per line); tables of one-line rows keep rows.
- Empty optional: `return std::nullopt;`.
- Explicit types; `auto` only for cumbersome ones (iterators).
- Everything that can be `const` to be `const`
- Pointers: `const macro_use* const p{std::get_if<...>(r)};`.
- Use `and`/`or`/`not` and `std::format("{}", v)` (not `std::to_string`).
- Backend access: always `machine& x{tc.machine()};`, even for one use, never
  `tc.machine().call(...)`; placed just before its use, blank line around.

### Blank lines

- `qa/lint/format-source.sh` applies the rules (class member order, then
  clang-format); `qa/lint/format-source.py` holds them. Accept what it removes.
- Around multiline statements and declarations; an attached comment or note
  stays with its statement.
- After the `{` of a multiline signature, lambda head or condition
- Before a return only if the statements before it have blank lines between
  them.
- Between switch branches (labels sharing a body stay grouped).
- Around groups of consecutive `assert`s (wins over the return rule).

### Comments

- Never delete or reword a comment; move it with its code. Change one only when
  it is wrong.
- Lowercase, no trailing punctuation, code names in single quotes.
- One short line, only what code cannot show; never restate code or address the
  reviewer. Non-obvious new code: brief rationale (why, not what).
- Marker: `todo:`.
- Magic numbers (`+ 1`, `subspan(1, n - 2)`) in new or touched code: a `note:`
  line below naming offset and reason, e.g. `// note: +1 because the text
  starts after the opening quote`. Multiline `note:` blocks get a blank line
  above and below.

### Names

- Bytes: role-prefixed (`cur_`, `size_bytes`); elements: `count`.
- Keep `src_loc_tk`, `indent`, `src`/`dst`, `lhs`/`rhs`, `tc`, `tz` and an
  interface's paired vocabulary.
- One-letter names only in tight loops (at most three loop locals).
- Same parameter names across declarations, definitions and overrides.
- Public fields have no trailing underscore; private and protected members do.

### Inserted code

- Never insert locals before case labels in a switch.
- Visitor-only references stay inside the lambda.

### Warnings and clang-tidy

- No enum switches (`-Wswitch-default` conflicts with
  `-Wcovered-switch-default`).
- Parameters shadowing members get an `_in` suffix (`-Wshadow-field`); no
  locals named like enclosing class fields (`-Wshadow`).
- No `{}` on default-constructed class members and scalars
- Overrides keep the base's visibility.

## Code

- Cognitive simplicity over brevity and new syntax; some performance cost is
  fine.
- Multi-pass designs (one rule per pass, with a before/after example comment)
  over single-pass state machines.
- Early returns, flat paths: `if`/`else` over two `if`s; flat `else if` chains
  are fine; no nested if/else inside an else.
- Duplicates become a named helper; split nested calls into named steps.
- Plain lookup loops when ranges read worse.
- Preserve behavior and cleanup when restructuring.
- Code user input cannot reach is dead: `assert`, remove or
  `std::unreachable()`. Reachable code gets a test.

## Baz code

- Tests and examples omit the `{}` of a default-initialized type or array (the
  compiler assumes it): `var s = str`, `var a = i8[4]`, `res = T`.

## Tests

- Enough: `qa/coverage/test-coverage.sh --target=x86` (script alias of
  `x86_64`; the compiler takes `x86_64`) and `--target=rv32i-fpga`.
- New functionality and bug fixes get a test that fails without the change,
  when possible.
- New error test: check each `line:column` in its `.out` points at the token
  where the error is detected (print the source line with a caret), not only
  that the message reads well.
- `UPDATE=1 qa/coverage/test-coverage.sh --target=x86 run` writes expected
  `.out` files instead of comparing; run for x86 and rv32i, then review the
  diff.
- After every change to `roome/src/main.baz`, run `roome/qa/test.sh` and check
  nothing broke. No formatter or linter run needed there (they act on compiler
  source only).

## Process

- Mechanical refactors preserve behavior; ask before changing optimization,
  allocation or functionality.
- Big change (several files or interfaces): plan in the todo file, wait for
  go-ahead.
- Large headers are fine: classes `toc` uses stay in `toc.hpp`; do not split
  for size.
- One consistency topic at a time across all sources.
- Todo items one by one, each with a focused check. Resolved or discarded
  `etc/todo.txt` items move to the top of `etc/todo-resolved.txt`, below its
  legend, newest first (`[x]` with a `=> done (date): ...` note); never delete
  them.
- Done means: built, focused tests run, linter clean, formatter applied (keep
  its output).
- Never commit; the user does. Only when told to: message is always `.` (no
  body, no trailer).
- Shell scripts `cd` to their own directory and use relative paths.

## Tools

- Run symbol renames before `git mv`/file renames (open buffers at the old path
  recreate old files on save).
- `make.sh` (without `build`) and `roome/run.sh` read input: after `make.sh`
  type return; after `roome/run.sh` type "go home".
