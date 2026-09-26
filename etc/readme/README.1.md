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
* limited support for non-inlined functions
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
* string and character literals: `var name[16] i8 = "baz\n"`, `'a'`, `'\x41'`
* optional bounds checking at runtime
  * optional line number
* inlined functions
* limited support for non-inlined functions: `func noinline name(...) { ... }`
* methods on user defined types: `func list.add(x) { ... }` is called as
  `lst.add(x)` with `lst` as the implicit parameter `self`
* partial ub-free support
* keywords: `func`, `type`, `dat`, `var`, `const`, `foo`, `loop`, `if`, `else`,
  `continue`, `break`, `return`
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `equal`, `read`,
  `write`, `exit`, `i8`, `i16`, `i32`, `i64`

## Howto

* to compile the compiler, then compile and run `prog.baz` run `./make.sh`
  (`./make.sh build` only compiles the compiler)
* after that use `./run.sh myprogram.baz` to compile, assemble and run a
  program, `./run.sh` alone uses `prog.baz`
  * writes `myprogram.s`, `myprogram-without-comments.s`, `myprogram.o` and the
    binary `myprogram`
  * optional parameters: _variable storage size_, _bounds check_, with _line number
    information_ and _jump optimizations_ e.g:
    * `./run.sh myprogram.baz --vars=262144`: reserves 262144 bytes for
      variables, no runtime checks
    * `./run.sh myprogram.baz --vars=262144 --checks=upper`: checks upper
      bounds without line number information and is often enough to ensure
      catching negative values (faster)
    * `./run.sh myprogram.baz --vars=262144 --checks=upper,line`: checks
      upper bounds with line number information
    * `./run.sh myprogram.baz --vars=262144 --checks=upper,lower,line`: checks
      bounds with line number information
    * option `--vars=SIZE` reserves variable storage in bytes (default: 65536,
      decimal or `0x` hex, positive multiple of 16)
    * option `--checks=TYPE` also accepts `frame` (non-inlined function frame
      capacity) and `alias` (calls where a result or argument may share
      storage)
    * option `--nopt` disables post processing jump optimizations
    * option `--reproduce-source` writes reproduced source to `diff.baz`
      and checks that it matches the input
    * to compile for rv32i and run in QEMU user mode use `--target=rv32i`
    * to compile a bare-metal image `gen-rv32i.bin` and run it on the QEMU
      `virt` machine use `--target=rv32i-qemu` (option `--stack=SIZE` sets
      the stack size, default: 65536, multiple of 16)
    * to compile a bare-metal image `gen-rv32i.bin` and run it in the fpga
      soft core emulator use `--target=rv32i-fpga`
* to run the tests `qa/coverage/test-all.sh` and see coverage report in
  `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Related

* rv32i soft core fpga implementation running `gen-rv32i.bin` compiled from
  `prog-uart.baz`
  * <https://github.com/calint/tang-nano-9k--riscv--cache-psram>
  * <https://github.com/calint/tang-nano-20k--riscv--cache-sdram>
  * to compile and run in an emulator of fpga soft core rv32i:
    `./run.sh prog-uart.baz --target=rv32i-fpga`
  * to run on hardware use `scripts/fpga-connect-serial.sh`

## Source

```text
