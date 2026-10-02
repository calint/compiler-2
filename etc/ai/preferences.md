# general preferences for ai assistants

These apply to any project this user works on. Project specifics live in the
project's own file (for compiler-2: `etc/ai/project.md`).

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
