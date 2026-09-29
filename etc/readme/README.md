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

* built-in integer types (64, 32, 16, 8 bit, 64 bit only on x86_64)
* built-in boolean type
* user defined types
* data
* variables with the type deduced from the initializer
* constants
* arrays
* array iteration
* string, character, record and array literals
* opt-in checks against undefined behavior
  * array bounds at runtime, optionally reporting the line number
  * non-inlined function frame capacity at runtime
  * compile time rejection of calls where a result or argument may share
    storage
* inlined functions
* limited support for non-inlined functions
* methods and constructors on user defined types
* partial ub-free support
* keywords: `func`, `noinline`, `type`, `dat`, `var`, `const`, `foo`, `loop`,
  `if`, `else`, `continue`, `break`, `return`, `self`, `and`, `or`, `not`
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `equal`, `read`,
  `write`, `exit`, `i`, `i8`, `i16`, `i32`, `i64`

## Howto

* `./make.sh` compiles the compiler then compiles and runs `prog.baz`,
  `./make.sh build` only compiles the compiler
* `./run.sh [options] [NAME.baz]` compiles, assembles and runs `NAME.baz`
  (default: `prog.baz`) passing options to `baz`, writes `NAME.s` and
  `NAME-without-comments.s`, x86_64 and rv32i also `NAME.o` and the binary
  `NAME`, rv32i-qemu and rv32i-fpga the image `NAME-TARGET.bin`, rv32i targets
  run in qemu user mode, the qemu virt machine or the fpga soft core emulator
  * `./run.sh myprogram.baz --checks=upper,line`
  * `./run.sh myprogram.baz --target=rv32i-qemu --stack=0x20000`
* `tutorial.baz` is a tour of the language from the easiest to the most
  difficult concepts
* `qa/coverage/test-all.sh` runs the tests, coverage report in
  `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

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
                        stack grows down from the end of the 8 mib memory,
                        fails when code, data, variables and stack do not fit
  --vars=SIZE         variable storage in bytes, decimal or 0x hex, must be a
                      multiple of 16 (default: 65536)
  --stack=SIZE        rv32i-qemu and rv32i-fpga stack in bytes, decimal or 0x
                      hex, must be a multiple of 16 (default: 65536)
  --checks=LIST       comma separated checks, replaces earlier --checks
  --bin=FILE          rv32i targets binary image (default: file without
                      extension followed by -MACHINE.bin)
  --nopt              no jump optimizations
  --reproduce-source  write reproduced source to diff.baz and check that it
                      matches the input
  --help, -h          this help

checks:
  upper  runtime upper array bounds, often enough to also catch negative
         indexes
  lower  runtime lower array bounds
  line   report line number on failed bounds check
  frame  runtime non-inlined function frame capacity
  alias  compile time rejection of calls where a result or argument may
         share storage
  noub   all checks against undefined behavior: upper, lower, frame and
         alias

examples:
  ./baz prog.baz > prog.s
  ./baz --vars=0x40000 --checks=upper prog.baz > prog.s
  ./baz --checks=upper,lower,line,frame prog.baz > prog.s
  ./baz --target=rv32i-qemu --stack=0x20000 prog.baz > prog.s
  ./baz --target=rv32i-fpga --checks=upper,line prog.baz > prog.s
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
C/C++ Header                    54           5520           2103          17554
C++                              1             66             19            329
-------------------------------------------------------------------------------
SUM:                            55           5586           2122          17883
-------------------------------------------------------------------------------
```

## Sample

```text
# user types are defined using keyword `type`

# built-in types are `i64`, `i32`, `i16`, `i8` and `bool`, `i64` only on x86_64

# default type is used if omitted (`i64` on x86_64 and `i32` on rv32i)

type point { x, y }

type object { pos point, color i32, }

type world { locations i[8] }
# `i` is the default integer type of the target platform

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
dat    nums = i[4]{ 1 } # remaining elements are zeroed
dat    str1 = str{ 3, "baz" } # a string initializes an `i8` array
                             # remaining fields are zeroed
dat greeted = "names greeted: "
dat   names = 0

# default is to inline functions

func assert(ok bool) { if not ok exit(1) }
# exit is a built-in function

func print(str i8[]) {
    write(1, str)
    # `write` is a built-in function that operates on file descriptors
    # it has 2 more optional arguments: count and start index
}

func point.fooz() {
    self.x = 0b10    # binary value 2
    self.y = 0xb     # hex value 11
}
# functions can act on user types: `func point.fooz()` is called as `p.fooz()`

func point.sum() res {
    res = self.x + self.y
}
# methods can have a "return"

func bar(arg) {
    if arg == 0 return
    arg = 0xff
}
# default function argument type is `i64` on x86_64 and `i32` on rv32i
# function arguments and "return" are equivalent to mutable references

func baz(arg) res {
    res = arg * 2
}
# return is a reference to the target with optional type
# it is accessed as a variable, in this case `res`

func inv(i i32) res i32 {
    res = ~i
}
# type of "return" and arguments can be defined, use `i` for default integer type
# of target platform

func faz(arg i32[]) {
    arg[1] = 0xfe
}
# array arguments are declared with the element type followed by `[]`, `i[]`
# for the default type

func str.input() {
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

func point.x(x) {
    self.x = x
}
# types can have methods with same name as fields

func object.at(x, y, color i32) self {
    self.pos = point.at(x, y)
    self.color = color
}

const yes = 1
const no = 0
const maybe = -1
# constants can be declared in any scope and shadow outer declarations

# limited support for non-inlined functions
# arguments and "return" are references to memory locations
# array arguments not supported

func noinline print_num(num) {
    # 19 digits of an i64 plus the sign
    const buf_count = 20

    var buf = i8[buf_count]{}
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
        const maybe = 33
        assert(maybe == 33)
    }

    assert(maybe == -1)

    assert(nums[0] == 1 and nums[3] == 0)
    assert(str1.len == 3 and str1.data[2] == 'z')

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

    assert(a != 0 and (a >= 7 or a < 0))
    # `and`, `or` and `not` stop evaluating as soon as the result is known

    var small = i8(100)
    small = small + small
    assert(small == -56)
    # `i8(100)` gives the variable type `i8`, arithmetic wraps at its width

    var wide = i(i16(small))
    assert(wide == -56)
    # `i(x)` converts to the default type

#   small = wide
#   compile time error because the value might not fit, narrowing must be
#   acknowledged with a conversion

    var letter = '\x41'
    assert(letter == 'A')
    # a character literal is a byte value, escapes `\n` and `\x41` are supported

    var arr = i32[4]{}

    var ix = 1
    arr[ix] = 2
    arr[ix + 1] = arr[ix]
    assert(arr[1] == 2)
    assert(arr[2] == 2)

    array_copy(arr[2], arr, 2)
    assert(arr[0] == 2)
    # `array_copy` is a built-in function: copy from, to, number of elements

    var arr1 = i32[8]{}
    array_copy(arr, arr1, 4)
    var eq = arrays_equal(arr[1], arr1[1], 3)
    # type `bool` is built-in and deduced from expression type
    # `arrays_equal` is built-in function comparing source and destination
    assert(eq)

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

    var arr4 = arr
    assert(equal(arr, arr4))
    # initializing from an array copies it, `equal` compares same size arrays

#   arr[ix] = ~inv(arr[ix - 1])
#   `--checks=alias` rejects this because "return" and the argument may share
#   storage

    ix = 3
    var tmp = ~inv(arr[ix - 1])
    arr[ix] = tmp
    assert(arr[ix] == 2)

    faz(arr)
    assert(arr[1] == 0xfe)

    var arr3 = i[]{ 3, 5 }
    foo arr3 {
        e = e + i + n
    }
    assert(arr3[0] == 3 + 0 + 2)
    assert(arr3[1] == 5 + 1 + 2)
    # `foo` is a language construct that iterates over an array injecting:
    #   `e`: current element
    #   `i`: index starting at 0
    #   `n`: constant array size

    var p = point{}

    p.fooz()
    # call on user type method

    assert(p.x == 2)
    assert(p.y == 0xb)

    var q = p
    # user type initializer may be an expression

    assert(equal(p, q))
    # `equal` is built-in function to compare user types for equality or same
    # size arrays

    q.x = 3
    assert(not equal(p, q))

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

    var o3 = object[2]{}
    o3[0].pos.y = 73

    assert(o3[0].pos.y == 73)
    o3[1] = object.at(2, 74, 0xffffff)
    assert(o3[1].pos.y == 74)

    o3[1].pos.fooz()
    assert(o3[1].pos.sum() == 13)
    # methods can be called on fields and array elements

    var worlds = world[8]{}
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

    var arr2 = i[]{ -1, 2 }
    assert(array_length(arr2) == 2)
    assert(arr2[0] == -1)
    assert(arr2[1] == 2)

    var counter = 0
    var nm = str{}
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
    mov qword [rbp + 256], 0
    cmp.182.12:
    cmp qword [rbp + 256], 0
    sete r15b
    bool.182.12.end:
    func.assert.182.5:
        if.37.27.182.5:
        cmp.37.27.182.5:
        cmp r15b, 0
        jne if.37.24.182.5.end
        if.37.27.182.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.182.5.end:
    func.assert.182.5.end:
    mov qword [rbp + 256], -1
    cmp.185.12:
    cmp qword [rbp + 256], -1
    sete r15b
    bool.185.12.end:
    func.assert.185.5:
        if.37.27.185.5:
        cmp.37.27.185.5:
        cmp r15b, 0
        jne if.37.24.185.5.end
        if.37.27.185.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.185.5.end:
    func.assert.185.5.end:
        func.assert.191.9:
            if.37.27.191.9:
            cmp.37.27.191.9:
            if.37.24.191.9.end:
        func.assert.191.9.end:
    func.assert.194.5:
        if.37.27.194.5:
        cmp.37.27.194.5:
        if.37.24.194.5.end:
    func.assert.194.5.end:
    cmp.196.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.196.12.end
    cmp.196.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.196.12.end:
    func.assert.196.5:
        if.37.27.196.5:
        cmp.37.27.196.5:
        cmp r15b, 0
        jne if.37.24.196.5.end
        if.37.27.196.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.196.5.end:
    func.assert.196.5.end:
    cmp.197.12:
    cmp byte [rbp + 96], 3
    sete r15b
    jne bool.197.12.end
    cmp.197.30:
    cmp byte [rbp + 99], 122
    sete r15b
    bool.197.12.end:
    func.assert.197.5:
        if.37.27.197.5:
        cmp.37.27.197.5:
        cmp r15b, 0
        jne if.37.24.197.5.end
        if.37.27.197.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.197.5.end:
    func.assert.197.5.end:
    mov qword [rbp + 264], 7
    cmp.200.12:
        mov r14, qword [rbp + 264]
        and r14, 3
    cmp r14, 3
    sete r15b
    bool.200.12.end:
    func.assert.200.5:
        if.37.27.200.5:
        cmp.37.27.200.5:
        cmp r15b, 0
        jne if.37.24.200.5.end
        if.37.27.200.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.200.5.end:
    func.assert.200.5.end:
    cmp.201.12:
        mov r14, qword [rbp + 264]
        or r14, 8
    cmp r14, 15
    sete r15b
    bool.201.12.end:
    func.assert.201.5:
        if.37.27.201.5:
        cmp.37.27.201.5:
        cmp r15b, 0
        jne if.37.24.201.5.end
        if.37.27.201.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.201.5.end:
    func.assert.201.5.end:
    cmp.202.12:
        mov r14, qword [rbp + 264]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.202.12.end:
    func.assert.202.5:
        if.37.27.202.5:
        cmp.37.27.202.5:
        cmp r15b, 0
        jne if.37.24.202.5.end
        if.37.27.202.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.202.5.end:
    func.assert.202.5.end:
    cmp.203.12:
        mov r14, qword [rbp + 264]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.203.12.end:
    func.assert.203.5:
        if.37.27.203.5:
        cmp.37.27.203.5:
        cmp r15b, 0
        jne if.37.24.203.5.end
        if.37.27.203.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.203.5.end:
    func.assert.203.5.end:
    cmp.204.12:
        mov r14, qword [rbp + 264]
        neg r14
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.204.12.end:
    func.assert.204.5:
        if.37.27.204.5:
        cmp.37.27.204.5:
        cmp r15b, 0
        jne if.37.24.204.5.end
        if.37.27.204.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.204.5.end:
    func.assert.204.5.end:
    cmp.207.12:
        mov r14, qword [rbp + 264]
        mov r13, qword [rbp + 264]
        sal r13, 1
        add r14, r13
    cmp r14, 21
    sete r15b
    bool.207.12.end:
    func.assert.207.5:
        if.37.27.207.5:
        cmp.37.27.207.5:
        cmp r15b, 0
        jne if.37.24.207.5.end
        if.37.27.207.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.207.5.end:
    func.assert.207.5.end:
    cmp.208.12:
        mov r14, qword [rbp + 264]
        add r14, qword [rbp + 264]
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.208.12.end:
    func.assert.208.5:
        if.37.27.208.5:
        cmp.37.27.208.5:
        cmp r15b, 0
        jne if.37.24.208.5.end
        if.37.27.208.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.208.5.end:
    func.assert.208.5.end:
    cmp.211.12:
    cmp qword [rbp + 264], 0
    setne r15b
    je bool.211.12.end
    cmp.211.23:
    cmp.211.24:
    cmp qword [rbp + 264], 7
    setge r15b
    jge bool.211.12.end
    cmp.211.34:
    cmp qword [rbp + 264], 0
    setl r15b
    bool.211.12.end:
    func.assert.211.5:
        if.37.27.211.5:
        cmp.37.27.211.5:
        cmp r15b, 0
        jne if.37.24.211.5.end
        if.37.27.211.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.211.5.end:
    func.assert.211.5.end:
    mov byte [rbp + 272], 100
    mov r15b, byte [rbp + 272]
    add r15b, byte [rbp + 272]
    mov byte [rbp + 272], r15b
    cmp.216.12:
    cmp byte [rbp + 272], -56
    sete r15b
    bool.216.12.end:
    func.assert.216.5:
        if.37.27.216.5:
        cmp.37.27.216.5:
        cmp r15b, 0
        jne if.37.24.216.5.end
        if.37.27.216.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.216.5.end:
    func.assert.216.5.end:
    movsx r15, byte [rbp + 272]
    mov qword [rbp + 280], r15
    cmp.220.12:
    cmp qword [rbp + 280], -56
    sete r15b
    bool.220.12.end:
    func.assert.220.5:
        if.37.27.220.5:
        cmp.37.27.220.5:
        cmp r15b, 0
        jne if.37.24.220.5.end
        if.37.27.220.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.220.5.end:
    func.assert.220.5.end:
    mov qword [rbp + 288], 65
    cmp.228.12:
    cmp qword [rbp + 288], 65
    sete r15b
    bool.228.12.end:
    func.assert.228.5:
        if.37.27.228.5:
        cmp.37.27.228.5:
        cmp r15b, 0
        jne if.37.24.228.5.end
        if.37.27.228.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.228.5.end:
    func.assert.228.5.end:
    mov qword [rbp + 296], 0
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 1
    mov r15, qword [rbp + 312]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 296], 2
    mov r15, qword [rbp + 312]
    add r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14, qword [rbp + 312]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 296]
    mov dword [rbp + r15 * 4 + 296], r13d
    cmp.236.12:
    cmp dword [rbp + 300], 2
    sete r15b
    bool.236.12.end:
    func.assert.236.5:
        if.37.27.236.5:
        cmp.37.27.236.5:
        cmp r15b, 0
        jne if.37.24.236.5.end
        if.37.27.236.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.236.5.end:
    func.assert.236.5.end:
    cmp.237.12:
    cmp dword [rbp + 304], 2
    sete r15b
    bool.237.12.end:
    func.assert.237.5:
        if.37.27.237.5:
        cmp.37.27.237.5:
        cmp r15b, 0
        jne if.37.24.237.5.end
        if.37.27.237.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.237.5.end:
    func.assert.237.5.end:
    mov r15, 2
    mov r14, 2
    test r14, r14
    js baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    mov r13, r15
    add r13, r14
    cmp r13, 4
    jg baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jg baz_bounds_panic
    mov rax, qword [rbp + r14 * 4 + 296]
    mov qword [rbp + 296], rax
    cmp.240.12:
    cmp dword [rbp + 296], 2
    sete r15b
    bool.240.12.end:
    func.assert.240.5:
        if.37.27.240.5:
        cmp.37.27.240.5:
        cmp r15b, 0
        jne if.37.24.240.5.end
        if.37.27.240.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.240.5.end:
    func.assert.240.5.end:
    mov qword [rbp + 320], 0
    mov qword [rbp + 328], 0
    mov qword [rbp + 336], 0
    mov qword [rbp + 344], 0
    mov r15, 4
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jg baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    cmp r15, 8
    jg baz_bounds_panic
    mov rax, qword [rbp + 296]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 328], rax
    cmp.245.14:
        mov rcx, 3
        mov r15, 1
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
        mov r14, rcx
        add r14, r15
        cmp r14, 4
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 296]
        mov r15, 1
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
        mov r14, rcx
        add r14, r15
        cmp r14, 8
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 320]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 352]
    bool.245.14.end:
    cmp.248.12:
    mov r15b, byte [rbp + 352]
    bool.248.12.end:
    func.assert.248.5:
        if.37.27.248.5:
        cmp.37.27.248.5:
        cmp r15b, 0
        jne if.37.24.248.5.end
        if.37.27.248.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.248.5.end:
    func.assert.248.5.end:
    mov dword [rbp + 328], -1
    cmp.251.12:
        mov rcx, 4
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 296]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 320]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.251.12.end:
    func.assert.251.5:
        if.37.27.251.5:
        cmp.37.27.251.5:
        cmp r15b, 0
        jne if.37.24.251.5.end
        if.37.27.251.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.251.5.end:
    func.assert.251.5.end:
    mov rax, qword [rbp + 296]
    mov qword [rbp + 356], rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 364], rax
    cmp.254.12:
        lea rsi, [rbp + 296]
        lea rdi, [rbp + 356]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.254.12.end:
    func.assert.254.5:
        if.37.27.254.5:
        cmp.37.27.254.5:
        cmp r15b, 0
        jne if.37.24.254.5.end
        if.37.27.254.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.254.5.end:
    func.assert.254.5.end:
    mov qword [rbp + 312], 3
    mov r15, qword [rbp + 312]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.262.16:
        mov r14d, dword [rbp + r15 * 4 + 296]
        mov dword [rbp + 372], r14d
        not dword [rbp + 372]
    func.inv.262.16.end:
    not dword [rbp + 372]
    mov r15, qword [rbp + 312]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 372]
    mov dword [rbp + r15 * 4 + 296], r14d
    cmp.264.12:
    mov r14, qword [rbp + 312]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 296], 2
    sete r15b
    bool.264.12.end:
    func.assert.264.5:
        if.37.27.264.5:
        cmp.37.27.264.5:
        cmp r15b, 0
        jne if.37.24.264.5.end
        if.37.27.264.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.264.5.end:
    func.assert.264.5.end:
    func.faz.266.5:
        mov dword [rbp + 300], 254
    func.faz.266.5.end:
    cmp.267.12:
    cmp dword [rbp + 300], 254
    sete r15b
    bool.267.12.end:
    func.assert.267.5:
        if.37.27.267.5:
        cmp.37.27.267.5:
        cmp r15b, 0
        jne if.37.24.267.5.end
        if.37.27.267.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.267.5.end:
    func.assert.267.5.end:
    mov qword [rbp + 376], 3
    mov qword [rbp + 384], 5
    lea r15, [rbp + 376]
    mov qword [rbp + 400], 0
    foo.270.5:
        mov r14, qword [rbp + 400]
        add qword [r15], r14
        add qword [r15], 2
        foo.270.5.continue:
            add r15, 8
            inc qword [rbp + 400]
            cmp qword [rbp + 400], 2
            jne foo.270.5
    foo.270.5.end:
    cmp.273.12:
    cmp qword [rbp + 376], 5
    sete r15b
    bool.273.12.end:
    func.assert.273.5:
        if.37.27.273.5:
        cmp.37.27.273.5:
        cmp r15b, 0
        jne if.37.24.273.5.end
        if.37.27.273.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.273.5.end:
    func.assert.273.5.end:
    cmp.274.12:
    cmp qword [rbp + 384], 8
    sete r15b
    bool.274.12.end:
    func.assert.274.5:
        if.37.27.274.5:
        cmp.37.27.274.5:
        cmp r15b, 0
        jne if.37.24.274.5.end
        if.37.27.274.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.274.5.end:
    func.assert.274.5.end:
    mov qword [rbp + 392], 0
    mov qword [rbp + 400], 0
    func.point.fooz.282.7:
        mov qword [rbp + 392], 2
        mov qword [rbp + 400], 11
    func.point.fooz.282.7.end:
    cmp.285.12:
    cmp qword [rbp + 392], 2
    sete r15b
    bool.285.12.end:
    func.assert.285.5:
        if.37.27.285.5:
        cmp.37.27.285.5:
        cmp r15b, 0
        jne if.37.24.285.5.end
        if.37.27.285.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.285.5.end:
    func.assert.285.5.end:
    cmp.286.12:
    cmp qword [rbp + 400], 11
    sete r15b
    bool.286.12.end:
    func.assert.286.5:
        if.37.27.286.5:
        cmp.37.27.286.5:
        cmp r15b, 0
        jne if.37.24.286.5.end
        if.37.27.286.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.286.5.end:
    func.assert.286.5.end:
    mov rax, qword [rbp + 392]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 400]
    mov qword [rbp + 416], rax
    cmp.291.12:
        lea rsi, [rbp + 392]
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    bool.291.12.end:
    func.assert.291.5:
        if.37.27.291.5:
        cmp.37.27.291.5:
        cmp r15b, 0
        jne if.37.24.291.5.end
        if.37.27.291.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.291.5.end:
    func.assert.291.5.end:
    mov qword [rbp + 408], 3
    cmp.296.12:
        lea rsi, [rbp + 392]
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    bool.296.12.end:
    func.assert.296.5:
        if.37.27.296.5:
        cmp.37.27.296.5:
        cmp r15b, 0
        jne if.37.24.296.5.end
        if.37.27.296.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.296.5.end:
    func.assert.296.5.end:
    mov qword [rbp + 424], 0
    func.bar.299.5:
        if.58.8.299.5:
        cmp.58.8.299.5:
        cmp qword [rbp + 424], 0
        je func.bar.299.5.end
        if.58.8.299.5.code:
        if.58.5.299.5.end:
        mov qword [rbp + 424], 255
    func.bar.299.5.end:
    cmp.300.12:
    cmp qword [rbp + 424], 0
    sete r15b
    bool.300.12.end:
    func.assert.300.5:
        if.37.27.300.5:
        cmp.37.27.300.5:
        cmp r15b, 0
        jne if.37.24.300.5.end
        if.37.27.300.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.300.5.end:
    func.assert.300.5.end:
    mov qword [rbp + 424], 1
    func.bar.303.5:
        if.58.8.303.5:
        cmp.58.8.303.5:
        cmp qword [rbp + 424], 0
        je func.bar.303.5.end
        if.58.8.303.5.code:
        if.58.5.303.5.end:
        mov qword [rbp + 424], 255
    func.bar.303.5.end:
    cmp.304.12:
    cmp qword [rbp + 424], 255
    sete r15b
    bool.304.12.end:
    func.assert.304.5:
        if.37.27.304.5:
        cmp.37.27.304.5:
        cmp r15b, 0
        jne if.37.24.304.5.end
        if.37.27.304.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.304.5.end:
    func.assert.304.5.end:
    mov qword [rbp + 432], 1
    func.baz.307.13:
        mov r15, qword [rbp + 432]
        mov qword [rbp + 440], r15
        sal qword [rbp + 440], 1
    func.baz.307.13.end:
    cmp.308.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.308.12.end:
    func.assert.308.5:
        if.37.27.308.5:
        cmp.37.27.308.5:
        cmp r15b, 0
        jne if.37.24.308.5.end
        if.37.27.308.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.308.5.end:
    func.assert.308.5.end:
    func.baz.310.9:
        mov qword [rbp + 440], 2
    func.baz.310.9.end:
    cmp.311.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.311.12.end:
    func.assert.311.5:
        if.37.27.311.5:
        cmp.37.27.311.5:
        cmp r15b, 0
        jne if.37.24.311.5.end
        if.37.27.311.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.311.5.end:
    func.assert.311.5.end:
    mov qword [rbp + 448], 5
    lea r15, [rbp + 464]
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
    lea r15, [rbp + 456]
    mov qword [rbp + 464], r15
    lea r15, [rbp + 448]
    mov qword [rbp + 472], r15
    lea rbx, [rbp + 464]
    call func.factorial
    cmp.315.12:
    cmp qword [rbp + 456], 120
    sete r15b
    bool.315.12.end:
    func.assert.315.5:
        if.37.27.315.5:
        cmp.37.27.315.5:
        cmp r15b, 0
        jne if.37.24.315.5.end
        if.37.27.315.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.315.5.end:
    func.assert.315.5.end:
    func.baz.317.20:
        mov qword [rbp + 464], 6
    func.baz.317.20.end:
    mov qword [rbp + 472], 0
    cmp.318.12:
    cmp qword [rbp + 464], 6
    sete r15b
    bool.318.12.end:
    func.assert.318.5:
        if.37.27.318.5:
        cmp.37.27.318.5:
        cmp r15b, 0
        jne if.37.24.318.5.end
        if.37.27.318.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.318.5.end:
    func.assert.318.5.end:
    func.point.at.320.14:
        mov qword [rbp + 480], -1
        mov qword [rbp + 488], -2
    func.point.at.320.14.end:
    cmp.324.12:
    cmp qword [rbp + 480], -1
    sete r15b
    bool.324.12.end:
    func.assert.324.5:
        if.37.27.324.5:
        cmp.37.27.324.5:
        cmp r15b, 0
        jne if.37.24.324.5.end
        if.37.27.324.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.324.5.end:
    func.assert.324.5.end:
    cmp.325.12:
    cmp qword [rbp + 488], -2
    sete r15b
    bool.325.12.end:
    func.assert.325.5:
        if.37.27.325.5:
        cmp.37.27.325.5:
        cmp r15b, 0
        jne if.37.24.325.5.end
        if.37.27.325.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.325.5.end:
    func.assert.325.5.end:
    func.point.x.327.8:
        mov qword [rbp + 480], 2
    func.point.x.327.8.end:
    cmp.328.12:
    cmp qword [rbp + 480], 2
    sete r15b
    bool.328.12.end:
    func.assert.328.5:
        if.37.27.328.5:
        cmp.37.27.328.5:
        cmp r15b, 0
        jne if.37.24.328.5.end
        if.37.27.328.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.328.5.end:
    func.assert.328.5.end:
    cmp.329.12:
        func.point.sum.329.15:
            mov r14, qword [rbp + 480]
            add r14, qword [rbp + 488]
        func.point.sum.329.15.end:
    cmp r14, 0
    sete r15b
    bool.329.12.end:
    func.assert.329.5:
        if.37.27.329.5:
        cmp.37.27.329.5:
        cmp r15b, 0
        jne if.37.24.329.5.end
        if.37.27.329.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.329.5.end:
    func.assert.329.5.end:
    mov qword [rbp + 496], 1
    mov qword [rbp + 504], 2
    mov r15, qword [rbp + 496]
    imul r15, 10
    mov qword [rbp + 512], r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 520], r15
    mov dword [rbp + 528], 16711680
    mov dword [rbp + 532], 0
    cmp.335.12:
    cmp qword [rbp + 512], 10
    sete r15b
    bool.335.12.end:
    func.assert.335.5:
        if.37.27.335.5:
        cmp.37.27.335.5:
        cmp r15b, 0
        jne if.37.24.335.5.end
        if.37.27.335.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.335.5.end:
    func.assert.335.5.end:
    cmp.336.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.336.12.end:
    func.assert.336.5:
        if.37.27.336.5:
        cmp.37.27.336.5:
        cmp r15b, 0
        jne if.37.24.336.5.end
        if.37.27.336.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.336.5.end:
    func.assert.336.5.end:
    cmp.337.12:
    cmp dword [rbp + 528], 16711680
    sete r15b
    bool.337.12.end:
    func.assert.337.5:
        if.37.27.337.5:
        cmp.37.27.337.5:
        cmp r15b, 0
        jne if.37.24.337.5.end
        if.37.27.337.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.337.5.end:
    func.assert.337.5.end:
    mov r15, qword [rbp + 496]
    mov qword [rbp + 536], r15
    neg qword [rbp + 536]
    mov r15, qword [rbp + 504]
    mov qword [rbp + 544], r15
    neg qword [rbp + 544]
    mov rax, qword [rbp + 536]
    mov qword [rbp + 512], rax
    mov rax, qword [rbp + 544]
    mov qword [rbp + 520], rax
    cmp.341.12:
    cmp qword [rbp + 512], -1
    sete r15b
    bool.341.12.end:
    func.assert.341.5:
        if.37.27.341.5:
        cmp.37.27.341.5:
        cmp r15b, 0
        jne if.37.24.341.5.end
        if.37.27.341.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.341.5.end:
    func.assert.341.5.end:
    cmp.342.12:
    cmp qword [rbp + 520], -2
    sete r15b
    bool.342.12.end:
    func.assert.342.5:
        if.37.27.342.5:
        cmp.37.27.342.5:
        cmp r15b, 0
        jne if.37.24.342.5.end
        if.37.27.342.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.342.5.end:
    func.assert.342.5.end:
    lea rsi, [rbp + 512]
    lea rdi, [rbp + 552]
    mov rcx, 24
    rep movsb
    cmp.345.12:
    cmp qword [rbp + 552], -1
    sete r15b
    bool.345.12.end:
    func.assert.345.5:
        if.37.27.345.5:
        cmp.37.27.345.5:
        cmp r15b, 0
        jne if.37.24.345.5.end
        if.37.27.345.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.345.5.end:
    func.assert.345.5.end:
    cmp.346.12:
    cmp qword [rbp + 560], -2
    sete r15b
    bool.346.12.end:
    func.assert.346.5:
        if.37.27.346.5:
        cmp.37.27.346.5:
        cmp r15b, 0
        jne if.37.24.346.5.end
        if.37.27.346.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.346.5.end:
    func.assert.346.5.end:
    cmp.347.12:
    cmp dword [rbp + 568], 16711680
    sete r15b
    bool.347.12.end:
    func.assert.347.5:
        if.37.27.347.5:
        cmp.37.27.347.5:
        cmp r15b, 0
        jne if.37.24.347.5.end
        if.37.27.347.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.347.5.end:
    func.assert.347.5.end:
    mov r15, qword [rbp + 496]
    mov qword [rbp + 552], r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 560], r15
    cmp.350.12:
    cmp qword [rbp + 552], 1
    sete r15b
    bool.350.12.end:
    func.assert.350.5:
        if.37.27.350.5:
        cmp.37.27.350.5:
        cmp r15b, 0
        jne if.37.24.350.5.end
        if.37.27.350.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.350.5.end:
    func.assert.350.5.end:
    mov r15, qword [rbp + 504]
    mov qword [rbp + 552], r15
    mov r15, qword [rbp + 496]
    mov qword [rbp + 560], r15
    cmp.352.12:
    cmp qword [rbp + 552], 2
    sete r15b
    bool.352.12.end:
    func.assert.352.5:
        if.37.27.352.5:
        cmp.37.27.352.5:
        cmp r15b, 0
        jne if.37.24.352.5.end
        if.37.27.352.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.352.5.end:
    func.assert.352.5.end:
    xor al, al
    lea rdi, [rbp + 576]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 584], 73
    cmp.362.12:
    cmp qword [rbp + 584], 73
    sete r15b
    bool.362.12.end:
    func.assert.362.5:
        if.37.27.362.5:
        cmp.37.27.362.5:
        cmp r15b, 0
        jne if.37.24.362.5.end
        if.37.27.362.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.362.5.end:
    func.assert.362.5.end:
    func.object.at.363.13:
        func.point.at.114.16.363.13:
            mov qword [rbp + 600], 2
            mov qword [rbp + 608], 74
        func.point.at.114.16.363.13.end:
        mov dword [rbp + 616], 16777215
    func.object.at.363.13.end:
    cmp.364.12:
    cmp qword [rbp + 608], 74
    sete r15b
    bool.364.12.end:
    func.assert.364.5:
        if.37.27.364.5:
        cmp.37.27.364.5:
        cmp r15b, 0
        jne if.37.24.364.5.end
        if.37.27.364.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.364.5.end:
    func.assert.364.5.end:
    func.point.fooz.366.15:
        mov qword [rbp + 600], 2
        mov qword [rbp + 608], 11
    func.point.fooz.366.15.end:
    cmp.367.12:
        func.point.sum.367.22:
            mov r14, qword [rbp + 600]
            add r14, qword [rbp + 608]
        func.point.sum.367.22.end:
    cmp r14, 13
    sete r15b
    bool.367.12.end:
    func.assert.367.5:
        if.37.27.367.5:
        cmp.37.27.367.5:
        cmp r15b, 0
        jne if.37.24.367.5.end
        if.37.27.367.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.367.5.end:
    func.assert.367.5.end:
    xor al, al
    lea rdi, [rbp + 624]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 696], 65518
    cmp.372.12:
    cmp qword [rbp + 696], 65518
    sete r15b
    bool.372.12.end:
    func.assert.372.5:
        if.37.27.372.5:
        cmp.37.27.372.5:
        cmp r15b, 0
        jne if.37.24.372.5.end
        if.37.27.372.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.372.5.end:
    func.assert.372.5.end:
    mov rcx, 8
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 688]
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 624]
    shl rcx, 3
    rep movsb
    cmp.381.12:
    cmp qword [rbp + 632], 65518
    sete r15b
    bool.381.12.end:
    func.assert.381.5:
        if.37.27.381.5:
        cmp.37.27.381.5:
        cmp r15b, 0
        jne if.37.24.381.5.end
        if.37.27.381.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.381.5.end:
    func.assert.381.5.end:
    cmp.382.12:
        mov rcx, 8
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 624]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 688]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.382.12.end:
    func.assert.382.5:
        if.37.27.382.5:
        cmp.37.27.382.5:
        cmp r15b, 0
        jne if.37.24.382.5.end
        if.37.27.382.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.382.5.end:
    func.assert.382.5.end:
    mov qword [rbp + 1136], -1
    mov qword [rbp + 1144], 2
    cmp.389.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.389.12.end:
    func.assert.389.5:
        if.37.27.389.5:
        cmp.37.27.389.5:
        cmp r15b, 0
        jne if.37.24.389.5.end
        if.37.27.389.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.389.5.end:
    func.assert.389.5.end:
    cmp.390.12:
    cmp qword [rbp + 1136], -1
    sete r15b
    bool.390.12.end:
    func.assert.390.5:
        if.37.27.390.5:
        cmp.37.27.390.5:
        cmp r15b, 0
        jne if.37.24.390.5.end
        if.37.27.390.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.390.5.end:
    func.assert.390.5.end:
    cmp.391.12:
    cmp qword [rbp + 1144], 2
    sete r15b
    bool.391.12.end:
    func.assert.391.5:
        if.37.27.391.5:
        cmp.37.27.391.5:
        cmp r15b, 0
        jne if.37.24.391.5.end
        if.37.27.391.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.391.5.end:
    func.assert.391.5.end:
    mov qword [rbp + 1152], 0
    xor al, al
    lea rdi, [rbp + 1160]
    mov rcx, 128
    rep stosb
    func.print.395.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.395.5.end:
    loop.396.5:
        add qword [rbp + 1152], 1
        lea r15, [rbp + 1288]
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
        lea r15, [rbp + 1152]
        mov qword [rbp + 1288], r15
        lea rbx, [rbp + 1288]
        call func.print_num
        func.print.399.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.399.9.end:
        func.print.400.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.400.9.end:
        func.str.input.401.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1161]
            mov rax, 0
            syscall
            mov qword [rbp + 1288], rax
            mov r15b, byte [rbp + 1288]
            mov byte [rbp + 1160], r15b
            sub byte [rbp + 1160], 1
        func.str.input.401.12.end:
        if.403.12:
        cmp.403.12:
        cmp byte [rbp + 1160], 0
        jle loop.396.5.end
        if.403.12.code:
        if.405.19:
        cmp.405.19:
        cmp byte [rbp + 1160], 4
        jg if.403.9.else
        if.405.19.code:
            func.print.406.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.406.13.end:
            jmp loop.396.5
        if.403.9.else:
            func.greet.409.13:
                func.print.94.5.409.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.94.5.409.13.end:
                func.str.print.95.10.409.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1160]
                    test rdx, rdx
                    js baz_bounds_panic
                    cmp rdx, 127
                    jg baz_bounds_panic
                    lea rsi, [rbp + 1161]
                    mov rax, 1
                    syscall
                func.str.print.95.10.409.13.end:
                func.print.96.5.409.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.96.5.409.13.end:
                func.print.97.5.409.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.97.5.409.13.end:
                add qword [rbp + 240], 1
            func.greet.409.13.end:
        if.403.9.end:
    jmp loop.396.5
    loop.396.5.end:
    func.print.413.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 224]
        mov rax, 1
        syscall
    func.print.413.5.end:
    lea r15, [rbp + 1288]
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
    lea r15, [rbp + 240]
    mov qword [rbp + 1288], r15
    lea rbx, [rbp + 1288]
    call func.print_num
    func.print.415.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.415.5.end:
    mov dword [rbp + 1288], 543521122
    mov dword [rbp + 1292], 1836020326
    mov dword [rbp + 1296], 2053202464
    mov byte [rbp + 1300], 10
    mov rdi, 1
    mov rdx, 3
    test rdx, rdx
    js baz_bounds_panic
    cmp rdx, 13
    jg baz_bounds_panic
    lea rsi, [rbp + 1288]
    mov rax, 1
    syscall
    mov rdi, 1
    mov rdx, 1
    mov r15, 13
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    test rdx, rdx
    js baz_bounds_panic
    mov r14, rdx
    add r14, r15
    cmp r14, 13
    jg baz_bounds_panic
    lea rsi, [rbp + 1288]
    add rsi, r15
    mov rax, 1
    syscall
    mov rdi, 0
    mov rax, 60
    syscall
func.print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
    mov byte [rbx + 40], 0
    if.137.8:
    cmp.137.8:
    cmp qword [rbx + 32], 0
    jge if.137.5.end
    if.137.8.code:
        mov byte [rbx + 40], 1
    if.137.5.end:
    if.140.8:
    cmp.140.8:
    cmp qword [rbx + 32], 0
    jle if.140.5.end
    if.140.8.code:
        neg qword [rbx + 32]
    if.140.5.end:
    mov qword [rbx + 48], 20
    loop.145.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
            mov r14, 48
            mov r13, qword [rbx + 32]
            mov rax, r13
            cqo
            mov r12, 10
            idiv r12
            mov r13, rdx
            sub r14, r13
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 32]
        cqo
        mov r15, 10
        idiv r15
        mov qword [rbx + 32], rax
        if.149.12:
        cmp.149.12:
        cmp qword [rbx + 32], 0
        jne loop.145.5
        if.149.12.code:
        if.149.9.end:
    loop.145.5.end:
    if.152.8:
    cmp.152.8:
    cmp byte [rbx + 40], 0
    je if.152.5.end
    if.152.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.152.5.end:
    mov qword [rbx + 56], 0
    loop.158.5:
        mov r15, qword [rbx + 56]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        test r14, r14
        js baz_bounds_panic
        cmp r14, 20
        jge baz_bounds_panic
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 56], 1
        add qword [rbx + 48], 1
        if.162.12:
        cmp.162.12:
        cmp qword [rbx + 48], 20
        jne loop.158.5
        if.162.12.code:
        if.162.9.end:
    loop.158.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    test rdx, rdx
    js baz_bounds_panic
    cmp rdx, 20
    jg baz_bounds_panic
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.170.8:
    cmp.170.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.170.5.end
    if.170.8.code:
        ret
    if.170.5.end:
    mov r15, qword [rbx + 8]
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
    sub qword [rbx + 16], 1
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
    mov qword [r15], r14
    ret
size.func.factorial equ 32
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
baz_bounds_panic:
    mov rax, 60
    mov rdi, 255
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
;[29:1] dat nums = i[4]{ 1 }
;[29:8] nums: i64[4] (32 B @ [rbp + 64])
;[30:1] dat str1 = str{ 3, "baz" }
;[30:8] str1: str (128 B @ [rbp + 96])
;[32:1] dat greeted = "names greeted: "
;[32:5] greeted: i8[15] (15 B @ [rbp + 224])
;[33:1] dat names = 0
;[33:7] names: i64 (8 B @ [rbp + 240])
;[118:7] const yes = 1
;[119:7] const no = 0
;[120:7] const maybe = -1
;
main:
;   [180:5] var answer = 0
;   [180:9] answer: i64 (8 B @ [rbp + 256])
;   [180:9] answer = 0
;   [180:18] 0
    mov qword [rbp + 256], 0
;   [182:5] assert(answer == 0)
;   [182:12] allocate scratch register -> r15
;   [182:12] ? answer == 0
;   [182:12] ? answer == 0
    cmp.182.12:
    cmp qword [rbp + 256], 0
    sete r15b
    bool.182.12.end:
;   [37:6] assert(ok bool)
    func.assert.182.5:
;       [182:5] alias ok -> r15b
        if.37.27.182.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.182.5:
        cmp r15b, 0
        jne if.37.24.182.5.end
        if.37.27.182.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.182.5.end:
;       [182:5] free scratch register r15
    func.assert.182.5.end:
;   [184:5] answer = maybe
;   [184:14] maybe
    mov qword [rbp + 256], -1
;   [185:5] assert(answer == -1)
;   [185:12] allocate scratch register -> r15
;   [185:12] ? answer == -1
;   [185:12] ? answer == -1
    cmp.185.12:
    cmp qword [rbp + 256], -1
    sete r15b
    bool.185.12.end:
;   [37:6] assert(ok bool)
    func.assert.185.5:
;       [185:5] alias ok -> r15b
        if.37.27.185.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.185.5:
        cmp r15b, 0
        jne if.37.24.185.5.end
        if.37.27.185.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.185.5.end:
;       [185:5] free scratch register r15
    func.assert.185.5.end:
;       [190:15] const maybe = 33
;       [191:9] assert(maybe == 33)
;       [37:6] assert(ok bool)
        func.assert.191.9:
;           [191:9] alias ok -> 1
            if.37.27.191.9:
;           [37:27] ? not ok
;           [37:27] ? shorthand: not ok
            cmp.37.27.191.9:
;           [37:31] const eval to false
            if.37.24.191.9.end:
        func.assert.191.9.end:
;   [194:5] assert(maybe == -1)
;   [37:6] assert(ok bool)
    func.assert.194.5:
;       [194:5] alias ok -> 1
        if.37.27.194.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.194.5:
;       [37:31] const eval to false
        if.37.24.194.5.end:
    func.assert.194.5.end:
;   [196:5] assert(nums[0] == 1 and nums[3] == 0)
;   [196:12] allocate scratch register -> r15
;   [196:12] ? nums[0] == 1 and nums[3] == 0
;   [196:12] ? nums[0] == 1
    cmp.196.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.196.12.end
;   [196:29] ? nums[3] == 0
    cmp.196.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.196.12.end:
;   [37:6] assert(ok bool)
    func.assert.196.5:
;       [196:5] alias ok -> r15b
        if.37.27.196.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.196.5:
        cmp r15b, 0
        jne if.37.24.196.5.end
        if.37.27.196.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.196.5.end:
;       [196:5] free scratch register r15
    func.assert.196.5.end:
;   [197:5] assert(str1.len == 3 and str1.data[2] == 'z')
;   [197:12] allocate scratch register -> r15
;   [197:12] ? str1.len == 3 and str1.data[2] == 'z'
;   [197:12] ? str1.len == 3
    cmp.197.12:
    cmp byte [rbp + 96], 3
    sete r15b
    jne bool.197.12.end
;   [197:30] ? str1.data[2] == 'z'
    cmp.197.30:
    cmp byte [rbp + 99], 122
    sete r15b
    bool.197.12.end:
;   [37:6] assert(ok bool)
    func.assert.197.5:
;       [197:5] alias ok -> r15b
        if.37.27.197.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.197.5:
        cmp r15b, 0
        jne if.37.24.197.5.end
        if.37.27.197.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.197.5.end:
;       [197:5] free scratch register r15
    func.assert.197.5.end:
;   [199:5] var a = 7
;   [199:9] a: i64 (8 B @ [rbp + 264])
;   [199:9] a = 7
;   [199:13] 7
    mov qword [rbp + 264], 7
;   [200:5] assert(a & 3 == 3)
;   [200:12] allocate scratch register -> r15
;   [200:12] ? a & 3 == 3
;   [200:12] ? a & 3 == 3
    cmp.200.12:
;   [200:12] allocate scratch register -> r14
;       [200:12] a
        mov r14, qword [rbp + 264]
;       [200:12] r14 & 3
;       [200:12] src: folded constant '& 3'
        and r14, 3
    cmp r14, 3
;   [200:12] free scratch register r14
    sete r15b
    bool.200.12.end:
;   [37:6] assert(ok bool)
    func.assert.200.5:
;       [200:5] alias ok -> r15b
        if.37.27.200.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.200.5:
        cmp r15b, 0
        jne if.37.24.200.5.end
        if.37.27.200.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.200.5.end:
;       [200:5] free scratch register r15
    func.assert.200.5.end:
;   [201:5] assert(a | 8 == 15)
;   [201:12] allocate scratch register -> r15
;   [201:12] ? a | 8 == 15
;   [201:12] ? a | 8 == 15
    cmp.201.12:
;   [201:12] allocate scratch register -> r14
;       [201:12] a
        mov r14, qword [rbp + 264]
;       [201:12] r14 | 8
;       [201:12] src: folded constant '| 8'
        or r14, 8
    cmp r14, 15
;   [201:12] free scratch register r14
    sete r15b
    bool.201.12.end:
;   [37:6] assert(ok bool)
    func.assert.201.5:
;       [201:5] alias ok -> r15b
        if.37.27.201.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.201.5:
        cmp r15b, 0
        jne if.37.24.201.5.end
        if.37.27.201.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.201.5.end:
;       [201:5] free scratch register r15
    func.assert.201.5.end:
;   [202:5] assert(a ^ 1 == 6)
;   [202:12] allocate scratch register -> r15
;   [202:12] ? a ^ 1 == 6
;   [202:12] ? a ^ 1 == 6
    cmp.202.12:
;   [202:12] allocate scratch register -> r14
;       [202:12] a
        mov r14, qword [rbp + 264]
;       [202:12] r14 ^ 1
;       [202:12] src: folded constant '^ 1'
        xor r14, 1
    cmp r14, 6
;   [202:12] free scratch register r14
    sete r15b
    bool.202.12.end:
;   [37:6] assert(ok bool)
    func.assert.202.5:
;       [202:5] alias ok -> r15b
        if.37.27.202.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.202.5:
        cmp r15b, 0
        jne if.37.24.202.5.end
        if.37.27.202.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.202.5.end:
;       [202:5] free scratch register r15
    func.assert.202.5.end:
;   [203:5] assert(a << 2 == 28)
;   [203:12] allocate scratch register -> r15
;   [203:12] ? a << 2 == 28
;   [203:12] ? a << 2 == 28
    cmp.203.12:
;   [203:12] allocate scratch register -> r14
;       [203:12] a
        mov r14, qword [rbp + 264]
;       [203:17] r14 << 2
;       [203:17] src: constant
        sal r14, 2
    cmp r14, 28
;   [203:12] free scratch register r14
    sete r15b
    bool.203.12.end:
;   [37:6] assert(ok bool)
    func.assert.203.5:
;       [203:5] alias ok -> r15b
        if.37.27.203.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.203.5:
        cmp r15b, 0
        jne if.37.24.203.5.end
        if.37.27.203.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.203.5.end:
;       [203:5] free scratch register r15
    func.assert.203.5.end:
;   [204:5] assert(-a >> 1 == -4)
;   [204:12] allocate scratch register -> r15
;   [204:12] ? -a >> 1 == -4
;   [204:12] ? -a >> 1 == -4
    cmp.204.12:
;   [204:12] allocate scratch register -> r14
;       [204:13] -a
        mov r14, qword [rbp + 264]
        neg r14
;       [204:18] r14 >> 1
;       [204:18] src: constant
        sar r14, 1
    cmp r14, -4
;   [204:12] free scratch register r14
    sete r15b
    bool.204.12.end:
;   [37:6] assert(ok bool)
    func.assert.204.5:
;       [204:5] alias ok -> r15b
        if.37.27.204.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.204.5:
        cmp r15b, 0
        jne if.37.24.204.5.end
        if.37.27.204.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.204.5.end:
;       [204:5] free scratch register r15
    func.assert.204.5.end:
;   [207:5] assert(a + a << 1 == 21)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? a + a << 1 == 21
;   [207:12] ? a + a << 1 == 21
    cmp.207.12:
;   [207:12] allocate scratch register -> r14
;       [207:12] a
        mov r14, qword [rbp + 264]
;       [207:18] r14 + a << 1
;       [207:18] src: expression
;       [207:18] allocate scratch register -> r13
;       [207:16] a
        mov r13, qword [rbp + 264]
;       [207:21] r13 << 1
;       [207:21] src: constant
        sal r13, 1
        add r14, r13
;       [207:18] free scratch register r13
    cmp r14, 21
;   [207:12] free scratch register r14
    sete r15b
    bool.207.12.end:
;   [37:6] assert(ok bool)
    func.assert.207.5:
;       [207:5] alias ok -> r15b
        if.37.27.207.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.207.5:
        cmp r15b, 0
        jne if.37.24.207.5.end
        if.37.27.207.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.207.5.end:
;       [207:5] free scratch register r15
    func.assert.207.5.end:
;   [208:5] assert((a + a) << 1 == 28)
;   [208:12] allocate scratch register -> r15
;   [208:12] ? (a + a) << 1 == 28
;   [208:12] ? (a + a) << 1 == 28
    cmp.208.12:
;   [208:12] allocate scratch register -> r14
;       [208:13] r14 = (a + a)
;       [208:13] = expression
;       [208:13] a
        mov r14, qword [rbp + 264]
;       [208:17] r14 + a
;       [208:17] src: operand
        add r14, qword [rbp + 264]
;       [208:23] r14 << 1
;       [208:23] src: constant
        sal r14, 1
    cmp r14, 28
;   [208:12] free scratch register r14
    sete r15b
    bool.208.12.end:
;   [37:6] assert(ok bool)
    func.assert.208.5:
;       [208:5] alias ok -> r15b
        if.37.27.208.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.208.5:
        cmp r15b, 0
        jne if.37.24.208.5.end
        if.37.27.208.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.208.5.end:
;       [208:5] free scratch register r15
    func.assert.208.5.end:
;   [211:5] assert(a != 0 and (a >= 7 or a < 0))
;   [211:12] allocate scratch register -> r15
;   [211:12] ? a != 0 and (a >= 7 or a < 0)
;   [211:12] ? a != 0
    cmp.211.12:
    cmp qword [rbp + 264], 0
    setne r15b
    je bool.211.12.end
    cmp.211.23:
;   [211:23] ? (a >= 7 or a < 0)
;   [211:24] ? a >= 7
    cmp.211.24:
    cmp qword [rbp + 264], 7
    setge r15b
    jge bool.211.12.end
;   [211:34] ? a < 0
    cmp.211.34:
    cmp qword [rbp + 264], 0
    setl r15b
    bool.211.12.end:
;   [37:6] assert(ok bool)
    func.assert.211.5:
;       [211:5] alias ok -> r15b
        if.37.27.211.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.211.5:
        cmp r15b, 0
        jne if.37.24.211.5.end
        if.37.27.211.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.211.5.end:
;       [211:5] free scratch register r15
    func.assert.211.5.end:
;   [214:5] var small = i8(100)
;   [214:9] small: i8 (1 B @ [rbp + 272])
;   [214:9] small = i8(100)
    mov byte [rbp + 272], 100
;   [215:5] small = small + small
;   [215:13] instructions without scratch register 3, with 4
;   [215:13] allocate scratch register -> r15
;   [215:13] small
    mov r15b, byte [rbp + 272]
;   [215:21] r15b + small
;   [215:21] src: operand
    add r15b, byte [rbp + 272]
    mov byte [rbp + 272], r15b
;   [215:13] free scratch register r15
;   [216:5] assert(small == -56)
;   [216:12] allocate scratch register -> r15
;   [216:12] ? small == -56
;   [216:12] ? small == -56
    cmp.216.12:
    cmp byte [rbp + 272], -56
    sete r15b
    bool.216.12.end:
;   [37:6] assert(ok bool)
    func.assert.216.5:
;       [216:5] alias ok -> r15b
        if.37.27.216.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.216.5:
        cmp r15b, 0
        jne if.37.24.216.5.end
        if.37.27.216.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.216.5.end:
;       [216:5] free scratch register r15
    func.assert.216.5.end:
;   [219:5] var wide = i(i16(small))
;   [219:9] wide: i64 (8 B @ [rbp + 280])
;   [219:9] wide = i(i16(small))
;   [219:16] wide = i(i16(small))
;   [219:16] = expression
;   [219:16] instructions without scratch register 2, with 3
;   [219:18] wide = i16(small)
;   [219:18] = expression
;   [219:18] instructions without scratch register 2, with 3
;   [219:22] small
;   [219:22] allocate scratch register -> r15
    movsx r15, byte [rbp + 272]
    mov qword [rbp + 280], r15
;   [219:22] free scratch register r15
;   [220:5] assert(wide == -56)
;   [220:12] allocate scratch register -> r15
;   [220:12] ? wide == -56
;   [220:12] ? wide == -56
    cmp.220.12:
    cmp qword [rbp + 280], -56
    sete r15b
    bool.220.12.end:
;   [37:6] assert(ok bool)
    func.assert.220.5:
;       [220:5] alias ok -> r15b
        if.37.27.220.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.220.5:
        cmp r15b, 0
        jne if.37.24.220.5.end
        if.37.27.220.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.220.5.end:
;       [220:5] free scratch register r15
    func.assert.220.5.end:
;   [227:5] var letter = '\x41'
;   [227:9] letter: i64 (8 B @ [rbp + 288])
;   [227:9] letter = '\x41'
;   [227:18] '\x41'
    mov qword [rbp + 288], 65
;   [228:5] assert(letter == 'A')
;   [228:12] allocate scratch register -> r15
;   [228:12] ? letter == 'A'
;   [228:12] ? letter == 'A'
    cmp.228.12:
    cmp qword [rbp + 288], 65
    sete r15b
    bool.228.12.end:
;   [37:6] assert(ok bool)
    func.assert.228.5:
;       [228:5] alias ok -> r15b
        if.37.27.228.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.228.5:
        cmp r15b, 0
        jne if.37.24.228.5.end
        if.37.27.228.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.228.5.end:
;       [228:5] free scratch register r15
    func.assert.228.5.end:
;   [231:5] var arr = i32[4]{}
;   [231:9] arr: i32[4] (16 B @ [rbp + 296])
;   [231:9] arr = i32[4]{}
;   [231:15] zero remaining elements: 4 * 4 B = 16 B
;   [231:15] size <= 32 B, use mov
    mov qword [rbp + 296], 0
    mov qword [rbp + 304], 0
;   [233:5] var ix = 1
;   [233:9] ix: i64 (8 B @ [rbp + 312])
;   [233:9] ix = 1
;   [233:14] 1
    mov qword [rbp + 312], 1
;   [234:5] arr[ix] = 2
;   [234:9] allocate scratch register -> r15
;   [234:9] set array index
;   [234:9] ix
    mov r15, qword [rbp + 312]
;   [234:9] bounds check
;   [234:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [234:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [234:15] 2
    mov dword [rbp + r15 * 4 + 296], 2
;   [234:5] free scratch register r15
;   [235:5] arr[ix + 1] = arr[ix]
;   [235:9] allocate scratch register -> r15
;   [235:9] set array index
;   [235:9] ix
    mov r15, qword [rbp + 312]
;   [235:9] r15 + 1
;   [235:9] src: folded constant '+ 1'
    add r15, 1
;   [235:9] bounds check
;   [235:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [235:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [235:19] arr[ix]
;   [235:23] allocate scratch register -> r14
;   [235:23] set array index
;   [235:23] ix
    mov r14, qword [rbp + 312]
;   [235:23] bounds check
;   [235:23] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
;   [235:23] upper bound (--checks=upper)
    cmp r14, 4
    jge baz_bounds_panic
;   [235:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 296]
    mov dword [rbp + r15 * 4 + 296], r13d
;   [235:19] free scratch register r13
;   [235:19] free scratch register r14
;   [235:5] free scratch register r15
;   [236:5] assert(arr[1] == 2)
;   [236:12] allocate scratch register -> r15
;   [236:12] ? arr[1] == 2
;   [236:12] ? arr[1] == 2
    cmp.236.12:
    cmp dword [rbp + 300], 2
    sete r15b
    bool.236.12.end:
;   [37:6] assert(ok bool)
    func.assert.236.5:
;       [236:5] alias ok -> r15b
        if.37.27.236.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.236.5:
        cmp r15b, 0
        jne if.37.24.236.5.end
        if.37.27.236.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.236.5.end:
;       [236:5] free scratch register r15
    func.assert.236.5.end:
;   [237:5] assert(arr[2] == 2)
;   [237:12] allocate scratch register -> r15
;   [237:12] ? arr[2] == 2
;   [237:12] ? arr[2] == 2
    cmp.237.12:
    cmp dword [rbp + 304], 2
    sete r15b
    bool.237.12.end:
;   [37:6] assert(ok bool)
    func.assert.237.5:
;       [237:5] alias ok -> r15b
        if.37.27.237.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.237.5:
        cmp r15b, 0
        jne if.37.24.237.5.end
        if.37.27.237.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.237.5.end:
;       [237:5] free scratch register r15
    func.assert.237.5.end:
;   [239:5] array_copy(arr[2], arr, 2)
;   [239:5] allocate scratch register -> r15
;   [239:29] 2
;   [239:29] 2
    mov r15, 2
;   [239:16] arr[2]
;   [239:20] allocate scratch register -> r14
;   [239:20] set array index
;   [239:20] 2
    mov r14, 2
;   [239:20] bounds check
;   [239:20] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
;   [239:20] upper bound (--checks=upper)
;   [239:20] allocate scratch register -> r13
    mov r13, r15
    add r13, r14
    cmp r13, 4
;   [239:20] free scratch register r13
    jg baz_bounds_panic
;   [239:24] arr
;   [239:24] bounds check
;   [239:24] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [239:24] upper bound (--checks=upper)
    cmp r15, 4
    jg baz_bounds_panic
;   [239:5] size <= 16 B, use mov
;   [239:5] allocate named register rax
    mov rax, qword [rbp + r14 * 4 + 296]
    mov qword [rbp + 296], rax
;   [239:5] free named register rax
;   [239:5] free scratch register r14
;   [239:5] free scratch register r15
;   [240:5] assert(arr[0] == 2)
;   [240:12] allocate scratch register -> r15
;   [240:12] ? arr[0] == 2
;   [240:12] ? arr[0] == 2
    cmp.240.12:
    cmp dword [rbp + 296], 2
    sete r15b
    bool.240.12.end:
;   [37:6] assert(ok bool)
    func.assert.240.5:
;       [240:5] alias ok -> r15b
        if.37.27.240.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.240.5:
        cmp r15b, 0
        jne if.37.24.240.5.end
        if.37.27.240.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.240.5.end:
;       [240:5] free scratch register r15
    func.assert.240.5.end:
;   [243:5] var arr1 = i32[8]{}
;   [243:9] arr1: i32[8] (32 B @ [rbp + 320])
;   [243:9] arr1 = i32[8]{}
;   [243:16] zero remaining elements: 8 * 4 B = 32 B
;   [243:16] size <= 32 B, use mov
    mov qword [rbp + 320], 0
    mov qword [rbp + 328], 0
    mov qword [rbp + 336], 0
    mov qword [rbp + 344], 0
;   [244:5] array_copy(arr, arr1, 4)
;   [244:5] allocate scratch register -> r15
;   [244:27] 4
;   [244:27] 4
    mov r15, 4
;   [244:16] arr
;   [244:16] bounds check
;   [244:16] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [244:16] upper bound (--checks=upper)
    cmp r15, 4
    jg baz_bounds_panic
;   [244:21] arr1
;   [244:21] bounds check
;   [244:21] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [244:21] upper bound (--checks=upper)
    cmp r15, 8
    jg baz_bounds_panic
;   [244:5] size <= 16 B, use mov
;   [244:5] allocate named register rax
    mov rax, qword [rbp + 296]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 328], rax
;   [244:5] free named register rax
;   [244:5] free scratch register r15
;   [245:5] var eq = arrays_equal(arr[1], arr1[1], 3)
;   [245:9] eq: bool (1 B @ [rbp + 352])
;   [245:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [245:14] ? arrays_equal(arr[1], arr1[1], 3)
;   [245:14] ? shorthand: arrays_equal(arr[1], arr1[1], 3)
    cmp.245.14:
;       [245:14] arrays_equal(arr[1], arr1[1], 3)
;       [245:14] allocate named register rsi
;       [245:14] allocate named register rdi
;       [245:14] allocate named register rcx
;       [245:44] 3
;       [245:44] 3
        mov rcx, 3
;       [245:27] arr[1]
;       [245:31] allocate scratch register -> r15
;       [245:31] set array index
;       [245:31] 1
        mov r15, 1
;       [245:31] bounds check
;       [245:31] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
;       [245:31] upper bound (--checks=upper)
;       [245:31] allocate scratch register -> r14
        mov r14, rcx
        add r14, r15
        cmp r14, 4
;       [245:31] free scratch register r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 296]
;       [245:14] free scratch register r15
;       [245:35] arr1[1]
;       [245:40] allocate scratch register -> r15
;       [245:40] set array index
;       [245:40] 1
        mov r15, 1
;       [245:40] bounds check
;       [245:40] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
;       [245:40] upper bound (--checks=upper)
;       [245:40] allocate scratch register -> r14
        mov r14, rcx
        add r14, r15
        cmp r14, 8
;       [245:40] free scratch register r14
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 320]
;       [245:14] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [245:14] free named register rcx
;       [245:14] free named register rdi
;       [245:14] free named register rsi
        sete byte [rbp + 352]
    bool.245.14.end:
;   [248:5] assert(eq)
;   [248:12] allocate scratch register -> r15
;   [248:12] ? eq
;   [248:12] ? shorthand: eq
    cmp.248.12:
    mov r15b, byte [rbp + 352]
    bool.248.12.end:
;   [37:6] assert(ok bool)
    func.assert.248.5:
;       [248:5] alias ok -> r15b
        if.37.27.248.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.248.5:
        cmp r15b, 0
        jne if.37.24.248.5.end
        if.37.27.248.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.248.5.end:
;       [248:5] free scratch register r15
    func.assert.248.5.end:
;   [250:5] arr1[2] = -1
;   [250:15] instructions without scratch register 1, with 2
;   [250:16] -1
    mov dword [rbp + 328], -1
;   [251:5] assert(not arrays_equal(arr, arr1, 4))
;   [251:12] allocate scratch register -> r15
;   [251:12] ? not arrays_equal(arr, arr1, 4)
;   [251:12] ? shorthand: not arrays_equal(arr, arr1, 4)
    cmp.251.12:
;       [251:16] arrays_equal(arr, arr1, 4)
;       [251:16] allocate named register rsi
;       [251:16] allocate named register rdi
;       [251:16] allocate named register rcx
;       [251:40] 4
;       [251:40] 4
        mov rcx, 4
;       [251:29] arr
;       [251:29] bounds check
;       [251:29] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [251:29] upper bound (--checks=upper)
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 296]
;       [251:34] arr1
;       [251:34] bounds check
;       [251:34] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [251:34] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 320]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [251:16] free named register rcx
;       [251:16] free named register rdi
;       [251:16] free named register rsi
        setne r15b
    bool.251.12.end:
;   [37:6] assert(ok bool)
    func.assert.251.5:
;       [251:5] alias ok -> r15b
        if.37.27.251.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.251.5:
        cmp r15b, 0
        jne if.37.24.251.5.end
        if.37.27.251.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.251.5.end:
;       [251:5] free scratch register r15
    func.assert.251.5.end:
;   [253:5] var arr4 = arr
;   [253:9] arr4: i32[4] (16 B @ [rbp + 356])
;   [253:9] arr4 = arr
;   [253:16] size <= 16 B, use mov
;   [253:16] allocate named register rax
    mov rax, qword [rbp + 296]
    mov qword [rbp + 356], rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 364], rax
;   [253:16] free named register rax
;   [254:5] assert(equal(arr, arr4))
;   [254:12] allocate scratch register -> r15
;   [254:12] ? equal(arr, arr4)
;   [254:12] ? shorthand: equal(arr, arr4)
    cmp.254.12:
;       [254:12] equal(arr, arr4)
;       [254:12] allocate named register rsi
;       [254:12] allocate named register rdi
;       [254:12] allocate named register rcx
;       [254:18] arr
        lea rsi, [rbp + 296]
;       [254:23] arr4
        lea rdi, [rbp + 356]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
;       [254:12] free named register rcx
;       [254:12] free named register rdi
;       [254:12] free named register rsi
        sete r15b
    bool.254.12.end:
;   [37:6] assert(ok bool)
    func.assert.254.5:
;       [254:5] alias ok -> r15b
        if.37.27.254.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.254.5:
        cmp r15b, 0
        jne if.37.24.254.5.end
        if.37.27.254.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.254.5.end:
;       [254:5] free scratch register r15
    func.assert.254.5.end:
;   [261:5] ix = 3
;   [261:10] 3
    mov qword [rbp + 312], 3
;   [262:5] var tmp = ~inv(arr[ix - 1])
;   [262:9] tmp: i32 (4 B @ [rbp + 372])
;   [262:9] tmp = ~inv(arr[ix - 1])
;   [262:16] tmp = ~inv(arr[ix - 1])
;   [262:16] = expression
;   [262:16] ~inv(arr[ix - 1])
;   [262:16] instructions without scratch register 12, with 12
;   [262:24] allocate scratch register -> r15
;   [262:24] set array index
;   [262:24] ix
    mov r15, qword [rbp + 312]
;   [262:24] r15 - 1
;   [262:24] src: folded constant '- 1'
    sub r15, 1
;   [262:24] bounds check
;   [262:24] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [262:24] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [262:16] instructions without scratch register 6, with 7
;   [70:6] inv(i i32) res i32
    func.inv.262.16:
;       [262:16] alias res -> tmp
;       [262:16] alias i -> arr (lea: rbp + r15 * 4 + 296)
;       [71:5] res = ~i
;       [71:11] instructions without scratch register 3, with 3
;       [71:12] ~i
;       [71:12] allocate scratch register -> r14
        mov r14d, dword [rbp + r15 * 4 + 296]
        mov dword [rbp + 372], r14d
;       [71:12] free scratch register r14
        not dword [rbp + 372]
    func.inv.262.16.end:
    not dword [rbp + 372]
;       [262:16] free scratch register r15
;   [263:5] arr[ix] = tmp
;   [263:9] allocate scratch register -> r15
;   [263:9] set array index
;   [263:9] ix
    mov r15, qword [rbp + 312]
;   [263:9] bounds check
;   [263:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [263:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [263:15] tmp
;   [263:15] allocate scratch register -> r14
    mov r14d, dword [rbp + 372]
    mov dword [rbp + r15 * 4 + 296], r14d
;   [263:15] free scratch register r14
;   [263:5] free scratch register r15
;   [264:5] assert(arr[ix] == 2)
;   [264:12] allocate scratch register -> r15
;   [264:12] ? arr[ix] == 2
;   [264:12] ? arr[ix] == 2
    cmp.264.12:
;   [264:16] allocate scratch register -> r14
;   [264:16] set array index
;   [264:16] ix
    mov r14, qword [rbp + 312]
;   [264:16] bounds check
;   [264:16] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
;   [264:16] upper bound (--checks=upper)
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 296], 2
;   [264:12] free scratch register r14
    sete r15b
    bool.264.12.end:
;   [37:6] assert(ok bool)
    func.assert.264.5:
;       [264:5] alias ok -> r15b
        if.37.27.264.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.264.5:
        cmp r15b, 0
        jne if.37.24.264.5.end
        if.37.27.264.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.264.5.end:
;       [264:5] free scratch register r15
    func.assert.264.5.end:
;   [266:5] faz(arr)
;   [76:6] faz(arg i32[])
    func.faz.266.5:
;       [266:5] alias arg -> arr
;       [77:5] arg[1] = 0xfe
;       [77:14] 0xfe
        mov dword [rbp + 300], 254
    func.faz.266.5.end:
;   [267:5] assert(arr[1] == 0xfe)
;   [267:12] allocate scratch register -> r15
;   [267:12] ? arr[1] == 0xfe
;   [267:12] ? arr[1] == 0xfe
    cmp.267.12:
    cmp dword [rbp + 300], 254
    sete r15b
    bool.267.12.end:
;   [37:6] assert(ok bool)
    func.assert.267.5:
;       [267:5] alias ok -> r15b
        if.37.27.267.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.267.5:
        cmp r15b, 0
        jne if.37.24.267.5.end
        if.37.27.267.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.267.5.end:
;       [267:5] free scratch register r15
    func.assert.267.5.end:
;   [269:5] var arr3 = i[]{ 3, 5 }
;   [269:9] arr3: i64[2] (16 B @ [rbp + 376])
;   [269:9] arr3 = i[]{ 3, 5 }
;   [269:19] size <= 16 B, use immediates
    mov qword [rbp + 376], 3
    mov qword [rbp + 384], 5
;   [270:5] foo arr3
;   [270:9] allocate scratch register -> r15
;   [270:9] e: i64 (r15)
;   [270:9] i: i64 (8 B @ [rbp + 400])
;   [270:9] const n = 2
;   [270:9] initiate iterator e
    lea r15, [rbp + 376]
;   [270:9] initiate counter i
    mov qword [rbp + 400], 0
    foo.270.5:
;       [271:9] e = e + i + n
;       [271:13] instructions without scratch register 3, with 4
;       [271:13] e
;       [271:17] e + i
;       [271:17] src: operand
;       [271:17] allocate scratch register -> r14
        mov r14, qword [rbp + 400]
        add qword [r15], r14
;       [271:17] free scratch register r14
;       [271:13] e + 2
;       [271:13] src: folded constant '+ n'
        add qword [r15], 2
        foo.270.5.continue:
            add r15, 8
            inc qword [rbp + 400]
            cmp qword [rbp + 400], 2
            jne foo.270.5
    foo.270.5.end:
;   [270:5] free scratch register r15
;   [273:5] assert(arr3[0] == 3 + 0 + 2)
;   [273:12] allocate scratch register -> r15
;   [273:12] ? arr3[0] == 3 + 0 + 2
;   [273:12] ? arr3[0] == 3 + 0 + 2
    cmp.273.12:
;   [273:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 376], 5
    sete r15b
    bool.273.12.end:
;   [37:6] assert(ok bool)
    func.assert.273.5:
;       [273:5] alias ok -> r15b
        if.37.27.273.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.273.5:
        cmp r15b, 0
        jne if.37.24.273.5.end
        if.37.27.273.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.273.5.end:
;       [273:5] free scratch register r15
    func.assert.273.5.end:
;   [274:5] assert(arr3[1] == 5 + 1 + 2)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? arr3[1] == 5 + 1 + 2
;   [274:12] ? arr3[1] == 5 + 1 + 2
    cmp.274.12:
;   [274:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 384], 8
    sete r15b
    bool.274.12.end:
;   [37:6] assert(ok bool)
    func.assert.274.5:
;       [274:5] alias ok -> r15b
        if.37.27.274.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.274.5:
        cmp r15b, 0
        jne if.37.24.274.5.end
        if.37.27.274.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.274.5.end:
;       [274:5] free scratch register r15
    func.assert.274.5.end:
;   [280:5] var p = point{}
;   [280:9] p: point (16 B @ [rbp + 392])
;   [280:9] p = point{}
;   [280:13] zero remaining fields: 16 B
;   [280:13] size <= 32 B, use mov
    mov qword [rbp + 392], 0
    mov qword [rbp + 400], 0
;   [282:7] p.fooz()
;   [46:6] point.fooz()
    func.point.fooz.282.7:
;       [282:7] alias self -> p
;       [47:5] self.x = 0b10
;       [47:14] 0b10
        mov qword [rbp + 392], 2
;       [48:5] self.y = 0xb
;       [48:14] 0xb
        mov qword [rbp + 400], 11
    func.point.fooz.282.7.end:
;   [285:5] assert(p.x == 2)
;   [285:12] allocate scratch register -> r15
;   [285:12] ? p.x == 2
;   [285:12] ? p.x == 2
    cmp.285.12:
    cmp qword [rbp + 392], 2
    sete r15b
    bool.285.12.end:
;   [37:6] assert(ok bool)
    func.assert.285.5:
;       [285:5] alias ok -> r15b
        if.37.27.285.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.285.5:
        cmp r15b, 0
        jne if.37.24.285.5.end
        if.37.27.285.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.285.5.end:
;       [285:5] free scratch register r15
    func.assert.285.5.end:
;   [286:5] assert(p.y == 0xb)
;   [286:12] allocate scratch register -> r15
;   [286:12] ? p.y == 0xb
;   [286:12] ? p.y == 0xb
    cmp.286.12:
    cmp qword [rbp + 400], 11
    sete r15b
    bool.286.12.end:
;   [37:6] assert(ok bool)
    func.assert.286.5:
;       [286:5] alias ok -> r15b
        if.37.27.286.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.286.5:
        cmp r15b, 0
        jne if.37.24.286.5.end
        if.37.27.286.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.286.5.end:
;       [286:5] free scratch register r15
    func.assert.286.5.end:
;   [288:5] var q = p
;   [288:9] q: point (16 B @ [rbp + 408])
;   [288:9] q = p
;   [288:13] size <= 16 B, use mov
;   [288:13] allocate named register rax
    mov rax, qword [rbp + 392]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 400]
    mov qword [rbp + 416], rax
;   [288:13] free named register rax
;   [291:5] assert(equal(p, q))
;   [291:12] allocate scratch register -> r15
;   [291:12] ? equal(p, q)
;   [291:12] ? shorthand: equal(p, q)
    cmp.291.12:
;       [291:12] equal(p, q)
;       [291:12] allocate named register rsi
;       [291:12] allocate named register rdi
;       [291:12] allocate named register rcx
;       [291:18] p
        lea rsi, [rbp + 392]
;       [291:21] q
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
;       [291:12] free named register rcx
;       [291:12] free named register rdi
;       [291:12] free named register rsi
        sete r15b
    bool.291.12.end:
;   [37:6] assert(ok bool)
    func.assert.291.5:
;       [291:5] alias ok -> r15b
        if.37.27.291.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.291.5:
        cmp r15b, 0
        jne if.37.24.291.5.end
        if.37.27.291.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.291.5.end:
;       [291:5] free scratch register r15
    func.assert.291.5.end:
;   [295:5] q.x = 3
;   [295:11] 3
    mov qword [rbp + 408], 3
;   [296:5] assert(not equal(p, q))
;   [296:12] allocate scratch register -> r15
;   [296:12] ? not equal(p, q)
;   [296:12] ? shorthand: not equal(p, q)
    cmp.296.12:
;       [296:16] equal(p, q)
;       [296:16] allocate named register rsi
;       [296:16] allocate named register rdi
;       [296:16] allocate named register rcx
;       [296:22] p
        lea rsi, [rbp + 392]
;       [296:25] q
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
;       [296:16] free named register rcx
;       [296:16] free named register rdi
;       [296:16] free named register rsi
        setne r15b
    bool.296.12.end:
;   [37:6] assert(ok bool)
    func.assert.296.5:
;       [296:5] alias ok -> r15b
        if.37.27.296.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.296.5:
        cmp r15b, 0
        jne if.37.24.296.5.end
        if.37.27.296.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.296.5.end:
;       [296:5] free scratch register r15
    func.assert.296.5.end:
;   [298:5] var i = 0
;   [298:9] i: i64 (8 B @ [rbp + 424])
;   [298:9] i = 0
;   [298:13] 0
    mov qword [rbp + 424], 0
;   [299:5] bar(i)
;   [57:6] bar(arg)
    func.bar.299.5:
;       [299:5] alias arg -> i
        if.58.8.299.5:
;       [58:8] ? arg == 0
;       [58:8] ? arg == 0
        cmp.58.8.299.5:
        cmp qword [rbp + 424], 0
        je func.bar.299.5.end
        if.58.8.299.5.code:
;           [58:17] return
        if.58.5.299.5.end:
;       [59:5] arg = 0xff
;       [59:11] 0xff
        mov qword [rbp + 424], 255
    func.bar.299.5.end:
;   [300:5] assert(i == 0)
;   [300:12] allocate scratch register -> r15
;   [300:12] ? i == 0
;   [300:12] ? i == 0
    cmp.300.12:
    cmp qword [rbp + 424], 0
    sete r15b
    bool.300.12.end:
;   [37:6] assert(ok bool)
    func.assert.300.5:
;       [300:5] alias ok -> r15b
        if.37.27.300.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.300.5:
        cmp r15b, 0
        jne if.37.24.300.5.end
        if.37.27.300.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.300.5.end:
;       [300:5] free scratch register r15
    func.assert.300.5.end:
;   [302:5] i = 1
;   [302:9] 1
    mov qword [rbp + 424], 1
;   [303:5] bar(i)
;   [57:6] bar(arg)
    func.bar.303.5:
;       [303:5] alias arg -> i
        if.58.8.303.5:
;       [58:8] ? arg == 0
;       [58:8] ? arg == 0
        cmp.58.8.303.5:
        cmp qword [rbp + 424], 0
        je func.bar.303.5.end
        if.58.8.303.5.code:
;           [58:17] return
        if.58.5.303.5.end:
;       [59:5] arg = 0xff
;       [59:11] 0xff
        mov qword [rbp + 424], 255
    func.bar.303.5.end:
;   [304:5] assert(i == 0xff)
;   [304:12] allocate scratch register -> r15
;   [304:12] ? i == 0xff
;   [304:12] ? i == 0xff
    cmp.304.12:
    cmp qword [rbp + 424], 255
    sete r15b
    bool.304.12.end:
;   [37:6] assert(ok bool)
    func.assert.304.5:
;       [304:5] alias ok -> r15b
        if.37.27.304.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.304.5:
        cmp r15b, 0
        jne if.37.24.304.5.end
        if.37.27.304.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.304.5.end:
;       [304:5] free scratch register r15
    func.assert.304.5.end:
;   [306:5] var j = 1
;   [306:9] j: i64 (8 B @ [rbp + 432])
;   [306:9] j = 1
;   [306:13] 1
    mov qword [rbp + 432], 1
;   [307:5] var k = baz(j)
;   [307:9] k: i64 (8 B @ [rbp + 440])
;   [307:9] k = baz(j)
;   [307:13] k = baz(j)
;   [307:13] = expression
;   [307:13] baz(j)
;   [64:6] baz(arg) res
    func.baz.307.13:
;       [307:13] alias res -> k
;       [307:13] alias arg -> j
;       [65:5] res = arg * 2
;       [65:11] instructions without scratch register 3, with 3
;       [65:11] arg
;       [65:11] allocate scratch register -> r15
        mov r15, qword [rbp + 432]
        mov qword [rbp + 440], r15
;       [65:11] free scratch register r15
;       [65:11] res * 2
;       [65:11] src: folded constant '* 2'
        sal qword [rbp + 440], 1
    func.baz.307.13.end:
;   [308:5] assert(k == 2)
;   [308:12] allocate scratch register -> r15
;   [308:12] ? k == 2
;   [308:12] ? k == 2
    cmp.308.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.308.12.end:
;   [37:6] assert(ok bool)
    func.assert.308.5:
;       [308:5] alias ok -> r15b
        if.37.27.308.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.308.5:
        cmp r15b, 0
        jne if.37.24.308.5.end
        if.37.27.308.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.308.5.end:
;       [308:5] free scratch register r15
    func.assert.308.5.end:
;   [310:5] k = baz(1)
;   [310:9] k = baz(1)
;   [310:9] = expression
;   [310:9] baz(1)
;   [64:6] baz(arg) res
    func.baz.310.9:
;       [310:9] alias res -> k
;       [310:9] alias arg -> 1
;       [65:5] res = arg * 2
;       [65:11] res = 2
;       [65:11] src: folded constant 'arg * 2'
        mov qword [rbp + 440], 2
    func.baz.310.9.end:
;   [311:5] assert(k == 2)
;   [311:12] allocate scratch register -> r15
;   [311:12] ? k == 2
;   [311:12] ? k == 2
    cmp.311.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.311.12.end:
;   [37:6] assert(ok bool)
    func.assert.311.5:
;       [311:5] alias ok -> r15b
        if.37.27.311.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.311.5:
        cmp r15b, 0
        jne if.37.24.311.5.end
        if.37.27.311.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.311.5.end:
;       [311:5] free scratch register r15
    func.assert.311.5.end:
;   [313:5] var five = 5
;   [313:9] five: i64 (8 B @ [rbp + 448])
;   [313:9] five = 5
;   [313:16] 5
    mov qword [rbp + 448], 5
;   [314:5] var f = factorial(five)
;   [314:9] f: i64 (8 B @ [rbp + 456])
;   [314:9] f = factorial(five)
;   [314:13] f = factorial(five)
;   [314:13] = expression
;   [314:13] factorial(five)
;   [314:13] frame capacity check (--checks=frame)
;   [314:13] allocate scratch register -> r15
;   [314:13] allocate scratch register -> r14
    lea r15, [rbp + 464]
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
;   [314:13] free scratch register r14
;   [314:13] free scratch register r15
;   [314:13] result address in callee frame
;   [314:13] allocate scratch register -> r15
    lea r15, [rbp + 456]
    mov qword [rbp + 464], r15
;   [314:13] free scratch register r15
;   [314:13] address of argument 'five' to parameter 'n'
;   [314:13] allocate scratch register -> r15
    lea r15, [rbp + 448]
    mov qword [rbp + 472], r15
;   [314:13] free scratch register r15
;   [314:13] set function frame base
    lea rbx, [rbp + 464]
    call func.factorial
;   [315:5] assert(f == 120)
;   [315:12] allocate scratch register -> r15
;   [315:12] ? f == 120
;   [315:12] ? f == 120
    cmp.315.12:
    cmp qword [rbp + 456], 120
    sete r15b
    bool.315.12.end:
;   [37:6] assert(ok bool)
    func.assert.315.5:
;       [315:5] alias ok -> r15b
        if.37.27.315.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.315.5:
        cmp r15b, 0
        jne if.37.24.315.5.end
        if.37.27.315.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.315.5.end:
;       [315:5] free scratch register r15
    func.assert.315.5.end:
;   [317:5] var p0 = point{baz(3), 0}
;   [317:9] p0: point (16 B @ [rbp + 464])
;   [317:9] p0 = point{baz(3), 0}
;   [317:20] copy field 'x'
;   [317:20] p0.x = baz(3)
;   [317:20] = expression
;   [317:20] baz(3)
;   [64:6] baz(arg) res
    func.baz.317.20:
;       [317:20] alias res -> p0.x (lea: rbp + 464)
;       [317:20] alias arg -> 3
;       [65:5] res = arg * 2
;       [65:11] res = 6
;       [65:11] src: folded constant 'arg * 2'
        mov qword [rbp + 464], 6
    func.baz.317.20.end:
;   [317:28] copy field 'y'
    mov qword [rbp + 472], 0
;   [318:5] assert(p0.x == 6)
;   [318:12] allocate scratch register -> r15
;   [318:12] ? p0.x == 6
;   [318:12] ? p0.x == 6
    cmp.318.12:
    cmp qword [rbp + 464], 6
    sete r15b
    bool.318.12.end:
;   [37:6] assert(ok bool)
    func.assert.318.5:
;       [318:5] alias ok -> r15b
        if.37.27.318.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.318.5:
        cmp r15b, 0
        jne if.37.24.318.5.end
        if.37.27.318.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.318.5.end:
;       [318:5] free scratch register r15
    func.assert.318.5.end:
;   [320:5] var pt = point.at(-1, -2)
;   [320:9] pt: point (16 B @ [rbp + 480])
;   [320:9] pt = point.at(-1, -2)
;   [320:14] point.at(-1, -2)
;   [102:6] point.at(x, y) self
    func.point.at.320.14:
;       [320:14] alias self -> pt
;       [320:14] alias x -> -1
;       [320:14] alias y -> -2
;       [103:5] self.x = x
;       [103:14] x
        mov qword [rbp + 480], -1
;       [104:5] self.y = y
;       [104:14] y
        mov qword [rbp + 488], -2
    func.point.at.320.14.end:
;   [324:5] assert(pt.x == -1)
;   [324:12] allocate scratch register -> r15
;   [324:12] ? pt.x == -1
;   [324:12] ? pt.x == -1
    cmp.324.12:
    cmp qword [rbp + 480], -1
    sete r15b
    bool.324.12.end:
;   [37:6] assert(ok bool)
    func.assert.324.5:
;       [324:5] alias ok -> r15b
        if.37.27.324.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.324.5:
        cmp r15b, 0
        jne if.37.24.324.5.end
        if.37.27.324.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.324.5.end:
;       [324:5] free scratch register r15
    func.assert.324.5.end:
;   [325:5] assert(pt.y == -2)
;   [325:12] allocate scratch register -> r15
;   [325:12] ? pt.y == -2
;   [325:12] ? pt.y == -2
    cmp.325.12:
    cmp qword [rbp + 488], -2
    sete r15b
    bool.325.12.end:
;   [37:6] assert(ok bool)
    func.assert.325.5:
;       [325:5] alias ok -> r15b
        if.37.27.325.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.325.5:
        cmp r15b, 0
        jne if.37.24.325.5.end
        if.37.27.325.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.325.5.end:
;       [325:5] free scratch register r15
    func.assert.325.5.end:
;   [327:8] pt.x(2)
;   [108:6] point.x(x)
    func.point.x.327.8:
;       [327:8] alias self -> pt
;       [327:8] alias x -> 2
;       [109:5] self.x = x
;       [109:14] x
        mov qword [rbp + 480], 2
    func.point.x.327.8.end:
;   [328:5] assert(pt.x == 2)
;   [328:12] allocate scratch register -> r15
;   [328:12] ? pt.x == 2
;   [328:12] ? pt.x == 2
    cmp.328.12:
    cmp qword [rbp + 480], 2
    sete r15b
    bool.328.12.end:
;   [37:6] assert(ok bool)
    func.assert.328.5:
;       [328:5] alias ok -> r15b
        if.37.27.328.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.328.5:
        cmp r15b, 0
        jne if.37.24.328.5.end
        if.37.27.328.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.328.5.end:
;       [328:5] free scratch register r15
    func.assert.328.5.end:
;   [329:5] assert(pt.sum() == 0)
;   [329:12] allocate scratch register -> r15
;   [329:12] ? pt.sum() == 0
;   [329:12] ? pt.sum() == 0
    cmp.329.12:
;   [329:12] allocate scratch register -> r14
;       [329:15] r14 = pt.sum()
;       [329:15] = expression
;       [329:15] pt.sum()
;       [52:6] point.sum() res
        func.point.sum.329.15:
;           [329:15] alias res -> r14
;           [329:15] alias self -> pt
;           [53:5] res = self.x + self.y
;           [53:11] self.x
            mov r14, qword [rbp + 480]
;           [53:20] res + self.y
;           [53:20] src: operand
            add r14, qword [rbp + 488]
        func.point.sum.329.15.end:
    cmp r14, 0
;   [329:12] free scratch register r14
    sete r15b
    bool.329.12.end:
;   [37:6] assert(ok bool)
    func.assert.329.5:
;       [329:5] alias ok -> r15b
        if.37.27.329.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.329.5:
        cmp r15b, 0
        jne if.37.24.329.5.end
        if.37.27.329.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.329.5.end:
;       [329:5] free scratch register r15
    func.assert.329.5.end:
;   [331:5] var x = 1
;   [331:9] x: i64 (8 B @ [rbp + 496])
;   [331:9] x = 1
;   [331:13] 1
    mov qword [rbp + 496], 1
;   [332:5] var y = 2
;   [332:9] y: i64 (8 B @ [rbp + 504])
;   [332:9] y = 2
;   [332:13] 2
    mov qword [rbp + 504], 2
;   [334:5] var o1 = object{{x * 10, y}, 0xff0000}
;   [334:9] o1: object (24 B @ [rbp + 512])
;   [334:9] o1 = object{{x * 10, y}, 0xff0000}
;   [334:21] copy field 'pos'
;   [334:22] copy field 'x'
;   [334:22] instructions without scratch register 5, with 3
;   [334:22] allocate scratch register -> r15
;   [334:22] x
    mov r15, qword [rbp + 496]
;   [334:22] r15 * 10
;   [334:22] src: folded constant '* 10'
    imul r15, 10
    mov qword [rbp + 512], r15
;   [334:22] free scratch register r15
;   [334:30] copy field 'y'
;   [334:30] allocate scratch register -> r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 520], r15
;   [334:30] free scratch register r15
;   [334:34] copy field 'color'
    mov dword [rbp + 528], 16711680
;   [334:14] zero padding: 4 B
;   [334:14] size <= 32 B, use mov
    mov dword [rbp + 532], 0
;   [335:5] assert(o1.pos.x == 10)
;   [335:12] allocate scratch register -> r15
;   [335:12] ? o1.pos.x == 10
;   [335:12] ? o1.pos.x == 10
    cmp.335.12:
    cmp qword [rbp + 512], 10
    sete r15b
    bool.335.12.end:
;   [37:6] assert(ok bool)
    func.assert.335.5:
;       [335:5] alias ok -> r15b
        if.37.27.335.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.335.5:
        cmp r15b, 0
        jne if.37.24.335.5.end
        if.37.27.335.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.335.5.end:
;       [335:5] free scratch register r15
    func.assert.335.5.end:
;   [336:5] assert(o1.pos.y == 2)
;   [336:12] allocate scratch register -> r15
;   [336:12] ? o1.pos.y == 2
;   [336:12] ? o1.pos.y == 2
    cmp.336.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.336.12.end:
;   [37:6] assert(ok bool)
    func.assert.336.5:
;       [336:5] alias ok -> r15b
        if.37.27.336.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.336.5:
        cmp r15b, 0
        jne if.37.24.336.5.end
        if.37.27.336.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.336.5.end:
;       [336:5] free scratch register r15
    func.assert.336.5.end:
;   [337:5] assert(o1.color == 0xff0000)
;   [337:12] allocate scratch register -> r15
;   [337:12] ? o1.color == 0xff0000
;   [337:12] ? o1.color == 0xff0000
    cmp.337.12:
    cmp dword [rbp + 528], 16711680
    sete r15b
    bool.337.12.end:
;   [37:6] assert(ok bool)
    func.assert.337.5:
;       [337:5] alias ok -> r15b
        if.37.27.337.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.337.5:
        cmp r15b, 0
        jne if.37.24.337.5.end
        if.37.27.337.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.337.5.end:
;       [337:5] free scratch register r15
    func.assert.337.5.end:
;   [339:5] var p1 = point{-x, -y}
;   [339:9] p1: point (16 B @ [rbp + 536])
;   [339:9] p1 = point{-x, -y}
;   [339:20] copy field 'x'
;   [339:20] instructions without scratch register 3, with 3
;   [339:20] allocate scratch register -> r15
    mov r15, qword [rbp + 496]
    mov qword [rbp + 536], r15
;   [339:20] free scratch register r15
    neg qword [rbp + 536]
;   [339:24] copy field 'y'
;   [339:24] instructions without scratch register 3, with 3
;   [339:24] allocate scratch register -> r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 544], r15
;   [339:24] free scratch register r15
    neg qword [rbp + 544]
;   [340:5] o1.pos = p1
;   [340:14] size <= 16 B, use mov
;   [340:14] allocate named register rax
    mov rax, qword [rbp + 536]
    mov qword [rbp + 512], rax
    mov rax, qword [rbp + 544]
    mov qword [rbp + 520], rax
;   [340:14] free named register rax
;   [341:5] assert(o1.pos.x == -1)
;   [341:12] allocate scratch register -> r15
;   [341:12] ? o1.pos.x == -1
;   [341:12] ? o1.pos.x == -1
    cmp.341.12:
    cmp qword [rbp + 512], -1
    sete r15b
    bool.341.12.end:
;   [37:6] assert(ok bool)
    func.assert.341.5:
;       [341:5] alias ok -> r15b
        if.37.27.341.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.341.5:
        cmp r15b, 0
        jne if.37.24.341.5.end
        if.37.27.341.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.341.5.end:
;       [341:5] free scratch register r15
    func.assert.341.5.end:
;   [342:5] assert(o1.pos.y == -2)
;   [342:12] allocate scratch register -> r15
;   [342:12] ? o1.pos.y == -2
;   [342:12] ? o1.pos.y == -2
    cmp.342.12:
    cmp qword [rbp + 520], -2
    sete r15b
    bool.342.12.end:
;   [37:6] assert(ok bool)
    func.assert.342.5:
;       [342:5] alias ok -> r15b
        if.37.27.342.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.342.5:
        cmp r15b, 0
        jne if.37.24.342.5.end
        if.37.27.342.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.342.5.end:
;       [342:5] free scratch register r15
    func.assert.342.5.end:
;   [344:5] var o2 = o1
;   [344:9] o2: object (24 B @ [rbp + 552])
;   [344:9] o2 = o1
;   [344:14] allocate named register rsi
;   [344:14] allocate named register rdi
;   [344:14] allocate named register rcx
    lea rsi, [rbp + 512]
    lea rdi, [rbp + 552]
    mov rcx, 24
    rep movsb
;   [344:14] free named register rcx
;   [344:14] free named register rdi
;   [344:14] free named register rsi
;   [345:5] assert(o2.pos.x == -1)
;   [345:12] allocate scratch register -> r15
;   [345:12] ? o2.pos.x == -1
;   [345:12] ? o2.pos.x == -1
    cmp.345.12:
    cmp qword [rbp + 552], -1
    sete r15b
    bool.345.12.end:
;   [37:6] assert(ok bool)
    func.assert.345.5:
;       [345:5] alias ok -> r15b
        if.37.27.345.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.345.5:
        cmp r15b, 0
        jne if.37.24.345.5.end
        if.37.27.345.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.345.5.end:
;       [345:5] free scratch register r15
    func.assert.345.5.end:
;   [346:5] assert(o2.pos.y == -2)
;   [346:12] allocate scratch register -> r15
;   [346:12] ? o2.pos.y == -2
;   [346:12] ? o2.pos.y == -2
    cmp.346.12:
    cmp qword [rbp + 560], -2
    sete r15b
    bool.346.12.end:
;   [37:6] assert(ok bool)
    func.assert.346.5:
;       [346:5] alias ok -> r15b
        if.37.27.346.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.346.5:
        cmp r15b, 0
        jne if.37.24.346.5.end
        if.37.27.346.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.346.5.end:
;       [346:5] free scratch register r15
    func.assert.346.5.end:
;   [347:5] assert(o2.color == 0xff0000)
;   [347:12] allocate scratch register -> r15
;   [347:12] ? o2.color == 0xff0000
;   [347:12] ? o2.color == 0xff0000
    cmp.347.12:
    cmp dword [rbp + 568], 16711680
    sete r15b
    bool.347.12.end:
;   [37:6] assert(ok bool)
    func.assert.347.5:
;       [347:5] alias ok -> r15b
        if.37.27.347.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.347.5:
        cmp r15b, 0
        jne if.37.24.347.5.end
        if.37.27.347.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.347.5.end:
;       [347:5] free scratch register r15
    func.assert.347.5.end:
;   [349:5] o2.pos = {x, y}
;   [349:15] copy field 'x'
;   [349:15] allocate scratch register -> r15
    mov r15, qword [rbp + 496]
    mov qword [rbp + 552], r15
;   [349:15] free scratch register r15
;   [349:18] copy field 'y'
;   [349:18] allocate scratch register -> r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 560], r15
;   [349:18] free scratch register r15
;   [350:5] assert(o2.pos.x == 1)
;   [350:12] allocate scratch register -> r15
;   [350:12] ? o2.pos.x == 1
;   [350:12] ? o2.pos.x == 1
    cmp.350.12:
    cmp qword [rbp + 552], 1
    sete r15b
    bool.350.12.end:
;   [37:6] assert(ok bool)
    func.assert.350.5:
;       [350:5] alias ok -> r15b
        if.37.27.350.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.350.5:
        cmp r15b, 0
        jne if.37.24.350.5.end
        if.37.27.350.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.350.5.end:
;       [350:5] free scratch register r15
    func.assert.350.5.end:
;   [351:5] o2.pos = point{y, x}
;   [351:20] copy field 'x'
;   [351:20] allocate scratch register -> r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 552], r15
;   [351:20] free scratch register r15
;   [351:23] copy field 'y'
;   [351:23] allocate scratch register -> r15
    mov r15, qword [rbp + 496]
    mov qword [rbp + 560], r15
;   [351:23] free scratch register r15
;   [352:5] assert(o2.pos.x == 2)
;   [352:12] allocate scratch register -> r15
;   [352:12] ? o2.pos.x == 2
;   [352:12] ? o2.pos.x == 2
    cmp.352.12:
    cmp qword [rbp + 552], 2
    sete r15b
    bool.352.12.end:
;   [37:6] assert(ok bool)
    func.assert.352.5:
;       [352:5] alias ok -> r15b
        if.37.27.352.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.352.5:
        cmp r15b, 0
        jne if.37.24.352.5.end
        if.37.27.352.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.352.5.end:
;       [352:5] free scratch register r15
    func.assert.352.5.end:
;   [359:5] var o3 = object[2]{}
;   [359:9] o3: object[2] (48 B @ [rbp + 576])
;   [359:9] o3 = object[2]{}
;   [359:14] zero remaining elements: 2 * 24 B = 48 B
;   [359:14] allocate named register rax
;   [359:14] allocate named register rdi
;   [359:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 576]
    mov rcx, 48
    rep stosb
;   [359:14] free named register rcx
;   [359:14] free named register rdi
;   [359:14] free named register rax
;   [360:5] o3[0].pos.y = 73
;   [360:19] 73
    mov qword [rbp + 584], 73
;   [362:5] assert(o3[0].pos.y == 73)
;   [362:12] allocate scratch register -> r15
;   [362:12] ? o3[0].pos.y == 73
;   [362:12] ? o3[0].pos.y == 73
    cmp.362.12:
    cmp qword [rbp + 584], 73
    sete r15b
    bool.362.12.end:
;   [37:6] assert(ok bool)
    func.assert.362.5:
;       [362:5] alias ok -> r15b
        if.37.27.362.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.362.5:
        cmp r15b, 0
        jne if.37.24.362.5.end
        if.37.27.362.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.362.5.end:
;       [362:5] free scratch register r15
    func.assert.362.5.end:
;   [363:5] o3[1] = object.at(2, 74, 0xffffff)
;   [363:13] object.at(2, 74, 0xffffff)
;   [113:6] object.at(x, y, color i32) self
    func.object.at.363.13:
;       [363:13] alias self -> o3 (lea: rbp + 600)
;       [363:13] alias x -> 2
;       [363:13] alias y -> 74
;       [363:13] alias color -> 16777215
;       [114:5] self.pos = point.at(x, y)
;       [114:16] point.at(x, y)
;       [102:6] point.at(x, y) self
        func.point.at.114.16.363.13:
;           [114:16] alias self -> self.pos (lea: rbp + 600)
;           [114:16] alias x -> 2
;           [114:16] alias y -> 74
;           [103:5] self.x = x
;           [103:14] x
            mov qword [rbp + 600], 2
;           [104:5] self.y = y
;           [104:14] y
            mov qword [rbp + 608], 74
        func.point.at.114.16.363.13.end:
;       [115:5] self.color = color
;       [115:18] color
        mov dword [rbp + 616], 16777215
    func.object.at.363.13.end:
;   [364:5] assert(o3[1].pos.y == 74)
;   [364:12] allocate scratch register -> r15
;   [364:12] ? o3[1].pos.y == 74
;   [364:12] ? o3[1].pos.y == 74
    cmp.364.12:
    cmp qword [rbp + 608], 74
    sete r15b
    bool.364.12.end:
;   [37:6] assert(ok bool)
    func.assert.364.5:
;       [364:5] alias ok -> r15b
        if.37.27.364.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.364.5:
        cmp r15b, 0
        jne if.37.24.364.5.end
        if.37.27.364.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.364.5.end:
;       [364:5] free scratch register r15
    func.assert.364.5.end:
;   [366:15] o3[1].pos.fooz()
;   [46:6] point.fooz()
    func.point.fooz.366.15:
;       [366:15] alias self -> o3.pos (lea: rbp + 600)
;       [47:5] self.x = 0b10
;       [47:14] 0b10
        mov qword [rbp + 600], 2
;       [48:5] self.y = 0xb
;       [48:14] 0xb
        mov qword [rbp + 608], 11
    func.point.fooz.366.15.end:
;   [367:5] assert(o3[1].pos.sum() == 13)
;   [367:12] allocate scratch register -> r15
;   [367:12] ? o3[1].pos.sum() == 13
;   [367:12] ? o3[1].pos.sum() == 13
    cmp.367.12:
;   [367:12] allocate scratch register -> r14
;       [367:22] r14 = o3[1].pos.sum()
;       [367:22] = expression
;       [367:22] o3[1].pos.sum()
;       [52:6] point.sum() res
        func.point.sum.367.22:
;           [367:22] alias res -> r14
;           [367:22] alias self -> o3.pos (lea: rbp + 600)
;           [53:5] res = self.x + self.y
;           [53:11] self.x
            mov r14, qword [rbp + 600]
;           [53:20] res + self.y
;           [53:20] src: operand
            add r14, qword [rbp + 608]
        func.point.sum.367.22.end:
    cmp r14, 13
;   [367:12] free scratch register r14
    sete r15b
    bool.367.12.end:
;   [37:6] assert(ok bool)
    func.assert.367.5:
;       [367:5] alias ok -> r15b
        if.37.27.367.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.367.5:
        cmp r15b, 0
        jne if.37.24.367.5.end
        if.37.27.367.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.367.5.end:
;       [367:5] free scratch register r15
    func.assert.367.5.end:
;   [370:5] var worlds = world[8]{}
;   [370:9] worlds: world[8] (512 B @ [rbp + 624])
;   [370:9] worlds = world[8]{}
;   [370:18] zero remaining elements: 8 * 64 B = 512 B
;   [370:18] allocate named register rax
;   [370:18] allocate named register rdi
;   [370:18] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 624]
    mov rcx, 512
    rep stosb
;   [370:18] free named register rcx
;   [370:18] free named register rdi
;   [370:18] free named register rax
;   [371:5] worlds[1].locations[1] = 0xffee
;   [371:30] 0xffee
    mov qword [rbp + 696], 65518
;   [372:5] assert(worlds[1].locations[1] == 0xffee)
;   [372:12] allocate scratch register -> r15
;   [372:12] ? worlds[1].locations[1] == 0xffee
;   [372:12] ? worlds[1].locations[1] == 0xffee
    cmp.372.12:
    cmp qword [rbp + 696], 65518
    sete r15b
    bool.372.12.end:
;   [37:6] assert(ok bool)
    func.assert.372.5:
;       [372:5] alias ok -> r15b
        if.37.27.372.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.372.5:
        cmp r15b, 0
        jne if.37.24.372.5.end
        if.37.27.372.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.372.5.end:
;       [372:5] free scratch register r15
    func.assert.372.5.end:
;   [374:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [374:5] allocate named register rsi
;   [374:5] allocate named register rdi
;   [374:5] allocate named register rcx
;   [377:9] array_length(worlds[0].locations)
;   [377:9] rcx = array_length(worlds[0].locations)
;   [377:9] = expression
;   [377:9] array_length(worlds[0].locations)
    mov rcx, 8
;   [375:9] worlds[1].locations
;   [375:9] bounds check
;   [375:9] lower bound (--checks=lower)
    test rcx, rcx
    js baz_bounds_panic
;   [375:9] upper bound (--checks=upper)
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 688]
;   [376:9] worlds[0].locations
;   [376:9] bounds check
;   [376:9] lower bound (--checks=lower)
    test rcx, rcx
    js baz_bounds_panic
;   [376:9] upper bound (--checks=upper)
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 624]
    shl rcx, 3
    rep movsb
;   [374:5] free named register rcx
;   [374:5] free named register rdi
;   [374:5] free named register rsi
;   [381:5] assert(worlds[0].locations[1] == 0xffee)
;   [381:12] allocate scratch register -> r15
;   [381:12] ? worlds[0].locations[1] == 0xffee
;   [381:12] ? worlds[0].locations[1] == 0xffee
    cmp.381.12:
    cmp qword [rbp + 632], 65518
    sete r15b
    bool.381.12.end:
;   [37:6] assert(ok bool)
    func.assert.381.5:
;       [381:5] alias ok -> r15b
        if.37.27.381.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.381.5:
        cmp r15b, 0
        jne if.37.24.381.5.end
        if.37.27.381.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.381.5.end:
;       [381:5] free scratch register r15
    func.assert.381.5.end:
;   [382:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [382:12] allocate scratch register -> r15
;   [382:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [382:12] ? shorthand: arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
    cmp.382.12:
;       [382:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;       [382:12] allocate named register rsi
;       [382:12] allocate named register rdi
;       [382:12] allocate named register rcx
;       [385:14] array_length(worlds[0].locations)
;       [385:14] rcx = array_length(worlds[0].locations)
;       [385:14] = expression
;       [385:14] array_length(worlds[0].locations)
        mov rcx, 8
;       [383:14] worlds[0].locations
;       [383:14] bounds check
;       [383:14] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [383:14] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 624]
;       [384:14] worlds[1].locations
;       [384:14] bounds check
;       [384:14] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [384:14] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 688]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [382:12] free named register rcx
;       [382:12] free named register rdi
;       [382:12] free named register rsi
        sete r15b
    bool.382.12.end:
;   [37:6] assert(ok bool)
    func.assert.382.5:
;       [382:5] alias ok -> r15b
        if.37.27.382.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.382.5:
        cmp r15b, 0
        jne if.37.24.382.5.end
        if.37.27.382.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.382.5.end:
;       [382:5] free scratch register r15
    func.assert.382.5.end:
;   [388:5] var arr2 = i[]{ -1, 2 }
;   [388:9] arr2: i64[2] (16 B @ [rbp + 1136])
;   [388:9] arr2 = i[]{ -1, 2 }
;   [388:19] size <= 16 B, use immediates
    mov qword [rbp + 1136], -1
    mov qword [rbp + 1144], 2
;   [389:5] assert(array_length(arr2) == 2)
;   [389:12] allocate scratch register -> r15
;   [389:12] ? array_length(arr2) == 2
;   [389:12] ? array_length(arr2) == 2
    cmp.389.12:
;   [389:12] allocate scratch register -> r14
;       [389:12] r14 = array_length(arr2)
;       [389:12] = expression
;       [389:12] array_length(arr2)
        mov r14, 2
    cmp r14, 2
;   [389:12] free scratch register r14
    sete r15b
    bool.389.12.end:
;   [37:6] assert(ok bool)
    func.assert.389.5:
;       [389:5] alias ok -> r15b
        if.37.27.389.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.389.5:
        cmp r15b, 0
        jne if.37.24.389.5.end
        if.37.27.389.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.389.5.end:
;       [389:5] free scratch register r15
    func.assert.389.5.end:
;   [390:5] assert(arr2[0] == -1)
;   [390:12] allocate scratch register -> r15
;   [390:12] ? arr2[0] == -1
;   [390:12] ? arr2[0] == -1
    cmp.390.12:
    cmp qword [rbp + 1136], -1
    sete r15b
    bool.390.12.end:
;   [37:6] assert(ok bool)
    func.assert.390.5:
;       [390:5] alias ok -> r15b
        if.37.27.390.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.390.5:
        cmp r15b, 0
        jne if.37.24.390.5.end
        if.37.27.390.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.390.5.end:
;       [390:5] free scratch register r15
    func.assert.390.5.end:
;   [391:5] assert(arr2[1] == 2)
;   [391:12] allocate scratch register -> r15
;   [391:12] ? arr2[1] == 2
;   [391:12] ? arr2[1] == 2
    cmp.391.12:
    cmp qword [rbp + 1144], 2
    sete r15b
    bool.391.12.end:
;   [37:6] assert(ok bool)
    func.assert.391.5:
;       [391:5] alias ok -> r15b
        if.37.27.391.5:
;       [37:27] ? not ok
;       [37:27] ? shorthand: not ok
        cmp.37.27.391.5:
        cmp r15b, 0
        jne if.37.24.391.5.end
        if.37.27.391.5.code:
;           [37:34] exit(1)
;           [37:34] allocate named register rdi
;           [37:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [37:34] free named register rdi
        if.37.24.391.5.end:
;       [391:5] free scratch register r15
    func.assert.391.5.end:
;   [393:5] var counter = 0
;   [393:9] counter: i64 (8 B @ [rbp + 1152])
;   [393:9] counter = 0
;   [393:19] 0
    mov qword [rbp + 1152], 0
;   [394:5] var nm = str{}
;   [394:9] nm: str (128 B @ [rbp + 1160])
;   [394:9] nm = str{}
;   [394:14] zero remaining fields: 128 B
;   [394:14] allocate named register rax
;   [394:14] allocate named register rdi
;   [394:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1160]
    mov rcx, 128
    rep stosb
;   [394:14] free named register rcx
;   [394:14] free named register rdi
;   [394:14] free named register rax
;   [395:5] print(hello)
;   [40:6] print(str i8[])
    func.print.395.5:
;       [395:5] alias str -> hello
;       [41:5] write(1, str)
;       [41:5] allocate named register rdi
;       [41:5] allocate named register rsi
;       [41:5] allocate named register rdx
;       [41:11] 1
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
;       [41:5] allocate named register rax
        mov rax, 1
        syscall
;       [41:5] free named register rax
;       [41:5] free named register rdx
;       [41:5] free named register rsi
;       [41:5] free named register rdi
    func.print.395.5.end:
;   [396:5] label
    loop.396.5:
;       [397:9] counter = counter + 1
;       [397:19] instructions without scratch register 1, with 3
;       [397:19] counter
;       [397:19] counter + 1
;       [397:19] src: folded constant '+ 1'
        add qword [rbp + 1152], 1
;       [398:9] print_num(counter)
;       [398:9] frame capacity check (--checks=frame)
;       [398:9] allocate scratch register -> r15
;       [398:9] allocate scratch register -> r14
        lea r15, [rbp + 1288]
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
;       [398:9] free scratch register r14
;       [398:9] free scratch register r15
;       [398:9] address of argument 'counter' to parameter 'num'
;       [398:9] allocate scratch register -> r15
        lea r15, [rbp + 1152]
        mov qword [rbp + 1288], r15
;       [398:9] free scratch register r15
;       [398:9] set function frame base
        lea rbx, [rbp + 1288]
        call func.print_num
;       [399:9] print(colon)
;       [40:6] print(str i8[])
        func.print.399.9:
;           [399:9] alias str -> colon
;           [41:5] write(1, str)
;           [41:5] allocate named register rdi
;           [41:5] allocate named register rsi
;           [41:5] allocate named register rdx
;           [41:11] 1
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
;           [41:5] allocate named register rax
            mov rax, 1
            syscall
;           [41:5] free named register rax
;           [41:5] free named register rdx
;           [41:5] free named register rsi
;           [41:5] free named register rdi
        func.print.399.9.end:
;       [400:9] print(prompt1)
;       [40:6] print(str i8[])
        func.print.400.9:
;           [400:9] alias str -> prompt1
;           [41:5] write(1, str)
;           [41:5] allocate named register rdi
;           [41:5] allocate named register rsi
;           [41:5] allocate named register rdx
;           [41:11] 1
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
;           [41:5] allocate named register rax
            mov rax, 1
            syscall
;           [41:5] free named register rax
;           [41:5] free named register rdx
;           [41:5] free named register rsi
;           [41:5] free named register rdi
        func.print.400.9.end:
;       [401:12] nm.input()
;       [82:6] str.input()
        func.str.input.401.12:
;           [401:12] alias self -> nm
;           [83:5] var nbytes = read(0, self.data)
;           [83:9] nbytes: i64 (8 B @ [rbp + 1288])
;           [83:9] nbytes = read(0, self.data)
;           [83:18] nbytes = read(0, self.data)
;           [83:18] = expression
;           [83:18] read(0, self.data)
;           [83:18] allocate named register rdi
;           [83:18] allocate named register rsi
;           [83:18] allocate named register rdx
;           [83:23] 0
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1161]
;           [83:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1288], rax
;           [83:18] free named register rax
;           [83:18] free named register rdx
;           [83:18] free named register rsi
;           [83:18] free named register rdi
;           [86:5] self.len = i8(nbytes - 1)
;           [86:16] self.len = i8(nbytes - 1)
;           [86:16] = expression
;           [86:16] instructions without scratch register 3, with 3
;           [86:19] instructions without scratch register 3, with 3
;           [86:19] nbytes
;           [86:19] allocate scratch register -> r15
            mov r15b, byte [rbp + 1288]
            mov byte [rbp + 1160], r15b
;           [86:19] free scratch register r15
;           [86:19] self.len - 1
;           [86:19] src: folded constant '- 1'
            sub byte [rbp + 1160], 1
        func.str.input.401.12.end:
        if.403.12:
;       [403:12] ? nm.len <= 0
;       [403:12] ? nm.len <= 0
        cmp.403.12:
        cmp byte [rbp + 1160], 0
        jle loop.396.5.end
        if.403.12.code:
;           [404:13] break
        if.405.19:
;       [405:19] ? nm.len <= 4
;       [405:19] ? nm.len <= 4
        cmp.405.19:
        cmp byte [rbp + 1160], 4
        jg if.403.9.else
        if.405.19.code:
;           [406:13] print(prompt2)
;           [40:6] print(str i8[])
            func.print.406.13:
;               [406:13] alias str -> prompt2
;               [41:5] write(1, str)
;               [41:5] allocate named register rdi
;               [41:5] allocate named register rsi
;               [41:5] allocate named register rdx
;               [41:11] 1
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
;               [41:5] allocate named register rax
                mov rax, 1
                syscall
;               [41:5] free named register rax
;               [41:5] free named register rdx
;               [41:5] free named register rsi
;               [41:5] free named register rdi
            func.print.406.13.end:
;           [407:13] continue
            jmp loop.396.5
        if.403.9.else:
;           [409:13] greet(nm)
;           [93:6] greet(name str)
            func.greet.409.13:
;               [409:13] alias name -> nm
;               [94:5] print(prompt3)
;               [40:6] print(str i8[])
                func.print.94.5.409.13:
;                   [94:5] alias str -> prompt3
;                   [41:5] write(1, str)
;                   [41:5] allocate named register rdi
;                   [41:5] allocate named register rsi
;                   [41:5] allocate named register rdx
;                   [41:11] 1
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
;                   [41:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [41:5] free named register rax
;                   [41:5] free named register rdx
;                   [41:5] free named register rsi
;                   [41:5] free named register rdi
                func.print.94.5.409.13.end:
;               [95:10] name.print()
;               [89:6] str.print()
                func.str.print.95.10.409.13:
;                   [95:10] alias self -> name
;                   [90:5] write(1, self.data, self.len)
;                   [90:5] allocate named register rdi
;                   [90:5] allocate named register rsi
;                   [90:5] allocate named register rdx
;                   [90:11] 1
                    mov rdi, 1
;                   [90:25] self.len
                    movsx rdx, byte [rbp + 1160]
;                   [90:14] bounds check
;                   [90:14] lower bound (--checks=lower)
                    test rdx, rdx
                    js baz_bounds_panic
;                   [90:14] upper bound (--checks=upper)
                    cmp rdx, 127
                    jg baz_bounds_panic
                    lea rsi, [rbp + 1161]
;                   [90:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [90:5] free named register rax
;                   [90:5] free named register rdx
;                   [90:5] free named register rsi
;                   [90:5] free named register rdi
                func.str.print.95.10.409.13.end:
;               [96:5] print(dot)
;               [40:6] print(str i8[])
                func.print.96.5.409.13:
;                   [96:5] alias str -> dot
;                   [41:5] write(1, str)
;                   [41:5] allocate named register rdi
;                   [41:5] allocate named register rsi
;                   [41:5] allocate named register rdx
;                   [41:11] 1
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
;                   [41:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [41:5] free named register rax
;                   [41:5] free named register rdx
;                   [41:5] free named register rsi
;                   [41:5] free named register rdi
                func.print.96.5.409.13.end:
;               [97:5] print(nl)
;               [40:6] print(str i8[])
                func.print.97.5.409.13:
;                   [97:5] alias str -> nl
;                   [41:5] write(1, str)
;                   [41:5] allocate named register rdi
;                   [41:5] allocate named register rsi
;                   [41:5] allocate named register rdx
;                   [41:11] 1
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
;                   [41:5] allocate named register rax
                    mov rax, 1
                    syscall
;                   [41:5] free named register rax
;                   [41:5] free named register rdx
;                   [41:5] free named register rsi
;                   [41:5] free named register rdi
                func.print.97.5.409.13.end:
;               [98:5] names = names + 1
;               [98:13] instructions without scratch register 1, with 3
;               [98:13] names
;               [98:13] names + 1
;               [98:13] src: folded constant '+ 1'
                add qword [rbp + 240], 1
            func.greet.409.13.end:
        if.403.9.end:
    jmp loop.396.5
    loop.396.5.end:
;   [413:5] print(greeted)
;   [40:6] print(str i8[])
    func.print.413.5:
;       [413:5] alias str -> greeted
;       [41:5] write(1, str)
;       [41:5] allocate named register rdi
;       [41:5] allocate named register rsi
;       [41:5] allocate named register rdx
;       [41:11] 1
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 224]
;       [41:5] allocate named register rax
        mov rax, 1
        syscall
;       [41:5] free named register rax
;       [41:5] free named register rdx
;       [41:5] free named register rsi
;       [41:5] free named register rdi
    func.print.413.5.end:
;   [414:5] print_num(names)
;   [414:5] frame capacity check (--checks=frame)
;   [414:5] allocate scratch register -> r15
;   [414:5] allocate scratch register -> r14
    lea r15, [rbp + 1288]
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
;   [414:5] free scratch register r14
;   [414:5] free scratch register r15
;   [414:5] address of argument 'names' to parameter 'num'
;   [414:5] allocate scratch register -> r15
    lea r15, [rbp + 240]
    mov qword [rbp + 1288], r15
;   [414:5] free scratch register r15
;   [414:5] set function frame base
    lea rbx, [rbp + 1288]
    call func.print_num
;   [415:5] print(nl)
;   [40:6] print(str i8[])
    func.print.415.5:
;       [415:5] alias str -> nl
;       [41:5] write(1, str)
;       [41:5] allocate named register rdi
;       [41:5] allocate named register rsi
;       [41:5] allocate named register rdx
;       [41:11] 1
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
;       [41:5] allocate named register rax
        mov rax, 1
        syscall
;       [41:5] free named register rax
;       [41:5] free named register rdx
;       [41:5] free named register rsi
;       [41:5] free named register rdi
    func.print.415.5.end:
;   [417:5] var bye = "bye from baz\n"
;   [417:9] bye: i8[13] (13 B @ [rbp + 1288])
;   [417:9] bye = "bye from baz\n"
;   [417:15] size <= 16 B, use immediates
    mov dword [rbp + 1288], 543521122
    mov dword [rbp + 1292], 1836020326
    mov dword [rbp + 1296], 2053202464
    mov byte [rbp + 1300], 10
;   [418:5] write(1, bye, 3)
;   [418:5] allocate named register rdi
;   [418:5] allocate named register rsi
;   [418:5] allocate named register rdx
;   [418:11] 1
    mov rdi, 1
;   [418:19] 3
    mov rdx, 3
;   [418:14] bounds check
;   [418:14] lower bound (--checks=lower)
    test rdx, rdx
    js baz_bounds_panic
;   [418:14] upper bound (--checks=upper)
    cmp rdx, 13
    jg baz_bounds_panic
    lea rsi, [rbp + 1288]
;   [418:5] allocate named register rax
    mov rax, 1
    syscall
;   [418:5] free named register rax
;   [418:5] free named register rdx
;   [418:5] free named register rsi
;   [418:5] free named register rdi
;   [419:5] write(1, bye, 1, array_length(bye) - 1)
;   [419:5] allocate named register rdi
;   [419:5] allocate named register rsi
;   [419:5] allocate named register rdx
;   [419:11] 1
    mov rdi, 1
;   [419:19] 1
    mov rdx, 1
;   [419:22] allocate scratch register -> r15
;   [419:22] r15 = array_length(bye)
;   [419:22] = expression
;   [419:22] array_length(bye)
    mov r15, 13
;   [419:22] r15 - 1
;   [419:22] src: folded constant '- 1'
    sub r15, 1
;   [419:22] bounds check
;   [419:22] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
    test rdx, rdx
    js baz_bounds_panic
;   [419:22] upper bound (--checks=upper)
;   [419:22] allocate scratch register -> r14
    mov r14, rdx
    add r14, r15
    cmp r14, 13
;   [419:22] free scratch register r14
    jg baz_bounds_panic
    lea rsi, [rbp + 1288]
    add rsi, r15
;   [419:5] free scratch register r15
;   [419:5] allocate named register rax
    mov rax, 1
    syscall
;   [419:5] free named register rax
;   [419:5] free named register rdx
;   [419:5] free named register rsi
;   [419:5] free named register rdi
    mov rdi, 0
    mov rax, 60
    syscall

;
;[127:15] noinline print_num(num)
func.print_num:
;   [127:25] num: i64 (8 B @ [rbx])
;   [129:11] const buf_count = 20
;   [131:5] var buf = i8[buf_count]{}
;   [131:9] buf: i8[20] (20 B @ [rbx + 8])
;   [131:9] buf = i8[buf_count]{}
;   [131:15] zero remaining elements: 20 * 1 B = 20 B
;   [131:15] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [132:5] var n = num
;   [132:9] n: i64 (8 B @ [rbx + 32])
;   [132:9] n = num
;   [132:13] num
;   [132:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [132:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [132:13] free scratch register r14
;   [132:13] free scratch register r15
;   [133:5] var is_negative = false
;   [133:9] is_negative: bool (1 B @ [rbx + 40])
;   [133:9] is_negative = false
    mov byte [rbx + 40], 0
    if.137.8:
;   [137:8] ? n < 0
;   [137:8] ? n < 0
    cmp.137.8:
    cmp qword [rbx + 32], 0
    jge if.137.5.end
    if.137.8.code:
;       [138:9] is_negative = true
        mov byte [rbx + 40], 1
    if.137.5.end:
    if.140.8:
;   [140:8] ? n > 0
;   [140:8] ? n > 0
    cmp.140.8:
    cmp qword [rbx + 32], 0
    jle if.140.5.end
    if.140.8.code:
;       [141:9] n = -n
;       [141:13] instructions without scratch register 1, with 3
;       [141:14] -n
        neg qword [rbx + 32]
    if.140.5.end:
;   [144:5] var i = buf_count
;   [144:9] i: i64 (8 B @ [rbx + 48])
;   [144:9] i = buf_count
;   [144:13] buf_count
    mov qword [rbx + 48], 20
;   [145:5] label
    loop.145.5:
;       [146:9] i = i - 1
;       [146:13] instructions without scratch register 1, with 3
;       [146:13] i
;       [146:13] i - 1
;       [146:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
;       [147:9] buf[i] = i8('0' - n % 10)
;       [147:13] allocate scratch register -> r15
;       [147:13] set array index
;       [147:13] i
        mov r15, qword [rbx + 48]
;       [147:13] bounds check
;       [147:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [147:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [147:18] buf = i8('0' - n % 10)
;       [147:18] = expression
;       [147:18] allocate scratch register -> r14
;           [147:21] r14 = 48
;           [147:21] src: folded constant '+ '0''
            mov r14, 48
;           [147:29] r14 - n % 10
;           [147:29] src: expression
;           [147:29] allocate scratch register -> r13
;           [147:27] n
            mov r13, qword [rbx + 32]
;           [147:31] r13 % 10
;           [147:31] src: constant
;           [147:31] allocate named register rax
            mov rax, r13
;           [147:31] allocate named register rdx
            cqo
;           [147:31] allocate scratch register -> r12
            mov r12, 10
            idiv r12
;           [147:31] free scratch register r12
            mov r13, rdx
;           [147:31] free named register rdx
;           [147:31] free named register rax
            sub r14, r13
;           [147:29] free scratch register r13
        mov byte [rbx + r15 + 8], r14b
;       [147:18] free scratch register r14
;       [147:9] free scratch register r15
;       [148:9] n = n / 10
;       [148:13] instructions without scratch register 5, with 7
;       [148:13] n
;       [148:17] n / 10
;       [148:17] src: constant
;       [148:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [148:17] allocate named register rdx
        cqo
;       [148:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [148:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [148:17] free named register rdx
;       [148:17] free named register rax
        if.149.12:
;       [149:12] ? n == 0
;       [149:12] ? n == 0
        cmp.149.12:
        cmp qword [rbx + 32], 0
        jne loop.145.5
        if.149.12.code:
;           [149:19] break
        if.149.9.end:
    loop.145.5.end:
    if.152.8:
;   [152:8] ? is_negative
;   [152:8] ? shorthand: is_negative
    cmp.152.8:
    cmp byte [rbx + 40], 0
    je if.152.5.end
    if.152.8.code:
;       [153:9] i = i - 1
;       [153:13] instructions without scratch register 1, with 3
;       [153:13] i
;       [153:13] i - 1
;       [153:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
;       [154:9] buf[i] = '-'
;       [154:13] allocate scratch register -> r15
;       [154:13] set array index
;       [154:13] i
        mov r15, qword [rbx + 48]
;       [154:13] bounds check
;       [154:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [154:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [154:18] '-'
        mov byte [rbx + r15 + 8], 45
;       [154:9] free scratch register r15
    if.152.5.end:
;   [157:5] var write_pos = 0
;   [157:9] write_pos: i64 (8 B @ [rbx + 56])
;   [157:9] write_pos = 0
;   [157:21] 0
    mov qword [rbx + 56], 0
;   [158:5] label
    loop.158.5:
;       [159:9] buf[write_pos] = buf[i]
;       [159:13] allocate scratch register -> r15
;       [159:13] set array index
;       [159:13] write_pos
        mov r15, qword [rbx + 56]
;       [159:13] bounds check
;       [159:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [159:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [159:26] buf[i]
;       [159:30] allocate scratch register -> r14
;       [159:30] set array index
;       [159:30] i
        mov r14, qword [rbx + 48]
;       [159:30] bounds check
;       [159:30] lower bound (--checks=lower)
        test r14, r14
        js baz_bounds_panic
;       [159:30] upper bound (--checks=upper)
        cmp r14, 20
        jge baz_bounds_panic
;       [159:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [159:26] free scratch register r13
;       [159:26] free scratch register r14
;       [159:9] free scratch register r15
;       [160:9] write_pos = write_pos + 1
;       [160:21] instructions without scratch register 1, with 3
;       [160:21] write_pos
;       [160:21] write_pos + 1
;       [160:21] src: folded constant '+ 1'
        add qword [rbx + 56], 1
;       [161:9] i = i + 1
;       [161:13] instructions without scratch register 1, with 3
;       [161:13] i
;       [161:13] i + 1
;       [161:13] src: folded constant '+ 1'
        add qword [rbx + 48], 1
        if.162.12:
;       [162:12] ? i == buf_count
;       [162:12] ? i == buf_count
        cmp.162.12:
        cmp qword [rbx + 48], 20
        jne loop.158.5
        if.162.12.code:
;           [162:27] break
        if.162.9.end:
    loop.158.5.end:
;   [165:5] write(1, buf, write_pos)
;   [165:5] allocate named register rdi
;   [165:5] allocate named register rsi
;   [165:5] allocate named register rdx
;   [165:11] 1
    mov rdi, 1
;   [165:19] write_pos
    mov rdx, qword [rbx + 56]
;   [165:14] bounds check
;   [165:14] lower bound (--checks=lower)
    test rdx, rdx
    js baz_bounds_panic
;   [165:14] upper bound (--checks=upper)
    cmp rdx, 20
    jg baz_bounds_panic
    lea rsi, [rbx + 8]
;   [165:5] allocate named register rax
    mov rax, 1
    syscall
;   [165:5] free named register rax
;   [165:5] free named register rdx
;   [165:5] free named register rsi
;   [165:5] free named register rdi
    ret
size.func.print_num equ 64
;
;[168:15] noinline factorial(n) res
func.factorial:
;   [168:28] res: i64 (8 B @ [rbx])
;   [168:25] n: i64 (8 B @ [rbx + 8])
;   [169:5] res = 1
;   [169:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [169:11] 1
    mov qword [r15], 1
;   [169:5] free scratch register r15
    if.170.8:
;   [170:8] ? n <= 1
;   [170:8] ? n <= 1
    cmp.170.8:
;   [170:8] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
;   [170:8] free scratch register r15
    jg if.170.5.end
    if.170.8.code:
;       [170:15] return
        ret
    if.170.5.end:
;   [172:5] var m = n - 1
;   [172:9] m: i64 (8 B @ [rbx + 16])
;   [172:9] m = n - 1
;   [172:13] instructions without scratch register 4, with 4
;   [172:13] n
;   [172:13] allocate scratch register -> r15
    mov r15, qword [rbx + 8]
;   [172:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
;   [172:13] free scratch register r14
;   [172:13] free scratch register r15
;   [172:13] m - 1
;   [172:13] src: folded constant '- 1'
    sub qword [rbx + 16], 1
;   [173:5] var partial = factorial(m)
;   [173:9] partial: i64 (8 B @ [rbx + 24])
;   [173:9] partial = factorial(m)
;   [173:19] partial = factorial(m)
;   [173:19] = expression
;   [173:19] factorial(m)
;   [173:19] frame capacity check (--checks=frame)
;   [173:19] allocate scratch register -> r15
;   [173:19] allocate scratch register -> r14
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
;   [173:19] free scratch register r14
;   [173:19] free scratch register r15
;   [173:19] result address in callee frame
;   [173:19] allocate scratch register -> r15
    lea r15, [rbx + 24]
    mov qword [rbx + 32], r15
;   [173:19] free scratch register r15
;   [173:19] address of argument 'm' to parameter 'n'
;   [173:19] allocate scratch register -> r15
    lea r15, [rbx + 16]
    mov qword [rbx + 40], r15
;   [173:19] free scratch register r15
;   [173:19] before call: save allocated registers
    push rbx
;   [173:19] set function frame base
    lea rbx, [rbx + 32]
    call func.factorial
;   [173:19] after call: restore saved registers
    pop rbx
;   [174:5] res = n * partial
;   [174:5] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [174:11] instructions without scratch register 6, with 4
;   [174:11] allocate scratch register -> r14
;   [174:11] n
;   [174:11] allocate scratch register -> r13
    mov r13, qword [rbx + 8]
    mov r14, qword [r13]
;   [174:11] free scratch register r13
;   [174:15] r14 * partial
;   [174:15] src: operand
    imul r14, qword [rbx + 24]
    mov qword [r15], r14
;   [174:11] free scratch register r14
;   [174:5] free scratch register r15
    ret
size.func.factorial equ 32
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
; bounds failure handler (--checks=upper or --checks=lower)
baz_bounds_panic:
;    exit with error code 255
    mov rax, 60
    mov rdi, 255
    syscall

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
;[29:21] [0]
;[29:21] i64
dq 1
;[29:15] pad 3 'i64' of size 8
times 24 db 0
;[30:8] str1
;[30:20] i8
db 3
;[30:23] i8[127]
db `baz`
;[30:23] zero remaining array
times 124 db 0
;[32:5] greeted
;[32:15] i8[15]
db `names greeted: `
; padding 1 B
times 1 db 0
;[33:7] names
;[33:15] i64
dq 0
dat.end:

section .bss.vars nobits alloc write
align 16
vars:
resb 65536
vars.end:
; free named register rbp

;   removed jumps to next code: 129
;    removed unreachable jumps: 2
; removed same target branches: 52
; inverted branches over jumps: 7

; max scratch registers in use: 4
;            max frames in use: 10
;              dat var padding: 8 B
;                max vars size: 1045 B
```
