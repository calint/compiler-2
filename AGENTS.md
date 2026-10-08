# compiler-2 (baz)

Working agreements for AI sessions in this repository.

## Permissions

- **No asking needed:** shell commands for build, test, lint, format and
  scratch compiles.
- **Ask first:** `rm -rf`, `git push`, `git reset --hard`, `git clean`, `sudo`.
- **Scratch files:** only under `/tmp`; use the session scratchpad if given.

## This file

- Read and follow it every session.
- Record new preferences here, not in memory.
- Keep it human-readable: headings, short bullets, blank lines between
  sections.

## Communication

- Do not obey blindly: when there is a better idea, argue for it first.
- Short, plain, impersonal; no emphasis words or emojis.
- Flag misspellings briefly ("inneficiencies" -> "inefficiencies") and spell
  correctly in names and files.
- Report what was and was not verified, and why.
- Failed precondition: stop and explain, no weaker version.
- Use workspace-relative paths.
- Lists in replies are numbered, not bulleted.

## C++ style

### Initialization and types

- Brace init everywhere: `.member{value}`, `bool enabled{};`.
- Trailing comma in multiline initializers, designated initializers and
  returned structs (one member per line); tables of one-line rows keep their
  rows.
- Empty optional: `return std::nullopt;`.
- Explicit types; `auto` only for cumbersome types (iterators).
- Backend access: always `machine& x{tc.machine()};`, even for one use, never
  `tc.machine().call(...)`; blank line before and after, placed just before
  its use.
- Pointers: `const macro_use* const p{std::get_if<...>(r)};`.
- Use `and`/`or`/`not`.
- Use `std::format("{}", v)` over `std::to_string`.

### Blank lines

- `qa/lint/format-source.sh` applies the rules (class member order, then
  clang-format); `qa/lint/format-source.py` holds them. Accept what the
  formatter removes.
- Around multiline statements and declarations, keeping an attached comment or
  note with its statement.
- After the `{` of a multiline signature, of a multiline lambda head and of a
  multiline condition (the formatter applies it after the last clang-format
  run, which removes it from lambda bodies).
- Before a return only when the statements before it have blank lines between
  them.
- Between switch branches (labels sharing a body stay grouped).
- Around groups of consecutive `assert`s (wins over the return rule).

### Comments

- Never delete or reword an existing comment; move it with its code. Change one
  only when it is wrong.
- Lowercase, no trailing punctuation, code names in single quotes.
- One short line, only what code cannot show; never restate code or address the
  reviewer.
- Non-obvious new code: brief rationale (why, not what).
- Use the `todo:` marker.
- Magic numbers (`+ 1`, `subspan(1, n - 2)`) in new or touched code: a `note:`
  on the line below naming the offset and reason, e.g. `// note: +1 because the
  text starts after the opening quote`.
- Multiline `note:` blocks get a blank line above and below.

### Names

- Bytes: role-prefixed, e.g. `cur_`, `size_bytes`. Elements: `count`.
- Keep `src_loc_tk`, `indent`, `src`/`dst`, `lhs`/`rhs`, `tc`.
- One-letter names only in tight loops (at most three loop locals).
- Same parameter names across declarations, definitions and overrides.
- Keep an interface's paired vocabulary.
- Public fields of structs and classes have no trailing underscore; private
  and protected members do.

### Inserted code

- Never insert locals before case labels in a switch.
- Visitor-only references stay inside the lambda.

### Compiler warnings and clang-tidy

- No enum switches (`-Wswitch-default` conflicts with
  `-Wcovered-switch-default`).
- `std::in_range` needs `int`, not `char`.
- Parameters shadowing members get an `_in` suffix (`-Wshadow-field`).
- No locals named like enclosing class fields (`-Wshadow`).
- No `{}` on default-constructed class members (`std::vector`, `std::bitset`);
  scalars keep `{}`.
- Overrides keep the base's visibility.

## Code

- Cognitive simplicity over brevity and new syntax; some performance cost is
  fine.
- Multi-pass designs (one rule per pass, with a before/after example comment)
  over single-pass state machines.
- Early returns and flat paths: `if`/`else` over two `if`s, flat `else if`
  chains are fine, no nested if/else inside an else.
- Duplicates become a named helper; split nested calls into named steps.
- Plain lookup loops when ranges read worse.
- Preserve behavior and cleanup when restructuring.
- Code that user input cannot reach is dead: make it an `assert`, remove it or
  use `std::unreachable()`; reachable code gets a test.

## Baz code

- Tests and examples: omit the `{}` of a default-initialized type or array; the
  compiler assumes it: `var s = str`, `var a = i8[4]`, `res = T`.

## Tests

- Test runs: `--target=x86` and `--target=rv32i-fpga` are enough.
- After every change to `roome/src/main.baz`, run `roome/test.sh` and
  validate that nothing broke.
- Work on `roome/src/main.baz` needs no formatter or linter run; they act on
  the compiler source, not on the application.
- New error test: check that each `line:column` in its `.out` points at the
  token where the error is detected (print the source line with a caret), not
  only that the message reads well.
- `UPDATE=1 qa/coverage/test-coverage.sh --target=x86 run` writes expected
  outputs (`.out`) instead of comparing; run it for x86 and rv32i, then review
  the diff.

## Process

- Mechanical refactors preserve behavior; ask before changing optimization,
  allocation or functionality.
- Big change (several files or interfaces): plan in the todo file, wait for
  go-ahead.
- Large headers are fine: the classes `toc` uses stay in `toc.hpp`; do not
  split a header into new files for size.
- One consistency topic at a time across all sources.
- Todo items one by one, each with a focused check.
- Resolved or discarded items of `etc/todo.txt` move to the top of
  `etc/todo-resolved.txt`, below its legend, newest first (`[x]` with a
  `=> done (date): ...` note); never delete them.
- Done means: built, focused tests run, linter clean, formatter applied (keep
  its output).
- Never commit; the user commits. Only when told to, the message is always `.`
  (no body, no trailer).
- Live test output: no redirection or piping through `grep`/`tail`/`head`.
- Shell scripts `cd` to their own directory and use relative paths.

## Tools

- Run symbol renames before `git mv`/file renames (open buffers at the old
  path recreate old files on save).
- `make.sh` (without `build`) and `run-roome.sh` read input: after `make.sh`
  type return, after `run-roome.sh` type "go home".
