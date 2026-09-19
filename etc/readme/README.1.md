# compiler-2: baz

Experimental compiler for a minimalistic, specialized language that targets NASM
x86_64 assembly on Linux.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by NASM for x86_64
* super-loop program with non-reentrant inlined functions

## Supports

* built-in integer types (64, 32, 16, 8 bit)
* built-in boolean type
* user defined types
* data
* variables
* constants
* arrays
* array iteration
* optional bounds checking at runtime
  * optional line number
* inlined functions
* keywords: `func`, `type`, `dat`, `var`, `const`, `foo`, `loop`, `if`, `else`,
  `continue`, `break`, `return`
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`,
  `address_of`, `equal`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
  code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
  run `prog.baz`
  * optional parameters: _stack size_, _bounds check_, with _line number
    information_ and _jump optimizations_ in boolean expression e.g:
    * `./run-baz.sh myprogram.baz --stack=262144`: stack size, no runtime checks
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper`: checks upper
      bounds without line number information and is often enough to ensure
      catching negative values (faster)
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper,line`: checks
      upper bounds with line number information
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper,lower,line`: checks
      bounds with line number information
    * option `--nopt` disables post processing jump optimizations in boolean
      expression
    * option `--no-reproduce` skips writing and verifying `diff.baz`;
      assembly source comments remain enabled
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Memory Layout

Initialized `dat` objects are stored in declaration order in `.data`, followed
by the 16-byte-aligned `.bss.stack` variable arena. `--stack` sets the arena's
reserved size; its zero-filled storage does not occupy executable file bytes.
Variables use positive offsets from `rbp`, which points to the start of data.
The compiler reserves `rbp` (including `ebp`, `bp`, and `bpl`); these registers
cannot be used directly or allocated to function parameters. `rsp` remains on
the OS-provided stack. Bounds-error reporting may overwrite `rbp` immediately
before exiting.

## Source

```text
