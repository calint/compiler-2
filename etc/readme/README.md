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
* inlined and non-inlined functions
* checks against undefined behavior, selected with `--checks=LIST`
  * `noub` turns on all of them, `line` adds the line number to the report of
    a failed bounds, division, shift, overlap or overflow check, `-NAME` turns
    a check off after the others are applied, e.g. `--checks=noub,-division`
  * `upper`, `lower`: array bounds at runtime; a constant index outside the
    array or a constant count that is negative or passes the end of the array
    is a compile error also without the checks
  * `frame`: non-inlined function frame capacity at runtime
  * `division`: division by zero and `MIN / -1` at runtime; a constant zero
    divisor is a compile error also without the check
  * `shift`: shift count outside the type width at runtime, bits shifted out
    by `<<` are discarded; a constant count outside the width is a compile
    error also without the check
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
  * always on: compile time rejection of calls and assignments where the
    value may read the destination under another name (aliasing), and of
    arguments that may share storage when a parameter is `mut`
* keywords: `func`, `noinline`, `mut`, `type`, `dat`, `var`, `let`, `foo`,
  `loop`, `if`, `else`, `continue`, `break`, `return`, `self`, `and`, `or`,
  `not`, `true`, `false`, `include`
* `include "lib.baz"` at the top of a file adds the definitions of another file
  where it is, relative to the including file, each file once
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `read`,
  `write`, `sector_read`, `sector_write`, `exit`, `int`, `i8`, `i16`, `i32`, `i64`
* read and write sectors from and to file on `x86_64` and `rv32i`
* read and write sectors from and to sd card on fpga hardware using `rv32i-fpga`

## Howto

* [`./make.sh`](make.sh) compiles the compiler then compiles and runs
  [`prog.baz`](prog.baz), `./make.sh build` only compiles the compiler
* [`./run.sh`](run.sh)`[options] [NAME.baz]` compiles, assembles and runs
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
* how the compiler works in [`etc/docs/README.md`](etc/docs/README.md)
* todo list of planned fixes and features in [`etc/todo.txt`](etc/todo.txt)

## Usage

```text
usage: ./baz [options] [file]
compiles file (default: prog.baz) to assembly on stdout, rv32i targets also
write a binary image

options:
  --target=MACHINE    x86_64 (default): linux, nasm
                      rv32i: linux, llvm assembler, qemu user mode
                      rv32i-qemu: bare-metal image for the qemu virt machine,
                        fails when code, data, variables and stack exceed its
                        128 MiB
                      rv32i-fpga: bare-metal image for the fpga soft core,
                        stack grows down from the end of the memory (see
                        --memory), fails when code, data, variables and stack
                        do not fit
  --vars=SIZE         variable storage in bytes, decimal or 0x hex, must be a
                      multiple of 16 (default: 65536)
  --stack=SIZE        rv32i-qemu and rv32i-fpga stack in bytes, decimal or 0x
                      hex, must be a multiple of 16, at least 48 and at
                      most the memory of the target (default: 65536)
  --memory=SIZE       rv32i-fpga memory in bytes, decimal or 0x hex, must be a
                      multiple of 4096 (default: 8388608), the emulator is
                      built for the default
  --checks=LIST       comma separated checks, replaces earlier --checks
  --report=LIST       comma separated reports after the code, replaces earlier
                      --report
  --bin=FILE          rv32i targets binary image (default: file without
                      extension followed by -MACHINE.bin)
  --sdcard=FILE       x86_64 and rv32i sd card image that 'sector_read' and
                      'sector_write' open at run time, relative to the working
                      directory of the program (default: sdcard.img)
  --nopt              no jump optimizations
  --versions=KEPT     of two versions of the same code keep the smaller
                      (default), or for testing the first or the last
  --reproduce-source  check that the source written back from the parse
                      matches the input, else write it to diff.baz
  --help, -h          this help

reports:
  registers  how the scratch registers are used at the busiest point: what
             each call frame holds, what a noinline frame would save and what
             each callee holds

checks:
  upper    runtime upper array bounds only, a negative index passes
  lower    runtime lower array bounds, catches negative indexes
  line     report line number on failed bounds, division, shift, overlap or
           overflow check
  frame    runtime non-inlined function frame capacity
  division runtime division by zero and 'MIN / -1', a constant 'MIN / -1' is
           rejected at compile time
  shift    runtime shift count below zero or not below the width of the type
  overlap  runtime 'array_copy' whose destination starts inside the source
  stack    runtime stack capacity at non-inlined calls on rv32i-qemu and
           rv32i-fpga, the other targets have an operating system that stops
           a program that overflows the stack
  overflow runtime signed overflow of '+', '-', '*' and unary '-', a constant
           expression that overflows is rejected at compile time
  noub     all checks against undefined behavior: upper, lower, frame,
           division, shift, overlap, stack and overflow; a program accepted
           with 'noub' has no undefined behavior, by default none is on;
           arguments, a result and globals that may share storage are
           rejected at compile time always
  -NAME    turns a check off after the others are applied, e.g.
           noub,-division, +NAME is NAME

examples:
  ./baz prog.baz > prog.s
  ./baz --vars=0x40000 --checks=upper prog.baz > prog.s
  ./baz --checks=upper,lower,line,frame prog.baz > prog.s
  ./baz --target=rv32i-qemu --stack=0x20000 prog.baz > prog.s
  ./baz --target=rv32i-fpga --checks=upper,lower prog.baz > prog.s
  ./baz --target=rv32i-fpga --memory=0x100000 prog.baz > prog.s
  ./baz --target=rv32i-qemu --bin=image.bin prog.baz > prog.s
```

## Related

* rv32i soft core fpga implementation running the binary image compiled with
  option `--target=rv32i-fpga`
  * <https://github.com/calint/tang-nano-9k--riscv--cache-psram>
  * <https://github.com/calint/tang-nano-20k--riscv--cache-sdram>

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    66           9263           6604          27350
C++                              1            228             88            851
-------------------------------------------------------------------------------
SUM:                            67           9491           6692          28201
-------------------------------------------------------------------------------
```

## Reflection

Some thoughts about the transition to AI-assisted development:
[reflection.md](etc/readme/reflection.md)

## Sample

```text
# user types are defined using keyword `type`

# built-in types are `i64`, `i32`, `i16`, `i8` and `bool`, `i64` only on x86_64

# default type is used if omitted (`i64` on x86_64 and `i32` on rv32i)

type point { x, y }

type object { pos point, color i32 }

type world { locations[8] }
# array of default integer type of target platform

type str {
    len i8,
    data i8[127],
    # trailing comma allowed
}

# initial data is declared before variables

dat   hello = "hello world from baz\n"
dat prompt1 = "enter name:\n"
dat prompt2 = "that is not a name.\n"
dat prompt3 = "hello "
dat     dot = "."
dat      nl = "\n"
dat   colon = ": "
dat    nums = [4]{ 1 } # remaining elements are zeroed
dat    str1 = str{ 3 } # remaining fields are zeroed
dat    str2 = str{ 3, "baz" } # a string initializes an `i8` array

dat greeted = "names greeted: "
dat   names = 0

# default is to inline functions

func assert(ok bool) { if not ok exit(1) }
# `exit` is a built-in function

func print(str i8[]) {
    write(1, str)
    # `write` is a built-in function that operates on file descriptors
    # it has 2 more optional arguments: count and start index
}

func mut point.fooz() {
    self.x = 0b10    # binary value 2
    self.y = 0xb     # hex value 11
}
# functions can act on user types: `func point.fooz()` is called as `p.fooz()`
# a function that writes to `self` is declared as `mut`

func point.sum() res {
    res = self.x + self.y
}
# methods can have a "return"

func bar(arg mut) {
    if arg == 0 return
    arg = 0xff
}
# default function argument type is `i64` on x86_64 and `i32` on rv32i
# function arguments are read-only references, `mut` allows writing them

func baz(arg) res {
    res = arg * 2
}
# return is a reference to the target with optional type
# it is accessed as a variable, in this case `res`

func inv(i i32) res i32 {
    res = ~i
}
# type of "return" and arguments can be defined, use `int` for default integer
# type of target platform

func faz(arg mut i32[]) {
    arg[1] = 0xfe
}
# array arguments are declared with the element type followed by `[]`

func foz(arg mut []) {
    arg[1] = 0xfe
}
# array without type specified defaults to target default integer type

func mut str.input() {
    var nbytes = read(0, self.data)
    # read is built-in function that operates on file descriptors
    # it has 2 more optional arguments: count and start index
    self.len = i8(nbytes - 1)
}

func str.print() {
    write(1, self.data, self.len)
}

func greet(name str) {
    print(prompt3)
    name.print()
    print(dot)
    print(nl)
    names = names + 1
}
# data is global, any function can read and assign it

func point.at(x, y) self {
    self.x = x
    self.y = y
}
# a constructor builds its result `self` and must assign every field

func mut point.x(x) {
    self.x = x
}
# types can have methods with same name as fields

func object.at(x, y, color i32) self {
    self.pos = point.at(x, y)
    self.color = color
}

let yes = 1
let no = 0
let maybe = -1
# constants can be declared in any scope and shadow outer declarations, not a
# parameter or the result in the body of its function

# non-inlined functions are called with the same type and aliasing rules as
# inlined functions
# arguments and "return" are references to memory locations
# an array argument gets a body for each array length

func noinline print_num(num) {
    # 19 digits of an i64 plus the sign
    let buf_count = 20

    var buf = i8[buf_count]
    var n = num
    var is_negative = false

    # digits are taken from the negative value because the most negative i64
    # has no positive counterpart
    if n < 0 {
        is_negative = true
    }
    if n > 0 {
        n = -n
    }

    var i = buf_count
    loop {
        i = i - 1
        buf[i] = i8('0' - n % 10)
        n = n / 10
        if n == 0 break
    }

    if is_negative {
        i = i - 1
        buf[i] = '-'
    }

    var write_pos = 0
    loop {
        buf[write_pos] = buf[i]
        write_pos = write_pos + 1
        i = i + 1
        if i == buf_count break
    }

    write(1, buf, write_pos)
}

func noinline factorial(n) res {
    res = 1
    if n <= 1 return

    var m = n - 1
    var partial = factorial(m)
    res = n * partial
}
# a non-inlined function can call itself
# `return` leaves early, "return" must be assigned on every path

func main() {
    var answer = 0
    # variables must have initializer
    assert(answer == 0)

    answer = maybe
    assert(answer == -1)

    {
        # a code block opens a new scope
        # constants and variables shadow outer scope
        let maybe = 33
        assert(maybe == 33)
    }

    assert(maybe == -1)

    assert(nums[0] == 1 and nums[3] == 0)
    assert(str2.len == 3 and str2.data[2] == 'z')

    var a = 7
    assert(a & 3 == 3)
    assert(a | 8 == 15)
    assert(a ^ 1 == 6)
    assert(a << 2 == 28)
    assert(-a >> 1 == -4)
    # bitwise and, or, xor and shifts, `>>` keeps the sign

    assert(a + a << 1 == 21)
    assert((a + a) << 1 == 28)
    # unlike c, bitwise operations and shifts bind tighter than arithmetic
    # precedence from lowest to highest: `+ -`, `* / %`, `|`, `&`, `^`,
    # `<< >>`

    assert(a != 0 and (a >= 7 or a < 0))
    # `and`, `or` and `not` stop evaluating as soon as the result is known

    var small = i8(100)
    small = i8(small + small)
    assert(small == -56)
    # `i8(100)` gives the variable type `i8`, the arithmetic of a conversion
    # wraps at its width

    var wide = int(i16(small))
    assert(wide == -56)
    # `int(x)` converts to the default type

#   small = wide
#   compile time error because the value might not fit, narrowing must be
#   acknowledged with a conversion

    var letter = '\x41'
    assert(letter == 'A')
    # a character literal is a byte value, escapes `\n` and `\x41` are supported

    var arr = i32[4]

    var ix = 1
    arr[ix] = 2
    arr[ix + 1] = arr[ix]
    assert(arr[1] == 2)
    assert(arr[2] == 2)

    array_copy(arr[2], arr, 2)
    assert(arr[0] == 2)
    # `array_copy` is a built-in function: copy from, to, number of elements

    var arr1 = i32[8]
    array_copy(arr, arr1, 4)
    var eq = arrays_equal(arr[1], arr1[1], 3)
    # type `bool` is built-in and deduced from expression type
    # `arrays_equal` is built-in function comparing source and destination
    assert(eq)

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

    var arr4 = arr
    assert(arr == arr4)
    # initializing from an array copies it, `==` compares same size arrays

#   arr[ix] = ~inv(arr[ix - 1])
#   rejected because "return" and the argument may share storage

    ix = 3
    var tmp = ~inv(arr[ix - 1])
    arr[ix] = tmp
    assert(arr[ix] == 2)

    faz(arr)
    assert(arr[1] == 0xfe)

    var arr3 = []{ 3, 5 }
    foo arr3 {
        e = e + i + n
    }
    assert(arr3[0] == 3 + 0 + 2)
    assert(arr3[1] == 5 + 1 + 2)
    # `foo` is a language construct that iterates over an array injecting:
    #   `e`: current element
    #   `i`: index starting at 0
    #   `n`: constant array size

    var p = point

    p.fooz()
    # call on user type method

    assert(p.x == 2)
    assert(p.y == 0xb)

    var q = p
    # user type initializer may be an expression

    assert(p == q)
    # `==` and `!=` compare the bytes of user types and of same size arrays,
    # the padding between fields is zero so equal values compare equal

    q.x = 3
    assert(p != q)

    var i = 0
    bar(i)
    assert(i == 0)

    i = 1
    bar(i)
    assert(i == 0xff)

    var j = 1
    var k = baz(j)
    assert(k == 2)

    k = baz(1)
    assert(k == 2)

    var five = 5
    var f = factorial(five)
    assert(f == 120)

    var p0 = point{baz(3), 0}
    assert(p0.x == 6)

    var pt = point.at(-1, -2)
    # a constructor builds its result `self` in the destination
    # `pt` has the type of the constructor

    assert(pt.x == -1)
    assert(pt.y == -2)

    pt.x(2)
    assert(pt.x == 2)
    assert(pt.sum() == 0)

    var x = 1
    var y = 2

    var o1 = object{{x * 10, y}, 0xff0000}
    assert(o1.pos.x == 10)
    assert(o1.pos.y == 2)
    assert(o1.color == 0xff0000)

    var p1 = point{-x, -y}
    o1.pos = p1
    assert(o1.pos.x == -1)
    assert(o1.pos.y == -2)

    var o2 = o1
    assert(o2.pos.x == -1)
    assert(o2.pos.y == -2)
    assert(o2.color == 0xff0000)

    o2.pos = {x, y}
    assert(o2.pos.x == 1)
    o2.pos = {y, x}
    assert(o2.pos.x == 2)
    # an assigned literal takes its type from the destination and names none

#   o2.pos = {o2.pos.y, o2.pos.x}
#   compile time error because `o2.pos.x` is read after the first field
#   overwrote it

    var o3 = object[2]
    o3[0].pos.y = 73

    assert(o3[0].pos.y == 73)
    o3[1] = object.at(2, 74, 0xffffff)
    assert(o3[1].pos.y == 74)

    o3[1].pos.fooz()
    assert(o3[1].pos.sum() == 13)
    # methods can be called on fields and array elements

    var worlds = world[8]
    worlds[1].locations[1] = 0xffee
    assert(worlds[1].locations[1] == 0xffee)

    array_copy(
        worlds[1].locations,
        worlds[0].locations,
        array_length(worlds[0].locations)
    )
    # `array_length` is built-in

    assert(worlds[0].locations[1] == 0xffee)
    assert(arrays_equal(
             worlds[0].locations,
             worlds[1].locations,
             array_length(worlds[0].locations)
          ))

    var arr2 = []{ -1, 2 }
    assert(array_length(arr2) == 2)
    assert(arr2[0] == -1)
    assert(arr2[1] == 2)

    var counter = 0
    var nm = str
    print(hello)
    loop {
        counter = counter + 1
        print_num(counter)
        print(colon)
        print(prompt1)
        nm.input()
        # ctrl-d reads no bytes and gives -1
        if nm.len <= 0 {
            break
        } else if nm.len <= 4 {
            print(prompt2)
            continue
        } else {
            greet(nm)
        }
    }

    print(greeted)
    print_num(names)
    print(nl)

    var bye = "bye from baz\n"
    write(1, bye, 3)
    write(1, bye, 1, array_length(bye) - 1)
    # a string initializes an `i8` array of its size
    # writes 3 elements, then 1 element from the last index
}

```

## Generates

```nasm
default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 384], 0
    cmp.190.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.190.12.end:
    func.assert..190.5:
        if.38.24.190.5:
        cmp.38.27.190.5:
        test r15b, r15b
        jne if.38.24.190.5.end
        if.38.24.190.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.190.5.end:
    func.assert..190.5.end:
    mov qword [rbp + 384], -1
    cmp.193.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.193.12.end:
    func.assert..193.5:
        if.38.24.193.5:
        cmp.38.27.193.5:
        test r15b, r15b
        jne if.38.24.193.5.end
        if.38.24.193.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.193.5.end:
    func.assert..193.5.end:
        func.assert..199.9:
            if.38.24.199.9:
            cmp.38.27.199.9:
            if.38.24.199.9.end:
        func.assert..199.9.end:
    func.assert..202.5:
        if.38.24.202.5:
        cmp.38.27.202.5:
        if.38.24.202.5.end:
    func.assert..202.5.end:
    cmp.204.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.204.12.end
    cmp.204.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.204.12.end:
    func.assert..204.5:
        if.38.24.204.5:
        cmp.38.27.204.5:
        test r15b, r15b
        jne if.38.24.204.5.end
        if.38.24.204.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.204.5.end:
    func.assert..204.5.end:
    cmp.205.12:
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.205.12.end
    cmp.205.30:
    cmp byte [rbp + 227], 122
    sete r15b
    bool.205.12.end:
    func.assert..205.5:
        if.38.24.205.5:
        cmp.38.27.205.5:
        test r15b, r15b
        jne if.38.24.205.5.end
        if.38.24.205.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.205.5.end:
    func.assert..205.5.end:
    mov qword [rbp + 392], 7
    cmp.208.12:
        mov r14, qword [rbp + 392]
        and r14, 3
    cmp r14, 3
    sete r15b
    bool.208.12.end:
    func.assert..208.5:
        if.38.24.208.5:
        cmp.38.27.208.5:
        test r15b, r15b
        jne if.38.24.208.5.end
        if.38.24.208.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.208.5.end:
    func.assert..208.5.end:
    cmp.209.12:
        mov r14, qword [rbp + 392]
        or r14, 8
    cmp r14, 15
    sete r15b
    bool.209.12.end:
    func.assert..209.5:
        if.38.24.209.5:
        cmp.38.27.209.5:
        test r15b, r15b
        jne if.38.24.209.5.end
        if.38.24.209.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.209.5.end:
    func.assert..209.5.end:
    cmp.210.12:
        mov r14, qword [rbp + 392]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.210.12.end:
    func.assert..210.5:
        if.38.24.210.5:
        cmp.38.27.210.5:
        test r15b, r15b
        jne if.38.24.210.5.end
        if.38.24.210.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.210.5.end:
    func.assert..210.5.end:
    cmp.211.12:
        mov r14, qword [rbp + 392]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.211.12.end:
    func.assert..211.5:
        if.38.24.211.5:
        cmp.38.27.211.5:
        test r15b, r15b
        jne if.38.24.211.5.end
        if.38.24.211.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.211.5.end:
    func.assert..211.5.end:
    cmp.212.12:
        mov r14, qword [rbp + 392]
        neg r14
        jo baz_overflow_line_212
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.212.12.end:
    func.assert..212.5:
        if.38.24.212.5:
        cmp.38.27.212.5:
        test r15b, r15b
        jne if.38.24.212.5.end
        if.38.24.212.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.212.5.end:
    func.assert..212.5.end:
    cmp.215.12:
        mov r14, qword [rbp + 392]
        mov r13, qword [rbp + 392]
        sal r13, 1
        add r14, r13
        jo baz_overflow_line_215
    cmp r14, 21
    sete r15b
    bool.215.12.end:
    func.assert..215.5:
        if.38.24.215.5:
        cmp.38.27.215.5:
        test r15b, r15b
        jne if.38.24.215.5.end
        if.38.24.215.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.215.5.end:
    func.assert..215.5.end:
    cmp.216.12:
        mov r14, qword [rbp + 392]
        add r14, qword [rbp + 392]
        jo baz_overflow_line_216
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.216.12.end:
    func.assert..216.5:
        if.38.24.216.5:
        cmp.38.27.216.5:
        test r15b, r15b
        jne if.38.24.216.5.end
        if.38.24.216.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.216.5.end:
    func.assert..216.5.end:
    cmp.221.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.221.12.end
    cmp.221.23:
    cmp.221.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.221.12.end
    cmp.221.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.221.12.end:
    func.assert..221.5:
        if.38.24.221.5:
        cmp.38.27.221.5:
        test r15b, r15b
        jne if.38.24.221.5.end
        if.38.24.221.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.221.5.end:
    func.assert..221.5.end:
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.226.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.226.12.end:
    func.assert..226.5:
        if.38.24.226.5:
        cmp.38.27.226.5:
        test r15b, r15b
        jne if.38.24.226.5.end
        if.38.24.226.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.226.5.end:
    func.assert..226.5.end:
    movsx r15w, byte [rbp + 400]
    movsx r15, r15w
    mov qword [rbp + 408], r15
    cmp.231.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.231.12.end:
    func.assert..231.5:
        if.38.24.231.5:
        cmp.38.27.231.5:
        test r15b, r15b
        jne if.38.24.231.5.end
        if.38.24.231.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.231.5.end:
    func.assert..231.5.end:
    mov qword [rbp + 416], 65
    cmp.239.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.239.12.end:
    func.assert..239.5:
        if.38.24.239.5:
        cmp.38.27.239.5:
        test r15b, r15b
        jne if.38.24.239.5.end
        if.38.24.239.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.239.5.end:
    func.assert..239.5.end:
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
    mov qword [rbp + 440], 1
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_245
    mov dword [rbp + r15 * 4 + 424], 2
    mov r15, qword [rbp + 440]
    inc r15
    jo baz_overflow_line_246
    cmp r15, 4
    jae baz_bounds_line_246
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_246
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
    cmp.247.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.247.12.end:
    func.assert..247.5:
        if.38.24.247.5:
        cmp.38.27.247.5:
        test r15b, r15b
        jne if.38.24.247.5.end
        if.38.24.247.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.247.5.end:
    func.assert..247.5.end:
    cmp.248.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.248.12.end:
    func.assert..248.5:
        if.38.24.248.5:
        cmp.38.27.248.5:
        test r15b, r15b
        jne if.38.24.248.5.end
        if.38.24.248.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.248.5.end:
    func.assert..248.5.end:
    lea rsi, [rbp + 432]
    lea rdi, [rbp + 424]
    mov rcx, 2
    sal rcx, 2
    mov r15, rdi
    sub r15, rsi
    je .Lbaz_overlap.0
    cmp r15, rcx
    jb baz_overlap_line_250
    .Lbaz_overlap.0:
    rep movsb
    cmp.251.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.251.12.end:
    func.assert..251.5:
        if.38.24.251.5:
        cmp.38.27.251.5:
        test r15b, r15b
        jne if.38.24.251.5.end
        if.38.24.251.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.251.5.end:
    func.assert..251.5.end:
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
    mov r15, qword [rbp + 424]
    mov qword [rbp + 448], r15
    mov r15, qword [rbp + 432]
    mov qword [rbp + 456], r15
    lea rsi, [rbp + 428]
    lea rdi, [rbp + 452]
    cmpsq
    jne .Lbaz_equal.1
    cmpsd
    .Lbaz_equal.1:
    sete byte [rbp + 480]
    func.assert..259.5:
        if.38.24.259.5:
        cmp.38.27.259.5:
        cmp byte [rbp + 480], 0
        jne if.38.24.259.5.end
        if.38.24.259.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.259.5.end:
    func.assert..259.5.end:
    mov dword [rbp + 456], -1
    cmp.262.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 448]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        sete r15b
    test r15b, r15b
    sete r15b
    bool.262.12.end:
    func.assert..262.5:
        if.38.24.262.5:
        cmp.38.27.262.5:
        test r15b, r15b
        jne if.38.24.262.5.end
        if.38.24.262.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.262.5.end:
    func.assert..262.5.end:
    mov r15, qword [rbp + 424]
    mov qword [rbp + 484], r15
    mov r15, qword [rbp + 432]
    mov qword [rbp + 492], r15
    cmp.265.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.3
        cmpsq
        .Lbaz_equal.3:
        sete r15b
    bool.265.12.end:
    func.assert..265.5:
        if.38.24.265.5:
        cmp.38.27.265.5:
        test r15b, r15b
        jne if.38.24.265.5.end
        if.38.24.265.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.265.5.end:
    func.assert..265.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    dec r15
    jo baz_overflow_line_272
    cmp r15, 4
    jae baz_bounds_line_272
    func.inv..272.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv..272.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_273
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.274.12:
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_274
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
    bool.274.12.end:
    func.assert..274.5:
        if.38.24.274.5:
        cmp.38.27.274.5:
        test r15b, r15b
        jne if.38.24.274.5.end
        if.38.24.274.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.274.5.end:
    func.assert..274.5.end:
    func.faz..276.5:
        mov dword [rbp + 428], 254
    func.faz..276.5.end:
    cmp.277.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.277.12.end:
    func.assert..277.5:
        if.38.24.277.5:
        cmp.38.27.277.5:
        test r15b, r15b
        jne if.38.24.277.5.end
        if.38.24.277.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.277.5.end:
    func.assert..277.5.end:
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov r14, 0
    foo.280.5:
        add qword [r15], r14
        jo baz_overflow_line_281
        add qword [r15], 2
        jo baz_overflow_line_281
        foo.280.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.280.5
    foo.280.5.end:
    cmp.283.12:
    cmp qword [rbp + 504], 5
    sete r15b
    bool.283.12.end:
    func.assert..283.5:
        if.38.24.283.5:
        cmp.38.27.283.5:
        test r15b, r15b
        jne if.38.24.283.5.end
        if.38.24.283.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.283.5.end:
    func.assert..283.5.end:
    cmp.284.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.284.12.end:
    func.assert..284.5:
        if.38.24.284.5:
        cmp.38.27.284.5:
        test r15b, r15b
        jne if.38.24.284.5.end
        if.38.24.284.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.284.5.end:
    func.assert..284.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz..292.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz..292.7.end:
    cmp.295.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.295.12.end:
    func.assert..295.5:
        if.38.24.295.5:
        cmp.38.27.295.5:
        test r15b, r15b
        jne if.38.24.295.5.end
        if.38.24.295.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.295.5.end:
    func.assert..295.5.end:
    cmp.296.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.296.12.end:
    func.assert..296.5:
        if.38.24.296.5:
        cmp.38.27.296.5:
        test r15b, r15b
        jne if.38.24.296.5.end
        if.38.24.296.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.296.5.end:
    func.assert..296.5.end:
    mov r15, qword [rbp + 520]
    mov qword [rbp + 536], r15
    mov r15, qword [rbp + 528]
    mov qword [rbp + 544], r15
    cmp.301.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.4
        cmpsq
        .Lbaz_equal.4:
        sete r15b
    bool.301.12.end:
    func.assert..301.5:
        if.38.24.301.5:
        cmp.38.27.301.5:
        test r15b, r15b
        jne if.38.24.301.5.end
        if.38.24.301.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.301.5.end:
    func.assert..301.5.end:
    mov qword [rbp + 536], 3
    cmp.306.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.5
        cmpsq
        .Lbaz_equal.5:
        setne r15b
    bool.306.12.end:
    func.assert..306.5:
        if.38.24.306.5:
        cmp.38.27.306.5:
        test r15b, r15b
        jne if.38.24.306.5.end
        if.38.24.306.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.306.5.end:
    func.assert..306.5.end:
    mov qword [rbp + 552], 0
    func.bar..309.5:
        if.60.5.309.5:
        cmp.60.8.309.5:
        cmp qword [rbp + 552], 0
        je func.bar..309.5.end
        if.60.5.309.5.code:
        if.60.5.309.5.end:
        mov qword [rbp + 552], 255
    func.bar..309.5.end:
    cmp.310.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.310.12.end:
    func.assert..310.5:
        if.38.24.310.5:
        cmp.38.27.310.5:
        test r15b, r15b
        jne if.38.24.310.5.end
        if.38.24.310.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.310.5.end:
    func.assert..310.5.end:
    mov qword [rbp + 552], 1
    func.bar..313.5:
        if.60.5.313.5:
        cmp.60.8.313.5:
        cmp qword [rbp + 552], 0
        je func.bar..313.5.end
        if.60.5.313.5.code:
        if.60.5.313.5.end:
        mov qword [rbp + 552], 255
    func.bar..313.5.end:
    cmp.314.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.314.12.end:
    func.assert..314.5:
        if.38.24.314.5:
        cmp.38.27.314.5:
        test r15b, r15b
        jne if.38.24.314.5.end
        if.38.24.314.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.314.5.end:
    func.assert..314.5.end:
    mov qword [rbp + 560], 1
    func.baz..317.13:
        mov r15, qword [rbp + 560]
        imul r15, 2
        jo baz_overflow_line_67
        mov qword [rbp + 568], r15
    func.baz..317.13.end:
    cmp.318.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.318.12.end:
    func.assert..318.5:
        if.38.24.318.5:
        cmp.38.27.318.5:
        test r15b, r15b
        jne if.38.24.318.5.end
        if.38.24.318.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.318.5.end:
    func.assert..318.5.end:
    func.baz..320.9:
        mov qword [rbp + 568], 2
    func.baz..320.9.end:
    cmp.321.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.321.12.end:
    func.assert..321.5:
        if.38.24.321.5:
        cmp.38.27.321.5:
        test r15b, r15b
        jne if.38.24.321.5.end
        if.38.24.321.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.321.5.end:
    func.assert..321.5.end:
    mov qword [rbp + 576], 5
    mov r15, strict qword vars.end - size.func.factorial - 592
    cmp r15, rbp
    jl baz_frame_overflow
    lea r15, [rbp + 584]
    mov qword [rbp + 592], r15
    lea r15, [rbp + 576]
    mov qword [rbp + 600], r15
    lea rbx, [rbp + 592]
    call func.factorial
    cmp.325.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.325.12.end:
    func.assert..325.5:
        if.38.24.325.5:
        cmp.38.27.325.5:
        test r15b, r15b
        jne if.38.24.325.5.end
        if.38.24.325.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.325.5.end:
    func.assert..325.5.end:
    func.baz..327.20:
        mov qword [rbp + 592], 6
    func.baz..327.20.end:
    mov qword [rbp + 600], 0
    cmp.328.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.328.12.end:
    func.assert..328.5:
        if.38.24.328.5:
        cmp.38.27.328.5:
        test r15b, r15b
        jne if.38.24.328.5.end
        if.38.24.328.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.328.5.end:
    func.assert..328.5.end:
    func.point.at..330.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at..330.14.end:
    cmp.334.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.334.12.end:
    func.assert..334.5:
        if.38.24.334.5:
        cmp.38.27.334.5:
        test r15b, r15b
        jne if.38.24.334.5.end
        if.38.24.334.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.334.5.end:
    func.assert..334.5.end:
    cmp.335.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.335.12.end:
    func.assert..335.5:
        if.38.24.335.5:
        cmp.38.27.335.5:
        test r15b, r15b
        jne if.38.24.335.5.end
        if.38.24.335.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.335.5.end:
    func.assert..335.5.end:
    func.point.x..337.8:
        mov qword [rbp + 608], 2
    func.point.x..337.8.end:
    cmp.338.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.338.12.end:
    func.assert..338.5:
        if.38.24.338.5:
        cmp.38.27.338.5:
        test r15b, r15b
        jne if.38.24.338.5.end
        if.38.24.338.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.338.5.end:
    func.assert..338.5.end:
    cmp.339.12:
        func.point.sum..339.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
            jo baz_overflow_line_55
        func.point.sum..339.15.end:
    test r14, r14
    sete r15b
    bool.339.12.end:
    func.assert..339.5:
        if.38.24.339.5:
        cmp.38.27.339.5:
        test r15b, r15b
        jne if.38.24.339.5.end
        if.38.24.339.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.339.5.end:
    func.assert..339.5.end:
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    jo baz_overflow_line_344
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.345.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.345.12.end:
    func.assert..345.5:
        if.38.24.345.5:
        cmp.38.27.345.5:
        test r15b, r15b
        jne if.38.24.345.5.end
        if.38.24.345.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.345.5.end:
    func.assert..345.5.end:
    cmp.346.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.346.12.end:
    func.assert..346.5:
        if.38.24.346.5:
        cmp.38.27.346.5:
        test r15b, r15b
        jne if.38.24.346.5.end
        if.38.24.346.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.346.5.end:
    func.assert..346.5.end:
    cmp.347.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.347.12.end:
    func.assert..347.5:
        if.38.24.347.5:
        cmp.38.27.347.5:
        test r15b, r15b
        jne if.38.24.347.5.end
        if.38.24.347.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.347.5.end:
    func.assert..347.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
    neg qword [rbp + 664]
    jo baz_overflow_line_349
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
    neg qword [rbp + 672]
    jo baz_overflow_line_349
    mov r15, qword [rbp + 664]
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 672]
    mov qword [rbp + 648], r15
    cmp.351.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.351.12.end:
    func.assert..351.5:
        if.38.24.351.5:
        cmp.38.27.351.5:
        test r15b, r15b
        jne if.38.24.351.5.end
        if.38.24.351.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.351.5.end:
    func.assert..351.5.end:
    cmp.352.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.352.12.end:
    func.assert..352.5:
        if.38.24.352.5:
        cmp.38.27.352.5:
        test r15b, r15b
        jne if.38.24.352.5.end
        if.38.24.352.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.352.5.end:
    func.assert..352.5.end:
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.355.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.355.12.end:
    func.assert..355.5:
        if.38.24.355.5:
        cmp.38.27.355.5:
        test r15b, r15b
        jne if.38.24.355.5.end
        if.38.24.355.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.355.5.end:
    func.assert..355.5.end:
    cmp.356.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.356.12.end:
    func.assert..356.5:
        if.38.24.356.5:
        cmp.38.27.356.5:
        test r15b, r15b
        jne if.38.24.356.5.end
        if.38.24.356.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.356.5.end:
    func.assert..356.5.end:
    cmp.357.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.357.12.end:
    func.assert..357.5:
        if.38.24.357.5:
        cmp.38.27.357.5:
        test r15b, r15b
        jne if.38.24.357.5.end
        if.38.24.357.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.357.5.end:
    func.assert..357.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.360.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.360.12.end:
    func.assert..360.5:
        if.38.24.360.5:
        cmp.38.27.360.5:
        test r15b, r15b
        jne if.38.24.360.5.end
        if.38.24.360.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.360.5.end:
    func.assert..360.5.end:
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.362.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.362.12.end:
    func.assert..362.5:
        if.38.24.362.5:
        cmp.38.27.362.5:
        test r15b, r15b
        jne if.38.24.362.5.end
        if.38.24.362.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.362.5.end:
    func.assert..362.5.end:
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.372.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.372.12.end:
    func.assert..372.5:
        if.38.24.372.5:
        cmp.38.27.372.5:
        test r15b, r15b
        jne if.38.24.372.5.end
        if.38.24.372.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.372.5.end:
    func.assert..372.5.end:
    mov dword [rbp + 748], 0
    func.object.at..373.13:
        func.point.at..120.16.373.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at..120.16.373.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at..373.13.end:
    cmp.374.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.374.12.end:
    func.assert..374.5:
        if.38.24.374.5:
        cmp.38.27.374.5:
        test r15b, r15b
        jne if.38.24.374.5.end
        if.38.24.374.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.374.5.end:
    func.assert..374.5.end:
    func.point.fooz..376.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz..376.15.end:
    cmp.377.12:
        func.point.sum..377.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
            jo baz_overflow_line_55
        func.point.sum..377.22.end:
    cmp r14, 13
    sete r15b
    bool.377.12.end:
    func.assert..377.5:
        if.38.24.377.5:
        cmp.38.27.377.5:
        test r15b, r15b
        jne if.38.24.377.5.end
        if.38.24.377.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.377.5.end:
    func.assert..377.5.end:
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.382.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.382.12.end:
    func.assert..382.5:
        if.38.24.382.5:
        cmp.38.27.382.5:
        test r15b, r15b
        jne if.38.24.382.5.end
        if.38.24.382.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.382.5.end:
    func.assert..382.5.end:
    lea rsi, [rbp + 816]
    lea rdi, [rbp + 752]
    mov rcx, 8
    sal rcx, 3
    mov r15, rdi
    sub r15, rsi
    je .Lbaz_overlap.6
    cmp r15, rcx
    jb baz_overlap_line_384
    .Lbaz_overlap.6:
    rep movsb
    cmp.391.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.391.12.end:
    func.assert..391.5:
        if.38.24.391.5:
        cmp.38.27.391.5:
        test r15b, r15b
        jne if.38.24.391.5.end
        if.38.24.391.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.391.5.end:
    func.assert..391.5.end:
    lea rsi, [rbp + 752]
    lea rdi, [rbp + 816]
    mov rcx, 8
    repe cmpsq
    sete r15b
    func.assert..392.5:
        if.38.24.392.5:
        cmp.38.27.392.5:
        test r15b, r15b
        jne if.38.24.392.5.end
        if.38.24.392.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.392.5.end:
    func.assert..392.5.end:
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    func.assert..399.5:
        if.38.24.399.5:
        cmp.38.27.399.5:
        if.38.24.399.5.end:
    func.assert..399.5.end:
    cmp.400.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.400.12.end:
    func.assert..400.5:
        if.38.24.400.5:
        cmp.38.27.400.5:
        test r15b, r15b
        jne if.38.24.400.5.end
        if.38.24.400.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.400.5.end:
    func.assert..400.5.end:
    cmp.401.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.401.12.end:
    func.assert..401.5:
        if.38.24.401.5:
        cmp.38.27.401.5:
        test r15b, r15b
        jne if.38.24.401.5.end
        if.38.24.401.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.401.5.end:
    func.assert..401.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print..405.5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    func.print..405.5.end:
    loop.406.5:
        inc qword [rbp + 1280]
        jo baz_overflow_line_407
        mov r15, strict qword vars.end - size.func.print_num - 1416
        cmp r15, rbp
        jl baz_frame_overflow
        lea r15, [rbp + 1280]
        mov qword [rbp + 1416], r15
        lea rbx, [rbp + 1416]
        call func.print_num
        func.print..409.9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        func.print..409.9.end:
        func.print..410.9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        func.print..410.9.end:
        func.str.input..411.12:
            mov rdi, 0
            lea rsi, [rbp + 1289]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15, qword [rbp + 1416]
            dec r15
            mov byte [rbp + 1288], r15b
        func.str.input..411.12.end:
        if.413.9:
        cmp.413.12:
        cmp byte [rbp + 1288], 0
        jle loop.406.5.end
        if.413.9.code:
        if.413.9.1:
        cmp.415.19:
        cmp byte [rbp + 1288], 4
        jg if.413.9.else
        if.413.9.1.code:
            func.print..416.13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            func.print..416.13.end:
            jmp loop.406.5
        if.413.9.else:
            func.greet..419.13:
                func.print..100.5.419.13:
                    mov rdi, 1
                    lea rsi, [rbp + 53]
                    mov rdx, 6
                    mov rax, 1
                    syscall
                func.print..100.5.419.13.end:
                func.str.print..101.10.419.13:
                    mov rdi, 1
                    lea rsi, [rbp + 1289]
                    movsx rdx, byte [rbp + 1288]
                    cmp rdx, 127
                    ja baz_bounds_line_96
                    mov rax, 1
                    syscall
                func.str.print..101.10.419.13.end:
                func.print..102.5.419.13:
                    mov rdi, 1
                    lea rsi, [rbp + 59]
                    mov rdx, 1
                    mov rax, 1
                    syscall
                func.print..102.5.419.13.end:
                func.print..103.5.419.13:
                    mov rdi, 1
                    lea rsi, [rbp + 60]
                    mov rdx, 1
                    mov rax, 1
                    syscall
                func.print..103.5.419.13.end:
                inc qword [rbp + 368]
                jo baz_overflow_line_104
            func.greet..419.13.end:
        if.413.9.end:
    jmp loop.406.5
    loop.406.5.end:
    func.print..423.5:
        mov rdi, 1
        lea rsi, [rbp + 352]
        mov rdx, 15
        mov rax, 1
        syscall
    func.print..423.5.end:
    mov r15, strict qword vars.end - size.func.print_num - 1416
    cmp r15, rbp
    jl baz_frame_overflow
    lea r15, [rbp + 368]
    mov qword [rbp + 1416], r15
    lea rbx, [rbp + 1416]
    call func.print_num
    func.print..425.5:
        mov rdi, 1
        lea rsi, [rbp + 60]
        mov rdx, 1
        mov rax, 1
        syscall
    func.print..425.5.end:
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
    mov rdi, 1
    lea rsi, [rbp + 1416]
    mov rdx, 3
    mov rax, 1
    syscall
    mov rdi, 1
    lea rsi, [rbp + 1416]
    mov rdx, 1
    add rsi, 12
    mov rax, 1
    syscall
    mov rdi, 0
    jmp baz_exit
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.178.5:
    cmp.178.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.178.5.end
    if.178.5.code:
        ret
    if.178.5.end:
    mov r15, qword [rbx + 8]
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
    dec qword [rbx + 16]
    jo baz_overflow_line_180
    mov r15, strict qword vars.end - size.func.factorial - 32
    cmp r15, rbx
    jl baz_frame_overflow
    lea r15, [rbx + 24]
    mov qword [rbx + 32], r15
    lea r15, [rbx + 16]
    mov qword [rbx + 40], r15
    push rbx
    lea rbx, [rbx + 32]
    call func.factorial
    pop rbx
    mov r15, qword [rbx]
    mov r13, qword [rbx + 8]
    mov r14, qword [r13]
    imul r14, qword [rbx + 24]
    jo baz_overflow_line_182
    mov qword [r15], r14
    ret
size.func.factorial equ 32
func.print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
    mov byte [rbx + 40], 0
    if.145.5:
    cmp.145.8:
    cmp qword [rbx + 32], 0
    jge if.145.5.end
    if.145.5.code:
        mov byte [rbx + 40], 1
    if.145.5.end:
    if.148.5:
    cmp.148.8:
    cmp qword [rbx + 32], 0
    jle if.148.5.end
    if.148.5.code:
        neg qword [rbx + 32]
        jo baz_overflow_line_149
    if.148.5.end:
    mov qword [rbx + 48], 20
    loop.153.5:
        dec qword [rbx + 48]
        jo baz_overflow_line_154
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_155
        mov r14, 48
        mov r13, qword [rbx + 32]
        mov rax, r13
        mov r12, 10
        cqo
        idiv r12
        mov r13, rdx
        sub r14, r13
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 32]
        mov r15, 10
        cqo
        idiv r15
        mov qword [rbx + 32], rax
        if.157.9:
        cmp.157.12:
        cmp qword [rbx + 32], 0
        jne loop.153.5
        if.157.9.code:
        if.157.9.end:
    loop.153.5.end:
    if.160.5:
    cmp.160.8:
    cmp byte [rbx + 40], 0
    je if.160.5.end
    if.160.5.code:
        dec qword [rbx + 48]
        jo baz_overflow_line_161
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_162
        mov byte [rbx + r15 + 8], 45
    if.160.5.end:
    mov qword [rbx + 56], 0
    loop.166.5:
        mov r15, qword [rbx + 56]
        cmp r15, 20
        jae baz_bounds_line_167
        mov r14, qword [rbx + 48]
        cmp r14, 20
        jae baz_bounds_line_167
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        inc qword [rbx + 56]
        jo baz_overflow_line_168
        inc qword [rbx + 48]
        jo baz_overflow_line_169
        if.170.9:
        cmp.170.12:
        cmp qword [rbx + 48], 20
        jne loop.166.5
        if.170.9.code:
        if.170.9.end:
    loop.166.5.end:
    mov rdi, 1
    lea rsi, [rbx + 8]
    mov rdx, qword [rbx + 56]
    cmp rdx, 20
    ja baz_bounds_line_173
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
baz_frame_overflow:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_frame_overflow]
    mov rdx, 23
    syscall
    mov rdi, 254
    jmp baz_exit
section .rodata
msg_frame_overflow:
db `\npanic: frame overflow\n`
section .text
baz_overlap_line_250:
    mov rbp, 250
    jmp baz_overlap_panic_file_0
baz_overlap_line_384:
    mov rbp, 384
baz_overlap_panic_file_0:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overlap_file_0]
    mov rdx, 28
    syscall
    mov rbx, 251
    jmp baz_report_line
section .rodata
msg_overlap_file_0:
db `\npanic: overlap at prog.baz:`
section .text
baz_overflow_line_55:
    mov rbp, 55
    jmp baz_overflow_panic_file_0
baz_overflow_line_67:
    mov rbp, 67
    jmp baz_overflow_panic_file_0
baz_overflow_line_104:
    mov rbp, 104
    jmp baz_overflow_panic_file_0
baz_overflow_line_149:
    mov rbp, 149
    jmp baz_overflow_panic_file_0
baz_overflow_line_154:
    mov rbp, 154
    jmp baz_overflow_panic_file_0
baz_overflow_line_161:
    mov rbp, 161
    jmp baz_overflow_panic_file_0
baz_overflow_line_168:
    mov rbp, 168
    jmp baz_overflow_panic_file_0
baz_overflow_line_169:
    mov rbp, 169
    jmp baz_overflow_panic_file_0
baz_overflow_line_180:
    mov rbp, 180
    jmp baz_overflow_panic_file_0
baz_overflow_line_182:
    mov rbp, 182
    jmp baz_overflow_panic_file_0
baz_overflow_line_212:
    mov rbp, 212
    jmp baz_overflow_panic_file_0
baz_overflow_line_215:
    mov rbp, 215
    jmp baz_overflow_panic_file_0
baz_overflow_line_216:
    mov rbp, 216
    jmp baz_overflow_panic_file_0
baz_overflow_line_246:
    mov rbp, 246
    jmp baz_overflow_panic_file_0
baz_overflow_line_272:
    mov rbp, 272
    jmp baz_overflow_panic_file_0
baz_overflow_line_281:
    mov rbp, 281
    jmp baz_overflow_panic_file_0
baz_overflow_line_344:
    mov rbp, 344
    jmp baz_overflow_panic_file_0
baz_overflow_line_349:
    mov rbp, 349
    jmp baz_overflow_panic_file_0
baz_overflow_line_407:
    mov rbp, 407
baz_overflow_panic_file_0:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overflow_file_0]
    mov rdx, 29
    syscall
    mov rbx, 249
    jmp baz_report_line
section .rodata
msg_overflow_file_0:
db `\npanic: overflow at prog.baz:`
section .text
baz_bounds_line_96:
    mov rbp, 96
    jmp baz_bounds_panic_file_0
baz_bounds_line_155:
    mov rbp, 155
    jmp baz_bounds_panic_file_0
baz_bounds_line_162:
    mov rbp, 162
    jmp baz_bounds_panic_file_0
baz_bounds_line_167:
    mov rbp, 167
    jmp baz_bounds_panic_file_0
baz_bounds_line_173:
    mov rbp, 173
    jmp baz_bounds_panic_file_0
baz_bounds_line_245:
    mov rbp, 245
    jmp baz_bounds_panic_file_0
baz_bounds_line_246:
    mov rbp, 246
    jmp baz_bounds_panic_file_0
baz_bounds_line_272:
    mov rbp, 272
    jmp baz_bounds_panic_file_0
baz_bounds_line_273:
    mov rbp, 273
    jmp baz_bounds_panic_file_0
baz_bounds_line_274:
    mov rbp, 274
baz_bounds_panic_file_0:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_bounds_file_0]
    mov rdx, 27
    syscall
    mov rbx, 255
section .rodata
msg_bounds_file_0:
db `\npanic: bounds at prog.baz:`
section .text
baz_report_line:
    mov rax, rbp
    mov rdi, strict qword num_buffer + 20
    mov byte [rdi], 10
    dec rdi
    mov rcx, 10
.convert_loop:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jne .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdx, strict qword num_buffer + 21
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rdi, rbx
section .bss
num_buffer:
resb 21
section .text
baz_exit:
    mov rax, 60
    syscall
section .data
align 16
dat:
db `hello world from baz\n`
db `enter name:\n`
db `that is not a name.\n`
db `hello `
db `.`
db `\n`
db `: `
times 1 db 0
dq 1
times 24 db 0
db 3
times 127 db 0
db 3
db `baz`
times 124 db 0
db `names greeted: `
times 1 db 0
dq 0
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 65536
vars.end:
```

## With comments

```nasm

;
; generated by baz
;

default rel

section .text
bits 64
global _start
_start:

; allocate named register rbp
lea rbp, [dat]

;[prog.baz:7:1] point : 16 B    fields:
;[prog.baz:7:1]       name :  offset :    size :  array? : array size
;[prog.baz:7:1]          x :       0 :       8 :      no :           
;[prog.baz:7:1]          y :       8 :       8 :      no :           
;
;[prog.baz:9:1] object : 24 B    fields:
;[prog.baz:9:1]       name :  offset :    size :  array? : array size
;[prog.baz:9:1]        pos :       0 :      16 :      no :           
;[prog.baz:9:1]      color :      16 :       4 :      no :           
;
;[prog.baz:11:1] world : 64 B    fields:
;[prog.baz:11:1]       name :  offset :    size :  array? : array size
;[prog.baz:11:1]  locations :       0 :      64 :     yes :          8
;
;[prog.baz:14:1] str : 128 B    fields:
;[prog.baz:14:1]       name :  offset :    size :  array? : array size
;[prog.baz:14:1]        len :       0 :       1 :      no :           
;[prog.baz:14:1]       data :       1 :     127 :     yes :        127
;
;[prog.baz:22:1] dat hello = "hello world from baz\n"
;[prog.baz:22:7] hello: i8[21] (21 B @ [rbp])
;[prog.baz:23:1] dat prompt1 = "enter name:\n"
;[prog.baz:23:5] prompt1: i8[12] (12 B @ [rbp + 21])
;[prog.baz:24:1] dat prompt2 = "that is not a name.\n"
;[prog.baz:24:5] prompt2: i8[20] (20 B @ [rbp + 33])
;[prog.baz:25:1] dat prompt3 = "hello "
;[prog.baz:25:5] prompt3: i8[6] (6 B @ [rbp + 53])
;[prog.baz:26:1] dat dot = "."
;[prog.baz:26:9] dot: i8[1] (1 B @ [rbp + 59])
;[prog.baz:27:1] dat nl = "\n"
;[prog.baz:27:10] nl: i8[1] (1 B @ [rbp + 60])
;[prog.baz:28:1] dat colon = ": "
;[prog.baz:28:7] colon: i8[2] (2 B @ [rbp + 61])
;[prog.baz:29:1] dat nums = [4]{ 1 }
;[prog.baz:29:8] nums: i64[4] (32 B @ [rbp + 64])
;[prog.baz:30:1] dat str1 = str{ 3 }
;[prog.baz:30:8] str1: str (128 B @ [rbp + 96])
;[prog.baz:31:1] dat str2 = str{ 3, "baz" }
;[prog.baz:31:8] str2: str (128 B @ [rbp + 224])
;[prog.baz:33:1] dat greeted = "names greeted: "
;[prog.baz:33:5] greeted: i8[15] (15 B @ [rbp + 352])
;[prog.baz:34:1] dat names = 0
;[prog.baz:34:7] names: i64 (8 B @ [rbp + 368])
;[prog.baz:124:1] let yes = 1
;[prog.baz:124:5] const yes = 1
;[prog.baz:125:1] let no = 0
;[prog.baz:125:5] const no = 0
;[prog.baz:126:1] let maybe = -1
;[prog.baz:126:5] const maybe = -1
;
main:
;   [prog.baz:188:5] var answer = 0
;   [prog.baz:188:9] answer: i64 (8 B @ [rbp + 384])
;   [prog.baz:188:9] answer = 0
;   [prog.baz:188:18] answer = 0
;   [prog.baz:188:18] src: folded constant '0'
    mov qword [rbp + 384], 0
;   [prog.baz:190:5] assert(answer == 0)
;   [prog.baz:190:12] allocate scratch register -> r15
;   [prog.baz:190:12] ? answer == 0
    cmp.190.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.190.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..190.5:
;       [prog.baz:190:5] bind ok -> r15b
        if.38.24.190.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.190.5:
        test r15b, r15b
        jne if.38.24.190.5.end
        if.38.24.190.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.190.5.end:
    func.assert..190.5.end:
;       [prog.baz:190:5] free scratch register r15
;   [prog.baz:192:5] answer = maybe
;   [prog.baz:192:14] answer = -1
;   [prog.baz:192:14] src: folded constant 'maybe'
    mov qword [rbp + 384], -1
;   [prog.baz:193:5] assert(answer == -1)
;   [prog.baz:193:12] allocate scratch register -> r15
;   [prog.baz:193:12] ? answer == -1
    cmp.193.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.193.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..193.5:
;       [prog.baz:193:5] bind ok -> r15b
        if.38.24.193.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.193.5:
        test r15b, r15b
        jne if.38.24.193.5.end
        if.38.24.193.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.193.5.end:
    func.assert..193.5.end:
;       [prog.baz:193:5] free scratch register r15
;       [prog.baz:198:9] let maybe = 33
;       [prog.baz:198:13] const maybe = 33
;       [prog.baz:199:9] assert(maybe == 33)
;       [prog.baz:38:6] assert(ok bool)
        func.assert..199.9:
;           [prog.baz:199:9] bind ok -> 1
            if.38.24.199.9:
;           [prog.baz:38:27] ? shorthand: not ok
            cmp.38.27.199.9:
;           [prog.baz:38:31] const eval to false
            if.38.24.199.9.end:
        func.assert..199.9.end:
;   [prog.baz:202:5] assert(maybe == -1)
;   [prog.baz:38:6] assert(ok bool)
    func.assert..202.5:
;       [prog.baz:202:5] bind ok -> 1
        if.38.24.202.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.202.5:
;       [prog.baz:38:31] const eval to false
        if.38.24.202.5.end:
    func.assert..202.5.end:
;   [prog.baz:204:5] assert(nums[0] == 1 and nums[3] == 0)
;   [prog.baz:204:12] allocate scratch register -> r15
;   [prog.baz:204:12] ? nums[0] == 1 and nums[3] == 0
;   [prog.baz:204:12] ? nums[0] == 1
    cmp.204.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.204.12.end
;   [prog.baz:204:29] ? nums[3] == 0
    cmp.204.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.204.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..204.5:
;       [prog.baz:204:5] bind ok -> r15b
        if.38.24.204.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.204.5:
        test r15b, r15b
        jne if.38.24.204.5.end
        if.38.24.204.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.204.5.end:
    func.assert..204.5.end:
;       [prog.baz:204:5] free scratch register r15
;   [prog.baz:205:5] assert(str2.len == 3 and str2.data[2] == 'z')
;   [prog.baz:205:12] allocate scratch register -> r15
;   [prog.baz:205:12] ? str2.len == 3 and str2.data[2] == 'z'
;   [prog.baz:205:12] ? str2.len == 3
    cmp.205.12:
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.205.12.end
;   [prog.baz:205:30] ? str2.data[2] == 'z'
    cmp.205.30:
    cmp byte [rbp + 227], 122
    sete r15b
    bool.205.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..205.5:
;       [prog.baz:205:5] bind ok -> r15b
        if.38.24.205.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.205.5:
        test r15b, r15b
        jne if.38.24.205.5.end
        if.38.24.205.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.205.5.end:
    func.assert..205.5.end:
;       [prog.baz:205:5] free scratch register r15
;   [prog.baz:207:5] var a = 7
;   [prog.baz:207:9] a: i64 (8 B @ [rbp + 392])
;   [prog.baz:207:9] a = 7
;   [prog.baz:207:13] a = 7
;   [prog.baz:207:13] src: folded constant '7'
    mov qword [rbp + 392], 7
;   [prog.baz:208:5] assert(a & 3 == 3)
;   [prog.baz:208:12] allocate scratch register -> r15
;   [prog.baz:208:12] ? a & 3 == 3
    cmp.208.12:
;   [prog.baz:208:12] allocate scratch register -> r14
;       [prog.baz:208:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:208:12] r14 & 3
;       [prog.baz:208:12] src: folded constant '& 3'
        and r14, 3
    cmp r14, 3
    sete r15b
;   [prog.baz:208:12] free scratch register r14
    bool.208.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..208.5:
;       [prog.baz:208:5] bind ok -> r15b
        if.38.24.208.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.208.5:
        test r15b, r15b
        jne if.38.24.208.5.end
        if.38.24.208.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.208.5.end:
    func.assert..208.5.end:
;       [prog.baz:208:5] free scratch register r15
;   [prog.baz:209:5] assert(a | 8 == 15)
;   [prog.baz:209:12] allocate scratch register -> r15
;   [prog.baz:209:12] ? a | 8 == 15
    cmp.209.12:
;   [prog.baz:209:12] allocate scratch register -> r14
;       [prog.baz:209:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:209:12] r14 | 8
;       [prog.baz:209:12] src: folded constant '| 8'
        or r14, 8
    cmp r14, 15
    sete r15b
;   [prog.baz:209:12] free scratch register r14
    bool.209.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..209.5:
;       [prog.baz:209:5] bind ok -> r15b
        if.38.24.209.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.209.5:
        test r15b, r15b
        jne if.38.24.209.5.end
        if.38.24.209.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.209.5.end:
    func.assert..209.5.end:
;       [prog.baz:209:5] free scratch register r15
;   [prog.baz:210:5] assert(a ^ 1 == 6)
;   [prog.baz:210:12] allocate scratch register -> r15
;   [prog.baz:210:12] ? a ^ 1 == 6
    cmp.210.12:
;   [prog.baz:210:12] allocate scratch register -> r14
;       [prog.baz:210:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:210:12] r14 ^ 1
;       [prog.baz:210:12] src: folded constant '^ 1'
        xor r14, 1
    cmp r14, 6
    sete r15b
;   [prog.baz:210:12] free scratch register r14
    bool.210.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..210.5:
;       [prog.baz:210:5] bind ok -> r15b
        if.38.24.210.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.210.5:
        test r15b, r15b
        jne if.38.24.210.5.end
        if.38.24.210.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.210.5.end:
    func.assert..210.5.end:
;       [prog.baz:210:5] free scratch register r15
;   [prog.baz:211:5] assert(a << 2 == 28)
;   [prog.baz:211:12] allocate scratch register -> r15
;   [prog.baz:211:12] ? a << 2 == 28
    cmp.211.12:
;   [prog.baz:211:12] allocate scratch register -> r14
;       [prog.baz:211:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:211:17] r14 << 2
;       [prog.baz:211:17] src: constant
        sal r14, 2
    cmp r14, 28
    sete r15b
;   [prog.baz:211:12] free scratch register r14
    bool.211.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..211.5:
;       [prog.baz:211:5] bind ok -> r15b
        if.38.24.211.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.211.5:
        test r15b, r15b
        jne if.38.24.211.5.end
        if.38.24.211.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.211.5.end:
    func.assert..211.5.end:
;       [prog.baz:211:5] free scratch register r15
;   [prog.baz:212:5] assert(-a >> 1 == -4)
;   [prog.baz:212:12] allocate scratch register -> r15
;   [prog.baz:212:12] ? -a >> 1 == -4
    cmp.212.12:
;   [prog.baz:212:12] allocate scratch register -> r14
;       [prog.baz:212:13] -a
        mov r14, qword [rbp + 392]
        neg r14
        jo baz_overflow_line_212
;       [prog.baz:212:18] r14 >> 1
;       [prog.baz:212:18] src: constant
        sar r14, 1
    cmp r14, -4
    sete r15b
;   [prog.baz:212:12] free scratch register r14
    bool.212.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..212.5:
;       [prog.baz:212:5] bind ok -> r15b
        if.38.24.212.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.212.5:
        test r15b, r15b
        jne if.38.24.212.5.end
        if.38.24.212.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.212.5.end:
    func.assert..212.5.end:
;       [prog.baz:212:5] free scratch register r15
;   [prog.baz:215:5] assert(a + a << 1 == 21)
;   [prog.baz:215:12] allocate scratch register -> r15
;   [prog.baz:215:12] ? a + a << 1 == 21
    cmp.215.12:
;   [prog.baz:215:12] allocate scratch register -> r14
;       [prog.baz:215:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:215:16] r14 + a << 1
;       [prog.baz:215:16] src: expression
;       [prog.baz:215:16] allocate scratch register -> r13
;       [prog.baz:215:16] a
        mov r13, qword [rbp + 392]
;       [prog.baz:215:21] r13 << 1
;       [prog.baz:215:21] src: constant
        sal r13, 1
        add r14, r13
        jo baz_overflow_line_215
;       [prog.baz:215:16] free scratch register r13
    cmp r14, 21
    sete r15b
;   [prog.baz:215:12] free scratch register r14
    bool.215.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..215.5:
;       [prog.baz:215:5] bind ok -> r15b
        if.38.24.215.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.215.5:
        test r15b, r15b
        jne if.38.24.215.5.end
        if.38.24.215.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.215.5.end:
    func.assert..215.5.end:
;       [prog.baz:215:5] free scratch register r15
;   [prog.baz:216:5] assert((a + a) << 1 == 28)
;   [prog.baz:216:12] allocate scratch register -> r15
;   [prog.baz:216:12] ? (a + a) << 1 == 28
    cmp.216.12:
;   [prog.baz:216:12] allocate scratch register -> r14
;       [prog.baz:216:13] r14 = (a + a)
;       [prog.baz:216:13] = expression
;       [prog.baz:216:13] a
        mov r14, qword [rbp + 392]
;       [prog.baz:216:17] r14 + a
;       [prog.baz:216:17] src: operand
        add r14, qword [rbp + 392]
        jo baz_overflow_line_216
;       [prog.baz:216:23] r14 << 1
;       [prog.baz:216:23] src: constant
        sal r14, 1
    cmp r14, 28
    sete r15b
;   [prog.baz:216:12] free scratch register r14
    bool.216.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..216.5:
;       [prog.baz:216:5] bind ok -> r15b
        if.38.24.216.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.216.5:
        test r15b, r15b
        jne if.38.24.216.5.end
        if.38.24.216.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.216.5.end:
    func.assert..216.5.end:
;       [prog.baz:216:5] free scratch register r15
;   [prog.baz:221:5] assert(a != 0 and (a >= 7 or a < 0))
;   [prog.baz:221:12] allocate scratch register -> r15
;   [prog.baz:221:12] ? a != 0 and (a >= 7 or a < 0)
;   [prog.baz:221:12] ? a != 0
    cmp.221.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.221.12.end
    cmp.221.23:
;   [prog.baz:221:23] ? (a >= 7 or a < 0)
;   [prog.baz:221:24] ? a >= 7
    cmp.221.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.221.12.end
;   [prog.baz:221:34] ? a < 0
    cmp.221.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.221.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..221.5:
;       [prog.baz:221:5] bind ok -> r15b
        if.38.24.221.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.221.5:
        test r15b, r15b
        jne if.38.24.221.5.end
        if.38.24.221.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.221.5.end:
    func.assert..221.5.end:
;       [prog.baz:221:5] free scratch register r15
;   [prog.baz:224:5] var small = i8(100)
;   [prog.baz:224:9] small: i8 (1 B @ [rbp + 400])
;   [prog.baz:224:9] small = i8(100)
;   [prog.baz:224:17] small = 100
;   [prog.baz:224:17] src: folded constant 'i8(100)'
    mov byte [rbp + 400], 100
;   [prog.baz:225:5] small = i8(small + small)
;   [prog.baz:225:13] small = i8(small + small)
;   [prog.baz:225:13] = expression
;   [prog.baz:225:16] allocate scratch register -> r15
;   [prog.baz:225:16] small
    mov r15b, byte [rbp + 400]
;   [prog.baz:225:24] r15b + small
;   [prog.baz:225:24] src: operand
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
;   [prog.baz:225:16] free scratch register r15
;   [prog.baz:226:5] assert(small == -56)
;   [prog.baz:226:12] allocate scratch register -> r15
;   [prog.baz:226:12] ? small == -56
    cmp.226.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.226.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..226.5:
;       [prog.baz:226:5] bind ok -> r15b
        if.38.24.226.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.226.5:
        test r15b, r15b
        jne if.38.24.226.5.end
        if.38.24.226.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.226.5.end:
    func.assert..226.5.end:
;       [prog.baz:226:5] free scratch register r15
;   [prog.baz:230:5] var wide = int(i16(small))
;   [prog.baz:230:9] wide: i64 (8 B @ [rbp + 408])
;   [prog.baz:230:9] wide = int(i16(small))
;   [prog.baz:230:16] wide = int(i16(small))
;   [prog.baz:230:16] = expression
;   [prog.baz:230:20] wide = i16(small)
;   [prog.baz:230:20] = expression
;   [prog.baz:230:20] allocate scratch register -> r15
;   [prog.baz:230:24] small
    movsx r15w, byte [rbp + 400]
    movsx r15, r15w
    mov qword [rbp + 408], r15
;   [prog.baz:230:20] free scratch register r15
;   [prog.baz:231:5] assert(wide == -56)
;   [prog.baz:231:12] allocate scratch register -> r15
;   [prog.baz:231:12] ? wide == -56
    cmp.231.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.231.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..231.5:
;       [prog.baz:231:5] bind ok -> r15b
        if.38.24.231.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.231.5:
        test r15b, r15b
        jne if.38.24.231.5.end
        if.38.24.231.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.231.5.end:
    func.assert..231.5.end:
;       [prog.baz:231:5] free scratch register r15
;   [prog.baz:238:5] var letter = '\x41'
;   [prog.baz:238:9] letter: i64 (8 B @ [rbp + 416])
;   [prog.baz:238:9] letter = '\x41'
;   [prog.baz:238:18] letter = 65
;   [prog.baz:238:18] src: folded constant '\x41'
    mov qword [rbp + 416], 65
;   [prog.baz:239:5] assert(letter == 'A')
;   [prog.baz:239:12] allocate scratch register -> r15
;   [prog.baz:239:12] ? letter == 'A'
    cmp.239.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.239.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..239.5:
;       [prog.baz:239:5] bind ok -> r15b
        if.38.24.239.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.239.5:
        test r15b, r15b
        jne if.38.24.239.5.end
        if.38.24.239.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.239.5.end:
    func.assert..239.5.end:
;       [prog.baz:239:5] free scratch register r15
;   [prog.baz:242:5] var arr = i32[4]
;   [prog.baz:242:9] arr: i32[4] (16 B @ [rbp + 424])
;   [prog.baz:242:9] arr = i32[4]
;   [prog.baz:242:15] zero remaining elements: 4 * 4 B = 16 B
;   [prog.baz:242:15] size <= 32 B, use mov
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
;   [prog.baz:244:5] var ix = 1
;   [prog.baz:244:9] ix: i64 (8 B @ [rbp + 440])
;   [prog.baz:244:9] ix = 1
;   [prog.baz:244:14] ix = 1
;   [prog.baz:244:14] src: folded constant '1'
    mov qword [rbp + 440], 1
;   [prog.baz:245:5] arr[ix] = 2
;   [prog.baz:245:9] allocate scratch register -> r15
;   [prog.baz:245:9] set array index
;   [prog.baz:245:9] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:245:9] bounds check begin
;   [prog.baz:245:9] lower bound
;   [prog.baz:245:9] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:245:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_245
;   [prog.baz:245:9] bounds check end
;   [prog.baz:245:15] arr[ix] = 2
;   [prog.baz:245:15] src: folded constant '2'
    mov dword [rbp + r15 * 4 + 424], 2
;   [prog.baz:245:5] free scratch register r15
;   [prog.baz:246:5] arr[ix + 1] = arr[ix]
;   [prog.baz:246:9] allocate scratch register -> r15
;   [prog.baz:246:9] set array index
;   [prog.baz:246:9] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:246:9] r15 + 1
;   [prog.baz:246:9] src: folded constant '+ 1'
    inc r15
    jo baz_overflow_line_246
;   [prog.baz:246:9] bounds check begin
;   [prog.baz:246:9] lower bound
;   [prog.baz:246:9] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:246:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_246
;   [prog.baz:246:9] bounds check end
;   [prog.baz:246:19] arr[ix]
;   [prog.baz:246:23] allocate scratch register -> r14
;   [prog.baz:246:23] set array index
;   [prog.baz:246:23] ix
    mov r14, qword [rbp + 440]
;   [prog.baz:246:23] bounds check begin
;   [prog.baz:246:23] lower bound
;   [prog.baz:246:23] r14 lower bound covered by the unsigned upper bound
;   [prog.baz:246:23] upper bound
    cmp r14, 4
    jae baz_bounds_line_246
;   [prog.baz:246:23] bounds check end
;   [prog.baz:246:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
;   [prog.baz:246:19] free scratch register r13
;   [prog.baz:246:19] free scratch register r14
;   [prog.baz:246:5] free scratch register r15
;   [prog.baz:247:5] assert(arr[1] == 2)
;   [prog.baz:247:12] allocate scratch register -> r15
;   [prog.baz:247:12] ? arr[1] == 2
    cmp.247.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.247.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..247.5:
;       [prog.baz:247:5] bind ok -> r15b
        if.38.24.247.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.247.5:
        test r15b, r15b
        jne if.38.24.247.5.end
        if.38.24.247.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.247.5.end:
    func.assert..247.5.end:
;       [prog.baz:247:5] free scratch register r15
;   [prog.baz:248:5] assert(arr[2] == 2)
;   [prog.baz:248:12] allocate scratch register -> r15
;   [prog.baz:248:12] ? arr[2] == 2
    cmp.248.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.248.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..248.5:
;       [prog.baz:248:5] bind ok -> r15b
        if.38.24.248.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.248.5:
        test r15b, r15b
        jne if.38.24.248.5.end
        if.38.24.248.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.248.5.end:
    func.assert..248.5.end:
;       [prog.baz:248:5] free scratch register r15
;   [prog.baz:250:5] array_copy(arr[2], arr, 2)
;   [prog.baz:250:5] allocate named register rsi
;   [prog.baz:250:5] allocate named register rdi
;   [prog.baz:250:5] allocate named register rcx
;   [prog.baz:250:16] arr[2]
    lea rsi, [rbp + 432]
;   [prog.baz:250:24] arr
    lea rdi, [rbp + 424]
;   [prog.baz:250:29] rcx = 2
;   [prog.baz:250:29] src: folded constant '2'
    mov rcx, 2
    sal rcx, 2
;   [prog.baz:250:5] overlap check begin
;   [prog.baz:250:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [prog.baz:250:5] the same range is not an overlap
    je .Lbaz_overlap.0
;   [prog.baz:250:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_250
    .Lbaz_overlap.0:
;   [prog.baz:250:5] free scratch register r15
;   [prog.baz:250:5] overlap check end
    rep movsb
;   [prog.baz:250:5] free named register rcx
;   [prog.baz:250:5] free named register rdi
;   [prog.baz:250:5] free named register rsi
;   [prog.baz:251:5] assert(arr[0] == 2)
;   [prog.baz:251:12] allocate scratch register -> r15
;   [prog.baz:251:12] ? arr[0] == 2
    cmp.251.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.251.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..251.5:
;       [prog.baz:251:5] bind ok -> r15b
        if.38.24.251.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.251.5:
        test r15b, r15b
        jne if.38.24.251.5.end
        if.38.24.251.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.251.5.end:
    func.assert..251.5.end:
;       [prog.baz:251:5] free scratch register r15
;   [prog.baz:254:5] var arr1 = i32[8]
;   [prog.baz:254:9] arr1: i32[8] (32 B @ [rbp + 448])
;   [prog.baz:254:9] arr1 = i32[8]
;   [prog.baz:254:16] zero remaining elements: 8 * 4 B = 32 B
;   [prog.baz:254:16] size <= 32 B, use mov
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
;   [prog.baz:255:5] array_copy(arr, arr1, 4)
;   [prog.baz:255:16] arr
;   [prog.baz:255:21] arr1
;   [prog.baz:255:5] size <= 16 B, use mov
;   [prog.baz:255:5] allocate scratch register -> r15
    mov r15, qword [rbp + 424]
    mov qword [rbp + 448], r15
    mov r15, qword [rbp + 432]
    mov qword [rbp + 456], r15
;   [prog.baz:255:5] free scratch register r15
;   [prog.baz:256:5] var eq = arrays_equal(arr[1], arr1[1], 3)
;   [prog.baz:256:9] eq: bool (1 B @ [rbp + 480])
;   [prog.baz:256:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [prog.baz:256:14] eq = arrays_equal(arr[1], arr1[1], 3)
;   [prog.baz:256:14] = expression
;   [prog.baz:256:14] arrays_equal(arr[1], arr1[1], 3)
;   [prog.baz:256:14] allocate named register rsi
;   [prog.baz:256:14] allocate named register rdi
;   [prog.baz:256:27] arr[1]
    lea rsi, [rbp + 428]
;   [prog.baz:256:35] arr1[1]
    lea rdi, [rbp + 452]
    cmpsq
    jne .Lbaz_equal.1
    cmpsd
    .Lbaz_equal.1:
;   [prog.baz:256:14] free named register rdi
;   [prog.baz:256:14] free named register rsi
    sete byte [rbp + 480]
;   [prog.baz:259:5] assert(eq)
;   [prog.baz:38:6] assert(ok bool)
    func.assert..259.5:
;       [prog.baz:259:5] bind ok -> eq
        if.38.24.259.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.259.5:
        cmp byte [rbp + 480], 0
        jne if.38.24.259.5.end
        if.38.24.259.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.259.5.end:
    func.assert..259.5.end:
;   [prog.baz:261:5] arr1[2] = -1
;   [prog.baz:261:15] arr1[2] = -1
;   [prog.baz:261:15] src: folded constant '-1'
    mov dword [rbp + 456], -1
;   [prog.baz:262:5] assert(not arrays_equal(arr, arr1, 4))
;   [prog.baz:262:12] allocate scratch register -> r15
;   [prog.baz:262:12] ? shorthand: not arrays_equal(arr, arr1, 4)
    cmp.262.12:
;       [prog.baz:262:16] r15b = arrays_equal(arr, arr1, 4)
;       [prog.baz:262:16] = expression
;       [prog.baz:262:16] arrays_equal(arr, arr1, 4)
;       [prog.baz:262:16] allocate named register rsi
;       [prog.baz:262:16] allocate named register rdi
;       [prog.baz:262:29] arr
        lea rsi, [rbp + 424]
;       [prog.baz:262:34] arr1
        lea rdi, [rbp + 448]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
;       [prog.baz:262:16] free named register rdi
;       [prog.baz:262:16] free named register rsi
        sete r15b
    test r15b, r15b
    sete r15b
    bool.262.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..262.5:
;       [prog.baz:262:5] bind ok -> r15b
        if.38.24.262.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.262.5:
        test r15b, r15b
        jne if.38.24.262.5.end
        if.38.24.262.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.262.5.end:
    func.assert..262.5.end:
;       [prog.baz:262:5] free scratch register r15
;   [prog.baz:264:5] var arr4 = arr
;   [prog.baz:264:9] arr4: i32[4] (16 B @ [rbp + 484])
;   [prog.baz:264:9] arr4 = arr
;   [prog.baz:264:16] size <= 16 B, use mov
;   [prog.baz:264:16] allocate scratch register -> r15
    mov r15, qword [rbp + 424]
    mov qword [rbp + 484], r15
    mov r15, qword [rbp + 432]
    mov qword [rbp + 492], r15
;   [prog.baz:264:16] free scratch register r15
;   [prog.baz:265:5] assert(arr == arr4)
;   [prog.baz:265:12] allocate scratch register -> r15
;   [prog.baz:265:12] ? arr == arr4
    cmp.265.12:
;       [prog.baz:265:12] allocate named register rsi
;       [prog.baz:265:12] allocate named register rdi
;       [prog.baz:265:12] arr
        lea rsi, [rbp + 424]
;       [prog.baz:265:19] arr4
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.3
        cmpsq
        .Lbaz_equal.3:
;       [prog.baz:265:12] free named register rdi
;       [prog.baz:265:12] free named register rsi
        sete r15b
    bool.265.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..265.5:
;       [prog.baz:265:5] bind ok -> r15b
        if.38.24.265.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.265.5:
        test r15b, r15b
        jne if.38.24.265.5.end
        if.38.24.265.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.265.5.end:
    func.assert..265.5.end:
;       [prog.baz:265:5] free scratch register r15
;   [prog.baz:271:5] ix = 3
;   [prog.baz:271:10] ix = 3
;   [prog.baz:271:10] src: folded constant '3'
    mov qword [rbp + 440], 3
;   [prog.baz:272:5] var tmp = ~inv(arr[ix - 1])
;   [prog.baz:272:9] tmp: i32 (4 B @ [rbp + 500])
;   [prog.baz:272:9] tmp = ~inv(arr[ix - 1])
;   [prog.baz:272:16] tmp = ~inv(arr[ix - 1])
;   [prog.baz:272:16] = expression
;   [prog.baz:272:16] ~inv(arr[ix - 1])
;   [prog.baz:272:24] allocate scratch register -> r15
;   [prog.baz:272:24] set array index
;   [prog.baz:272:24] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:272:24] r15 - 1
;   [prog.baz:272:24] src: folded constant '- 1'
    dec r15
    jo baz_overflow_line_272
;   [prog.baz:272:24] bounds check begin
;   [prog.baz:272:24] lower bound
;   [prog.baz:272:24] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:272:24] upper bound
    cmp r15, 4
    jae baz_bounds_line_272
;   [prog.baz:272:24] bounds check end
;   [prog.baz:72:6] inv(i i32) res i32
    func.inv..272.16:
;       [prog.baz:272:16] bind res -> tmp
;       [prog.baz:272:16] bind i -> arr[ix - 1] (address: rbp + r15 * 4 + 424)
;       [prog.baz:73:5] res = ~i
;       [prog.baz:73:11] kept the first of two versions of 3 and 3 instructions, the first of equal sizes
;       [prog.baz:73:12] ~i
;       [prog.baz:73:12] allocate scratch register -> r14
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
;       [prog.baz:73:12] free scratch register r14
        not dword [rbp + 500]
    func.inv..272.16.end:
;       [prog.baz:272:16] free scratch register r15
    not dword [rbp + 500]
;   [prog.baz:273:5] arr[ix] = tmp
;   [prog.baz:273:9] allocate scratch register -> r15
;   [prog.baz:273:9] set array index
;   [prog.baz:273:9] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:273:9] bounds check begin
;   [prog.baz:273:9] lower bound
;   [prog.baz:273:9] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:273:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_273
;   [prog.baz:273:9] bounds check end
;   [prog.baz:273:15] tmp
;   [prog.baz:273:15] allocate scratch register -> r14
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
;   [prog.baz:273:15] free scratch register r14
;   [prog.baz:273:5] free scratch register r15
;   [prog.baz:274:5] assert(arr[ix] == 2)
;   [prog.baz:274:12] allocate scratch register -> r15
;   [prog.baz:274:12] ? arr[ix] == 2
    cmp.274.12:
;   [prog.baz:274:16] allocate scratch register -> r14
;   [prog.baz:274:16] set array index
;   [prog.baz:274:16] ix
    mov r14, qword [rbp + 440]
;   [prog.baz:274:16] bounds check begin
;   [prog.baz:274:16] lower bound
;   [prog.baz:274:16] r14 lower bound covered by the unsigned upper bound
;   [prog.baz:274:16] upper bound
    cmp r14, 4
    jae baz_bounds_line_274
;   [prog.baz:274:16] bounds check end
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
;   [prog.baz:274:12] free scratch register r14
    bool.274.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..274.5:
;       [prog.baz:274:5] bind ok -> r15b
        if.38.24.274.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.274.5:
        test r15b, r15b
        jne if.38.24.274.5.end
        if.38.24.274.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.274.5.end:
    func.assert..274.5.end:
;       [prog.baz:274:5] free scratch register r15
;   [prog.baz:276:5] faz(arr)
;   [prog.baz:78:6] faz(arg mut i32[])
    func.faz..276.5:
;       [prog.baz:276:5] bind arg -> arr
;       [prog.baz:79:5] arg[1] = 0xfe
;       [prog.baz:79:14] arg[1] = 254
;       [prog.baz:79:14] src: folded constant '0xfe'
        mov dword [rbp + 428], 254
    func.faz..276.5.end:
;   [prog.baz:277:5] assert(arr[1] == 0xfe)
;   [prog.baz:277:12] allocate scratch register -> r15
;   [prog.baz:277:12] ? arr[1] == 0xfe
    cmp.277.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.277.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..277.5:
;       [prog.baz:277:5] bind ok -> r15b
        if.38.24.277.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.277.5:
        test r15b, r15b
        jne if.38.24.277.5.end
        if.38.24.277.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.277.5.end:
    func.assert..277.5.end:
;       [prog.baz:277:5] free scratch register r15
;   [prog.baz:279:5] var arr3 = []{ 3, 5 }
;   [prog.baz:279:9] arr3: i64[2] (16 B @ [rbp + 504])
;   [prog.baz:279:9] arr3 = []{ 3, 5 }
;   [prog.baz:279:18] size <= 16 B, use immediates
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
;   [prog.baz:280:5] foo arr3
;   [prog.baz:280:9] allocate scratch register -> r15
;   [prog.baz:280:9] initialize iterator e
    lea r15, [rbp + 504]
;   [prog.baz:280:5] allocate scratch register -> r14
;   [prog.baz:280:9] e: i64 (r15)
;   [prog.baz:280:9] i: i64 (r14)
;   [prog.baz:280:9] const n = 2
;   [prog.baz:280:5] initialize counter i
    mov r14, 0
    foo.280.5:
;       [prog.baz:281:9] e = e + i + n
;       [prog.baz:281:13] kept the first of two versions of 4 and 6 instructions, the smaller
;       [prog.baz:281:13] e
;       [prog.baz:281:17] e + i
;       [prog.baz:281:17] src: operand
        add qword [r15], r14
        jo baz_overflow_line_281
;       [prog.baz:281:13] e + 2
;       [prog.baz:281:13] src: folded constant '+ n'
        add qword [r15], 2
        jo baz_overflow_line_281
        foo.280.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.280.5
    foo.280.5.end:
;   [prog.baz:280:5] free scratch register r14
;   [prog.baz:280:5] free scratch register r15
;   [prog.baz:283:5] assert(arr3[0] == 3 + 0 + 2)
;   [prog.baz:283:12] allocate scratch register -> r15
;   [prog.baz:283:12] ? arr3[0] == 3 + 0 + 2
    cmp.283.12:
;   [prog.baz:283:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 504], 5
    sete r15b
    bool.283.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..283.5:
;       [prog.baz:283:5] bind ok -> r15b
        if.38.24.283.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.283.5:
        test r15b, r15b
        jne if.38.24.283.5.end
        if.38.24.283.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.283.5.end:
    func.assert..283.5.end:
;       [prog.baz:283:5] free scratch register r15
;   [prog.baz:284:5] assert(arr3[1] == 5 + 1 + 2)
;   [prog.baz:284:12] allocate scratch register -> r15
;   [prog.baz:284:12] ? arr3[1] == 5 + 1 + 2
    cmp.284.12:
;   [prog.baz:284:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 512], 8
    sete r15b
    bool.284.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..284.5:
;       [prog.baz:284:5] bind ok -> r15b
        if.38.24.284.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.284.5:
        test r15b, r15b
        jne if.38.24.284.5.end
        if.38.24.284.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.284.5.end:
    func.assert..284.5.end:
;       [prog.baz:284:5] free scratch register r15
;   [prog.baz:290:5] var p = point
;   [prog.baz:290:9] p: point (16 B @ [rbp + 520])
;   [prog.baz:290:9] p = point
;   [prog.baz:290:13] zero remaining fields: 16 B
;   [prog.baz:290:13] size <= 32 B, use mov
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
;   [prog.baz:292:7] p.fooz()
;   [prog.baz:47:10] mut point.fooz()
    func.point.fooz..292.7:
;       [prog.baz:292:7] bind self -> p
;       [prog.baz:48:5] self.x = 0b10
;       [prog.baz:48:14] self.x = 2
;       [prog.baz:48:14] src: folded constant '0b10'
        mov qword [rbp + 520], 2
;       [prog.baz:49:5] self.y = 0xb
;       [prog.baz:49:14] self.y = 11
;       [prog.baz:49:14] src: folded constant '0xb'
        mov qword [rbp + 528], 11
    func.point.fooz..292.7.end:
;   [prog.baz:295:5] assert(p.x == 2)
;   [prog.baz:295:12] allocate scratch register -> r15
;   [prog.baz:295:12] ? p.x == 2
    cmp.295.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.295.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..295.5:
;       [prog.baz:295:5] bind ok -> r15b
        if.38.24.295.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.295.5:
        test r15b, r15b
        jne if.38.24.295.5.end
        if.38.24.295.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.295.5.end:
    func.assert..295.5.end:
;       [prog.baz:295:5] free scratch register r15
;   [prog.baz:296:5] assert(p.y == 0xb)
;   [prog.baz:296:12] allocate scratch register -> r15
;   [prog.baz:296:12] ? p.y == 0xb
    cmp.296.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.296.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..296.5:
;       [prog.baz:296:5] bind ok -> r15b
        if.38.24.296.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.296.5:
        test r15b, r15b
        jne if.38.24.296.5.end
        if.38.24.296.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.296.5.end:
    func.assert..296.5.end:
;       [prog.baz:296:5] free scratch register r15
;   [prog.baz:298:5] var q = p
;   [prog.baz:298:9] q: point (16 B @ [rbp + 536])
;   [prog.baz:298:9] q = p
;   [prog.baz:298:13] size <= 16 B, use mov
;   [prog.baz:298:13] allocate scratch register -> r15
    mov r15, qword [rbp + 520]
    mov qword [rbp + 536], r15
    mov r15, qword [rbp + 528]
    mov qword [rbp + 544], r15
;   [prog.baz:298:13] free scratch register r15
;   [prog.baz:301:5] assert(p == q)
;   [prog.baz:301:12] allocate scratch register -> r15
;   [prog.baz:301:12] ? p == q
    cmp.301.12:
;       [prog.baz:301:12] allocate named register rsi
;       [prog.baz:301:12] allocate named register rdi
;       [prog.baz:301:12] p
        lea rsi, [rbp + 520]
;       [prog.baz:301:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.4
        cmpsq
        .Lbaz_equal.4:
;       [prog.baz:301:12] free named register rdi
;       [prog.baz:301:12] free named register rsi
        sete r15b
    bool.301.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..301.5:
;       [prog.baz:301:5] bind ok -> r15b
        if.38.24.301.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.301.5:
        test r15b, r15b
        jne if.38.24.301.5.end
        if.38.24.301.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.301.5.end:
    func.assert..301.5.end:
;       [prog.baz:301:5] free scratch register r15
;   [prog.baz:305:5] q.x = 3
;   [prog.baz:305:11] q.x = 3
;   [prog.baz:305:11] src: folded constant '3'
    mov qword [rbp + 536], 3
;   [prog.baz:306:5] assert(p != q)
;   [prog.baz:306:12] allocate scratch register -> r15
;   [prog.baz:306:12] ? p != q
    cmp.306.12:
;       [prog.baz:306:12] allocate named register rsi
;       [prog.baz:306:12] allocate named register rdi
;       [prog.baz:306:12] p
        lea rsi, [rbp + 520]
;       [prog.baz:306:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.5
        cmpsq
        .Lbaz_equal.5:
;       [prog.baz:306:12] free named register rdi
;       [prog.baz:306:12] free named register rsi
        setne r15b
    bool.306.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..306.5:
;       [prog.baz:306:5] bind ok -> r15b
        if.38.24.306.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.306.5:
        test r15b, r15b
        jne if.38.24.306.5.end
        if.38.24.306.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.306.5.end:
    func.assert..306.5.end:
;       [prog.baz:306:5] free scratch register r15
;   [prog.baz:308:5] var i = 0
;   [prog.baz:308:9] i: i64 (8 B @ [rbp + 552])
;   [prog.baz:308:9] i = 0
;   [prog.baz:308:13] i = 0
;   [prog.baz:308:13] src: folded constant '0'
    mov qword [rbp + 552], 0
;   [prog.baz:309:5] bar(i)
;   [prog.baz:59:6] bar(arg mut)
    func.bar..309.5:
;       [prog.baz:309:5] bind arg -> i
        if.60.5.309.5:
;       [prog.baz:60:8] ? arg == 0
        cmp.60.8.309.5:
        cmp qword [rbp + 552], 0
        je func.bar..309.5.end
        if.60.5.309.5.code:
;           [prog.baz:60:17] return
        if.60.5.309.5.end:
;       [prog.baz:61:5] arg = 0xff
;       [prog.baz:61:11] arg = 255
;       [prog.baz:61:11] src: folded constant '0xff'
        mov qword [rbp + 552], 255
    func.bar..309.5.end:
;   [prog.baz:310:5] assert(i == 0)
;   [prog.baz:310:12] allocate scratch register -> r15
;   [prog.baz:310:12] ? i == 0
    cmp.310.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.310.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..310.5:
;       [prog.baz:310:5] bind ok -> r15b
        if.38.24.310.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.310.5:
        test r15b, r15b
        jne if.38.24.310.5.end
        if.38.24.310.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.310.5.end:
    func.assert..310.5.end:
;       [prog.baz:310:5] free scratch register r15
;   [prog.baz:312:5] i = 1
;   [prog.baz:312:9] i = 1
;   [prog.baz:312:9] src: folded constant '1'
    mov qword [rbp + 552], 1
;   [prog.baz:313:5] bar(i)
;   [prog.baz:59:6] bar(arg mut)
    func.bar..313.5:
;       [prog.baz:313:5] bind arg -> i
        if.60.5.313.5:
;       [prog.baz:60:8] ? arg == 0
        cmp.60.8.313.5:
        cmp qword [rbp + 552], 0
        je func.bar..313.5.end
        if.60.5.313.5.code:
;           [prog.baz:60:17] return
        if.60.5.313.5.end:
;       [prog.baz:61:5] arg = 0xff
;       [prog.baz:61:11] arg = 255
;       [prog.baz:61:11] src: folded constant '0xff'
        mov qword [rbp + 552], 255
    func.bar..313.5.end:
;   [prog.baz:314:5] assert(i == 0xff)
;   [prog.baz:314:12] allocate scratch register -> r15
;   [prog.baz:314:12] ? i == 0xff
    cmp.314.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.314.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..314.5:
;       [prog.baz:314:5] bind ok -> r15b
        if.38.24.314.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.314.5:
        test r15b, r15b
        jne if.38.24.314.5.end
        if.38.24.314.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.314.5.end:
    func.assert..314.5.end:
;       [prog.baz:314:5] free scratch register r15
;   [prog.baz:316:5] var j = 1
;   [prog.baz:316:9] j: i64 (8 B @ [rbp + 560])
;   [prog.baz:316:9] j = 1
;   [prog.baz:316:13] j = 1
;   [prog.baz:316:13] src: folded constant '1'
    mov qword [rbp + 560], 1
;   [prog.baz:317:5] var k = baz(j)
;   [prog.baz:317:9] k: i64 (8 B @ [rbp + 568])
;   [prog.baz:317:9] k = baz(j)
;   [prog.baz:317:13] k = baz(j)
;   [prog.baz:317:13] = expression
;   [prog.baz:317:13] baz(j)
;   [prog.baz:66:6] baz(arg) res
    func.baz..317.13:
;       [prog.baz:317:13] bind res -> k
;       [prog.baz:317:13] bind arg -> j
;       [prog.baz:67:5] res = arg * 2
;       [prog.baz:67:11] kept the second of two versions of 6 and 4 instructions, the smaller
;       [prog.baz:67:11] allocate scratch register -> r15
;       [prog.baz:67:11] arg
        mov r15, qword [rbp + 560]
;       [prog.baz:67:11] r15 * 2
;       [prog.baz:67:11] src: folded constant '* 2'
        imul r15, 2
        jo baz_overflow_line_67
        mov qword [rbp + 568], r15
;       [prog.baz:67:11] free scratch register r15
    func.baz..317.13.end:
;   [prog.baz:318:5] assert(k == 2)
;   [prog.baz:318:12] allocate scratch register -> r15
;   [prog.baz:318:12] ? k == 2
    cmp.318.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.318.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..318.5:
;       [prog.baz:318:5] bind ok -> r15b
        if.38.24.318.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.318.5:
        test r15b, r15b
        jne if.38.24.318.5.end
        if.38.24.318.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.318.5.end:
    func.assert..318.5.end:
;       [prog.baz:318:5] free scratch register r15
;   [prog.baz:320:5] k = baz(1)
;   [prog.baz:320:9] k = baz(1)
;   [prog.baz:320:9] = expression
;   [prog.baz:320:9] baz(1)
;   [prog.baz:66:6] baz(arg) res
    func.baz..320.9:
;       [prog.baz:320:9] bind res -> k
;       [prog.baz:320:9] bind arg -> 1
;       [prog.baz:67:5] res = arg * 2
;       [prog.baz:67:11] res = 2
;       [prog.baz:67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 568], 2
    func.baz..320.9.end:
;   [prog.baz:321:5] assert(k == 2)
;   [prog.baz:321:12] allocate scratch register -> r15
;   [prog.baz:321:12] ? k == 2
    cmp.321.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.321.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..321.5:
;       [prog.baz:321:5] bind ok -> r15b
        if.38.24.321.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.321.5:
        test r15b, r15b
        jne if.38.24.321.5.end
        if.38.24.321.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.321.5.end:
    func.assert..321.5.end:
;       [prog.baz:321:5] free scratch register r15
;   [prog.baz:323:5] var five = 5
;   [prog.baz:323:9] five: i64 (8 B @ [rbp + 576])
;   [prog.baz:323:9] five = 5
;   [prog.baz:323:16] five = 5
;   [prog.baz:323:16] src: folded constant '5'
    mov qword [rbp + 576], 5
;   [prog.baz:324:5] var f = factorial(five)
;   [prog.baz:324:9] f: i64 (8 B @ [rbp + 584])
;   [prog.baz:324:9] f = factorial(five)
;   [prog.baz:324:13] f = factorial(five)
;   [prog.baz:324:13] = expression
;   [prog.baz:324:13] factorial(five)
;   [prog.baz:324:13] frame capacity check begin
;   [prog.baz:324:13] callee storage starts after the caller's storage (rbp + 592)
;   [prog.baz:324:13] allocate scratch register -> r15
    mov r15, strict qword vars.end - size.func.factorial - 592
    cmp r15, rbp
    jl baz_frame_overflow
;   [prog.baz:324:13] free scratch register r15
;   [prog.baz:324:13] frame capacity check end
;   [prog.baz:324:13] result address in callee frame
;   [prog.baz:324:13] allocate scratch register -> r15
    lea r15, [rbp + 584]
    mov qword [rbp + 592], r15
;   [prog.baz:324:13] free scratch register r15
;   [prog.baz:324:13] address of argument 'five' to parameter 'n'
;   [prog.baz:324:13] allocate scratch register -> r15
    lea r15, [rbp + 576]
    mov qword [rbp + 600], r15
;   [prog.baz:324:13] free scratch register r15
;   [prog.baz:324:13] set function frame base
    lea rbx, [rbp + 592]
    call func.factorial
;   [prog.baz:325:5] assert(f == 120)
;   [prog.baz:325:12] allocate scratch register -> r15
;   [prog.baz:325:12] ? f == 120
    cmp.325.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.325.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..325.5:
;       [prog.baz:325:5] bind ok -> r15b
        if.38.24.325.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.325.5:
        test r15b, r15b
        jne if.38.24.325.5.end
        if.38.24.325.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.325.5.end:
    func.assert..325.5.end:
;       [prog.baz:325:5] free scratch register r15
;   [prog.baz:327:5] var p0 = point{baz(3), 0}
;   [prog.baz:327:9] p0: point (16 B @ [rbp + 592])
;   [prog.baz:327:9] p0 = point{baz(3), 0}
;   [prog.baz:327:20] copy field 'x'
;   [prog.baz:327:20] p0.x = baz(3)
;   [prog.baz:327:20] = expression
;   [prog.baz:327:20] baz(3)
;   [prog.baz:66:6] baz(arg) res
    func.baz..327.20:
;       [prog.baz:327:20] bind res -> p0.x (address: rbp + 592)
;       [prog.baz:327:20] bind arg -> 3
;       [prog.baz:67:5] res = arg * 2
;       [prog.baz:67:11] res = 6
;       [prog.baz:67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 592], 6
    func.baz..327.20.end:
;   [prog.baz:327:28] copy field 'y'
;   [prog.baz:327:28] p0.y = 0
;   [prog.baz:327:28] src: folded constant '0'
    mov qword [rbp + 600], 0
;   [prog.baz:328:5] assert(p0.x == 6)
;   [prog.baz:328:12] allocate scratch register -> r15
;   [prog.baz:328:12] ? p0.x == 6
    cmp.328.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.328.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..328.5:
;       [prog.baz:328:5] bind ok -> r15b
        if.38.24.328.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.328.5:
        test r15b, r15b
        jne if.38.24.328.5.end
        if.38.24.328.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.328.5.end:
    func.assert..328.5.end:
;       [prog.baz:328:5] free scratch register r15
;   [prog.baz:330:5] var pt = point.at(-1, -2)
;   [prog.baz:330:9] pt: point (16 B @ [rbp + 608])
;   [prog.baz:330:9] pt = point.at(-1, -2)
;   [prog.baz:330:14] point.at(-1, -2)
;   [prog.baz:108:6] point.at(x, y) self
    func.point.at..330.14:
;       [prog.baz:330:14] bind self -> pt
;       [prog.baz:330:14] bind x -> -1
;       [prog.baz:330:14] bind y -> -2
;       [prog.baz:109:5] self.x = x
;       [prog.baz:109:14] self.x = -1
;       [prog.baz:109:14] src: folded constant 'x'
        mov qword [rbp + 608], -1
;       [prog.baz:110:5] self.y = y
;       [prog.baz:110:14] self.y = -2
;       [prog.baz:110:14] src: folded constant 'y'
        mov qword [rbp + 616], -2
    func.point.at..330.14.end:
;   [prog.baz:334:5] assert(pt.x == -1)
;   [prog.baz:334:12] allocate scratch register -> r15
;   [prog.baz:334:12] ? pt.x == -1
    cmp.334.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.334.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..334.5:
;       [prog.baz:334:5] bind ok -> r15b
        if.38.24.334.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.334.5:
        test r15b, r15b
        jne if.38.24.334.5.end
        if.38.24.334.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.334.5.end:
    func.assert..334.5.end:
;       [prog.baz:334:5] free scratch register r15
;   [prog.baz:335:5] assert(pt.y == -2)
;   [prog.baz:335:12] allocate scratch register -> r15
;   [prog.baz:335:12] ? pt.y == -2
    cmp.335.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.335.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..335.5:
;       [prog.baz:335:5] bind ok -> r15b
        if.38.24.335.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.335.5:
        test r15b, r15b
        jne if.38.24.335.5.end
        if.38.24.335.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.335.5.end:
    func.assert..335.5.end:
;       [prog.baz:335:5] free scratch register r15
;   [prog.baz:337:8] pt.x(2)
;   [prog.baz:114:10] mut point.x(x)
    func.point.x..337.8:
;       [prog.baz:337:8] bind self -> pt
;       [prog.baz:337:8] bind x -> 2
;       [prog.baz:115:5] self.x = x
;       [prog.baz:115:14] self.x = 2
;       [prog.baz:115:14] src: folded constant 'x'
        mov qword [rbp + 608], 2
    func.point.x..337.8.end:
;   [prog.baz:338:5] assert(pt.x == 2)
;   [prog.baz:338:12] allocate scratch register -> r15
;   [prog.baz:338:12] ? pt.x == 2
    cmp.338.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.338.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..338.5:
;       [prog.baz:338:5] bind ok -> r15b
        if.38.24.338.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.338.5:
        test r15b, r15b
        jne if.38.24.338.5.end
        if.38.24.338.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.338.5.end:
    func.assert..338.5.end:
;       [prog.baz:338:5] free scratch register r15
;   [prog.baz:339:5] assert(pt.sum() == 0)
;   [prog.baz:339:12] allocate scratch register -> r15
;   [prog.baz:339:12] ? pt.sum() == 0
    cmp.339.12:
;   [prog.baz:339:12] allocate scratch register -> r14
;       [prog.baz:339:15] r14 = pt.sum()
;       [prog.baz:339:15] = expression
;       [prog.baz:339:15] pt.sum()
;       [prog.baz:54:6] point.sum() res
        func.point.sum..339.15:
;           [prog.baz:339:15] bind res -> r14
;           [prog.baz:339:15] bind self -> pt
;           [prog.baz:55:5] res = self.x + self.y
;           [prog.baz:55:11] self.x
            mov r14, qword [rbp + 608]
;           [prog.baz:55:20] res + self.y
;           [prog.baz:55:20] src: operand
            add r14, qword [rbp + 616]
            jo baz_overflow_line_55
        func.point.sum..339.15.end:
    test r14, r14
    sete r15b
;   [prog.baz:339:12] free scratch register r14
    bool.339.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..339.5:
;       [prog.baz:339:5] bind ok -> r15b
        if.38.24.339.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.339.5:
        test r15b, r15b
        jne if.38.24.339.5.end
        if.38.24.339.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.339.5.end:
    func.assert..339.5.end:
;       [prog.baz:339:5] free scratch register r15
;   [prog.baz:341:5] var x = 1
;   [prog.baz:341:9] x: i64 (8 B @ [rbp + 624])
;   [prog.baz:341:9] x = 1
;   [prog.baz:341:13] x = 1
;   [prog.baz:341:13] src: folded constant '1'
    mov qword [rbp + 624], 1
;   [prog.baz:342:5] var y = 2
;   [prog.baz:342:9] y: i64 (8 B @ [rbp + 632])
;   [prog.baz:342:9] y = 2
;   [prog.baz:342:13] y = 2
;   [prog.baz:342:13] src: folded constant '2'
    mov qword [rbp + 632], 2
;   [prog.baz:344:5] var o1 = object{{x * 10, y}, 0xff0000}
;   [prog.baz:344:9] o1: object (24 B @ [rbp + 640])
;   [prog.baz:344:9] o1 = object{{x * 10, y}, 0xff0000}
;   [prog.baz:344:21] copy field 'pos'
;   [prog.baz:344:22] copy field 'x'
;   [prog.baz:344:22] kept the second of two versions of 6 and 4 instructions, the smaller
;   [prog.baz:344:22] allocate scratch register -> r15
;   [prog.baz:344:22] x
    mov r15, qword [rbp + 624]
;   [prog.baz:344:22] r15 * 10
;   [prog.baz:344:22] src: folded constant '* 10'
    imul r15, 10
    jo baz_overflow_line_344
    mov qword [rbp + 640], r15
;   [prog.baz:344:22] free scratch register r15
;   [prog.baz:344:30] copy field 'y'
;   [prog.baz:344:30] y
;   [prog.baz:344:30] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
;   [prog.baz:344:30] free scratch register r15
;   [prog.baz:344:34] copy field 'color'
;   [prog.baz:344:34] o1.color = 16711680
;   [prog.baz:344:34] src: folded constant '0xff0000'
    mov dword [rbp + 656], 16711680
;   [prog.baz:344:14] zero padding: 4 B
;   [prog.baz:344:14] size <= 32 B, use mov
    mov dword [rbp + 660], 0
;   [prog.baz:345:5] assert(o1.pos.x == 10)
;   [prog.baz:345:12] allocate scratch register -> r15
;   [prog.baz:345:12] ? o1.pos.x == 10
    cmp.345.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.345.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..345.5:
;       [prog.baz:345:5] bind ok -> r15b
        if.38.24.345.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.345.5:
        test r15b, r15b
        jne if.38.24.345.5.end
        if.38.24.345.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.345.5.end:
    func.assert..345.5.end:
;       [prog.baz:345:5] free scratch register r15
;   [prog.baz:346:5] assert(o1.pos.y == 2)
;   [prog.baz:346:12] allocate scratch register -> r15
;   [prog.baz:346:12] ? o1.pos.y == 2
    cmp.346.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.346.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..346.5:
;       [prog.baz:346:5] bind ok -> r15b
        if.38.24.346.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.346.5:
        test r15b, r15b
        jne if.38.24.346.5.end
        if.38.24.346.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.346.5.end:
    func.assert..346.5.end:
;       [prog.baz:346:5] free scratch register r15
;   [prog.baz:347:5] assert(o1.color == 0xff0000)
;   [prog.baz:347:12] allocate scratch register -> r15
;   [prog.baz:347:12] ? o1.color == 0xff0000
    cmp.347.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.347.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..347.5:
;       [prog.baz:347:5] bind ok -> r15b
        if.38.24.347.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.347.5:
        test r15b, r15b
        jne if.38.24.347.5.end
        if.38.24.347.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.347.5.end:
    func.assert..347.5.end:
;       [prog.baz:347:5] free scratch register r15
;   [prog.baz:349:5] var p1 = point{-x, -y}
;   [prog.baz:349:9] p1: point (16 B @ [rbp + 664])
;   [prog.baz:349:9] p1 = point{-x, -y}
;   [prog.baz:349:20] copy field 'x'
;   [prog.baz:349:20] kept the first of two versions of 4 and 4 instructions, the first of equal sizes
;   [prog.baz:349:21] -x
;   [prog.baz:349:21] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
;   [prog.baz:349:21] free scratch register r15
    neg qword [rbp + 664]
    jo baz_overflow_line_349
;   [prog.baz:349:24] copy field 'y'
;   [prog.baz:349:24] kept the first of two versions of 4 and 4 instructions, the first of equal sizes
;   [prog.baz:349:25] -y
;   [prog.baz:349:25] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
;   [prog.baz:349:25] free scratch register r15
    neg qword [rbp + 672]
    jo baz_overflow_line_349
;   [prog.baz:350:5] o1.pos = p1
;   [prog.baz:350:14] size <= 16 B, use mov
;   [prog.baz:350:14] allocate scratch register -> r15
    mov r15, qword [rbp + 664]
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 672]
    mov qword [rbp + 648], r15
;   [prog.baz:350:14] free scratch register r15
;   [prog.baz:351:5] assert(o1.pos.x == -1)
;   [prog.baz:351:12] allocate scratch register -> r15
;   [prog.baz:351:12] ? o1.pos.x == -1
    cmp.351.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.351.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..351.5:
;       [prog.baz:351:5] bind ok -> r15b
        if.38.24.351.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.351.5:
        test r15b, r15b
        jne if.38.24.351.5.end
        if.38.24.351.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.351.5.end:
    func.assert..351.5.end:
;       [prog.baz:351:5] free scratch register r15
;   [prog.baz:352:5] assert(o1.pos.y == -2)
;   [prog.baz:352:12] allocate scratch register -> r15
;   [prog.baz:352:12] ? o1.pos.y == -2
    cmp.352.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.352.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..352.5:
;       [prog.baz:352:5] bind ok -> r15b
        if.38.24.352.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.352.5:
        test r15b, r15b
        jne if.38.24.352.5.end
        if.38.24.352.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.352.5.end:
    func.assert..352.5.end:
;       [prog.baz:352:5] free scratch register r15
;   [prog.baz:354:5] var o2 = o1
;   [prog.baz:354:9] o2: object (24 B @ [rbp + 680])
;   [prog.baz:354:9] o2 = o1
;   [prog.baz:354:14] allocate named register rsi
;   [prog.baz:354:14] allocate named register rdi
;   [prog.baz:354:14] allocate named register rcx
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
;   [prog.baz:354:14] free named register rcx
;   [prog.baz:354:14] free named register rdi
;   [prog.baz:354:14] free named register rsi
;   [prog.baz:355:5] assert(o2.pos.x == -1)
;   [prog.baz:355:12] allocate scratch register -> r15
;   [prog.baz:355:12] ? o2.pos.x == -1
    cmp.355.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.355.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..355.5:
;       [prog.baz:355:5] bind ok -> r15b
        if.38.24.355.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.355.5:
        test r15b, r15b
        jne if.38.24.355.5.end
        if.38.24.355.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.355.5.end:
    func.assert..355.5.end:
;       [prog.baz:355:5] free scratch register r15
;   [prog.baz:356:5] assert(o2.pos.y == -2)
;   [prog.baz:356:12] allocate scratch register -> r15
;   [prog.baz:356:12] ? o2.pos.y == -2
    cmp.356.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.356.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..356.5:
;       [prog.baz:356:5] bind ok -> r15b
        if.38.24.356.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.356.5:
        test r15b, r15b
        jne if.38.24.356.5.end
        if.38.24.356.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.356.5.end:
    func.assert..356.5.end:
;       [prog.baz:356:5] free scratch register r15
;   [prog.baz:357:5] assert(o2.color == 0xff0000)
;   [prog.baz:357:12] allocate scratch register -> r15
;   [prog.baz:357:12] ? o2.color == 0xff0000
    cmp.357.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.357.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..357.5:
;       [prog.baz:357:5] bind ok -> r15b
        if.38.24.357.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.357.5:
        test r15b, r15b
        jne if.38.24.357.5.end
        if.38.24.357.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.357.5.end:
    func.assert..357.5.end:
;       [prog.baz:357:5] free scratch register r15
;   [prog.baz:359:5] o2.pos = {x, y}
;   [prog.baz:359:15] copy field 'x'
;   [prog.baz:359:15] x
;   [prog.baz:359:15] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
;   [prog.baz:359:15] free scratch register r15
;   [prog.baz:359:18] copy field 'y'
;   [prog.baz:359:18] y
;   [prog.baz:359:18] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
;   [prog.baz:359:18] free scratch register r15
;   [prog.baz:360:5] assert(o2.pos.x == 1)
;   [prog.baz:360:12] allocate scratch register -> r15
;   [prog.baz:360:12] ? o2.pos.x == 1
    cmp.360.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.360.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..360.5:
;       [prog.baz:360:5] bind ok -> r15b
        if.38.24.360.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.360.5:
        test r15b, r15b
        jne if.38.24.360.5.end
        if.38.24.360.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.360.5.end:
    func.assert..360.5.end:
;       [prog.baz:360:5] free scratch register r15
;   [prog.baz:361:5] o2.pos = {y, x}
;   [prog.baz:361:15] copy field 'x'
;   [prog.baz:361:15] y
;   [prog.baz:361:15] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
;   [prog.baz:361:15] free scratch register r15
;   [prog.baz:361:18] copy field 'y'
;   [prog.baz:361:18] x
;   [prog.baz:361:18] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
;   [prog.baz:361:18] free scratch register r15
;   [prog.baz:362:5] assert(o2.pos.x == 2)
;   [prog.baz:362:12] allocate scratch register -> r15
;   [prog.baz:362:12] ? o2.pos.x == 2
    cmp.362.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.362.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..362.5:
;       [prog.baz:362:5] bind ok -> r15b
        if.38.24.362.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.362.5:
        test r15b, r15b
        jne if.38.24.362.5.end
        if.38.24.362.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.362.5.end:
    func.assert..362.5.end:
;       [prog.baz:362:5] free scratch register r15
;   [prog.baz:369:5] var o3 = object[2]
;   [prog.baz:369:9] o3: object[2] (48 B @ [rbp + 704])
;   [prog.baz:369:9] o3 = object[2]
;   [prog.baz:369:14] zero remaining elements: 2 * 24 B = 48 B
;   [prog.baz:369:14] allocate named register rax
;   [prog.baz:369:14] allocate named register rdi
;   [prog.baz:369:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
;   [prog.baz:369:14] free named register rcx
;   [prog.baz:369:14] free named register rdi
;   [prog.baz:369:14] free named register rax
;   [prog.baz:370:5] o3[0].pos.y = 73
;   [prog.baz:370:19] o3[0].pos.y = 73
;   [prog.baz:370:19] src: folded constant '73'
    mov qword [rbp + 712], 73
;   [prog.baz:372:5] assert(o3[0].pos.y == 73)
;   [prog.baz:372:12] allocate scratch register -> r15
;   [prog.baz:372:12] ? o3[0].pos.y == 73
    cmp.372.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.372.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..372.5:
;       [prog.baz:372:5] bind ok -> r15b
        if.38.24.372.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.372.5:
        test r15b, r15b
        jne if.38.24.372.5.end
        if.38.24.372.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.372.5.end:
    func.assert..372.5.end:
;       [prog.baz:372:5] free scratch register r15
;   [prog.baz:373:5] o3[1] = object.at(2, 74, 0xffffff)
;   [prog.baz:373:13] object.at(2, 74, 0xffffff)
;   [prog.baz:373:13] zero padding: 4 B
;   [prog.baz:373:13] size <= 32 B, use mov
    mov dword [rbp + 748], 0
;   [prog.baz:119:6] object.at(x, y, color i32) self
    func.object.at..373.13:
;       [prog.baz:373:13] bind self -> o3[1] (address: rbp + 728)
;       [prog.baz:373:13] bind x -> 2
;       [prog.baz:373:13] bind y -> 74
;       [prog.baz:373:13] bind color -> 16777215
;       [prog.baz:120:5] self.pos = point.at(x, y)
;       [prog.baz:120:16] point.at(x, y)
;       [prog.baz:108:6] point.at(x, y) self
        func.point.at..120.16.373.13:
;           [prog.baz:120:16] bind self -> self.pos (address: rbp + 728)
;           [prog.baz:120:16] bind x -> 2
;           [prog.baz:120:16] bind y -> 74
;           [prog.baz:109:5] self.x = x
;           [prog.baz:109:14] self.x = 2
;           [prog.baz:109:14] src: folded constant 'x'
            mov qword [rbp + 728], 2
;           [prog.baz:110:5] self.y = y
;           [prog.baz:110:14] self.y = 74
;           [prog.baz:110:14] src: folded constant 'y'
            mov qword [rbp + 736], 74
        func.point.at..120.16.373.13.end:
;       [prog.baz:121:5] self.color = color
;       [prog.baz:121:18] self.color = 16777215
;       [prog.baz:121:18] src: folded constant 'color'
        mov dword [rbp + 744], 16777215
    func.object.at..373.13.end:
;   [prog.baz:374:5] assert(o3[1].pos.y == 74)
;   [prog.baz:374:12] allocate scratch register -> r15
;   [prog.baz:374:12] ? o3[1].pos.y == 74
    cmp.374.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.374.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..374.5:
;       [prog.baz:374:5] bind ok -> r15b
        if.38.24.374.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.374.5:
        test r15b, r15b
        jne if.38.24.374.5.end
        if.38.24.374.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.374.5.end:
    func.assert..374.5.end:
;       [prog.baz:374:5] free scratch register r15
;   [prog.baz:376:15] o3[1].pos.fooz()
;   [prog.baz:47:10] mut point.fooz()
    func.point.fooz..376.15:
;       [prog.baz:376:15] bind self -> o3[1].pos (address: rbp + 728)
;       [prog.baz:48:5] self.x = 0b10
;       [prog.baz:48:14] self.x = 2
;       [prog.baz:48:14] src: folded constant '0b10'
        mov qword [rbp + 728], 2
;       [prog.baz:49:5] self.y = 0xb
;       [prog.baz:49:14] self.y = 11
;       [prog.baz:49:14] src: folded constant '0xb'
        mov qword [rbp + 736], 11
    func.point.fooz..376.15.end:
;   [prog.baz:377:5] assert(o3[1].pos.sum() == 13)
;   [prog.baz:377:12] allocate scratch register -> r15
;   [prog.baz:377:12] ? o3[1].pos.sum() == 13
    cmp.377.12:
;   [prog.baz:377:12] allocate scratch register -> r14
;       [prog.baz:377:22] r14 = o3[1].pos.sum()
;       [prog.baz:377:22] = expression
;       [prog.baz:377:22] o3[1].pos.sum()
;       [prog.baz:54:6] point.sum() res
        func.point.sum..377.22:
;           [prog.baz:377:22] bind res -> r14
;           [prog.baz:377:22] bind self -> o3[1].pos (address: rbp + 728)
;           [prog.baz:55:5] res = self.x + self.y
;           [prog.baz:55:11] self.x
            mov r14, qword [rbp + 728]
;           [prog.baz:55:20] res + self.y
;           [prog.baz:55:20] src: operand
            add r14, qword [rbp + 736]
            jo baz_overflow_line_55
        func.point.sum..377.22.end:
    cmp r14, 13
    sete r15b
;   [prog.baz:377:12] free scratch register r14
    bool.377.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..377.5:
;       [prog.baz:377:5] bind ok -> r15b
        if.38.24.377.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.377.5:
        test r15b, r15b
        jne if.38.24.377.5.end
        if.38.24.377.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.377.5.end:
    func.assert..377.5.end:
;       [prog.baz:377:5] free scratch register r15
;   [prog.baz:380:5] var worlds = world[8]
;   [prog.baz:380:9] worlds: world[8] (512 B @ [rbp + 752])
;   [prog.baz:380:9] worlds = world[8]
;   [prog.baz:380:18] zero remaining elements: 8 * 64 B = 512 B
;   [prog.baz:380:18] allocate named register rax
;   [prog.baz:380:18] allocate named register rdi
;   [prog.baz:380:18] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
;   [prog.baz:380:18] free named register rcx
;   [prog.baz:380:18] free named register rdi
;   [prog.baz:380:18] free named register rax
;   [prog.baz:381:5] worlds[1].locations[1] = 0xffee
;   [prog.baz:381:30] worlds[1].locations[1] = 65518
;   [prog.baz:381:30] src: folded constant '0xffee'
    mov qword [rbp + 824], 65518
;   [prog.baz:382:5] assert(worlds[1].locations[1] == 0xffee)
;   [prog.baz:382:12] allocate scratch register -> r15
;   [prog.baz:382:12] ? worlds[1].locations[1] == 0xffee
    cmp.382.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.382.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..382.5:
;       [prog.baz:382:5] bind ok -> r15b
        if.38.24.382.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.382.5:
        test r15b, r15b
        jne if.38.24.382.5.end
        if.38.24.382.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.382.5.end:
    func.assert..382.5.end:
;       [prog.baz:382:5] free scratch register r15
;   [prog.baz:384:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [prog.baz:384:5] allocate named register rsi
;   [prog.baz:384:5] allocate named register rdi
;   [prog.baz:384:5] allocate named register rcx
;   [prog.baz:385:9] worlds[1].locations
    lea rsi, [rbp + 816]
;   [prog.baz:386:9] worlds[0].locations
    lea rdi, [rbp + 752]
;   [prog.baz:387:9] rcx = 8
;   [prog.baz:387:9] src: folded constant 'array_length(worlds[0].locations)'
    mov rcx, 8
    sal rcx, 3
;   [prog.baz:384:5] overlap check begin
;   [prog.baz:384:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [prog.baz:384:5] the same range is not an overlap
    je .Lbaz_overlap.6
;   [prog.baz:384:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_384
    .Lbaz_overlap.6:
;   [prog.baz:384:5] free scratch register r15
;   [prog.baz:384:5] overlap check end
    rep movsb
;   [prog.baz:384:5] free named register rcx
;   [prog.baz:384:5] free named register rdi
;   [prog.baz:384:5] free named register rsi
;   [prog.baz:391:5] assert(worlds[0].locations[1] == 0xffee)
;   [prog.baz:391:12] allocate scratch register -> r15
;   [prog.baz:391:12] ? worlds[0].locations[1] == 0xffee
    cmp.391.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.391.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..391.5:
;       [prog.baz:391:5] bind ok -> r15b
        if.38.24.391.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.391.5:
        test r15b, r15b
        jne if.38.24.391.5.end
        if.38.24.391.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.391.5.end:
    func.assert..391.5.end:
;       [prog.baz:391:5] free scratch register r15
;   [prog.baz:392:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [prog.baz:392:12] allocate scratch register -> r15
;   [prog.baz:392:12] r15b = arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [prog.baz:392:12] = expression
;   [prog.baz:392:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [prog.baz:392:12] allocate named register rsi
;   [prog.baz:392:12] allocate named register rdi
;   [prog.baz:392:12] allocate named register rcx
;   [prog.baz:393:14] worlds[0].locations
    lea rsi, [rbp + 752]
;   [prog.baz:394:14] worlds[1].locations
    lea rdi, [rbp + 816]
    mov rcx, 8
    repe cmpsq
;   [prog.baz:392:12] free named register rcx
;   [prog.baz:392:12] free named register rdi
;   [prog.baz:392:12] free named register rsi
    sete r15b
;   [prog.baz:38:6] assert(ok bool)
    func.assert..392.5:
;       [prog.baz:392:5] bind ok -> r15b
        if.38.24.392.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.392.5:
        test r15b, r15b
        jne if.38.24.392.5.end
        if.38.24.392.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.392.5.end:
    func.assert..392.5.end:
;       [prog.baz:392:5] free scratch register r15
;   [prog.baz:398:5] var arr2 = []{ -1, 2 }
;   [prog.baz:398:9] arr2: i64[2] (16 B @ [rbp + 1264])
;   [prog.baz:398:9] arr2 = []{ -1, 2 }
;   [prog.baz:398:18] size <= 16 B, use immediates
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
;   [prog.baz:399:5] assert(array_length(arr2) == 2)
;   [prog.baz:38:6] assert(ok bool)
    func.assert..399.5:
;       [prog.baz:399:5] bind ok -> 1
        if.38.24.399.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.399.5:
;       [prog.baz:38:31] const eval to false
        if.38.24.399.5.end:
    func.assert..399.5.end:
;   [prog.baz:400:5] assert(arr2[0] == -1)
;   [prog.baz:400:12] allocate scratch register -> r15
;   [prog.baz:400:12] ? arr2[0] == -1
    cmp.400.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.400.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..400.5:
;       [prog.baz:400:5] bind ok -> r15b
        if.38.24.400.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.400.5:
        test r15b, r15b
        jne if.38.24.400.5.end
        if.38.24.400.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.400.5.end:
    func.assert..400.5.end:
;       [prog.baz:400:5] free scratch register r15
;   [prog.baz:401:5] assert(arr2[1] == 2)
;   [prog.baz:401:12] allocate scratch register -> r15
;   [prog.baz:401:12] ? arr2[1] == 2
    cmp.401.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.401.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert..401.5:
;       [prog.baz:401:5] bind ok -> r15b
        if.38.24.401.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.401.5:
        test r15b, r15b
        jne if.38.24.401.5.end
        if.38.24.401.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            jmp baz_exit
;           [prog.baz:38:34] free named register rdi
        if.38.24.401.5.end:
    func.assert..401.5.end:
;       [prog.baz:401:5] free scratch register r15
;   [prog.baz:403:5] var counter = 0
;   [prog.baz:403:9] counter: i64 (8 B @ [rbp + 1280])
;   [prog.baz:403:9] counter = 0
;   [prog.baz:403:19] counter = 0
;   [prog.baz:403:19] src: folded constant '0'
    mov qword [rbp + 1280], 0
;   [prog.baz:404:5] var nm = str
;   [prog.baz:404:9] nm: str (128 B @ [rbp + 1288])
;   [prog.baz:404:9] nm = str
;   [prog.baz:404:14] zero remaining fields: 128 B
;   [prog.baz:404:14] allocate named register rax
;   [prog.baz:404:14] allocate named register rdi
;   [prog.baz:404:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
;   [prog.baz:404:14] free named register rcx
;   [prog.baz:404:14] free named register rdi
;   [prog.baz:404:14] free named register rax
;   [prog.baz:405:5] print(hello)
;   [prog.baz:41:6] print(str i8[])
    func.print..405.5:
;       [prog.baz:405:5] bind str -> hello
;       [prog.baz:42:5] write(1, str)
;       [prog.baz:42:5] allocate named register rdi
;       [prog.baz:42:5] allocate named register rsi
;       [prog.baz:42:5] allocate named register rdx
;       [prog.baz:42:11] rdi = 1
;       [prog.baz:42:11] src: folded constant '1'
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
;       [prog.baz:42:5] allocate named register rax
        mov rax, 1
        syscall
;       [prog.baz:42:5] free named register rax
;       [prog.baz:42:5] free named register rdx
;       [prog.baz:42:5] free named register rsi
;       [prog.baz:42:5] free named register rdi
    func.print..405.5.end:
;   [prog.baz:406:5] loop
    loop.406.5:
;       [prog.baz:407:9] counter = counter + 1
;       [prog.baz:407:19] kept the first of two versions of 2 and 4 instructions, the smaller
;       [prog.baz:407:19] counter
;       [prog.baz:407:19] counter + 1
;       [prog.baz:407:19] src: folded constant '+ 1'
        inc qword [rbp + 1280]
        jo baz_overflow_line_407
;       [prog.baz:408:9] print_num(counter)
;       [prog.baz:408:9] frame capacity check begin
;       [prog.baz:408:9] callee storage starts after the caller's storage (rbp + 1416)
;       [prog.baz:408:9] allocate scratch register -> r15
        mov r15, strict qword vars.end - size.func.print_num - 1416
        cmp r15, rbp
        jl baz_frame_overflow
;       [prog.baz:408:9] free scratch register r15
;       [prog.baz:408:9] frame capacity check end
;       [prog.baz:408:9] address of argument 'counter' to parameter 'num'
;       [prog.baz:408:9] allocate scratch register -> r15
        lea r15, [rbp + 1280]
        mov qword [rbp + 1416], r15
;       [prog.baz:408:9] free scratch register r15
;       [prog.baz:408:9] set function frame base
        lea rbx, [rbp + 1416]
        call func.print_num
;       [prog.baz:409:9] print(colon)
;       [prog.baz:41:6] print(str i8[])
        func.print..409.9:
;           [prog.baz:409:9] bind str -> colon
;           [prog.baz:42:5] write(1, str)
;           [prog.baz:42:5] allocate named register rdi
;           [prog.baz:42:5] allocate named register rsi
;           [prog.baz:42:5] allocate named register rdx
;           [prog.baz:42:11] rdi = 1
;           [prog.baz:42:11] src: folded constant '1'
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
;           [prog.baz:42:5] allocate named register rax
            mov rax, 1
            syscall
;           [prog.baz:42:5] free named register rax
;           [prog.baz:42:5] free named register rdx
;           [prog.baz:42:5] free named register rsi
;           [prog.baz:42:5] free named register rdi
        func.print..409.9.end:
;       [prog.baz:410:9] print(prompt1)
;       [prog.baz:41:6] print(str i8[])
        func.print..410.9:
;           [prog.baz:410:9] bind str -> prompt1
;           [prog.baz:42:5] write(1, str)
;           [prog.baz:42:5] allocate named register rdi
;           [prog.baz:42:5] allocate named register rsi
;           [prog.baz:42:5] allocate named register rdx
;           [prog.baz:42:11] rdi = 1
;           [prog.baz:42:11] src: folded constant '1'
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
;           [prog.baz:42:5] allocate named register rax
            mov rax, 1
            syscall
;           [prog.baz:42:5] free named register rax
;           [prog.baz:42:5] free named register rdx
;           [prog.baz:42:5] free named register rsi
;           [prog.baz:42:5] free named register rdi
        func.print..410.9.end:
;       [prog.baz:411:12] nm.input()
;       [prog.baz:88:10] mut str.input()
        func.str.input..411.12:
;           [prog.baz:411:12] bind self -> nm
;           [prog.baz:89:5] var nbytes = read(0, self.data)
;           [prog.baz:89:9] nbytes: i64 (8 B @ [rbp + 1416])
;           [prog.baz:89:9] nbytes = read(0, self.data)
;           [prog.baz:89:18] nbytes = read(0, self.data)
;           [prog.baz:89:18] = expression
;           [prog.baz:89:18] read(0, self.data)
;           [prog.baz:89:18] allocate named register rdi
;           [prog.baz:89:18] allocate named register rsi
;           [prog.baz:89:18] allocate named register rdx
;           [prog.baz:89:23] rdi = 0
;           [prog.baz:89:23] src: folded constant '0'
            mov rdi, 0
            lea rsi, [rbp + 1289]
            mov rdx, 127
;           [prog.baz:89:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
;           [prog.baz:89:18] free named register rax
;           [prog.baz:89:18] free named register rdx
;           [prog.baz:89:18] free named register rsi
;           [prog.baz:89:18] free named register rdi
;           [prog.baz:92:5] self.len = i8(nbytes - 1)
;           [prog.baz:92:16] self.len = i8(nbytes - 1)
;           [prog.baz:92:16] = expression
;           [prog.baz:92:16] allocate scratch register -> r15
;           [prog.baz:92:19] nbytes
            mov r15, qword [rbp + 1416]
;           [prog.baz:92:19] r15 - 1
;           [prog.baz:92:19] src: folded constant '- 1'
            dec r15
            mov byte [rbp + 1288], r15b
;           [prog.baz:92:16] free scratch register r15
        func.str.input..411.12.end:
        if.413.9:
;       [prog.baz:413:12] ? nm.len <= 0
        cmp.413.12:
        cmp byte [rbp + 1288], 0
        jle loop.406.5.end
        if.413.9.code:
;           [prog.baz:414:13] break
        if.413.9.1:
;       [prog.baz:415:19] ? nm.len <= 4
        cmp.415.19:
        cmp byte [rbp + 1288], 4
        jg if.413.9.else
        if.413.9.1.code:
;           [prog.baz:416:13] print(prompt2)
;           [prog.baz:41:6] print(str i8[])
            func.print..416.13:
;               [prog.baz:416:13] bind str -> prompt2
;               [prog.baz:42:5] write(1, str)
;               [prog.baz:42:5] allocate named register rdi
;               [prog.baz:42:5] allocate named register rsi
;               [prog.baz:42:5] allocate named register rdx
;               [prog.baz:42:11] rdi = 1
;               [prog.baz:42:11] src: folded constant '1'
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
;               [prog.baz:42:5] allocate named register rax
                mov rax, 1
                syscall
;               [prog.baz:42:5] free named register rax
;               [prog.baz:42:5] free named register rdx
;               [prog.baz:42:5] free named register rsi
;               [prog.baz:42:5] free named register rdi
            func.print..416.13.end:
;           [prog.baz:417:13] continue
            jmp loop.406.5
        if.413.9.else:
;           [prog.baz:419:13] greet(nm)
;           [prog.baz:99:6] greet(name str)
            func.greet..419.13:
;               [prog.baz:419:13] bind name -> nm
;               [prog.baz:100:5] print(prompt3)
;               [prog.baz:41:6] print(str i8[])
                func.print..100.5.419.13:
;                   [prog.baz:100:5] bind str -> prompt3
;                   [prog.baz:42:5] write(1, str)
;                   [prog.baz:42:5] allocate named register rdi
;                   [prog.baz:42:5] allocate named register rsi
;                   [prog.baz:42:5] allocate named register rdx
;                   [prog.baz:42:11] rdi = 1
;                   [prog.baz:42:11] src: folded constant '1'
                    mov rdi, 1
                    lea rsi, [rbp + 53]
                    mov rdx, 6
;                   [prog.baz:42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:42:5] free named register rax
;                   [prog.baz:42:5] free named register rdx
;                   [prog.baz:42:5] free named register rsi
;                   [prog.baz:42:5] free named register rdi
                func.print..100.5.419.13.end:
;               [prog.baz:101:10] name.print()
;               [prog.baz:95:6] str.print()
                func.str.print..101.10.419.13:
;                   [prog.baz:101:10] bind self -> name
;                   [prog.baz:96:5] write(1, self.data, self.len)
;                   [prog.baz:96:5] allocate named register rdi
;                   [prog.baz:96:5] allocate named register rsi
;                   [prog.baz:96:5] allocate named register rdx
;                   [prog.baz:96:11] rdi = 1
;                   [prog.baz:96:11] src: folded constant '1'
                    mov rdi, 1
                    lea rsi, [rbp + 1289]
;                   [prog.baz:96:25] self.len
                    movsx rdx, byte [rbp + 1288]
;                   [prog.baz:96:25] bounds check begin
;                   [prog.baz:96:25] lower bound
;                   [prog.baz:96:25] rdx lower bound covered by the unsigned upper bound
;                   [prog.baz:96:25] upper bound
                    cmp rdx, 127
                    ja baz_bounds_line_96
;                   [prog.baz:96:25] bounds check end
;                   [prog.baz:96:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:96:5] free named register rax
;                   [prog.baz:96:5] free named register rdx
;                   [prog.baz:96:5] free named register rsi
;                   [prog.baz:96:5] free named register rdi
                func.str.print..101.10.419.13.end:
;               [prog.baz:102:5] print(dot)
;               [prog.baz:41:6] print(str i8[])
                func.print..102.5.419.13:
;                   [prog.baz:102:5] bind str -> dot
;                   [prog.baz:42:5] write(1, str)
;                   [prog.baz:42:5] allocate named register rdi
;                   [prog.baz:42:5] allocate named register rsi
;                   [prog.baz:42:5] allocate named register rdx
;                   [prog.baz:42:11] rdi = 1
;                   [prog.baz:42:11] src: folded constant '1'
                    mov rdi, 1
                    lea rsi, [rbp + 59]
                    mov rdx, 1
;                   [prog.baz:42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:42:5] free named register rax
;                   [prog.baz:42:5] free named register rdx
;                   [prog.baz:42:5] free named register rsi
;                   [prog.baz:42:5] free named register rdi
                func.print..102.5.419.13.end:
;               [prog.baz:103:5] print(nl)
;               [prog.baz:41:6] print(str i8[])
                func.print..103.5.419.13:
;                   [prog.baz:103:5] bind str -> nl
;                   [prog.baz:42:5] write(1, str)
;                   [prog.baz:42:5] allocate named register rdi
;                   [prog.baz:42:5] allocate named register rsi
;                   [prog.baz:42:5] allocate named register rdx
;                   [prog.baz:42:11] rdi = 1
;                   [prog.baz:42:11] src: folded constant '1'
                    mov rdi, 1
                    lea rsi, [rbp + 60]
                    mov rdx, 1
;                   [prog.baz:42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:42:5] free named register rax
;                   [prog.baz:42:5] free named register rdx
;                   [prog.baz:42:5] free named register rsi
;                   [prog.baz:42:5] free named register rdi
                func.print..103.5.419.13.end:
;               [prog.baz:104:5] names = names + 1
;               [prog.baz:104:13] kept the first of two versions of 2 and 4 instructions, the smaller
;               [prog.baz:104:13] names
;               [prog.baz:104:13] names + 1
;               [prog.baz:104:13] src: folded constant '+ 1'
                inc qword [rbp + 368]
                jo baz_overflow_line_104
            func.greet..419.13.end:
        if.413.9.end:
    jmp loop.406.5
    loop.406.5.end:
;   [prog.baz:423:5] print(greeted)
;   [prog.baz:41:6] print(str i8[])
    func.print..423.5:
;       [prog.baz:423:5] bind str -> greeted
;       [prog.baz:42:5] write(1, str)
;       [prog.baz:42:5] allocate named register rdi
;       [prog.baz:42:5] allocate named register rsi
;       [prog.baz:42:5] allocate named register rdx
;       [prog.baz:42:11] rdi = 1
;       [prog.baz:42:11] src: folded constant '1'
        mov rdi, 1
        lea rsi, [rbp + 352]
        mov rdx, 15
;       [prog.baz:42:5] allocate named register rax
        mov rax, 1
        syscall
;       [prog.baz:42:5] free named register rax
;       [prog.baz:42:5] free named register rdx
;       [prog.baz:42:5] free named register rsi
;       [prog.baz:42:5] free named register rdi
    func.print..423.5.end:
;   [prog.baz:424:5] print_num(names)
;   [prog.baz:424:5] frame capacity check begin
;   [prog.baz:424:5] callee storage starts after the caller's storage (rbp + 1416)
;   [prog.baz:424:5] allocate scratch register -> r15
    mov r15, strict qword vars.end - size.func.print_num - 1416
    cmp r15, rbp
    jl baz_frame_overflow
;   [prog.baz:424:5] free scratch register r15
;   [prog.baz:424:5] frame capacity check end
;   [prog.baz:424:5] address of argument 'names' to parameter 'num'
;   [prog.baz:424:5] allocate scratch register -> r15
    lea r15, [rbp + 368]
    mov qword [rbp + 1416], r15
;   [prog.baz:424:5] free scratch register r15
;   [prog.baz:424:5] set function frame base
    lea rbx, [rbp + 1416]
    call func.print_num
;   [prog.baz:425:5] print(nl)
;   [prog.baz:41:6] print(str i8[])
    func.print..425.5:
;       [prog.baz:425:5] bind str -> nl
;       [prog.baz:42:5] write(1, str)
;       [prog.baz:42:5] allocate named register rdi
;       [prog.baz:42:5] allocate named register rsi
;       [prog.baz:42:5] allocate named register rdx
;       [prog.baz:42:11] rdi = 1
;       [prog.baz:42:11] src: folded constant '1'
        mov rdi, 1
        lea rsi, [rbp + 60]
        mov rdx, 1
;       [prog.baz:42:5] allocate named register rax
        mov rax, 1
        syscall
;       [prog.baz:42:5] free named register rax
;       [prog.baz:42:5] free named register rdx
;       [prog.baz:42:5] free named register rsi
;       [prog.baz:42:5] free named register rdi
    func.print..425.5.end:
;   [prog.baz:427:5] var bye = "bye from baz\n"
;   [prog.baz:427:9] bye: i8[13] (13 B @ [rbp + 1416])
;   [prog.baz:427:9] bye = "bye from baz\n"
;   [prog.baz:427:15] size <= 16 B, use immediates
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
;   [prog.baz:428:5] write(1, bye, 3)
;   [prog.baz:428:5] allocate named register rdi
;   [prog.baz:428:5] allocate named register rsi
;   [prog.baz:428:5] allocate named register rdx
;   [prog.baz:428:11] rdi = 1
;   [prog.baz:428:11] src: folded constant '1'
    mov rdi, 1
    lea rsi, [rbp + 1416]
;   [prog.baz:428:19] rdx = 3
;   [prog.baz:428:19] src: folded constant '3'
    mov rdx, 3
;   [prog.baz:428:5] allocate named register rax
    mov rax, 1
    syscall
;   [prog.baz:428:5] free named register rax
;   [prog.baz:428:5] free named register rdx
;   [prog.baz:428:5] free named register rsi
;   [prog.baz:428:5] free named register rdi
;   [prog.baz:429:5] write(1, bye, 1, array_length(bye) - 1)
;   [prog.baz:429:5] allocate named register rdi
;   [prog.baz:429:5] allocate named register rsi
;   [prog.baz:429:5] allocate named register rdx
;   [prog.baz:429:11] rdi = 1
;   [prog.baz:429:11] src: folded constant '1'
    mov rdi, 1
    lea rsi, [rbp + 1416]
;   [prog.baz:429:19] rdx = 1
;   [prog.baz:429:19] src: folded constant '1'
    mov rdx, 1
    add rsi, 12
;   [prog.baz:429:5] allocate named register rax
    mov rax, 1
    syscall
;   [prog.baz:429:5] free named register rax
;   [prog.baz:429:5] free named register rdx
;   [prog.baz:429:5] free named register rsi
;   [prog.baz:429:5] free named register rdi
    mov rdi, 0
    jmp baz_exit
;
;[prog.baz:176:15] noinline factorial(n) res
func.factorial:
; allocate named register rbx
;   [prog.baz:176:28] res: i64 (address @ [rbx])
;   [prog.baz:176:25] n: i64 (address @ [rbx + 8])
;   [prog.baz:177:5] res = 1
;   [prog.baz:177:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [prog.baz:177:11] res = 1
;   [prog.baz:177:11] src: folded constant '1'
    mov qword [r15], 1
;   [prog.baz:177:5] free scratch register r15
    if.178.5:
;   [prog.baz:178:8] ? n <= 1
    cmp.178.8:
;   [prog.baz:178:8] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.178.5.end
;   [prog.baz:178:8] free scratch register r15
    if.178.5.code:
;       [prog.baz:178:15] return
        ret
    if.178.5.end:
;   [prog.baz:180:5] var m = n - 1
;   [prog.baz:180:9] m: i64 (8 B @ [rbx + 16])
;   [prog.baz:180:9] m = n - 1
;   [prog.baz:180:13] kept the first of two versions of 5 and 5 instructions, the first of equal sizes
;   [prog.baz:180:13] n
;   [prog.baz:180:13] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
;   [prog.baz:180:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
;   [prog.baz:180:13] free scratch register r14
;   [prog.baz:180:13] free scratch register r15
;   [prog.baz:180:13] m - 1
;   [prog.baz:180:13] src: folded constant '- 1'
    dec qword [rbx + 16]
    jo baz_overflow_line_180
;   [prog.baz:181:5] var partial = factorial(m)
;   [prog.baz:181:9] partial: i64 (8 B @ [rbx + 24])
;   [prog.baz:181:9] partial = factorial(m)
;   [prog.baz:181:19] partial = factorial(m)
;   [prog.baz:181:19] = expression
;   [prog.baz:181:19] factorial(m)
;   [prog.baz:181:19] frame capacity check begin
;   [prog.baz:181:19] callee storage starts after the caller's storage (rbx + 32)
;   [prog.baz:181:19] allocate scratch register -> r15
    mov r15, strict qword vars.end - size.func.factorial - 32
    cmp r15, rbx
    jl baz_frame_overflow
;   [prog.baz:181:19] free scratch register r15
;   [prog.baz:181:19] frame capacity check end
;   [prog.baz:181:19] result address in callee frame
;   [prog.baz:181:19] allocate scratch register -> r15
    lea r15, [rbx + 24]
    mov qword [rbx + 32], r15
;   [prog.baz:181:19] free scratch register r15
;   [prog.baz:181:19] address of argument 'm' to parameter 'n'
;   [prog.baz:181:19] allocate scratch register -> r15
    lea r15, [rbx + 16]
    mov qword [rbx + 40], r15
;   [prog.baz:181:19] free scratch register r15
;   [prog.baz:181:19] before call: save allocated registers
    push rbx
;   [prog.baz:181:19] set function frame base
    lea rbx, [rbx + 32]
    call func.factorial
;   [prog.baz:181:19] after call: restore saved registers
    pop rbx
;   [prog.baz:182:5] res = n * partial
;   [prog.baz:182:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [prog.baz:182:11] kept the second of two versions of 7 and 5 instructions, the smaller
;   [prog.baz:182:11] allocate scratch register -> r14
;   [prog.baz:182:11] n
;   [prog.baz:182:11] allocate scratch register -> r13
    mov r13, qword [rbx + 8]
    mov r14, qword [r13]
;   [prog.baz:182:11] free scratch register r13
;   [prog.baz:182:15] r14 * partial
;   [prog.baz:182:15] src: operand
    imul r14, qword [rbx + 24]
    jo baz_overflow_line_182
    mov qword [r15], r14
;   [prog.baz:182:11] free scratch register r14
;   [prog.baz:182:5] free scratch register r15
    ret
; free named register rbx
size.func.factorial equ 32
;
;[prog.baz:135:15] noinline print_num(num)
func.print_num:
; allocate named register rbx
;   [prog.baz:135:25] num: i64 (address @ [rbx])
;   [prog.baz:137:5] let buf_count = 20
;   [prog.baz:137:9] const buf_count = 20
;   [prog.baz:139:5] var buf = i8[buf_count]
;   [prog.baz:139:9] buf: i8[20] (20 B @ [rbx + 8])
;   [prog.baz:139:9] buf = i8[buf_count]
;   [prog.baz:139:15] zero remaining elements: 20 * 1 B = 20 B
;   [prog.baz:139:15] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [prog.baz:140:5] var n = num
;   [prog.baz:140:9] n: i64 (8 B @ [rbx + 32])
;   [prog.baz:140:9] n = num
;   [prog.baz:140:13] num
;   [prog.baz:140:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [prog.baz:140:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [prog.baz:140:13] free scratch register r14
;   [prog.baz:140:13] free scratch register r15
;   [prog.baz:141:5] var is_negative = false
;   [prog.baz:141:9] is_negative: bool (1 B @ [rbx + 40])
;   [prog.baz:141:9] is_negative = false
;   [prog.baz:141:23] is_negative = 0
;   [prog.baz:141:23] src: folded constant 'false'
    mov byte [rbx + 40], 0
    if.145.5:
;   [prog.baz:145:8] ? n < 0
    cmp.145.8:
    cmp qword [rbx + 32], 0
    jge if.145.5.end
    if.145.5.code:
;       [prog.baz:146:9] is_negative = true
;       [prog.baz:146:23] is_negative = 1
;       [prog.baz:146:23] src: folded constant 'true'
        mov byte [rbx + 40], 1
    if.145.5.end:
    if.148.5:
;   [prog.baz:148:8] ? n > 0
    cmp.148.8:
    cmp qword [rbx + 32], 0
    jle if.148.5.end
    if.148.5.code:
;       [prog.baz:149:9] n = -n
;       [prog.baz:149:13] kept the first of two versions of 2 and 4 instructions, the smaller
;       [prog.baz:149:14] -n
        neg qword [rbx + 32]
        jo baz_overflow_line_149
    if.148.5.end:
;   [prog.baz:152:5] var i = buf_count
;   [prog.baz:152:9] i: i64 (8 B @ [rbx + 48])
;   [prog.baz:152:9] i = buf_count
;   [prog.baz:152:13] i = 20
;   [prog.baz:152:13] src: folded constant 'buf_count'
    mov qword [rbx + 48], 20
;   [prog.baz:153:5] loop
    loop.153.5:
;       [prog.baz:154:9] i = i - 1
;       [prog.baz:154:13] kept the first of two versions of 2 and 4 instructions, the smaller
;       [prog.baz:154:13] i
;       [prog.baz:154:13] i - 1
;       [prog.baz:154:13] src: folded constant '- 1'
        dec qword [rbx + 48]
        jo baz_overflow_line_154
;       [prog.baz:155:9] buf[i] = i8('0' - n % 10)
;       [prog.baz:155:13] allocate scratch register -> r15
;       [prog.baz:155:13] set array index
;       [prog.baz:155:13] i
        mov r15, qword [rbx + 48]
;       [prog.baz:155:13] bounds check begin
;       [prog.baz:155:13] lower bound
;       [prog.baz:155:13] r15 lower bound covered by the unsigned upper bound
;       [prog.baz:155:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_155
;       [prog.baz:155:13] bounds check end
;       [prog.baz:155:18] buf[i] = i8('0' - n % 10)
;       [prog.baz:155:18] = expression
;       [prog.baz:155:18] allocate scratch register -> r14
;       [prog.baz:155:21] r14 = 48
;       [prog.baz:155:21] src: folded constant + '0'
        mov r14, 48
;       [prog.baz:155:27] r14 - n % 10
;       [prog.baz:155:27] src: expression
;       [prog.baz:155:27] allocate scratch register -> r13
;       [prog.baz:155:27] n
        mov r13, qword [rbx + 32]
;       [prog.baz:155:31] r13 % 10
;       [prog.baz:155:31] src: constant
;       [prog.baz:155:31] allocate named register rax
        mov rax, r13
;       [prog.baz:155:31] allocate named register rdx
;       [prog.baz:155:31] allocate scratch register -> r12
        mov r12, 10
        cqo
        idiv r12
;       [prog.baz:155:31] free scratch register r12
        mov r13, rdx
;       [prog.baz:155:31] free named register rdx
;       [prog.baz:155:31] free named register rax
        sub r14, r13
;       [prog.baz:155:27] free scratch register r13
        mov byte [rbx + r15 + 8], r14b
;       [prog.baz:155:18] free scratch register r14
;       [prog.baz:155:9] free scratch register r15
;       [prog.baz:156:9] n = n / 10
;       [prog.baz:156:13] kept the first of two versions of 5 and 7 instructions, the smaller
;       [prog.baz:156:13] n
;       [prog.baz:156:17] n / 10
;       [prog.baz:156:17] src: constant
;       [prog.baz:156:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [prog.baz:156:17] allocate named register rdx
;       [prog.baz:156:17] allocate scratch register -> r15
        mov r15, 10
        cqo
        idiv r15
;       [prog.baz:156:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [prog.baz:156:17] free named register rdx
;       [prog.baz:156:17] free named register rax
        if.157.9:
;       [prog.baz:157:12] ? n == 0
        cmp.157.12:
        cmp qword [rbx + 32], 0
        jne loop.153.5
        if.157.9.code:
;           [prog.baz:157:19] break
        if.157.9.end:
    loop.153.5.end:
    if.160.5:
;   [prog.baz:160:8] ? shorthand: is_negative
    cmp.160.8:
    cmp byte [rbx + 40], 0
    je if.160.5.end
    if.160.5.code:
;       [prog.baz:161:9] i = i - 1
;       [prog.baz:161:13] kept the first of two versions of 2 and 4 instructions, the smaller
;       [prog.baz:161:13] i
;       [prog.baz:161:13] i - 1
;       [prog.baz:161:13] src: folded constant '- 1'
        dec qword [rbx + 48]
        jo baz_overflow_line_161
;       [prog.baz:162:9] buf[i] = '-'
;       [prog.baz:162:13] allocate scratch register -> r15
;       [prog.baz:162:13] set array index
;       [prog.baz:162:13] i
        mov r15, qword [rbx + 48]
;       [prog.baz:162:13] bounds check begin
;       [prog.baz:162:13] lower bound
;       [prog.baz:162:13] r15 lower bound covered by the unsigned upper bound
;       [prog.baz:162:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_162
;       [prog.baz:162:13] bounds check end
;       [prog.baz:162:18] buf[i] = 45
;       [prog.baz:162:18] src: folded constant '-'
        mov byte [rbx + r15 + 8], 45
;       [prog.baz:162:9] free scratch register r15
    if.160.5.end:
;   [prog.baz:165:5] var write_pos = 0
;   [prog.baz:165:9] write_pos: i64 (8 B @ [rbx + 56])
;   [prog.baz:165:9] write_pos = 0
;   [prog.baz:165:21] write_pos = 0
;   [prog.baz:165:21] src: folded constant '0'
    mov qword [rbx + 56], 0
;   [prog.baz:166:5] loop
    loop.166.5:
;       [prog.baz:167:9] buf[write_pos] = buf[i]
;       [prog.baz:167:13] allocate scratch register -> r15
;       [prog.baz:167:13] set array index
;       [prog.baz:167:13] write_pos
        mov r15, qword [rbx + 56]
;       [prog.baz:167:13] bounds check begin
;       [prog.baz:167:13] lower bound
;       [prog.baz:167:13] r15 lower bound covered by the unsigned upper bound
;       [prog.baz:167:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_167
;       [prog.baz:167:13] bounds check end
;       [prog.baz:167:26] buf[i]
;       [prog.baz:167:30] allocate scratch register -> r14
;       [prog.baz:167:30] set array index
;       [prog.baz:167:30] i
        mov r14, qword [rbx + 48]
;       [prog.baz:167:30] bounds check begin
;       [prog.baz:167:30] lower bound
;       [prog.baz:167:30] r14 lower bound covered by the unsigned upper bound
;       [prog.baz:167:30] upper bound
        cmp r14, 20
        jae baz_bounds_line_167
;       [prog.baz:167:30] bounds check end
;       [prog.baz:167:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [prog.baz:167:26] free scratch register r13
;       [prog.baz:167:26] free scratch register r14
;       [prog.baz:167:9] free scratch register r15
;       [prog.baz:168:9] write_pos = write_pos + 1
;       [prog.baz:168:21] kept the first of two versions of 2 and 4 instructions, the smaller
;       [prog.baz:168:21] write_pos
;       [prog.baz:168:21] write_pos + 1
;       [prog.baz:168:21] src: folded constant '+ 1'
        inc qword [rbx + 56]
        jo baz_overflow_line_168
;       [prog.baz:169:9] i = i + 1
;       [prog.baz:169:13] kept the first of two versions of 2 and 4 instructions, the smaller
;       [prog.baz:169:13] i
;       [prog.baz:169:13] i + 1
;       [prog.baz:169:13] src: folded constant '+ 1'
        inc qword [rbx + 48]
        jo baz_overflow_line_169
        if.170.9:
;       [prog.baz:170:12] ? i == buf_count
        cmp.170.12:
        cmp qword [rbx + 48], 20
        jne loop.166.5
        if.170.9.code:
;           [prog.baz:170:27] break
        if.170.9.end:
    loop.166.5.end:
;   [prog.baz:173:5] write(1, buf, write_pos)
;   [prog.baz:173:5] allocate named register rdi
;   [prog.baz:173:5] allocate named register rsi
;   [prog.baz:173:5] allocate named register rdx
;   [prog.baz:173:11] rdi = 1
;   [prog.baz:173:11] src: folded constant '1'
    mov rdi, 1
    lea rsi, [rbx + 8]
;   [prog.baz:173:19] write_pos
    mov rdx, qword [rbx + 56]
;   [prog.baz:173:19] bounds check begin
;   [prog.baz:173:19] lower bound
;   [prog.baz:173:19] rdx lower bound covered by the unsigned upper bound
;   [prog.baz:173:19] upper bound
    cmp rdx, 20
    ja baz_bounds_line_173
;   [prog.baz:173:19] bounds check end
;   [prog.baz:173:5] allocate named register rax
    mov rax, 1
    syscall
;   [prog.baz:173:5] free named register rax
;   [prog.baz:173:5] free named register rdx
;   [prog.baz:173:5] free named register rsi
;   [prog.baz:173:5] free named register rdi
    ret
; free named register rbx
size.func.print_num equ 64
; frame overflow handler (--checks=frame)
baz_frame_overflow:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_frame_overflow]
    mov rdx, 23
    syscall
    mov rdi, 254
    jmp baz_exit
section .rodata
msg_frame_overflow:
db `\npanic: frame overflow\n`
section .text
; overlap failure handler (--checks=overlap)
baz_overlap_line_250:
    mov rbp, 250
    jmp baz_overlap_panic_file_0
baz_overlap_line_384:
    mov rbp, 384
baz_overlap_panic_file_0:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overlap_file_0]
    mov rdx, 28
    syscall
    mov rbx, 251
    jmp baz_report_line
section .rodata
msg_overlap_file_0:
db `\npanic: overlap at prog.baz:`
section .text
; overflow failure handler (--checks=overflow)
baz_overflow_line_55:
    mov rbp, 55
    jmp baz_overflow_panic_file_0
baz_overflow_line_67:
    mov rbp, 67
    jmp baz_overflow_panic_file_0
baz_overflow_line_104:
    mov rbp, 104
    jmp baz_overflow_panic_file_0
baz_overflow_line_149:
    mov rbp, 149
    jmp baz_overflow_panic_file_0
baz_overflow_line_154:
    mov rbp, 154
    jmp baz_overflow_panic_file_0
baz_overflow_line_161:
    mov rbp, 161
    jmp baz_overflow_panic_file_0
baz_overflow_line_168:
    mov rbp, 168
    jmp baz_overflow_panic_file_0
baz_overflow_line_169:
    mov rbp, 169
    jmp baz_overflow_panic_file_0
baz_overflow_line_180:
    mov rbp, 180
    jmp baz_overflow_panic_file_0
baz_overflow_line_182:
    mov rbp, 182
    jmp baz_overflow_panic_file_0
baz_overflow_line_212:
    mov rbp, 212
    jmp baz_overflow_panic_file_0
baz_overflow_line_215:
    mov rbp, 215
    jmp baz_overflow_panic_file_0
baz_overflow_line_216:
    mov rbp, 216
    jmp baz_overflow_panic_file_0
baz_overflow_line_246:
    mov rbp, 246
    jmp baz_overflow_panic_file_0
baz_overflow_line_272:
    mov rbp, 272
    jmp baz_overflow_panic_file_0
baz_overflow_line_281:
    mov rbp, 281
    jmp baz_overflow_panic_file_0
baz_overflow_line_344:
    mov rbp, 344
    jmp baz_overflow_panic_file_0
baz_overflow_line_349:
    mov rbp, 349
    jmp baz_overflow_panic_file_0
baz_overflow_line_407:
    mov rbp, 407
baz_overflow_panic_file_0:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overflow_file_0]
    mov rdx, 29
    syscall
    mov rbx, 249
    jmp baz_report_line
section .rodata
msg_overflow_file_0:
db `\npanic: overflow at prog.baz:`
section .text
; bounds failure handler (--checks=upper or --checks=lower)
baz_bounds_line_96:
    mov rbp, 96
    jmp baz_bounds_panic_file_0
baz_bounds_line_155:
    mov rbp, 155
    jmp baz_bounds_panic_file_0
baz_bounds_line_162:
    mov rbp, 162
    jmp baz_bounds_panic_file_0
baz_bounds_line_167:
    mov rbp, 167
    jmp baz_bounds_panic_file_0
baz_bounds_line_173:
    mov rbp, 173
    jmp baz_bounds_panic_file_0
baz_bounds_line_245:
    mov rbp, 245
    jmp baz_bounds_panic_file_0
baz_bounds_line_246:
    mov rbp, 246
    jmp baz_bounds_panic_file_0
baz_bounds_line_272:
    mov rbp, 272
    jmp baz_bounds_panic_file_0
baz_bounds_line_273:
    mov rbp, 273
    jmp baz_bounds_panic_file_0
baz_bounds_line_274:
    mov rbp, 274
baz_bounds_panic_file_0:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_bounds_file_0]
    mov rdx, 27
    syscall
    mov rbx, 255
section .rodata
msg_bounds_file_0:
db `\npanic: bounds at prog.baz:`
section .text
baz_report_line:
;    line number is in 'rbp'
    mov rax, rbp
;    convert to string
    mov rdi, strict qword num_buffer + 20
    mov byte [rdi], 10
    dec rdi
    mov rcx, 10
.convert_loop:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jne .convert_loop
    inc rdi
;    print line number to stderr
    mov rax, 1
    mov rsi, rdi
    mov rdx, strict qword num_buffer + 21
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rdi, rbx
section .bss
num_buffer:
resb 21
section .text
baz_exit:
    mov rax, 60
    syscall

section .data
align 16
dat:
;[prog.baz:22:7] hello
;[prog.baz:22:15] i8[21]
db `hello world from baz\n`
;[prog.baz:23:5] prompt1
;[prog.baz:23:15] i8[12]
db `enter name:\n`
;[prog.baz:24:5] prompt2
;[prog.baz:24:15] i8[20]
db `that is not a name.\n`
;[prog.baz:25:5] prompt3
;[prog.baz:25:15] i8[6]
db `hello `
;[prog.baz:26:9] dot
;[prog.baz:26:15] i8[1]
db `.`
;[prog.baz:27:10] nl
;[prog.baz:27:15] i8[1]
db `\n`
;[prog.baz:28:7] colon
;[prog.baz:28:15] i8[2]
db `: `
; padding 1 B
times 1 db 0
;[prog.baz:29:8] nums
;[prog.baz:29:15] i64[4]
dq 1
;[prog.baz:29:15] zero remaining elements: 3 * 8 B = 24 B
times 24 db 0
;[prog.baz:30:8] str1
;[prog.baz:30:20] i8
db 3
;[prog.baz:30:15] zero remaining fields: 127 B
times 127 db 0
;[prog.baz:31:8] str2
;[prog.baz:31:20] i8
db 3
;[prog.baz:31:23] i8[127]
db `baz`
;[prog.baz:31:23] zero remaining elements: 124 * 1 B = 124 B
times 124 db 0
;[prog.baz:33:5] greeted
;[prog.baz:33:15] i8[15]
db `names greeted: `
; padding 1 B
times 1 db 0
;[prog.baz:34:7] names
;[prog.baz:34:15] i64
dq 0
dat.end:

section .bss.vars nobits alloc write
align 16
vars:
resb 65536
vars.end:
; free named register rbp

;           noinline functions:
;                    factorial: 1 body, 2 calls, 29 instructions
;                    print_num: 1 body, 2 calls, 67 instructions
;
;   removed jumps to next code: 75
;    removed unreachable jumps: 2
; removed same target branches: 0
; inverted branches over jumps: 7
; max scratch registers in use: 4
;            max scopes in use: 10
;                     dat size: 376 B
;              dat var padding: 8 B
;           max body vars size: 1045 B
;                 instructions: 900
;
; register use at the peak: 6 of 13 registers live, 2 named by instructions
;
;   held  frame, registers (allocated at)
;      6  print_num (noinline body)
;           r15 155:13
;           r14 155:18
;           r13 155:27
;           rax 155:31 (named)
;           rdx 155:31 (named)
;           r12 155:31
;
; the peak is in a function with a body of its own, compiled with all 13
; registers free, its callers are not on this stack
;
; per callee, the most registers one call holds itself
;
;   own  calls  callee
;     6      1  print_num (noinline body)
;     4      1  main
;     4      9  print
;     4      1  str.input
;     4      1  str.print
;     3      1  factorial (noinline body)
;     1     60  assert
;     1      3  baz
;     1      1  inv
;     0      2  bar
;     0      1  faz
;     0      1  greet
;     0      1  object.at
;     0      2  point.at
;     0      2  point.fooz
;     0      2  point.sum
;     0      1  point.x
;
; calls of functions with a body of their own save the registers held at the
; call
;
;   saved  calls  callee, most saved at
;       1      2  factorial 181:19
;       0      2  print_num 408:9
```
