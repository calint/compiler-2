# fuzzers

Fuzzers of the compiler and of the programs it builds. Every fuzzer prints the
first example of each finding and keeps it in `findings/` (ignored by git).
`fuzz-all.sh [scale] [seed]` runs them all.

## Undefined behavior of a baz program

1. signed overflow of `+ - *` and unary `-`: `overflow`
2. division by zero, `MIN / -1`, `MIN % -1`: `division`
3. shift count outside the width of the type: `shift`
4. index outside an array, also nested in fields: bounds (`upper`, `lower`)
5. range outside an array in `array_copy`, `arrays_equal`, `read`, `write`:
   bounds
6. `array_copy` whose destination starts inside the source: `overlap`
7. unbounded recursion of a `noinline` function: `frame`, `stack` or the
   operating system
8. narrowing, an unset result, an unset variable: rejected by the compiler
9. aliasing arguments and results: rejected by the compiler

With `--checks=noub` a program ends in a caught failure or runs to the end with
the value of the language, never with the result of a wrapped or out of range
operation.

## Fuzzers

| script | what it does |
| --- | --- |
| `fuzz-constructs.py`, `constructs.py` | one small program with a known result per construct of the tutorial (and a program with an error per rejection rule, reported on the line of the mistake), the tutorial itself, on both targets and with and without checks |
| `fuzz-coverage.py` | runs the generators without the compiler and fails when a construct of `constructs.py` is never emitted by any of them |
| `fuzz-night.sh [hours] [seed]` | `fuzz-all.sh` in rounds with new seeds and growing scale until the stop time, lists the new findings, log in `findings/night.log` |
| `fuzz-ub.sh` | python model of arithmetic, widths, shifts, arrays, indexes and counts of every width, arrays as parameters, one expression in many contexts (argument, index, count, divisor, shift count, comparison, result), a wider later element that must be rejected or exact, calls and recursion against x86_64 and rv32i-fpga, `--mutants` turns off one check at a time and requires a mismatch |
| `fuzz-exprs.py` | python model of nested expressions (unary, conversions, comparisons, `and`/`or`/`not` with short circuit, constants, loops) |
| `fuzz-data.py` | python model of user types (records, nested arrays, `bool`), `dat` and `var` initializers (partial, strings, characters), `let` constants and read-only variables with shadowing, constant expressions, copies, `==` and `!=` of records and arrays, `foo`, `array_copy`, `arrays_equal`, `write` of a part of an array |
| `fuzz-flow.py` | python model of `if` / `else if` / `else`, `loop` with `break` and `continue`, blocks that shadow, `exit`, functions with a result, early `return`, `noinline` |
| `fuzz-calls.py` | python model of inlined and `noinline` functions whose body reads and writes `res` in many shapes (`res = (p + res) * 3`, `res = 3 - res`, ...) called as a variable, an assignment, an argument, an index, a shift count, a divisor, an operand of a comparison, a condition, the count of `array_copy`, inside another call, with the checks, with and without `--nopt`, on both targets |
| `fuzz-methods.py` | python model of records with nested records and arrays, `mut` methods, methods that only read `self` (also on a `let` receiver), a method named like a field, constructors, `noinline` functions that take a record, functions that return one, calls on variables, fields and array elements |
| `fuzz-generics.py` | python model of generic types with constants and types, aliases, methods that see the constants, generic functions with type arguments given and deduced |
| `fuzz-reject.py` | programs that break a rule of the tutorial (write a `let` or a constant, narrowing, unassigned result, aliasing, constant index, repeated name, `break` outside a loop, ...) must be rejected on the line of the mistake, the twin without the mistake must be accepted |
| `fuzz-io.py` | python model of `read` and `write` with count and start index on strings, `dat`, variables and array fields |
| `fuzz-programs.py` | valid random programs of functions, methods, records, arrays, `foo`, loops, recursion: no model, every build must end the same and without a panic |
| `fuzz-compiler.py` | mutations of the sources in `qa/coverage/tests` into the sanitizer build of the compiler: crashes, hangs, rejections without a position, invalid assembly, signals from programs compiled with `noub`, differences between targets, `--nopt`, checks and `--reproduce-source` |
| `fuzz-options.py` | the command line: sizes, check lists, targets, odd files |
| `fuzz-roome.py` | mutated sessions of `roome/src/main.baz` on both targets |
| `fuzz-alias.sh` | the alias check against a brute force of the bytes |
| `fuzz-ub.sh --mutants`, `fuzz-alias-long.sh` | the check of the checks, a longer alias run |
| `fuzz-reduce.py`, `fuzz-reduce-roome.py` | shrink a finding |

`build-asan.sh` builds `build/baz-asan` (AddressSanitizer and
UndefinedBehaviorSanitizer) that most fuzzers use, run it after the compiler
changed.

A compile that takes more than 60 seconds (`SLOW_COMPILE` in `fuzzlib.py`) is
the finding `compiler-slow`, the program is kept with the time and options.
About every third program of the model fuzzers is built with its definitions in
an included file (`split_include` in `fuzzlib.py`).

## Reading a finding

- `findings/NAME.baz` is the program, `findings/NAME.txt` the category, the key
  that tells findings apart and the details.
- `fuzz-reduce.py FILE.baz --match=REGEX --options="--target=rv32i-fpga
  --checks=noub"` shrinks a program that makes the compiler print text,
  `--accepted-by="--target=x86_64"` keeps it valid.
- delete the files of a finding to see it reported again.

## Differences that are not defects

- the default integer is 64 bits on x86_64 and 32 bits on rv32i, a program of
  default integers can overflow on one target only
- the uart of rv32i-fpga takes every file descriptor in `write`
- with `--checks=noub` a constant index offset is checked at run time instead of
  rejected at compile time
- register use differs between targets and between `--checks`, an expression
  that needs too many registers is rejected with "reduce expression complexity"
- programs are linked with `baz.ld`, which places `.bss.vars` after `.data`: a
  plain `ld` gives false failures of the frame check
- a literal that does not fit inside `i32(...)` wraps, the arithmetic of an
  explicit conversion wraps
- an integer is accepted where a `bool` is expected and a `bool` where an
  integer is, the value is not changed
- `read` on the uart of rv32i-fpga returns one line, on a file as many bytes as
  fit; each read at the end of the input takes one end of transmission byte
- a very long expression compiles in more than linear time
