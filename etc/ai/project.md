# compiler-2 (baz) project file for ai assistants

Read `etc/ai/preferences.md` first (general preferences). This file is specific
to this repository. The workspace root is `/home/c/w/compiler-2`; never use any
duplicate or alternate path, especially `/home/c/compiler-2`.

## what this is

`baz` is a compiler for the baz language written in C++26. It emits x86_64
(nasm syntax) and RV32I assembly (targets `rv32i`, `rv32i-qemu`,
`rv32i-fpga`), and for RV32I also a flat binary image. The compiler is
header-only: `make.sh` compiles only `src/main.cpp` with `clang++` and
`-Weverything -Werror`. New code stays in headers (`#pragma once`, `inline`
where needed) included from `main.cpp`.

## do not touch

- `README.md` is generated from `etc/readme/README.1.md` (see
  `etc/readme/make.sh`). Never update or mention it.
- Generated and scratch files in the root (`prog*`, `roome.s`, `gen-*.bin`,
  `*.profraw`) are not source.

## build, test, lint

- Build: `./make.sh -O3 build`. For this workspace run `make.sh build` only
  to build; do not run the demo/program step. Use the normal build without
  `prof` or `asan` (the user runs the instrumented coverage separately).
- Coverage runner: `qa/coverage/test-coverage.sh --target=T run` with T one of
  `x86`, `rv32i`, `rv32i-qemu`, `rv32i-fpga` (subcommands clean|build|run|
  report). `cases.sh` is the sourced case list; tests are
  `qa/coverage/tests/NNN.baz` numbered from 001 (`NNN.out` expected
  output, `NNN.in` input, `COMPERR`/`DIFF`/`RUN_ERR` variants in `cases.sh`).
- Other suites: `qa/coverage/test-cli.sh`, `qa/coverage/test-arena.py`
  (executable), `qa/coverage/test-rv32i.sh` (builds `test-rv32i-address.cpp`,
  backend tests that embed baz sources and expected assembly; update their
  expectations when emitted code changes). Golden scripts with an `update`
  mode: `test-string-stores.sh` (557), `test-bulk-widths.sh` (590),
  `test-constant-folding.sh` (593), `test-equal-compares.sh` (598);
  `test-deduced-types.sh` (609) has no golden file. `qa/coverage/test-all.sh`
  runs everything.
- Do not run `test-all` per todo item (it also rebuilds `baz` instrumented);
  run a focused check, the user runs `test-all` after a batch. Show the suite
  output plainly.
- For non-coverage builds and tests prefer `./make.sh -O3 build`.
- Always run `qa/lint/clang-tidy.sh` and fix its findings before considering a
  task done (it overwrites the tracked `qa/lint/clang-tidy.log`; restore it
  with `git checkout qa/lint/clang-tidy.log` unless the user wants the log).
  `readability-trailing-comma` requires a trailing comma in brace initializer
  lists that span lines.
- The rv32i-fpga suite also verifies the image: the compiler's own binary must
  equal what `llvm-mc` + `ld.lld -T qa/coverage/rv32i-image.ld` +
  `llvm-objcopy -O binary` produce from the generated assembly. The user values
  this byte identity; keep it, and point out when llvm-mc's code is worse. Known
  gap: the compiler expands some `li` constants (for example 2560) as
  `lui`/`addi` where llvm-mc uses `addi`/`slli`; avoid emitting `li` for values
  that may differ (`set_loop_end` uses `address_of` for that reason).
- Compile a program: `./baz --target=x86_64|rv32i|rv32i-qemu|rv32i-fpga
  [--checks=upper,lower,line,frame,noub] [--vars=N] [--bin=FILE] file.baz`
  writes assembly to stdout; the last line `instructions: N` counts
  instructions. Example used for code-quality surveys:
  `./baz --target=rv32i-fpga --vars=0x20000 --checks=noub,line
  etc/roome/roome.baz`.

## formatting and layout tools

- Format all C++ under `src/` with clang-format by default (not only changed
  lines) before presenting a checkpoint.
- `qa/lint/format-source.py [--apply] [files]` (libclang) enforces class
  layout: private data and types at the class top, private functions in one
  `private:` section at the bottom sorted alphabetically, constructors and
  defaulted/deleted special members first in the first public section, then
  data, then methods sorted by name, overrides before virtuals before own
  methods, static methods last under a framed `// statics` marker. Run it
  (dry run, then `--apply`) after adding or moving members, then clang-format.
- Style scripts in `etc/ai/style-scripts/` (`signature-blank.py`,
  `tight-return.py`, `assert-groups.py`; dry-run with no args from the
  workspace root, `apply` to write): run all three before a checkpoint, then
  clang-format, then dry-run again.

## working rules specific to this repo

- Plans for big changes go to `etc/todo.txt`; wait for the go-ahead. Specialized
  instruction tricks go there too.
- Survey and report files: `etc/roome-inefficiencies-rv32i.txt` and
  `etc/roome-inefficiencies-x86_64.txt` list local inefficiencies found in the
  generated code of `etc/roome/roome.baz` and which were fixed.
- Backend priorities: local assembly and register use that matches
  straightforward hand-written code (no redundant copies, loads or scratch
  allocations); local fixes within one machine operation first; context across
  operations goes under the todo items about register allocation spanning
  operations or constants known at compile time. An optimization that saves no
  instructions and only an extra scratch register briefly within one operation
  is low priority.
- RV32I scratch allocation order: `t0-t6`, `s0-s11`, `tp/gp/ra`, `a1-a7`, then
  `a0` last because syscalls overwrite the result register; skip reserved and
  live registers; keep argument registers late to reduce builtin conflicts.
  `s0` holds the arena base (`dat`), `s1` the noninline frame base while one is
  active, `sp` is native. x86 scratch order: `r15 r14 r13 r12 r10 r9 r8 r11`,
  last resort `rbx rsi rdi rcx rdx rax`; `rbp` is the variables base, `rbx`
  the noninline frame base.
- Mechanical refactors preserve emitted assembly. Parity checks compare the
  generated assembly (all coverage tests, both targets, several modes) of an
  old and a new compiler built from `git archive HEAD src`; copy `./baz` to a
  baseline before rebuilding. Compare stdout, stderr and exit codes; run the
  coverage suites for compile-error messages.
- A reached `std::unreachable()` is UB, so an `-O3` build can pass by luck;
  after parser or statement changes also build `-g -O0` and compile every test
  on both targets, listing exit codes >= 128.

## architecture essentials

Pipeline: `tokenizer` -> recursive-descent parsing of statements
(`stmt_*`, `expr_*`), with parsing and code generation interleaved in
constructors and `compile()` -> `machine` interface -> `assembler` classes.

- `toc` is the compile context: frames/scopes, variables, constants, types,
  functions, label generation, bounds-check options. `statement` is the base
  class (`compile`, `source_to`, `compile_lea`, `compile_boolean`,
  `visit_reads`). `decouple.hpp` / `decouple_impl.hpp` break circular includes
  (grammar dispatch factories, shared structs `operand`, `var_info`,
  `ident_info`).
- `machine.hpp` is the backend interface (about 70 pure virtual operations:
  `copy_value`, `add_subtract`, `compare_and_branch`, `check_bounds`,
  bulk `copy`/`zero`/equal, register allocation, calls, io, data emission).
  Implementations: `machine_x86_64`; `machine_rv32i` with
  `machine_rv32i_bare_metal` (read/write/exit routines, stack setup) and the
  final targets `machine_rv32i_qemu` and `machine_rv32i_fpga`. Only `main.cpp`
  includes concrete machines. `emit_most_efficient` tries two code shapes and
  keeps the shorter.
- `assembler.hpp` is the line model (text, labels, jump records) with jump
  optimization (jump to next, branch over jump, same target) and instruction
  counting; `assembler_x86_64` and `assembler_rv32i` emit structured
  instructions (registers as names, `immediate` for symbols) and
  `assembler_rv32i` also encodes the binary image. RV32I far jumps grow
  branch -> inverted branch + `j` -> `jump` through a scratch register.
- Registers: one LIFO `allocations_` stack of named and scratch registers
  (free in reverse order) plus an `unavailable_registers_` mask; `address_scope`
  protects operand registers during lowering and frees temporaries on exit.
  Calls to noninline functions save only allocated registers.
- Language notes: `let` (constant or read-only var), `var x = INIT` (type
  deduced, always initialized), `dat x = INIT`, params read-only unless
  `NAME mut`, methods `func [noinline] [mut] T.m`, constructors `func T.m(...)
  self`, arrays type-first (`arr i8[]`, default-type arrays `arr[]`),
  `foo array[, count]` loops with element `e` and counter `i`, explicit
  narrowing `i8(x)`, `true`/`false` are `bool`, natural alignment of record
  fields and variables, constant folding in `expr_arith`.
- Bounds checks (`--checks`): `upper`, `lower`, `line`, `frame`. A single index
  or count checked with both bounds uses one unsigned upper compare (it also
  fails negatives while the limit is below 2^31 on RV32I / 2^63 on x86); the
  sum form `index + count` tests both signs; a count already found
  non-negative in the same operation (`lower_checked_registers_`, cleared on
  re-allocation) is not tested again; the comments in the assembly say so.
- Bulk operations on RV32I: unrolled when small, otherwise loops ending at an
  end pointer (`set_loop_end`), widths from alignment (`access_start`); x86
  uses `rep movsb/stosb` and `repe cmps*`. Two memory operands of one copy that
  share a `lui` upper part share one base (`copy_through_shared_base`).
- rv32i-fpga: image at address 0, uart at `-12` (in, `-1` = no byte) and `-8`
  (out), `ebreak` ends the program with `a0` as exit code, memory ends at
  0x800000 (`--stack` default 0x10000), word accesses drop the low two address
  bits so accesses must be aligned. rv32i-qemu runs under
  `qemu-system-riscv32 -machine virt -bios none`.
- Variables live in `.bss` after the initialized data (`dat`); every variable
  has an initializer, there is no blanket zeroing.

`etc/ai/architecture-log.md` is the long dated history of design decisions and
checkpoints; search it for the reasoning behind a specific feature before
changing it.
