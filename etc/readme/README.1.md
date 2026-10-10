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

* built-in integer types (64, 32, 16, 8 bit, 64 bit only on x86_64) and boolean
* user defined types with methods and constructors
* data, constants, arrays and array iteration
* variables with the type deduced from the initializer
* string, character, user type and array initializers
* inlined and non-inlined functions
* checks against undefined behavior, selected with `--checks=LIST`
  * `noub` turns on all of them, `line` adds the line number to the report of
    a failed bounds, division, shift, overlap or overflow check, `-NAME` turns
    a check off after the others are applied, e.g. `--checks=noub,-division`
  * `upper`, `lower`: array bounds at runtime
  * `frame`: non-inlined function frame capacity at runtime
  * `division`: division by zero and `MIN / -1` at runtime
  * `shift`: shift count outside the type width at runtime, a constant count
    is rejected at compile time
  * `overlap`: `array_copy` whose destination starts inside the source at
    runtime, a copy down the array is allowed
  * `stack`: stack capacity at `noinline` calls on `rv32i-qemu` and
    `rv32i-fpga` (`--memory` sets the memory of `rv32i-fpga`), the other
    targets run in an operating system that stops a program that overflows the
    stack
  * `overflow`: signed overflow of `+`, `-`, `*` and unary `-` at the width of
    the destination at runtime, a constant expression that overflows is
    rejected at compile time, the arithmetic of an explicit conversion such as
    `i8(a + b)` still wraps
  * `alias`: on by default, compile time rejection of calls and assignments
    where the value may read the destination under another name
  * always on: compile time rejection of arguments that may share storage when
    a parameter is `mut`
* keywords: `func`, `noinline`, `mut`, `type`, `dat`, `var`, `let`, `foo`,
  `loop`, `if`, `else`, `continue`, `break`, `return`, `self`, `and`, `or`,
  `not`, `true`, `false`, `include`
* `include "lib.baz"` at the top of a file adds the definitions of another file
  where it is, relative to the including file, each file once
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `read`,
  `write`, `exit`, `int`, `i8`, `i16`, `i32`, `i64`

## Howto

* [`./make.sh`](make.sh) compiles the compiler then compiles and runs
  [`prog.baz`](prog.baz), `./make.sh build` only compiles the compiler
* [`./run.sh`](run.sh)` [options] [NAME.baz]` compiles, assembles and runs
  `NAME.baz` (default: `prog.baz`), the options are passed to `baz`
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
* [`etc/tutorial.baz`](etc/tutorial.baz) is a tour of the language from the
  easiest to the most difficult concepts
* [`qa/coverage/test-all.sh`](qa/coverage/test-all.sh) runs the tests, coverage
  report in `qa/coverage/report/`
* neovim (lazyvim, see [`etc/nvim/tree-sitter-baz/`](etc/nvim/tree-sitter-baz/))
  with syntax highlighting and lsp (symbols, go to definition, rename,
  references)
* example application [`roome/src/main.baz`](roome/src/main.baz)
* todo list of planned fixes and features in [`etc/todo.txt`](etc/todo.txt)

## Usage

```text
