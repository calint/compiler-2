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
* opt-in checks that makes the language ub-free
* basic support for generics

## Supports

* built-in integer types (64, 32, 16, 8 bit, 64 bit only on x86_64)
* built-in boolean type
* user defined types
* data
* variables with the type deduced from the initializer
* constants
* arrays
* array iteration
* string, character, user type and array initializers
* opt-in checks against undefined behavior
  * array bounds at runtime, optionally reporting the line number
  * non-inlined function frame capacity at runtime
  * compile time rejection of calls where a result or argument may share
    storage
* inlined functions
* support for non-inlined functions
* methods and constructors on user defined types
* partial ub-free support
* basic support for generics
* keywords: `func`, `noinline`, `mut`, `type`, `dat`, `var`, `let`, `foo`,
  `loop`, `if`, `else`, `continue`, `break`, `return`, `self`, `and`, `or`,
  `not`
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `equal`, `read`,
  `write`, `exit`, `int`, `i8`, `i16`, `i32`, `i64`

## Howto

* `./make.sh` compiles the compiler then compiles and runs `prog.baz`,
  `./make.sh build` only compiles the compiler
* `./run.sh [options] [NAME.baz]` compiles, assembles and runs `NAME.baz`
  (default: `prog.baz`), the options are passed to `baz`
  * compiles to `NAME.s` and writes `NAME-without-comments.s`
  * builds the program depending on `--target`
    * `x86_64` (default): assembles `NAME.o` and links the binary `NAME`
    * `rv32i`: assembles `NAME.o` and links the binary `NAME`
    * `rv32i-qemu` and `rv32i-fpga`: `NAME.s` is not assembled or linked, `baz`
      write the image `NAME-TARGET.bin`
  * runs the program
    * `x86_64`: natively
    * `rv32i`: in qemu user mode
    * `rv32i-qemu`: on the qemu `virt` machine
    * `rv32i-fpga`: on the fpga soft core emulator
  * `./run.sh myprogram.baz --checks=upper,line`
  * `./run.sh myprogram.baz --target=rv32i-qemu --stack=0x20000`
* `tutorial.baz` is a tour of the language from the easiest to the most
  difficult concepts
* `qa/coverage/test-all.sh` runs the tests, coverage report in
  `qa/coverage/report/`
* neovim (specifically lazyvim see `etc/nvim/tree-sitter-baz/`)
  * syntax highlighting
  * lsp
    * symbols view
    * go to definition
    * rename
    * references
* example application `etc/roome/roome.baz`
* todo list of planned fixes and features in `etc/todo.txt`

## Usage

```text
