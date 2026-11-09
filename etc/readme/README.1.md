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
* support for reentrant non-inlined functions
* ub-free with `--checks=noub`
* basic support for generics

## Supports

* built-in integer types (64, 32, 16, 8 bit, 64 bit only on x86_64) and boolean,
  a boolean is not an integer, `int(b)` converts it to 1 or 0
* user defined types with methods and constructors
* data, constants, arrays and array iteration
* variables with the type deduced from the initializer
* string, character, user type and array initializers
* numbers in decimal, `0x` hex and `0b` binary; a leading zero is decimal,
  `010` is 10, not octal as in C
* inlined and non-inlined functions
* checks against undefined behavior, selected with `--checks=LIST`
  * `noub` turns on all of them, `line` adds the line number to the report of
    a failed bounds, division, shift, overlap or overflow check, `-NAME` turns
    a check off after the others are applied, e.g. `--checks=noub,-division`
  * `upper`, `lower`: array bounds at runtime; a constant index outside the
    array or a constant count that is negative or passes the end of the array
    is a compile error also without the checks, on an unsized array parameter
    when it passes the longest array the target addresses
  * `frame`: non-inlined function frame capacity at runtime
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
  * always on: compile time rejection of calls and assignments where the
    value may read the destination under another name (aliasing), of
    arguments that may share storage when a parameter is `mut`, and of an
    argument or operand that a call in a later one may write, e.g.
    `f(a[i], h(i))` with `h` writing `i`; evaluation is left to right
  * a failed check prints `panic: NAME` on a line of its own on standard error
    (the uart on the bare-metal targets), with `line` `panic: NAME at FILE:LINE`
    where FILE is the file name without its directory, and exits with the code
    of the check: `bounds` 255, `frame overflow` 254, `division` 253, `shift`
    252, `overlap` 251, `stack overflow` 250, `overflow` 249; a failed
    `sector_read` or `sector_write` prints `panic: sd card` and exits with 248
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
  space (no-break space, soft hyphen, zero width and bidi characters) as
  `\xNN`
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `read`,
  `write`, `sector_read`, `sector_write`, `exit`, `int`, `i8`, `i16`, `i32`, `i64`
* read and write sectors from and to file on `x86_64` and `rv32i`, the file
  (`--sdcard`) must exist and hold the sector; a sector of 32 bits or less,
  e.g. `i32(-2)`, is its value modulo 2^32 on every target
* read and write sectors from and to sd card on fpga hardware using `rv32i-fpga`

## Howto

* requirements: `clang++` 23 or later (tested with 23.1), `nasm`, `ld`,
  `llvm-mc`, `ld.lld`, `qemu-riscv32`, `qemu-system-riscv32`, `g++` (C++23)
  for the fpga emulator of `rv32i-fpga`; the tests need `python3` and
  `llvm-objcopy`, the coverage report `llvm-profdata` and `llvm-cov`
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
    * `rv32i`: assembles `NAME.o`, links `NAME`, runs in qemu user mode
    * `rv32i-qemu`: `baz` writes the image `NAME-TARGET.bin` (`NAME.s` is not
      assembled or linked), runs on the qemu `virt` machine
    * `rv32i-fpga`: `baz` writes the image `NAME-TARGET.bin` (`NAME.s` is not
      assembled or linked), runs on the fpga soft core emulator
  * `./run.sh myprogram.baz --checks=upper,line`
  * `./run.sh myprogram.baz --target=rv32i-qemu --stack=0x20000`
* `baz` exits with 0 on success and 1 on an error
* [`etc/tutorial.baz`](etc/tutorial.baz) is a tour of the language from the
  easiest to the most difficult concepts
  * generics, type aliases, the zero value `{}`, `let` and `foo arr, count`
    are explained there; inside `foo` the names `e`, `i` and `n` hide outer
    names and cannot be declared in its body
* [`qa/coverage/test-all.sh`](qa/coverage/test-all.sh) runs the tests, coverage
  report in `qa/coverage/report/`
* neovim (lazyvim, see [`etc/nvim/tree-sitter-baz/`](etc/nvim/tree-sitter-baz/))
  with syntax highlighting and lsp (symbols, go to definition, rename,
  references)
* example application [`roome/src/main.baz`](roome/src/main.baz)
* how the compiler works in [`etc/docs/README.md`](etc/docs/README.md)
* todo list of planned fixes and features in [`etc/todo.txt`](etc/todo.txt)

## Usage

```text
