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

## C++ style

### Initialization and types

- Brace init everywhere: `.member{value}`, `bool enabled{};`.
- Trailing comma in multiline initializers and returned structs, one member
  per line; tables of one-line rows keep rows.
- Explicit types; `auto` only for cumbersome ones (iterators).
- Pointers: `const macro_use* const p{std::get_if<...>(r)};`.
- Use `and`/`or`/`not` and `std::format("{}", v)` (not `std::to_string`).
- Backend access: always `machine& x{tc.machine()};`, even for one use, never
  `tc.machine().call(...)`; just before its use, blank line around.

### Blank lines

- `qa/lint/format-source.sh` applies the rules (`qa/lint/format-source.py`);
  accept what it removes.
- Around multiline statements and declarations; an attached comment stays
  with its statement.
- After the `{` of a multiline signature, lambda head or condition.
- Before a return only if the statements before it have blank lines between
  them.
- Between switch branches (labels sharing a body stay grouped).
- Around groups of consecutive `assert`s (wins over the return rule).

### Comments

- Never delete or reword a comment; move it with its code. Change one only when
  it is wrong.
- Lowercase, no trailing punctuation, code names in single quotes.
- One short line, only what code cannot show (why, not what).
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
- Multi-pass designs (one rule per pass, with a before/after example comment)
  over single-pass state machines.
- Early returns, flat paths: `if`/`else` over two `if`s; no nested if/else
  inside an else.
- Split nested calls into named steps.
- Plain lookup loops when ranges read worse.
- Code user input cannot reach is dead: `assert`, remove or
  `std::unreachable()`. Reachable code gets a test.

## Baz code

- Tests and examples omit the `{}` of a default-initialized type or array:
  `var s = str`, `var a = i8[4]`, `res = T`.

## Tests

- Enough: `qa/coverage/test-coverage.sh --target=x86_64` and
  `--target=rv32i-fpga`.
- New error test: check each `line:column` in its `.out` points at the token
  where the error is detected (print the source line with a caret).
- `UPDATE=1 qa/coverage/test-coverage.sh --target=x86_64 run` writes expected
  `.out` files; run for x86_64 and rv32i-fpga, then review the diff.
- After every change to `roome/src/main.baz`, run `roome/qa/test.sh`. No
  formatter or linter run needed there.

## Process

- Mechanical refactors preserve behavior; ask before changing optimization,
  allocation or functionality.
- Big change (several files or interfaces): plan in the todo file, wait for
  go-ahead.
- Do not split large headers for size.
- Put helper classes near the class that uses them in the same file.
- One consistency topic at a time across all sources.
- Todo items one by one, each with a focused check. Resolved or discarded
  `etc/todo.txt` items move to the top of `etc/todo-resolved.txt`, below its
  legend, newest first (`[x]` with a `=> done (date): ...` note).
- Done means: built, focused tests run, linter clean, formatter applied.
- Never commit unless told; the message is always `.` (no body, no trailer).
- Shell scripts `cd` to their own directory and use relative paths.

## Tools

- Run symbol renames before `git mv`/file renames (open buffers at the old path
  recreate old files on save).
- `make.sh` (without `build`) and `roome/run.sh` read input: after `make.sh`
  type return; after `roome/run.sh` type "go home".
