# fuzzers

Fuzzers of the compiler and of the programs it builds. Every fuzzer prints the
first example of each finding and keeps it in `findings/` (ignored by git), and
ends with 1 when the run had a finding, also one kept by an earlier run.
`fuzz-all.sh [scale] [seed]` runs them all and prints the seed first, the same
seed and scale repeat the run; its work directories are in a temporary
directory that is removed at the end, also of a run that is stopped.

## Targets

A program that a differential fuzzer runs is built for x86_64 (nasm, native),
rv32i-fpga (the emulator `fpga-emulator/osqa`) and, by a hash of its text, one
of rv32i (llvm-mc, qemu user mode) and rv32i-qemu (qemu system with a uart),
the commands of `qa/coverage/test-coverage.sh`; `FUZZ_TARGETS=all` builds it
for all four, `FUZZ_TARGETS=fixed` for the first two (`with_rotated` in
`fuzzlib.py`). A model fuzzer builds a program with several option lists for
the first two, with the first list for the third. The image the compiler
writes for rv32i must have the bytes llvm assembles from the listing
(`image-mismatch`). A crash in the qemu system traps without end, so a run
there that times out where x86_64 ends is a difference. `fuzz-constructs.py`
runs every program on all four targets, `fuzz-reject.py` on the first two,
`fuzz-options.py` on the target of its options, `fuzz-roome.py` as its table
row says, `fuzz-whole.py` each variant of a program on its targets with
`--checks=noub`, on one of them with another option list and on one without
checks.

## Undefined behavior of a baz program

The classes are those of the catalog, 8.1 of `etc/docs/README.md`:
`fuzz-ub.py` models the run-time classes, `fuzz-effects.py` the evaluation
order, parts written before and shared storage, `fuzz-alias.sh` the aliasing
of paths, `fuzz-reject.py` and `fuzz-constructs.py` the compile-time
rejections.

With `--checks=noub` a program ends in a caught failure or runs to the end with
the value of the language, never with the result of a wrapped or out of range
operation.

## Fuzzers

| script | what it does |
| --- | --- |
| `fuzz-constructs.py`, `constructs.py` | one small program with a known result per construct of the tutorial (and a program with an error per rejection rule, reported on the line of the mistake), the tutorial itself, on every target, with and without checks and with `--nopt` |
| `fuzz-coverage.py` | runs the generators without the compiler and fails when a construct of `constructs.py` is never emitted by any of them |
| `fuzz-night.sh [hours] [seed]` | `fuzz-all.sh` in rounds with new seeds and growing scale until the stop time, lists the new findings, log in `findings/night.log` |
| `fuzz-ub.py` | python model of arithmetic, widths, shifts, arrays, indexes and counts of every width, arrays as parameters, one expression in many contexts (argument, index, count, divisor, shift count, comparison, result), a wider later element that must be rejected or exact, calls and recursion against the targets |
| `fuzz-exprs.py` | python model of nested expressions (unary, conversions, comparisons, `and`/`or`/`not` with short circuit, constants, loops); with `FUZZ_SPELLING=1` each typed value is spelled as a constant and as a variable, both programs must end alike |
| `fuzz-data.py` | python model of user types (records, nested arrays, `bool`), `dat` and `var` initializers (partial, strings, characters), `let` constants and read-only variables with shadowing, constant expressions, copies, `==` and `!=` of records and arrays, `foo`, `array_copy`, `arrays_equal`, `write` of a part of an array |
| `fuzz-flow.py` | python model of `if` / `else if` / `else`, `loop` with `break` and `continue`, blocks that shadow, `exit`, functions with a result, early `return`, `noinline` |
| `fuzz-bools.py` | python model of conditions: `bool` variables, fields, array elements, parameters and results, `let` bools, `and` / `or` / `not` with short circuit, `bool == bool`, comparisons of mixed widths with a literal on either side, conditions as arguments of inlined and `noinline` functions with `loop`, `foo`, `break`, `continue` and an early `return`, with `--nopt`, `--versions=nested` and without checks |
| `fuzz-calls.py` | python model of inlined and `noinline` functions whose body reads and writes `res` in many shapes (`res = (p + res) * 3`, `res = 3 - res`, ...) called as a variable, an assignment, an argument, an index, a shift count, a divisor, an operand of a comparison, a condition, the count of `array_copy`, inside another call, with the checks, with and without `--nopt`, on the targets |
| `fuzz-effects.py` | python model of calls that write globals, a field of a global record and `mut` arguments next to the other operands of the same statement (arguments, a list of `+` and `-`, the sides of a comparison, a value written in parts, an assignment to a global the value writes): evaluation left to right, the rejections of evaluation order, of parts written before and of shared storage, each program with every function inlined and with every function `noinline`, on the targets |
| `fuzz-methods.py` | python model of records with nested records and arrays, `mut` methods, methods that only read `self` (also on a `let` receiver), a method named like a field, constructors, `noinline` functions that take a record, functions that return one, calls on variables, fields and array elements |
| `fuzz-generics.py` | python model of generic types with constants and types, aliases, methods that see the constants, generic functions with type arguments given and deduced |
| `fuzz-reject.py` | programs that break a rule of the tutorial (write a `let` or a constant, narrowing, unassigned result, aliasing, constant index, repeated name, `break` outside a loop, ...) must be rejected on the line of the mistake, the twin without the mistake must be accepted |
| `fuzz-io.py` | python model of `read` and `write` with count and start index on strings, `dat`, variables and array fields; descriptors served and not (-1, 3, 256, 2^31, 2^32 + 1, ...) of every width, constant or computed at run time, also from served values, counts and starts at the edges of the range and past them, alone, in lists and in inlined and `noinline` functions, with and without the checks |
| `fuzz-programs.py` | valid random programs of functions, methods, records, arrays, `foo`, loops, recursion: no model, every build must end the same and without a panic, also with every function inlined and with every function `noinline` |
| `fuzz-whole.py` | python model of the whole language in one program: records with nested arrays, generic types and functions, methods, constructors, `mut` and unsized array parameters, recursion, `foo` with a count and nested, loops, shadowing blocks, conversions, every operator, hex and binary literals, comments, `let`, `dat`, `array_copy`, `arrays_equal`, `write`, `read` of one line of input; the output and the exit code or the first panic, for a default integer of 64 and of 32 bits, compared with every function inlined, every function `noinline` and a mix, with `--nopt`, `--versions`, `--checks=noub,line`, `--reproduce-source` and without checks |
| `fuzz-versions.py` | python model of functions with `mut` and read-only array and scalar parameters of `i8`, `i16` and `i32`, elements as `mut` arguments, calls nested in arguments and indexes, a result, an early `return`, `loop` with `break` and `continue`, `foo`, `array_copy`, `exit` and `dat` globals, with values at the edges of their types so that the checks of `noub` fail inside the bodies; each program with every function inlined, with every function `noinline` and a mix, each built with `--nopt` and every `--versions` value on the targets |
| `fuzz-combos.py` | python model of features the other fuzzers keep apart, combined: `mut` and read-only methods on elements of record arrays reached through run-time indexes, also through `e` of `foo`; a generic type and generic functions (`noinline` or inlined) over arrays of each element type and length; an inlined recursion ended by a constant and a `noinline` one over arrays of two lengths, with `return` and `exit`; `dat` globals written by calls and read in arithmetic, indexes and receivers; `read`, `write`, `sector_read` and `sector_write` into fields of elements with run-time counts and starts; built on the four targets and with `--nopt`, `--versions`, `--checks=noub,line`, a subset of the checks, all `noinline` and all inlined |
| `fuzz-compiler.py` | mutations of the sources in `qa/coverage/tests` into the sanitizer build of the compiler: crashes, hangs, rejections without a position, invalid assembly, signals from programs compiled with `noub`, differences between targets, `--nopt`, checks and `--reproduce-source` |
| `fuzz-mutate.py` | mutations of the structure of the sources in `qa/coverage/tests`, mostly of the accepted ones (types, array sizes, numbers at the edges, `mut`, `noinline`, statements moved, wrapped and taken from another test, blocks nested, definitions swapped and renamed, arguments) into the sanitizer build of the compiler on the four targets with random options: crashes, hangs, rejections without a position |
| `fuzz-options.py` | the command line: sizes, check lists, targets, odd files |
| `fuzz-roome.py` | mutated sessions of `roome/src/main.baz` on x86_64 and rv32i-fpga, every other one also on rv32i |
| `fuzz-alias.sh` | the alias check against a brute force of the bytes |
| `fuzz-ub.sh --mutants` | the check of the checks: turns off one check at a time and requires a mismatch of `fuzz-ub.py` |
| `fuzz-reduce.py`, `fuzz-reduce-roome.py` | shrink a finding |

`build-asan.sh` builds `build/baz-asan` (AddressSanitizer and
UndefinedBehaviorSanitizer) that most fuzzers use, run it after the compiler
changed; a compile through `fuzzlib.py` stops the fuzzer when the compiler is
older than a file of `src/` (`assert_fresh`).

A compile that takes more than 60 seconds (`SLOW_COMPILE` in `fuzzlib.py`) is
the finding `compiler-slow`, the program is kept with the time and options.
About every third program of the model fuzzers is built with its definitions in
an included file (`split_include` in `fuzzlib.py`).

## Reading a finding

- `findings/NAME.baz` is the program, `findings/NAME.txt` the category, the key
  that tells findings apart, the seed and the index of the program and the
  details; `fuzz-flow.py bisect SEED INDEX` and `fuzz-whole.py --one SEED
  INDEX` take them.
- `fuzz-reduce.py FILE.baz --match=REGEX --options="--target=rv32i-fpga
  --checks=noub"` shrinks a program that makes the compiler print text,
  `--accepted-by="--target=x86_64"` keeps it valid.
- delete the files of a finding to see it reported again.
- a fixed finding is reduced and added as a case of `qa/coverage/cases.sh`.

## Differences that are not defects

- the default integer is 64 bits on x86_64 and 32 bits on rv32i, a program of
  default integers can overflow on one target only
- the uart of rv32i-qemu and rv32i-fpga carries standard error with the
  output, a `write` to 2 shows in the output there only
- register use differs between targets and between `--checks`, an expression
  that needs too many registers is rejected with "out of scratch registers,
  simplify the expression"
- programs are linked with `baz.ld`, which places `.bss.vars` after `.data`: a
  plain `ld` gives false failures of the frame check
- a literal that does not fit inside `i32(...)` wraps, the arithmetic of an
  explicit conversion wraps
- `read` on the uart of rv32i-qemu and rv32i-fpga returns one line, on a file
  as many bytes as fit; each read at the end of the input takes one end of
  transmission byte; a carriage return reads as a newline
- a sector past the end of the card panics `sd card` on x86_64 and rv32i,
  rv32i-fpga does not detect it; rv32i-qemu has no sd card, a sector
  transfer panics `sd card` there, the tutorial too
- nasm 3.02 may stall ("unable to find valid values for all labels") on x86_64
  code with many jumps near the short jump limit, e.g. with
  `--checks=noub,line`; the code has no size that depends on a label, the
  stall is in nasm's jump sizing (`nasm -O1` assembles it), reduced in
  `etc/nasm-issue.s`
