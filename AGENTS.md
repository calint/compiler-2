# compiler-2 (baz)

Working agreements for AI sessions in this repository.

## Permissions

- **No asking needed:** shell commands for build, test, lint, format and
  scratch compiles.
- **Ask first:** `rm -rf`, `git push`, `git reset --hard`, `git clean`, `sudo`.
- **Scratch files:** only in `/tmp`.
- **Workspace root:** `/home/c/w/compiler-2`.

## This file

- Read and follow it every session; it replaces the memory file
  `preferences.md`.
- Record new preferences here, not in memory.
- Keep it formatted for human readers: headings, short bullets, sub-bullets
  for lists, blank lines between sections.

## Communication

- Short, plain, impersonal; no emphasis words or emojis.
- Flag misspellings briefly ("inneficiencies" -> "inefficiencies") and spell
  correctly in names and files.
- Report what was and was not verified, and why.
- Failed precondition: stop and explain, no weaker version.
- Use workspace-relative paths.
- Lists in replies are numbered, not bulleted, so items can be referred to by
  number.

## C++ style

### Initialization and types

- Brace init everywhere: `.member{value}`, `bool enabled{};`.
- Trailing comma in multiline initializers.
- Empty optional: `return std::nullopt;`.
- Explicit types; `auto` only for cumbersome types (iterators).
- Backend access: always `machine& x{tc.machine()};`, even for one use, never
  `tc.machine().call(...)`; a blank line before and after it, placed just
  before its use.
- Pointers: `const macro_use* const p{std::get_if<...>(r)};`.
- Use `and`/`or`/`not`.
- Use `std::format("{}", v)` over `std::to_string`.

### Blank lines

- Around multiline statements and declarations: a blank line before and after,
  in all source; a comment above or a note below stays with its statement.
  `qa/lint/format-source.sh` applies them with the class member order and then
  runs clang-format (`qa/lint/format-source.py` holds the rules, one rule is a
  function in its blank line section).
- Before and after a multiline `if`, after a multiline `for`, `while`,
  `switch` or `try`, so statements have some space between them (the script
  covers it).
- After the `{` of a multiline signature.
- Before a return when a gap between the statements before it has a blank
  line; a block without blank lines between its statements keeps the return
  with them (the script covers it).
- Between switch branches (labels sharing a body stay grouped).
- Around groups of consecutive `assert`s (wins over the tight-return rule).
- Accept what the formatter removes.

### Comments

- Never delete or reword an existing comment; move it with its code, in its
  own words. Change one only when it is wrong, and replace it with a correct
  one.
- Lowercase, no trailing punctuation, code names in single quotes.
- Use the `todo:` marker.
- Multiline `note:` blocks get a blank line above and below.

### Names

- Bytes: role-prefixed, e.g. `cur_`, `size_bytes`. Elements: `count`.
- Keep `src_loc_tk`, `indent`, `src`/`dst`, `lhs`/`rhs`, `tc`.

### Inserted code

- Never insert locals before case labels in a switch.
- Visitor-only references stay inside the lambda.

### Compiler warnings

- No enum switches (`-Wswitch-default` conflicts with
  `-Wcovered-switch-default`).
- `std::in_range` needs `int`, not `char`.
- Parameters shadowing members get an `_in` suffix (`-Wshadow-field`).
- No locals named like enclosing class fields (`-Wshadow`).

### clang-tidy

- No `{}` on default-constructed class members (`std::vector`, `std::bitset`);
  scalars keep `{}`.
- Overrides keep the base's visibility.

## Process

- Mechanical refactors preserve behavior; ask before changing optimization,
  allocation or functionality.
- Big change (several files or interfaces): plan in the todo file, wait for
  go-ahead.
- One consistency topic at a time across all sources.
- Todo items one by one, each with a focused check.
- Finished items of `etc/todo.txt` move to `etc/todo-resolved.txt` (right
  after its header, `[x]` with a `=> done (date): ...` note); never delete
  them.
- Done means:
  - built;
  - focused tests run;
  - linter clean;
  - formatter applied (keep its output).
- Never commit; the user commits. Only when told to commit, the message is
  always `.` (no body, no trailer).
- Live test output: no redirection or piping through `grep`/`tail`/`head`.
- Shell scripts `cd` to their own directory and use relative paths.

## Code

- Cognitive simplicity over brevity and new syntax; some performance cost is
  fine ("bio-ai").
- Multi-pass designs (one rule per pass, with a before/after example comment)
  over single-pass state machines.
- Early returns and flat paths.
  - `if`/`else` over two `if`s.
  - Flat `else if` chains are fine.
  - No nested if/else inside an else.
- Duplicates become a named helper; split nested calls into named steps.
- Plain lookup loops when ranges read worse.
- Preserve behavior and cleanup when restructuring.
- Code that user input cannot reach is dead code: turn it into an `assert` (or
  remove it) instead of testing it; reachable code gets a test.

## Baz code

- Tests and examples in baz: an initializer or an assignment omits the `{}` of
  a type or an array, the compiler assumes it: `var s = str`, `var a = i8[4]`,
  `res = T`, not `var s = str{}`.

## Tests

- Test runs: `--target=x86` and `--target=rv32i-fpga` are enough.
- While working on `etc/roome/roome.baz`, compile it for both x86_64 and
  rv32i-fpga to catch register issues (register exhaustion); x86_64 needs
  `--vars=0x20000`.
- After every change to `etc/roome/roome.baz`, run `etc/roome/test.sh` and
  validate that nothing broke.
- A new error test: check that each `line:column` in its `.out` points at the
  token where the error is detected (print the source line with a caret), not
  only that the message reads well.
- `UPDATE=1 qa/coverage/test-coverage.sh --target=x86 run` writes the expected
  outputs (`.out`) instead of comparing; run it for x86 and rv32i, then review
  the diff.

## Todo

- A resolved or discarded item leaves `etc/todo.txt` and goes to the top of
  `etc/todo-resolved.txt`, below its legend, newest first.

## Tools

- Run symbol renames before `git mv`/file renames (open buffers at the old
  path recreate old files on save).
- Editing pitfalls: see memory `editing.md`.
- `make.sh` (without `build`) and `run-roome.sh` read input: after `make.sh`
  type return, after `run-roome.sh` type "go home".

## Naming

- One-letter names only in tight loops (at most three loop locals).
- Same parameter names across declarations, definitions and overrides.
- Keep an interface's paired vocabulary.

## Comments

- One short line, only what code cannot show; never restate code or address
  the reviewer.
- Magic numbers (`+ 1`, `subspan(1, n - 2)`) in new or touched code: add a
  `note:` on the line below.
  - The note names the offset and its reason, e.g. `// note: +1 because the
    text starts after the opening quote`.
- Non-obvious new code: brief rationale (why, not what).
