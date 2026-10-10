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
| `fuzz-ub.sh` | python model of arithmetic, widths, shifts, arrays, indexes and counts of every width, arrays as parameters, one expression in many contexts (argument, index, count, divisor, shift count, comparison, result), a wider later element that must be rejected or exact, calls and recursion against x86_64 and rv32i-fpga, `--mutants` turns off one check at a time and requires a mismatch |
| `fuzz-exprs.py` | python model of nested expressions (unary, conversions, comparisons, `and`/`or`/`not` with short circuit, constants, loops) |
| `fuzz-programs.py` | valid random programs of functions, methods, records, arrays, `foo`, loops, recursion: no model, every build must end the same and without a panic |
| `fuzz-compiler.py` | mutations of the sources in `qa/coverage/tests` into the sanitizer build of the compiler: crashes, hangs, rejections without a position, invalid assembly, signals from programs compiled with `noub`, differences between targets, `--nopt`, checks and `--reproduce-source` |
| `fuzz-options.py` | the command line: sizes, check lists, targets, odd files |
| `fuzz-roome.py` | mutated sessions of `roome/src/main.baz` on both targets |
| `fuzz-alias.sh` | the alias check against a brute force of the bytes |
| `fuzz-reduce.py`, `fuzz-reduce-roome.py` | shrink a finding |

`build-asan.sh` builds `build/baz-asan` (AddressSanitizer and
UndefinedBehaviorSanitizer) that most fuzzers use, run it after the compiler
changed.

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
- a very long expression compiles in more than linear time
