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
* support for reentrant non-inlined functions
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
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `read`,
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
  upper  runtime upper array bounds only, a negative index passes
  lower  runtime lower array bounds, catches negative indexes
  line   report line number on failed bounds check
  frame  runtime non-inlined function frame capacity
  alias  compile time rejection of calls where a result may share storage
         with an argument
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
C/C++ Header                    53           7645           3165          22731
C++                              1            179             53            582
-------------------------------------------------------------------------------
SUM:                            54           7824           3218          23313
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
# type of "return" and arguments can be defined, use `i` for default integer type
# of target platform

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

    assert(a != 0 and (a >= 7 or a < 0))
    # `and`, `or` and `not` stop evaluating as soon as the result is known

    var small = i8(100)
    small = small + small
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
#   `--checks=alias` rejects this because "return" and the argument may share
#   storage

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
    cmp.218.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.218.12.end
    cmp.218.23:
    cmp.218.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.218.12.end
    cmp.218.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.218.12.end:
    func.assert.218.5:
        if.38.27.218.5:
        cmp.38.27.218.5:
        cmp r15b, 0
        jne if.38.24.218.5.end
        if.38.27.218.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.218.5.end:
    func.assert.218.5.end:
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.223.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.223.12.end:
    func.assert.223.5:
        if.38.27.223.5:
        cmp.38.27.223.5:
        cmp r15b, 0
        jne if.38.24.223.5.end
        if.38.27.223.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.223.5.end:
    func.assert.223.5.end:
    movsx r15, byte [rbp + 400]
    mov qword [rbp + 408], r15
    cmp.227.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.227.12.end:
    func.assert.227.5:
        if.38.27.227.5:
        cmp.38.27.227.5:
        cmp r15b, 0
        jne if.38.24.227.5.end
        if.38.27.227.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.227.5.end:
    func.assert.227.5.end:
    mov qword [rbp + 416], 65
    cmp.235.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.235.12.end:
    func.assert.235.5:
        if.38.27.235.5:
        cmp.38.27.235.5:
        cmp r15b, 0
        jne if.38.24.235.5.end
        if.38.27.235.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.235.5.end:
    func.assert.235.5.end:
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
    mov qword [rbp + 440], 1
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_241
    mov dword [rbp + r15 * 4 + 424], 2
    mov r15, qword [rbp + 440]
    add r15, 1
    cmp r15, 4
    jae baz_bounds_line_242
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_242
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
    cmp.243.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.243.12.end:
    func.assert.243.5:
        if.38.27.243.5:
        cmp.38.27.243.5:
        cmp r15b, 0
        jne if.38.24.243.5.end
        if.38.27.243.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.243.5.end:
    func.assert.243.5.end:
    cmp.244.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.244.12.end:
    func.assert.244.5:
        if.38.27.244.5:
        cmp.38.27.244.5:
        cmp r15b, 0
        jne if.38.24.244.5.end
        if.38.27.244.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.244.5.end:
    func.assert.244.5.end:
    mov r15, 2
    mov r14, 2
    test r14, r14
    js baz_bounds_line_246
    test r15, r15
    js baz_bounds_line_246
    lea r13, [r15 + r14]
    cmp r13, 4
    jg baz_bounds_line_246
    cmp r15, 4
    ja baz_bounds_line_246
    mov rax, qword [rbp + r14 * 4 + 424]
    mov qword [rbp + 424], rax
    cmp.247.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.247.12.end:
    func.assert.247.5:
        if.38.27.247.5:
        cmp.38.27.247.5:
        cmp r15b, 0
        jne if.38.24.247.5.end
        if.38.27.247.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.247.5.end:
    func.assert.247.5.end:
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
    mov r15, 4
    cmp r15, 4
    ja baz_bounds_line_251
    cmp r15, 8
    ja baz_bounds_line_251
    mov rax, qword [rbp + 424]
    mov qword [rbp + 448], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 456], rax
    cmp.252.14:
        mov rcx, 3
        mov r15, 1
        test r15, r15
        js baz_bounds_line_252
        test rcx, rcx
        js baz_bounds_line_252
        lea r14, [rcx + r15]
        cmp r14, 4
        jg baz_bounds_line_252
        lea rsi, [rbp + r15 * 4 + 424]
        mov r15, 1
        test r15, r15
        js baz_bounds_line_252
        lea r14, [rcx + r15]
        cmp r14, 8
        jg baz_bounds_line_252
        lea rdi, [rbp + r15 * 4 + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 480]
    bool.252.14.end:
    cmp.255.12:
    mov r15b, byte [rbp + 480]
    bool.255.12.end:
    func.assert.255.5:
        if.38.27.255.5:
        cmp.38.27.255.5:
        cmp r15b, 0
        jne if.38.24.255.5.end
        if.38.27.255.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.255.5.end:
    func.assert.255.5.end:
    mov dword [rbp + 456], -1
    cmp.258.12:
        mov rcx, 4
        cmp rcx, 4
        ja baz_bounds_line_258
        lea rsi, [rbp + 424]
        cmp rcx, 8
        ja baz_bounds_line_258
        lea rdi, [rbp + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.258.12.end:
    func.assert.258.5:
        if.38.27.258.5:
        cmp.38.27.258.5:
        cmp r15b, 0
        jne if.38.24.258.5.end
        if.38.27.258.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.258.5.end:
    func.assert.258.5.end:
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
    cmp.261.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    cmp r15b, 0
    bool.261.12.end:
    func.assert.261.5:
        if.38.27.261.5:
        cmp.38.27.261.5:
        cmp r15b, 0
        jne if.38.24.261.5.end
        if.38.27.261.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.261.5.end:
    func.assert.261.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    sub r15, 1
    cmp r15, 4
    jae baz_bounds_line_269
    func.inv.269.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv.269.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_270
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.271.12:
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_271
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
    bool.271.12.end:
    func.assert.271.5:
        if.38.27.271.5:
        cmp.38.27.271.5:
        cmp r15b, 0
        jne if.38.24.271.5.end
        if.38.27.271.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.271.5.end:
    func.assert.271.5.end:
    func.faz.273.5:
        mov dword [rbp + 428], 254
    func.faz.273.5.end:
    cmp.274.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.274.12.end:
    func.assert.274.5:
        if.38.27.274.5:
        cmp.38.27.274.5:
        cmp r15b, 0
        jne if.38.24.274.5.end
        if.38.27.274.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.274.5.end:
    func.assert.274.5.end:
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov r14, 0
    foo.277.5:
        add qword [r15], r14
        add qword [r15], 2
        foo.277.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.277.5
    foo.277.5.end:
    cmp.280.12:
    cmp qword [rbp + 504], 5
    sete r15b
    bool.280.12.end:
    func.assert.280.5:
        if.38.27.280.5:
        cmp.38.27.280.5:
        cmp r15b, 0
        jne if.38.24.280.5.end
        if.38.27.280.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.280.5.end:
    func.assert.280.5.end:
    cmp.281.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.281.12.end:
    func.assert.281.5:
        if.38.27.281.5:
        cmp.38.27.281.5:
        cmp r15b, 0
        jne if.38.24.281.5.end
        if.38.27.281.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.281.5.end:
    func.assert.281.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz.289.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz.289.7.end:
    cmp.292.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.292.12.end:
    func.assert.292.5:
        if.38.27.292.5:
        cmp.38.27.292.5:
        cmp r15b, 0
        jne if.38.24.292.5.end
        if.38.27.292.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.292.5.end:
    func.assert.292.5.end:
    cmp.293.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.293.12.end:
    func.assert.293.5:
        if.38.27.293.5:
        cmp.38.27.293.5:
        cmp r15b, 0
        jne if.38.24.293.5.end
        if.38.27.293.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.293.5.end:
    func.assert.293.5.end:
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
    cmp.298.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    cmp r15b, 0
    bool.298.12.end:
    func.assert.298.5:
        if.38.27.298.5:
        cmp.38.27.298.5:
        cmp r15b, 0
        jne if.38.24.298.5.end
        if.38.27.298.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.298.5.end:
    func.assert.298.5.end:
    mov qword [rbp + 536], 3
    cmp.303.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    cmp r15b, 0
    bool.303.12.end:
    func.assert.303.5:
        if.38.27.303.5:
        cmp.38.27.303.5:
        cmp r15b, 0
        jne if.38.24.303.5.end
        if.38.27.303.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.303.5.end:
    func.assert.303.5.end:
    mov qword [rbp + 552], 0
    func.bar.306.5:
        if.60.8.306.5:
        cmp.60.8.306.5:
        cmp qword [rbp + 552], 0
        je func.bar.306.5.end
        if.60.8.306.5.code:
        if.60.5.306.5.end:
        mov qword [rbp + 552], 255
    func.bar.306.5.end:
    cmp.307.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.307.12.end:
    func.assert.307.5:
        if.38.27.307.5:
        cmp.38.27.307.5:
        cmp r15b, 0
        jne if.38.24.307.5.end
        if.38.27.307.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.307.5.end:
    func.assert.307.5.end:
    mov qword [rbp + 552], 1
    func.bar.310.5:
        if.60.8.310.5:
        cmp.60.8.310.5:
        cmp qword [rbp + 552], 0
        je func.bar.310.5.end
        if.60.8.310.5.code:
        if.60.5.310.5.end:
        mov qword [rbp + 552], 255
    func.bar.310.5.end:
    cmp.311.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.311.12.end:
    func.assert.311.5:
        if.38.27.311.5:
        cmp.38.27.311.5:
        cmp r15b, 0
        jne if.38.24.311.5.end
        if.38.27.311.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.311.5.end:
    func.assert.311.5.end:
    mov qword [rbp + 560], 1
    func.baz.314.13:
        mov r15, qword [rbp + 560]
        mov qword [rbp + 568], r15
        sal qword [rbp + 568], 1
    func.baz.314.13.end:
    cmp.315.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.315.12.end:
    func.assert.315.5:
        if.38.27.315.5:
        cmp.38.27.315.5:
        cmp r15b, 0
        jne if.38.24.315.5.end
        if.38.27.315.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.315.5.end:
    func.assert.315.5.end:
    func.baz.317.9:
        mov qword [rbp + 568], 2
    func.baz.317.9.end:
    cmp.318.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.318.12.end:
    func.assert.318.5:
        if.38.27.318.5:
        cmp.38.27.318.5:
        cmp r15b, 0
        jne if.38.24.318.5.end
        if.38.27.318.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.318.5.end:
    func.assert.318.5.end:
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
    cmp.322.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.322.12.end:
    func.assert.322.5:
        if.38.27.322.5:
        cmp.38.27.322.5:
        cmp r15b, 0
        jne if.38.24.322.5.end
        if.38.27.322.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.322.5.end:
    func.assert.322.5.end:
    func.baz.324.20:
        mov qword [rbp + 592], 6
    func.baz.324.20.end:
    mov qword [rbp + 600], 0
    cmp.325.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.325.12.end:
    func.assert.325.5:
        if.38.27.325.5:
        cmp.38.27.325.5:
        cmp r15b, 0
        jne if.38.24.325.5.end
        if.38.27.325.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.325.5.end:
    func.assert.325.5.end:
    func.point.at.327.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at.327.14.end:
    cmp.331.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.331.12.end:
    func.assert.331.5:
        if.38.27.331.5:
        cmp.38.27.331.5:
        cmp r15b, 0
        jne if.38.24.331.5.end
        if.38.27.331.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.331.5.end:
    func.assert.331.5.end:
    cmp.332.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.332.12.end:
    func.assert.332.5:
        if.38.27.332.5:
        cmp.38.27.332.5:
        cmp r15b, 0
        jne if.38.24.332.5.end
        if.38.27.332.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.332.5.end:
    func.assert.332.5.end:
    func.point.x.334.8:
        mov qword [rbp + 608], 2
    func.point.x.334.8.end:
    cmp.335.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.335.12.end:
    func.assert.335.5:
        if.38.27.335.5:
        cmp.38.27.335.5:
        cmp r15b, 0
        jne if.38.24.335.5.end
        if.38.27.335.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.335.5.end:
    func.assert.335.5.end:
    cmp.336.12:
        func.point.sum.336.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
        func.point.sum.336.15.end:
    cmp r14, 0
    sete r15b
    bool.336.12.end:
    func.assert.336.5:
        if.38.27.336.5:
        cmp.38.27.336.5:
        cmp r15b, 0
        jne if.38.24.336.5.end
        if.38.27.336.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.336.5.end:
    func.assert.336.5.end:
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.342.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.342.12.end:
    func.assert.342.5:
        if.38.27.342.5:
        cmp.38.27.342.5:
        cmp r15b, 0
        jne if.38.24.342.5.end
        if.38.27.342.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.342.5.end:
    func.assert.342.5.end:
    cmp.343.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.343.12.end:
    func.assert.343.5:
        if.38.27.343.5:
        cmp.38.27.343.5:
        cmp r15b, 0
        jne if.38.24.343.5.end
        if.38.27.343.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.343.5.end:
    func.assert.343.5.end:
    cmp.344.12:
    cmp dword [rbp + 656], 16711680
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
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
    neg qword [rbp + 664]
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
    neg qword [rbp + 672]
    mov rax, qword [rbp + 664]
    mov qword [rbp + 640], rax
    mov rax, qword [rbp + 672]
    mov qword [rbp + 648], rax
    cmp.348.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.348.12.end:
    func.assert.348.5:
        if.38.27.348.5:
        cmp.38.27.348.5:
        cmp r15b, 0
        jne if.38.24.348.5.end
        if.38.27.348.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.348.5.end:
    func.assert.348.5.end:
    cmp.349.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.349.12.end:
    func.assert.349.5:
        if.38.27.349.5:
        cmp.38.27.349.5:
        cmp r15b, 0
        jne if.38.24.349.5.end
        if.38.27.349.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.349.5.end:
    func.assert.349.5.end:
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.352.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.352.12.end:
    func.assert.352.5:
        if.38.27.352.5:
        cmp.38.27.352.5:
        cmp r15b, 0
        jne if.38.24.352.5.end
        if.38.27.352.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.352.5.end:
    func.assert.352.5.end:
    cmp.353.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.353.12.end:
    func.assert.353.5:
        if.38.27.353.5:
        cmp.38.27.353.5:
        cmp r15b, 0
        jne if.38.24.353.5.end
        if.38.27.353.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.353.5.end:
    func.assert.353.5.end:
    cmp.354.12:
    cmp dword [rbp + 696], 16711680
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
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.357.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.357.12.end:
    func.assert.357.5:
        if.38.27.357.5:
        cmp.38.27.357.5:
        cmp r15b, 0
        jne if.38.24.357.5.end
        if.38.27.357.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.357.5.end:
    func.assert.357.5.end:
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.359.12:
    cmp qword [rbp + 680], 2
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
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.369.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.369.12.end:
    func.assert.369.5:
        if.38.27.369.5:
        cmp.38.27.369.5:
        cmp r15b, 0
        jne if.38.24.369.5.end
        if.38.27.369.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.369.5.end:
    func.assert.369.5.end:
    mov dword [rbp + 748], 0
    func.object.at.370.13:
        func.point.at.120.16.370.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at.120.16.370.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at.370.13.end:
    cmp.371.12:
    cmp qword [rbp + 736], 74
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
    func.point.fooz.373.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz.373.15.end:
    cmp.374.12:
        func.point.sum.374.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
        func.point.sum.374.22.end:
    cmp r14, 13
    sete r15b
    bool.374.12.end:
    func.assert.374.5:
        if.38.27.374.5:
        cmp.38.27.374.5:
        cmp r15b, 0
        jne if.38.24.374.5.end
        if.38.27.374.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.374.5.end:
    func.assert.374.5.end:
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.379.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.379.12.end:
    func.assert.379.5:
        if.38.27.379.5:
        cmp.38.27.379.5:
        cmp r15b, 0
        jne if.38.24.379.5.end
        if.38.27.379.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.379.5.end:
    func.assert.379.5.end:
    mov r15, 8
    cmp r15, 8
    ja baz_bounds_line_382
    cmp r15, 8
    ja baz_bounds_line_383
    lea rsi, [rbp + 816]
    lea rdi, [rbp + 752]
    mov rcx, 64
    rep movsb
    cmp.388.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.388.12.end:
    func.assert.388.5:
        if.38.27.388.5:
        cmp.38.27.388.5:
        cmp r15b, 0
        jne if.38.24.388.5.end
        if.38.27.388.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.388.5.end:
    func.assert.388.5.end:
    cmp.389.12:
        mov rcx, 8
        cmp rcx, 8
        ja baz_bounds_line_390
        lea rsi, [rbp + 752]
        cmp rcx, 8
        ja baz_bounds_line_391
        lea rdi, [rbp + 816]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.389.12.end:
    func.assert.389.5:
        if.38.27.389.5:
        cmp.38.27.389.5:
        cmp r15b, 0
        jne if.38.24.389.5.end
        if.38.27.389.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.389.5.end:
    func.assert.389.5.end:
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    func.assert.396.5:
        if.38.27.396.5:
        cmp.38.27.396.5:
        if.38.24.396.5.end:
    func.assert.396.5.end:
    cmp.397.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.397.12.end:
    func.assert.397.5:
        if.38.27.397.5:
        cmp.38.27.397.5:
        cmp r15b, 0
        jne if.38.24.397.5.end
        if.38.27.397.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.397.5.end:
    func.assert.397.5.end:
    cmp.398.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.398.12.end:
    func.assert.398.5:
        if.38.27.398.5:
        cmp.38.27.398.5:
        cmp r15b, 0
        jne if.38.24.398.5.end
        if.38.27.398.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.398.5.end:
    func.assert.398.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print.402.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.402.5.end:
    loop.403.5:
        add qword [rbp + 1280], 1
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
        func.print.406.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.406.9.end:
        func.print.407.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.407.9.end:
        func.str.input.408.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
            sub byte [rbp + 1288], 1
        func.str.input.408.12.end:
        if.410.12:
        cmp.410.12:
        cmp byte [rbp + 1288], 0
        jle loop.403.5.end
        if.410.12.code:
        if.412.19:
        cmp.412.19:
        cmp byte [rbp + 1288], 4
        jg if.410.9.else
        if.412.19.code:
            func.print.413.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.413.13.end:
            jmp loop.403.5
        if.410.9.else:
            func.greet.416.13:
                func.print.100.5.416.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.100.5.416.13.end:
                func.str.print.101.10.416.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1288]
                    cmp rdx, 127
                    ja baz_bounds_line_96
                    lea rsi, [rbp + 1289]
                    mov rax, 1
                    syscall
                func.str.print.101.10.416.13.end:
                func.print.102.5.416.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.102.5.416.13.end:
                func.print.103.5.416.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.103.5.416.13.end:
                add qword [rbp + 368], 1
            func.greet.416.13.end:
        if.410.9.end:
    jmp loop.403.5
    loop.403.5.end:
    func.print.420.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
        mov rax, 1
        syscall
    func.print.420.5.end:
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
    func.print.422.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.422.5.end:
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
    mov rdi, 1
    mov rdx, 3
    cmp rdx, 13
    ja baz_bounds_line_425
    lea rsi, [rbp + 1416]
    mov rax, 1
    syscall
    mov rdi, 1
    mov rdx, 1
    mov r15, 12
    test r15, r15
    js baz_bounds_line_426
    test rdx, rdx
    js baz_bounds_line_426
    lea r14, [rdx + r15]
    cmp r14, 13
    jg baz_bounds_line_426
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
    if.147.5.end:
    mov qword [rbx + 48], 20
    loop.152.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_154
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
        add qword [rbx + 48], 1
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
baz_bounds_line_241:
    mov rbp, 241
    jmp baz_bounds_panic
baz_bounds_line_242:
    mov rbp, 242
    jmp baz_bounds_panic
baz_bounds_line_246:
    mov rbp, 246
    jmp baz_bounds_panic
baz_bounds_line_251:
    mov rbp, 251
    jmp baz_bounds_panic
baz_bounds_line_252:
    mov rbp, 252
    jmp baz_bounds_panic
baz_bounds_line_258:
    mov rbp, 258
    jmp baz_bounds_panic
baz_bounds_line_269:
    mov rbp, 269
    jmp baz_bounds_panic
baz_bounds_line_270:
    mov rbp, 270
    jmp baz_bounds_panic
baz_bounds_line_271:
    mov rbp, 271
    jmp baz_bounds_panic
baz_bounds_line_382:
    mov rbp, 382
    jmp baz_bounds_panic
baz_bounds_line_383:
    mov rbp, 383
    jmp baz_bounds_panic
baz_bounds_line_390:
    mov rbp, 390
    jmp baz_bounds_panic
baz_bounds_line_391:
    mov rbp, 391
    jmp baz_bounds_panic
baz_bounds_line_425:
    mov rbp, 425
    jmp baz_bounds_panic
baz_bounds_line_426:
    mov rbp, 426
baz_bounds_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
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
section .rodata
msg_panic:
db `panic: bounds at line `
msg_panic_len equ $ - msg_panic
section .bss
num_buffer:
resb 21
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
;   [218:5] assert(a != 0 and (a >= 7 or a < 0))
;   [218:12] allocate scratch register -> r15
;   [218:12] ? a != 0 and (a >= 7 or a < 0)
;   [218:12] ? a != 0
    cmp.218.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.218.12.end
    cmp.218.23:
;   [218:23] ? (a >= 7 or a < 0)
;   [218:24] ? a >= 7
    cmp.218.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.218.12.end
;   [218:34] ? a < 0
    cmp.218.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.218.12.end:
;   [38:6] assert(ok bool)
    func.assert.218.5:
;       [218:5] alias ok -> r15b
        if.38.27.218.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.218.5:
        cmp r15b, 0
        jne if.38.24.218.5.end
        if.38.27.218.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.218.5.end:
;       [218:5] free scratch register r15
    func.assert.218.5.end:
;   [221:5] var small = i8(100)
;   [221:9] small: i8 (1 B @ [rbp + 400])
;   [221:9] small = i8(100)
    mov byte [rbp + 400], 100
;   [222:5] small = small + small
;   [222:13] instructions without scratch register 3, with 4
;   [222:13] allocate scratch register -> r15
;   [222:13] small
    mov r15b, byte [rbp + 400]
;   [222:21] r15b + small
;   [222:21] src: operand
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
;   [222:13] free scratch register r15
;   [223:5] assert(small == -56)
;   [223:12] allocate scratch register -> r15
;   [223:12] ? small == -56
;   [223:12] ? small == -56
    cmp.223.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.223.12.end:
;   [38:6] assert(ok bool)
    func.assert.223.5:
;       [223:5] alias ok -> r15b
        if.38.27.223.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.223.5:
        cmp r15b, 0
        jne if.38.24.223.5.end
        if.38.27.223.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.223.5.end:
;       [223:5] free scratch register r15
    func.assert.223.5.end:
;   [226:5] var wide = int(i16(small))
;   [226:9] wide: i64 (8 B @ [rbp + 408])
;   [226:9] wide = int(i16(small))
;   [226:16] wide = int(i16(small))
;   [226:16] = expression
;   [226:16] instructions without scratch register 2, with 3
;   [226:20] wide = i16(small)
;   [226:20] = expression
;   [226:20] instructions without scratch register 2, with 3
;   [226:24] small
;   [226:24] allocate scratch register -> r15
    movsx r15, byte [rbp + 400]
    mov qword [rbp + 408], r15
;   [226:24] free scratch register r15
;   [227:5] assert(wide == -56)
;   [227:12] allocate scratch register -> r15
;   [227:12] ? wide == -56
;   [227:12] ? wide == -56
    cmp.227.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.227.12.end:
;   [38:6] assert(ok bool)
    func.assert.227.5:
;       [227:5] alias ok -> r15b
        if.38.27.227.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.227.5:
        cmp r15b, 0
        jne if.38.24.227.5.end
        if.38.27.227.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.227.5.end:
;       [227:5] free scratch register r15
    func.assert.227.5.end:
;   [234:5] var letter = '\x41'
;   [234:9] letter: i64 (8 B @ [rbp + 416])
;   [234:9] letter = '\x41'
;   [234:18] '\x41'
    mov qword [rbp + 416], 65
;   [235:5] assert(letter == 'A')
;   [235:12] allocate scratch register -> r15
;   [235:12] ? letter == 'A'
;   [235:12] ? letter == 'A'
    cmp.235.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.235.12.end:
;   [38:6] assert(ok bool)
    func.assert.235.5:
;       [235:5] alias ok -> r15b
        if.38.27.235.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.235.5:
        cmp r15b, 0
        jne if.38.24.235.5.end
        if.38.27.235.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.235.5.end:
;       [235:5] free scratch register r15
    func.assert.235.5.end:
;   [238:5] var arr = i32[4]
;   [238:9] arr: i32[4] (16 B @ [rbp + 424])
;   [238:9] arr = i32[4]
;   [238:15] zero remaining elements: 4 * 4 B = 16 B
;   [238:15] size <= 32 B, use mov
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
;   [240:5] var ix = 1
;   [240:9] ix: i64 (8 B @ [rbp + 440])
;   [240:9] ix = 1
;   [240:14] 1
    mov qword [rbp + 440], 1
;   [241:5] arr[ix] = 2
;   [241:9] allocate scratch register -> r15
;   [241:9] set array index
;   [241:9] ix
    mov r15, qword [rbp + 440]
;   [241:9] bounds check begin
;   [241:9] lower bound
;   [241:9] r15 lower bound covered by the unsigned upper bound
;   [241:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_241
;   [241:9] bounds check end
;   [241:15] 2
    mov dword [rbp + r15 * 4 + 424], 2
;   [241:5] free scratch register r15
;   [242:5] arr[ix + 1] = arr[ix]
;   [242:9] allocate scratch register -> r15
;   [242:9] set array index
;   [242:9] ix
    mov r15, qword [rbp + 440]
;   [242:9] r15 + 1
;   [242:9] src: folded constant '+ 1'
    add r15, 1
;   [242:9] bounds check begin
;   [242:9] lower bound
;   [242:9] r15 lower bound covered by the unsigned upper bound
;   [242:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_242
;   [242:9] bounds check end
;   [242:19] arr[ix]
;   [242:23] allocate scratch register -> r14
;   [242:23] set array index
;   [242:23] ix
    mov r14, qword [rbp + 440]
;   [242:23] bounds check begin
;   [242:23] lower bound
;   [242:23] r14 lower bound covered by the unsigned upper bound
;   [242:23] upper bound
    cmp r14, 4
    jae baz_bounds_line_242
;   [242:23] bounds check end
;   [242:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
;   [242:19] free scratch register r13
;   [242:19] free scratch register r14
;   [242:5] free scratch register r15
;   [243:5] assert(arr[1] == 2)
;   [243:12] allocate scratch register -> r15
;   [243:12] ? arr[1] == 2
;   [243:12] ? arr[1] == 2
    cmp.243.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.243.12.end:
;   [38:6] assert(ok bool)
    func.assert.243.5:
;       [243:5] alias ok -> r15b
        if.38.27.243.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.243.5:
        cmp r15b, 0
        jne if.38.24.243.5.end
        if.38.27.243.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.243.5.end:
;       [243:5] free scratch register r15
    func.assert.243.5.end:
;   [244:5] assert(arr[2] == 2)
;   [244:12] allocate scratch register -> r15
;   [244:12] ? arr[2] == 2
;   [244:12] ? arr[2] == 2
    cmp.244.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.244.12.end:
;   [38:6] assert(ok bool)
    func.assert.244.5:
;       [244:5] alias ok -> r15b
        if.38.27.244.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.244.5:
        cmp r15b, 0
        jne if.38.24.244.5.end
        if.38.27.244.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.244.5.end:
;       [244:5] free scratch register r15
    func.assert.244.5.end:
;   [246:5] array_copy(arr[2], arr, 2)
;   [246:5] allocate scratch register -> r15
;   [246:29] 2
;   [246:29] 2
    mov r15, 2
;   [246:16] arr[2]
;   [246:20] allocate scratch register -> r14
;   [246:20] set array index
;   [246:20] 2
    mov r14, 2
;   [246:20] bounds check begin
;   [246:20] lower bound
    test r14, r14
    js baz_bounds_line_246
    test r15, r15
    js baz_bounds_line_246
;   [246:20] upper bound
;   [246:20] allocate scratch register -> r13
    lea r13, [r15 + r14]
    cmp r13, 4
;   [246:20] free scratch register r13
    jg baz_bounds_line_246
;   [246:20] bounds check end
;   [246:24] arr
;   [246:24] bounds check begin
;   [246:24] lower bound
;   [246:24] r15 lower bound covered by the unsigned upper bound
;   [246:24] upper bound
    cmp r15, 4
    ja baz_bounds_line_246
;   [246:24] bounds check end
;   [246:5] size <= 16 B, use mov
;   [246:5] allocate named register rax
    mov rax, qword [rbp + r14 * 4 + 424]
    mov qword [rbp + 424], rax
;   [246:5] free named register rax
;   [246:5] free scratch register r14
;   [246:5] free scratch register r15
;   [247:5] assert(arr[0] == 2)
;   [247:12] allocate scratch register -> r15
;   [247:12] ? arr[0] == 2
;   [247:12] ? arr[0] == 2
    cmp.247.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.247.12.end:
;   [38:6] assert(ok bool)
    func.assert.247.5:
;       [247:5] alias ok -> r15b
        if.38.27.247.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.247.5:
        cmp r15b, 0
        jne if.38.24.247.5.end
        if.38.27.247.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.247.5.end:
;       [247:5] free scratch register r15
    func.assert.247.5.end:
;   [250:5] var arr1 = i32[8]
;   [250:9] arr1: i32[8] (32 B @ [rbp + 448])
;   [250:9] arr1 = i32[8]
;   [250:16] zero remaining elements: 8 * 4 B = 32 B
;   [250:16] size <= 32 B, use mov
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
;   [251:5] array_copy(arr, arr1, 4)
;   [251:5] allocate scratch register -> r15
;   [251:27] 4
;   [251:27] 4
    mov r15, 4
;   [251:16] arr
;   [251:16] bounds check begin
;   [251:16] lower bound
;   [251:16] r15 lower bound covered by the unsigned upper bound
;   [251:16] upper bound
    cmp r15, 4
    ja baz_bounds_line_251
;   [251:16] bounds check end
;   [251:21] arr1
;   [251:21] bounds check begin
;   [251:21] lower bound
;   [251:21] r15 lower bound covered by the unsigned upper bound
;   [251:21] upper bound
    cmp r15, 8
    ja baz_bounds_line_251
;   [251:21] bounds check end
;   [251:5] size <= 16 B, use mov
;   [251:5] allocate named register rax
    mov rax, qword [rbp + 424]
    mov qword [rbp + 448], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 456], rax
;   [251:5] free named register rax
;   [251:5] free scratch register r15
;   [252:5] var eq = arrays_equal(arr[1], arr1[1], 3)
;   [252:9] eq: bool (1 B @ [rbp + 480])
;   [252:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [252:14] ? arrays_equal(arr[1], arr1[1], 3)
;   [252:14] ? shorthand: arrays_equal(arr[1], arr1[1], 3)
    cmp.252.14:
;       [252:14] arrays_equal(arr[1], arr1[1], 3)
;       [252:14] allocate named register rsi
;       [252:14] allocate named register rdi
;       [252:14] allocate named register rcx
;       [252:44] 3
;       [252:44] 3
        mov rcx, 3
;       [252:27] arr[1]
;       [252:31] allocate scratch register -> r15
;       [252:31] set array index
;       [252:31] 1
        mov r15, 1
;       [252:31] bounds check begin
;       [252:31] lower bound
        test r15, r15
        js baz_bounds_line_252
        test rcx, rcx
        js baz_bounds_line_252
;       [252:31] upper bound
;       [252:31] allocate scratch register -> r14
        lea r14, [rcx + r15]
        cmp r14, 4
;       [252:31] free scratch register r14
        jg baz_bounds_line_252
;       [252:31] bounds check end
        lea rsi, [rbp + r15 * 4 + 424]
;       [252:14] free scratch register r15
;       [252:35] arr1[1]
;       [252:40] allocate scratch register -> r15
;       [252:40] set array index
;       [252:40] 1
        mov r15, 1
;       [252:40] bounds check begin
;       [252:40] lower bound
;       [252:40] count rcx lower bound already checked
        test r15, r15
        js baz_bounds_line_252
;       [252:40] upper bound
;       [252:40] allocate scratch register -> r14
        lea r14, [rcx + r15]
        cmp r14, 8
;       [252:40] free scratch register r14
        jg baz_bounds_line_252
;       [252:40] bounds check end
        lea rdi, [rbp + r15 * 4 + 448]
;       [252:14] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [252:14] free named register rcx
;       [252:14] free named register rdi
;       [252:14] free named register rsi
        sete byte [rbp + 480]
    bool.252.14.end:
;   [255:5] assert(eq)
;   [255:12] allocate scratch register -> r15
;   [255:12] ? eq
;   [255:12] ? shorthand: eq
    cmp.255.12:
    mov r15b, byte [rbp + 480]
    bool.255.12.end:
;   [38:6] assert(ok bool)
    func.assert.255.5:
;       [255:5] alias ok -> r15b
        if.38.27.255.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.255.5:
        cmp r15b, 0
        jne if.38.24.255.5.end
        if.38.27.255.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.255.5.end:
;       [255:5] free scratch register r15
    func.assert.255.5.end:
;   [257:5] arr1[2] = -1
;   [257:15] instructions without scratch register 1, with 2
;   [257:16] -1
    mov dword [rbp + 456], -1
;   [258:5] assert(not arrays_equal(arr, arr1, 4))
;   [258:12] allocate scratch register -> r15
;   [258:12] ? not arrays_equal(arr, arr1, 4)
;   [258:12] ? shorthand: not arrays_equal(arr, arr1, 4)
    cmp.258.12:
;       [258:16] arrays_equal(arr, arr1, 4)
;       [258:16] allocate named register rsi
;       [258:16] allocate named register rdi
;       [258:16] allocate named register rcx
;       [258:40] 4
;       [258:40] 4
        mov rcx, 4
;       [258:29] arr
;       [258:29] bounds check begin
;       [258:29] lower bound
;       [258:29] rcx lower bound covered by the unsigned upper bound
;       [258:29] upper bound
        cmp rcx, 4
        ja baz_bounds_line_258
;       [258:29] bounds check end
        lea rsi, [rbp + 424]
;       [258:34] arr1
;       [258:34] bounds check begin
;       [258:34] lower bound
;       [258:34] rcx lower bound covered by the unsigned upper bound
;       [258:34] upper bound
        cmp rcx, 8
        ja baz_bounds_line_258
;       [258:34] bounds check end
        lea rdi, [rbp + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [258:16] free named register rcx
;       [258:16] free named register rdi
;       [258:16] free named register rsi
        setne r15b
    bool.258.12.end:
;   [38:6] assert(ok bool)
    func.assert.258.5:
;       [258:5] alias ok -> r15b
        if.38.27.258.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.258.5:
        cmp r15b, 0
        jne if.38.24.258.5.end
        if.38.27.258.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.258.5.end:
;       [258:5] free scratch register r15
    func.assert.258.5.end:
;   [260:5] var arr4 = arr
;   [260:9] arr4: i32[4] (16 B @ [rbp + 484])
;   [260:9] arr4 = arr
;   [260:16] size <= 16 B, use mov
;   [260:16] allocate named register rax
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
;   [260:16] free named register rax
;   [261:5] assert(arr == arr4)
;   [261:12] allocate scratch register -> r15
;   [261:12] ? arr == arr4
;   [261:12] ? arr == arr4
    cmp.261.12:
;       [261:12] allocate named register rsi
;       [261:12] allocate named register rdi
;       [261:12] allocate named register rcx
;       [261:12] arr
        lea rsi, [rbp + 424]
;       [261:19] arr4
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
;       [261:12] free named register rcx
;       [261:12] free named register rdi
;       [261:12] free named register rsi
        sete r15b
    cmp r15b, 0
    bool.261.12.end:
;   [38:6] assert(ok bool)
    func.assert.261.5:
;       [261:5] alias ok -> r15b
        if.38.27.261.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.261.5:
        cmp r15b, 0
        jne if.38.24.261.5.end
        if.38.27.261.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.261.5.end:
;       [261:5] free scratch register r15
    func.assert.261.5.end:
;   [268:5] ix = 3
;   [268:10] 3
    mov qword [rbp + 440], 3
;   [269:5] var tmp = ~inv(arr[ix - 1])
;   [269:9] tmp: i32 (4 B @ [rbp + 500])
;   [269:9] tmp = ~inv(arr[ix - 1])
;   [269:16] tmp = ~inv(arr[ix - 1])
;   [269:16] = expression
;   [269:16] ~inv(arr[ix - 1])
;   [269:16] instructions without scratch register 10, with 10
;   [269:24] allocate scratch register -> r15
;   [269:24] set array index
;   [269:24] ix
    mov r15, qword [rbp + 440]
;   [269:24] r15 - 1
;   [269:24] src: folded constant '- 1'
    sub r15, 1
;   [269:24] bounds check begin
;   [269:24] lower bound
;   [269:24] r15 lower bound covered by the unsigned upper bound
;   [269:24] upper bound
    cmp r15, 4
    jae baz_bounds_line_269
;   [269:24] bounds check end
;   [269:16] instructions without scratch register 6, with 7
;   [72:6] inv(i i32) res i32
    func.inv.269.16:
;       [269:16] alias res -> tmp
;       [269:16] alias i -> arr (lea: rbp + r15 * 4 + 424)
;       [73:5] res = ~i
;       [73:11] instructions without scratch register 3, with 3
;       [73:12] ~i
;       [73:12] allocate scratch register -> r14
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
;       [73:12] free scratch register r14
        not dword [rbp + 500]
    func.inv.269.16.end:
    not dword [rbp + 500]
;       [269:16] free scratch register r15
;   [270:5] arr[ix] = tmp
;   [270:9] allocate scratch register -> r15
;   [270:9] set array index
;   [270:9] ix
    mov r15, qword [rbp + 440]
;   [270:9] bounds check begin
;   [270:9] lower bound
;   [270:9] r15 lower bound covered by the unsigned upper bound
;   [270:9] upper bound
    cmp r15, 4
    jae baz_bounds_line_270
;   [270:9] bounds check end
;   [270:15] tmp
;   [270:15] allocate scratch register -> r14
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
;   [270:15] free scratch register r14
;   [270:5] free scratch register r15
;   [271:5] assert(arr[ix] == 2)
;   [271:12] allocate scratch register -> r15
;   [271:12] ? arr[ix] == 2
;   [271:12] ? arr[ix] == 2
    cmp.271.12:
;   [271:16] allocate scratch register -> r14
;   [271:16] set array index
;   [271:16] ix
    mov r14, qword [rbp + 440]
;   [271:16] bounds check begin
;   [271:16] lower bound
;   [271:16] r14 lower bound covered by the unsigned upper bound
;   [271:16] upper bound
    cmp r14, 4
    jae baz_bounds_line_271
;   [271:16] bounds check end
    cmp dword [rbp + r14 * 4 + 424], 2
;   [271:12] free scratch register r14
    sete r15b
    bool.271.12.end:
;   [38:6] assert(ok bool)
    func.assert.271.5:
;       [271:5] alias ok -> r15b
        if.38.27.271.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.271.5:
        cmp r15b, 0
        jne if.38.24.271.5.end
        if.38.27.271.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.271.5.end:
;       [271:5] free scratch register r15
    func.assert.271.5.end:
;   [273:5] faz(arr)
;   [78:6] faz(arg mut i32[])
    func.faz.273.5:
;       [273:5] alias arg -> arr
;       [79:5] arg[1] = 0xfe
;       [79:14] 0xfe
        mov dword [rbp + 428], 254
    func.faz.273.5.end:
;   [274:5] assert(arr[1] == 0xfe)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? arr[1] == 0xfe
;   [274:12] ? arr[1] == 0xfe
    cmp.274.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.274.12.end:
;   [38:6] assert(ok bool)
    func.assert.274.5:
;       [274:5] alias ok -> r15b
        if.38.27.274.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.274.5:
        cmp r15b, 0
        jne if.38.24.274.5.end
        if.38.27.274.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.274.5.end:
;       [274:5] free scratch register r15
    func.assert.274.5.end:
;   [276:5] var arr3 = []{ 3, 5 }
;   [276:9] arr3: i64[2] (16 B @ [rbp + 504])
;   [276:9] arr3 = []{ 3, 5 }
;   [276:18] size <= 16 B, use immediates
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
;   [277:5] foo arr3
;   [277:9] allocate scratch register -> r15
;   [277:9] initiate iterator e
    lea r15, [rbp + 504]
;   [277:5] allocate scratch register -> r14
;   [277:9] e: i64 (r15)
;   [277:9] i: i64 (r14)
;   [277:9] const n = 2
;   [277:5] initiate counter i
    mov r14, 0
    foo.277.5:
;       [278:9] e = e + i + n
;       [278:13] instructions without scratch register 2, with 4
;       [278:13] e
;       [278:17] e + i
;       [278:17] src: operand
        add qword [r15], r14
;       [278:13] e + 2
;       [278:13] src: folded constant '+ n'
        add qword [r15], 2
        foo.277.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.277.5
    foo.277.5.end:
;   [277:5] free scratch register r14
;   [277:5] free scratch register r15
;   [280:5] assert(arr3[0] == 3 + 0 + 2)
;   [280:12] allocate scratch register -> r15
;   [280:12] ? arr3[0] == 3 + 0 + 2
;   [280:12] ? arr3[0] == 3 + 0 + 2
    cmp.280.12:
;   [280:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 504], 5
    sete r15b
    bool.280.12.end:
;   [38:6] assert(ok bool)
    func.assert.280.5:
;       [280:5] alias ok -> r15b
        if.38.27.280.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.280.5:
        cmp r15b, 0
        jne if.38.24.280.5.end
        if.38.27.280.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.280.5.end:
;       [280:5] free scratch register r15
    func.assert.280.5.end:
;   [281:5] assert(arr3[1] == 5 + 1 + 2)
;   [281:12] allocate scratch register -> r15
;   [281:12] ? arr3[1] == 5 + 1 + 2
;   [281:12] ? arr3[1] == 5 + 1 + 2
    cmp.281.12:
;   [281:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 512], 8
    sete r15b
    bool.281.12.end:
;   [38:6] assert(ok bool)
    func.assert.281.5:
;       [281:5] alias ok -> r15b
        if.38.27.281.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.281.5:
        cmp r15b, 0
        jne if.38.24.281.5.end
        if.38.27.281.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.281.5.end:
;       [281:5] free scratch register r15
    func.assert.281.5.end:
;   [287:5] var p = point
;   [287:9] p: point (16 B @ [rbp + 520])
;   [287:9] p = point
;   [287:13] zero remaining fields: 16 B
;   [287:13] size <= 32 B, use mov
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
;   [289:7] p.fooz()
;   [47:10] mut point.fooz()
    func.point.fooz.289.7:
;       [289:7] alias self -> p
;       [48:5] self.x = 0b10
;       [48:14] 0b10
        mov qword [rbp + 520], 2
;       [49:5] self.y = 0xb
;       [49:14] 0xb
        mov qword [rbp + 528], 11
    func.point.fooz.289.7.end:
;   [292:5] assert(p.x == 2)
;   [292:12] allocate scratch register -> r15
;   [292:12] ? p.x == 2
;   [292:12] ? p.x == 2
    cmp.292.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.292.12.end:
;   [38:6] assert(ok bool)
    func.assert.292.5:
;       [292:5] alias ok -> r15b
        if.38.27.292.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.292.5:
        cmp r15b, 0
        jne if.38.24.292.5.end
        if.38.27.292.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.292.5.end:
;       [292:5] free scratch register r15
    func.assert.292.5.end:
;   [293:5] assert(p.y == 0xb)
;   [293:12] allocate scratch register -> r15
;   [293:12] ? p.y == 0xb
;   [293:12] ? p.y == 0xb
    cmp.293.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.293.12.end:
;   [38:6] assert(ok bool)
    func.assert.293.5:
;       [293:5] alias ok -> r15b
        if.38.27.293.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.293.5:
        cmp r15b, 0
        jne if.38.24.293.5.end
        if.38.27.293.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.293.5.end:
;       [293:5] free scratch register r15
    func.assert.293.5.end:
;   [295:5] var q = p
;   [295:9] q: point (16 B @ [rbp + 536])
;   [295:9] q = p
;   [295:13] size <= 16 B, use mov
;   [295:13] allocate named register rax
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
;   [295:13] free named register rax
;   [298:5] assert(p == q)
;   [298:12] allocate scratch register -> r15
;   [298:12] ? p == q
;   [298:12] ? p == q
    cmp.298.12:
;       [298:12] allocate named register rsi
;       [298:12] allocate named register rdi
;       [298:12] allocate named register rcx
;       [298:12] p
        lea rsi, [rbp + 520]
;       [298:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
;       [298:12] free named register rcx
;       [298:12] free named register rdi
;       [298:12] free named register rsi
        sete r15b
    cmp r15b, 0
    bool.298.12.end:
;   [38:6] assert(ok bool)
    func.assert.298.5:
;       [298:5] alias ok -> r15b
        if.38.27.298.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.298.5:
        cmp r15b, 0
        jne if.38.24.298.5.end
        if.38.27.298.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.298.5.end:
;       [298:5] free scratch register r15
    func.assert.298.5.end:
;   [302:5] q.x = 3
;   [302:11] 3
    mov qword [rbp + 536], 3
;   [303:5] assert(p != q)
;   [303:12] allocate scratch register -> r15
;   [303:12] ? p != q
;   [303:12] ? p != q
    cmp.303.12:
;       [303:12] allocate named register rsi
;       [303:12] allocate named register rdi
;       [303:12] allocate named register rcx
;       [303:12] p
        lea rsi, [rbp + 520]
;       [303:17] q
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
;       [303:12] free named register rcx
;       [303:12] free named register rdi
;       [303:12] free named register rsi
        setne r15b
    cmp r15b, 0
    bool.303.12.end:
;   [38:6] assert(ok bool)
    func.assert.303.5:
;       [303:5] alias ok -> r15b
        if.38.27.303.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.303.5:
        cmp r15b, 0
        jne if.38.24.303.5.end
        if.38.27.303.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.303.5.end:
;       [303:5] free scratch register r15
    func.assert.303.5.end:
;   [305:5] var i = 0
;   [305:9] i: i64 (8 B @ [rbp + 552])
;   [305:9] i = 0
;   [305:13] 0
    mov qword [rbp + 552], 0
;   [306:5] bar(i)
;   [59:6] bar(arg mut)
    func.bar.306.5:
;       [306:5] alias arg -> i
        if.60.8.306.5:
;       [60:8] ? arg == 0
;       [60:8] ? arg == 0
        cmp.60.8.306.5:
        cmp qword [rbp + 552], 0
        je func.bar.306.5.end
        if.60.8.306.5.code:
;           [60:17] return
        if.60.5.306.5.end:
;       [61:5] arg = 0xff
;       [61:11] 0xff
        mov qword [rbp + 552], 255
    func.bar.306.5.end:
;   [307:5] assert(i == 0)
;   [307:12] allocate scratch register -> r15
;   [307:12] ? i == 0
;   [307:12] ? i == 0
    cmp.307.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.307.12.end:
;   [38:6] assert(ok bool)
    func.assert.307.5:
;       [307:5] alias ok -> r15b
        if.38.27.307.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.307.5:
        cmp r15b, 0
        jne if.38.24.307.5.end
        if.38.27.307.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.307.5.end:
;       [307:5] free scratch register r15
    func.assert.307.5.end:
;   [309:5] i = 1
;   [309:9] 1
    mov qword [rbp + 552], 1
;   [310:5] bar(i)
;   [59:6] bar(arg mut)
    func.bar.310.5:
;       [310:5] alias arg -> i
        if.60.8.310.5:
;       [60:8] ? arg == 0
;       [60:8] ? arg == 0
        cmp.60.8.310.5:
        cmp qword [rbp + 552], 0
        je func.bar.310.5.end
        if.60.8.310.5.code:
;           [60:17] return
        if.60.5.310.5.end:
;       [61:5] arg = 0xff
;       [61:11] 0xff
        mov qword [rbp + 552], 255
    func.bar.310.5.end:
;   [311:5] assert(i == 0xff)
;   [311:12] allocate scratch register -> r15
;   [311:12] ? i == 0xff
;   [311:12] ? i == 0xff
    cmp.311.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.311.12.end:
;   [38:6] assert(ok bool)
    func.assert.311.5:
;       [311:5] alias ok -> r15b
        if.38.27.311.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.311.5:
        cmp r15b, 0
        jne if.38.24.311.5.end
        if.38.27.311.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.311.5.end:
;       [311:5] free scratch register r15
    func.assert.311.5.end:
;   [313:5] var j = 1
;   [313:9] j: i64 (8 B @ [rbp + 560])
;   [313:9] j = 1
;   [313:13] 1
    mov qword [rbp + 560], 1
;   [314:5] var k = baz(j)
;   [314:9] k: i64 (8 B @ [rbp + 568])
;   [314:9] k = baz(j)
;   [314:13] k = baz(j)
;   [314:13] = expression
;   [314:13] baz(j)
;   [66:6] baz(arg) res
    func.baz.314.13:
;       [314:13] alias res -> k
;       [314:13] alias arg -> j
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 3, with 3
;       [67:11] arg
;       [67:11] allocate scratch register -> r15
        mov r15, qword [rbp + 560]
        mov qword [rbp + 568], r15
;       [67:11] free scratch register r15
;       [67:11] res * 2
;       [67:11] src: folded constant '* 2'
        sal qword [rbp + 568], 1
    func.baz.314.13.end:
;   [315:5] assert(k == 2)
;   [315:12] allocate scratch register -> r15
;   [315:12] ? k == 2
;   [315:12] ? k == 2
    cmp.315.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.315.12.end:
;   [38:6] assert(ok bool)
    func.assert.315.5:
;       [315:5] alias ok -> r15b
        if.38.27.315.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.315.5:
        cmp r15b, 0
        jne if.38.24.315.5.end
        if.38.27.315.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.315.5.end:
;       [315:5] free scratch register r15
    func.assert.315.5.end:
;   [317:5] k = baz(1)
;   [317:9] k = baz(1)
;   [317:9] = expression
;   [317:9] baz(1)
;   [66:6] baz(arg) res
    func.baz.317.9:
;       [317:9] alias res -> k
;       [317:9] alias arg -> 1
;       [67:5] res = arg * 2
;       [67:11] res = 2
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 568], 2
    func.baz.317.9.end:
;   [318:5] assert(k == 2)
;   [318:12] allocate scratch register -> r15
;   [318:12] ? k == 2
;   [318:12] ? k == 2
    cmp.318.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.318.12.end:
;   [38:6] assert(ok bool)
    func.assert.318.5:
;       [318:5] alias ok -> r15b
        if.38.27.318.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.318.5:
        cmp r15b, 0
        jne if.38.24.318.5.end
        if.38.27.318.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.318.5.end:
;       [318:5] free scratch register r15
    func.assert.318.5.end:
;   [320:5] var five = 5
;   [320:9] five: i64 (8 B @ [rbp + 576])
;   [320:9] five = 5
;   [320:16] 5
    mov qword [rbp + 576], 5
;   [321:5] var f = factorial(five)
;   [321:9] f: i64 (8 B @ [rbp + 584])
;   [321:9] f = factorial(five)
;   [321:13] f = factorial(five)
;   [321:13] = expression
;   [321:13] factorial(five)
;   [321:13] frame capacity check begin
;   [321:13] allocate scratch register -> r15
;   [321:13] allocate scratch register -> r14
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
;   [321:13] free scratch register r14
;   [321:13] free scratch register r15
;   [321:13] frame capacity check end
;   [321:13] result address in callee frame
;   [321:13] allocate scratch register -> r15
    lea r15, [rbp + 584]
    mov qword [rbp + 592], r15
;   [321:13] free scratch register r15
;   [321:13] address of argument 'five' to parameter 'n'
;   [321:13] allocate scratch register -> r15
    lea r15, [rbp + 576]
    mov qword [rbp + 600], r15
;   [321:13] free scratch register r15
;   [321:13] set function frame base
    lea rbx, [rbp + 592]
    call func.factorial
;   [322:5] assert(f == 120)
;   [322:12] allocate scratch register -> r15
;   [322:12] ? f == 120
;   [322:12] ? f == 120
    cmp.322.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.322.12.end:
;   [38:6] assert(ok bool)
    func.assert.322.5:
;       [322:5] alias ok -> r15b
        if.38.27.322.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.322.5:
        cmp r15b, 0
        jne if.38.24.322.5.end
        if.38.27.322.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.322.5.end:
;       [322:5] free scratch register r15
    func.assert.322.5.end:
;   [324:5] var p0 = point{baz(3), 0}
;   [324:9] p0: point (16 B @ [rbp + 592])
;   [324:9] p0 = point{baz(3), 0}
;   [324:20] copy field 'x'
;   [324:20] p0.x = baz(3)
;   [324:20] = expression
;   [324:20] baz(3)
;   [66:6] baz(arg) res
    func.baz.324.20:
;       [324:20] alias res -> p0.x (lea: rbp + 592)
;       [324:20] alias arg -> 3
;       [67:5] res = arg * 2
;       [67:11] res = 6
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 592], 6
    func.baz.324.20.end:
;   [324:28] copy field 'y'
    mov qword [rbp + 600], 0
;   [325:5] assert(p0.x == 6)
;   [325:12] allocate scratch register -> r15
;   [325:12] ? p0.x == 6
;   [325:12] ? p0.x == 6
    cmp.325.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.325.12.end:
;   [38:6] assert(ok bool)
    func.assert.325.5:
;       [325:5] alias ok -> r15b
        if.38.27.325.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.325.5:
        cmp r15b, 0
        jne if.38.24.325.5.end
        if.38.27.325.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.325.5.end:
;       [325:5] free scratch register r15
    func.assert.325.5.end:
;   [327:5] var pt = point.at(-1, -2)
;   [327:9] pt: point (16 B @ [rbp + 608])
;   [327:9] pt = point.at(-1, -2)
;   [327:14] point.at(-1, -2)
;   [108:6] point.at(x, y) self
    func.point.at.327.14:
;       [327:14] alias self -> pt
;       [327:14] alias x -> -1
;       [327:14] alias y -> -2
;       [109:5] self.x = x
;       [109:14] x
        mov qword [rbp + 608], -1
;       [110:5] self.y = y
;       [110:14] y
        mov qword [rbp + 616], -2
    func.point.at.327.14.end:
;   [331:5] assert(pt.x == -1)
;   [331:12] allocate scratch register -> r15
;   [331:12] ? pt.x == -1
;   [331:12] ? pt.x == -1
    cmp.331.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.331.12.end:
;   [38:6] assert(ok bool)
    func.assert.331.5:
;       [331:5] alias ok -> r15b
        if.38.27.331.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.331.5:
        cmp r15b, 0
        jne if.38.24.331.5.end
        if.38.27.331.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.331.5.end:
;       [331:5] free scratch register r15
    func.assert.331.5.end:
;   [332:5] assert(pt.y == -2)
;   [332:12] allocate scratch register -> r15
;   [332:12] ? pt.y == -2
;   [332:12] ? pt.y == -2
    cmp.332.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.332.12.end:
;   [38:6] assert(ok bool)
    func.assert.332.5:
;       [332:5] alias ok -> r15b
        if.38.27.332.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.332.5:
        cmp r15b, 0
        jne if.38.24.332.5.end
        if.38.27.332.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.332.5.end:
;       [332:5] free scratch register r15
    func.assert.332.5.end:
;   [334:8] pt.x(2)
;   [114:10] mut point.x(x)
    func.point.x.334.8:
;       [334:8] alias self -> pt
;       [334:8] alias x -> 2
;       [115:5] self.x = x
;       [115:14] x
        mov qword [rbp + 608], 2
    func.point.x.334.8.end:
;   [335:5] assert(pt.x == 2)
;   [335:12] allocate scratch register -> r15
;   [335:12] ? pt.x == 2
;   [335:12] ? pt.x == 2
    cmp.335.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.335.12.end:
;   [38:6] assert(ok bool)
    func.assert.335.5:
;       [335:5] alias ok -> r15b
        if.38.27.335.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.335.5:
        cmp r15b, 0
        jne if.38.24.335.5.end
        if.38.27.335.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.335.5.end:
;       [335:5] free scratch register r15
    func.assert.335.5.end:
;   [336:5] assert(pt.sum() == 0)
;   [336:12] allocate scratch register -> r15
;   [336:12] ? pt.sum() == 0
;   [336:12] ? pt.sum() == 0
    cmp.336.12:
;   [336:12] allocate scratch register -> r14
;       [336:15] r14 = pt.sum()
;       [336:15] = expression
;       [336:15] pt.sum()
;       [54:6] point.sum() res
        func.point.sum.336.15:
;           [336:15] alias res -> r14
;           [336:15] alias self -> pt
;           [55:5] res = self.x + self.y
;           [55:11] self.x
            mov r14, qword [rbp + 608]
;           [55:20] res + self.y
;           [55:20] src: operand
            add r14, qword [rbp + 616]
        func.point.sum.336.15.end:
    cmp r14, 0
;   [336:12] free scratch register r14
    sete r15b
    bool.336.12.end:
;   [38:6] assert(ok bool)
    func.assert.336.5:
;       [336:5] alias ok -> r15b
        if.38.27.336.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.336.5:
        cmp r15b, 0
        jne if.38.24.336.5.end
        if.38.27.336.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.336.5.end:
;       [336:5] free scratch register r15
    func.assert.336.5.end:
;   [338:5] var x = 1
;   [338:9] x: i64 (8 B @ [rbp + 624])
;   [338:9] x = 1
;   [338:13] 1
    mov qword [rbp + 624], 1
;   [339:5] var y = 2
;   [339:9] y: i64 (8 B @ [rbp + 632])
;   [339:9] y = 2
;   [339:13] 2
    mov qword [rbp + 632], 2
;   [341:5] var o1 = object{{x * 10, y}, 0xff0000}
;   [341:9] o1: object (24 B @ [rbp + 640])
;   [341:9] o1 = object{{x * 10, y}, 0xff0000}
;   [341:21] copy field 'pos'
;   [341:22] copy field 'x'
;   [341:22] instructions without scratch register 5, with 3
;   [341:22] allocate scratch register -> r15
;   [341:22] x
    mov r15, qword [rbp + 624]
;   [341:22] r15 * 10
;   [341:22] src: folded constant '* 10'
    imul r15, 10
    mov qword [rbp + 640], r15
;   [341:22] free scratch register r15
;   [341:30] copy field 'y'
;   [341:30] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
;   [341:30] free scratch register r15
;   [341:34] copy field 'color'
    mov dword [rbp + 656], 16711680
;   [341:14] zero padding: 4 B
;   [341:14] size <= 32 B, use mov
    mov dword [rbp + 660], 0
;   [342:5] assert(o1.pos.x == 10)
;   [342:12] allocate scratch register -> r15
;   [342:12] ? o1.pos.x == 10
;   [342:12] ? o1.pos.x == 10
    cmp.342.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.342.12.end:
;   [38:6] assert(ok bool)
    func.assert.342.5:
;       [342:5] alias ok -> r15b
        if.38.27.342.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.342.5:
        cmp r15b, 0
        jne if.38.24.342.5.end
        if.38.27.342.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.342.5.end:
;       [342:5] free scratch register r15
    func.assert.342.5.end:
;   [343:5] assert(o1.pos.y == 2)
;   [343:12] allocate scratch register -> r15
;   [343:12] ? o1.pos.y == 2
;   [343:12] ? o1.pos.y == 2
    cmp.343.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.343.12.end:
;   [38:6] assert(ok bool)
    func.assert.343.5:
;       [343:5] alias ok -> r15b
        if.38.27.343.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.343.5:
        cmp r15b, 0
        jne if.38.24.343.5.end
        if.38.27.343.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.343.5.end:
;       [343:5] free scratch register r15
    func.assert.343.5.end:
;   [344:5] assert(o1.color == 0xff0000)
;   [344:12] allocate scratch register -> r15
;   [344:12] ? o1.color == 0xff0000
;   [344:12] ? o1.color == 0xff0000
    cmp.344.12:
    cmp dword [rbp + 656], 16711680
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
;   [346:5] var p1 = point{-x, -y}
;   [346:9] p1: point (16 B @ [rbp + 664])
;   [346:9] p1 = point{-x, -y}
;   [346:20] copy field 'x'
;   [346:20] instructions without scratch register 3, with 3
;   [346:20] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
;   [346:20] free scratch register r15
    neg qword [rbp + 664]
;   [346:24] copy field 'y'
;   [346:24] instructions without scratch register 3, with 3
;   [346:24] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
;   [346:24] free scratch register r15
    neg qword [rbp + 672]
;   [347:5] o1.pos = p1
;   [347:14] size <= 16 B, use mov
;   [347:14] allocate named register rax
    mov rax, qword [rbp + 664]
    mov qword [rbp + 640], rax
    mov rax, qword [rbp + 672]
    mov qword [rbp + 648], rax
;   [347:14] free named register rax
;   [348:5] assert(o1.pos.x == -1)
;   [348:12] allocate scratch register -> r15
;   [348:12] ? o1.pos.x == -1
;   [348:12] ? o1.pos.x == -1
    cmp.348.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.348.12.end:
;   [38:6] assert(ok bool)
    func.assert.348.5:
;       [348:5] alias ok -> r15b
        if.38.27.348.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.348.5:
        cmp r15b, 0
        jne if.38.24.348.5.end
        if.38.27.348.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.348.5.end:
;       [348:5] free scratch register r15
    func.assert.348.5.end:
;   [349:5] assert(o1.pos.y == -2)
;   [349:12] allocate scratch register -> r15
;   [349:12] ? o1.pos.y == -2
;   [349:12] ? o1.pos.y == -2
    cmp.349.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.349.12.end:
;   [38:6] assert(ok bool)
    func.assert.349.5:
;       [349:5] alias ok -> r15b
        if.38.27.349.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.349.5:
        cmp r15b, 0
        jne if.38.24.349.5.end
        if.38.27.349.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.349.5.end:
;       [349:5] free scratch register r15
    func.assert.349.5.end:
;   [351:5] var o2 = o1
;   [351:9] o2: object (24 B @ [rbp + 680])
;   [351:9] o2 = o1
;   [351:14] allocate named register rsi
;   [351:14] allocate named register rdi
;   [351:14] allocate named register rcx
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
;   [351:14] free named register rcx
;   [351:14] free named register rdi
;   [351:14] free named register rsi
;   [352:5] assert(o2.pos.x == -1)
;   [352:12] allocate scratch register -> r15
;   [352:12] ? o2.pos.x == -1
;   [352:12] ? o2.pos.x == -1
    cmp.352.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.352.12.end:
;   [38:6] assert(ok bool)
    func.assert.352.5:
;       [352:5] alias ok -> r15b
        if.38.27.352.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.352.5:
        cmp r15b, 0
        jne if.38.24.352.5.end
        if.38.27.352.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.352.5.end:
;       [352:5] free scratch register r15
    func.assert.352.5.end:
;   [353:5] assert(o2.pos.y == -2)
;   [353:12] allocate scratch register -> r15
;   [353:12] ? o2.pos.y == -2
;   [353:12] ? o2.pos.y == -2
    cmp.353.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.353.12.end:
;   [38:6] assert(ok bool)
    func.assert.353.5:
;       [353:5] alias ok -> r15b
        if.38.27.353.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.353.5:
        cmp r15b, 0
        jne if.38.24.353.5.end
        if.38.27.353.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.353.5.end:
;       [353:5] free scratch register r15
    func.assert.353.5.end:
;   [354:5] assert(o2.color == 0xff0000)
;   [354:12] allocate scratch register -> r15
;   [354:12] ? o2.color == 0xff0000
;   [354:12] ? o2.color == 0xff0000
    cmp.354.12:
    cmp dword [rbp + 696], 16711680
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
;   [356:5] o2.pos = {x, y}
;   [356:15] copy field 'x'
;   [356:15] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
;   [356:15] free scratch register r15
;   [356:18] copy field 'y'
;   [356:18] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
;   [356:18] free scratch register r15
;   [357:5] assert(o2.pos.x == 1)
;   [357:12] allocate scratch register -> r15
;   [357:12] ? o2.pos.x == 1
;   [357:12] ? o2.pos.x == 1
    cmp.357.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.357.12.end:
;   [38:6] assert(ok bool)
    func.assert.357.5:
;       [357:5] alias ok -> r15b
        if.38.27.357.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.357.5:
        cmp r15b, 0
        jne if.38.24.357.5.end
        if.38.27.357.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.357.5.end:
;       [357:5] free scratch register r15
    func.assert.357.5.end:
;   [358:5] o2.pos = point{y, x}
;   [358:20] copy field 'x'
;   [358:20] allocate scratch register -> r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
;   [358:20] free scratch register r15
;   [358:23] copy field 'y'
;   [358:23] allocate scratch register -> r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
;   [358:23] free scratch register r15
;   [359:5] assert(o2.pos.x == 2)
;   [359:12] allocate scratch register -> r15
;   [359:12] ? o2.pos.x == 2
;   [359:12] ? o2.pos.x == 2
    cmp.359.12:
    cmp qword [rbp + 680], 2
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
;   [366:5] var o3 = object[2]
;   [366:9] o3: object[2] (48 B @ [rbp + 704])
;   [366:9] o3 = object[2]
;   [366:14] zero remaining elements: 2 * 24 B = 48 B
;   [366:14] allocate named register rax
;   [366:14] allocate named register rdi
;   [366:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
;   [366:14] free named register rcx
;   [366:14] free named register rdi
;   [366:14] free named register rax
;   [367:5] o3[0].pos.y = 73
;   [367:19] 73
    mov qword [rbp + 712], 73
;   [369:5] assert(o3[0].pos.y == 73)
;   [369:12] allocate scratch register -> r15
;   [369:12] ? o3[0].pos.y == 73
;   [369:12] ? o3[0].pos.y == 73
    cmp.369.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.369.12.end:
;   [38:6] assert(ok bool)
    func.assert.369.5:
;       [369:5] alias ok -> r15b
        if.38.27.369.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.369.5:
        cmp r15b, 0
        jne if.38.24.369.5.end
        if.38.27.369.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.369.5.end:
;       [369:5] free scratch register r15
    func.assert.369.5.end:
;   [370:5] o3[1] = object.at(2, 74, 0xffffff)
;   [370:13] zero padding: 4 B
;   [370:13] size <= 32 B, use mov
    mov dword [rbp + 748], 0
;   [370:13] object.at(2, 74, 0xffffff)
;   [119:6] object.at(x, y, color i32) self
    func.object.at.370.13:
;       [370:13] alias self -> o3 (lea: rbp + 728)
;       [370:13] alias x -> 2
;       [370:13] alias y -> 74
;       [370:13] alias color -> 16777215
;       [120:5] self.pos = point.at(x, y)
;       [120:16] point.at(x, y)
;       [108:6] point.at(x, y) self
        func.point.at.120.16.370.13:
;           [120:16] alias self -> self.pos (lea: rbp + 728)
;           [120:16] alias x -> 2
;           [120:16] alias y -> 74
;           [109:5] self.x = x
;           [109:14] x
            mov qword [rbp + 728], 2
;           [110:5] self.y = y
;           [110:14] y
            mov qword [rbp + 736], 74
        func.point.at.120.16.370.13.end:
;       [121:5] self.color = color
;       [121:18] color
        mov dword [rbp + 744], 16777215
    func.object.at.370.13.end:
;   [371:5] assert(o3[1].pos.y == 74)
;   [371:12] allocate scratch register -> r15
;   [371:12] ? o3[1].pos.y == 74
;   [371:12] ? o3[1].pos.y == 74
    cmp.371.12:
    cmp qword [rbp + 736], 74
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
;   [373:15] o3[1].pos.fooz()
;   [47:10] mut point.fooz()
    func.point.fooz.373.15:
;       [373:15] alias self -> o3.pos (lea: rbp + 728)
;       [48:5] self.x = 0b10
;       [48:14] 0b10
        mov qword [rbp + 728], 2
;       [49:5] self.y = 0xb
;       [49:14] 0xb
        mov qword [rbp + 736], 11
    func.point.fooz.373.15.end:
;   [374:5] assert(o3[1].pos.sum() == 13)
;   [374:12] allocate scratch register -> r15
;   [374:12] ? o3[1].pos.sum() == 13
;   [374:12] ? o3[1].pos.sum() == 13
    cmp.374.12:
;   [374:12] allocate scratch register -> r14
;       [374:22] r14 = o3[1].pos.sum()
;       [374:22] = expression
;       [374:22] o3[1].pos.sum()
;       [54:6] point.sum() res
        func.point.sum.374.22:
;           [374:22] alias res -> r14
;           [374:22] alias self -> o3.pos (lea: rbp + 728)
;           [55:5] res = self.x + self.y
;           [55:11] self.x
            mov r14, qword [rbp + 728]
;           [55:20] res + self.y
;           [55:20] src: operand
            add r14, qword [rbp + 736]
        func.point.sum.374.22.end:
    cmp r14, 13
;   [374:12] free scratch register r14
    sete r15b
    bool.374.12.end:
;   [38:6] assert(ok bool)
    func.assert.374.5:
;       [374:5] alias ok -> r15b
        if.38.27.374.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.374.5:
        cmp r15b, 0
        jne if.38.24.374.5.end
        if.38.27.374.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.374.5.end:
;       [374:5] free scratch register r15
    func.assert.374.5.end:
;   [377:5] var worlds = world[8]
;   [377:9] worlds: world[8] (512 B @ [rbp + 752])
;   [377:9] worlds = world[8]
;   [377:18] zero remaining elements: 8 * 64 B = 512 B
;   [377:18] allocate named register rax
;   [377:18] allocate named register rdi
;   [377:18] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
;   [377:18] free named register rcx
;   [377:18] free named register rdi
;   [377:18] free named register rax
;   [378:5] worlds[1].locations[1] = 0xffee
;   [378:30] 0xffee
    mov qword [rbp + 824], 65518
;   [379:5] assert(worlds[1].locations[1] == 0xffee)
;   [379:12] allocate scratch register -> r15
;   [379:12] ? worlds[1].locations[1] == 0xffee
;   [379:12] ? worlds[1].locations[1] == 0xffee
    cmp.379.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.379.12.end:
;   [38:6] assert(ok bool)
    func.assert.379.5:
;       [379:5] alias ok -> r15b
        if.38.27.379.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.379.5:
        cmp r15b, 0
        jne if.38.24.379.5.end
        if.38.27.379.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.379.5.end:
;       [379:5] free scratch register r15
    func.assert.379.5.end:
;   [381:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [381:5] allocate scratch register -> r15
;   [384:9] array_length(worlds[0].locations)
;   [384:9] r15 = 8
;   [384:9] src: folded constant 'array_length(worlds[0].locations)'
    mov r15, 8
;   [382:9] worlds[1].locations
;   [382:9] bounds check begin
;   [382:9] lower bound
;   [382:9] r15 lower bound covered by the unsigned upper bound
;   [382:9] upper bound
    cmp r15, 8
    ja baz_bounds_line_382
;   [382:9] bounds check end
;   [383:9] worlds[0].locations
;   [383:9] bounds check begin
;   [383:9] lower bound
;   [383:9] r15 lower bound covered by the unsigned upper bound
;   [383:9] upper bound
    cmp r15, 8
    ja baz_bounds_line_383
;   [383:9] bounds check end
;   [381:5] allocate named register rsi
;   [381:5] allocate named register rdi
;   [381:5] allocate named register rcx
    lea rsi, [rbp + 816]
    lea rdi, [rbp + 752]
    mov rcx, 64
    rep movsb
;   [381:5] free named register rcx
;   [381:5] free named register rdi
;   [381:5] free named register rsi
;   [381:5] free scratch register r15
;   [388:5] assert(worlds[0].locations[1] == 0xffee)
;   [388:12] allocate scratch register -> r15
;   [388:12] ? worlds[0].locations[1] == 0xffee
;   [388:12] ? worlds[0].locations[1] == 0xffee
    cmp.388.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.388.12.end:
;   [38:6] assert(ok bool)
    func.assert.388.5:
;       [388:5] alias ok -> r15b
        if.38.27.388.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.388.5:
        cmp r15b, 0
        jne if.38.24.388.5.end
        if.38.27.388.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.388.5.end:
;       [388:5] free scratch register r15
    func.assert.388.5.end:
;   [389:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [389:12] allocate scratch register -> r15
;   [389:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [389:12] ? shorthand: arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
    cmp.389.12:
;       [389:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;       [389:12] allocate named register rsi
;       [389:12] allocate named register rdi
;       [389:12] allocate named register rcx
;       [392:14] array_length(worlds[0].locations)
;       [392:14] rcx = 8
;       [392:14] src: folded constant 'array_length(worlds[0].locations)'
        mov rcx, 8
;       [390:14] worlds[0].locations
;       [390:14] bounds check begin
;       [390:14] lower bound
;       [390:14] rcx lower bound covered by the unsigned upper bound
;       [390:14] upper bound
        cmp rcx, 8
        ja baz_bounds_line_390
;       [390:14] bounds check end
        lea rsi, [rbp + 752]
;       [391:14] worlds[1].locations
;       [391:14] bounds check begin
;       [391:14] lower bound
;       [391:14] rcx lower bound covered by the unsigned upper bound
;       [391:14] upper bound
        cmp rcx, 8
        ja baz_bounds_line_391
;       [391:14] bounds check end
        lea rdi, [rbp + 816]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [389:12] free named register rcx
;       [389:12] free named register rdi
;       [389:12] free named register rsi
        sete r15b
    bool.389.12.end:
;   [38:6] assert(ok bool)
    func.assert.389.5:
;       [389:5] alias ok -> r15b
        if.38.27.389.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.389.5:
        cmp r15b, 0
        jne if.38.24.389.5.end
        if.38.27.389.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.389.5.end:
;       [389:5] free scratch register r15
    func.assert.389.5.end:
;   [395:5] var arr2 = []{ -1, 2 }
;   [395:9] arr2: i64[2] (16 B @ [rbp + 1264])
;   [395:9] arr2 = []{ -1, 2 }
;   [395:18] size <= 16 B, use immediates
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
;   [396:5] assert(array_length(arr2) == 2)
;   [38:6] assert(ok bool)
    func.assert.396.5:
;       [396:5] alias ok -> 1
        if.38.27.396.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.396.5:
;       [38:31] const eval to false
        if.38.24.396.5.end:
    func.assert.396.5.end:
;   [397:5] assert(arr2[0] == -1)
;   [397:12] allocate scratch register -> r15
;   [397:12] ? arr2[0] == -1
;   [397:12] ? arr2[0] == -1
    cmp.397.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.397.12.end:
;   [38:6] assert(ok bool)
    func.assert.397.5:
;       [397:5] alias ok -> r15b
        if.38.27.397.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.397.5:
        cmp r15b, 0
        jne if.38.24.397.5.end
        if.38.27.397.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.397.5.end:
;       [397:5] free scratch register r15
    func.assert.397.5.end:
;   [398:5] assert(arr2[1] == 2)
;   [398:12] allocate scratch register -> r15
;   [398:12] ? arr2[1] == 2
;   [398:12] ? arr2[1] == 2
    cmp.398.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.398.12.end:
;   [38:6] assert(ok bool)
    func.assert.398.5:
;       [398:5] alias ok -> r15b
        if.38.27.398.5:
;       [38:27] ? not ok
;       [38:27] ? shorthand: not ok
        cmp.38.27.398.5:
        cmp r15b, 0
        jne if.38.24.398.5.end
        if.38.27.398.5.code:
;           [38:34] exit(1)
;           [38:34] allocate named register rdi
;           [38:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [38:34] free named register rdi
        if.38.24.398.5.end:
;       [398:5] free scratch register r15
    func.assert.398.5.end:
;   [400:5] var counter = 0
;   [400:9] counter: i64 (8 B @ [rbp + 1280])
;   [400:9] counter = 0
;   [400:19] 0
    mov qword [rbp + 1280], 0
;   [401:5] var nm = str
;   [401:9] nm: str (128 B @ [rbp + 1288])
;   [401:9] nm = str
;   [401:14] zero remaining fields: 128 B
;   [401:14] allocate named register rax
;   [401:14] allocate named register rdi
;   [401:14] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
;   [401:14] free named register rcx
;   [401:14] free named register rdi
;   [401:14] free named register rax
;   [402:5] print(hello)
;   [41:6] print(str i8[])
    func.print.402.5:
;       [402:5] alias str -> hello
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
    func.print.402.5.end:
;   [403:5] label
    loop.403.5:
;       [404:9] counter = counter + 1
;       [404:19] instructions without scratch register 1, with 3
;       [404:19] counter
;       [404:19] counter + 1
;       [404:19] src: folded constant '+ 1'
        add qword [rbp + 1280], 1
;       [405:9] print_num(counter)
;       [405:9] frame capacity check begin
;       [405:9] allocate scratch register -> r15
;       [405:9] allocate scratch register -> r14
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
;       [405:9] free scratch register r14
;       [405:9] free scratch register r15
;       [405:9] frame capacity check end
;       [405:9] address of argument 'counter' to parameter 'num'
;       [405:9] allocate scratch register -> r15
        lea r15, [rbp + 1280]
        mov qword [rbp + 1416], r15
;       [405:9] free scratch register r15
;       [405:9] set function frame base
        lea rbx, [rbp + 1416]
        call func.print_num
;       [406:9] print(colon)
;       [41:6] print(str i8[])
        func.print.406.9:
;           [406:9] alias str -> colon
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
        func.print.406.9.end:
;       [407:9] print(prompt1)
;       [41:6] print(str i8[])
        func.print.407.9:
;           [407:9] alias str -> prompt1
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
        func.print.407.9.end:
;       [408:12] nm.input()
;       [88:10] mut str.input()
        func.str.input.408.12:
;           [408:12] alias self -> nm
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
        func.str.input.408.12.end:
        if.410.12:
;       [410:12] ? nm.len <= 0
;       [410:12] ? nm.len <= 0
        cmp.410.12:
        cmp byte [rbp + 1288], 0
        jle loop.403.5.end
        if.410.12.code:
;           [411:13] break
        if.412.19:
;       [412:19] ? nm.len <= 4
;       [412:19] ? nm.len <= 4
        cmp.412.19:
        cmp byte [rbp + 1288], 4
        jg if.410.9.else
        if.412.19.code:
;           [413:13] print(prompt2)
;           [41:6] print(str i8[])
            func.print.413.13:
;               [413:13] alias str -> prompt2
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
            func.print.413.13.end:
;           [414:13] continue
            jmp loop.403.5
        if.410.9.else:
;           [416:13] greet(nm)
;           [99:6] greet(name str)
            func.greet.416.13:
;               [416:13] alias name -> nm
;               [100:5] print(prompt3)
;               [41:6] print(str i8[])
                func.print.100.5.416.13:
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
                func.print.100.5.416.13.end:
;               [101:10] name.print()
;               [95:6] str.print()
                func.str.print.101.10.416.13:
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
                func.str.print.101.10.416.13.end:
;               [102:5] print(dot)
;               [41:6] print(str i8[])
                func.print.102.5.416.13:
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
                func.print.102.5.416.13.end:
;               [103:5] print(nl)
;               [41:6] print(str i8[])
                func.print.103.5.416.13:
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
                func.print.103.5.416.13.end:
;               [104:5] names = names + 1
;               [104:13] instructions without scratch register 1, with 3
;               [104:13] names
;               [104:13] names + 1
;               [104:13] src: folded constant '+ 1'
                add qword [rbp + 368], 1
            func.greet.416.13.end:
        if.410.9.end:
    jmp loop.403.5
    loop.403.5.end:
;   [420:5] print(greeted)
;   [41:6] print(str i8[])
    func.print.420.5:
;       [420:5] alias str -> greeted
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
    func.print.420.5.end:
;   [421:5] print_num(names)
;   [421:5] frame capacity check begin
;   [421:5] allocate scratch register -> r15
;   [421:5] allocate scratch register -> r14
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
;   [421:5] free scratch register r14
;   [421:5] free scratch register r15
;   [421:5] frame capacity check end
;   [421:5] address of argument 'names' to parameter 'num'
;   [421:5] allocate scratch register -> r15
    lea r15, [rbp + 368]
    mov qword [rbp + 1416], r15
;   [421:5] free scratch register r15
;   [421:5] set function frame base
    lea rbx, [rbp + 1416]
    call func.print_num
;   [422:5] print(nl)
;   [41:6] print(str i8[])
    func.print.422.5:
;       [422:5] alias str -> nl
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
    func.print.422.5.end:
;   [424:5] var bye = "bye from baz\n"
;   [424:9] bye: i8[13] (13 B @ [rbp + 1416])
;   [424:9] bye = "bye from baz\n"
;   [424:15] size <= 16 B, use immediates
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
;   [425:5] write(1, bye, 3)
;   [425:5] allocate named register rdi
;   [425:5] allocate named register rsi
;   [425:5] allocate named register rdx
;   [425:11] 1
    mov rdi, 1
;   [425:19] 3
    mov rdx, 3
;   [425:14] bounds check begin
;   [425:14] lower bound
;   [425:14] rdx lower bound covered by the unsigned upper bound
;   [425:14] upper bound
    cmp rdx, 13
    ja baz_bounds_line_425
;   [425:14] bounds check end
    lea rsi, [rbp + 1416]
;   [425:5] allocate named register rax
    mov rax, 1
    syscall
;   [425:5] free named register rax
;   [425:5] free named register rdx
;   [425:5] free named register rsi
;   [425:5] free named register rdi
;   [426:5] write(1, bye, 1, array_length(bye) - 1)
;   [426:5] allocate named register rdi
;   [426:5] allocate named register rsi
;   [426:5] allocate named register rdx
;   [426:11] 1
    mov rdi, 1
;   [426:19] 1
    mov rdx, 1
;   [426:22] allocate scratch register -> r15
;   [426:22] r15 = 12
;   [426:22] src: folded constant 'array_length(bye) - 1'
    mov r15, 12
;   [426:22] bounds check begin
;   [426:22] lower bound
    test r15, r15
    js baz_bounds_line_426
    test rdx, rdx
    js baz_bounds_line_426
;   [426:22] upper bound
;   [426:22] allocate scratch register -> r14
    lea r14, [rdx + r15]
    cmp r14, 13
;   [426:22] free scratch register r14
    jg baz_bounds_line_426
;   [426:22] bounds check end
    lea rsi, [rbp + 1416]
    add rsi, r15
;   [426:5] free scratch register r15
;   [426:5] allocate named register rax
    mov rax, 1
    syscall
;   [426:5] free named register rax
;   [426:5] free named register rdx
;   [426:5] free named register rsi
;   [426:5] free named register rdi
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
;   [179:13] instructions without scratch register 4, with 4
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
;   [181:11] instructions without scratch register 6, with 4
;   [181:11] allocate scratch register -> r14
;   [181:11] n
;   [181:11] allocate scratch register -> r13
    mov r13, qword [rbx + 8]
    mov r14, qword [r13]
;   [181:11] free scratch register r13
;   [181:15] r14 * partial
;   [181:15] src: operand
    imul r14, qword [rbx + 24]
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
;       [148:13] instructions without scratch register 1, with 3
;       [148:14] -n
        neg qword [rbx + 32]
    if.147.5.end:
;   [151:5] var i = buf_count
;   [151:9] i: i64 (8 B @ [rbx + 48])
;   [151:9] i = buf_count
;   [151:13] buf_count
    mov qword [rbx + 48], 20
;   [152:5] label
    loop.152.5:
;       [153:9] i = i - 1
;       [153:13] instructions without scratch register 1, with 3
;       [153:13] i
;       [153:13] i - 1
;       [153:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
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
            cqo
;           [154:31] allocate scratch register -> r12
            mov r12, 10
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
;       [155:13] instructions without scratch register 5, with 7
;       [155:13] n
;       [155:17] n / 10
;       [155:17] src: constant
;       [155:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [155:17] allocate named register rdx
        cqo
;       [155:17] allocate scratch register -> r15
        mov r15, 10
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
;       [160:13] instructions without scratch register 1, with 3
;       [160:13] i
;       [160:13] i - 1
;       [160:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
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
;       [167:21] instructions without scratch register 1, with 3
;       [167:21] write_pos
;       [167:21] write_pos + 1
;       [167:21] src: folded constant '+ 1'
        add qword [rbx + 56], 1
;       [168:9] i = i + 1
;       [168:13] instructions without scratch register 1, with 3
;       [168:13] i
;       [168:13] i + 1
;       [168:13] src: folded constant '+ 1'
        add qword [rbx + 48], 1
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
baz_bounds_line_241:
    mov rbp, 241
    jmp baz_bounds_panic
baz_bounds_line_242:
    mov rbp, 242
    jmp baz_bounds_panic
baz_bounds_line_246:
    mov rbp, 246
    jmp baz_bounds_panic
baz_bounds_line_251:
    mov rbp, 251
    jmp baz_bounds_panic
baz_bounds_line_252:
    mov rbp, 252
    jmp baz_bounds_panic
baz_bounds_line_258:
    mov rbp, 258
    jmp baz_bounds_panic
baz_bounds_line_269:
    mov rbp, 269
    jmp baz_bounds_panic
baz_bounds_line_270:
    mov rbp, 270
    jmp baz_bounds_panic
baz_bounds_line_271:
    mov rbp, 271
    jmp baz_bounds_panic
baz_bounds_line_382:
    mov rbp, 382
    jmp baz_bounds_panic
baz_bounds_line_383:
    mov rbp, 383
    jmp baz_bounds_panic
baz_bounds_line_390:
    mov rbp, 390
    jmp baz_bounds_panic
baz_bounds_line_391:
    mov rbp, 391
    jmp baz_bounds_panic
baz_bounds_line_425:
    mov rbp, 425
    jmp baz_bounds_panic
baz_bounds_line_426:
    mov rbp, 426
baz_bounds_panic:
;    print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
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
section .rodata
msg_panic:
db `panic: bounds at line `
msg_panic_len equ $ - msg_panic
section .bss
num_buffer:
resb 21

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
;                    factorial: 1 body, 2 calls, 35 instructions
;                    print_num: 1 body, 2 calls, 62 instructions
;
;   removed jumps to next code: 129
;    removed unreachable jumps: 2
; removed same target branches: 54
; inverted branches over jumps: 7
; max scratch registers in use: 4
;            max frames in use: 10
;                     dat size: 376 B
;              dat var padding: 8 B
;                max vars size: 1045 B
;                 instructions: 978
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
;       0      2  print_num 405:9
```
