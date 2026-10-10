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
usage: ./baz [options] [file]
compiles file (default: prog.baz) to assembly on stdout, rv32i targets also
write a binary image

options:
  --target=MACHINE    x86_64 (default): linux, nasm
                      rv32i: linux, llvm assembler, qemu user mode
                      rv32i-qemu: bare-metal image for the qemu virt machine
                      rv32i-fpga: bare-metal image for the fpga soft core,
                        stack grows down from the end of the memory (see
                        --memory), fails when code, data, variables and stack
                        do not fit
  --vars=SIZE         variable storage in bytes, decimal or 0x hex, must be a
                      multiple of 16 (default: 65536)
  --stack=SIZE        rv32i-qemu and rv32i-fpga stack in bytes, decimal or 0x
                      hex, must be a multiple of 16 (default: 65536)
  --memory=SIZE       rv32i-fpga memory in bytes, decimal or 0x hex, must be a
                      multiple of 4096 (default: 8388608), the emulator is
                      built for the default
  --checks=LIST       comma separated checks, replaces earlier --checks
  --report=LIST       comma separated reports after the code, replaces earlier
                      --report
  --bin=FILE          rv32i targets binary image (default: file without
                      extension followed by -MACHINE.bin)
  --nopt              no jump optimizations
  --reproduce-source  write reproduced source to diff.baz and check that it
                      matches the input
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
  ./baz --target=rv32i-fpga --checks=upper,line prog.baz > prog.s
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
C/C++ Header                    57           8235           3578          24445
C++                              1            195             56            670
-------------------------------------------------------------------------------
SUM:                            58           8430           3634          25115
-------------------------------------------------------------------------------
```

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
    o2.pos = point{y, x}
    assert(o2.pos.x == 2)
    # the type name of a record literal is optional when assigned

#   o2.pos = point{o2.pos.y, o2.pos.x}
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
    shl rcx, 2
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
    shl rcx, 2
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
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 480]
    bool.254.14.end:
    cmp.257.12:
    mov r15b, byte [rbp + 480]
    bool.257.12.end:
    func.assert.257.5:
        if.38.27.257.5:
        cmp.38.27.257.5:
        cmp r15b, 0
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
        shl rcx, 2
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
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
    cmp.263.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    cmp r15b, 0
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
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
    cmp.300.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    cmp r15b, 0
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
    cmp r15b, 0
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
    mov rax, qword [rbp + 664]
    mov qword [rbp + 640], rax
    mov rax, qword [rbp + 672]
    mov qword [rbp + 648], rax
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
    shl rcx, 3
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
        shl rcx, 3
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
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
            sub byte [rbp + 1288], 1
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
        neg qword [rbx + 32]
        jo baz_overflow_line_148
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
    mov rdi, strict qword num_buffer + 19
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
    mov rdx, strict qword num_buffer + 20
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
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    jmp baz_report_line
section .rodata
msg_panic:
db `panic: bounds at line `
msg_panic_len equ $ - msg_panic
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

;[7:1] point : 16 B    fields:
;[7:1]       name :  offset :    size :  array? : array size
;[7:1]          x :       0 :       8 :      no :           
;[7:1]          y :       8 :       8 :      no :           
;
;[9:1] object : 24 B    fields:
;[9:1]       name :  offset :    size :  array? : array size
;[9:1]        pos :       0 :      16 :      no :           
;[9:1]      color :      16 :       4 :      no :           
;
;[11:1] world : 64 B    fields:
;[11:1]       name :  offset :    size :  array? : array size
;[11:1]  locations :       0 :      64 :     yes :          8
;
;[14:1] str : 128 B    fields:
;[14:1]       name :  offset :    size :  array? : array size
;[14:1]        len :       0 :       1 :      no :           
;[14:1]       data :       1 :     127 :     yes :        127
;
;[22:1] dat hello = "hello world from baz\n"
;[22:7] hello: i8[21] (21 B @ [rbp])
;[23:1] dat prompt1 = "enter name:\n"
;[23:5] prompt1: i8[12] (12 B @ [rbp + 21])
;[24:1] dat prompt2 = "that is not a name.\n"
;[24:5] prompt2: i8[20] (20 B @ [rbp + 33])
;[25:1] dat prompt3 = "hello "
;[25:5] prompt3: i8[6] (6 B @ [rbp + 53])
;[26:1] dat dot = "."
;[26:9] dot: i8[1] (1 B @ [rbp + 59])
;[27:1] dat nl = "\n"
;[27:10] nl: i8[1] (1 B @ [rbp + 60])
;[28:1] dat colon = ": "
;[28:7] colon: i8[2] (2 B @ [rbp + 61])
;[29:1] dat nums = [4]{ 1 }
;[29:8] nums: i64[4] (32 B @ [rbp + 64])
;[30:1] dat str1 = str{ 3 }
;[30:8] str1: str (128 B @ [rbp + 96])
;[31:1] dat str2 = str{ 3, "baz" }
;[31:8] str2: str (128 B @ [rbp + 224])
;[33:1] dat greeted = "names greeted: "
;[33:5] greeted: i8[15] (15 B @ [rbp + 352])
;[34:1] dat names = 0
;[34:7] names: i64 (8 B @ [rbp + 368])
;[124:5] const yes = 1
;[125:5] const no = 0
;[126:5] const maybe = -1
;
main:
;   [187:5] var answer = 0
;   [187:9] answer: i64 (8 B @ [rbp + 384])
;   [187:9] answer = 0
;   [187:18] 0
    mov qword [rbp + 384], 0
;   [189:5] assert(answer == 0)
;   [189:12] allocate scratch register -> r15
;   [189:12] ? answer == 0
;   [189:12] ? answer == 0
    cmp.189.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.189.12.end:
;   [38:6] assert(ok bool)
    func.assert.189.5:
;       [189:5] alias ok -> r15b
        if.38.27.189.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.189.5:
        cmp r15b, 0
        jne if.38.24.189.5.end
        if.38.27.189.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.189.5.end:
;       [189:5] free scratch register r15
    func.assert.189.5.end:
;   [191:5] answer = maybe
;   [191:14] maybe
    mov qword [rbp + 384], -1
;   [192:5] assert(answer == -1)
;   [192:12] allocate scratch register -> r15
;   [192:12] ? answer == -1
;   [192:12] ? answer == -1
    cmp.192.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.192.12.end:
;   [38:6] assert(ok bool)
    func.assert.192.5:
;       [192:5] alias ok -> r15b
        if.38.27.192.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.192.5:
        cmp r15b, 0
        jne if.38.24.192.5.end
        if.38.27.192.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.192.5.end:
;       [192:5] free scratch register r15
    func.assert.192.5.end:
;       [197:13] const maybe = 33
;       [198:9] assert(maybe == 33)
;       [38:6] assert(ok bool)
        func.assert.198.9:
;           [198:9] alias ok -> 1
            if.38.27.198.9:
;           [38:27] ? not ok
;           [38:27] ? shorthand: not ok
            cmp.38.27.198.9:
;           [38:31] const eval to false
            if.38.24.198.9.end:
        func.assert.198.9.end:
;   [201:5] assert(maybe == -1)
;   [38:6] assert(ok bool)
    func.assert.201.5:
;       [201:5] alias ok -> 1
        if.38.27.201.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.201.5:
;       [38:31] const eval to false
        if.38.24.201.5.end:
    func.assert.201.5.end:
;   [203:5] assert(nums[0] == 1 and nums[3] == 0)
;   [203:12] allocate scratch register -> r15
;   [203:12] ? nums[0] == 1 and nums[3] == 0
;   [203:12] ? nums[0] == 1
    cmp.203.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.203.12.end
;   [203:29] ? nums[3] == 0
    cmp.203.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.203.12.end:
;   [38:6] assert(ok bool)
    func.assert.203.5:
;       [203:5] alias ok -> r15b
        if.38.27.203.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.203.5:
        cmp r15b, 0
        jne if.38.24.203.5.end
        if.38.27.203.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.203.5.end:
;       [203:5] free scratch register r15
    func.assert.203.5.end:
;   [204:5] assert(str2.len == 3 and str2.data[2] == 'z')
;   [204:12] allocate scratch register -> r15
;   [204:12] ? str2.len == 3 and str2.data[2] == 'z'
;   [204:12] ? str2.len == 3
    cmp.204.12:
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.204.12.end
;   [204:30] ? str2.data[2] == 'z'
    cmp.204.30:
    cmp byte [rbp + 227], 122
    sete r15b
    bool.204.12.end:
;   [38:6] assert(ok bool)
    func.assert.204.5:
;       [204:5] alias ok -> r15b
        if.38.27.204.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.204.5:
        cmp r15b, 0
        jne if.38.24.204.5.end
        if.38.27.204.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.204.5.end:
;       [204:5] free scratch register r15
    func.assert.204.5.end:
;   [206:5] var a = 7
;   [206:9] a: i64 (8 B @ [rbp + 392])
;   [206:9] a = 7
;   [206:13] 7
    mov qword [rbp + 392], 7
;   [207:5] assert(a & 3 == 3)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? a & 3 == 3
;   [207:12] ? a & 3 == 3
    cmp.207.12:
;   [207:12] allocate scratch register -> r14
;       [207:12] a
        mov r14, qword [rbp + 392]
;       [207:12] r14 & 3
;       [207:12] src: folded constant '& 3'
        and r14, 3
    cmp r14, 3
;   [207:12] free scratch register r14
    sete r15b
    bool.207.12.end:
;   [38:6] assert(ok bool)
    func.assert.207.5:
;       [207:5] alias ok -> r15b
        if.38.27.207.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.207.5:
        cmp r15b, 0
        jne if.38.24.207.5.end
        if.38.27.207.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.207.5.end:
;       [207:5] free scratch register r15
    func.assert.207.5.end:
;   [208:5] assert(a | 8 == 15)
;   [208:12] allocate scratch register -> r15
;   [208:12] ? a | 8 == 15
;   [208:12] ? a | 8 == 15
    cmp.208.12:
;   [208:12] allocate scratch register -> r14
;       [208:12] a
        mov r14, qword [rbp + 392]
;       [208:12] r14 | 8
;       [208:12] src: folded constant '| 8'
        or r14, 8
    cmp r14, 15
;   [208:12] free scratch register r14
    sete r15b
    bool.208.12.end:
;   [38:6] assert(ok bool)
    func.assert.208.5:
;       [208:5] alias ok -> r15b
        if.38.27.208.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.208.5:
        cmp r15b, 0
        jne if.38.24.208.5.end
        if.38.27.208.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.208.5.end:
;       [208:5] free scratch register r15
    func.assert.208.5.end:
;   [209:5] assert(a ^ 1 == 6)
;   [209:12] allocate scratch register -> r15
;   [209:12] ? a ^ 1 == 6
;   [209:12] ? a ^ 1 == 6
    cmp.209.12:
;   [209:12] allocate scratch register -> r14
;       [209:12] a
        mov r14, qword [rbp + 392]
;       [209:12] r14 ^ 1
;       [209:12] src: folded constant '^ 1'
        xor r14, 1
    cmp r14, 6
;   [209:12] free scratch register r14
    sete r15b
    bool.209.12.end:
;   [38:6] assert(ok bool)
    func.assert.209.5:
;       [209:5] alias ok -> r15b
        if.38.27.209.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.209.5:
        cmp r15b, 0
        jne if.38.24.209.5.end
        if.38.27.209.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.209.5.end:
;       [209:5] free scratch register r15
    func.assert.209.5.end:
;   [210:5] assert(a << 2 == 28)
;   [210:12] allocate scratch register -> r15
;   [210:12] ? a << 2 == 28
;   [210:12] ? a << 2 == 28
    cmp.210.12:
;   [210:12] allocate scratch register -> r14
;       [210:12] a
        mov r14, qword [rbp + 392]
;       [210:17] r14 << 2
;       [210:17] src: constant
        sal r14, 2
    cmp r14, 28
;   [210:12] free scratch register r14
    sete r15b
    bool.210.12.end:
;   [38:6] assert(ok bool)
    func.assert.210.5:
;       [210:5] alias ok -> r15b
        if.38.27.210.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.210.5:
        cmp r15b, 0
        jne if.38.24.210.5.end
        if.38.27.210.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.210.5.end:
;       [210:5] free scratch register r15
    func.assert.210.5.end:
;   [211:5] assert(-a >> 1 == -4)
;   [211:12] allocate scratch register -> r15
;   [211:12] ? -a >> 1 == -4
;   [211:12] ? -a >> 1 == -4
    cmp.211.12:
;   [211:12] allocate scratch register -> r14
;       [211:13] -a
        mov r14, qword [rbp + 392]
        neg r14
        jo baz_overflow_line_211
;       [211:18] r14 >> 1
;       [211:18] src: constant
        sar r14, 1
    cmp r14, -4
;   [211:12] free scratch register r14
    sete r15b
    bool.211.12.end:
;   [38:6] assert(ok bool)
    func.assert.211.5:
;       [211:5] alias ok -> r15b
        if.38.27.211.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.211.5:
        cmp r15b, 0
        jne if.38.24.211.5.end
        if.38.27.211.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.211.5.end:
;       [211:5] free scratch register r15
    func.assert.211.5.end:
;   [214:5] assert(a + a << 1 == 21)
;   [214:12] allocate scratch register -> r15
;   [214:12] ? a + a << 1 == 21
;   [214:12] ? a + a << 1 == 21
    cmp.214.12:
;   [214:12] allocate scratch register -> r14
;       [214:12] a
        mov r14, qword [rbp + 392]
;       [214:18] r14 + a << 1
;       [214:18] src: expression
;       [214:18] allocate scratch register -> r13
;       [214:16] a
        mov r13, qword [rbp + 392]
;       [214:21] r13 << 1
;       [214:21] src: constant
        sal r13, 1
        add r14, r13
        jo baz_overflow_line_214
;       [214:18] free scratch register r13
    cmp r14, 21
;   [214:12] free scratch register r14
    sete r15b
    bool.214.12.end:
;   [38:6] assert(ok bool)
    func.assert.214.5:
;       [214:5] alias ok -> r15b
        if.38.27.214.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.214.5:
        cmp r15b, 0
        jne if.38.24.214.5.end
        if.38.27.214.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.214.5.end:
;       [214:5] free scratch register r15
    func.assert.214.5.end:
;   [215:5] assert((a + a) << 1 == 28)
;   [215:12] allocate scratch register -> r15
;   [215:12] ? (a + a) << 1 == 28
;   [215:12] ? (a + a) << 1 == 28
    cmp.215.12:
;   [215:12] allocate scratch register -> r14
;       [215:13] r14 = (a + a)
;       [215:13] = expression
;       [215:13] a
        mov r14, qword [rbp + 392]
;       [215:17] r14 + a
;       [215:17] src: operand
        add r14, qword [rbp + 392]
        jo baz_overflow_line_215
;       [215:23] r14 << 1
;       [215:23] src: constant
        sal r14, 1
    cmp r14, 28
;   [215:12] free scratch register r14
    sete r15b
    bool.215.12.end:
;   [38:6] assert(ok bool)
    func.assert.215.5:
;       [215:5] alias ok -> r15b
        if.38.27.215.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.215.5:
        cmp r15b, 0
        jne if.38.24.215.5.end
        if.38.27.215.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.215.5.end:
;       [215:5] free scratch register r15
    func.assert.215.5.end:
;   [220:5] assert(a != 0 and (a >= 7 or a < 0))
;   [220:12] allocate scratch register -> r15
;   [220:12] ? a != 0 and (a >= 7 or a < 0)
;   [220:12] ? a != 0
    cmp.220.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.220.12.end
    cmp.220.23:
;   [220:23] ? (a >= 7 or a < 0)
;   [220:24] ? a >= 7
    cmp.220.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.220.12.end
;   [220:34] ? a < 0
    cmp.220.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.220.12.end:
;   [38:6] assert(ok bool)
    func.assert.220.5:
;       [220:5] alias ok -> r15b
        if.38.27.220.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.220.5:
        cmp r15b, 0
        jne if.38.24.220.5.end
        if.38.27.220.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.220.5.end:
;       [220:5] free scratch register r15
    func.assert.220.5.end:
;   [223:5] var small = i8(100)
;   [223:9] small: i8 (1 B @ [rbp + 400])
;   [223:9] small = i8(100)
    mov byte [rbp + 400], 100
;   [224:5] small = i8(small + small)
;   [224:13] small = i8(small + small)
;   [224:13] = expression
;   [224:13] instructions without scratch register 3, with 3
;   [224:16] instructions without scratch register 3, with 4
;   [224:16] allocate scratch register -> r15
;   [224:16] small
    mov r15b, byte [rbp + 400]
;   [224:24] r15b + small
;   [224:24] src: operand
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
;   [224:16] free scratch register r15
;   [225:5] assert(small == -56)
;   [225:12] allocate scratch register -> r15
;   [225:12] ? small == -56
;   [225:12] ? small == -56
    cmp.225.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.225.12.end:
;   [38:6] assert(ok bool)
    func.assert.225.5:
;       [225:5] alias ok -> r15b
        if.38.27.225.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.225.5:
        cmp r15b, 0
        jne if.38.24.225.5.end
        if.38.27.225.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.225.5.end:
;       [225:5] free scratch register r15
    func.assert.225.5.end:
;   [228:5] var wide = int(i16(small))
;   [228:9] wide: i64 (8 B @ [rbp + 408])
;   [228:9] wide = int(i16(small))
;   [228:16] wide = int(i16(small))
;   [228:16] = expression
;   [228:20] wide = i16(small)
;   [228:20] = expression
;   [228:20] allocate scratch register -> r15
;   [228:24] small
    movsx r15w, byte [rbp + 400]
    movsx r15, r15w
    mov qword [rbp + 408], r15
;   [228:20] free scratch register r15
;   [229:5] assert(wide == -56)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? wide == -56
;   [229:12] ? wide == -56
    cmp.229.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.229.12.end:
;   [38:6] assert(ok bool)
    func.assert.229.5:
;       [229:5] alias ok -> r15b
        if.38.27.229.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.229.5:
        cmp r15b, 0
        jne if.38.24.229.5.end
        if.38.27.229.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.229.5.end:
;       [229:5] free scratch register r15
    func.assert.229.5.end:
;   [236:5] var letter = '\x41'
;   [236:9] letter: i64 (8 B @ [rbp + 416])
;   [236:9] letter = '\x41'
;   [236:18] '\x41'
    mov qword [rbp + 416], 65
;   [237:5] assert(letter == 'A')
;   [237:12] allocate scratch register -> r15
;   [237:12] ? letter == 'A'
;   [237:12] ? letter == 'A'
    cmp.237.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.237.12.end:
;   [38:6] assert(ok bool)
    func.assert.237.5:
;       [237:5] alias ok -> r15b
        if.38.27.237.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.237.5:
        cmp r15b, 0
        jne if.38.24.237.5.end
        if.38.27.237.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.237.5.end:
;       [237:5] free scratch register r15
    func.assert.237.5.end:
;   [240:5] var arr = i32[4]
;   [240:9] arr: i32[4] (16 B @ [rbp + 424])
;   [240:9] arr = i32[4]
;   [240:15] zero remaining elements: 4 * 4 B = 16 B
;   [240:15] size <= 32 B, use mov
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
;   [242:5] var ix = 1
;   [242:9] ix: i64 (8 B @ [rbp + 440])
;   [242:9] ix = 1
;   [242:14] 1
    mov qword [rbp + 440], 1
;   [243:5] arr[ix] = 2
;   [243:9] allocate scratch register -> r15
;   [243:9] set array index
;   [243:9] ix
    mov r15, qword [rbp + 440]
;   [243:9] bounds check begin
;   [243:9] lower bound
;   [243:9] r15 lower bound covered by the unsigned upper bound
;   [243:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_243
;   [243:9] bounds check end
;   [243:15] 2
    mov dword [rbp + r15 * 4 + 424], 2
;   [243:5] free scratch register r15
;   [244:5] arr[ix + 1] = arr[ix]
;   [244:9] allocate scratch register -> r15
;   [244:9] set array index
;   [244:9] ix
    mov r15, qword [rbp + 440]
;   [244:9] r15 + 1
;   [244:9] src: folded constant '+ 1'
    add r15, 1
    jo baz_overflow_line_244
;   [244:9] bounds check begin
;   [244:9] lower bound
;   [244:9] r15 lower bound covered by the unsigned upper bound
;   [244:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_244
;   [244:9] bounds check end
;   [244:19] arr[ix]
;   [244:23] allocate scratch register -> r14
;   [244:23] set array index
;   [244:23] ix
    mov r14, qword [rbp + 440]
;   [244:23] bounds check begin
;   [244:23] lower bound
;   [244:23] r14 lower bound covered by the unsigned upper bound
;   [244:23] upper bound
    cmp r14, 4
    jae baz_bounds_line_244
;   [244:23] bounds check end
;   [244:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
;   [244:19] free scratch register r13
;   [244:19] free scratch register r14
;   [244:5] free scratch register r15
;   [245:5] assert(arr[1] == 2)
;   [245:12] allocate scratch register -> r15
;   [245:12] ? arr[1] == 2
;   [245:12] ? arr[1] == 2
    cmp.245.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.245.12.end:
;   [38:6] assert(ok bool)
    func.assert.245.5:
;       [245:5] alias ok -> r15b
        if.38.27.245.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.245.5:
        cmp r15b, 0
        jne if.38.24.245.5.end
        if.38.27.245.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.245.5.end:
;       [245:5] free scratch register r15
    func.assert.245.5.end:
;   [246:5] assert(arr[2] == 2)
;   [246:12] allocate scratch register -> r15
;   [246:12] ? arr[2] == 2
;   [246:12] ? arr[2] == 2
    cmp.246.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.246.12.end:
;   [38:6] assert(ok bool)
    func.assert.246.5:
;       [246:5] alias ok -> r15b
        if.38.27.246.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.246.5:
        cmp r15b, 0
        jne if.38.24.246.5.end
        if.38.27.246.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.246.5.end:
;       [246:5] free scratch register r15
    func.assert.246.5.end:
;   [248:5] array_copy(arr[2], arr, 2)
;   [248:5] allocate named register rsi
;   [248:5] allocate named register rdi
;   [248:5] allocate named register rcx
;   [248:29] 2
;   [248:29] 2
    mov rcx, 2
;   [248:16] arr[2]
;   [248:20] allocate scratch register -> r15
;   [248:20] set array index
;   [248:20] 2
    mov r15, 2
;   [248:20] bounds check begin
;   [248:20] lower bound
    test r15, r15
    js baz_bounds_line_248
    test rcx, rcx
    js baz_bounds_line_248
;   [248:20] upper bound
;   [248:20] allocate scratch register -> r14
    lea r14, [rcx + r15]
    cmp r14, 4
;   [248:20] free scratch register r14
    ja baz_bounds_line_248
;   [248:20] bounds check end
    lea rsi, [rbp + r15 * 4 + 424]
;   [248:5] free scratch register r15
;   [248:24] arr
;   [248:24] bounds check begin
;   [248:24] lower bound
;   [248:24] rcx lower bound covered by the unsigned upper bound
;   [248:24] upper bound
    cmp rcx, 4
    ja baz_bounds_line_248
;   [248:24] bounds check end
    lea rdi, [rbp + 424]
    shl rcx, 2
;   [248:5] overlap check begin
;   [248:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [248:5] the same range is not an overlap
    je .Lbaz_overlap.0
;   [248:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_248
    .Lbaz_overlap.0:
;   [248:5] free scratch register r15
;   [248:5] overlap check end
    rep movsb
;   [248:5] free named register rcx
;   [248:5] free named register rdi
;   [248:5] free named register rsi
;   [249:5] assert(arr[0] == 2)
;   [249:12] allocate scratch register -> r15
;   [249:12] ? arr[0] == 2
;   [249:12] ? arr[0] == 2
    cmp.249.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.249.12.end:
;   [38:6] assert(ok bool)
    func.assert.249.5:
;       [249:5] alias ok -> r15b
        if.38.27.249.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.249.5:
        cmp r15b, 0
        jne if.38.24.249.5.end
        if.38.27.249.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.249.5.end:
;       [249:5] free scratch register r15
    func.assert.249.5.end:
;   [252:5] var arr1 = i32[8]
;   [252:9] arr1: i32[8] (32 B @ [rbp + 448])
;   [252:9] arr1 = i32[8]
;   [252:16] zero remaining elements: 8 * 4 B = 32 B
;   [252:16] size <= 32 B, use mov
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
;   [253:5] array_copy(arr, arr1, 4)
;   [253:5] allocate named register rsi
;   [253:5] allocate named register rdi
;   [253:5] allocate named register rcx
;   [253:27] 4
;   [253:27] 4
    mov rcx, 4
;   [253:16] arr
;   [253:16] bounds check begin
;   [253:16] lower bound
;   [253:16] rcx lower bound covered by the unsigned upper bound
;   [253:16] upper bound
    cmp rcx, 4
    ja baz_bounds_line_253
;   [253:16] bounds check end
    lea rsi, [rbp + 424]
;   [253:21] arr1
;   [253:21] bounds check begin
;   [253:21] lower bound
;   [253:21] rcx lower bound covered by the unsigned upper bound
;   [253:21] upper bound
    cmp rcx, 8
    ja baz_bounds_line_253
;   [253:21] bounds check end
    lea rdi, [rbp + 448]
    shl rcx, 2
;   [253:5] overlap check begin
;   [253:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [253:5] the same range is not an overlap
    je .Lbaz_overlap.1
;   [253:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_253
    .Lbaz_overlap.1:
;   [253:5] free scratch register r15
;   [253:5] overlap check end
    rep movsb
;   [253:5] free named register rcx
;   [253:5] free named register rdi
;   [253:5] free named register rsi
;   [254:5] var eq = arrays_equal(arr[1], arr1[1], 3)
;   [254:9] eq: bool (1 B @ [rbp + 480])
;   [254:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [254:14] ? arrays_equal(arr[1], arr1[1], 3)
;   [254:14] ? shorthand: arrays_equal(arr[1], arr1[1], 3)
    cmp.254.14:
;       [254:14] arrays_equal(arr[1], arr1[1], 3)
;       [254:14] allocate named register rsi
;       [254:14] allocate named register rdi
;       [254:14] allocate named register rcx
;       [254:44] 3
;       [254:44] 3
        mov rcx, 3
;       [254:27] arr[1]
;       [254:31] allocate scratch register -> r15
;       [254:31] set array index
;       [254:31] 1
        mov r15, 1
;       [254:31] bounds check begin
;       [254:31] lower bound
        test r15, r15
        js baz_bounds_line_254
        test rcx, rcx
        js baz_bounds_line_254
;       [254:31] upper bound
;       [254:31] allocate scratch register -> r14
        lea r14, [rcx + r15]
        cmp r14, 4
;       [254:31] free scratch register r14
        ja baz_bounds_line_254
;       [254:31] bounds check end
        lea rsi, [rbp + r15 * 4 + 424]
;       [254:14] free scratch register r15
;       [254:35] arr1[1]
;       [254:40] allocate scratch register -> r15
;       [254:40] set array index
;       [254:40] 1
        mov r15, 1
;       [254:40] bounds check begin
;       [254:40] lower bound
;       [254:40] count rcx lower bound already checked
        test r15, r15
        js baz_bounds_line_254
;       [254:40] upper bound
;       [254:40] allocate scratch register -> r14
        lea r14, [rcx + r15]
        cmp r14, 8
;       [254:40] free scratch register r14
        ja baz_bounds_line_254
;       [254:40] bounds check end
        lea rdi, [rbp + r15 * 4 + 448]
;       [254:14] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [254:14] free named register rcx
;       [254:14] free named register rdi
;       [254:14] free named register rsi
        sete byte [rbp + 480]
    bool.254.14.end:
;   [257:5] assert(eq)
;   [257:12] allocate scratch register -> r15
;   [257:12] ? eq
;   [257:12] ? shorthand: eq
    cmp.257.12:
    mov r15b, byte [rbp + 480]
    bool.257.12.end:
;   [38:6] assert(ok bool)
    func.assert.257.5:
;       [257:5] alias ok -> r15b
        if.38.27.257.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.257.5:
        cmp r15b, 0
        jne if.38.24.257.5.end
        if.38.27.257.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.257.5.end:
;       [257:5] free scratch register r15
    func.assert.257.5.end:
;   [259:5] arr1[2] = -1
;   [259:15] instructions without scratch register 1, with 2
;   [259:16] -1
    mov dword [rbp + 456], -1
;   [260:5] assert(not arrays_equal(arr, arr1, 4))
;   [260:12] allocate scratch register -> r15
;   [260:12] ? not arrays_equal(arr, arr1, 4)
;   [260:12] ? shorthand: not arrays_equal(arr, arr1, 4)
    cmp.260.12:
;       [260:16] arrays_equal(arr, arr1, 4)
;       [260:16] allocate named register rsi
;       [260:16] allocate named register rdi
;       [260:16] allocate named register rcx
;       [260:40] 4
;       [260:40] 4
        mov rcx, 4
;       [260:29] arr
;       [260:29] bounds check begin
;       [260:29] lower bound
;       [260:29] rcx lower bound covered by the unsigned upper bound
;       [260:29] upper bound
        cmp rcx, 4
        ja baz_bounds_line_260
;       [260:29] bounds check end
        lea rsi, [rbp + 424]
;       [260:34] arr1
;       [260:34] bounds check begin
;       [260:34] lower bound
;       [260:34] rcx lower bound covered by the unsigned upper bound
;       [260:34] upper bound
        cmp rcx, 8
        ja baz_bounds_line_260
;       [260:34] bounds check end
        lea rdi, [rbp + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [260:16] free named register rcx
;       [260:16] free named register rdi
;       [260:16] free named register rsi
        setne r15b
    bool.260.12.end:
;   [38:6] assert(ok bool)
    func.assert.260.5:
;       [260:5] alias ok -> r15b
        if.38.27.260.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.260.5:
        cmp r15b, 0
        jne if.38.24.260.5.end
        if.38.27.260.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.260.5.end:
;       [260:5] free scratch register r15
    func.assert.260.5.end:
;   [262:5] var arr4 = arr
;   [262:9] arr4: i32[4] (16 B @ [rbp + 484])
;   [262:9] arr4 = arr
;   [262:16] size <= 16 B, use mov
;   [262:16] allocate named register rax
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
;   [262:16] free named register rax
;   [263:5] assert(arr == arr4)
;   [263:12] allocate scratch register -> r15
;   [263:12] ? arr == arr4
;   [263:12] ? arr == arr4
    cmp.263.12:
;       [263:12] allocate named register rsi
;       [263:12] allocate named register rdi
;       [263:12] allocate named register rcx
;       [263:12] arr
        lea rsi, [rbp + 424]
;       [263:19] arr4
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
;       [263:12] free named register rcx
;       [263:12] free named register rdi
;       [263:12] free named register rsi
        sete r15b
    cmp r15b, 0
    bool.263.12.end:
;   [38:6] assert(ok bool)
    func.assert.263.5:
;       [263:5] alias ok -> r15b
        if.38.27.263.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.263.5:
        cmp r15b, 0
        jne if.38.24.263.5.end
        if.38.27.263.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.263.5.end:
;       [263:5] free scratch register r15
    func.assert.263.5.end:
;   [270:5] ix = 3
;   [270:10] 3
    mov qword [rbp + 440], 3
;   [271:5] var tmp = ~inv(arr[ix - 1])
;   [271:9] tmp: i32 (4 B @ [rbp + 500])
;   [271:9] tmp = ~inv(arr[ix - 1])
;   [271:16] tmp = ~inv(arr[ix - 1])
;   [271:16] = expression
;   [271:16] ~inv(arr[ix - 1])
;   [271:16] instructions without scratch register 11, with 11
;   [271:24] allocate scratch register -> r15
;   [271:24] set array index
;   [271:24] ix
    mov r15, qword [rbp + 440]
;   [271:24] r15 - 1
;   [271:24] src: folded constant '- 1'
    sub r15, 1
    jo baz_overflow_line_271
;   [271:24] bounds check begin
;   [271:24] lower bound
;   [271:24] r15 lower bound covered by the unsigned upper bound
;   [271:24] upper bound
    cmp r15, 4
    jae baz_bounds_line_271
;   [271:24] bounds check end
;   [271:16] instructions without scratch register 6, with 7
;   [72:6] inv(i i32) res i32
    func.inv.271.16:
;       [271:16] alias res -> tmp
;       [271:16] alias i -> arr (lea: rbp + r15 * 4 + 424)
;       [73:5] res = ~i
;       [73:11] instructions without scratch register 3, with 3
;       [73:12] ~i
;       [73:12] allocate scratch register -> r14
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
;       [73:12] free scratch register r14
        not dword [rbp + 500]
    func.inv.271.16.end:
    not dword [rbp + 500]
;       [271:16] free scratch register r15
;   [272:5] arr[ix] = tmp
;   [272:9] allocate scratch register -> r15
;   [272:9] set array index
;   [272:9] ix
    mov r15, qword [rbp + 440]
;   [272:9] bounds check begin
;   [272:9] lower bound
;   [272:9] r15 lower bound covered by the unsigned upper bound
;   [272:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_272
;   [272:9] bounds check end
;   [272:15] tmp
;   [272:15] allocate scratch register -> r14
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
;   [272:15] free scratch register r14
;   [272:5] free scratch register r15
;   [273:5] assert(arr[ix] == 2)
;   [273:12] allocate scratch register -> r15
;   [273:12] ? arr[ix] == 2
;   [273:12] ? arr[ix] == 2
    cmp.273.12:
;   [273:16] allocate scratch register -> r14
;   [273:16] set array index
;   [273:16] ix
    mov r14, qword [rbp + 440]
;   [273:16] bounds check begin
;   [273:16] lower bound
;   [273:16] r14 lower bound covered by the unsigned upper bound
;   [273:16] upper bound
    cmp r14, 4
    jae baz_bounds_line_273
;   [273:16] bounds check end
    cmp dword [rbp + r14 * 4 + 424], 2
;   [273:12] free scratch register r14
    sete r15b
    bool.273.12.end:
;   [38:6] assert(ok bool)
    func.assert.273.5:
;       [273:5] alias ok -> r15b
        if.38.27.273.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.273.5:
        cmp r15b, 0
        jne if.38.24.273.5.end
        if.38.27.273.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.273.5.end:
;       [273:5] free scratch register r15
    func.assert.273.5.end:
;   [275:5] faz(arr)
;   [78:6] faz(arg mut i32[])
    func.faz.275.5:
;       [275:5] alias arg -> arr
;       [79:5] arg[1] = 0xfe
;       [79:14] 0xfe
        mov dword [rbp + 428], 254
    func.faz.275.5.end:
;   [276:5] assert(arr[1] == 0xfe)
;   [276:12] allocate scratch register -> r15
;   [276:12] ? arr[1] == 0xfe
;   [276:12] ? arr[1] == 0xfe
    cmp.276.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.276.12.end:
;   [38:6] assert(ok bool)
    func.assert.276.5:
;       [276:5] alias ok -> r15b
        if.38.27.276.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.276.5:
        cmp r15b, 0
        jne if.38.24.276.5.end
        if.38.27.276.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.276.5.end:
;       [276:5] free scratch register r15
    func.assert.276.5.end:
;   [278:5] var arr3 = []{ 3, 5 }
;   [278:9] arr3: i64[2] (16 B @ [rbp + 504])
;   [278:9] arr3 = []{ 3, 5 }
;   [278:18] size <= 16 B, use immediates
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
;   [279:5] foo arr3
;   [279:9] allocate scratch register -> r15
;   [279:9] initiate iterator e
    lea r15, [rbp + 504]
;   [279:5] allocate scratch register -> r14
;   [279:9] e: i64 (r15)
;   [279:9] i: i64 (r14)
;   [279:9] const n = 2
;   [279:5] initiate counter i
    mov r14, 0
    foo.279.5:
;       [280:9] e = e + i + n
;       [280:13] instructions without scratch register 4, with 6
;       [280:13] e
;       [280:17] e + i
;       [280:17] src: operand
        add qword [r15], r14
        jo baz_overflow_line_280
;       [280:13] e + 2
;       [280:13] src: folded constant '+ n'
        add qword [r15], 2
        jo baz_overflow_line_280
        foo.279.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.279.5
    foo.279.5.end:
;   [279:5] free scratch register r14
;   [279:5] free scratch register r15
;   [282:5] assert(arr3[0] == 3 + 0 + 2)
;   [282:12] allocate scratch register -> r15
;   [282:12] ? arr3[0] == 3 + 0 + 2
;   [282:12] ? arr3[0] == 3 + 0 + 2
    cmp.282.12:
;   [282:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 504], 5
    sete r15b
    bool.282.12.end:
;   [38:6] assert(ok bool)
    func.assert.282.5:
;       [282:5] alias ok -> r15b
        if.38.27.282.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.282.5:
        cmp r15b, 0
        jne if.38.24.282.5.end
        if.38.27.282.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.282.5.end:
;       [282:5] free scratch register r15
    func.assert.282.5.end:
;   [283:5] assert(arr3[1] == 5 + 1 + 2)
;   [283:12] allocate scratch register -> r15
;   [283:12] ? arr3[1] == 5 + 1 + 2
;   [283:12] ? arr3[1] == 5 + 1 + 2
    cmp.283.12:
;   [283:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 512], 8
    sete r15b
    bool.283.12.end:
;   [38:6] assert(ok bool)
    func.assert.283.5:
;       [283:5] alias ok -> r15b
        if.38.27.283.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.283.5:
        cmp r15b, 0
        jne if.38.24.283.5.end
        if.38.27.283.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.283.5.end:
;       [283:5] free scratch register r15
    func.assert.283.5.end:
;   [289:5] var p = point
;   [289:9] p: point (16 B @ [rbp + 520])
;   [289:9] p = point
;   [289:13] zero remaining fields: 16 B
;   [289:13] size <= 32 B, use mov
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
;   [291:7] p.fooz()
;   [47:10] mut point.fooz()
    func.point.fooz.291.7:
;       [291:7] alias self -> p
;       [48:5] self.x = 0b10
;       [48:14] 0b10
        mov qword [rbp + 520], 2
;       [49:5] self.y = 0xb
;       [49:14] 0xb
        mov qword [rbp + 528], 11
    func.point.fooz.291.7.end:
;   [294:5] assert(p.x == 2)
;   [294:12] allocate scratch register -> r15
;   [294:12] ? p.x == 2
;   [294:12] ? p.x == 2
    cmp.294.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.294.12.end:
;   [38:6] assert(ok bool)
    func.assert.294.5:
;       [294:5] alias ok -> r15b
        if.38.27.294.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.294.5:
        cmp r15b, 0
        jne if.38.24.294.5.end
        if.38.27.294.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.294.5.end:
;       [294:5] free scratch register r15
    func.assert.294.5.end:
;   [295:5] assert(p.y == 0xb)
;   [295:12] allocate scratch register -> r15
;   [295:12] ? p.y == 0xb
;   [295:12] ? p.y == 0xb
    cmp.295.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.295.12.end:
;   [38:6] assert(ok bool)
    func.assert.295.5:
;       [295:5] alias ok -> r15b
        if.38.27.295.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.295.5:
        cmp r15b, 0
        jne if.38.24.295.5.end
        if.38.27.295.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.295.5.end:
;       [295:5] free scratch register r15
    func.assert.295.5.end:
;   [297:5] var q = p
;   [297:9] q: point (16 B @ [rbp + 536])
;   [297:9] q = p
;   [297:13] size <= 16 B, use mov
;   [297:13] allocate named register rax
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
;   [297:13] free named register rax
;   [300:5] assert(p == q)
;   [300:12] allocate scratch register -> r15
;   [300:12] ? p == q
;   [300:12] ? p == q
    cmp.300.12:
;       [300:12] allocate named register rsi
;       [300:12] allocate named register rdi
;       [300:12] allocate named register rcx
;       [300:12] p
        lea rsi, [rbp + 520]
;       [300:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
;       [300:12] free named register rcx
;       [300:12] free named register rdi
;       [300:12] free named register rsi
        sete r15b
    cmp r15b, 0
    bool.300.12.end:
;   [38:6] assert(ok bool)
    func.assert.300.5:
;       [300:5] alias ok -> r15b
        if.38.27.300.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.300.5:
        cmp r15b, 0
        jne if.38.24.300.5.end
        if.38.27.300.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.300.5.end:
;       [300:5] free scratch register r15
    func.assert.300.5.end:
;   [304:5] q.x = 3
;   [304:11] 3
    mov qword [rbp + 536], 3
;   [305:5] assert(p != q)
;   [305:12] allocate scratch register -> r15
;   [305:12] ? p != q
;   [305:12] ? p != q
    cmp.305.12:
;       [305:12] allocate named register rsi
;       [305:12] allocate named register rdi
;       [305:12] allocate named register rcx
;       [305:12] p
        lea rsi, [rbp + 520]
;       [305:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
;       [305:12] free named register rcx
;       [305:12] free named register rdi
;       [305:12] free named register rsi
        setne r15b
    cmp r15b, 0
    bool.305.12.end:
;   [38:6] assert(ok bool)
    func.assert.305.5:
;       [305:5] alias ok -> r15b
        if.38.27.305.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.305.5:
        cmp r15b, 0
        jne if.38.24.305.5.end
        if.38.27.305.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.305.5.end:
;       [305:5] free scratch register r15
    func.assert.305.5.end:
;   [307:5] var i = 0
;   [307:9] i: i64 (8 B @ [rbp + 552])
;   [307:9] i = 0
;   [307:13] 0
    mov qword [rbp + 552], 0
;   [308:5] bar(i)
;   [59:6] bar(arg mut)
    func.bar.308.5:
;       [308:5] alias arg -> i
        if.60.8.308.5:
;       [60:8] ? arg == 0
;       [60:8] ? arg == 0
        cmp.60.8.308.5:
        cmp qword [rbp + 552], 0
        je func.bar.308.5.end
        if.60.8.308.5.code:
;           [60:17] return
        if.60.5.308.5.end:
;       [61:5] arg = 0xff
;       [61:11] 0xff
        mov qword [rbp + 552], 255
    func.bar.308.5.end:
;   [309:5] assert(i == 0)
;   [309:12] allocate scratch register -> r15
;   [309:12] ? i == 0
;   [309:12] ? i == 0
    cmp.309.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.309.12.end:
;   [38:6] assert(ok bool)
    func.assert.309.5:
;       [309:5] alias ok -> r15b
        if.38.27.309.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.309.5:
        cmp r15b, 0
        jne if.38.24.309.5.end
        if.38.27.309.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.309.5.end:
;       [309:5] free scratch register r15
    func.assert.309.5.end:
;   [311:5] i = 1
;   [311:9] 1
    mov qword [rbp + 552], 1
;   [312:5] bar(i)
;   [59:6] bar(arg mut)
    func.bar.312.5:
;       [312:5] alias arg -> i
        if.60.8.312.5:
;       [60:8] ? arg == 0
;       [60:8] ? arg == 0
        cmp.60.8.312.5:
        cmp qword [rbp + 552], 0
        je func.bar.312.5.end
        if.60.8.312.5.code:
;           [60:17] return
        if.60.5.312.5.end:
;       [61:5] arg = 0xff
;       [61:11] 0xff
        mov qword [rbp + 552], 255
    func.bar.312.5.end:
;   [313:5] assert(i == 0xff)
;   [313:12] allocate scratch register -> r15
;   [313:12] ? i == 0xff
;   [313:12] ? i == 0xff
    cmp.313.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.313.12.end:
;   [38:6] assert(ok bool)
    func.assert.313.5:
;       [313:5] alias ok -> r15b
        if.38.27.313.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.313.5:
        cmp r15b, 0
        jne if.38.24.313.5.end
        if.38.27.313.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.313.5.end:
;       [313:5] free scratch register r15
    func.assert.313.5.end:
;   [315:5] var j = 1
;   [315:9] j: i64 (8 B @ [rbp + 560])
;   [315:9] j = 1
;   [315:13] 1
    mov qword [rbp + 560], 1
;   [316:5] var k = baz(j)
;   [316:9] k: i64 (8 B @ [rbp + 568])
;   [316:9] k = baz(j)
;   [316:13] k = baz(j)
;   [316:13] = expression
;   [316:13] baz(j)
;   [66:6] baz(arg) res
    func.baz.316.13:
;       [316:13] alias res -> k
;       [316:13] alias arg -> j
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 6, with 4
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, qword [rbp + 560]
;       [67:11] r15 * 2
;       [67:11] src: folded constant '* 2'
        imul r15, 2
        jo baz_overflow_line_67
        mov qword [rbp + 568], r15
;       [67:11] free scratch register r15
    func.baz.316.13.end:
;   [317:5] assert(k == 2)
;   [317:12] allocate scratch register -> r15
;   [317:12] ? k == 2
;   [317:12] ? k == 2
    cmp.317.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.317.12.end:
;   [38:6] assert(ok bool)
    func.assert.317.5:
;       [317:5] alias ok -> r15b
        if.38.27.317.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.317.5:
        cmp r15b, 0
        jne if.38.24.317.5.end
        if.38.27.317.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.317.5.end:
;       [317:5] free scratch register r15
    func.assert.317.5.end:
;   [319:5] k = baz(1)
;   [319:9] k = baz(1)
;   [319:9] = expression
;   [319:9] baz(1)
;   [66:6] baz(arg) res
    func.baz.319.9:
;       [319:9] alias res -> k
;       [319:9] alias arg -> 1
;       [67:5] res = arg * 2
;       [67:11] res = 2
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 568], 2
    func.baz.319.9.end:
;   [320:5] assert(k == 2)
;   [320:12] allocate scratch register -> r15
;   [320:12] ? k == 2
;   [320:12] ? k == 2
    cmp.320.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.320.12.end:
;   [38:6] assert(ok bool)
    func.assert.320.5:
;       [320:5] alias ok -> r15b
        if.38.27.320.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.320.5:
        cmp r15b, 0
        jne if.38.24.320.5.end
        if.38.27.320.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.320.5.end:
;       [320:5] free scratch register r15
    func.assert.320.5.end:
;   [322:5] var five = 5
;   [322:9] five: i64 (8 B @ [rbp + 576])
;   [322:9] five = 5
;   [322:16] 5
    mov qword [rbp + 576], 5
;   [323:5] var f = factorial(five)
;   [323:9] f: i64 (8 B @ [rbp + 584])
;   [323:9] f = factorial(five)
;   [323:13] f = factorial(five)
;   [323:13] = expression
;   [323:13] factorial(five)
;   [323:13] frame capacity check begin
;   [323:13] allocate scratch register -> r15
;   [323:13] allocate scratch register -> r14
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
;   [323:13] free scratch register r14
;   [323:13] free scratch register r15
;   [323:13] frame capacity check end
;   [323:13] result address in callee frame
;   [323:13] allocate scratch register -> r15
    lea r15, [rbp + 584]
    mov qword [rbp + 592], r15
;   [323:13] free scratch register r15
;   [323:13] address of argument 'five' to parameter 'n'
;   [323:13] allocate scratch register -> r15
    lea r15, [rbp + 576]
    mov qword [rbp + 600], r15
;   [323:13] free scratch register r15
;   [323:13] set function frame base
    lea rbx, [rbp + 592]
    call func.factorial
;   [324:5] assert(f == 120)
;   [324:12] allocate scratch register -> r15
;   [324:12] ? f == 120
;   [324:12] ? f == 120
    cmp.324.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.324.12.end:
;   [38:6] assert(ok bool)
    func.assert.324.5:
;       [324:5] alias ok -> r15b
        if.38.27.324.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.324.5:
        cmp r15b, 0
        jne if.38.24.324.5.end
        if.38.27.324.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.324.5.end:
;       [324:5] free scratch register r15
    func.assert.324.5.end:
;   [326:5] var p0 = point{baz(3), 0}
;   [326:9] p0: point (16 B @ [rbp + 592])
;   [326:9] p0 = point{baz(3), 0}
;   [326:20] copy field 'x'
;   [326:20] p0.x = baz(3)
;   [326:20] = expression
;   [326:20] baz(3)
;   [66:6] baz(arg) res
    func.baz.326.20:
;       [326:20] alias res -> p0.x (lea: rbp + 592)
;       [326:20] alias arg -> 3
;       [67:5] res = arg * 2
;       [67:11] res = 6
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 592], 6
    func.baz.326.20.end:
;   [326:28] copy field 'y'
    mov qword [rbp + 600], 0
;   [327:5] assert(p0.x == 6)
;   [327:12] allocate scratch register -> r15
;   [327:12] ? p0.x == 6
;   [327:12] ? p0.x == 6
    cmp.327.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.327.12.end:
;   [38:6] assert(ok bool)
    func.assert.327.5:
;       [327:5] alias ok -> r15b
        if.38.27.327.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.327.5:
        cmp r15b, 0
        jne if.38.24.327.5.end
        if.38.27.327.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.327.5.end:
;       [327:5] free scratch register r15
    func.assert.327.5.end:
;   [329:5] var pt = point.at(-1, -2)
;   [329:9] pt: point (16 B @ [rbp + 608])
;   [329:9] pt = point.at(-1, -2)
;   [329:14] point.at(-1, -2)
;   [108:6] point.at(x, y) self
    func.point.at.329.14:
;       [329:14] alias self -> pt
;       [329:14] alias x -> -1
;       [329:14] alias y -> -2
;       [109:5] self.x = x
;       [109:14] x
        mov qword [rbp + 608], -1
;       [110:5] self.y = y
;       [110:14] y
        mov qword [rbp + 616], -2
    func.point.at.329.14.end:
;   [333:5] assert(pt.x == -1)
;   [333:12] allocate scratch register -> r15
;   [333:12] ? pt.x == -1
;   [333:12] ? pt.x == -1
    cmp.333.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.333.12.end:
;   [38:6] assert(ok bool)
    func.assert.333.5:
;       [333:5] alias ok -> r15b
        if.38.27.333.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.333.5:
        cmp r15b, 0
        jne if.38.24.333.5.end
        if.38.27.333.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.333.5.end:
;       [333:5] free scratch register r15
    func.assert.333.5.end:
;   [334:5] assert(pt.y == -2)
;   [334:12] allocate scratch register -> r15
;   [334:12] ? pt.y == -2
;   [334:12] ? pt.y == -2
    cmp.334.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.334.12.end:
;   [38:6] assert(ok bool)
    func.assert.334.5:
;       [334:5] alias ok -> r15b
        if.38.27.334.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.334.5:
        cmp r15b, 0
        jne if.38.24.334.5.end
        if.38.27.334.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.334.5.end:
;       [334:5] free scratch register r15
    func.assert.334.5.end:
;   [336:8] pt.x(2)
;   [114:10] mut point.x(x)
    func.point.x.336.8:
;       [336:8] alias self -> pt
;       [336:8] alias x -> 2
;       [115:5] self.x = x
;       [115:14] x
        mov qword [rbp + 608], 2
    func.point.x.336.8.end:
;   [337:5] assert(pt.x == 2)
;   [337:12] allocate scratch register -> r15
;   [337:12] ? pt.x == 2
;   [337:12] ? pt.x == 2
    cmp.337.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.337.12.end:
;   [38:6] assert(ok bool)
    func.assert.337.5:
;       [337:5] alias ok -> r15b
        if.38.27.337.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.337.5:
        cmp r15b, 0
        jne if.38.24.337.5.end
        if.38.27.337.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.337.5.end:
;       [337:5] free scratch register r15
    func.assert.337.5.end:
;   [338:5] assert(pt.sum() == 0)
;   [338:12] allocate scratch register -> r15
;   [338:12] ? pt.sum() == 0
;   [338:12] ? pt.sum() == 0
    cmp.338.12:
;   [338:12] allocate scratch register -> r14
;       [338:15] r14 = pt.sum()
;       [338:15] = expression
;       [338:15] pt.sum()
;       [54:6] point.sum() res
        func.point.sum.338.15:
;           [338:15] alias res -> r14
;           [338:15] alias self -> pt
;           [55:5] res = self.x + self.y
;           [55:11] self.x
            mov r14, qword [rbp + 608]
;           [55:20] res + self.y
;           [55:20] src: operand
            add r14, qword [rbp + 616]
            jo baz_overflow_line_55
        func.point.sum.338.15.end:
    cmp r14, 0
;   [338:12] free scratch register r14
    sete r15b
    bool.338.12.end:
;   [38:6] assert(ok bool)
    func.assert.338.5:
;       [338:5] alias ok -> r15b
        if.38.27.338.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.338.5:
        cmp r15b, 0
        jne if.38.24.338.5.end
        if.38.27.338.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.338.5.end:
;       [338:5] free scratch register r15
    func.assert.338.5.end:
;   [340:5] var x = 1
;   [340:9] x: i64 (8 B @ [rbp + 624])
;   [340:9] x = 1
;   [340:13] 1
    mov qword [rbp + 624], 1
;   [341:5] var y = 2
;   [341:9] y: i64 (8 B @ [rbp + 632])
;   [341:9] y = 2
;   [341:13] 2
    mov qword [rbp + 632], 2
;   [343:5] var o1 = object{{x * 10, y}, 0xff0000}
;   [343:9] o1: object (24 B @ [rbp + 640])
;   [343:9] o1 = object{{x * 10, y}, 0xff0000}
;   [343:21] copy field 'pos'
;   [343:22] copy field 'x'
;   [343:22] instructions without scratch register 6, with 4
;   [343:22] allocate scratch register -> r15
;   [343:22] x
    mov r15, qword [rbp + 624]
;   [343:22] r15 * 10
;   [343:22] src: folded constant '* 10'
    imul r15, 10
    jo baz_overflow_line_343
    mov qword [rbp + 640], r15
;   [343:22] free scratch register r15
;   [343:30] copy field 'y'
;   [343:30] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
;   [343:30] free scratch register r15
;   [343:34] copy field 'color'
    mov dword [rbp + 656], 16711680
;   [343:14] zero padding: 4 B
;   [343:14] size <= 32 B, use mov
    mov dword [rbp + 660], 0
;   [344:5] assert(o1.pos.x == 10)
;   [344:12] allocate scratch register -> r15
;   [344:12] ? o1.pos.x == 10
;   [344:12] ? o1.pos.x == 10
    cmp.344.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.344.12.end:
;   [38:6] assert(ok bool)
    func.assert.344.5:
;       [344:5] alias ok -> r15b
        if.38.27.344.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.344.5:
        cmp r15b, 0
        jne if.38.24.344.5.end
        if.38.27.344.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.344.5.end:
;       [344:5] free scratch register r15
    func.assert.344.5.end:
;   [345:5] assert(o1.pos.y == 2)
;   [345:12] allocate scratch register -> r15
;   [345:12] ? o1.pos.y == 2
;   [345:12] ? o1.pos.y == 2
    cmp.345.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.345.12.end:
;   [38:6] assert(ok bool)
    func.assert.345.5:
;       [345:5] alias ok -> r15b
        if.38.27.345.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.345.5:
        cmp r15b, 0
        jne if.38.24.345.5.end
        if.38.27.345.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.345.5.end:
;       [345:5] free scratch register r15
    func.assert.345.5.end:
;   [346:5] assert(o1.color == 0xff0000)
;   [346:12] allocate scratch register -> r15
;   [346:12] ? o1.color == 0xff0000
;   [346:12] ? o1.color == 0xff0000
    cmp.346.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.346.12.end:
;   [38:6] assert(ok bool)
    func.assert.346.5:
;       [346:5] alias ok -> r15b
        if.38.27.346.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.346.5:
        cmp r15b, 0
        jne if.38.24.346.5.end
        if.38.27.346.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.346.5.end:
;       [346:5] free scratch register r15
    func.assert.346.5.end:
;   [348:5] var p1 = point{-x, -y}
;   [348:9] p1: point (16 B @ [rbp + 664])
;   [348:9] p1 = point{-x, -y}
;   [348:20] copy field 'x'
;   [348:20] instructions without scratch register 4, with 4
;   [348:20] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
;   [348:20] free scratch register r15
    neg qword [rbp + 664]
    jo baz_overflow_line_348
;   [348:24] copy field 'y'
;   [348:24] instructions without scratch register 4, with 4
;   [348:24] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
;   [348:24] free scratch register r15
    neg qword [rbp + 672]
    jo baz_overflow_line_348
;   [349:5] o1.pos = p1
;   [349:14] size <= 16 B, use mov
;   [349:14] allocate named register rax
    mov rax, qword [rbp + 664]
    mov qword [rbp + 640], rax
    mov rax, qword [rbp + 672]
    mov qword [rbp + 648], rax
;   [349:14] free named register rax
;   [350:5] assert(o1.pos.x == -1)
;   [350:12] allocate scratch register -> r15
;   [350:12] ? o1.pos.x == -1
;   [350:12] ? o1.pos.x == -1
    cmp.350.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.350.12.end:
;   [38:6] assert(ok bool)
    func.assert.350.5:
;       [350:5] alias ok -> r15b
        if.38.27.350.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.350.5:
        cmp r15b, 0
        jne if.38.24.350.5.end
        if.38.27.350.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.350.5.end:
;       [350:5] free scratch register r15
    func.assert.350.5.end:
;   [351:5] assert(o1.pos.y == -2)
;   [351:12] allocate scratch register -> r15
;   [351:12] ? o1.pos.y == -2
;   [351:12] ? o1.pos.y == -2
    cmp.351.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.351.12.end:
;   [38:6] assert(ok bool)
    func.assert.351.5:
;       [351:5] alias ok -> r15b
        if.38.27.351.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.351.5:
        cmp r15b, 0
        jne if.38.24.351.5.end
        if.38.27.351.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.351.5.end:
;       [351:5] free scratch register r15
    func.assert.351.5.end:
;   [353:5] var o2 = o1
;   [353:9] o2: object (24 B @ [rbp + 680])
;   [353:9] o2 = o1
;   [353:14] allocate named register rsi
;   [353:14] allocate named register rdi
;   [353:14] allocate named register rcx
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
;   [353:14] free named register rcx
;   [353:14] free named register rdi
;   [353:14] free named register rsi
;   [354:5] assert(o2.pos.x == -1)
;   [354:12] allocate scratch register -> r15
;   [354:12] ? o2.pos.x == -1
;   [354:12] ? o2.pos.x == -1
    cmp.354.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.354.12.end:
;   [38:6] assert(ok bool)
    func.assert.354.5:
;       [354:5] alias ok -> r15b
        if.38.27.354.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.354.5:
        cmp r15b, 0
        jne if.38.24.354.5.end
        if.38.27.354.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.354.5.end:
;       [354:5] free scratch register r15
    func.assert.354.5.end:
;   [355:5] assert(o2.pos.y == -2)
;   [355:12] allocate scratch register -> r15
;   [355:12] ? o2.pos.y == -2
;   [355:12] ? o2.pos.y == -2
    cmp.355.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.355.12.end:
;   [38:6] assert(ok bool)
    func.assert.355.5:
;       [355:5] alias ok -> r15b
        if.38.27.355.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.355.5:
        cmp r15b, 0
        jne if.38.24.355.5.end
        if.38.27.355.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.355.5.end:
;       [355:5] free scratch register r15
    func.assert.355.5.end:
;   [356:5] assert(o2.color == 0xff0000)
;   [356:12] allocate scratch register -> r15
;   [356:12] ? o2.color == 0xff0000
;   [356:12] ? o2.color == 0xff0000
    cmp.356.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.356.12.end:
;   [38:6] assert(ok bool)
    func.assert.356.5:
;       [356:5] alias ok -> r15b
        if.38.27.356.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.356.5:
        cmp r15b, 0
        jne if.38.24.356.5.end
        if.38.27.356.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.356.5.end:
;       [356:5] free scratch register r15
    func.assert.356.5.end:
;   [358:5] o2.pos = {x, y}
;   [358:15] copy field 'x'
;   [358:15] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
;   [358:15] free scratch register r15
;   [358:18] copy field 'y'
;   [358:18] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
;   [358:18] free scratch register r15
;   [359:5] assert(o2.pos.x == 1)
;   [359:12] allocate scratch register -> r15
;   [359:12] ? o2.pos.x == 1
;   [359:12] ? o2.pos.x == 1
    cmp.359.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.359.12.end:
;   [38:6] assert(ok bool)
    func.assert.359.5:
;       [359:5] alias ok -> r15b
        if.38.27.359.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.359.5:
        cmp r15b, 0
        jne if.38.24.359.5.end
        if.38.27.359.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.359.5.end:
;       [359:5] free scratch register r15
    func.assert.359.5.end:
;   [360:5] o2.pos = point{y, x}
;   [360:20] copy field 'x'
;   [360:20] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
;   [360:20] free scratch register r15
;   [360:23] copy field 'y'
;   [360:23] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
;   [360:23] free scratch register r15
;   [361:5] assert(o2.pos.x == 2)
;   [361:12] allocate scratch register -> r15
;   [361:12] ? o2.pos.x == 2
;   [361:12] ? o2.pos.x == 2
    cmp.361.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.361.12.end:
;   [38:6] assert(ok bool)
    func.assert.361.5:
;       [361:5] alias ok -> r15b
        if.38.27.361.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.361.5:
        cmp r15b, 0
        jne if.38.24.361.5.end
        if.38.27.361.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.361.5.end:
;       [361:5] free scratch register r15
    func.assert.361.5.end:
;   [368:5] var o3 = object[2]
;   [368:9] o3: object[2] (48 B @ [rbp + 704])
;   [368:9] o3 = object[2]
;   [368:14] zero remaining elements: 2 * 24 B = 48 B
;   [368:14] allocate named register rax
;   [368:14] allocate named register rdi
;   [368:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
;   [368:14] free named register rcx
;   [368:14] free named register rdi
;   [368:14] free named register rax
;   [369:5] o3[0].pos.y = 73
;   [369:19] 73
    mov qword [rbp + 712], 73
;   [371:5] assert(o3[0].pos.y == 73)
;   [371:12] allocate scratch register -> r15
;   [371:12] ? o3[0].pos.y == 73
;   [371:12] ? o3[0].pos.y == 73
    cmp.371.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.371.12.end:
;   [38:6] assert(ok bool)
    func.assert.371.5:
;       [371:5] alias ok -> r15b
        if.38.27.371.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.371.5:
        cmp r15b, 0
        jne if.38.24.371.5.end
        if.38.27.371.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.371.5.end:
;       [371:5] free scratch register r15
    func.assert.371.5.end:
;   [372:5] o3[1] = object.at(2, 74, 0xffffff)
;   [372:13] zero padding: 4 B
;   [372:13] size <= 32 B, use mov
    mov dword [rbp + 748], 0
;   [372:13] object.at(2, 74, 0xffffff)
;   [119:6] object.at(x, y, color i32) self
    func.object.at.372.13:
;       [372:13] alias self -> o3 (lea: rbp + 728)
;       [372:13] alias x -> 2
;       [372:13] alias y -> 74
;       [372:13] alias color -> 16777215
;       [120:5] self.pos = point.at(x, y)
;       [120:16] point.at(x, y)
;       [108:6] point.at(x, y) self
        func.point.at.120.16.372.13:
;           [120:16] alias self -> self.pos (lea: rbp + 728)
;           [120:16] alias x -> 2
;           [120:16] alias y -> 74
;           [109:5] self.x = x
;           [109:14] x
            mov qword [rbp + 728], 2
;           [110:5] self.y = y
;           [110:14] y
            mov qword [rbp + 736], 74
        func.point.at.120.16.372.13.end:
;       [121:5] self.color = color
;       [121:18] color
        mov dword [rbp + 744], 16777215
    func.object.at.372.13.end:
;   [373:5] assert(o3[1].pos.y == 74)
;   [373:12] allocate scratch register -> r15
;   [373:12] ? o3[1].pos.y == 74
;   [373:12] ? o3[1].pos.y == 74
    cmp.373.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.373.12.end:
;   [38:6] assert(ok bool)
    func.assert.373.5:
;       [373:5] alias ok -> r15b
        if.38.27.373.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.373.5:
        cmp r15b, 0
        jne if.38.24.373.5.end
        if.38.27.373.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.373.5.end:
;       [373:5] free scratch register r15
    func.assert.373.5.end:
;   [375:15] o3[1].pos.fooz()
;   [47:10] mut point.fooz()
    func.point.fooz.375.15:
;       [375:15] alias self -> o3.pos (lea: rbp + 728)
;       [48:5] self.x = 0b10
;       [48:14] 0b10
        mov qword [rbp + 728], 2
;       [49:5] self.y = 0xb
;       [49:14] 0xb
        mov qword [rbp + 736], 11
    func.point.fooz.375.15.end:
;   [376:5] assert(o3[1].pos.sum() == 13)
;   [376:12] allocate scratch register -> r15
;   [376:12] ? o3[1].pos.sum() == 13
;   [376:12] ? o3[1].pos.sum() == 13
    cmp.376.12:
;   [376:12] allocate scratch register -> r14
;       [376:22] r14 = o3[1].pos.sum()
;       [376:22] = expression
;       [376:22] o3[1].pos.sum()
;       [54:6] point.sum() res
        func.point.sum.376.22:
;           [376:22] alias res -> r14
;           [376:22] alias self -> o3.pos (lea: rbp + 728)
;           [55:5] res = self.x + self.y
;           [55:11] self.x
            mov r14, qword [rbp + 728]
;           [55:20] res + self.y
;           [55:20] src: operand
            add r14, qword [rbp + 736]
            jo baz_overflow_line_55
        func.point.sum.376.22.end:
    cmp r14, 13
;   [376:12] free scratch register r14
    sete r15b
    bool.376.12.end:
;   [38:6] assert(ok bool)
    func.assert.376.5:
;       [376:5] alias ok -> r15b
        if.38.27.376.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.376.5:
        cmp r15b, 0
        jne if.38.24.376.5.end
        if.38.27.376.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.376.5.end:
;       [376:5] free scratch register r15
    func.assert.376.5.end:
;   [379:5] var worlds = world[8]
;   [379:9] worlds: world[8] (512 B @ [rbp + 752])
;   [379:9] worlds = world[8]
;   [379:18] zero remaining elements: 8 * 64 B = 512 B
;   [379:18] allocate named register rax
;   [379:18] allocate named register rdi
;   [379:18] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
;   [379:18] free named register rcx
;   [379:18] free named register rdi
;   [379:18] free named register rax
;   [380:5] worlds[1].locations[1] = 0xffee
;   [380:30] 0xffee
    mov qword [rbp + 824], 65518
;   [381:5] assert(worlds[1].locations[1] == 0xffee)
;   [381:12] allocate scratch register -> r15
;   [381:12] ? worlds[1].locations[1] == 0xffee
;   [381:12] ? worlds[1].locations[1] == 0xffee
    cmp.381.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.381.12.end:
;   [38:6] assert(ok bool)
    func.assert.381.5:
;       [381:5] alias ok -> r15b
        if.38.27.381.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.381.5:
        cmp r15b, 0
        jne if.38.24.381.5.end
        if.38.27.381.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.381.5.end:
;       [381:5] free scratch register r15
    func.assert.381.5.end:
;   [383:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [383:5] allocate named register rsi
;   [383:5] allocate named register rdi
;   [383:5] allocate named register rcx
;   [386:9] array_length(worlds[0].locations)
;   [386:9] rcx = 8
;   [386:9] src: folded constant 'array_length(worlds[0].locations)'
    mov rcx, 8
;   [384:9] worlds[1].locations
;   [384:9] bounds check begin
;   [384:9] lower bound
;   [384:9] rcx lower bound covered by the unsigned upper bound
;   [384:9] upper bound
    cmp rcx, 8
    ja baz_bounds_line_384
;   [384:9] bounds check end
    lea rsi, [rbp + 816]
;   [385:9] worlds[0].locations
;   [385:9] bounds check begin
;   [385:9] lower bound
;   [385:9] rcx lower bound covered by the unsigned upper bound
;   [385:9] upper bound
    cmp rcx, 8
    ja baz_bounds_line_385
;   [385:9] bounds check end
    lea rdi, [rbp + 752]
    shl rcx, 3
;   [383:5] overlap check begin
;   [383:5] allocate scratch register -> r15
    mov r15, rdi
    sub r15, rsi
;   [383:5] the same range is not an overlap
    je .Lbaz_overlap.2
;   [383:5] destination starts inside the source
    cmp r15, rcx
    jb baz_overlap_line_383
    .Lbaz_overlap.2:
;   [383:5] free scratch register r15
;   [383:5] overlap check end
    rep movsb
;   [383:5] free named register rcx
;   [383:5] free named register rdi
;   [383:5] free named register rsi
;   [390:5] assert(worlds[0].locations[1] == 0xffee)
;   [390:12] allocate scratch register -> r15
;   [390:12] ? worlds[0].locations[1] == 0xffee
;   [390:12] ? worlds[0].locations[1] == 0xffee
    cmp.390.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.390.12.end:
;   [38:6] assert(ok bool)
    func.assert.390.5:
;       [390:5] alias ok -> r15b
        if.38.27.390.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.390.5:
        cmp r15b, 0
        jne if.38.24.390.5.end
        if.38.27.390.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.390.5.end:
;       [390:5] free scratch register r15
    func.assert.390.5.end:
;   [391:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [391:12] allocate scratch register -> r15
;   [391:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [391:12] ? shorthand: arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
    cmp.391.12:
;       [391:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;       [391:12] allocate named register rsi
;       [391:12] allocate named register rdi
;       [391:12] allocate named register rcx
;       [394:14] array_length(worlds[0].locations)
;       [394:14] rcx = 8
;       [394:14] src: folded constant 'array_length(worlds[0].locations)'
        mov rcx, 8
;       [392:14] worlds[0].locations
;       [392:14] bounds check begin
;       [392:14] lower bound
;       [392:14] rcx lower bound covered by the unsigned upper bound
;       [392:14] upper bound
        cmp rcx, 8
        ja baz_bounds_line_392
;       [392:14] bounds check end
        lea rsi, [rbp + 752]
;       [393:14] worlds[1].locations
;       [393:14] bounds check begin
;       [393:14] lower bound
;       [393:14] rcx lower bound covered by the unsigned upper bound
;       [393:14] upper bound
        cmp rcx, 8
        ja baz_bounds_line_393
;       [393:14] bounds check end
        lea rdi, [rbp + 816]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [391:12] free named register rcx
;       [391:12] free named register rdi
;       [391:12] free named register rsi
        sete r15b
    bool.391.12.end:
;   [38:6] assert(ok bool)
    func.assert.391.5:
;       [391:5] alias ok -> r15b
        if.38.27.391.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.391.5:
        cmp r15b, 0
        jne if.38.24.391.5.end
        if.38.27.391.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.391.5.end:
;       [391:5] free scratch register r15
    func.assert.391.5.end:
;   [397:5] var arr2 = []{ -1, 2 }
;   [397:9] arr2: i64[2] (16 B @ [rbp + 1264])
;   [397:9] arr2 = []{ -1, 2 }
;   [397:18] size <= 16 B, use immediates
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
;   [398:5] assert(array_length(arr2) == 2)
;   [38:6] assert(ok bool)
    func.assert.398.5:
;       [398:5] alias ok -> 1
        if.38.27.398.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.398.5:
;       [38:31] const eval to false
        if.38.24.398.5.end:
    func.assert.398.5.end:
;   [399:5] assert(arr2[0] == -1)
;   [399:12] allocate scratch register -> r15
;   [399:12] ? arr2[0] == -1
;   [399:12] ? arr2[0] == -1
    cmp.399.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.399.12.end:
;   [38:6] assert(ok bool)
    func.assert.399.5:
;       [399:5] alias ok -> r15b
        if.38.27.399.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.399.5:
        cmp r15b, 0
        jne if.38.24.399.5.end
        if.38.27.399.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.399.5.end:
;       [399:5] free scratch register r15
    func.assert.399.5.end:
;   [400:5] assert(arr2[1] == 2)
;   [400:12] allocate scratch register -> r15
;   [400:12] ? arr2[1] == 2
;   [400:12] ? arr2[1] == 2
    cmp.400.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.400.12.end:
;   [38:6] assert(ok bool)
    func.assert.400.5:
;       [400:5] alias ok -> r15b
        if.38.27.400.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.400.5:
        cmp r15b, 0
        jne if.38.24.400.5.end
        if.38.27.400.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.400.5.end:
;       [400:5] free scratch register r15
    func.assert.400.5.end:
;   [402:5] var counter = 0
;   [402:9] counter: i64 (8 B @ [rbp + 1280])
;   [402:9] counter = 0
;   [402:19] 0
    mov qword [rbp + 1280], 0
;   [403:5] var nm = str
;   [403:9] nm: str (128 B @ [rbp + 1288])
;   [403:9] nm = str
;   [403:14] zero remaining fields: 128 B
;   [403:14] allocate named register rax
;   [403:14] allocate named register rdi
;   [403:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
;   [403:14] free named register rcx
;   [403:14] free named register rdi
;   [403:14] free named register rax
;   [404:5] print(hello)
;   [41:6] print(str i8[])
    func.print.404.5:
;       [404:5] alias str -> hello
;       [42:5] write(1, str)
;       [42:5] allocate named register rdi
;       [42:5] allocate named register rsi
;       [42:5] allocate named register rdx
;       [42:11] 1
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
;       [42:5] allocate named register rax
        mov rax, 1
        syscall
;       [42:5] free named register rax
;       [42:5] free named register rdx
;       [42:5] free named register rsi
;       [42:5] free named register rdi
    func.print.404.5.end:
;   [405:5] label
    loop.405.5:
;       [406:9] counter = counter + 1
;       [406:19] instructions without scratch register 2, with 4
;       [406:19] counter
;       [406:19] counter + 1
;       [406:19] src: folded constant '+ 1'
        add qword [rbp + 1280], 1
        jo baz_overflow_line_406
;       [407:9] print_num(counter)
;       [407:9] frame capacity check begin
;       [407:9] allocate scratch register -> r15
;       [407:9] allocate scratch register -> r14
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
;       [407:9] free scratch register r14
;       [407:9] free scratch register r15
;       [407:9] frame capacity check end
;       [407:9] address of argument 'counter' to parameter 'num'
;       [407:9] allocate scratch register -> r15
        lea r15, [rbp + 1280]
        mov qword [rbp + 1416], r15
;       [407:9] free scratch register r15
;       [407:9] set function frame base
        lea rbx, [rbp + 1416]
        call func.print_num
;       [408:9] print(colon)
;       [41:6] print(str i8[])
        func.print.408.9:
;           [408:9] alias str -> colon
;           [42:5] write(1, str)
;           [42:5] allocate named register rdi
;           [42:5] allocate named register rsi
;           [42:5] allocate named register rdx
;           [42:11] 1
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
;           [42:5] allocate named register rax
            mov rax, 1
            syscall
;           [42:5] free named register rax
;           [42:5] free named register rdx
;           [42:5] free named register rsi
;           [42:5] free named register rdi
        func.print.408.9.end:
;       [409:9] print(prompt1)
;       [41:6] print(str i8[])
        func.print.409.9:
;           [409:9] alias str -> prompt1
;           [42:5] write(1, str)
;           [42:5] allocate named register rdi
;           [42:5] allocate named register rsi
;           [42:5] allocate named register rdx
;           [42:11] 1
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
;           [42:5] allocate named register rax
            mov rax, 1
            syscall
;           [42:5] free named register rax
;           [42:5] free named register rdx
;           [42:5] free named register rsi
;           [42:5] free named register rdi
        func.print.409.9.end:
;       [410:12] nm.input()
;       [88:10] mut str.input()
        func.str.input.410.12:
;           [410:12] alias self -> nm
;           [89:5] var nbytes = read(0, self.data)
;           [89:9] nbytes: i64 (8 B @ [rbp + 1416])
;           [89:9] nbytes = read(0, self.data)
;           [89:18] nbytes = read(0, self.data)
;           [89:18] = expression
;           [89:18] read(0, self.data)
;           [89:18] allocate named register rdi
;           [89:18] allocate named register rsi
;           [89:18] allocate named register rdx
;           [89:23] 0
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
;           [89:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
;           [89:18] free named register rax
;           [89:18] free named register rdx
;           [89:18] free named register rsi
;           [89:18] free named register rdi
;           [92:5] self.len = i8(nbytes - 1)
;           [92:16] self.len = i8(nbytes - 1)
;           [92:16] = expression
;           [92:16] instructions without scratch register 3, with 3
;           [92:19] instructions without scratch register 3, with 3
;           [92:19] nbytes
;           [92:19] allocate scratch register -> r15
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
;           [92:19] free scratch register r15
;           [92:19] self.len - 1
;           [92:19] src: folded constant '- 1'
            sub byte [rbp + 1288], 1
        func.str.input.410.12.end:
        if.412.12:
;       [412:12] ? nm.len <= 0
;       [412:12] ? nm.len <= 0
        cmp.412.12:
        cmp byte [rbp + 1288], 0
        jle loop.405.5.end
        if.412.12.code:
;           [413:13] break
        if.414.19:
;       [414:19] ? nm.len <= 4
;       [414:19] ? nm.len <= 4
        cmp.414.19:
        cmp byte [rbp + 1288], 4
        jg if.412.9.else
        if.414.19.code:
;           [415:13] print(prompt2)
;           [41:6] print(str i8[])
            func.print.415.13:
;               [415:13] alias str -> prompt2
;               [42:5] write(1, str)
;               [42:5] allocate named register rdi
;               [42:5] allocate named register rsi
;               [42:5] allocate named register rdx
;               [42:11] 1
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
;               [42:5] allocate named register rax
                mov rax, 1
                syscall
;               [42:5] free named register rax
;               [42:5] free named register rdx
;               [42:5] free named register rsi
;               [42:5] free named register rdi
            func.print.415.13.end:
;           [416:13] continue
            jmp loop.405.5
        if.412.9.else:
;           [418:13] greet(nm)
;           [99:6] greet(name str)
            func.greet.418.13:
;               [418:13] alias name -> nm
;               [100:5] print(prompt3)
;               [41:6] print(str i8[])
                func.print.100.5.418.13:
;                   [100:5] alias str -> prompt3
;                   [42:5] write(1, str)
;                   [42:5] allocate named register rdi
;                   [42:5] allocate named register rsi
;                   [42:5] allocate named register rdx
;                   [42:11] 1
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
;                   [42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [42:5] free named register rax
;                   [42:5] free named register rdx
;                   [42:5] free named register rsi
;                   [42:5] free named register rdi
                func.print.100.5.418.13.end:
;               [101:10] name.print()
;               [95:6] str.print()
                func.str.print.101.10.418.13:
;                   [101:10] alias self -> name
;                   [96:5] write(1, self.data, self.len)
;                   [96:5] allocate named register rdi
;                   [96:5] allocate named register rsi
;                   [96:5] allocate named register rdx
;                   [96:11] 1
                    mov rdi, 1
;                   [96:25] self.len
                    movsx rdx, byte [rbp + 1288]
;                   [96:14] bounds check begin
;                   [96:14] lower bound
;                   [96:14] rdx lower bound covered by the unsigned upper bound
;                   [96:14] upper bound
                    cmp rdx, 127
                    ja baz_bounds_line_96
;                   [96:14] bounds check end
                    lea rsi, [rbp + 1289]
;                   [96:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [96:5] free named register rax
;                   [96:5] free named register rdx
;                   [96:5] free named register rsi
;                   [96:5] free named register rdi
                func.str.print.101.10.418.13.end:
;               [102:5] print(dot)
;               [41:6] print(str i8[])
                func.print.102.5.418.13:
;                   [102:5] alias str -> dot
;                   [42:5] write(1, str)
;                   [42:5] allocate named register rdi
;                   [42:5] allocate named register rsi
;                   [42:5] allocate named register rdx
;                   [42:11] 1
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
;                   [42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [42:5] free named register rax
;                   [42:5] free named register rdx
;                   [42:5] free named register rsi
;                   [42:5] free named register rdi
                func.print.102.5.418.13.end:
;               [103:5] print(nl)
;               [41:6] print(str i8[])
                func.print.103.5.418.13:
;                   [103:5] alias str -> nl
;                   [42:5] write(1, str)
;                   [42:5] allocate named register rdi
;                   [42:5] allocate named register rsi
;                   [42:5] allocate named register rdx
;                   [42:11] 1
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
;                   [42:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [42:5] free named register rax
;                   [42:5] free named register rdx
;                   [42:5] free named register rsi
;                   [42:5] free named register rdi
                func.print.103.5.418.13.end:
;               [104:5] names = names + 1
;               [104:13] instructions without scratch register 2, with 4
;               [104:13] names
;               [104:13] names + 1
;               [104:13] src: folded constant '+ 1'
                add qword [rbp + 368], 1
                jo baz_overflow_line_104
            func.greet.418.13.end:
        if.412.9.end:
    jmp loop.405.5
    loop.405.5.end:
;   [422:5] print(greeted)
;   [41:6] print(str i8[])
    func.print.422.5:
;       [422:5] alias str -> greeted
;       [42:5] write(1, str)
;       [42:5] allocate named register rdi
;       [42:5] allocate named register rsi
;       [42:5] allocate named register rdx
;       [42:11] 1
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
;       [42:5] allocate named register rax
        mov rax, 1
        syscall
;       [42:5] free named register rax
;       [42:5] free named register rdx
;       [42:5] free named register rsi
;       [42:5] free named register rdi
    func.print.422.5.end:
;   [423:5] print_num(names)
;   [423:5] frame capacity check begin
;   [423:5] allocate scratch register -> r15
;   [423:5] allocate scratch register -> r14
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
;   [423:5] free scratch register r14
;   [423:5] free scratch register r15
;   [423:5] frame capacity check end
;   [423:5] address of argument 'names' to parameter 'num'
;   [423:5] allocate scratch register -> r15
    lea r15, [rbp + 368]
    mov qword [rbp + 1416], r15
;   [423:5] free scratch register r15
;   [423:5] set function frame base
    lea rbx, [rbp + 1416]
    call func.print_num
;   [424:5] print(nl)
;   [41:6] print(str i8[])
    func.print.424.5:
;       [424:5] alias str -> nl
;       [42:5] write(1, str)
;       [42:5] allocate named register rdi
;       [42:5] allocate named register rsi
;       [42:5] allocate named register rdx
;       [42:11] 1
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
;       [42:5] allocate named register rax
        mov rax, 1
        syscall
;       [42:5] free named register rax
;       [42:5] free named register rdx
;       [42:5] free named register rsi
;       [42:5] free named register rdi
    func.print.424.5.end:
;   [426:5] var bye = "bye from baz\n"
;   [426:9] bye: i8[13] (13 B @ [rbp + 1416])
;   [426:9] bye = "bye from baz\n"
;   [426:15] size <= 16 B, use immediates
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
;   [427:5] write(1, bye, 3)
;   [427:5] allocate named register rdi
;   [427:5] allocate named register rsi
;   [427:5] allocate named register rdx
;   [427:11] 1
    mov rdi, 1
;   [427:19] 3
    mov rdx, 3
;   [427:14] bounds check begin
;   [427:14] lower bound
;   [427:14] rdx lower bound covered by the unsigned upper bound
;   [427:14] upper bound
    cmp rdx, 13
    ja baz_bounds_line_427
;   [427:14] bounds check end
    lea rsi, [rbp + 1416]
;   [427:5] allocate named register rax
    mov rax, 1
    syscall
;   [427:5] free named register rax
;   [427:5] free named register rdx
;   [427:5] free named register rsi
;   [427:5] free named register rdi
;   [428:5] write(1, bye, 1, array_length(bye) - 1)
;   [428:5] allocate named register rdi
;   [428:5] allocate named register rsi
;   [428:5] allocate named register rdx
;   [428:11] 1
    mov rdi, 1
;   [428:19] 1
    mov rdx, 1
;   [428:22] allocate scratch register -> r15
;   [428:22] r15 = 12
;   [428:22] src: folded constant 'array_length(bye) - 1'
    mov r15, 12
;   [428:22] bounds check begin
;   [428:22] lower bound
    test r15, r15
    js baz_bounds_line_428
    test rdx, rdx
    js baz_bounds_line_428
;   [428:22] upper bound
;   [428:22] allocate scratch register -> r14
    lea r14, [rdx + r15]
    cmp r14, 13
;   [428:22] free scratch register r14
    ja baz_bounds_line_428
;   [428:22] bounds check end
    lea rsi, [rbp + 1416]
    add rsi, r15
;   [428:5] free scratch register r15
;   [428:5] allocate named register rax
    mov rax, 1
    syscall
;   [428:5] free named register rax
;   [428:5] free named register rdx
;   [428:5] free named register rsi
;   [428:5] free named register rdi
    mov rdi, 0
    mov rax, 60
    syscall

;
;[175:15] noinline factorial(n) res
func.factorial:
;   [175:28] res: i64 (8 B @ [rbx])
;   [175:25] n: i64 (8 B @ [rbx + 8])
;   [176:5] res = 1
;   [176:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [176:11] 1
    mov qword [r15], 1
;   [176:5] free scratch register r15
    if.177.8:
;   [177:8] ? n <= 1
;   [177:8] ? n <= 1
    cmp.177.8:
;   [177:8] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
;   [177:8] free scratch register r15
    jg if.177.5.end
    if.177.8.code:
;       [177:15] return
        ret
    if.177.5.end:
;   [179:5] var m = n - 1
;   [179:9] m: i64 (8 B @ [rbx + 16])
;   [179:9] m = n - 1
;   [179:13] instructions without scratch register 5, with 5
;   [179:13] n
;   [179:13] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
;   [179:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
;   [179:13] free scratch register r14
;   [179:13] free scratch register r15
;   [179:13] m - 1
;   [179:13] src: folded constant '- 1'
    sub qword [rbx + 16], 1
    jo baz_overflow_line_179
;   [180:5] var partial = factorial(m)
;   [180:9] partial: i64 (8 B @ [rbx + 24])
;   [180:9] partial = factorial(m)
;   [180:19] partial = factorial(m)
;   [180:19] = expression
;   [180:19] factorial(m)
;   [180:19] frame capacity check begin
;   [180:19] allocate scratch register -> r15
;   [180:19] allocate scratch register -> r14
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
;   [180:19] free scratch register r14
;   [180:19] free scratch register r15
;   [180:19] frame capacity check end
;   [180:19] result address in callee frame
;   [180:19] allocate scratch register -> r15
    lea r15, [rbx + 24]
    mov qword [rbx + 32], r15
;   [180:19] free scratch register r15
;   [180:19] address of argument 'm' to parameter 'n'
;   [180:19] allocate scratch register -> r15
    lea r15, [rbx + 16]
    mov qword [rbx + 40], r15
;   [180:19] free scratch register r15
;   [180:19] before call: save allocated registers
    push rbx
;   [180:19] set function frame base
    lea rbx, [rbx + 32]
    call func.factorial
;   [180:19] after call: restore saved registers
    pop rbx
;   [181:5] res = n * partial
;   [181:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [181:11] instructions without scratch register 7, with 5
;   [181:11] allocate scratch register -> r14
;   [181:11] n
;   [181:11] allocate scratch register -> r13
    mov r13, qword [rbx + 8]
    mov r14, qword [r13]
;   [181:11] free scratch register r13
;   [181:15] r14 * partial
;   [181:15] src: operand
    imul r14, qword [rbx + 24]
    jo baz_overflow_line_181
    mov qword [r15], r14
;   [181:11] free scratch register r14
;   [181:5] free scratch register r15
    ret
size.func.factorial equ 32
;
;[134:15] noinline print_num(num)
func.print_num:
;   [134:25] num: i64 (8 B @ [rbx])
;   [136:9] const buf_count = 20
;   [138:5] var buf = i8[buf_count]
;   [138:9] buf: i8[20] (20 B @ [rbx + 8])
;   [138:9] buf = i8[buf_count]
;   [138:15] zero remaining elements: 20 * 1 B = 20 B
;   [138:15] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [139:5] var n = num
;   [139:9] n: i64 (8 B @ [rbx + 32])
;   [139:9] n = num
;   [139:13] num
;   [139:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [139:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [139:13] free scratch register r14
;   [139:13] free scratch register r15
;   [140:5] var is_negative = false
;   [140:9] is_negative: bool (1 B @ [rbx + 40])
;   [140:9] is_negative = false
    mov byte [rbx + 40], 0
    if.144.8:
;   [144:8] ? n < 0
;   [144:8] ? n < 0
    cmp.144.8:
    cmp qword [rbx + 32], 0
    jge if.144.5.end
    if.144.8.code:
;       [145:9] is_negative = true
        mov byte [rbx + 40], 1
    if.144.5.end:
    if.147.8:
;   [147:8] ? n > 0
;   [147:8] ? n > 0
    cmp.147.8:
    cmp qword [rbx + 32], 0
    jle if.147.5.end
    if.147.8.code:
;       [148:9] n = -n
;       [148:13] instructions without scratch register 2, with 4
;       [148:14] -n
        neg qword [rbx + 32]
        jo baz_overflow_line_148
    if.147.5.end:
;   [151:5] var i = buf_count
;   [151:9] i: i64 (8 B @ [rbx + 48])
;   [151:9] i = buf_count
;   [151:13] buf_count
    mov qword [rbx + 48], 20
;   [152:5] label
    loop.152.5:
;       [153:9] i = i - 1
;       [153:13] instructions without scratch register 2, with 4
;       [153:13] i
;       [153:13] i - 1
;       [153:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
        jo baz_overflow_line_153
;       [154:9] buf[i] = i8('0' - n % 10)
;       [154:13] allocate scratch register -> r15
;       [154:13] set array index
;       [154:13] i
        mov r15, qword [rbx + 48]
;       [154:13] bounds check begin
;       [154:13] lower bound
;       [154:13] r15 lower bound covered by the unsigned upper bound
;       [154:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_154
;       [154:13] bounds check end
;       [154:18] buf = i8('0' - n % 10)
;       [154:18] = expression
;       [154:18] allocate scratch register -> r14
;           [154:21] r14 = 48
;           [154:21] src: folded constant '+ '0''
            mov r14, 48
;           [154:29] r14 - n % 10
;           [154:29] src: expression
;           [154:29] allocate scratch register -> r13
;           [154:27] n
            mov r13, qword [rbx + 32]
;           [154:31] r13 % 10
;           [154:31] src: constant
;           [154:31] allocate named register rax
            mov rax, r13
;           [154:31] allocate named register rdx
;           [154:31] allocate scratch register -> r12
            mov r12, 10
;           [154:31] division check begin
;           [154:31] zero divisor
            cmp r12, 0
            je baz_division_line_154
;           [154:31] minimum divided by -1 overflows
            cmp r12, -1
            jne .Lbaz_division.6
            mov rdx, -9223372036854775808
            cmp rax, rdx
            je baz_division_line_154
            .Lbaz_division.6:
;           [154:31] division check end
            cqo
            idiv r12
;           [154:31] free scratch register r12
            mov r13, rdx
;           [154:31] free named register rdx
;           [154:31] free named register rax
            sub r14, r13
;           [154:29] free scratch register r13
        mov byte [rbx + r15 + 8], r14b
;       [154:18] free scratch register r14
;       [154:9] free scratch register r15
;       [155:9] n = n / 10
;       [155:13] instructions without scratch register 13, with 15
;       [155:13] n
;       [155:17] n / 10
;       [155:17] src: constant
;       [155:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [155:17] allocate named register rdx
;       [155:17] allocate scratch register -> r15
        mov r15, 10
;       [155:17] division check begin
;       [155:17] zero divisor
        cmp r15, 0
        je baz_division_line_155
;       [155:17] minimum divided by -1 overflows
        cmp r15, -1
        jne .Lbaz_division.7
        mov rdx, -9223372036854775808
        cmp rax, rdx
        je baz_division_line_155
        .Lbaz_division.7:
;       [155:17] division check end
        cqo
        idiv r15
;       [155:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [155:17] free named register rdx
;       [155:17] free named register rax
        if.156.12:
;       [156:12] ? n == 0
;       [156:12] ? n == 0
        cmp.156.12:
        cmp qword [rbx + 32], 0
        jne loop.152.5
        if.156.12.code:
;           [156:19] break
        if.156.9.end:
    loop.152.5.end:
    if.159.8:
;   [159:8] ? is_negative
;   [159:8] ? shorthand: is_negative
    cmp.159.8:
    cmp byte [rbx + 40], 0
    je if.159.5.end
    if.159.8.code:
;       [160:9] i = i - 1
;       [160:13] instructions without scratch register 2, with 4
;       [160:13] i
;       [160:13] i - 1
;       [160:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
        jo baz_overflow_line_160
;       [161:9] buf[i] = '-'
;       [161:13] allocate scratch register -> r15
;       [161:13] set array index
;       [161:13] i
        mov r15, qword [rbx + 48]
;       [161:13] bounds check begin
;       [161:13] lower bound
;       [161:13] r15 lower bound covered by the unsigned upper bound
;       [161:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_161
;       [161:13] bounds check end
;       [161:18] '-'
        mov byte [rbx + r15 + 8], 45
;       [161:9] free scratch register r15
    if.159.5.end:
;   [164:5] var write_pos = 0
;   [164:9] write_pos: i64 (8 B @ [rbx + 56])
;   [164:9] write_pos = 0
;   [164:21] 0
    mov qword [rbx + 56], 0
;   [165:5] label
    loop.165.5:
;       [166:9] buf[write_pos] = buf[i]
;       [166:13] allocate scratch register -> r15
;       [166:13] set array index
;       [166:13] write_pos
        mov r15, qword [rbx + 56]
;       [166:13] bounds check begin
;       [166:13] lower bound
;       [166:13] r15 lower bound covered by the unsigned upper bound
;       [166:13] upper bound
        cmp r15, 20
        jae baz_bounds_line_166
;       [166:13] bounds check end
;       [166:26] buf[i]
;       [166:30] allocate scratch register -> r14
;       [166:30] set array index
;       [166:30] i
        mov r14, qword [rbx + 48]
;       [166:30] bounds check begin
;       [166:30] lower bound
;       [166:30] r14 lower bound covered by the unsigned upper bound
;       [166:30] upper bound
        cmp r14, 20
        jae baz_bounds_line_166
;       [166:30] bounds check end
;       [166:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [166:26] free scratch register r13
;       [166:26] free scratch register r14
;       [166:9] free scratch register r15
;       [167:9] write_pos = write_pos + 1
;       [167:21] instructions without scratch register 2, with 4
;       [167:21] write_pos
;       [167:21] write_pos + 1
;       [167:21] src: folded constant '+ 1'
        add qword [rbx + 56], 1
        jo baz_overflow_line_167
;       [168:9] i = i + 1
;       [168:13] instructions without scratch register 2, with 4
;       [168:13] i
;       [168:13] i + 1
;       [168:13] src: folded constant '+ 1'
        add qword [rbx + 48], 1
        jo baz_overflow_line_168
        if.169.12:
;       [169:12] ? i == buf_count
;       [169:12] ? i == buf_count
        cmp.169.12:
        cmp qword [rbx + 48], 20
        jne loop.165.5
        if.169.12.code:
;           [169:27] break
        if.169.9.end:
    loop.165.5.end:
;   [172:5] write(1, buf, write_pos)
;   [172:5] allocate named register rdi
;   [172:5] allocate named register rsi
;   [172:5] allocate named register rdx
;   [172:11] 1
    mov rdi, 1
;   [172:19] write_pos
    mov rdx, qword [rbx + 56]
;   [172:14] bounds check begin
;   [172:14] lower bound
;   [172:14] rdx lower bound covered by the unsigned upper bound
;   [172:14] upper bound
    cmp rdx, 20
    ja baz_bounds_line_172
;   [172:14] bounds check end
    lea rsi, [rbx + 8]
;   [172:5] allocate named register rax
    mov rax, 1
    syscall
;   [172:5] free named register rax
;   [172:5] free named register rdx
;   [172:5] free named register rsi
;   [172:5] free named register rdi
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
;    line number is in `rbp`
    mov rax, rbp
;    convert to string
    mov rdi, strict qword num_buffer + 19
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
    mov rdx, strict qword num_buffer + 20
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
; stack overflow handler (--checks=stack)
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
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    jmp baz_report_line
section .rodata
msg_panic:
db `panic: bounds at line `
msg_panic_len equ $ - msg_panic
section .text

section .data
align 16
dat:
;[22:7] hello
;[22:15] i8[21]
db `hello world from baz\n`
;[23:5] prompt1
;[23:15] i8[12]
db `enter name:\n`
;[24:5] prompt2
;[24:15] i8[20]
db `that is not a name.\n`
;[25:5] prompt3
;[25:15] i8[6]
db `hello `
;[26:9] dot
;[26:15] i8[1]
db `.`
;[27:10] nl
;[27:15] i8[1]
db `\n`
;[28:7] colon
;[28:15] i8[2]
db `: `
; padding 1 B
times 1 db 0
;[29:8] nums
;[29:15] i64[4]
;[29:20] [0]
;[29:20] i64
dq 1
;[29:15] pad 3 'i64' of size 8
times 24 db 0
;[30:8] str1
;[30:20] i8
db 3
;[30:18] zero remaining fields: 127 B
times 127 db 0
;[31:8] str2
;[31:20] i8
db 3
;[31:23] i8[127]
db `baz`
;[31:23] zero remaining array
times 124 db 0
;[33:5] greeted
;[33:15] i8[15]
db `names greeted: `
; padding 1 B
times 1 db 0
;[34:7] names
;[34:15] i64
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
;                    print_num: 1 body, 2 calls, 81 instructions
;
;   removed jumps to next code: 132
;    removed unreachable jumps: 2
; removed same target branches: 54
; inverted branches over jumps: 7
; max scratch registers in use: 4
;            max frames in use: 10
;                     dat size: 376 B
;              dat var padding: 8 B
;                max vars size: 1045 B
;                 instructions: 1101
;
; register use at the peak: 6 of 14 registers live, 2 named by instructions
;
;   held  frame, registers (allocated at)
;      6  print_num (noinline body)
;           r15 154:13
;           r14 154:18
;           r13 154:29
;           rax 154:31 (named)
;           rdx 154:31 (named)
;           r12 154:31
;
; the peak is in a function with a body of its own, compiled with all registers
; free, its callers are not on this stack
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
;     1      1  greet
;     1      6  inv
;     0      2  bar
;     0      1  faz
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
