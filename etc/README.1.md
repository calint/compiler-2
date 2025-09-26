# compiler-2: baz

Experimental compiler of minimalistic language to nasm x86_64 for Linux

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by nasm for x86_64
* super loop program with non-reentrant inlined functions
* support re-entrant non-inlined functions

## Supports

* built-in integer types (64, 32, 16, 8 bit)
* built-in boolean type
* user defined types
* inlined functions
* keywords: inline, func, field, var, loop, if, else, continue, break

## Howto

* to compile the compiler that compiles 'prog.baz' and assembles the generated
code run 'make.sh'
* after that use './run-baz.sh myprogram.baz' or './run-baz.sh' to compile and
run 'prog.baz'
* to run the tests 'qa/coverage/run-tests.sh' and see coverage report in 'qa/coverage/report/'

## Sample

```text
