# compiler-2 (baz)

Read [etc/ai/preferences.md](etc/ai/preferences.md) (general preferences) and
[etc/ai/project.md](etc/ai/project.md) (build, test, lint, style, architecture)
before changing code. The summary below does not replace them.

## permissions

- Run shell commands (build, tests, linters, formatters, scratch compiles)
  without asking; ask first only for destructive or shared-state actions
  (`rm -rf`, `git push`, `git reset --hard`, `git clean`, `sudo`).
- `/tmp` is the scratch area: create, edit, run and delete files there freely
  (for example the coverage scratch build `/tmp/baz-cov`, see
  [etc/ai/project.md](etc/ai/project.md)). Keep scratch output out of the repo.
- The workspace root is `/home/c/w/compiler-2`; never use other paths for it.

## communication

- Keep answers short and impersonal. State facts plainly, without emphasis or
  dominant phrasing; avoid words such as "exact" and "extremely" when not
  needed. No emojis.
- Correct the user's spelling when it happens: point out the misspelling
  briefly (for example "inneficiencies" -> "inefficiencies", "pay of" -> "pay
  off") and use the correct spelling in names and files.
- When a result is negative or partial, say so plainly: report what was
  verified, what was not, and why. Do not claim a goal the evidence does not
  support (for example when a precondition turned out not to hold, stop and
  explain instead of implementing a weaker version).
- Name files with workspace-relative paths.

## scope and process

- For mechanical refactors preserve behavior; ask before changing
  optimization, allocation policy or other functionality.
- When a requested change turns out to be big (several files or interfaces),
  stop, record the plan in the project's todo file and wait for the go-ahead.
- Handle one consistency topic at a time across all source files rather than
  making successive small local passes.
- Fix todo items one by one with a focused check each; the user runs the full
  suite after a batch.
- Before considering a task done: build, run the focused tests, run the
  project's linter and fix its findings.
- Format the code with the project's formatter before presenting a checkpoint;
  do not undo the formatter's output with follow-up whitespace edits.
- Keep a todo list for multi-step work and update it as each item completes.
- Show live test output while suites run. Do not redirect test progress to log
  files and do not pipe a run through grep or tail to show failures only; run
  the suite plainly so the full output is visible.
- Shell scripts use no absolute paths: `cd` to the script's directory first and
  use paths relative to it.

## code readability (priority over fewer lines)

- Cognitive simplicity beats newer syntax and beats brevity. The user calls
  himself "bio-ai" (tongue in cheek) and dislikes complexity; a simple
  algorithm is worth some performance cost.
- Prefer multi-pass designs, each pass one simple rule with its own comment and
  a before/after example, over a single-pass state machine.
- Use early returns and keep decision paths flat and explicit. A plain
  `if`/`else` is fine, even preferred over two separate `if`s, when both bodies
  are simple. Avoid nested "if/else, and inside the else another case" logic.
  A flat `else if` dispatch chain is fine.
- Extract duplicated code into descriptively named helper functions.
- Split nested calls such as `f(g(h(x)), -k())` into named intermediate
  variables, one step per line, instead of one long argument expression.
- Keep a straightforward lookup loop when a ranges find/projection plus
  iterator checks would be harder to read. Modern idioms are welcome when they
  genuinely simplify the expression.
- Preserve behavior and cleanup when restructuring; do not trade clarity for
  fewer lines.

## naming

- One-letter variables in small, tight loops with at most three loop-local
  variables; increasingly descriptive names as scope and complexity grow. Keep
  the established vocabulary of an interface consistent.
- Match parameter names across declarations, definitions and overrides; keep
  paired operations' vocabulary (`src`/`dst`, `lhs`/`rhs`) consistent.

## comments

- Write a comment only to say what the code cannot show on its own, in one
  short line. Do not restate the next line or explain the change to the
  reviewer.
- Explain magic numbers such as `+ 1`, `- 2` or `subspan(1, n - 2)` in new or
  or touched code with a `note:` comment saying what the number stands for,
  placed below the line that uses it.
- Always give new or modified non-obvious code a brief plain-language rationale
  comment: why it exists (constraints, ordering, preservation), not what it
  does.

## quick reference

- Build: `./make.sh -O3 build` (no demo step). Lint: `qa/lint/clang-tidy.sh`.
- Focused tests: `qa/coverage/test-coverage.sh --target=x86 run`; the user
  runs `qa/coverage/test-all.sh` after a batch.
- Never update or mention `README.md` (generated from `etc/readme/README.1.md`).
