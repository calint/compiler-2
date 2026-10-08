---
description: Sweep the source for design, code and comment improvements, fixing what is safe and asking about the rest
argument-hint: [path or topic, default src/]
---

Sweep `$ARGUMENTS` (default: `src/`) and look for improvements, as a final
round of review. Follow `AGENTS.md`.

## What to look for

- Design: misplaced responsibilities, classes with several modes, test-only
  code in production paths, interfaces that carry more than they need.
- Code: duplicates that should be one named helper, magic numbers, long
  parameter lists, flags that can be derived, inconsistent idioms, stale or
  redundant includes and declarations, dead code.
- Bugs: wrong results on unusual input (try the compiler on it), not only
  style.
- Comments: wrong, misplaced or stale ones (never reword or delete a correct
  one), inconsistent wording of messages.
- Consistency: one topic at a time across all sources.

## How to sweep

1. View every line of the files in scope, not only a sample; use greps and
   scripts for mechanical checks (unused functions, duplicated blocks, long
   functions, misspellings) and fuzzing the compiler for crashes.
2. Keep a list of findings with file and line, ranked by value.
3. Apply a change right away when it is in one file and behavior-preserving.
4. Wait for my approval when a change is in several files, an interface, or
   changes behavior; show an example or the diff of the proposal first.
5. Argue for a better idea instead of obeying blindly.

## Bugs

When a bug is found, write a test that fails first, show that it fails, then
fix the bug and show that the test passes.

## Done means

Built, focused tests run, linter clean, formatter applied. Run both
`qa/coverage/test-coverage.sh --target=x86` and `--target=rv32i-fpga` after a
refactor; do not pipe live test output. After a change to
`roome/src/main.baz`, run `roome/qa/test.sh`.

## Report

Short and plain: what was changed, what was proposed and is waiting for my
approval, what was checked and found nothing, and what was and was not
verified, and why. Do not commit.
