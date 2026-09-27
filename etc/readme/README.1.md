# compiler-2: baz

Experimental compiler for a minimalistic, specialized language targeting x86_64
(Linux) via NASM assembler and RV32I via the LLVM assembler running in QEMU
(Linux). For bare-metal RV32I (QEMU `virt` machine and an FPGA soft core) the
compiler writes the binary image itself.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler
* super-loop program with non-reentrant inlined functions
* support for non-inlined functions
* opt-in checks that make the language ub-free

## Supports

* built-in integer types (64, 32, 16, 8 bit)
* built-in boolean type
* user defined types
* data
* variables
* constants
* arrays
* array iteration
* string and character literals
* optional bounds checking at runtime
  * optional line number
* inlined functions
* limited support for non-inlined functions
* methods on user defined types
* partial ub-free support
* keywords: `func`, `type`, `dat`, `var`, `const`, `foo`, `loop`, `if`, `else`,
  `continue`, `break`, `return`
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `equal`, `read`,
  `write`, `exit`, `i8`, `i16`, `i32`, `i64`

## Howto

* `./make.sh` compiles the compiler then compiles and runs `prog.baz`,
  `./make.sh build` only compiles the compiler
* `./run.sh [options] [NAME.baz]` compiles, assembles and runs `NAME.baz`
  (default: `prog.baz`) passing options to `baz`, writes `NAME.s` and
  `NAME-without-comments.s`, x86_64 and rv32i also `NAME.o` and the binary
  `NAME`, rv32i targets run in qemu user mode, the qemu virt machine or the
  fpga soft core emulator
  * `./run.sh myprogram.baz --checks=upper,line`
  * `./run.sh myprogram.baz --target=rv32i-qemu --stack=0x20000`
* `qa/coverage/test-all.sh` runs the tests, coverage report in
  `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Usage

```text
