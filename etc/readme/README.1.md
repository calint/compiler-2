# compiler-2: baz

Experimental compiler for a minimalistic, specialized language targeting x86_64
(Linux) via NASM and RV32I via the LLVM assembler running in QEMU (Linux). For
bare-metal RV32I (QEMU `virt` machine and an FPGA soft core) the compiler writes
the binary image itself.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler
* super-loop program with non-reentrant inlined functions
* support for reentrant noinline functions
* ub-free with `--checks=noub`
* basic support for generics

## Supports

* built-in integer types `i64`, `i32`, `i16` and `i8` (`i64` only on x86_64)
  and `bool`; a type left out is the default type `int`, `i64` on x86_64 and
  `i32` on rv32i; a `bool` is not an integer, `int(b)` converts it to 1 or 0
* user defined types with methods and constructors
* data, constants, arrays and array iteration
* variables with the type deduced from the initializer
* string, character, user type and array initializers
* numbers in decimal, `0x` hex and `0b` binary; a leading zero is decimal,
  `010` is 10, not octal as in C
* functions inlined at every call (the default) and `noinline` functions,
  emitted once and called, which allows recursion of any depth
* checks against undefined behavior, selected with `--checks=LIST`
  * `noub` turns on all of them, `line` adds the line number to the report of
    a failed bounds, division, shift, overlap or overflow check, `-NAME` turns
    a check off after the others are applied, e.g. `--checks=noub,-division`
  * `upper`, `lower`: array bounds at runtime; a constant index outside the
    array or a constant count that is negative or passes the end of the array
    is a compile error also without the checks, on an unsized array parameter
    when it is negative or passes the longest array the target addresses
  * `frame`: noinline function frame capacity at runtime
  * `division`: division by zero and `MIN / -1`, also of `%`, at runtime; a
    constant `MIN / -1` is a compile error, of `i64` also without the check; a
    constant zero divisor is a compile error also without the check
  * `shift`: shift count outside the type width at runtime, bits shifted out
    by `<<` are discarded; a constant count outside the width is a compile
    error also without the check
  * `overlap`: `array_copy` whose destination starts inside the source at
    runtime, a copy down the array is allowed
  * `stack`: stack capacity at `noinline` calls on `rv32i-qemu` and
    `rv32i-fpga` (`--stack` sets its size, `--memory` the memory of
    `rv32i-fpga`), the other targets run in an operating system that stops a
    program that overflows the stack
  * `overflow`: signed overflow of `+`, `-`, `*` and unary `-` at the width of
    the destination, or of the first operand where there is none (`a + a > 0`),
    at runtime, a constant expression that overflows is rejected at compile
    time, the arithmetic of an explicit conversion such as `i8(a + b)` still
    wraps
  * a failed check prints `panic: NAME` on a line of its own on standard error
    (the uart on the bare-metal targets), with `line` `panic: NAME at FILE:LINE`
    where FILE is the path of the file relative to the directory of the main
    file, e.g. `lib/util.baz`, and exits with the code of the check: `bounds`
    255, `frame overflow` 254, `division` 253, `shift` 252, `overlap` 251,
    `stack overflow` 250, `overflow` 249; a failed `sector_read` or
    `sector_write` prints `panic: sd card` and exits with 248; a constant
    `exit` code outside 0 to 255 is rejected, a run-time one is taken modulo
    256, so `exit(-1)` of a variable ends with 255 as a `bounds` panic does
* rejected at compile time with or without `--checks`, since each would give
  undefined behavior or a result that depends on the target or on the order
  of evaluation (all cases: chapter 8 of
  [`etc/docs/README.md`](etc/docs/README.md))
  * a read of a variable or of a result before it is set, e.g. a result
    that is not set on every path
  * an assignment that narrows without a conversion, e.g. `b = w` with `b`
    an `i8` and `w` an `i16`, or a constant that does not fit its
    destination; `b = i8(w)` wraps on purpose
  * a `bool` used as an integer or an integer used as a condition
  * a constant index or count outside the array, a constant zero divisor, a
    constant shift count outside the type width and a constant `exit` code
    outside 0 to 255
  * calls and assignments where the value may read the destination under
    another name (aliasing), arguments that may share storage when a
    parameter is `mut`, and an argument or a side of a comparison that a
    call in a later one may write, e.g. `f(a[i], h(i))` or `i == h(i)` with
    `h` writing `i`; evaluation is left to right, an arithmetic operand is
    read at its place: `i + h(i)` adds the `i` from before the call
  * a statement that is never reached, e.g. one after `break`
* keywords: `func`, `noinline`, `mut`, `type`, `dat`, `var`, `let`, `foo`,
  `loop`, `if`, `else`, `continue`, `break`, `return`, `self`, `and`, `or`,
  `not`, `true`, `false`, `include`
* `include "lib.baz"` at the top of a file adds the definitions of another file
  where it is, relative to the directory of the including file as the file
  system resolves it (a `..` after a link to a directory is the parent of its
  target), each file once, also when reached through another path or a link
* a source file has `\n` or `\r\n` line ends, a carriage return without a
  line feed and a byte order mark are rejected; strings and comments take
  any other byte, a message shows a control byte, a byte outside a UTF-8
  character or the bytes of a character that is invisible or looks like a
  space (the spaces other than ` `, soft hyphen, zero width, filler, tag and
  bidi characters) as `\xNN`; a message starts with `FILE:LINE:COLUMN`, the
  column counts bytes from 1
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `read`,
  `write`, `sector_read`, `sector_write`, `exit`, `int`, `i8`, `i16`, `i32`, `i64`
* read and write sectors from and to file on `x86_64` and `rv32i`, the file
  (`--sdcard`) must exist and hold the sector, e.g. made with
  `truncate -s 8M sdcard.img`; a sector is its value modulo 2^32 on every
  target, e.g. -2 is 2^32 - 2, and x86_64 panics on one outside -2^31 to
  2^32 - 1, which a 32 bit register cannot hold
* read and write sectors from and to sd card on fpga hardware using `rv32i-fpga`

## Howto

* requirements: `clang++` 23 or later (tested with 23.1), `nasm`, `ld`,
  `llvm-mc`, `ld.lld`, `qemu-riscv32`, `qemu-system-riscv32`, `g++` (C++23)
  for the fpga emulator of `rv32i-fpga`; the tests need `python3` and
  `llvm-objcopy`, the coverage report `llvm-profdata` and `llvm-cov`, the
  tests of the emulator (`fpga-emulator/qa/test.sh`) `riscv64-elf-g++` with
  its `objcopy` and `objdump`; the versions last used are in
  [`etc/tools.txt`](etc/tools.txt)
* [`./make.sh`](make.sh) compiles the compiler then compiles and runs
  [`prog.baz`](prog.baz), `./make.sh build` only compiles the compiler
* [`./run.sh`](run.sh) `[options] [NAME.baz]` compiles, assembles and runs
  `NAME.baz` (default: `prog.baz`) with
  `--checks=noub,line --report=registers`, the options are passed to `baz`
  after them, a `--checks` replaces the default one; `NAME.baz` is relative
  to the repository root (`run.sh` changes to it first)
  * compiles to `NAME.s` and writes `NAME-without-comments.s`
  * builds and runs depending on `--target`
    * `x86_64` (default): assembles `NAME.o`, links `NAME`, runs natively
    * `rv32i`: assembles `NAME.o`, links `NAME`, runs in qemu user mode;
      `baz` also writes the image `NAME-TARGET.bin`, which
      `run-rv32i-bin-as-elf.sh` runs in qemu user mode
    * `rv32i-qemu`: `baz` writes the image `NAME-TARGET.bin` (`NAME.s` is not
      assembled or linked), runs on the qemu `virt` machine
    * `rv32i-fpga`: `baz` writes the image `NAME-TARGET.bin` (`NAME.s` is not
      assembled or linked), runs on the fpga soft core emulator
  * `./run.sh myprogram.baz --checks=upper,line`
  * `./run.sh myprogram.baz --target=rv32i-qemu --stack=0x20000`
* a first program: `hello.baz` in the repository root with

  ```text
  dat hello = "hello\n"

  func main() {
      write(1, hello)
  }
  ```

  runs with `./run.sh hello.baz`, `./run.sh hello.baz --target=rv32i` and so
  on for each target; the program ends with exit code 0 at the end of `main`
* `baz` exits with 0 on success and 1 on an error
* [`etc/tutorial.baz`](etc/tutorial.baz) is a tour of the language from the
  easiest to the most difficult concepts
  * generics, type aliases, the zero value `{}`, `let` and `foo arr, count`
    are explained there; inside `foo` the names `e`, `i` and `n` hide outer
    names and cannot be declared in its body
* tests: after `./make.sh -O3 build`,
  `qa/coverage/test-coverage.sh --target=x86_64 run` runs the cases of
  [`qa/coverage/cases.sh`](qa/coverage/cases.sh) on one target (also `rv32i`,
  `rv32i-qemu`, `rv32i-fpga`; one run at a time, they share files) and
  `qa/coverage/test-cli.sh` the command line;
  [`qa/coverage/test-all.sh`](qa/coverage/test-all.sh) runs every test on
  every target with a coverage and sanitizer build that replaces `./baz` and
  is many times slower, coverage report in `qa/coverage/report/`
* neovim (lazyvim, see [`etc/nvim/tree-sitter-baz/`](etc/nvim/tree-sitter-baz/))
  with syntax highlighting and lsp (symbols, go to definition, rename,
  references)
* example application [`roome/src/main.baz`](roome/src/main.baz), a text
  adventure that `roome/run.sh` runs on the fpga soft core emulator; type
  `help` for the commands and `go home` to end it
* how the compiler works in [`etc/docs/README.md`](etc/docs/README.md), which
  starts with what a reviewer reads first; the order to read the sources of
  `src/` is in its chapter 1.1
* todo list of planned fixes and features in [`etc/todo.txt`](etc/todo.txt)

## Usage

```text
