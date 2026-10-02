# compiler-2 (baz)

## permissions

- Shell (build, test, lint, format, scratch compiles): no asking. Ask for
  `rm -rf`, `git push`, `git reset --hard`, `git clean`, `sudo`.
- Scratch files only in `/tmp`. Workspace root: `/home/c/w/compiler-2`.

## agents.md

- Read and follow this file every session.
- Whenever a preference is added to memory, add it here in the same turn.

## communication

- Short, plain, impersonal; no emphasis words or emojis.
- Flag misspellings briefly ("inneficiencies" -> "inefficiencies"); spell
  correctly in names and files.
- Report what was and was not verified, and why. Failed precondition: stop and
  explain, no weaker version.
- Workspace-relative paths.

## c++ style

- Brace init everywhere: `.member{value}`, `bool enabled{};`, trailing comma in
  multiline initializers. Empty optional: `return std::nullopt;`.
- Explicit types; `auto` only for cumbersome types (iterators). Backend access:
  `machine& x{tc.machine()};` plus a blank line; pointers like
  `const macro_use* const p{std::get_if<...>(r)};`.
- `and`/`or`/`not`; `std::format("{}", v)` over `std::to_string`.
- Blank lines:
  - around multiline statements and declarations, and after the `{` of a
    multiline signature;
  - before a return, except return-only bodies or one single-line statement
    plus a single-line return;
  - between switch branches (labels sharing a body stay grouped);
  - around groups of consecutive `assert`s (wins over the tight-return rule);
  - accept what the formatter removes.
- Comments: lowercase, no trailing punctuation, code names in single quotes,
  `todo:` marker. Multiline `note:` blocks get a blank line above and below.
- Names: `cur_`, `size_bytes` (role-prefixed) for bytes, `count` for elements;
  keep `src_loc_tk`, `indent`, `src`/`dst`, `lhs`/`rhs`, `tc`.
- Inserted locals: never before case labels in a switch; visitor-only
  references stay inside the lambda.
- Warnings: no enum switches (`-Wswitch-default` vs `-Wcovered-switch-default`);
  `std::in_range` needs int, not `char`; `_in` suffix for params shadowing
  members (`-Wshadow-field`); no locals named like enclosing class fields
  (`-Wshadow`).
- clang-tidy: no `{}` on default-constructed class members (`std::vector`,
  `std::bitset`), scalars keep `{}`; overrides keep the base's visibility.

## process

- Mechanical refactors preserve behavior; ask before changing optimization,
  allocation or functionality.
- Big change (several files or interfaces): plan in the todo file, wait for
  go-ahead.
- One consistency topic at a time across all sources; todo items one by one,
  each with a focused check.
- Done = built, focused tests run, linter clean, formatter applied (keep its
  output).
- Live test output: no redirection or piping through grep/tail/head.
- Shell scripts `cd` to their own directory and use relative paths.

## code

- Cognitive simplicity over brevity and new syntax; some performance cost is
  fine ("bio-ai").
- Multi-pass designs (one rule per pass, before/after example comment) over
  single-pass state machines.
- Early returns, flat paths; `if`/`else` over two `if`s; flat `else if` chains
  ok; no nested if/else inside an else.
- Duplicates -> named helper. Split nested calls into named steps.
- Plain lookup loops when ranges read worse.

## naming

- One-letter names only in tight loops (at most three loop locals).
- Same parameter names across declarations, definitions and overrides; keep an
  interface's paired vocabulary.

## comments

- One short line, only what code cannot show; never restate code or address
  the reviewer.
- Magic numbers (`+ 1`, `subspan(1, n - 2)`) in new or touched code: `note:` on
  the line below.
- Non-obvious new code: brief rationale (why, not what).
