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
           overflow check, rv32i-fpga ignores it
  frame    runtime non-inlined function frame capacity
  alias    compile time rejection of calls where a result may share storage
           with an argument, on by default
  division runtime division by zero and 'MIN / -1'
  shift    runtime shift count below zero or not below the width of the type
  overlap  runtime 'array_copy' whose destination starts inside the source
  stack    runtime stack capacity at non-inlined calls on rv32i-qemu and
           rv32i-fpga, the other targets have an operating system that stops
           a program that overflows the stack
  overflow runtime signed overflow of '+', '-', '*' and unary '-', a constant
           expression that overflows is rejected at compile time
  noub     all checks against undefined behavior: upper, lower, frame, alias,
           division, shift, overlap, stack and overflow
  -NAME    turns a check off after the others are applied, e.g.
           noub,-division or -alias (also when noub is given), +NAME is NAME

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
C/C++ Header                    65           8981           4862          26597
C++                              1            235             69            776
-------------------------------------------------------------------------------
SUM:                            66           9216           4931          27373
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
# constants can be declared in any scope and shadow outer declarations

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
    # `i8(100)` gives the variable type `i8`, arithmetic wraps at its width

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
#   rejected because "return" and the argument may share storage,
#   `--checks=-alias` allows it

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
    cmp.189.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.189.12.end:
    func.assert.189.5:
        if.38.27.189.5:
        cmp.38.27.189.5:
        cmp r15b, 0
        jne if.38.24.189.5.end
        if.38.27.189.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.189.5.end:
    func.assert.189.5.end:
    mov qword [rbp + 384], -1
    cmp.192.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.192.12.end:
    func.assert.192.5:
        if.38.27.192.5:
        cmp.38.27.192.5:
        cmp r15b, 0
        jne if.38.24.192.5.end
        if.38.27.192.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.192.5.end:
    func.assert.192.5.end:
        func.assert.198.9:
            if.38.27.198.9:
            cmp.38.27.198.9:
            if.38.24.198.9.end:
        func.assert.198.9.end:
    func.assert.201.5:
        if.38.27.201.5:
        cmp.38.27.201.5:
        if.38.24.201.5.end:
    func.assert.201.5.end:
    cmp.203.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.203.12.end
    cmp.203.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.203.12.end:
    func.assert.203.5:
        if.38.27.203.5:
        cmp.38.27.203.5:
        cmp r15b, 0
        jne if.38.24.203.5.end
        if.38.27.203.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.203.5.end:
    func.assert.203.5.end:
    cmp.204.12:
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.204.12.end
    cmp.204.30:
    cmp byte [rbp + 227], 122
    sete r15b
    bool.204.12.end:
    func.assert.204.5:
        if.38.27.204.5:
        cmp.38.27.204.5:
        cmp r15b, 0
        jne if.38.24.204.5.end
        if.38.27.204.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.204.5.end:
    func.assert.204.5.end:
    mov qword [rbp + 392], 7
    cmp.207.12:
        mov r14, qword [rbp + 392]
        and r14, 3
    cmp r14, 3
    sete r15b
    bool.207.12.end:
    func.assert.207.5:
        if.38.27.207.5:
        cmp.38.27.207.5:
        cmp r15b, 0
        jne if.38.24.207.5.end
        if.38.27.207.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.207.5.end:
    func.assert.207.5.end:
    cmp.208.12:
        mov r14, qword [rbp + 392]
        or r14, 8
    cmp r14, 15
    sete r15b
    bool.208.12.end:
    func.assert.208.5:
        if.38.27.208.5:
        cmp.38.27.208.5:
        cmp r15b, 0
        jne if.38.24.208.5.end
        if.38.27.208.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.208.5.end:
    func.assert.208.5.end:
    cmp.209.12:
        mov r14, qword [rbp + 392]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.209.12.end:
    func.assert.209.5:
        if.38.27.209.5:
        cmp.38.27.209.5:
        cmp r15b, 0
        jne if.38.24.209.5.end
        if.38.27.209.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.209.5.end:
    func.assert.209.5.end:
    cmp.210.12:
        mov r14, qword [rbp + 392]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.210.12.end:
    func.assert.210.5:
        if.38.27.210.5:
        cmp.38.27.210.5:
        cmp r15b, 0
        jne if.38.24.210.5.end
        if.38.27.210.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.210.5.end:
    func.assert.210.5.end:
    cmp.211.12:
        mov r14, qword [rbp + 392]
        neg r14
        jo baz_overflow_line_211
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.211.12.end:
    func.assert.211.5:
        if.38.27.211.5:
        cmp.38.27.211.5:
        cmp r15b, 0
        jne if.38.24.211.5.end
        if.38.27.211.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.211.5.end:
    func.assert.211.5.end:
    cmp.214.12:
        mov r14, qword [rbp + 392]
        mov r13, qword [rbp + 392]
        sal r13, 1
        add r14, r13
        jo baz_overflow_line_214
    cmp r14, 21
    sete r15b
    bool.214.12.end:
    func.assert.214.5:
        if.38.27.214.5:
        cmp.38.27.214.5:
        cmp r15b, 0
        jne if.38.24.214.5.end
        if.38.27.214.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.214.5.end:
    func.assert.214.5.end:
    cmp.215.12:
        mov r14, qword [rbp + 392]
        add r14, qword [rbp + 392]
        jo baz_overflow_line_215
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.215.12.end:
    func.assert.215.5:
        if.38.27.215.5:
        cmp.38.27.215.5:
        cmp r15b, 0
        jne if.38.24.215.5.end
        if.38.27.215.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.215.5.end:
    func.assert.215.5.end:
    cmp.220.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.220.12.end
    cmp.220.23:
    cmp.220.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.220.12.end
    cmp.220.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.220.12.end:
    func.assert.220.5:
        if.38.27.220.5:
        cmp.38.27.220.5:
        cmp r15b, 0
        jne if.38.24.220.5.end
        if.38.27.220.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.220.5.end:
    func.assert.220.5.end:
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.225.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.225.12.end:
    func.assert.225.5:
        if.38.27.225.5:
        cmp.38.27.225.5:
        cmp r15b, 0
        jne if.38.24.225.5.end
        if.38.27.225.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.225.5.end:
    func.assert.225.5.end:
    movsx r15w, byte [rbp + 400]
    movsx r15, r15w
    mov qword [rbp + 408], r15
    cmp.229.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.229.12.end:
    func.assert.229.5:
        if.38.27.229.5:
        cmp.38.27.229.5:
        cmp r15b, 0
        jne if.38.24.229.5.end
        if.38.27.229.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.229.5.end:
    func.assert.229.5.end:
    mov qword [rbp + 416], 65
    cmp.237.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.237.12.end:
    func.assert.237.5:
        if.38.27.237.5:
        cmp.38.27.237.5:
        cmp r15b, 0
        jne if.38.24.237.5.end
        if.38.27.237.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.237.5.end:
    func.assert.237.5.end:
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
    mov qword [rbp + 440], 1
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_243
    mov dword [rbp + r15 * 4 + 424], 2
    mov r15, qword [rbp + 440]
    add r15, 1
    jo baz_overflow_line_244
    cmp r15, 4
    jae baz_bounds_line_244
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_244
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
    cmp.245.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.245.12.end:
    func.assert.245.5:
        if.38.27.245.5:
        cmp.38.27.245.5:
        cmp r15b, 0
        jne if.38.24.245.5.end
        if.38.27.245.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.245.5.end:
    func.assert.245.5.end:
    cmp.246.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.246.12.end:
    func.assert.246.5:
        if.38.27.246.5:
        cmp.38.27.246.5:
        cmp r15b, 0
        jne if.38.24.246.5.end
        if.38.27.246.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.246.5.end:
    func.assert.246.5.end:
    mov rcx, 2
    mov r15, 2
    test r15, r15
    js baz_bounds_line_248
    test rcx, rcx
    js baz_bounds_line_248
    lea r14, [rcx + r15]
    cmp r14, 4
    ja baz_bounds_line_248
    lea rsi, [rbp + r15 * 4 + 424]
    cmp rcx, 4
    ja baz_bounds_line_248
    lea rdi, [rbp + 424]
    sal rcx, 2
    mov r15, rdi
    sub r15, rsi
    je .Lbaz_overlap.0
    cmp r15, rcx
    jb baz_overlap_line_248
    .Lbaz_overlap.0:
    rep movsb
    cmp.249.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.249.12.end:
    func.assert.249.5:
        if.38.27.249.5:
        cmp.38.27.249.5:
        cmp r15b, 0
        jne if.38.24.249.5.end
        if.38.27.249.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.249.5.end:
    func.assert.249.5.end:
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
    mov rcx, 4
    cmp rcx, 4
    ja baz_bounds_line_253
    lea rsi, [rbp + 424]
    cmp rcx, 8
    ja baz_bounds_line_253
    lea rdi, [rbp + 448]
    sal rcx, 2
    mov r15, rdi
    sub r15, rsi
    je .Lbaz_overlap.1
    cmp r15, rcx
    jb baz_overlap_line_253
    .Lbaz_overlap.1:
    rep movsb
    cmp.254.14:
        mov rcx, 3
        mov r15, 1
        test r15, r15
        js baz_bounds_line_254
        test rcx, rcx
        js baz_bounds_line_254
        lea r14, [rcx + r15]
        cmp r14, 4
        ja baz_bounds_line_254
        lea rsi, [rbp + r15 * 4 + 424]
        mov r15, 1
        test r15, r15
        js baz_bounds_line_254
        lea r14, [rcx + r15]
        cmp r14, 8
        ja baz_bounds_line_254
        lea rdi, [rbp + r15 * 4 + 448]
        sal rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 480]
    bool.254.14.end:
    func.assert.257.5:
        if.38.27.257.5:
        cmp.38.27.257.5:
        cmp byte [rbp + 480], 0
        jne if.38.24.257.5.end
        if.38.27.257.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.257.5.end:
    func.assert.257.5.end:
    mov dword [rbp + 456], -1
    cmp.260.12:
        mov rcx, 4
        cmp rcx, 4
        ja baz_bounds_line_260
        lea rsi, [rbp + 424]
        cmp rcx, 8
        ja baz_bounds_line_260
        lea rdi, [rbp + 448]
        sal rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.260.12.end:
    func.assert.260.5:
        if.38.27.260.5:
        cmp.38.27.260.5:
        cmp r15b, 0
        jne if.38.24.260.5.end
        if.38.27.260.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.260.5.end:
    func.assert.260.5.end:
    mov r15, qword [rbp + 424]
    mov qword [rbp + 484], r15
    mov r15, qword [rbp + 432]
    mov qword [rbp + 492], r15
    cmp.263.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.263.12.end:
    func.assert.263.5:
        if.38.27.263.5:
        cmp.38.27.263.5:
        cmp r15b, 0
        jne if.38.24.263.5.end
        if.38.27.263.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.263.5.end:
    func.assert.263.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    sub r15, 1
    jo baz_overflow_line_271
    cmp r15, 4
    jae baz_bounds_line_271
    func.inv.271.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv.271.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_272
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.273.12:
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_273
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
    bool.273.12.end:
    func.assert.273.5:
        if.38.27.273.5:
        cmp.38.27.273.5:
        cmp r15b, 0
        jne if.38.24.273.5.end
        if.38.27.273.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.273.5.end:
    func.assert.273.5.end:
    func.faz.275.5:
        mov dword [rbp + 428], 254
    func.faz.275.5.end:
    cmp.276.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.276.12.end:
    func.assert.276.5:
        if.38.27.276.5:
        cmp.38.27.276.5:
        cmp r15b, 0
        jne if.38.24.276.5.end
        if.38.27.276.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.276.5.end:
    func.assert.276.5.end:
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov r14, 0
    foo.279.5:
        add qword [r15], r14
        jo baz_overflow_line_280
        add qword [r15], 2
        jo baz_overflow_line_280
        foo.279.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.279.5
    foo.279.5.end:
    cmp.282.12:
    cmp qword [rbp + 504], 5
    sete r15b
    bool.282.12.end:
    func.assert.282.5:
        if.38.27.282.5:
        cmp.38.27.282.5:
        cmp r15b, 0
        jne if.38.24.282.5.end
        if.38.27.282.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.282.5.end:
    func.assert.282.5.end:
    cmp.283.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.283.12.end:
    func.assert.283.5:
        if.38.27.283.5:
        cmp.38.27.283.5:
        cmp r15b, 0
        jne if.38.24.283.5.end
        if.38.27.283.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.283.5.end:
    func.assert.283.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz.291.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz.291.7.end:
    cmp.294.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.294.12.end:
    func.assert.294.5:
        if.38.27.294.5:
        cmp.38.27.294.5:
        cmp r15b, 0
        jne if.38.24.294.5.end
        if.38.27.294.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.294.5.end:
    func.assert.294.5.end:
    cmp.295.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.295.12.end:
    func.assert.295.5:
        if.38.27.295.5:
        cmp.38.27.295.5:
        cmp r15b, 0
        jne if.38.24.295.5.end
        if.38.27.295.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.295.5.end:
    func.assert.295.5.end:
    mov r15, qword [rbp + 520]
    mov qword [rbp + 536], r15
    mov r15, qword [rbp + 528]
    mov qword [rbp + 544], r15
    cmp.300.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    bool.300.12.end:
    func.assert.300.5:
        if.38.27.300.5:
        cmp.38.27.300.5:
        cmp r15b, 0
        jne if.38.24.300.5.end
        if.38.27.300.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.300.5.end:
    func.assert.300.5.end:
    mov qword [rbp + 536], 3
    cmp.305.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    bool.305.12.end:
    func.assert.305.5:
        if.38.27.305.5:
        cmp.38.27.305.5:
        cmp r15b, 0
        jne if.38.24.305.5.end
        if.38.27.305.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.305.5.end:
    func.assert.305.5.end:
    mov qword [rbp + 552], 0
    func.bar.308.5:
        if.60.8.308.5:
        cmp.60.8.308.5:
        cmp qword [rbp + 552], 0
        je func.bar.308.5.end
        if.60.8.308.5.code:
        if.60.5.308.5.end:
        mov qword [rbp + 552], 255
    func.bar.308.5.end:
    cmp.309.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.309.12.end:
    func.assert.309.5:
        if.38.27.309.5:
        cmp.38.27.309.5:
        cmp r15b, 0
        jne if.38.24.309.5.end
        if.38.27.309.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.309.5.end:
    func.assert.309.5.end:
    mov qword [rbp + 552], 1
    func.bar.312.5:
        if.60.8.312.5:
        cmp.60.8.312.5:
        cmp qword [rbp + 552], 0
        je func.bar.312.5.end
        if.60.8.312.5.code:
        if.60.5.312.5.end:
        mov qword [rbp + 552], 255
    func.bar.312.5.end:
    cmp.313.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.313.12.end:
    func.assert.313.5:
        if.38.27.313.5:
        cmp.38.27.313.5:
        cmp r15b, 0
        jne if.38.24.313.5.end
        if.38.27.313.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.313.5.end:
    func.assert.313.5.end:
    mov qword [rbp + 560], 1
    func.baz.316.13:
        mov r15, qword [rbp + 560]
        imul r15, 2
        jo baz_overflow_line_67
        mov qword [rbp + 568], r15
    func.baz.316.13.end:
    cmp.317.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.317.12.end:
    func.assert.317.5:
        if.38.27.317.5:
        cmp.38.27.317.5:
        cmp r15b, 0
        jne if.38.24.317.5.end
        if.38.27.317.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.317.5.end:
    func.assert.317.5.end:
    func.baz.319.9:
        mov qword [rbp + 568], 2
    func.baz.319.9.end:
    cmp.320.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.320.12.end:
    func.assert.320.5:
        if.38.27.320.5:
        cmp.38.27.320.5:
        cmp r15b, 0
        jne if.38.24.320.5.end
        if.38.27.320.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.320.5.end:
    func.assert.320.5.end:
    mov qword [rbp + 576], 5
    lea r15, [rbp + 592]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.factorial
    cmp r15, r14
    ja baz_frame_overflow
    lea r15, [rbp + 584]
    mov qword [rbp + 592], r15
    lea r15, [rbp + 576]
    mov qword [rbp + 600], r15
    lea rbx, [rbp + 592]
    call func.factorial
    cmp.324.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.324.12.end:
    func.assert.324.5:
        if.38.27.324.5:
        cmp.38.27.324.5:
        cmp r15b, 0
        jne if.38.24.324.5.end
        if.38.27.324.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.324.5.end:
    func.assert.324.5.end:
    func.baz.326.20:
        mov qword [rbp + 592], 6
    func.baz.326.20.end:
    mov qword [rbp + 600], 0
    cmp.327.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.327.12.end:
    func.assert.327.5:
        if.38.27.327.5:
        cmp.38.27.327.5:
        cmp r15b, 0
        jne if.38.24.327.5.end
        if.38.27.327.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.327.5.end:
    func.assert.327.5.end:
    func.point.at.329.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at.329.14.end:
    cmp.333.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.333.12.end:
    func.assert.333.5:
        if.38.27.333.5:
        cmp.38.27.333.5:
        cmp r15b, 0
        jne if.38.24.333.5.end
        if.38.27.333.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.333.5.end:
    func.assert.333.5.end:
    cmp.334.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.334.12.end:
    func.assert.334.5:
        if.38.27.334.5:
        cmp.38.27.334.5:
        cmp r15b, 0
        jne if.38.24.334.5.end
        if.38.27.334.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.334.5.end:
    func.assert.334.5.end:
    func.point.x.336.8:
        mov qword [rbp + 608], 2
    func.point.x.336.8.end:
    cmp.337.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.337.12.end:
    func.assert.337.5:
        if.38.27.337.5:
        cmp.38.27.337.5:
        cmp r15b, 0
        jne if.38.24.337.5.end
        if.38.27.337.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.337.5.end:
    func.assert.337.5.end:
    cmp.338.12:
        func.point.sum.338.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
            jo baz_overflow_line_55
        func.point.sum.338.15.end:
    cmp r14, 0
    sete r15b
    bool.338.12.end:
    func.assert.338.5:
        if.38.27.338.5:
        cmp.38.27.338.5:
        cmp r15b, 0
        jne if.38.24.338.5.end
        if.38.27.338.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.338.5.end:
    func.assert.338.5.end:
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    jo baz_overflow_line_343
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.344.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.344.12.end:
    func.assert.344.5:
        if.38.27.344.5:
        cmp.38.27.344.5:
        cmp r15b, 0
        jne if.38.24.344.5.end
        if.38.27.344.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.344.5.end:
    func.assert.344.5.end:
    cmp.345.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.345.12.end:
    func.assert.345.5:
        if.38.27.345.5:
        cmp.38.27.345.5:
        cmp r15b, 0
        jne if.38.24.345.5.end
        if.38.27.345.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.345.5.end:
    func.assert.345.5.end:
    cmp.346.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.346.12.end:
    func.assert.346.5:
        if.38.27.346.5:
        cmp.38.27.346.5:
        cmp r15b, 0
        jne if.38.24.346.5.end
        if.38.27.346.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.346.5.end:
    func.assert.346.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
    neg qword [rbp + 664]
    jo baz_overflow_line_348
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
    neg qword [rbp + 672]
    jo baz_overflow_line_348
    mov r15, qword [rbp + 664]
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 672]
    mov qword [rbp + 648], r15
    cmp.350.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.350.12.end:
    func.assert.350.5:
        if.38.27.350.5:
        cmp.38.27.350.5:
        cmp r15b, 0
        jne if.38.24.350.5.end
        if.38.27.350.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.350.5.end:
    func.assert.350.5.end:
    cmp.351.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.351.12.end:
    func.assert.351.5:
        if.38.27.351.5:
        cmp.38.27.351.5:
        cmp r15b, 0
        jne if.38.24.351.5.end
        if.38.27.351.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.351.5.end:
    func.assert.351.5.end:
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.354.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.354.12.end:
    func.assert.354.5:
        if.38.27.354.5:
        cmp.38.27.354.5:
        cmp r15b, 0
        jne if.38.24.354.5.end
        if.38.27.354.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.354.5.end:
    func.assert.354.5.end:
    cmp.355.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.355.12.end:
    func.assert.355.5:
        if.38.27.355.5:
        cmp.38.27.355.5:
        cmp r15b, 0
        jne if.38.24.355.5.end
        if.38.27.355.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.355.5.end:
    func.assert.355.5.end:
    cmp.356.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.356.12.end:
    func.assert.356.5:
        if.38.27.356.5:
        cmp.38.27.356.5:
        cmp r15b, 0
        jne if.38.24.356.5.end
        if.38.27.356.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.356.5.end:
    func.assert.356.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.359.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.359.12.end:
    func.assert.359.5:
        if.38.27.359.5:
        cmp.38.27.359.5:
        cmp r15b, 0
        jne if.38.24.359.5.end
        if.38.27.359.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.359.5.end:
    func.assert.359.5.end:
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.361.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.361.12.end:
    func.assert.361.5:
        if.38.27.361.5:
        cmp.38.27.361.5:
        cmp r15b, 0
        jne if.38.24.361.5.end
        if.38.27.361.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.361.5.end:
    func.assert.361.5.end:
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.371.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.371.12.end:
    func.assert.371.5:
        if.38.27.371.5:
        cmp.38.27.371.5:
        cmp r15b, 0
        jne if.38.24.371.5.end
        if.38.27.371.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.371.5.end:
    func.assert.371.5.end:
    mov dword [rbp + 748], 0
    func.object.at.372.13:
        func.point.at.120.16.372.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at.120.16.372.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at.372.13.end:
    cmp.373.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.373.12.end:
    func.assert.373.5:
        if.38.27.373.5:
        cmp.38.27.373.5:
        cmp r15b, 0
        jne if.38.24.373.5.end
        if.38.27.373.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.373.5.end:
    func.assert.373.5.end:
    func.point.fooz.375.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz.375.15.end:
    cmp.376.12:
        func.point.sum.376.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
            jo baz_overflow_line_55
        func.point.sum.376.22.end:
    cmp r14, 13
    sete r15b
    bool.376.12.end:
    func.assert.376.5:
        if.38.27.376.5:
        cmp.38.27.376.5:
        cmp r15b, 0
        jne if.38.24.376.5.end
        if.38.27.376.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.376.5.end:
    func.assert.376.5.end:
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.381.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.381.12.end:
    func.assert.381.5:
        if.38.27.381.5:
        cmp.38.27.381.5:
        cmp r15b, 0
        jne if.38.24.381.5.end
        if.38.27.381.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.381.5.end:
    func.assert.381.5.end:
    mov rcx, 8
    cmp rcx, 8
    ja baz_bounds_line_384
    lea rsi, [rbp + 816]
    cmp rcx, 8
    ja baz_bounds_line_385
    lea rdi, [rbp + 752]
    sal rcx, 3
    mov r15, rdi
    sub r15, rsi
    je .Lbaz_overlap.2
    cmp r15, rcx
    jb baz_overlap_line_383
    .Lbaz_overlap.2:
    rep movsb
    cmp.390.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.390.12.end:
    func.assert.390.5:
        if.38.27.390.5:
        cmp.38.27.390.5:
        cmp r15b, 0
        jne if.38.24.390.5.end
        if.38.27.390.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.390.5.end:
    func.assert.390.5.end:
    cmp.391.12:
        mov rcx, 8
        cmp rcx, 8
        ja baz_bounds_line_392
        lea rsi, [rbp + 752]
        cmp rcx, 8
        ja baz_bounds_line_393
        lea rdi, [rbp + 816]
        sal rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.391.12.end:
    func.assert.391.5:
        if.38.27.391.5:
        cmp.38.27.391.5:
        cmp r15b, 0
        jne if.38.24.391.5.end
        if.38.27.391.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.391.5.end:
    func.assert.391.5.end:
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    func.assert.398.5:
        if.38.27.398.5:
        cmp.38.27.398.5:
        if.38.24.398.5.end:
    func.assert.398.5.end:
    cmp.399.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.399.12.end:
    func.assert.399.5:
        if.38.27.399.5:
        cmp.38.27.399.5:
        cmp r15b, 0
        jne if.38.24.399.5.end
        if.38.27.399.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.399.5.end:
    func.assert.399.5.end:
    cmp.400.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.400.12.end:
    func.assert.400.5:
        if.38.27.400.5:
        cmp.38.27.400.5:
        cmp r15b, 0
        jne if.38.24.400.5.end
        if.38.27.400.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.400.5.end:
    func.assert.400.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print.404.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.404.5.end:
    loop.405.5:
        add qword [rbp + 1280], 1
        jo baz_overflow_line_406
        lea r15, [rbp + 1416]
        lea r14, [vars]
        cmp r15, r14
        jb baz_frame_overflow
        mov r14, strict qword vars.end
        cmp r15, r14
        ja baz_frame_overflow
        sub r14, r15
        mov r15, size.func.print_num
        cmp r15, r14
        ja baz_frame_overflow
        lea r15, [rbp + 1280]
        mov qword [rbp + 1416], r15
        lea rbx, [rbp + 1416]
        call func.print_num
        func.print.408.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.408.9.end:
        func.print.409.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.409.9.end:
        func.str.input.410.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15, qword [rbp + 1416]
            sub r15, 1
            mov byte [rbp + 1288], r15b
        func.str.input.410.12.end:
        if.412.12:
        cmp.412.12:
        cmp byte [rbp + 1288], 0
        jle loop.405.5.end
        if.412.12.code:
        if.414.19:
        cmp.414.19:
        cmp byte [rbp + 1288], 4
        jg if.412.9.else
        if.414.19.code:
            func.print.415.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.415.13.end:
            jmp loop.405.5
        if.412.9.else:
            func.greet.418.13:
                func.print.100.5.418.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.100.5.418.13.end:
                func.str.print.101.10.418.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1288]
                    cmp rdx, 127
                    ja baz_bounds_line_96
                    lea rsi, [rbp + 1289]
                    mov rax, 1
                    syscall
                func.str.print.101.10.418.13.end:
                func.print.102.5.418.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.102.5.418.13.end:
                func.print.103.5.418.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.103.5.418.13.end:
                add qword [rbp + 368], 1
                jo baz_overflow_line_104
            func.greet.418.13.end:
        if.412.9.end:
    jmp loop.405.5
    loop.405.5.end:
    func.print.422.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
        mov rax, 1
        syscall
    func.print.422.5.end:
    lea r15, [rbp + 1416]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.print_num
    cmp r15, r14
    ja baz_frame_overflow
    lea r15, [rbp + 368]
    mov qword [rbp + 1416], r15
    lea rbx, [rbp + 1416]
    call func.print_num
    func.print.424.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.424.5.end:
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
    mov rdi, 1
    mov rdx, 3
    cmp rdx, 13
    ja baz_bounds_line_427
    lea rsi, [rbp + 1416]
    mov rax, 1
    syscall
    mov rdi, 1
    mov rdx, 1
    mov r15, 12
    test r15, r15
    js baz_bounds_line_428
    test rdx, rdx
    js baz_bounds_line_428
    lea r14, [rdx + r15]
    cmp r14, 13
    ja baz_bounds_line_428
    lea rsi, [rbp + 1416]
    add rsi, r15
    mov rax, 1
    syscall
    mov rdi, 0
    mov rax, 60
    syscall
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.177.8:
    cmp.177.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.177.5.end
    if.177.8.code:
        ret
    if.177.5.end:
    mov r15, qword [rbx + 8]
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
    sub qword [rbx + 16], 1
    jo baz_overflow_line_179
    lea r15, [rbx + 32]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.factorial
    cmp r15, r14
    ja baz_frame_overflow
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
    jo baz_overflow_line_181
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
    if.144.8:
    cmp.144.8:
    cmp qword [rbx + 32], 0
    jge if.144.5.end
    if.144.8.code:
        mov byte [rbx + 40], 1
    if.144.5.end:
    if.147.8:
    cmp.147.8:
    cmp qword [rbx + 32], 0
    jle if.147.5.end
    if.147.8.code:
        mov r15, qword [rbx + 32]
        neg r15
        jo baz_overflow_line_148
        mov qword [rbx + 32], r15
    if.147.5.end:
    mov qword [rbx + 48], 20
    loop.152.5:
        sub qword [rbx + 48], 1
        jo baz_overflow_line_153
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_154
        mov r14, 48
        mov r13, qword [rbx + 32]
        mov rax, r13
        mov r12, 10
        cmp r12, 0
        je baz_division_line_154
        cmp r12, -1
        jne .Lbaz_division.6
        mov rdx, -9223372036854775808
        cmp rax, rdx
        je baz_division_line_154
        .Lbaz_division.6:
        cqo
        idiv r12
        mov r13, rdx
        sub r14, r13
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 32]
        mov r15, 10
        cmp r15, 0
        je baz_division_line_155
        cmp r15, -1
        jne .Lbaz_division.7
        mov rdx, -9223372036854775808
        cmp rax, rdx
        je baz_division_line_155
        .Lbaz_division.7:
        cqo
        idiv r15
        mov qword [rbx + 32], rax
        if.156.12:
        cmp.156.12:
        cmp qword [rbx + 32], 0
        jne loop.152.5
        if.156.12.code:
        if.156.9.end:
    loop.152.5.end:
    if.159.8:
    cmp.159.8:
    cmp byte [rbx + 40], 0
    je if.159.5.end
    if.159.8.code:
        sub qword [rbx + 48], 1
        jo baz_overflow_line_160
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_161
        mov byte [rbx + r15 + 8], 45
    if.159.5.end:
    mov qword [rbx + 56], 0
    loop.165.5:
        mov r15, qword [rbx + 56]
        cmp r15, 20
        jae baz_bounds_line_166
        mov r14, qword [rbx + 48]
        cmp r14, 20
        jae baz_bounds_line_166
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 56], 1
        jo baz_overflow_line_167
        add qword [rbx + 48], 1
        jo baz_overflow_line_168
        if.169.12:
        cmp.169.12:
        cmp qword [rbx + 48], 20
        jne loop.165.5
        if.169.12.code:
        if.169.9.end:
    loop.165.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    cmp rdx, 20
    ja baz_bounds_line_172
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
baz_frame_overflow:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_frame_overflow]
    mov rdx, msg_frame_overflow_len
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
msg_frame_overflow:
db `panic: frame overflow\n`
msg_frame_overflow_len equ $ - msg_frame_overflow
section .text
baz_division_line_154:
    mov rbp, 154
    jmp baz_division_panic
baz_division_line_155:
    mov rbp, 155
baz_division_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_division]
    mov rdx, msg_division_len
    syscall
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
    jnz .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdx, strict qword num_buffer + 21
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .bss
num_buffer:
resb 21
section .rodata
msg_division:
db `panic: division at line `
msg_division_len equ $ - msg_division
section .text
baz_overlap_line_248:
    mov rbp, 248
    jmp baz_overlap_panic
baz_overlap_line_253:
    mov rbp, 253
    jmp baz_overlap_panic
baz_overlap_line_383:
    mov rbp, 383
baz_overlap_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overlap]
    mov rdx, msg_overlap_len
    syscall
    jmp baz_report_line
section .rodata
msg_overlap:
db `panic: overlap at line `
msg_overlap_len equ $ - msg_overlap
section .text
baz_overflow_line_55:
    mov rbp, 55
    jmp baz_overflow_panic
baz_overflow_line_67:
    mov rbp, 67
    jmp baz_overflow_panic
baz_overflow_line_104:
    mov rbp, 104
    jmp baz_overflow_panic
baz_overflow_line_148:
    mov rbp, 148
    jmp baz_overflow_panic
baz_overflow_line_153:
    mov rbp, 153
    jmp baz_overflow_panic
baz_overflow_line_160:
    mov rbp, 160
    jmp baz_overflow_panic
baz_overflow_line_167:
    mov rbp, 167
    jmp baz_overflow_panic
baz_overflow_line_168:
    mov rbp, 168
    jmp baz_overflow_panic
baz_overflow_line_179:
    mov rbp, 179
    jmp baz_overflow_panic
baz_overflow_line_181:
    mov rbp, 181
    jmp baz_overflow_panic
baz_overflow_line_211:
    mov rbp, 211
    jmp baz_overflow_panic
baz_overflow_line_214:
    mov rbp, 214
    jmp baz_overflow_panic
baz_overflow_line_215:
    mov rbp, 215
    jmp baz_overflow_panic
baz_overflow_line_244:
    mov rbp, 244
    jmp baz_overflow_panic
baz_overflow_line_271:
    mov rbp, 271
    jmp baz_overflow_panic
baz_overflow_line_280:
    mov rbp, 280
    jmp baz_overflow_panic
baz_overflow_line_343:
    mov rbp, 343
    jmp baz_overflow_panic
baz_overflow_line_348:
    mov rbp, 348
    jmp baz_overflow_panic
baz_overflow_line_406:
    mov rbp, 406
baz_overflow_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overflow]
    mov rdx, msg_overflow_len
    syscall
    jmp baz_report_line
section .rodata
msg_overflow:
db `panic: overflow at line `
msg_overflow_len equ $ - msg_overflow
section .text
baz_shift_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_shift]
    mov rdx, msg_shift_len
    syscall
    jmp baz_report_line
section .rodata
msg_shift:
db `panic: shift at line `
msg_shift_len equ $ - msg_shift
section .text
baz_bounds_line_96:
    mov rbp, 96
    jmp baz_bounds_panic
baz_bounds_line_154:
    mov rbp, 154
    jmp baz_bounds_panic
baz_bounds_line_161:
    mov rbp, 161
    jmp baz_bounds_panic
baz_bounds_line_166:
    mov rbp, 166
    jmp baz_bounds_panic
baz_bounds_line_172:
    mov rbp, 172
    jmp baz_bounds_panic
baz_bounds_line_243:
    mov rbp, 243
    jmp baz_bounds_panic
baz_bounds_line_244:
    mov rbp, 244
    jmp baz_bounds_panic
baz_bounds_line_248:
    mov rbp, 248
    jmp baz_bounds_panic
baz_bounds_line_253:
    mov rbp, 253
    jmp baz_bounds_panic
baz_bounds_line_254:
    mov rbp, 254
    jmp baz_bounds_panic
baz_bounds_line_260:
    mov rbp, 260
    jmp baz_bounds_panic
baz_bounds_line_271:
    mov rbp, 271
    jmp baz_bounds_panic
baz_bounds_line_272:
    mov rbp, 272
    jmp baz_bounds_panic
baz_bounds_line_273:
    mov rbp, 273
    jmp baz_bounds_panic
baz_bounds_line_384:
    mov rbp, 384
    jmp baz_bounds_panic
baz_bounds_line_385:
    mov rbp, 385
    jmp baz_bounds_panic
baz_bounds_line_392:
    mov rbp, 392
    jmp baz_bounds_panic
baz_bounds_line_393:
    mov rbp, 393
    jmp baz_bounds_panic
baz_bounds_line_427:
    mov rbp, 427
    jmp baz_bounds_panic
baz_bounds_line_428:
    mov rbp, 428
baz_bounds_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_bounds]
    mov rdx, msg_bounds_len
    syscall
    jmp baz_report_line
section .rodata
msg_bounds:
db `panic: bounds at line `
msg_bounds_len equ $ - msg_bounds
section .text
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
times 3 dq 0
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
;   [prog.baz:187:5] var answer = 0
;   [prog.baz:187:9] answer: i64 (8 B @ [rbp + 384])
;   [prog.baz:187:9] answer = 0
;   [prog.baz:187:18] answer = 0
;   [prog.baz:187:18] src: folded constant '0'
    mov qword [rbp + 384], 0
;   [prog.baz:189:5] assert(answer == 0)
;   [prog.baz:189:12] allocate scratch register -> r15
;   [prog.baz:189:12] ? answer == 0
    cmp.189.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.189.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.189.5:
;       [prog.baz:189:5] alias ok -> r15b
        if.38.27.189.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.189.5:
        cmp r15b, 0
        jne if.38.24.189.5.end
        if.38.27.189.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.189.5.end:
;       [prog.baz:189:5] free scratch register r15
    func.assert.189.5.end:
;   [prog.baz:191:5] answer = maybe
;   [prog.baz:191:14] answer = -1
;   [prog.baz:191:14] src: folded constant 'maybe'
    mov qword [rbp + 384], -1
;   [prog.baz:192:5] assert(answer == -1)
;   [prog.baz:192:12] allocate scratch register -> r15
;   [prog.baz:192:12] ? answer == -1
    cmp.192.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.192.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.192.5:
;       [prog.baz:192:5] alias ok -> r15b
        if.38.27.192.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.192.5:
        cmp r15b, 0
        jne if.38.24.192.5.end
        if.38.27.192.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.192.5.end:
;       [prog.baz:192:5] free scratch register r15
    func.assert.192.5.end:
;       [prog.baz:197:9] let maybe = 33
;       [prog.baz:197:13] const maybe = 33
;       [prog.baz:198:9] assert(maybe == 33)
;       [prog.baz:38:6] assert(ok bool)
        func.assert.198.9:
;           [prog.baz:198:9] alias ok -> 1
            if.38.27.198.9:
;           [prog.baz:38:27] ? shorthand: not ok
            cmp.38.27.198.9:
;           [prog.baz:38:31] const eval to false
            if.38.24.198.9.end:
        func.assert.198.9.end:
;   [prog.baz:201:5] assert(maybe == -1)
;   [prog.baz:38:6] assert(ok bool)
    func.assert.201.5:
;       [prog.baz:201:5] alias ok -> 1
        if.38.27.201.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.201.5:
;       [prog.baz:38:31] const eval to false
        if.38.24.201.5.end:
    func.assert.201.5.end:
;   [prog.baz:203:5] assert(nums[0] == 1 and nums[3] == 0)
;   [prog.baz:203:12] allocate scratch register -> r15
;   [prog.baz:203:12] ? nums[0] == 1 and nums[3] == 0
;   [prog.baz:203:12] ? nums[0] == 1
    cmp.203.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.203.12.end
;   [prog.baz:203:29] ? nums[3] == 0
    cmp.203.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.203.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.203.5:
;       [prog.baz:203:5] alias ok -> r15b
        if.38.27.203.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.203.5:
        cmp r15b, 0
        jne if.38.24.203.5.end
        if.38.27.203.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.203.5.end:
;       [prog.baz:203:5] free scratch register r15
    func.assert.203.5.end:
;   [prog.baz:204:5] assert(str2.len == 3 and str2.data[2] == 'z')
;   [prog.baz:204:12] allocate scratch register -> r15
;   [prog.baz:204:12] ? str2.len == 3 and str2.data[2] == 'z'
;   [prog.baz:204:12] ? str2.len == 3
    cmp.204.12:
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.204.12.end
;   [prog.baz:204:30] ? str2.data[2] == 'z'
    cmp.204.30:
    cmp byte [rbp + 227], 122
    sete r15b
    bool.204.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.204.5:
;       [prog.baz:204:5] alias ok -> r15b
        if.38.27.204.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.204.5:
        cmp r15b, 0
        jne if.38.24.204.5.end
        if.38.27.204.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.204.5.end:
;       [prog.baz:204:5] free scratch register r15
    func.assert.204.5.end:
;   [prog.baz:206:5] var a = 7
;   [prog.baz:206:9] a: i64 (8 B @ [rbp + 392])
;   [prog.baz:206:9] a = 7
;   [prog.baz:206:13] a = 7
;   [prog.baz:206:13] src: folded constant '7'
    mov qword [rbp + 392], 7
;   [prog.baz:207:5] assert(a & 3 == 3)
;   [prog.baz:207:12] allocate scratch register -> r15
;   [prog.baz:207:12] ? a & 3 == 3
    cmp.207.12:
;   [prog.baz:207:12] allocate scratch register -> r14
;       [prog.baz:207:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:207:12] r14 & 3
;       [prog.baz:207:12] src: folded constant '& 3'
        and r14, 3
    cmp r14, 3
;   [prog.baz:207:12] free scratch register r14
    sete r15b
    bool.207.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.207.5:
;       [prog.baz:207:5] alias ok -> r15b
        if.38.27.207.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.207.5:
        cmp r15b, 0
        jne if.38.24.207.5.end
        if.38.27.207.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.207.5.end:
;       [prog.baz:207:5] free scratch register r15
    func.assert.207.5.end:
;   [prog.baz:208:5] assert(a | 8 == 15)
;   [prog.baz:208:12] allocate scratch register -> r15
;   [prog.baz:208:12] ? a | 8 == 15
    cmp.208.12:
;   [prog.baz:208:12] allocate scratch register -> r14
;       [prog.baz:208:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:208:12] r14 | 8
;       [prog.baz:208:12] src: folded constant '| 8'
        or r14, 8
    cmp r14, 15
;   [prog.baz:208:12] free scratch register r14
    sete r15b
    bool.208.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.208.5:
;       [prog.baz:208:5] alias ok -> r15b
        if.38.27.208.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.208.5:
        cmp r15b, 0
        jne if.38.24.208.5.end
        if.38.27.208.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.208.5.end:
;       [prog.baz:208:5] free scratch register r15
    func.assert.208.5.end:
;   [prog.baz:209:5] assert(a ^ 1 == 6)
;   [prog.baz:209:12] allocate scratch register -> r15
;   [prog.baz:209:12] ? a ^ 1 == 6
    cmp.209.12:
;   [prog.baz:209:12] allocate scratch register -> r14
;       [prog.baz:209:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:209:12] r14 ^ 1
;       [prog.baz:209:12] src: folded constant '^ 1'
        xor r14, 1
    cmp r14, 6
;   [prog.baz:209:12] free scratch register r14
    sete r15b
    bool.209.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.209.5:
;       [prog.baz:209:5] alias ok -> r15b
        if.38.27.209.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.209.5:
        cmp r15b, 0
        jne if.38.24.209.5.end
        if.38.27.209.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.209.5.end:
;       [prog.baz:209:5] free scratch register r15
    func.assert.209.5.end:
;   [prog.baz:210:5] assert(a << 2 == 28)
;   [prog.baz:210:12] allocate scratch register -> r15
;   [prog.baz:210:12] ? a << 2 == 28
    cmp.210.12:
;   [prog.baz:210:12] allocate scratch register -> r14
;       [prog.baz:210:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:210:17] r14 << 2
;       [prog.baz:210:17] src: constant
        sal r14, 2
    cmp r14, 28
;   [prog.baz:210:12] free scratch register r14
    sete r15b
    bool.210.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.210.5:
;       [prog.baz:210:5] alias ok -> r15b
        if.38.27.210.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.210.5:
        cmp r15b, 0
        jne if.38.24.210.5.end
        if.38.27.210.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.210.5.end:
;       [prog.baz:210:5] free scratch register r15
    func.assert.210.5.end:
;   [prog.baz:211:5] assert(-a >> 1 == -4)
;   [prog.baz:211:12] allocate scratch register -> r15
;   [prog.baz:211:12] ? -a >> 1 == -4
    cmp.211.12:
;   [prog.baz:211:12] allocate scratch register -> r14
;       [prog.baz:211:13] -a
        mov r14, qword [rbp + 392]
        neg r14
        jo baz_overflow_line_211
;       [prog.baz:211:18] r14 >> 1
;       [prog.baz:211:18] src: constant
        sar r14, 1
    cmp r14, -4
;   [prog.baz:211:12] free scratch register r14
    sete r15b
    bool.211.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.211.5:
;       [prog.baz:211:5] alias ok -> r15b
        if.38.27.211.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.211.5:
        cmp r15b, 0
        jne if.38.24.211.5.end
        if.38.27.211.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.211.5.end:
;       [prog.baz:211:5] free scratch register r15
    func.assert.211.5.end:
;   [prog.baz:214:5] assert(a + a << 1 == 21)
;   [prog.baz:214:12] allocate scratch register -> r15
;   [prog.baz:214:12] ? a + a << 1 == 21
    cmp.214.12:
;   [prog.baz:214:12] allocate scratch register -> r14
;       [prog.baz:214:12] a
        mov r14, qword [rbp + 392]
;       [prog.baz:214:16] r14 + a << 1
;       [prog.baz:214:16] src: expression
;       [prog.baz:214:16] allocate scratch register -> r13
;       [prog.baz:214:16] a
        mov r13, qword [rbp + 392]
;       [prog.baz:214:21] r13 << 1
;       [prog.baz:214:21] src: constant
        sal r13, 1
        add r14, r13
        jo baz_overflow_line_214
;       [prog.baz:214:16] free scratch register r13
    cmp r14, 21
;   [prog.baz:214:12] free scratch register r14
    sete r15b
    bool.214.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.214.5:
;       [prog.baz:214:5] alias ok -> r15b
        if.38.27.214.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.214.5:
        cmp r15b, 0
        jne if.38.24.214.5.end
        if.38.27.214.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.214.5.end:
;       [prog.baz:214:5] free scratch register r15
    func.assert.214.5.end:
;   [prog.baz:215:5] assert((a + a) << 1 == 28)
;   [prog.baz:215:12] allocate scratch register -> r15
;   [prog.baz:215:12] ? (a + a) << 1 == 28
    cmp.215.12:
;   [prog.baz:215:12] allocate scratch register -> r14
;       [prog.baz:215:13] r14 = (a + a)
;       [prog.baz:215:13] = expression
;       [prog.baz:215:13] a
        mov r14, qword [rbp + 392]
;       [prog.baz:215:17] r14 + a
;       [prog.baz:215:17] src: operand
        add r14, qword [rbp + 392]
        jo baz_overflow_line_215
;       [prog.baz:215:23] r14 << 1
;       [prog.baz:215:23] src: constant
        sal r14, 1
    cmp r14, 28
;   [prog.baz:215:12] free scratch register r14
    sete r15b
    bool.215.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.215.5:
;       [prog.baz:215:5] alias ok -> r15b
        if.38.27.215.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.215.5:
        cmp r15b, 0
        jne if.38.24.215.5.end
        if.38.27.215.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.215.5.end:
;       [prog.baz:215:5] free scratch register r15
    func.assert.215.5.end:
;   [prog.baz:220:5] assert(a != 0 and (a >= 7 or a < 0))
;   [prog.baz:220:12] allocate scratch register -> r15
;   [prog.baz:220:12] ? a != 0 and (a >= 7 or a < 0)
;   [prog.baz:220:12] ? a != 0
    cmp.220.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.220.12.end
    cmp.220.23:
;   [prog.baz:220:23] ? (a >= 7 or a < 0)
;   [prog.baz:220:24] ? a >= 7
    cmp.220.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.220.12.end
;   [prog.baz:220:34] ? a < 0
    cmp.220.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.220.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.220.5:
;       [prog.baz:220:5] alias ok -> r15b
        if.38.27.220.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.220.5:
        cmp r15b, 0
        jne if.38.24.220.5.end
        if.38.27.220.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.220.5.end:
;       [prog.baz:220:5] free scratch register r15
    func.assert.220.5.end:
;   [prog.baz:223:5] var small = i8(100)
;   [prog.baz:223:9] small: i8 (1 B @ [rbp + 400])
;   [prog.baz:223:9] small = i8(100)
;   [prog.baz:223:17] small = 100
;   [prog.baz:223:17] src: folded constant 'i8(100)'
    mov byte [rbp + 400], 100
;   [prog.baz:224:5] small = i8(small + small)
;   [prog.baz:224:13] small = i8(small + small)
;   [prog.baz:224:13] = expression
;   [prog.baz:224:13] smaller of two versions of 3 and 3 instructions
;   [prog.baz:224:16] allocate scratch register -> r15
;   [prog.baz:224:16] small
    mov r15b, byte [rbp + 400]
;   [prog.baz:224:24] r15b + small
;   [prog.baz:224:24] src: operand
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
;   [prog.baz:224:16] free scratch register r15
;   [prog.baz:225:5] assert(small == -56)
;   [prog.baz:225:12] allocate scratch register -> r15
;   [prog.baz:225:12] ? small == -56
    cmp.225.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.225.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.225.5:
;       [prog.baz:225:5] alias ok -> r15b
        if.38.27.225.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.225.5:
        cmp r15b, 0
        jne if.38.24.225.5.end
        if.38.27.225.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.225.5.end:
;       [prog.baz:225:5] free scratch register r15
    func.assert.225.5.end:
;   [prog.baz:228:5] var wide = int(i16(small))
;   [prog.baz:228:9] wide: i64 (8 B @ [rbp + 408])
;   [prog.baz:228:9] wide = int(i16(small))
;   [prog.baz:228:16] wide = int(i16(small))
;   [prog.baz:228:16] = expression
;   [prog.baz:228:20] wide = i16(small)
;   [prog.baz:228:20] = expression
;   [prog.baz:228:20] allocate scratch register -> r15
;   [prog.baz:228:24] small
    movsx r15w, byte [rbp + 400]
    movsx r15, r15w
    mov qword [rbp + 408], r15
;   [prog.baz:228:20] free scratch register r15
;   [prog.baz:229:5] assert(wide == -56)
;   [prog.baz:229:12] allocate scratch register -> r15
;   [prog.baz:229:12] ? wide == -56
    cmp.229.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.229.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.229.5:
;       [prog.baz:229:5] alias ok -> r15b
        if.38.27.229.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.229.5:
        cmp r15b, 0
        jne if.38.24.229.5.end
        if.38.27.229.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.229.5.end:
;       [prog.baz:229:5] free scratch register r15
    func.assert.229.5.end:
;   [prog.baz:236:5] var letter = '\x41'
;   [prog.baz:236:9] letter: i64 (8 B @ [rbp + 416])
;   [prog.baz:236:9] letter = '\x41'
;   [prog.baz:236:18] letter = 65
;   [prog.baz:236:18] src: folded constant ''\x41''
    mov qword [rbp + 416], 65
;   [prog.baz:237:5] assert(letter == 'A')
;   [prog.baz:237:12] allocate scratch register -> r15
;   [prog.baz:237:12] ? letter == 'A'
    cmp.237.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.237.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.237.5:
;       [prog.baz:237:5] alias ok -> r15b
        if.38.27.237.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.237.5:
        cmp r15b, 0
        jne if.38.24.237.5.end
        if.38.27.237.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.237.5.end:
;       [prog.baz:237:5] free scratch register r15
    func.assert.237.5.end:
;   [prog.baz:240:5] var arr = i32[4]
;   [prog.baz:240:9] arr: i32[4] (16 B @ [rbp + 424])
;   [prog.baz:240:9] arr = i32[4]
;   [prog.baz:240:15] zero remaining elements: 4 * 4 B = 16 B
;   [prog.baz:240:15] size <= 32 B, use mov
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
;   [prog.baz:242:5] var ix = 1
;   [prog.baz:242:9] ix: i64 (8 B @ [rbp + 440])
;   [prog.baz:242:9] ix = 1
;   [prog.baz:242:14] ix = 1
;   [prog.baz:242:14] src: folded constant '1'
    mov qword [rbp + 440], 1
;   [prog.baz:243:5] arr[ix] = 2
;   [prog.baz:243:9] allocate scratch register -> r15
;   [prog.baz:243:9] set array index
;   [prog.baz:243:9] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:243:9] bounds check begin
;   [prog.baz:243:9] lower bound
;   [prog.baz:243:9] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:243:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_243
;   [prog.baz:243:9] bounds check end
;   [prog.baz:243:15] arr = 2
;   [prog.baz:243:15] src: folded constant '2'
    mov dword [rbp + r15 * 4 + 424], 2
;   [prog.baz:243:5] free scratch register r15
;   [prog.baz:244:5] arr[ix + 1] = arr[ix]
;   [prog.baz:244:9] allocate scratch register -> r15
;   [prog.baz:244:9] set array index
;   [prog.baz:244:9] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:244:9] r15 + 1
;   [prog.baz:244:9] src: folded constant '+ 1'
    add r15, 1
    jo baz_overflow_line_244
;   [prog.baz:244:9] bounds check begin
;   [prog.baz:244:9] lower bound
;   [prog.baz:244:9] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:244:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_244
;   [prog.baz:244:9] bounds check end
;   [prog.baz:244:19] arr[ix]
;   [prog.baz:244:23] allocate scratch register -> r14
;   [prog.baz:244:23] set array index
;   [prog.baz:244:23] ix
    mov r14, qword [rbp + 440]
;   [prog.baz:244:23] bounds check begin
;   [prog.baz:244:23] lower bound
;   [prog.baz:244:23] r14 lower bound covered by the unsigned upper bound
;   [prog.baz:244:23] upper bound
    cmp r14, 4
    jae baz_bounds_line_244
;   [prog.baz:244:23] bounds check end
;   [prog.baz:244:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
;   [prog.baz:244:19] free scratch register r13
;   [prog.baz:244:19] free scratch register r14
;   [prog.baz:244:5] free scratch register r15
;   [prog.baz:245:5] assert(arr[1] == 2)
;   [prog.baz:245:12] allocate scratch register -> r15
;   [prog.baz:245:12] ? arr[1] == 2
    cmp.245.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.245.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.245.5:
;       [prog.baz:245:5] alias ok -> r15b
        if.38.27.245.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.245.5:
        cmp r15b, 0
        jne if.38.24.245.5.end
        if.38.27.245.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.245.5.end:
;       [prog.baz:245:5] free scratch register r15
    func.assert.245.5.end:
;   [prog.baz:246:5] assert(arr[2] == 2)
;   [prog.baz:246:12] allocate scratch register -> r15
;   [prog.baz:246:12] ? arr[2] == 2
    cmp.246.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.246.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.246.5:
;       [prog.baz:246:5] alias ok -> r15b
        if.38.27.246.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.246.5:
        cmp r15b, 0
        jne if.38.24.246.5.end
        if.38.27.246.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.246.5.end:
;       [prog.baz:246:5] free scratch register r15
    func.assert.246.5.end:
;   [prog.baz:248:5] array_copy(arr[2], arr, 2)
;   [prog.baz:248:5] allocate named register rsi
;   [prog.baz:248:5] allocate named register rdi
;   [prog.baz:248:5] allocate named register rcx
;   [prog.baz:248:29] 2
;   [prog.baz:248:29] rcx = 2
;   [prog.baz:248:29] src: folded constant '2'
    mov rcx, 2
;   [prog.baz:248:16] arr[2]
;   [prog.baz:248:20] allocate scratch register -> r15
;   [prog.baz:248:20] set array index
;   [prog.baz:248:20] r15 = 2
;   [prog.baz:248:20] src: folded constant '2'
    mov r15, 2
;   [prog.baz:248:20] bounds check begin
;   [prog.baz:248:20] lower bound
    test r15, r15
    js baz_bounds_line_248
    test rcx, rcx
    js baz_bounds_line_248
;   [prog.baz:248:20] upper bound
;   [prog.baz:248:20] allocate scratch register -> r14
    lea r14, [rcx + r15]
    cmp r14, 4
;   [prog.baz:248:20] free scratch register r14
    ja baz_bounds_line_248
;   [prog.baz:248:20] bounds check end
    lea rsi, [rbp + r15 * 4 + 424]
;   [prog.baz:248:5] free scratch register r15
;   [prog.baz:248:24] arr
;   [prog.baz:248:24] bounds check begin
;   [prog.baz:248:24] lower bound
;   [prog.baz:248:24] rcx lower bound covered by the unsigned upper bound
;   [prog.baz:248:24] upper bound
    cmp rcx, 4
    ja baz_bounds_line_248
;   [prog.baz:248:24] bounds check end
    lea rdi, [rbp + 424]
    sal rcx, 2
;   [prog.baz:248:5] overlap check begin
;   [prog.baz:248:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [prog.baz:248:5] the same range is not an overlap
    je .Lbaz_overlap.0
;   [prog.baz:248:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_248
    .Lbaz_overlap.0:
;   [prog.baz:248:5] free scratch register r15
;   [prog.baz:248:5] overlap check end
    rep movsb
;   [prog.baz:248:5] free named register rcx
;   [prog.baz:248:5] free named register rdi
;   [prog.baz:248:5] free named register rsi
;   [prog.baz:249:5] assert(arr[0] == 2)
;   [prog.baz:249:12] allocate scratch register -> r15
;   [prog.baz:249:12] ? arr[0] == 2
    cmp.249.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.249.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.249.5:
;       [prog.baz:249:5] alias ok -> r15b
        if.38.27.249.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.249.5:
        cmp r15b, 0
        jne if.38.24.249.5.end
        if.38.27.249.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.249.5.end:
;       [prog.baz:249:5] free scratch register r15
    func.assert.249.5.end:
;   [prog.baz:252:5] var arr1 = i32[8]
;   [prog.baz:252:9] arr1: i32[8] (32 B @ [rbp + 448])
;   [prog.baz:252:9] arr1 = i32[8]
;   [prog.baz:252:16] zero remaining elements: 8 * 4 B = 32 B
;   [prog.baz:252:16] size <= 32 B, use mov
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
;   [prog.baz:253:5] array_copy(arr, arr1, 4)
;   [prog.baz:253:5] allocate named register rsi
;   [prog.baz:253:5] allocate named register rdi
;   [prog.baz:253:5] allocate named register rcx
;   [prog.baz:253:27] 4
;   [prog.baz:253:27] rcx = 4
;   [prog.baz:253:27] src: folded constant '4'
    mov rcx, 4
;   [prog.baz:253:16] arr
;   [prog.baz:253:16] bounds check begin
;   [prog.baz:253:16] lower bound
;   [prog.baz:253:16] rcx lower bound covered by the unsigned upper bound
;   [prog.baz:253:16] upper bound
    cmp rcx, 4
    ja baz_bounds_line_253
;   [prog.baz:253:16] bounds check end
    lea rsi, [rbp + 424]
;   [prog.baz:253:21] arr1
;   [prog.baz:253:21] bounds check begin
;   [prog.baz:253:21] lower bound
;   [prog.baz:253:21] rcx lower bound covered by the unsigned upper bound
;   [prog.baz:253:21] upper bound
    cmp rcx, 8
    ja baz_bounds_line_253
;   [prog.baz:253:21] bounds check end
    lea rdi, [rbp + 448]
    sal rcx, 2
;   [prog.baz:253:5] overlap check begin
;   [prog.baz:253:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [prog.baz:253:5] the same range is not an overlap
    je .Lbaz_overlap.1
;   [prog.baz:253:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_253
    .Lbaz_overlap.1:
;   [prog.baz:253:5] free scratch register r15
;   [prog.baz:253:5] overlap check end
    rep movsb
;   [prog.baz:253:5] free named register rcx
;   [prog.baz:253:5] free named register rdi
;   [prog.baz:253:5] free named register rsi
;   [prog.baz:254:5] var eq = arrays_equal(arr[1], arr1[1], 3)
;   [prog.baz:254:9] eq: bool (1 B @ [rbp + 480])
;   [prog.baz:254:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [prog.baz:254:14] ? shorthand: arrays_equal(arr[1], arr1[1], 3)
    cmp.254.14:
;       [prog.baz:254:14] arrays_equal(arr[1], arr1[1], 3)
;       [prog.baz:254:14] allocate named register rsi
;       [prog.baz:254:14] allocate named register rdi
;       [prog.baz:254:14] allocate named register rcx
;       [prog.baz:254:44] 3
;       [prog.baz:254:44] rcx = 3
;       [prog.baz:254:44] src: folded constant '3'
        mov rcx, 3
;       [prog.baz:254:27] arr[1]
;       [prog.baz:254:31] allocate scratch register -> r15
;       [prog.baz:254:31] set array index
;       [prog.baz:254:31] r15 = 1
;       [prog.baz:254:31] src: folded constant '1'
        mov r15, 1
;       [prog.baz:254:31] bounds check begin
;       [prog.baz:254:31] lower bound
        test r15, r15
        js baz_bounds_line_254
        test rcx, rcx
        js baz_bounds_line_254
;       [prog.baz:254:31] upper bound
;       [prog.baz:254:31] allocate scratch register -> r14
        lea r14, [rcx + r15]
        cmp r14, 4
;       [prog.baz:254:31] free scratch register r14
        ja baz_bounds_line_254
;       [prog.baz:254:31] bounds check end
        lea rsi, [rbp + r15 * 4 + 424]
;       [prog.baz:254:14] free scratch register r15
;       [prog.baz:254:35] arr1[1]
;       [prog.baz:254:40] allocate scratch register -> r15
;       [prog.baz:254:40] set array index
;       [prog.baz:254:40] r15 = 1
;       [prog.baz:254:40] src: folded constant '1'
        mov r15, 1
;       [prog.baz:254:40] bounds check begin
;       [prog.baz:254:40] lower bound
;       [prog.baz:254:40] count rcx lower bound already checked
        test r15, r15
        js baz_bounds_line_254
;       [prog.baz:254:40] upper bound
;       [prog.baz:254:40] allocate scratch register -> r14
        lea r14, [rcx + r15]
        cmp r14, 8
;       [prog.baz:254:40] free scratch register r14
        ja baz_bounds_line_254
;       [prog.baz:254:40] bounds check end
        lea rdi, [rbp + r15 * 4 + 448]
;       [prog.baz:254:14] free scratch register r15
        sal rcx, 2
        test rcx, rcx
        repe cmpsb
;       [prog.baz:254:14] free named register rcx
;       [prog.baz:254:14] free named register rdi
;       [prog.baz:254:14] free named register rsi
        sete byte [rbp + 480]
    bool.254.14.end:
;   [prog.baz:257:5] assert(eq)
;   [prog.baz:38:6] assert(ok bool)
    func.assert.257.5:
;       [prog.baz:257:5] alias ok -> eq
        if.38.27.257.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.257.5:
        cmp byte [rbp + 480], 0
        jne if.38.24.257.5.end
        if.38.27.257.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.257.5.end:
    func.assert.257.5.end:
;   [prog.baz:259:5] arr1[2] = -1
;   [prog.baz:259:15] arr1 = -1
;   [prog.baz:259:15] src: folded constant '-1'
    mov dword [rbp + 456], -1
;   [prog.baz:260:5] assert(not arrays_equal(arr, arr1, 4))
;   [prog.baz:260:12] allocate scratch register -> r15
;   [prog.baz:260:12] ? shorthand: not arrays_equal(arr, arr1, 4)
    cmp.260.12:
;       [prog.baz:260:16] arrays_equal(arr, arr1, 4)
;       [prog.baz:260:16] allocate named register rsi
;       [prog.baz:260:16] allocate named register rdi
;       [prog.baz:260:16] allocate named register rcx
;       [prog.baz:260:40] 4
;       [prog.baz:260:40] rcx = 4
;       [prog.baz:260:40] src: folded constant '4'
        mov rcx, 4
;       [prog.baz:260:29] arr
;       [prog.baz:260:29] bounds check begin
;       [prog.baz:260:29] lower bound
;       [prog.baz:260:29] rcx lower bound covered by the unsigned upper bound
;       [prog.baz:260:29] upper bound
        cmp rcx, 4
        ja baz_bounds_line_260
;       [prog.baz:260:29] bounds check end
        lea rsi, [rbp + 424]
;       [prog.baz:260:34] arr1
;       [prog.baz:260:34] bounds check begin
;       [prog.baz:260:34] lower bound
;       [prog.baz:260:34] rcx lower bound covered by the unsigned upper bound
;       [prog.baz:260:34] upper bound
        cmp rcx, 8
        ja baz_bounds_line_260
;       [prog.baz:260:34] bounds check end
        lea rdi, [rbp + 448]
        sal rcx, 2
        test rcx, rcx
        repe cmpsb
;       [prog.baz:260:16] free named register rcx
;       [prog.baz:260:16] free named register rdi
;       [prog.baz:260:16] free named register rsi
        setne r15b
    bool.260.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.260.5:
;       [prog.baz:260:5] alias ok -> r15b
        if.38.27.260.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.260.5:
        cmp r15b, 0
        jne if.38.24.260.5.end
        if.38.27.260.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.260.5.end:
;       [prog.baz:260:5] free scratch register r15
    func.assert.260.5.end:
;   [prog.baz:262:5] var arr4 = arr
;   [prog.baz:262:9] arr4: i32[4] (16 B @ [rbp + 484])
;   [prog.baz:262:9] arr4 = arr
;   [prog.baz:262:16] size <= 16 B, use mov
;   [prog.baz:262:16] allocate scratch register -> r15
    mov r15, qword [rbp + 424]
    mov qword [rbp + 484], r15
    mov r15, qword [rbp + 432]
    mov qword [rbp + 492], r15
;   [prog.baz:262:16] free scratch register r15
;   [prog.baz:263:5] assert(arr == arr4)
;   [prog.baz:263:12] allocate scratch register -> r15
;   [prog.baz:263:12] ? arr == arr4
    cmp.263.12:
;       [prog.baz:263:12] allocate named register rsi
;       [prog.baz:263:12] allocate named register rdi
;       [prog.baz:263:12] arr
        lea rsi, [rbp + 424]
;       [prog.baz:263:19] arr4
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
;       [prog.baz:263:12] free named register rdi
;       [prog.baz:263:12] free named register rsi
        sete r15b
    bool.263.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.263.5:
;       [prog.baz:263:5] alias ok -> r15b
        if.38.27.263.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.263.5:
        cmp r15b, 0
        jne if.38.24.263.5.end
        if.38.27.263.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.263.5.end:
;       [prog.baz:263:5] free scratch register r15
    func.assert.263.5.end:
;   [prog.baz:270:5] ix = 3
;   [prog.baz:270:10] ix = 3
;   [prog.baz:270:10] src: folded constant '3'
    mov qword [rbp + 440], 3
;   [prog.baz:271:5] var tmp = ~inv(arr[ix - 1])
;   [prog.baz:271:9] tmp: i32 (4 B @ [rbp + 500])
;   [prog.baz:271:9] tmp = ~inv(arr[ix - 1])
;   [prog.baz:271:16] tmp = ~inv(arr[ix - 1])
;   [prog.baz:271:16] = expression
;   [prog.baz:271:16] ~inv(arr[ix - 1])
;   [prog.baz:271:16] smaller of two versions of 9 and 9 instructions
;   [prog.baz:271:24] allocate scratch register -> r15
;   [prog.baz:271:24] set array index
;   [prog.baz:271:24] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:271:24] r15 - 1
;   [prog.baz:271:24] src: folded constant '- 1'
    sub r15, 1
    jo baz_overflow_line_271
;   [prog.baz:271:24] bounds check begin
;   [prog.baz:271:24] lower bound
;   [prog.baz:271:24] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:271:24] upper bound
    cmp r15, 4
    jae baz_bounds_line_271
;   [prog.baz:271:24] bounds check end
;   [prog.baz:72:6] inv(i i32) res i32
    func.inv.271.16:
;       [prog.baz:271:16] alias res -> tmp
;       [prog.baz:271:16] alias i -> arr (lea: rbp + r15 * 4 + 424)
;       [prog.baz:73:5] res = ~i
;       [prog.baz:73:11] smaller of two versions of 3 and 3 instructions
;       [prog.baz:73:12] ~i
;       [prog.baz:73:12] allocate scratch register -> r14
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
;       [prog.baz:73:12] free scratch register r14
        not dword [rbp + 500]
;       [prog.baz:271:16] free scratch register r15
    func.inv.271.16.end:
    not dword [rbp + 500]
;   [prog.baz:272:5] arr[ix] = tmp
;   [prog.baz:272:9] allocate scratch register -> r15
;   [prog.baz:272:9] set array index
;   [prog.baz:272:9] ix
    mov r15, qword [rbp + 440]
;   [prog.baz:272:9] bounds check begin
;   [prog.baz:272:9] lower bound
;   [prog.baz:272:9] r15 lower bound covered by the unsigned upper bound
;   [prog.baz:272:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_272
;   [prog.baz:272:9] bounds check end
;   [prog.baz:272:15] tmp
;   [prog.baz:272:15] allocate scratch register -> r14
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
;   [prog.baz:272:15] free scratch register r14
;   [prog.baz:272:5] free scratch register r15
;   [prog.baz:273:5] assert(arr[ix] == 2)
;   [prog.baz:273:12] allocate scratch register -> r15
;   [prog.baz:273:12] ? arr[ix] == 2
    cmp.273.12:
;   [prog.baz:273:16] allocate scratch register -> r14
;   [prog.baz:273:16] set array index
;   [prog.baz:273:16] ix
    mov r14, qword [rbp + 440]
;   [prog.baz:273:16] bounds check begin
;   [prog.baz:273:16] lower bound
;   [prog.baz:273:16] r14 lower bound covered by the unsigned upper bound
;   [prog.baz:273:16] upper bound
    cmp r14, 4
    jae baz_bounds_line_273
;   [prog.baz:273:16] bounds check end
    cmp dword [rbp + r14 * 4 + 424], 2
;   [prog.baz:273:12] free scratch register r14
    sete r15b
    bool.273.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.273.5:
;       [prog.baz:273:5] alias ok -> r15b
        if.38.27.273.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.273.5:
        cmp r15b, 0
        jne if.38.24.273.5.end
        if.38.27.273.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.273.5.end:
;       [prog.baz:273:5] free scratch register r15
    func.assert.273.5.end:
;   [prog.baz:275:5] faz(arr)
;   [prog.baz:78:6] faz(arg mut i32[])
    func.faz.275.5:
;       [prog.baz:275:5] alias arg -> arr
;       [prog.baz:79:5] arg[1] = 0xfe
;       [prog.baz:79:14] arg = 254
;       [prog.baz:79:14] src: folded constant '0xfe'
        mov dword [rbp + 428], 254
    func.faz.275.5.end:
;   [prog.baz:276:5] assert(arr[1] == 0xfe)
;   [prog.baz:276:12] allocate scratch register -> r15
;   [prog.baz:276:12] ? arr[1] == 0xfe
    cmp.276.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.276.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.276.5:
;       [prog.baz:276:5] alias ok -> r15b
        if.38.27.276.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.276.5:
        cmp r15b, 0
        jne if.38.24.276.5.end
        if.38.27.276.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.276.5.end:
;       [prog.baz:276:5] free scratch register r15
    func.assert.276.5.end:
;   [prog.baz:278:5] var arr3 = []{ 3, 5 }
;   [prog.baz:278:9] arr3: i64[2] (16 B @ [rbp + 504])
;   [prog.baz:278:9] arr3 = []{ 3, 5 }
;   [prog.baz:278:18] size <= 16 B, use immediates
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
;   [prog.baz:279:5] foo arr3
;   [prog.baz:279:9] allocate scratch register -> r15
;   [prog.baz:279:9] initialize iterator e
    lea r15, [rbp + 504]
;   [prog.baz:279:5] allocate scratch register -> r14
;   [prog.baz:279:9] e: i64 (r15)
;   [prog.baz:279:9] i: i64 (r14)
;   [prog.baz:279:9] const n = 2
;   [prog.baz:279:5] initialize counter i
    mov r14, 0
    foo.279.5:
;       [prog.baz:280:9] e = e + i + n
;       [prog.baz:280:13] smaller of two versions of 4 and 6 instructions
;       [prog.baz:280:13] e
;       [prog.baz:280:17] e + i
;       [prog.baz:280:17] src: operand
        add qword [r15], r14
        jo baz_overflow_line_280
;       [prog.baz:280:13] e + 2
;       [prog.baz:280:13] src: folded constant '+ n'
        add qword [r15], 2
        jo baz_overflow_line_280
        foo.279.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.279.5
    foo.279.5.end:
;   [prog.baz:279:5] free scratch register r14
;   [prog.baz:279:5] free scratch register r15
;   [prog.baz:282:5] assert(arr3[0] == 3 + 0 + 2)
;   [prog.baz:282:12] allocate scratch register -> r15
;   [prog.baz:282:12] ? arr3[0] == 3 + 0 + 2
    cmp.282.12:
;   [prog.baz:282:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 504], 5
    sete r15b
    bool.282.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.282.5:
;       [prog.baz:282:5] alias ok -> r15b
        if.38.27.282.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.282.5:
        cmp r15b, 0
        jne if.38.24.282.5.end
        if.38.27.282.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.282.5.end:
;       [prog.baz:282:5] free scratch register r15
    func.assert.282.5.end:
;   [prog.baz:283:5] assert(arr3[1] == 5 + 1 + 2)
;   [prog.baz:283:12] allocate scratch register -> r15
;   [prog.baz:283:12] ? arr3[1] == 5 + 1 + 2
    cmp.283.12:
;   [prog.baz:283:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 512], 8
    sete r15b
    bool.283.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.283.5:
;       [prog.baz:283:5] alias ok -> r15b
        if.38.27.283.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.283.5:
        cmp r15b, 0
        jne if.38.24.283.5.end
        if.38.27.283.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.283.5.end:
;       [prog.baz:283:5] free scratch register r15
    func.assert.283.5.end:
;   [prog.baz:289:5] var p = point
;   [prog.baz:289:9] p: point (16 B @ [rbp + 520])
;   [prog.baz:289:9] p = point
;   [prog.baz:289:13] zero remaining fields: 16 B
;   [prog.baz:289:13] size <= 32 B, use mov
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
;   [prog.baz:291:7] p.fooz()
;   [prog.baz:47:10] mut point.fooz()
    func.point.fooz.291.7:
;       [prog.baz:291:7] alias self -> p
;       [prog.baz:48:5] self.x = 0b10
;       [prog.baz:48:14] self.x = 2
;       [prog.baz:48:14] src: folded constant '0b10'
        mov qword [rbp + 520], 2
;       [prog.baz:49:5] self.y = 0xb
;       [prog.baz:49:14] self.y = 11
;       [prog.baz:49:14] src: folded constant '0xb'
        mov qword [rbp + 528], 11
    func.point.fooz.291.7.end:
;   [prog.baz:294:5] assert(p.x == 2)
;   [prog.baz:294:12] allocate scratch register -> r15
;   [prog.baz:294:12] ? p.x == 2
    cmp.294.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.294.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.294.5:
;       [prog.baz:294:5] alias ok -> r15b
        if.38.27.294.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.294.5:
        cmp r15b, 0
        jne if.38.24.294.5.end
        if.38.27.294.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.294.5.end:
;       [prog.baz:294:5] free scratch register r15
    func.assert.294.5.end:
;   [prog.baz:295:5] assert(p.y == 0xb)
;   [prog.baz:295:12] allocate scratch register -> r15
;   [prog.baz:295:12] ? p.y == 0xb
    cmp.295.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.295.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.295.5:
;       [prog.baz:295:5] alias ok -> r15b
        if.38.27.295.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.295.5:
        cmp r15b, 0
        jne if.38.24.295.5.end
        if.38.27.295.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.295.5.end:
;       [prog.baz:295:5] free scratch register r15
    func.assert.295.5.end:
;   [prog.baz:297:5] var q = p
;   [prog.baz:297:9] q: point (16 B @ [rbp + 536])
;   [prog.baz:297:9] q = p
;   [prog.baz:297:13] size <= 16 B, use mov
;   [prog.baz:297:13] allocate scratch register -> r15
    mov r15, qword [rbp + 520]
    mov qword [rbp + 536], r15
    mov r15, qword [rbp + 528]
    mov qword [rbp + 544], r15
;   [prog.baz:297:13] free scratch register r15
;   [prog.baz:300:5] assert(p == q)
;   [prog.baz:300:12] allocate scratch register -> r15
;   [prog.baz:300:12] ? p == q
    cmp.300.12:
;       [prog.baz:300:12] allocate named register rsi
;       [prog.baz:300:12] allocate named register rdi
;       [prog.baz:300:12] p
        lea rsi, [rbp + 520]
;       [prog.baz:300:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
;       [prog.baz:300:12] free named register rdi
;       [prog.baz:300:12] free named register rsi
        sete r15b
    bool.300.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.300.5:
;       [prog.baz:300:5] alias ok -> r15b
        if.38.27.300.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.300.5:
        cmp r15b, 0
        jne if.38.24.300.5.end
        if.38.27.300.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.300.5.end:
;       [prog.baz:300:5] free scratch register r15
    func.assert.300.5.end:
;   [prog.baz:304:5] q.x = 3
;   [prog.baz:304:11] q.x = 3
;   [prog.baz:304:11] src: folded constant '3'
    mov qword [rbp + 536], 3
;   [prog.baz:305:5] assert(p != q)
;   [prog.baz:305:12] allocate scratch register -> r15
;   [prog.baz:305:12] ? p != q
    cmp.305.12:
;       [prog.baz:305:12] allocate named register rsi
;       [prog.baz:305:12] allocate named register rdi
;       [prog.baz:305:12] p
        lea rsi, [rbp + 520]
;       [prog.baz:305:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
;       [prog.baz:305:12] free named register rdi
;       [prog.baz:305:12] free named register rsi
        setne r15b
    bool.305.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.305.5:
;       [prog.baz:305:5] alias ok -> r15b
        if.38.27.305.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.305.5:
        cmp r15b, 0
        jne if.38.24.305.5.end
        if.38.27.305.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.305.5.end:
;       [prog.baz:305:5] free scratch register r15
    func.assert.305.5.end:
;   [prog.baz:307:5] var i = 0
;   [prog.baz:307:9] i: i64 (8 B @ [rbp + 552])
;   [prog.baz:307:9] i = 0
;   [prog.baz:307:13] i = 0
;   [prog.baz:307:13] src: folded constant '0'
    mov qword [rbp + 552], 0
;   [prog.baz:308:5] bar(i)
;   [prog.baz:59:6] bar(arg mut)
    func.bar.308.5:
;       [prog.baz:308:5] alias arg -> i
        if.60.8.308.5:
;       [prog.baz:60:8] ? arg == 0
        cmp.60.8.308.5:
        cmp qword [rbp + 552], 0
        je func.bar.308.5.end
        if.60.8.308.5.code:
;           [prog.baz:60:17] return
        if.60.5.308.5.end:
;       [prog.baz:61:5] arg = 0xff
;       [prog.baz:61:11] arg = 255
;       [prog.baz:61:11] src: folded constant '0xff'
        mov qword [rbp + 552], 255
    func.bar.308.5.end:
;   [prog.baz:309:5] assert(i == 0)
;   [prog.baz:309:12] allocate scratch register -> r15
;   [prog.baz:309:12] ? i == 0
    cmp.309.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.309.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.309.5:
;       [prog.baz:309:5] alias ok -> r15b
        if.38.27.309.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.309.5:
        cmp r15b, 0
        jne if.38.24.309.5.end
        if.38.27.309.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.309.5.end:
;       [prog.baz:309:5] free scratch register r15
    func.assert.309.5.end:
;   [prog.baz:311:5] i = 1
;   [prog.baz:311:9] i = 1
;   [prog.baz:311:9] src: folded constant '1'
    mov qword [rbp + 552], 1
;   [prog.baz:312:5] bar(i)
;   [prog.baz:59:6] bar(arg mut)
    func.bar.312.5:
;       [prog.baz:312:5] alias arg -> i
        if.60.8.312.5:
;       [prog.baz:60:8] ? arg == 0
        cmp.60.8.312.5:
        cmp qword [rbp + 552], 0
        je func.bar.312.5.end
        if.60.8.312.5.code:
;           [prog.baz:60:17] return
        if.60.5.312.5.end:
;       [prog.baz:61:5] arg = 0xff
;       [prog.baz:61:11] arg = 255
;       [prog.baz:61:11] src: folded constant '0xff'
        mov qword [rbp + 552], 255
    func.bar.312.5.end:
;   [prog.baz:313:5] assert(i == 0xff)
;   [prog.baz:313:12] allocate scratch register -> r15
;   [prog.baz:313:12] ? i == 0xff
    cmp.313.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.313.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.313.5:
;       [prog.baz:313:5] alias ok -> r15b
        if.38.27.313.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.313.5:
        cmp r15b, 0
        jne if.38.24.313.5.end
        if.38.27.313.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.313.5.end:
;       [prog.baz:313:5] free scratch register r15
    func.assert.313.5.end:
;   [prog.baz:315:5] var j = 1
;   [prog.baz:315:9] j: i64 (8 B @ [rbp + 560])
;   [prog.baz:315:9] j = 1
;   [prog.baz:315:13] j = 1
;   [prog.baz:315:13] src: folded constant '1'
    mov qword [rbp + 560], 1
;   [prog.baz:316:5] var k = baz(j)
;   [prog.baz:316:9] k: i64 (8 B @ [rbp + 568])
;   [prog.baz:316:9] k = baz(j)
;   [prog.baz:316:13] k = baz(j)
;   [prog.baz:316:13] = expression
;   [prog.baz:316:13] baz(j)
;   [prog.baz:66:6] baz(arg) res
    func.baz.316.13:
;       [prog.baz:316:13] alias res -> k
;       [prog.baz:316:13] alias arg -> j
;       [prog.baz:67:5] res = arg * 2
;       [prog.baz:67:11] smaller of two versions of 6 and 4 instructions
;       [prog.baz:67:11] allocate scratch register -> r15
;       [prog.baz:67:11] arg
        mov r15, qword [rbp + 560]
;       [prog.baz:67:11] r15 * 2
;       [prog.baz:67:11] src: folded constant '* 2'
        imul r15, 2
        jo baz_overflow_line_67
        mov qword [rbp + 568], r15
;       [prog.baz:67:11] free scratch register r15
    func.baz.316.13.end:
;   [prog.baz:317:5] assert(k == 2)
;   [prog.baz:317:12] allocate scratch register -> r15
;   [prog.baz:317:12] ? k == 2
    cmp.317.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.317.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.317.5:
;       [prog.baz:317:5] alias ok -> r15b
        if.38.27.317.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.317.5:
        cmp r15b, 0
        jne if.38.24.317.5.end
        if.38.27.317.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.317.5.end:
;       [prog.baz:317:5] free scratch register r15
    func.assert.317.5.end:
;   [prog.baz:319:5] k = baz(1)
;   [prog.baz:319:9] k = baz(1)
;   [prog.baz:319:9] = expression
;   [prog.baz:319:9] baz(1)
;   [prog.baz:66:6] baz(arg) res
    func.baz.319.9:
;       [prog.baz:319:9] alias res -> k
;       [prog.baz:319:9] alias arg -> 1
;       [prog.baz:67:5] res = arg * 2
;       [prog.baz:67:11] res = 2
;       [prog.baz:67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 568], 2
    func.baz.319.9.end:
;   [prog.baz:320:5] assert(k == 2)
;   [prog.baz:320:12] allocate scratch register -> r15
;   [prog.baz:320:12] ? k == 2
    cmp.320.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.320.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.320.5:
;       [prog.baz:320:5] alias ok -> r15b
        if.38.27.320.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.320.5:
        cmp r15b, 0
        jne if.38.24.320.5.end
        if.38.27.320.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.320.5.end:
;       [prog.baz:320:5] free scratch register r15
    func.assert.320.5.end:
;   [prog.baz:322:5] var five = 5
;   [prog.baz:322:9] five: i64 (8 B @ [rbp + 576])
;   [prog.baz:322:9] five = 5
;   [prog.baz:322:16] five = 5
;   [prog.baz:322:16] src: folded constant '5'
    mov qword [rbp + 576], 5
;   [prog.baz:323:5] var f = factorial(five)
;   [prog.baz:323:9] f: i64 (8 B @ [rbp + 584])
;   [prog.baz:323:9] f = factorial(five)
;   [prog.baz:323:13] f = factorial(five)
;   [prog.baz:323:13] = expression
;   [prog.baz:323:13] factorial(five)
;   [prog.baz:323:13] frame capacity check begin
;   [prog.baz:323:13] allocate scratch register -> r15
;   [prog.baz:323:13] allocate scratch register -> r14
    lea r15, [rbp + 592]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.factorial
    cmp r15, r14
    ja baz_frame_overflow
;   [prog.baz:323:13] free scratch register r14
;   [prog.baz:323:13] free scratch register r15
;   [prog.baz:323:13] frame capacity check end
;   [prog.baz:323:13] result address in callee frame
;   [prog.baz:323:13] allocate scratch register -> r15
    lea r15, [rbp + 584]
    mov qword [rbp + 592], r15
;   [prog.baz:323:13] free scratch register r15
;   [prog.baz:323:13] address of argument 'five' to parameter 'n'
;   [prog.baz:323:13] allocate scratch register -> r15
    lea r15, [rbp + 576]
    mov qword [rbp + 600], r15
;   [prog.baz:323:13] free scratch register r15
;   [prog.baz:323:13] set function frame base
    lea rbx, [rbp + 592]
    call func.factorial
;   [prog.baz:324:5] assert(f == 120)
;   [prog.baz:324:12] allocate scratch register -> r15
;   [prog.baz:324:12] ? f == 120
    cmp.324.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.324.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.324.5:
;       [prog.baz:324:5] alias ok -> r15b
        if.38.27.324.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.324.5:
        cmp r15b, 0
        jne if.38.24.324.5.end
        if.38.27.324.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.324.5.end:
;       [prog.baz:324:5] free scratch register r15
    func.assert.324.5.end:
;   [prog.baz:326:5] var p0 = point{baz(3), 0}
;   [prog.baz:326:9] p0: point (16 B @ [rbp + 592])
;   [prog.baz:326:9] p0 = point{baz(3), 0}
;   [prog.baz:326:20] copy field 'x'
;   [prog.baz:326:20] p0.x = baz(3)
;   [prog.baz:326:20] = expression
;   [prog.baz:326:20] baz(3)
;   [prog.baz:66:6] baz(arg) res
    func.baz.326.20:
;       [prog.baz:326:20] alias res -> p0.x (lea: rbp + 592)
;       [prog.baz:326:20] alias arg -> 3
;       [prog.baz:67:5] res = arg * 2
;       [prog.baz:67:11] res = 6
;       [prog.baz:67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 592], 6
    func.baz.326.20.end:
;   [prog.baz:326:28] copy field 'y'
;   [prog.baz:326:28] p0.y = 0
;   [prog.baz:326:28] src: folded constant '0'
    mov qword [rbp + 600], 0
;   [prog.baz:327:5] assert(p0.x == 6)
;   [prog.baz:327:12] allocate scratch register -> r15
;   [prog.baz:327:12] ? p0.x == 6
    cmp.327.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.327.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.327.5:
;       [prog.baz:327:5] alias ok -> r15b
        if.38.27.327.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.327.5:
        cmp r15b, 0
        jne if.38.24.327.5.end
        if.38.27.327.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.327.5.end:
;       [prog.baz:327:5] free scratch register r15
    func.assert.327.5.end:
;   [prog.baz:329:5] var pt = point.at(-1, -2)
;   [prog.baz:329:9] pt: point (16 B @ [rbp + 608])
;   [prog.baz:329:9] pt = point.at(-1, -2)
;   [prog.baz:329:14] point.at(-1, -2)
;   [prog.baz:108:6] point.at(x, y) self
    func.point.at.329.14:
;       [prog.baz:329:14] alias self -> pt
;       [prog.baz:329:14] alias x -> -1
;       [prog.baz:329:14] alias y -> -2
;       [prog.baz:109:5] self.x = x
;       [prog.baz:109:14] self.x = -1
;       [prog.baz:109:14] src: folded constant 'x'
        mov qword [rbp + 608], -1
;       [prog.baz:110:5] self.y = y
;       [prog.baz:110:14] self.y = -2
;       [prog.baz:110:14] src: folded constant 'y'
        mov qword [rbp + 616], -2
    func.point.at.329.14.end:
;   [prog.baz:333:5] assert(pt.x == -1)
;   [prog.baz:333:12] allocate scratch register -> r15
;   [prog.baz:333:12] ? pt.x == -1
    cmp.333.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.333.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.333.5:
;       [prog.baz:333:5] alias ok -> r15b
        if.38.27.333.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.333.5:
        cmp r15b, 0
        jne if.38.24.333.5.end
        if.38.27.333.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.333.5.end:
;       [prog.baz:333:5] free scratch register r15
    func.assert.333.5.end:
;   [prog.baz:334:5] assert(pt.y == -2)
;   [prog.baz:334:12] allocate scratch register -> r15
;   [prog.baz:334:12] ? pt.y == -2
    cmp.334.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.334.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.334.5:
;       [prog.baz:334:5] alias ok -> r15b
        if.38.27.334.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.334.5:
        cmp r15b, 0
        jne if.38.24.334.5.end
        if.38.27.334.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.334.5.end:
;       [prog.baz:334:5] free scratch register r15
    func.assert.334.5.end:
;   [prog.baz:336:8] pt.x(2)
;   [prog.baz:114:10] mut point.x(x)
    func.point.x.336.8:
;       [prog.baz:336:8] alias self -> pt
;       [prog.baz:336:8] alias x -> 2
;       [prog.baz:115:5] self.x = x
;       [prog.baz:115:14] self.x = 2
;       [prog.baz:115:14] src: folded constant 'x'
        mov qword [rbp + 608], 2
    func.point.x.336.8.end:
;   [prog.baz:337:5] assert(pt.x == 2)
;   [prog.baz:337:12] allocate scratch register -> r15
;   [prog.baz:337:12] ? pt.x == 2
    cmp.337.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.337.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.337.5:
;       [prog.baz:337:5] alias ok -> r15b
        if.38.27.337.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.337.5:
        cmp r15b, 0
        jne if.38.24.337.5.end
        if.38.27.337.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.337.5.end:
;       [prog.baz:337:5] free scratch register r15
    func.assert.337.5.end:
;   [prog.baz:338:5] assert(pt.sum() == 0)
;   [prog.baz:338:12] allocate scratch register -> r15
;   [prog.baz:338:12] ? pt.sum() == 0
    cmp.338.12:
;   [prog.baz:338:12] allocate scratch register -> r14
;       [prog.baz:338:15] r14 = pt.sum()
;       [prog.baz:338:15] = expression
;       [prog.baz:338:15] pt.sum()
;       [prog.baz:54:6] point.sum() res
        func.point.sum.338.15:
;           [prog.baz:338:15] alias res -> r14
;           [prog.baz:338:15] alias self -> pt
;           [prog.baz:55:5] res = self.x + self.y
;           [prog.baz:55:11] self.x
            mov r14, qword [rbp + 608]
;           [prog.baz:55:20] res + self.y
;           [prog.baz:55:20] src: operand
            add r14, qword [rbp + 616]
            jo baz_overflow_line_55
        func.point.sum.338.15.end:
    cmp r14, 0
;   [prog.baz:338:12] free scratch register r14
    sete r15b
    bool.338.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.338.5:
;       [prog.baz:338:5] alias ok -> r15b
        if.38.27.338.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.338.5:
        cmp r15b, 0
        jne if.38.24.338.5.end
        if.38.27.338.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.338.5.end:
;       [prog.baz:338:5] free scratch register r15
    func.assert.338.5.end:
;   [prog.baz:340:5] var x = 1
;   [prog.baz:340:9] x: i64 (8 B @ [rbp + 624])
;   [prog.baz:340:9] x = 1
;   [prog.baz:340:13] x = 1
;   [prog.baz:340:13] src: folded constant '1'
    mov qword [rbp + 624], 1
;   [prog.baz:341:5] var y = 2
;   [prog.baz:341:9] y: i64 (8 B @ [rbp + 632])
;   [prog.baz:341:9] y = 2
;   [prog.baz:341:13] y = 2
;   [prog.baz:341:13] src: folded constant '2'
    mov qword [rbp + 632], 2
;   [prog.baz:343:5] var o1 = object{{x * 10, y}, 0xff0000}
;   [prog.baz:343:9] o1: object (24 B @ [rbp + 640])
;   [prog.baz:343:9] o1 = object{{x * 10, y}, 0xff0000}
;   [prog.baz:343:21] copy field 'pos'
;   [prog.baz:343:22] copy field 'x'
;   [prog.baz:343:22] smaller of two versions of 6 and 4 instructions
;   [prog.baz:343:22] allocate scratch register -> r15
;   [prog.baz:343:22] x
    mov r15, qword [rbp + 624]
;   [prog.baz:343:22] r15 * 10
;   [prog.baz:343:22] src: folded constant '* 10'
    imul r15, 10
    jo baz_overflow_line_343
    mov qword [rbp + 640], r15
;   [prog.baz:343:22] free scratch register r15
;   [prog.baz:343:30] copy field 'y'
;   [prog.baz:343:30] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
;   [prog.baz:343:30] free scratch register r15
;   [prog.baz:343:34] copy field 'color'
;   [prog.baz:343:34] o1.color = 16711680
;   [prog.baz:343:34] src: folded constant '0xff0000'
    mov dword [rbp + 656], 16711680
;   [prog.baz:343:14] zero padding: 4 B
;   [prog.baz:343:14] size <= 32 B, use mov
    mov dword [rbp + 660], 0
;   [prog.baz:344:5] assert(o1.pos.x == 10)
;   [prog.baz:344:12] allocate scratch register -> r15
;   [prog.baz:344:12] ? o1.pos.x == 10
    cmp.344.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.344.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.344.5:
;       [prog.baz:344:5] alias ok -> r15b
        if.38.27.344.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.344.5:
        cmp r15b, 0
        jne if.38.24.344.5.end
        if.38.27.344.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.344.5.end:
;       [prog.baz:344:5] free scratch register r15
    func.assert.344.5.end:
;   [prog.baz:345:5] assert(o1.pos.y == 2)
;   [prog.baz:345:12] allocate scratch register -> r15
;   [prog.baz:345:12] ? o1.pos.y == 2
    cmp.345.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.345.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.345.5:
;       [prog.baz:345:5] alias ok -> r15b
        if.38.27.345.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.345.5:
        cmp r15b, 0
        jne if.38.24.345.5.end
        if.38.27.345.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.345.5.end:
;       [prog.baz:345:5] free scratch register r15
    func.assert.345.5.end:
;   [prog.baz:346:5] assert(o1.color == 0xff0000)
;   [prog.baz:346:12] allocate scratch register -> r15
;   [prog.baz:346:12] ? o1.color == 0xff0000
    cmp.346.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.346.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.346.5:
;       [prog.baz:346:5] alias ok -> r15b
        if.38.27.346.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.346.5:
        cmp r15b, 0
        jne if.38.24.346.5.end
        if.38.27.346.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.346.5.end:
;       [prog.baz:346:5] free scratch register r15
    func.assert.346.5.end:
;   [prog.baz:348:5] var p1 = point{-x, -y}
;   [prog.baz:348:9] p1: point (16 B @ [rbp + 664])
;   [prog.baz:348:9] p1 = point{-x, -y}
;   [prog.baz:348:20] copy field 'x'
;   [prog.baz:348:20] smaller of two versions of 4 and 4 instructions
;   [prog.baz:348:20] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
;   [prog.baz:348:20] free scratch register r15
    neg qword [rbp + 664]
    jo baz_overflow_line_348
;   [prog.baz:348:24] copy field 'y'
;   [prog.baz:348:24] smaller of two versions of 4 and 4 instructions
;   [prog.baz:348:24] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
;   [prog.baz:348:24] free scratch register r15
    neg qword [rbp + 672]
    jo baz_overflow_line_348
;   [prog.baz:349:5] o1.pos = p1
;   [prog.baz:349:14] size <= 16 B, use mov
;   [prog.baz:349:14] allocate scratch register -> r15
    mov r15, qword [rbp + 664]
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 672]
    mov qword [rbp + 648], r15
;   [prog.baz:349:14] free scratch register r15
;   [prog.baz:350:5] assert(o1.pos.x == -1)
;   [prog.baz:350:12] allocate scratch register -> r15
;   [prog.baz:350:12] ? o1.pos.x == -1
    cmp.350.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.350.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.350.5:
;       [prog.baz:350:5] alias ok -> r15b
        if.38.27.350.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.350.5:
        cmp r15b, 0
        jne if.38.24.350.5.end
        if.38.27.350.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.350.5.end:
;       [prog.baz:350:5] free scratch register r15
    func.assert.350.5.end:
;   [prog.baz:351:5] assert(o1.pos.y == -2)
;   [prog.baz:351:12] allocate scratch register -> r15
;   [prog.baz:351:12] ? o1.pos.y == -2
    cmp.351.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.351.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.351.5:
;       [prog.baz:351:5] alias ok -> r15b
        if.38.27.351.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.351.5:
        cmp r15b, 0
        jne if.38.24.351.5.end
        if.38.27.351.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.351.5.end:
;       [prog.baz:351:5] free scratch register r15
    func.assert.351.5.end:
;   [prog.baz:353:5] var o2 = o1
;   [prog.baz:353:9] o2: object (24 B @ [rbp + 680])
;   [prog.baz:353:9] o2 = o1
;   [prog.baz:353:14] allocate named register rsi
;   [prog.baz:353:14] allocate named register rdi
;   [prog.baz:353:14] allocate named register rcx
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
;   [prog.baz:353:14] free named register rcx
;   [prog.baz:353:14] free named register rdi
;   [prog.baz:353:14] free named register rsi
;   [prog.baz:354:5] assert(o2.pos.x == -1)
;   [prog.baz:354:12] allocate scratch register -> r15
;   [prog.baz:354:12] ? o2.pos.x == -1
    cmp.354.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.354.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.354.5:
;       [prog.baz:354:5] alias ok -> r15b
        if.38.27.354.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.354.5:
        cmp r15b, 0
        jne if.38.24.354.5.end
        if.38.27.354.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.354.5.end:
;       [prog.baz:354:5] free scratch register r15
    func.assert.354.5.end:
;   [prog.baz:355:5] assert(o2.pos.y == -2)
;   [prog.baz:355:12] allocate scratch register -> r15
;   [prog.baz:355:12] ? o2.pos.y == -2
    cmp.355.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.355.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.355.5:
;       [prog.baz:355:5] alias ok -> r15b
        if.38.27.355.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.355.5:
        cmp r15b, 0
        jne if.38.24.355.5.end
        if.38.27.355.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.355.5.end:
;       [prog.baz:355:5] free scratch register r15
    func.assert.355.5.end:
;   [prog.baz:356:5] assert(o2.color == 0xff0000)
;   [prog.baz:356:12] allocate scratch register -> r15
;   [prog.baz:356:12] ? o2.color == 0xff0000
    cmp.356.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.356.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.356.5:
;       [prog.baz:356:5] alias ok -> r15b
        if.38.27.356.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.356.5:
        cmp r15b, 0
        jne if.38.24.356.5.end
        if.38.27.356.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.356.5.end:
;       [prog.baz:356:5] free scratch register r15
    func.assert.356.5.end:
;   [prog.baz:358:5] o2.pos = {x, y}
;   [prog.baz:358:15] copy field 'x'
;   [prog.baz:358:15] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
;   [prog.baz:358:15] free scratch register r15
;   [prog.baz:358:18] copy field 'y'
;   [prog.baz:358:18] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
;   [prog.baz:358:18] free scratch register r15
;   [prog.baz:359:5] assert(o2.pos.x == 1)
;   [prog.baz:359:12] allocate scratch register -> r15
;   [prog.baz:359:12] ? o2.pos.x == 1
    cmp.359.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.359.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.359.5:
;       [prog.baz:359:5] alias ok -> r15b
        if.38.27.359.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.359.5:
        cmp r15b, 0
        jne if.38.24.359.5.end
        if.38.27.359.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.359.5.end:
;       [prog.baz:359:5] free scratch register r15
    func.assert.359.5.end:
;   [prog.baz:360:5] o2.pos = {y, x}
;   [prog.baz:360:15] copy field 'x'
;   [prog.baz:360:15] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
;   [prog.baz:360:15] free scratch register r15
;   [prog.baz:360:18] copy field 'y'
;   [prog.baz:360:18] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
;   [prog.baz:360:18] free scratch register r15
;   [prog.baz:361:5] assert(o2.pos.x == 2)
;   [prog.baz:361:12] allocate scratch register -> r15
;   [prog.baz:361:12] ? o2.pos.x == 2
    cmp.361.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.361.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.361.5:
;       [prog.baz:361:5] alias ok -> r15b
        if.38.27.361.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.361.5:
        cmp r15b, 0
        jne if.38.24.361.5.end
        if.38.27.361.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.361.5.end:
;       [prog.baz:361:5] free scratch register r15
    func.assert.361.5.end:
;   [prog.baz:368:5] var o3 = object[2]
;   [prog.baz:368:9] o3: object[2] (48 B @ [rbp + 704])
;   [prog.baz:368:9] o3 = object[2]
;   [prog.baz:368:14] zero remaining elements: 2 * 24 B = 48 B
;   [prog.baz:368:14] allocate named register rax
;   [prog.baz:368:14] allocate named register rdi
;   [prog.baz:368:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
;   [prog.baz:368:14] free named register rcx
;   [prog.baz:368:14] free named register rdi
;   [prog.baz:368:14] free named register rax
;   [prog.baz:369:5] o3[0].pos.y = 73
;   [prog.baz:369:19] o3.pos.y = 73
;   [prog.baz:369:19] src: folded constant '73'
    mov qword [rbp + 712], 73
;   [prog.baz:371:5] assert(o3[0].pos.y == 73)
;   [prog.baz:371:12] allocate scratch register -> r15
;   [prog.baz:371:12] ? o3[0].pos.y == 73
    cmp.371.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.371.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.371.5:
;       [prog.baz:371:5] alias ok -> r15b
        if.38.27.371.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.371.5:
        cmp r15b, 0
        jne if.38.24.371.5.end
        if.38.27.371.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.371.5.end:
;       [prog.baz:371:5] free scratch register r15
    func.assert.371.5.end:
;   [prog.baz:372:5] o3[1] = object.at(2, 74, 0xffffff)
;   [prog.baz:372:13] zero padding: 4 B
;   [prog.baz:372:13] size <= 32 B, use mov
    mov dword [rbp + 748], 0
;   [prog.baz:372:13] object.at(2, 74, 0xffffff)
;   [prog.baz:119:6] object.at(x, y, color i32) self
    func.object.at.372.13:
;       [prog.baz:372:13] alias self -> o3 (lea: rbp + 728)
;       [prog.baz:372:13] alias x -> 2
;       [prog.baz:372:13] alias y -> 74
;       [prog.baz:372:13] alias color -> 16777215
;       [prog.baz:120:5] self.pos = point.at(x, y)
;       [prog.baz:120:16] point.at(x, y)
;       [prog.baz:108:6] point.at(x, y) self
        func.point.at.120.16.372.13:
;           [prog.baz:120:16] alias self -> self.pos (lea: rbp + 728)
;           [prog.baz:120:16] alias x -> 2
;           [prog.baz:120:16] alias y -> 74
;           [prog.baz:109:5] self.x = x
;           [prog.baz:109:14] self.x = 2
;           [prog.baz:109:14] src: folded constant 'x'
            mov qword [rbp + 728], 2
;           [prog.baz:110:5] self.y = y
;           [prog.baz:110:14] self.y = 74
;           [prog.baz:110:14] src: folded constant 'y'
            mov qword [rbp + 736], 74
        func.point.at.120.16.372.13.end:
;       [prog.baz:121:5] self.color = color
;       [prog.baz:121:18] self.color = 16777215
;       [prog.baz:121:18] src: folded constant 'color'
        mov dword [rbp + 744], 16777215
    func.object.at.372.13.end:
;   [prog.baz:373:5] assert(o3[1].pos.y == 74)
;   [prog.baz:373:12] allocate scratch register -> r15
;   [prog.baz:373:12] ? o3[1].pos.y == 74
    cmp.373.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.373.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.373.5:
;       [prog.baz:373:5] alias ok -> r15b
        if.38.27.373.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.373.5:
        cmp r15b, 0
        jne if.38.24.373.5.end
        if.38.27.373.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.373.5.end:
;       [prog.baz:373:5] free scratch register r15
    func.assert.373.5.end:
;   [prog.baz:375:15] o3[1].pos.fooz()
;   [prog.baz:47:10] mut point.fooz()
    func.point.fooz.375.15:
;       [prog.baz:375:15] alias self -> o3.pos (lea: rbp + 728)
;       [prog.baz:48:5] self.x = 0b10
;       [prog.baz:48:14] self.x = 2
;       [prog.baz:48:14] src: folded constant '0b10'
        mov qword [rbp + 728], 2
;       [prog.baz:49:5] self.y = 0xb
;       [prog.baz:49:14] self.y = 11
;       [prog.baz:49:14] src: folded constant '0xb'
        mov qword [rbp + 736], 11
    func.point.fooz.375.15.end:
;   [prog.baz:376:5] assert(o3[1].pos.sum() == 13)
;   [prog.baz:376:12] allocate scratch register -> r15
;   [prog.baz:376:12] ? o3[1].pos.sum() == 13
    cmp.376.12:
;   [prog.baz:376:12] allocate scratch register -> r14
;       [prog.baz:376:22] r14 = o3[1].pos.sum()
;       [prog.baz:376:22] = expression
;       [prog.baz:376:22] o3[1].pos.sum()
;       [prog.baz:54:6] point.sum() res
        func.point.sum.376.22:
;           [prog.baz:376:22] alias res -> r14
;           [prog.baz:376:22] alias self -> o3.pos (lea: rbp + 728)
;           [prog.baz:55:5] res = self.x + self.y
;           [prog.baz:55:11] self.x
            mov r14, qword [rbp + 728]
;           [prog.baz:55:20] res + self.y
;           [prog.baz:55:20] src: operand
            add r14, qword [rbp + 736]
            jo baz_overflow_line_55
        func.point.sum.376.22.end:
    cmp r14, 13
;   [prog.baz:376:12] free scratch register r14
    sete r15b
    bool.376.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.376.5:
;       [prog.baz:376:5] alias ok -> r15b
        if.38.27.376.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.376.5:
        cmp r15b, 0
        jne if.38.24.376.5.end
        if.38.27.376.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.376.5.end:
;       [prog.baz:376:5] free scratch register r15
    func.assert.376.5.end:
;   [prog.baz:379:5] var worlds = world[8]
;   [prog.baz:379:9] worlds: world[8] (512 B @ [rbp + 752])
;   [prog.baz:379:9] worlds = world[8]
;   [prog.baz:379:18] zero remaining elements: 8 * 64 B = 512 B
;   [prog.baz:379:18] allocate named register rax
;   [prog.baz:379:18] allocate named register rdi
;   [prog.baz:379:18] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
;   [prog.baz:379:18] free named register rcx
;   [prog.baz:379:18] free named register rdi
;   [prog.baz:379:18] free named register rax
;   [prog.baz:380:5] worlds[1].locations[1] = 0xffee
;   [prog.baz:380:30] worlds.locations = 65518
;   [prog.baz:380:30] src: folded constant '0xffee'
    mov qword [rbp + 824], 65518
;   [prog.baz:381:5] assert(worlds[1].locations[1] == 0xffee)
;   [prog.baz:381:12] allocate scratch register -> r15
;   [prog.baz:381:12] ? worlds[1].locations[1] == 0xffee
    cmp.381.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.381.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.381.5:
;       [prog.baz:381:5] alias ok -> r15b
        if.38.27.381.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.381.5:
        cmp r15b, 0
        jne if.38.24.381.5.end
        if.38.27.381.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.381.5.end:
;       [prog.baz:381:5] free scratch register r15
    func.assert.381.5.end:
;   [prog.baz:383:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [prog.baz:383:5] allocate named register rsi
;   [prog.baz:383:5] allocate named register rdi
;   [prog.baz:383:5] allocate named register rcx
;   [prog.baz:386:9] array_length(worlds[0].locations)
;   [prog.baz:386:9] rcx = 8
;   [prog.baz:386:9] src: folded constant 'array_length(worlds[0].locations)'
    mov rcx, 8
;   [prog.baz:384:9] worlds[1].locations
;   [prog.baz:384:9] bounds check begin
;   [prog.baz:384:9] lower bound
;   [prog.baz:384:9] rcx lower bound covered by the unsigned upper bound
;   [prog.baz:384:9] upper bound
    cmp rcx, 8
    ja baz_bounds_line_384
;   [prog.baz:384:9] bounds check end
    lea rsi, [rbp + 816]
;   [prog.baz:385:9] worlds[0].locations
;   [prog.baz:385:9] bounds check begin
;   [prog.baz:385:9] lower bound
;   [prog.baz:385:9] rcx lower bound covered by the unsigned upper bound
;   [prog.baz:385:9] upper bound
    cmp rcx, 8
    ja baz_bounds_line_385
;   [prog.baz:385:9] bounds check end
    lea rdi, [rbp + 752]
    sal rcx, 3
;   [prog.baz:383:5] overlap check begin
;   [prog.baz:383:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [prog.baz:383:5] the same range is not an overlap
    je .Lbaz_overlap.2
;   [prog.baz:383:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_383
    .Lbaz_overlap.2:
;   [prog.baz:383:5] free scratch register r15
;   [prog.baz:383:5] overlap check end
    rep movsb
;   [prog.baz:383:5] free named register rcx
;   [prog.baz:383:5] free named register rdi
;   [prog.baz:383:5] free named register rsi
;   [prog.baz:390:5] assert(worlds[0].locations[1] == 0xffee)
;   [prog.baz:390:12] allocate scratch register -> r15
;   [prog.baz:390:12] ? worlds[0].locations[1] == 0xffee
    cmp.390.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.390.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.390.5:
;       [prog.baz:390:5] alias ok -> r15b
        if.38.27.390.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.390.5:
        cmp r15b, 0
        jne if.38.24.390.5.end
        if.38.27.390.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.390.5.end:
;       [prog.baz:390:5] free scratch register r15
    func.assert.390.5.end:
;   [prog.baz:391:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [prog.baz:391:12] allocate scratch register -> r15
;   [prog.baz:391:12] ? shorthand: arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
    cmp.391.12:
;       [prog.baz:391:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;       [prog.baz:391:12] allocate named register rsi
;       [prog.baz:391:12] allocate named register rdi
;       [prog.baz:391:12] allocate named register rcx
;       [prog.baz:394:14] array_length(worlds[0].locations)
;       [prog.baz:394:14] rcx = 8
;       [prog.baz:394:14] src: folded constant 'array_length(worlds[0].locations)'
        mov rcx, 8
;       [prog.baz:392:14] worlds[0].locations
;       [prog.baz:392:14] bounds check begin
;       [prog.baz:392:14] lower bound
;       [prog.baz:392:14] rcx lower bound covered by the unsigned upper bound
;       [prog.baz:392:14] upper bound
        cmp rcx, 8
        ja baz_bounds_line_392
;       [prog.baz:392:14] bounds check end
        lea rsi, [rbp + 752]
;       [prog.baz:393:14] worlds[1].locations
;       [prog.baz:393:14] bounds check begin
;       [prog.baz:393:14] lower bound
;       [prog.baz:393:14] rcx lower bound covered by the unsigned upper bound
;       [prog.baz:393:14] upper bound
        cmp rcx, 8
        ja baz_bounds_line_393
;       [prog.baz:393:14] bounds check end
        lea rdi, [rbp + 816]
        sal rcx, 3
        test rcx, rcx
        repe cmpsb
;       [prog.baz:391:12] free named register rcx
;       [prog.baz:391:12] free named register rdi
;       [prog.baz:391:12] free named register rsi
        sete r15b
    bool.391.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.391.5:
;       [prog.baz:391:5] alias ok -> r15b
        if.38.27.391.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.391.5:
        cmp r15b, 0
        jne if.38.24.391.5.end
        if.38.27.391.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.391.5.end:
;       [prog.baz:391:5] free scratch register r15
    func.assert.391.5.end:
;   [prog.baz:397:5] var arr2 = []{ -1, 2 }
;   [prog.baz:397:9] arr2: i64[2] (16 B @ [rbp + 1264])
;   [prog.baz:397:9] arr2 = []{ -1, 2 }
;   [prog.baz:397:18] size <= 16 B, use immediates
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
;   [prog.baz:398:5] assert(array_length(arr2) == 2)
;   [prog.baz:38:6] assert(ok bool)
    func.assert.398.5:
;       [prog.baz:398:5] alias ok -> 1
        if.38.27.398.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.398.5:
;       [prog.baz:38:31] const eval to false
        if.38.24.398.5.end:
    func.assert.398.5.end:
;   [prog.baz:399:5] assert(arr2[0] == -1)
;   [prog.baz:399:12] allocate scratch register -> r15
;   [prog.baz:399:12] ? arr2[0] == -1
    cmp.399.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.399.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.399.5:
;       [prog.baz:399:5] alias ok -> r15b
        if.38.27.399.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.399.5:
        cmp r15b, 0
        jne if.38.24.399.5.end
        if.38.27.399.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.399.5.end:
;       [prog.baz:399:5] free scratch register r15
    func.assert.399.5.end:
;   [prog.baz:400:5] assert(arr2[1] == 2)
;   [prog.baz:400:12] allocate scratch register -> r15
;   [prog.baz:400:12] ? arr2[1] == 2
    cmp.400.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.400.12.end:
;   [prog.baz:38:6] assert(ok bool)
    func.assert.400.5:
;       [prog.baz:400:5] alias ok -> r15b
        if.38.27.400.5:
;       [prog.baz:38:27] ? shorthand: not ok
        cmp.38.27.400.5:
        cmp r15b, 0
        jne if.38.24.400.5.end
        if.38.27.400.5.code:
;           [prog.baz:38:34] exit(1)
;           [prog.baz:38:34] allocate named register rdi
;           [prog.baz:38:39] rdi = 1
;           [prog.baz:38:39] src: folded constant '1'
            mov rdi, 1
            mov rax, 60
            syscall
;           [prog.baz:38:34] free named register rdi
        if.38.24.400.5.end:
;       [prog.baz:400:5] free scratch register r15
    func.assert.400.5.end:
;   [prog.baz:402:5] var counter = 0
;   [prog.baz:402:9] counter: i64 (8 B @ [rbp + 1280])
;   [prog.baz:402:9] counter = 0
;   [prog.baz:402:19] counter = 0
;   [prog.baz:402:19] src: folded constant '0'
    mov qword [rbp + 1280], 0
;   [prog.baz:403:5] var nm = str
;   [prog.baz:403:9] nm: str (128 B @ [rbp + 1288])
;   [prog.baz:403:9] nm = str
;   [prog.baz:403:14] zero remaining fields: 128 B
;   [prog.baz:403:14] allocate named register rax
;   [prog.baz:403:14] allocate named register rdi
;   [prog.baz:403:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
;   [prog.baz:403:14] free named register rcx
;   [prog.baz:403:14] free named register rdi
;   [prog.baz:403:14] free named register rax
;   [prog.baz:404:5] print(hello)
;   [prog.baz:41:6] print(str i8[])
    func.print.404.5:
;       [prog.baz:404:5] alias str -> hello
;       [prog.baz:42:5] write(1, str)
;       [prog.baz:42:5] allocate named register rdi
;       [prog.baz:42:5] allocate named register rsi
;       [prog.baz:42:5] allocate named register rdx
;       [prog.baz:42:11] rdi = 1
;       [prog.baz:42:11] src: folded constant '1'
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
;       [prog.baz:42:5] allocate named register rax
        mov rax, 1
        syscall
;       [prog.baz:42:5] free named register rax
;       [prog.baz:42:5] free named register rdx
;       [prog.baz:42:5] free named register rsi
;       [prog.baz:42:5] free named register rdi
    func.print.404.5.end:
;   [prog.baz:405:5] loop
    loop.405.5:
;       [prog.baz:406:9] counter = counter + 1
;       [prog.baz:406:19] smaller of two versions of 2 and 4 instructions
;       [prog.baz:406:19] counter
;       [prog.baz:406:19] counter + 1
;       [prog.baz:406:19] src: folded constant '+ 1'
        add qword [rbp + 1280], 1
        jo baz_overflow_line_406
;       [prog.baz:407:9] print_num(counter)
;       [prog.baz:407:9] frame capacity check begin
;       [prog.baz:407:9] allocate scratch register -> r15
;       [prog.baz:407:9] allocate scratch register -> r14
        lea r15, [rbp + 1416]
        lea r14, [vars]
        cmp r15, r14
        jb baz_frame_overflow
        mov r14, strict qword vars.end
        cmp r15, r14
        ja baz_frame_overflow
        sub r14, r15
        mov r15, size.func.print_num
        cmp r15, r14
        ja baz_frame_overflow
;       [prog.baz:407:9] free scratch register r14
;       [prog.baz:407:9] free scratch register r15
;       [prog.baz:407:9] frame capacity check end
;       [prog.baz:407:9] address of argument 'counter' to parameter 'num'
;       [prog.baz:407:9] allocate scratch register -> r15
        lea r15, [rbp + 1280]
        mov qword [rbp + 1416], r15
;       [prog.baz:407:9] free scratch register r15
;       [prog.baz:407:9] set function frame base
        lea rbx, [rbp + 1416]
        call func.print_num
;       [prog.baz:408:9] print(colon)
;       [prog.baz:41:6] print(str i8[])
        func.print.408.9:
;           [prog.baz:408:9] alias str -> colon
;           [prog.baz:42:5] write(1, str)
;           [prog.baz:42:5] allocate named register rdi
;           [prog.baz:42:5] allocate named register rsi
;           [prog.baz:42:5] allocate named register rdx
;           [prog.baz:42:11] rdi = 1
;           [prog.baz:42:11] src: folded constant '1'
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
;           [prog.baz:42:5] allocate named register rax
            mov rax, 1
            syscall
;           [prog.baz:42:5] free named register rax
;           [prog.baz:42:5] free named register rdx
;           [prog.baz:42:5] free named register rsi
;           [prog.baz:42:5] free named register rdi
        func.print.408.9.end:
;       [prog.baz:409:9] print(prompt1)
;       [prog.baz:41:6] print(str i8[])
        func.print.409.9:
;           [prog.baz:409:9] alias str -> prompt1
;           [prog.baz:42:5] write(1, str)
;           [prog.baz:42:5] allocate named register rdi
;           [prog.baz:42:5] allocate named register rsi
;           [prog.baz:42:5] allocate named register rdx
;           [prog.baz:42:11] rdi = 1
;           [prog.baz:42:11] src: folded constant '1'
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
;           [prog.baz:42:5] allocate named register rax
            mov rax, 1
            syscall
;           [prog.baz:42:5] free named register rax
;           [prog.baz:42:5] free named register rdx
;           [prog.baz:42:5] free named register rsi
;           [prog.baz:42:5] free named register rdi
        func.print.409.9.end:
;       [prog.baz:410:12] nm.input()
;       [prog.baz:88:10] mut str.input()
        func.str.input.410.12:
;           [prog.baz:410:12] alias self -> nm
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
            mov rdx, 127
            lea rsi, [rbp + 1289]
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
            sub r15, 1
            mov byte [rbp + 1288], r15b
;           [prog.baz:92:16] free scratch register r15
        func.str.input.410.12.end:
        if.412.12:
;       [prog.baz:412:12] ? nm.len <= 0
        cmp.412.12:
        cmp byte [rbp + 1288], 0
        jle loop.405.5.end
        if.412.12.code:
;           [prog.baz:413:13] break
        if.414.19:
;       [prog.baz:414:19] ? nm.len <= 4
        cmp.414.19:
        cmp byte [rbp + 1288], 4
        jg if.412.9.else
        if.414.19.code:
;           [prog.baz:415:13] print(prompt2)
;           [prog.baz:41:6] print(str i8[])
            func.print.415.13:
;               [prog.baz:415:13] alias str -> prompt2
;               [prog.baz:42:5] write(1, str)
;               [prog.baz:42:5] allocate named register rdi
;               [prog.baz:42:5] allocate named register rsi
;               [prog.baz:42:5] allocate named register rdx
;               [prog.baz:42:11] rdi = 1
;               [prog.baz:42:11] src: folded constant '1'
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
;               [prog.baz:42:5] allocate named register rax
                mov rax, 1
                syscall
;               [prog.baz:42:5] free named register rax
;               [prog.baz:42:5] free named register rdx
;               [prog.baz:42:5] free named register rsi
;               [prog.baz:42:5] free named register rdi
            func.print.415.13.end:
;           [prog.baz:416:13] continue
            jmp loop.405.5
        if.412.9.else:
;           [prog.baz:418:13] greet(nm)
;           [prog.baz:99:6] greet(name str)
            func.greet.418.13:
;               [prog.baz:418:13] alias name -> nm
;               [prog.baz:100:5] print(prompt3)
;               [prog.baz:41:6] print(str i8[])
                func.print.100.5.418.13:
;                   [prog.baz:100:5] alias str -> prompt3
;                   [prog.baz:42:5] write(1, str)
;                   [prog.baz:42:5] allocate named register rdi
;                   [prog.baz:42:5] allocate named register rsi
;                   [prog.baz:42:5] allocate named register rdx
;                   [prog.baz:42:11] rdi = 1
;                   [prog.baz:42:11] src: folded constant '1'
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
;                   [prog.baz:42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:42:5] free named register rax
;                   [prog.baz:42:5] free named register rdx
;                   [prog.baz:42:5] free named register rsi
;                   [prog.baz:42:5] free named register rdi
                func.print.100.5.418.13.end:
;               [prog.baz:101:10] name.print()
;               [prog.baz:95:6] str.print()
                func.str.print.101.10.418.13:
;                   [prog.baz:101:10] alias self -> name
;                   [prog.baz:96:5] write(1, self.data, self.len)
;                   [prog.baz:96:5] allocate named register rdi
;                   [prog.baz:96:5] allocate named register rsi
;                   [prog.baz:96:5] allocate named register rdx
;                   [prog.baz:96:11] rdi = 1
;                   [prog.baz:96:11] src: folded constant '1'
                    mov rdi, 1
;                   [prog.baz:96:25] self.len
                    movsx rdx, byte [rbp + 1288]
;                   [prog.baz:96:14] bounds check begin
;                   [prog.baz:96:14] lower bound
;                   [prog.baz:96:14] rdx lower bound covered by the unsigned upper bound
;                   [prog.baz:96:14] upper bound
                    cmp rdx, 127
                    ja baz_bounds_line_96
;                   [prog.baz:96:14] bounds check end
                    lea rsi, [rbp + 1289]
;                   [prog.baz:96:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:96:5] free named register rax
;                   [prog.baz:96:5] free named register rdx
;                   [prog.baz:96:5] free named register rsi
;                   [prog.baz:96:5] free named register rdi
                func.str.print.101.10.418.13.end:
;               [prog.baz:102:5] print(dot)
;               [prog.baz:41:6] print(str i8[])
                func.print.102.5.418.13:
;                   [prog.baz:102:5] alias str -> dot
;                   [prog.baz:42:5] write(1, str)
;                   [prog.baz:42:5] allocate named register rdi
;                   [prog.baz:42:5] allocate named register rsi
;                   [prog.baz:42:5] allocate named register rdx
;                   [prog.baz:42:11] rdi = 1
;                   [prog.baz:42:11] src: folded constant '1'
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
;                   [prog.baz:42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:42:5] free named register rax
;                   [prog.baz:42:5] free named register rdx
;                   [prog.baz:42:5] free named register rsi
;                   [prog.baz:42:5] free named register rdi
                func.print.102.5.418.13.end:
;               [prog.baz:103:5] print(nl)
;               [prog.baz:41:6] print(str i8[])
                func.print.103.5.418.13:
;                   [prog.baz:103:5] alias str -> nl
;                   [prog.baz:42:5] write(1, str)
;                   [prog.baz:42:5] allocate named register rdi
;                   [prog.baz:42:5] allocate named register rsi
;                   [prog.baz:42:5] allocate named register rdx
;                   [prog.baz:42:11] rdi = 1
;                   [prog.baz:42:11] src: folded constant '1'
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
;                   [prog.baz:42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [prog.baz:42:5] free named register rax
;                   [prog.baz:42:5] free named register rdx
;                   [prog.baz:42:5] free named register rsi
;                   [prog.baz:42:5] free named register rdi
                func.print.103.5.418.13.end:
;               [prog.baz:104:5] names = names + 1
;               [prog.baz:104:13] smaller of two versions of 2 and 4 instructions
;               [prog.baz:104:13] names
;               [prog.baz:104:13] names + 1
;               [prog.baz:104:13] src: folded constant '+ 1'
                add qword [rbp + 368], 1
                jo baz_overflow_line_104
            func.greet.418.13.end:
        if.412.9.end:
    jmp loop.405.5
    loop.405.5.end:
;   [prog.baz:422:5] print(greeted)
;   [prog.baz:41:6] print(str i8[])
    func.print.422.5:
;       [prog.baz:422:5] alias str -> greeted
;       [prog.baz:42:5] write(1, str)
;       [prog.baz:42:5] allocate named register rdi
;       [prog.baz:42:5] allocate named register rsi
;       [prog.baz:42:5] allocate named register rdx
;       [prog.baz:42:11] rdi = 1
;       [prog.baz:42:11] src: folded constant '1'
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
;       [prog.baz:42:5] allocate named register rax
        mov rax, 1
        syscall
;       [prog.baz:42:5] free named register rax
;       [prog.baz:42:5] free named register rdx
;       [prog.baz:42:5] free named register rsi
;       [prog.baz:42:5] free named register rdi
    func.print.422.5.end:
;   [prog.baz:423:5] print_num(names)
;   [prog.baz:423:5] frame capacity check begin
;   [prog.baz:423:5] allocate scratch register -> r15
;   [prog.baz:423:5] allocate scratch register -> r14
    lea r15, [rbp + 1416]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.print_num
    cmp r15, r14
    ja baz_frame_overflow
;   [prog.baz:423:5] free scratch register r14
;   [prog.baz:423:5] free scratch register r15
;   [prog.baz:423:5] frame capacity check end
;   [prog.baz:423:5] address of argument 'names' to parameter 'num'
;   [prog.baz:423:5] allocate scratch register -> r15
    lea r15, [rbp + 368]
    mov qword [rbp + 1416], r15
;   [prog.baz:423:5] free scratch register r15
;   [prog.baz:423:5] set function frame base
    lea rbx, [rbp + 1416]
    call func.print_num
;   [prog.baz:424:5] print(nl)
;   [prog.baz:41:6] print(str i8[])
    func.print.424.5:
;       [prog.baz:424:5] alias str -> nl
;       [prog.baz:42:5] write(1, str)
;       [prog.baz:42:5] allocate named register rdi
;       [prog.baz:42:5] allocate named register rsi
;       [prog.baz:42:5] allocate named register rdx
;       [prog.baz:42:11] rdi = 1
;       [prog.baz:42:11] src: folded constant '1'
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
;       [prog.baz:42:5] allocate named register rax
        mov rax, 1
        syscall
;       [prog.baz:42:5] free named register rax
;       [prog.baz:42:5] free named register rdx
;       [prog.baz:42:5] free named register rsi
;       [prog.baz:42:5] free named register rdi
    func.print.424.5.end:
;   [prog.baz:426:5] var bye = "bye from baz\n"
;   [prog.baz:426:9] bye: i8[13] (13 B @ [rbp + 1416])
;   [prog.baz:426:9] bye = "bye from baz\n"
;   [prog.baz:426:15] size <= 16 B, use immediates
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
;   [prog.baz:427:5] write(1, bye, 3)
;   [prog.baz:427:5] allocate named register rdi
;   [prog.baz:427:5] allocate named register rsi
;   [prog.baz:427:5] allocate named register rdx
;   [prog.baz:427:11] rdi = 1
;   [prog.baz:427:11] src: folded constant '1'
    mov rdi, 1
;   [prog.baz:427:19] rdx = 3
;   [prog.baz:427:19] src: folded constant '3'
    mov rdx, 3
;   [prog.baz:427:14] bounds check begin
;   [prog.baz:427:14] lower bound
;   [prog.baz:427:14] rdx lower bound covered by the unsigned upper bound
;   [prog.baz:427:14] upper bound
    cmp rdx, 13
    ja baz_bounds_line_427
;   [prog.baz:427:14] bounds check end
    lea rsi, [rbp + 1416]
;   [prog.baz:427:5] allocate named register rax
    mov rax, 1
    syscall
;   [prog.baz:427:5] free named register rax
;   [prog.baz:427:5] free named register rdx
;   [prog.baz:427:5] free named register rsi
;   [prog.baz:427:5] free named register rdi
;   [prog.baz:428:5] write(1, bye, 1, array_length(bye) - 1)
;   [prog.baz:428:5] allocate named register rdi
;   [prog.baz:428:5] allocate named register rsi
;   [prog.baz:428:5] allocate named register rdx
;   [prog.baz:428:11] rdi = 1
;   [prog.baz:428:11] src: folded constant '1'
    mov rdi, 1
;   [prog.baz:428:19] rdx = 1
;   [prog.baz:428:19] src: folded constant '1'
    mov rdx, 1
;   [prog.baz:428:22] allocate scratch register -> r15
;   [prog.baz:428:22] r15 = 12
;   [prog.baz:428:22] src: folded constant 'array_length(bye) - 1'
    mov r15, 12
;   [prog.baz:428:22] bounds check begin
;   [prog.baz:428:22] lower bound
    test r15, r15
    js baz_bounds_line_428
    test rdx, rdx
    js baz_bounds_line_428
;   [prog.baz:428:22] upper bound
;   [prog.baz:428:22] allocate scratch register -> r14
    lea r14, [rdx + r15]
    cmp r14, 13
;   [prog.baz:428:22] free scratch register r14
    ja baz_bounds_line_428
;   [prog.baz:428:22] bounds check end
    lea rsi, [rbp + 1416]
    add rsi, r15
;   [prog.baz:428:5] free scratch register r15
;   [prog.baz:428:5] allocate named register rax
    mov rax, 1
    syscall
;   [prog.baz:428:5] free named register rax
;   [prog.baz:428:5] free named register rdx
;   [prog.baz:428:5] free named register rsi
;   [prog.baz:428:5] free named register rdi
    mov rdi, 0
    mov rax, 60
    syscall

;
;[prog.baz:175:15] noinline factorial(n) res
func.factorial:
;   [prog.baz:175:28] res: i64 (8 B @ [rbx])
;   [prog.baz:175:25] n: i64 (8 B @ [rbx + 8])
;   [prog.baz:176:5] res = 1
;   [prog.baz:176:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [prog.baz:176:11] res = 1
;   [prog.baz:176:11] src: folded constant '1'
    mov qword [r15], 1
;   [prog.baz:176:5] free scratch register r15
    if.177.8:
;   [prog.baz:177:8] ? n <= 1
    cmp.177.8:
;   [prog.baz:177:8] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
;   [prog.baz:177:8] free scratch register r15
    jg if.177.5.end
    if.177.8.code:
;       [prog.baz:177:15] return
        ret
    if.177.5.end:
;   [prog.baz:179:5] var m = n - 1
;   [prog.baz:179:9] m: i64 (8 B @ [rbx + 16])
;   [prog.baz:179:9] m = n - 1
;   [prog.baz:179:13] smaller of two versions of 5 and 5 instructions
;   [prog.baz:179:13] n
;   [prog.baz:179:13] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
;   [prog.baz:179:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
;   [prog.baz:179:13] free scratch register r14
;   [prog.baz:179:13] free scratch register r15
;   [prog.baz:179:13] m - 1
;   [prog.baz:179:13] src: folded constant '- 1'
    sub qword [rbx + 16], 1
    jo baz_overflow_line_179
;   [prog.baz:180:5] var partial = factorial(m)
;   [prog.baz:180:9] partial: i64 (8 B @ [rbx + 24])
;   [prog.baz:180:9] partial = factorial(m)
;   [prog.baz:180:19] partial = factorial(m)
;   [prog.baz:180:19] = expression
;   [prog.baz:180:19] factorial(m)
;   [prog.baz:180:19] frame capacity check begin
;   [prog.baz:180:19] allocate scratch register -> r15
;   [prog.baz:180:19] allocate scratch register -> r14
    lea r15, [rbx + 32]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.factorial
    cmp r15, r14
    ja baz_frame_overflow
;   [prog.baz:180:19] free scratch register r14
;   [prog.baz:180:19] free scratch register r15
;   [prog.baz:180:19] frame capacity check end
;   [prog.baz:180:19] result address in callee frame
;   [prog.baz:180:19] allocate scratch register -> r15
    lea r15, [rbx + 24]
    mov qword [rbx + 32], r15
;   [prog.baz:180:19] free scratch register r15
;   [prog.baz:180:19] address of argument 'm' to parameter 'n'
;   [prog.baz:180:19] allocate scratch register -> r15
    lea r15, [rbx + 16]
    mov qword [rbx + 40], r15
;   [prog.baz:180:19] free scratch register r15
;   [prog.baz:180:19] before call: save allocated registers
    push rbx
;   [prog.baz:180:19] set function frame base
    lea rbx, [rbx + 32]
    call func.factorial
;   [prog.baz:180:19] after call: restore saved registers
    pop rbx
;   [prog.baz:181:5] res = n * partial
;   [prog.baz:181:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [prog.baz:181:11] smaller of two versions of 7 and 5 instructions
;   [prog.baz:181:11] allocate scratch register -> r14
;   [prog.baz:181:11] n
;   [prog.baz:181:11] allocate scratch register -> r13
    mov r13, qword [rbx + 8]
    mov r14, qword [r13]
;   [prog.baz:181:11] free scratch register r13
;   [prog.baz:181:15] r14 * partial
;   [prog.baz:181:15] src: operand
    imul r14, qword [rbx + 24]
    jo baz_overflow_line_181
    mov qword [r15], r14
;   [prog.baz:181:11] free scratch register r14
;   [prog.baz:181:5] free scratch register r15
    ret
size.func.factorial equ 32
;
;[prog.baz:134:15] noinline print_num(num)
func.print_num:
;   [prog.baz:134:25] num: i64 (8 B @ [rbx])
;   [prog.baz:136:5] let buf_count = 20
;   [prog.baz:136:9] const buf_count = 20
;   [prog.baz:138:5] var buf = i8[buf_count]
;   [prog.baz:138:9] buf: i8[20] (20 B @ [rbx + 8])
;   [prog.baz:138:9] buf = i8[buf_count]
;   [prog.baz:138:15] zero remaining elements: 20 * 1 B = 20 B
;   [prog.baz:138:15] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [prog.baz:139:5] var n = num
;   [prog.baz:139:9] n: i64 (8 B @ [rbx + 32])
;   [prog.baz:139:9] n = num
;   [prog.baz:139:13] num
;   [prog.baz:139:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [prog.baz:139:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [prog.baz:139:13] free scratch register r14
;   [prog.baz:139:13] free scratch register r15
;   [prog.baz:140:5] var is_negative = false
;   [prog.baz:140:9] is_negative: bool (1 B @ [rbx + 40])
;   [prog.baz:140:9] is_negative = false
;   [prog.baz:140:23] is_negative = 0
;   [prog.baz:140:23] src: folded constant 'false'
    mov byte [rbx + 40], 0
    if.144.8:
;   [prog.baz:144:8] ? n < 0
    cmp.144.8:
    cmp qword [rbx + 32], 0
    jge if.144.5.end
    if.144.8.code:
;       [prog.baz:145:9] is_negative = true
;       [prog.baz:145:23] is_negative = 1
;       [prog.baz:145:23] src: folded constant 'true'
        mov byte [rbx + 40], 1
    if.144.5.end:
    if.147.8:
;   [prog.baz:147:8] ? n > 0
    cmp.147.8:
    cmp qword [rbx + 32], 0
    jle if.147.5.end
    if.147.8.code:
;       [prog.baz:148:9] n = -n
;       [prog.baz:148:13] allocate scratch register -> r15
;       [prog.baz:148:14] -n
        mov r15, qword [rbx + 32]
        neg r15
        jo baz_overflow_line_148
        mov qword [rbx + 32], r15
;       [prog.baz:148:13] free scratch register r15
    if.147.5.end:
;   [prog.baz:151:5] var i = buf_count
;   [prog.baz:151:9] i: i64 (8 B @ [rbx + 48])
;   [prog.baz:151:9] i = buf_count
;   [prog.baz:151:13] i = 20
;   [prog.baz:151:13] src: folded constant 'buf_count'
    mov qword [rbx + 48], 20
;   [prog.baz:152:5] loop
    loop.152.5:
;       [prog.baz:153:9] i = i - 1
;       [prog.baz:153:13] smaller of two versions of 2 and 4 instructions
;       [prog.baz:153:13] i
;       [prog.baz:153:13] i - 1
;       [prog.baz:153:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
        jo baz_overflow_line_153
;       [prog.baz:154:9] buf[i] = i8('0' - n % 10)
;       [prog.baz:154:13] allocate scratch register -> r15
;       [prog.baz:154:13] set array index
;       [prog.baz:154:13] i
        mov r15, qword [rbx + 48]
;       [prog.baz:154:13] bounds check begin
;       [prog.baz:154:13] lower bound
;       [prog.baz:154:13] r15 lower bound covered by the unsigned upper bound
;       [prog.baz:154:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_154
;       [prog.baz:154:13] bounds check end
;       [prog.baz:154:18] buf = i8('0' - n % 10)
;       [prog.baz:154:18] = expression
;       [prog.baz:154:18] allocate scratch register -> r14
;       [prog.baz:154:21] r14 = 48
;       [prog.baz:154:21] src: folded constant '+ '0''
        mov r14, 48
;       [prog.baz:154:27] r14 - n % 10
;       [prog.baz:154:27] src: expression
;       [prog.baz:154:27] allocate scratch register -> r13
;       [prog.baz:154:27] n
        mov r13, qword [rbx + 32]
;       [prog.baz:154:31] r13 % 10
;       [prog.baz:154:31] src: constant
;       [prog.baz:154:31] allocate named register rax
        mov rax, r13
;       [prog.baz:154:31] allocate named register rdx
;       [prog.baz:154:31] allocate scratch register -> r12
        mov r12, 10
;       [prog.baz:154:31] division check begin
;       [prog.baz:154:31] zero divisor
        cmp r12, 0
        je baz_division_line_154
;       [prog.baz:154:31] minimum divided by -1 overflows
        cmp r12, -1
        jne .Lbaz_division.6
        mov rdx, -9223372036854775808
        cmp rax, rdx
        je baz_division_line_154
        .Lbaz_division.6:
;       [prog.baz:154:31] division check end
        cqo
        idiv r12
;       [prog.baz:154:31] free scratch register r12
        mov r13, rdx
;       [prog.baz:154:31] free named register rdx
;       [prog.baz:154:31] free named register rax
        sub r14, r13
;       [prog.baz:154:27] free scratch register r13
        mov byte [rbx + r15 + 8], r14b
;       [prog.baz:154:18] free scratch register r14
;       [prog.baz:154:9] free scratch register r15
;       [prog.baz:155:9] n = n / 10
;       [prog.baz:155:13] smaller of two versions of 12 and 14 instructions
;       [prog.baz:155:13] n
;       [prog.baz:155:17] n / 10
;       [prog.baz:155:17] src: constant
;       [prog.baz:155:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [prog.baz:155:17] allocate named register rdx
;       [prog.baz:155:17] allocate scratch register -> r15
        mov r15, 10
;       [prog.baz:155:17] division check begin
;       [prog.baz:155:17] zero divisor
        cmp r15, 0
        je baz_division_line_155
;       [prog.baz:155:17] minimum divided by -1 overflows
        cmp r15, -1
        jne .Lbaz_division.7
        mov rdx, -9223372036854775808
        cmp rax, rdx
        je baz_division_line_155
        .Lbaz_division.7:
;       [prog.baz:155:17] division check end
        cqo
        idiv r15
;       [prog.baz:155:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [prog.baz:155:17] free named register rdx
;       [prog.baz:155:17] free named register rax
        if.156.12:
;       [prog.baz:156:12] ? n == 0
        cmp.156.12:
        cmp qword [rbx + 32], 0
        jne loop.152.5
        if.156.12.code:
;           [prog.baz:156:19] break
        if.156.9.end:
    loop.152.5.end:
    if.159.8:
;   [prog.baz:159:8] ? shorthand: is_negative
    cmp.159.8:
    cmp byte [rbx + 40], 0
    je if.159.5.end
    if.159.8.code:
;       [prog.baz:160:9] i = i - 1
;       [prog.baz:160:13] smaller of two versions of 2 and 4 instructions
;       [prog.baz:160:13] i
;       [prog.baz:160:13] i - 1
;       [prog.baz:160:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
        jo baz_overflow_line_160
;       [prog.baz:161:9] buf[i] = '-'
;       [prog.baz:161:13] allocate scratch register -> r15
;       [prog.baz:161:13] set array index
;       [prog.baz:161:13] i
        mov r15, qword [rbx + 48]
;       [prog.baz:161:13] bounds check begin
;       [prog.baz:161:13] lower bound
;       [prog.baz:161:13] r15 lower bound covered by the unsigned upper bound
;       [prog.baz:161:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_161
;       [prog.baz:161:13] bounds check end
;       [prog.baz:161:18] buf = 45
;       [prog.baz:161:18] src: folded constant ''-''
        mov byte [rbx + r15 + 8], 45
;       [prog.baz:161:9] free scratch register r15
    if.159.5.end:
;   [prog.baz:164:5] var write_pos = 0
;   [prog.baz:164:9] write_pos: i64 (8 B @ [rbx + 56])
;   [prog.baz:164:9] write_pos = 0
;   [prog.baz:164:21] write_pos = 0
;   [prog.baz:164:21] src: folded constant '0'
    mov qword [rbx + 56], 0
;   [prog.baz:165:5] loop
    loop.165.5:
;       [prog.baz:166:9] buf[write_pos] = buf[i]
;       [prog.baz:166:13] allocate scratch register -> r15
;       [prog.baz:166:13] set array index
;       [prog.baz:166:13] write_pos
        mov r15, qword [rbx + 56]
;       [prog.baz:166:13] bounds check begin
;       [prog.baz:166:13] lower bound
;       [prog.baz:166:13] r15 lower bound covered by the unsigned upper bound
;       [prog.baz:166:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_166
;       [prog.baz:166:13] bounds check end
;       [prog.baz:166:26] buf[i]
;       [prog.baz:166:30] allocate scratch register -> r14
;       [prog.baz:166:30] set array index
;       [prog.baz:166:30] i
        mov r14, qword [rbx + 48]
;       [prog.baz:166:30] bounds check begin
;       [prog.baz:166:30] lower bound
;       [prog.baz:166:30] r14 lower bound covered by the unsigned upper bound
;       [prog.baz:166:30] upper bound
        cmp r14, 20
        jae baz_bounds_line_166
;       [prog.baz:166:30] bounds check end
;       [prog.baz:166:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [prog.baz:166:26] free scratch register r13
;       [prog.baz:166:26] free scratch register r14
;       [prog.baz:166:9] free scratch register r15
;       [prog.baz:167:9] write_pos = write_pos + 1
;       [prog.baz:167:21] smaller of two versions of 2 and 4 instructions
;       [prog.baz:167:21] write_pos
;       [prog.baz:167:21] write_pos + 1
;       [prog.baz:167:21] src: folded constant '+ 1'
        add qword [rbx + 56], 1
        jo baz_overflow_line_167
;       [prog.baz:168:9] i = i + 1
;       [prog.baz:168:13] smaller of two versions of 2 and 4 instructions
;       [prog.baz:168:13] i
;       [prog.baz:168:13] i + 1
;       [prog.baz:168:13] src: folded constant '+ 1'
        add qword [rbx + 48], 1
        jo baz_overflow_line_168
        if.169.12:
;       [prog.baz:169:12] ? i == buf_count
        cmp.169.12:
        cmp qword [rbx + 48], 20
        jne loop.165.5
        if.169.12.code:
;           [prog.baz:169:27] break
        if.169.9.end:
    loop.165.5.end:
;   [prog.baz:172:5] write(1, buf, write_pos)
;   [prog.baz:172:5] allocate named register rdi
;   [prog.baz:172:5] allocate named register rsi
;   [prog.baz:172:5] allocate named register rdx
;   [prog.baz:172:11] rdi = 1
;   [prog.baz:172:11] src: folded constant '1'
    mov rdi, 1
;   [prog.baz:172:19] write_pos
    mov rdx, qword [rbx + 56]
;   [prog.baz:172:14] bounds check begin
;   [prog.baz:172:14] lower bound
;   [prog.baz:172:14] rdx lower bound covered by the unsigned upper bound
;   [prog.baz:172:14] upper bound
    cmp rdx, 20
    ja baz_bounds_line_172
;   [prog.baz:172:14] bounds check end
    lea rsi, [rbx + 8]
;   [prog.baz:172:5] allocate named register rax
    mov rax, 1
    syscall
;   [prog.baz:172:5] free named register rax
;   [prog.baz:172:5] free named register rdx
;   [prog.baz:172:5] free named register rsi
;   [prog.baz:172:5] free named register rdi
    ret
size.func.print_num equ 64
; frame overflow handler (--checks=frame)
baz_frame_overflow:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_frame_overflow]
    mov rdx, msg_frame_overflow_len
    syscall
;    exit with error code 255
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
msg_frame_overflow:
db `panic: frame overflow\n`
msg_frame_overflow_len equ $ - msg_frame_overflow
section .text
; division failure handler (--checks=division)
baz_division_line_154:
    mov rbp, 154
    jmp baz_division_panic
baz_division_line_155:
    mov rbp, 155
baz_division_panic:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_division]
    mov rdx, msg_division_len
    syscall
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
    jnz .convert_loop
    inc rdi
;    print line number to stderr
    mov rax, 1
    mov rsi, rdi
    mov rdx, strict qword num_buffer + 21
    sub rdx, rdi
    mov rdi, 2
    syscall
;    exit with error code 255
    mov rax, 60
    mov rdi, 255
    syscall
section .bss
num_buffer:
resb 21
section .rodata
msg_division:
db `panic: division at line `
msg_division_len equ $ - msg_division
section .text
; overlap failure handler (--checks=overlap)
baz_overlap_line_248:
    mov rbp, 248
    jmp baz_overlap_panic
baz_overlap_line_253:
    mov rbp, 253
    jmp baz_overlap_panic
baz_overlap_line_383:
    mov rbp, 383
baz_overlap_panic:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overlap]
    mov rdx, msg_overlap_len
    syscall
    jmp baz_report_line
section .rodata
msg_overlap:
db `panic: overlap at line `
msg_overlap_len equ $ - msg_overlap
section .text
; overflow failure handler (--checks=overflow)
baz_overflow_line_55:
    mov rbp, 55
    jmp baz_overflow_panic
baz_overflow_line_67:
    mov rbp, 67
    jmp baz_overflow_panic
baz_overflow_line_104:
    mov rbp, 104
    jmp baz_overflow_panic
baz_overflow_line_148:
    mov rbp, 148
    jmp baz_overflow_panic
baz_overflow_line_153:
    mov rbp, 153
    jmp baz_overflow_panic
baz_overflow_line_160:
    mov rbp, 160
    jmp baz_overflow_panic
baz_overflow_line_167:
    mov rbp, 167
    jmp baz_overflow_panic
baz_overflow_line_168:
    mov rbp, 168
    jmp baz_overflow_panic
baz_overflow_line_179:
    mov rbp, 179
    jmp baz_overflow_panic
baz_overflow_line_181:
    mov rbp, 181
    jmp baz_overflow_panic
baz_overflow_line_211:
    mov rbp, 211
    jmp baz_overflow_panic
baz_overflow_line_214:
    mov rbp, 214
    jmp baz_overflow_panic
baz_overflow_line_215:
    mov rbp, 215
    jmp baz_overflow_panic
baz_overflow_line_244:
    mov rbp, 244
    jmp baz_overflow_panic
baz_overflow_line_271:
    mov rbp, 271
    jmp baz_overflow_panic
baz_overflow_line_280:
    mov rbp, 280
    jmp baz_overflow_panic
baz_overflow_line_343:
    mov rbp, 343
    jmp baz_overflow_panic
baz_overflow_line_348:
    mov rbp, 348
    jmp baz_overflow_panic
baz_overflow_line_406:
    mov rbp, 406
baz_overflow_panic:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_overflow]
    mov rdx, msg_overflow_len
    syscall
    jmp baz_report_line
section .rodata
msg_overflow:
db `panic: overflow at line `
msg_overflow_len equ $ - msg_overflow
section .text
; shift failure handler (--checks=shift)
baz_shift_panic:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_shift]
    mov rdx, msg_shift_len
    syscall
    jmp baz_report_line
section .rodata
msg_shift:
db `panic: shift at line `
msg_shift_len equ $ - msg_shift
section .text
; bounds failure handler (--checks=upper or --checks=lower)
baz_bounds_line_96:
    mov rbp, 96
    jmp baz_bounds_panic
baz_bounds_line_154:
    mov rbp, 154
    jmp baz_bounds_panic
baz_bounds_line_161:
    mov rbp, 161
    jmp baz_bounds_panic
baz_bounds_line_166:
    mov rbp, 166
    jmp baz_bounds_panic
baz_bounds_line_172:
    mov rbp, 172
    jmp baz_bounds_panic
baz_bounds_line_243:
    mov rbp, 243
    jmp baz_bounds_panic
baz_bounds_line_244:
    mov rbp, 244
    jmp baz_bounds_panic
baz_bounds_line_248:
    mov rbp, 248
    jmp baz_bounds_panic
baz_bounds_line_253:
    mov rbp, 253
    jmp baz_bounds_panic
baz_bounds_line_254:
    mov rbp, 254
    jmp baz_bounds_panic
baz_bounds_line_260:
    mov rbp, 260
    jmp baz_bounds_panic
baz_bounds_line_271:
    mov rbp, 271
    jmp baz_bounds_panic
baz_bounds_line_272:
    mov rbp, 272
    jmp baz_bounds_panic
baz_bounds_line_273:
    mov rbp, 273
    jmp baz_bounds_panic
baz_bounds_line_384:
    mov rbp, 384
    jmp baz_bounds_panic
baz_bounds_line_385:
    mov rbp, 385
    jmp baz_bounds_panic
baz_bounds_line_392:
    mov rbp, 392
    jmp baz_bounds_panic
baz_bounds_line_393:
    mov rbp, 393
    jmp baz_bounds_panic
baz_bounds_line_427:
    mov rbp, 427
    jmp baz_bounds_panic
baz_bounds_line_428:
    mov rbp, 428
baz_bounds_panic:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_bounds]
    mov rdx, msg_bounds_len
    syscall
    jmp baz_report_line
section .rodata
msg_bounds:
db `panic: bounds at line `
msg_bounds_len equ $ - msg_bounds
section .text

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
times 3 dq 0
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
;[prog.baz:31:23] zero remaining array
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
;                    factorial: 1 body, 2 calls, 37 instructions
;                    print_num: 1 body, 2 calls, 83 instructions
;
;   removed jumps to next code: 131
;    removed unreachable jumps: 2
; removed same target branches: 51
; inverted branches over jumps: 7
; max scratch registers in use: 4
;            max frames in use: 10
;                     dat size: 376 B
;              dat var padding: 8 B
;                max vars size: 1045 B
;                 instructions: 1099
;
; register use at the peak: 6 of 13 registers live, 2 named by instructions
;
;   held  frame, registers (allocated at)
;      6  print_num (noinline body)
;           r15 154:13
;           r14 154:18
;           r13 154:27
;           rax 154:31 (named)
;           rdx 154:31 (named)
;           r12 154:31
;
; the peak is in a function with a body of its own, compiled with all 13
; registers free, its callers are not on this stack
;
; per callee, the most registers one call holds itself
;
;   own  calls  callee
;     6      1  print_num (noinline body)
;     5      1  main
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
;       1      2  factorial 180:19
;       0      2  print_num 407:9
```
