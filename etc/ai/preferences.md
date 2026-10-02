# general preferences for ai assistants

These apply to any project this user works on. Project specifics live in the
project's own file (for compiler-2: `etc/ai/project.md`).

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
  himself "bio-ai" and dislikes complexity; a simple algorithm is worth some
  performance cost.
- Prefer multi-pass designs, each pass one simple rule with its own comment and
  a before/after example, over a single-pass state machine.
- Use early returns and keep decision paths flat and explicit. A plain
  `if`/`else` is fine, even preferred over two separate `if`s, when both bodies
  are simple. Avoid nested "if/else, and inside the else another case" logic.
  A flat `else if` dispatch chain is fine.
- Extract duplicated code into descriptively named helper functions.
- Keep a straightforward lookup loop when a ranges find/projection plus
  iterator checks would be harder to read. Modern idioms are welcome when they
  genuinely simplify the expression.
- For one algorithm shared by several operations (for example bulk zero, copy
  and compare), prefer a base class holding all the decision logic with
  subclasses implementing one virtual per-element operation, over free
  functions with callbacks.
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

## editing and tool practices

- Edit source with the editor's edit tools (replace or multi-replace), not with
  python or sed scripts in the terminal, so files show as modified in the
  editor's changes view. Terminal scripts are for bulk operations only
  (formatters, mass renames).
- Parallel replace calls on the same file can land at stale offsets and garble
  it; use one multi-replace call instead.
- Running a formatter in place (for example `clang-format -i`) in the terminal
  right after editor edits can make the next editor edit land at stale offsets;
  after formatting, re-read the file before editing and verify the disk content
  with grep.
- A file just written or edited through the editor may not be visible to the
  terminal right away ("cannot open file", or a script runs its old version);
  rerun the command, and rerun validations started right after an edit when
  results look inconsistent.
- In the VS Code terminal `{ echo ...; } > file` groups can capture
  shell-integration escape codes into the file; write generator scripts to a
  file and run `bash script > out` instead.
- In Perl bulk substitutions disambiguate captures next to `{` as `${2}`;
  `$2{ ... }` can be read as a hash lookup and silently drop replacement text.
  Check one transformed example before scaling up.
- Run symbol renames before moving or renaming the files they live in.
- A tool call with a stray character in its JSON (for example a leading `>` in
  a string) fails validation; re-issue it with clean strings.
