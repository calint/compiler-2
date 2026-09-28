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
  `continue`, `break`, `return`, `self`
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
C/C++ Header                    54           5443           2050          17360
C++                              1             66             19            329
-------------------------------------------------------------------------------
SUM:                            55           5509           2069          17689
-------------------------------------------------------------------------------
```

## Sample

```text
# user types are defined using keyword `type`

# built-in types are `i63`, `i32`, `i16`, `i8` and `bool`

# default type is used if ommitted (`i64` on x86_64 and 'i32' on rv32i)

type point {x, y}

type object {pos point, color i32}

type world { locations[8] }

type str {
    len i8,
    data[127] i8, # trailing comma allowed
}

# initial data is initialized before variables

dat   hello[] i8 = "hello world from baz\n"
dat prompt1[] i8 = "enter name:\n"
dat prompt2[] i8 = "that is not a name.\n"
dat prompt3[] i8 = "hello "
dat     dot[] i8 = "."
dat      nl[] i8 = "\n"
dat   colon[] i8 = ": "
dat    nums[4] = { 1 } # remaining elements are zeroed
dat    str1 str = { 3 } # remaining fields are zeroed

# default is to inline functions

func assert(ok bool) { if not ok exit(1) }
# exit is a built-in function

func print(str[] i8) {
    write(1, str)
    # write is a built-in function that operates on file descriptors
    # it has 2 more optional arguments: count and start index
}

# function arguments and return are equivalent to mutable references

# functions can act on user types: `func point.fooz()` is called as
# `p.fooz()`

func point.fooz() {
    self.x = 0b10    # binary value 2
    self.y = 0xb     # hex value 11
}

# default argument type is i64 on x86_64 and i32 on rv32i
# arguments are references to memory locations

func bar(arg) {
    if arg == 0 return
    arg = 0xff
}

# return is a reference to the target with optional type
# it is accessed as a variable, in this case `res`

func inv(i i32) res i32 {
    res = ~i
}

func baz(arg) res {
    res = arg * 2
}

# array arguments are declared with [] and optional type

func faz(arg[] i32) {
    arg[1] = 0xfe
}

func str.input() {
    var nbytes = read(0, self.data)
    # read is built-in function that operates on file descriptors
    # it has 2 more optional arguments: count and start index
    self.len = i8(nbytes - 1)
} 

func str.output() {
    write(1, self.data, self.len)
} 

# a constructor builds its result `self` and must assign every field:
# `func point.at(x, y) self` is called as `point.at(x, y)`

func point.at(x, y) self {
    self.x = x
    self.y = y
}

func object.at(x, y, color i32) self {
    self.pos = point.at(x, y)
    self.color = color
}

const yes = 1
const no = 0
const maybe = -1

# constants can be declared in any scope and shadow outer declarations

# limited support for non-inlined functions
# arguments and return are references to memory locations
# arrays not supported

func noinline print_num(num) {
    # 19 digits of an i64 plus the sign
    const buf_count = 20

    var buf[buf_count] i8
    var n = num
    var is_negative bool

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
 
    var write_pos
    loop {
        buf[write_pos] = buf[i]
        write_pos = write_pos + 1
        i = i + 1
        if i == buf_count break
    }
 
    write(1, buf, write_pos)
}

func main() {
    var answer
    # variables without initializer are zeroed
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

    var arr[4] i32
    # arrays without initializer are zeroed 

    var ix = 1
    # variables can have an initial expression

    arr[ix] = 2
    arr[ix + 1] = arr[ix]
    assert(arr[1] == 2)
    assert(arr[2] == 2)

    array_copy(arr[2], arr, 2)
    assert(arr[0] == 2)
    # `array_copy` is a built-in function: copy from, to, number of elements

    var arr1[8] i32
    array_copy(arr, arr1, 4)
    var eq bool = arrays_equal(arr[1], arr1[1], 3)
    # type `bool` is built-in
    # `arrays_equal` is built-in function comparing source and destination
    assert(eq)

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

#   arr[ix] = ~inv(arr[ix - 1])
#   compile time error because it could ub since the "return" of the function
#   and argument refers to same memory range (arr)

    ix = 3
    var tmp i32 = ~inv(arr[ix - 1])
    arr[ix] = tmp
    assert(arr[ix] == 2)

    faz(arr)
    assert(arr[1] == 0xfe)

    var arr3[] = { 3, 5 }
    foo arr3 {
        e = e + i + n
    }
    assert(arr3[0] == 3 + 0 + 2)
    assert(arr3[1] == 5 + 1 + 2)
    # `foo` is a language construct that iterates over an array injecting:
    #   `e`: current element
    #   `i`: index starting at 0
    #   `n`: constant array size

    var p point
    # user types without initializer are zeroed

    p.fooz()
    # call on user type method

    assert(p.x == 2)
    assert(p.y == 0xb)

    var q point = p
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

    var p0 point = {baz(3), 0}
    assert(p0.x == 6)

    var pt = point.at(-1, -2)
    # a constructor builds its result `self` in the destination
    # `pt` has the type of the constructor
 
    assert(pt.x == -1)
    assert(pt.y == -2)

    var x = 1
    var y = 2

    var o1 object = {{x * 10, y}, 0xff0000}
    assert(o1.pos.x == 10)
    assert(o1.pos.y == 2)
    assert(o1.color == 0xff0000)

    var p1 point = {-x, -y}
    o1.pos = p1
    assert(o1.pos.x == -1)
    assert(o1.pos.y == -2)

    var o2 object = o1
    assert(o2.pos.x == -1)
    assert(o2.pos.y == -2)
    assert(o2.color == 0xff0000)

    var o3[2] object
    o3[0].pos.y = 73

    assert(o3[0].pos.y == 73)
    o3[1] = object.at(2, 74, 0xffffff)
    assert(o3[1].pos.y == 74)

    var worlds[8] world
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
    var arr2[] = { -1, 2 }
    assert(array_length(arr2) == 2)
    assert(arr2[0] == -1)
    assert(arr2[1] == 2)

    var counter
    var nm str
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
            print(prompt3)
            nm.output()
            print(dot)
            print(nl)
        }
    }
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
    mov qword [rbp + 224], 0
    cmp.154.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.154.12.end:
    func.assert.154.5:
        if.32.27.154.5:
        cmp.32.27.154.5:
        cmp r15b, 0
        jne if.32.24.154.5.end
        if.32.27.154.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.154.5.end:
    func.assert.154.5.end:
    mov qword [rbp + 224], -1
    cmp.157.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.157.12.end:
    func.assert.157.5:
        if.32.27.157.5:
        cmp.32.27.157.5:
        cmp r15b, 0
        jne if.32.24.157.5.end
        if.32.27.157.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.157.5.end:
    func.assert.157.5.end:
        func.assert.163.9:
            if.32.27.163.9:
            cmp.32.27.163.9:
            if.32.24.163.9.end:
        func.assert.163.9.end:
    func.assert.166.5:
        if.32.27.166.5:
        cmp.32.27.166.5:
        if.32.24.166.5.end:
    func.assert.166.5.end:
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 232], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 232]
    mov dword [rbp + r15 * 4 + 232], r13d
    cmp.176.12:
    cmp dword [rbp + 236], 2
    sete r15b
    bool.176.12.end:
    func.assert.176.5:
        if.32.27.176.5:
        cmp.32.27.176.5:
        cmp r15b, 0
        jne if.32.24.176.5.end
        if.32.27.176.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.176.5.end:
    func.assert.176.5.end:
    cmp.177.12:
    cmp dword [rbp + 240], 2
    sete r15b
    bool.177.12.end:
    func.assert.177.5:
        if.32.27.177.5:
        cmp.32.27.177.5:
        cmp r15b, 0
        jne if.32.24.177.5.end
        if.32.27.177.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.177.5.end:
    func.assert.177.5.end:
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
    mov rax, qword [rbp + r14 * 4 + 232]
    mov qword [rbp + 232], rax
    cmp.180.12:
    cmp dword [rbp + 232], 2
    sete r15b
    bool.180.12.end:
    func.assert.180.5:
        if.32.27.180.5:
        cmp.32.27.180.5:
        cmp r15b, 0
        jne if.32.24.180.5.end
        if.32.27.180.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.180.5.end:
    func.assert.180.5.end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov r15, 4
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jg baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    cmp r15, 8
    jg baz_bounds_panic
    mov rax, qword [rbp + 232]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 240]
    mov qword [rbp + 264], rax
    cmp.185.19:
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
        lea rsi, [rbp + r15 * 4 + 232]
        mov r15, 1
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
        mov r14, rcx
        add r14, r15
        cmp r14, 8
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 288]
    bool.185.19.end:
    cmp.188.12:
    mov r15b, byte [rbp + 288]
    bool.188.12.end:
    func.assert.188.5:
        if.32.27.188.5:
        cmp.32.27.188.5:
        cmp r15b, 0
        jne if.32.24.188.5.end
        if.32.27.188.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.188.5.end:
    func.assert.188.5.end:
    mov dword [rbp + 264], -1
    cmp.191.12:
        mov rcx, 4
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.191.12.end:
    func.assert.191.5:
        if.32.27.191.5:
        cmp.32.27.191.5:
        cmp r15b, 0
        jne if.32.24.191.5.end
        if.32.27.191.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.191.5.end:
    func.assert.191.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.198.20:
        mov r14d, dword [rbp + r15 * 4 + 232]
        mov dword [rbp + 292], r14d
        not dword [rbp + 292]
    func.inv.198.20.end:
    not dword [rbp + 292]
    mov r15, qword [rbp + 248]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 292]
    mov dword [rbp + r15 * 4 + 232], r14d
    cmp.200.12:
    mov r14, qword [rbp + 248]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.200.12.end:
    func.assert.200.5:
        if.32.27.200.5:
        cmp.32.27.200.5:
        cmp r15b, 0
        jne if.32.24.200.5.end
        if.32.27.200.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.200.5.end:
    func.assert.200.5.end:
    func.faz.202.5:
        mov dword [rbp + 236], 254
    func.faz.202.5.end:
    cmp.203.12:
    cmp dword [rbp + 236], 254
    sete r15b
    bool.203.12.end:
    func.assert.203.5:
        if.32.27.203.5:
        cmp.32.27.203.5:
        cmp r15b, 0
        jne if.32.24.203.5.end
        if.32.27.203.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.203.5.end:
    func.assert.203.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.206.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.206.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.206.5
    foo.206.5.end:
    cmp.209.12:
    cmp qword [rbp + 296], 5
    sete r15b
    bool.209.12.end:
    func.assert.209.5:
        if.32.27.209.5:
        cmp.32.27.209.5:
        cmp r15b, 0
        jne if.32.24.209.5.end
        if.32.27.209.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.209.5.end:
    func.assert.209.5.end:
    cmp.210.12:
    cmp qword [rbp + 304], 8
    sete r15b
    bool.210.12.end:
    func.assert.210.5:
        if.32.27.210.5:
        cmp.32.27.210.5:
        cmp r15b, 0
        jne if.32.24.210.5.end
        if.32.27.210.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.210.5.end:
    func.assert.210.5.end:
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.219.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.219.7.end:
    cmp.222.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.222.12.end:
    func.assert.222.5:
        if.32.27.222.5:
        cmp.32.27.222.5:
        cmp r15b, 0
        jne if.32.24.222.5.end
        if.32.27.222.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.222.5.end:
    func.assert.222.5.end:
    cmp.223.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.223.12.end:
    func.assert.223.5:
        if.32.27.223.5:
        cmp.32.27.223.5:
        cmp r15b, 0
        jne if.32.24.223.5.end
        if.32.27.223.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.223.5.end:
    func.assert.223.5.end:
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.228.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.228.12.end:
    func.assert.228.5:
        if.32.27.228.5:
        cmp.32.27.228.5:
        cmp r15b, 0
        jne if.32.24.228.5.end
        if.32.27.228.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.228.5.end:
    func.assert.228.5.end:
    mov qword [rbp + 328], 3
    cmp.233.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        setne r15b
    bool.233.12.end:
    func.assert.233.5:
        if.32.27.233.5:
        cmp.32.27.233.5:
        cmp r15b, 0
        jne if.32.24.233.5.end
        if.32.27.233.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.233.5.end:
    func.assert.233.5.end:
    mov qword [rbp + 344], 0
    func.bar.236.5:
        if.55.8.236.5:
        cmp.55.8.236.5:
        cmp qword [rbp + 344], 0
        je func.bar.236.5.end
        if.55.8.236.5.code:
        if.55.5.236.5.end:
        mov qword [rbp + 344], 255
    func.bar.236.5.end:
    cmp.237.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.237.12.end:
    func.assert.237.5:
        if.32.27.237.5:
        cmp.32.27.237.5:
        cmp r15b, 0
        jne if.32.24.237.5.end
        if.32.27.237.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.237.5.end:
    func.assert.237.5.end:
    mov qword [rbp + 344], 1
    func.bar.240.5:
        if.55.8.240.5:
        cmp.55.8.240.5:
        cmp qword [rbp + 344], 0
        je func.bar.240.5.end
        if.55.8.240.5.code:
        if.55.5.240.5.end:
        mov qword [rbp + 344], 255
    func.bar.240.5.end:
    cmp.241.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.241.12.end:
    func.assert.241.5:
        if.32.27.241.5:
        cmp.32.27.241.5:
        cmp r15b, 0
        jne if.32.24.241.5.end
        if.32.27.241.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.241.5.end:
    func.assert.241.5.end:
    mov qword [rbp + 352], 1
    func.baz.244.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.244.13.end:
    cmp.245.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.245.12.end:
    func.assert.245.5:
        if.32.27.245.5:
        cmp.32.27.245.5:
        cmp r15b, 0
        jne if.32.24.245.5.end
        if.32.27.245.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.245.5.end:
    func.assert.245.5.end:
    func.baz.247.9:
        mov qword [rbp + 360], 2
    func.baz.247.9.end:
    cmp.248.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.248.12.end:
    func.assert.248.5:
        if.32.27.248.5:
        cmp.32.27.248.5:
        cmp r15b, 0
        jne if.32.24.248.5.end
        if.32.27.248.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.248.5.end:
    func.assert.248.5.end:
    func.baz.250.21:
        mov qword [rbp + 368], 6
    func.baz.250.21.end:
    mov qword [rbp + 376], 0
    cmp.251.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.251.12.end:
    func.assert.251.5:
        if.32.27.251.5:
        cmp.32.27.251.5:
        cmp r15b, 0
        jne if.32.24.251.5.end
        if.32.27.251.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.251.5.end:
    func.assert.251.5.end:
    func.point.at.253.14:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point.at.253.14.end:
    cmp.257.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.257.12.end:
    func.assert.257.5:
        if.32.27.257.5:
        cmp.32.27.257.5:
        cmp r15b, 0
        jne if.32.24.257.5.end
        if.32.27.257.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.257.5.end:
    func.assert.257.5.end:
    cmp.258.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.258.12.end:
    func.assert.258.5:
        if.32.27.258.5:
        cmp.32.27.258.5:
        cmp r15b, 0
        jne if.32.24.258.5.end
        if.32.27.258.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.258.5.end:
    func.assert.258.5.end:
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.264.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.264.12.end:
    func.assert.264.5:
        if.32.27.264.5:
        cmp.32.27.264.5:
        cmp r15b, 0
        jne if.32.24.264.5.end
        if.32.27.264.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.264.5.end:
    func.assert.264.5.end:
    cmp.265.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.265.12.end:
    func.assert.265.5:
        if.32.27.265.5:
        cmp.32.27.265.5:
        cmp r15b, 0
        jne if.32.24.265.5.end
        if.32.27.265.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.265.5.end:
    func.assert.265.5.end:
    cmp.266.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.266.12.end:
    func.assert.266.5:
        if.32.27.266.5:
        cmp.32.27.266.5:
        cmp r15b, 0
        jne if.32.24.266.5.end
        if.32.27.266.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.266.5.end:
    func.assert.266.5.end:
    mov r15, qword [rbp + 400]
    mov qword [rbp + 440], r15
    neg qword [rbp + 440]
    mov r15, qword [rbp + 408]
    mov qword [rbp + 448], r15
    neg qword [rbp + 448]
    mov rax, qword [rbp + 440]
    mov qword [rbp + 416], rax
    mov rax, qword [rbp + 448]
    mov qword [rbp + 424], rax
    cmp.270.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.270.12.end:
    func.assert.270.5:
        if.32.27.270.5:
        cmp.32.27.270.5:
        cmp r15b, 0
        jne if.32.24.270.5.end
        if.32.27.270.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.270.5.end:
    func.assert.270.5.end:
    cmp.271.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.271.12.end:
    func.assert.271.5:
        if.32.27.271.5:
        cmp.32.27.271.5:
        cmp r15b, 0
        jne if.32.24.271.5.end
        if.32.27.271.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.271.5.end:
    func.assert.271.5.end:
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.274.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.274.12.end:
    func.assert.274.5:
        if.32.27.274.5:
        cmp.32.27.274.5:
        cmp r15b, 0
        jne if.32.24.274.5.end
        if.32.27.274.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.274.5.end:
    func.assert.274.5.end:
    cmp.275.12:
    cmp qword [rbp + 464], -2
    sete r15b
    bool.275.12.end:
    func.assert.275.5:
        if.32.27.275.5:
        cmp.32.27.275.5:
        cmp r15b, 0
        jne if.32.24.275.5.end
        if.32.27.275.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.275.5.end:
    func.assert.275.5.end:
    cmp.276.12:
    cmp dword [rbp + 472], 16711680
    sete r15b
    bool.276.12.end:
    func.assert.276.5:
        if.32.27.276.5:
        cmp.32.27.276.5:
        cmp r15b, 0
        jne if.32.24.276.5.end
        if.32.27.276.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.276.5.end:
    func.assert.276.5.end:
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.281.12:
    cmp qword [rbp + 488], 73
    sete r15b
    bool.281.12.end:
    func.assert.281.5:
        if.32.27.281.5:
        cmp.32.27.281.5:
        cmp r15b, 0
        jne if.32.24.281.5.end
        if.32.27.281.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.281.5.end:
    func.assert.281.5.end:
    func.object.at.282.13:
        func.point.at.96.16.282.13:
            mov qword [rbp + 504], 2
            mov qword [rbp + 512], 74
        func.point.at.96.16.282.13.end:
        mov dword [rbp + 520], 16777215
    func.object.at.282.13.end:
    cmp.283.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.283.12.end:
    func.assert.283.5:
        if.32.27.283.5:
        cmp.32.27.283.5:
        cmp r15b, 0
        jne if.32.24.283.5.end
        if.32.27.283.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.283.5.end:
    func.assert.283.5.end:
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 600], 65518
    cmp.287.12:
    cmp qword [rbp + 600], 65518
    sete r15b
    bool.287.12.end:
    func.assert.287.5:
        if.32.27.287.5:
        cmp.32.27.287.5:
        cmp r15b, 0
        jne if.32.24.287.5.end
        if.32.27.287.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.287.5.end:
    func.assert.287.5.end:
    mov rcx, 8
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 592]
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 528]
    shl rcx, 3
    rep movsb
    cmp.296.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.296.12.end:
    func.assert.296.5:
        if.32.27.296.5:
        cmp.32.27.296.5:
        cmp r15b, 0
        jne if.32.24.296.5.end
        if.32.27.296.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.296.5.end:
    func.assert.296.5.end:
    cmp.297.12:
        mov rcx, 8
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 528]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 592]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.297.12.end:
    func.assert.297.5:
        if.32.27.297.5:
        cmp.32.27.297.5:
        cmp r15b, 0
        jne if.32.24.297.5.end
        if.32.27.297.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.297.5.end:
    func.assert.297.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.303.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.303.12.end:
    func.assert.303.5:
        if.32.27.303.5:
        cmp.32.27.303.5:
        cmp r15b, 0
        jne if.32.24.303.5.end
        if.32.27.303.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.303.5.end:
    func.assert.303.5.end:
    cmp.304.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.304.12.end:
    func.assert.304.5:
        if.32.27.304.5:
        cmp.32.27.304.5:
        cmp r15b, 0
        jne if.32.24.304.5.end
        if.32.27.304.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.304.5.end:
    func.assert.304.5.end:
    cmp.305.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.305.12.end:
    func.assert.305.5:
        if.32.27.305.5:
        cmp.32.27.305.5:
        cmp r15b, 0
        jne if.32.24.305.5.end
        if.32.27.305.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.305.5.end:
    func.assert.305.5.end:
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.309.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.309.5.end:
    loop.310.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1192]
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
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        lea rbx, [rbp + 1192]
        call func.print_num
        func.print.313.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.313.9.end:
        func.print.314.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.314.9.end:
        func.str.input.315.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.315.12.end:
        if.317.12:
        cmp.317.12:
        cmp byte [rbp + 1064], 0
        jle loop.310.5.end
        if.317.12.code:
        if.319.19:
        cmp.319.19:
        cmp byte [rbp + 1064], 4
        jg if.317.9.else
        if.319.19.code:
            func.print.320.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.320.13.end:
            jmp loop.310.5
        if.317.9.else:
            func.print.323.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.323.13.end:
            func.str.output.324.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 1064]
                test rdx, rdx
                js baz_bounds_panic
                cmp rdx, 127
                jg baz_bounds_panic
                lea rsi, [rbp + 1065]
                mov rax, 1
                syscall
            func.str.output.324.16.end:
            func.print.325.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.325.13.end:
            func.print.326.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.326.13.end:
        if.317.9.end:
    jmp loop.310.5
    loop.310.5.end:
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
    if.120.8:
    cmp.120.8:
    cmp qword [rbx + 32], 0
    jge if.120.5.end
    if.120.8.code:
        mov byte [rbx + 40], 1
    if.120.5.end:
    if.123.8:
    cmp.123.8:
    cmp qword [rbx + 32], 0
    jle if.123.5.end
    if.123.8.code:
        neg qword [rbx + 32]
    if.123.5.end:
    mov qword [rbx + 48], 20
    loop.128.5:
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
        if.132.12:
        cmp.132.12:
        cmp qword [rbx + 32], 0
        jne loop.128.5
        if.132.12.code:
        if.132.9.end:
    loop.128.5.end:
    if.135.8:
    cmp.135.8:
    cmp byte [rbx + 40], 0
    je if.135.5.end
    if.135.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.135.5.end:
    mov qword [rbx + 56], 0
    loop.141.5:
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
        if.145.12:
        cmp.145.12:
        cmp qword [rbx + 48], 20
        jne loop.141.5
        if.145.12.code:
        if.145.9.end:
    loop.141.5.end:
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
times 127 db 0
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 131072
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
;[13:1] str : 128 B    fields:
;[13:1]       name :  offset :    size :  array? : array size
;[13:1]        len :       0 :       1 :      no :           
;[13:1]       data :       1 :     127 :     yes :        127
;
;[20:1] dat hello[] i8 = "hello world from baz\n"
;[20:7] hello: i8[21] (21 B @ [rbp])
;[21:1] dat prompt1[] i8 = "enter name:\n"
;[21:5] prompt1: i8[12] (12 B @ [rbp + 21])
;[22:1] dat prompt2[] i8 = "that is not a name.\n"
;[22:5] prompt2: i8[20] (20 B @ [rbp + 33])
;[23:1] dat prompt3[] i8 = "hello "
;[23:5] prompt3: i8[6] (6 B @ [rbp + 53])
;[24:1] dat dot[] i8 = "."
;[24:9] dot: i8[1] (1 B @ [rbp + 59])
;[25:1] dat nl[] i8 = "\n"
;[25:10] nl: i8[1] (1 B @ [rbp + 60])
;[26:1] dat colon[] i8 = ": "
;[26:7] colon: i8[2] (2 B @ [rbp + 61])
;[27:1] dat nums[4] = { 1 }
;[27:8] nums: i64[4] (32 B @ [rbp + 64])
;[28:1] dat str1 str = { 3 }
;[28:8] str1: str (128 B @ [rbp + 96])
;[100:7] const yes = 1
;[101:7] const no = 0
;[102:7] const maybe = -1
;
main:
;   [152:5] var answer
;   [152:9] answer: i64 (8 B @ [rbp + 224])
;   [152:9] zero 1 * 8 B = 8 B
;   [152:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
;   [154:5] assert(answer == 0)
;   [154:12] allocate scratch register -> r15
;   [154:12] ? answer == 0
;   [154:12] ? answer == 0
    cmp.154.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.154.12.end:
;   [32:6] assert(ok bool)
    func.assert.154.5:
;       [154:5] alias ok -> r15b
        if.32.27.154.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.154.5:
        cmp r15b, 0
        jne if.32.24.154.5.end
        if.32.27.154.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.154.5.end:
;       [154:5] free scratch register r15
    func.assert.154.5.end:
;   [156:5] answer = maybe
;   [156:14] maybe
    mov qword [rbp + 224], -1
;   [157:5] assert(answer == -1)
;   [157:12] allocate scratch register -> r15
;   [157:12] ? answer == -1
;   [157:12] ? answer == -1
    cmp.157.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.157.12.end:
;   [32:6] assert(ok bool)
    func.assert.157.5:
;       [157:5] alias ok -> r15b
        if.32.27.157.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.157.5:
        cmp r15b, 0
        jne if.32.24.157.5.end
        if.32.27.157.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.157.5.end:
;       [157:5] free scratch register r15
    func.assert.157.5.end:
;       [162:15] const maybe = 33
;       [163:9] assert(maybe == 33)
;       [32:6] assert(ok bool)
        func.assert.163.9:
;           [163:9] alias ok -> 1
            if.32.27.163.9:
;           [32:27] ? not ok
;           [32:27] ? shorthand: not ok
            cmp.32.27.163.9:
;           [32:31] const eval to false
            if.32.24.163.9.end:
        func.assert.163.9.end:
;   [166:5] assert(maybe == -1)
;   [32:6] assert(ok bool)
    func.assert.166.5:
;       [166:5] alias ok -> 1
        if.32.27.166.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.166.5:
;       [32:31] const eval to false
        if.32.24.166.5.end:
    func.assert.166.5.end:
;   [168:5] var arr[4] i32
;   [168:9] arr: i32[4] (16 B @ [rbp + 232])
;   [168:9] zero 4 * 4 B = 16 B
;   [168:5] size <= 32 B, use mov
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
;   [171:5] var ix = 1
;   [171:9] ix: i64 (8 B @ [rbp + 248])
;   [171:9] ix = 1
;   [171:14] 1
    mov qword [rbp + 248], 1
;   [174:5] arr[ix] = 2
;   [174:9] allocate scratch register -> r15
;   [174:9] set array index
;   [174:9] ix
    mov r15, qword [rbp + 248]
;   [174:9] bounds check
;   [174:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [174:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [174:15] 2
    mov dword [rbp + r15 * 4 + 232], 2
;   [174:5] free scratch register r15
;   [175:5] arr[ix + 1] = arr[ix]
;   [175:9] allocate scratch register -> r15
;   [175:9] set array index
;   [175:9] ix
    mov r15, qword [rbp + 248]
;   [175:9] r15 + 1
;   [175:9] src: folded constant '+ 1'
    add r15, 1
;   [175:9] bounds check
;   [175:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [175:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [175:19] arr[ix]
;   [175:23] allocate scratch register -> r14
;   [175:23] set array index
;   [175:23] ix
    mov r14, qword [rbp + 248]
;   [175:23] bounds check
;   [175:23] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
;   [175:23] upper bound (--checks=upper)
    cmp r14, 4
    jge baz_bounds_panic
;   [175:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 232]
    mov dword [rbp + r15 * 4 + 232], r13d
;   [175:19] free scratch register r13
;   [175:19] free scratch register r14
;   [175:5] free scratch register r15
;   [176:5] assert(arr[1] == 2)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? arr[1] == 2
;   [176:12] ? arr[1] == 2
    cmp.176.12:
    cmp dword [rbp + 236], 2
    sete r15b
    bool.176.12.end:
;   [32:6] assert(ok bool)
    func.assert.176.5:
;       [176:5] alias ok -> r15b
        if.32.27.176.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.176.5:
        cmp r15b, 0
        jne if.32.24.176.5.end
        if.32.27.176.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.176.5.end:
;       [176:5] free scratch register r15
    func.assert.176.5.end:
;   [177:5] assert(arr[2] == 2)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? arr[2] == 2
;   [177:12] ? arr[2] == 2
    cmp.177.12:
    cmp dword [rbp + 240], 2
    sete r15b
    bool.177.12.end:
;   [32:6] assert(ok bool)
    func.assert.177.5:
;       [177:5] alias ok -> r15b
        if.32.27.177.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.177.5:
        cmp r15b, 0
        jne if.32.24.177.5.end
        if.32.27.177.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.177.5.end:
;       [177:5] free scratch register r15
    func.assert.177.5.end:
;   [179:5] array_copy(arr[2], arr, 2)
;   [179:5] allocate scratch register -> r15
;   [179:29] 2
;   [179:29] 2
    mov r15, 2
;   [179:16] arr[2]
;   [179:20] allocate scratch register -> r14
;   [179:20] set array index
;   [179:20] 2
    mov r14, 2
;   [179:20] bounds check
;   [179:20] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
;   [179:20] upper bound (--checks=upper)
;   [179:20] allocate scratch register -> r13
    mov r13, r15
    add r13, r14
    cmp r13, 4
;   [179:20] free scratch register r13
    jg baz_bounds_panic
;   [179:24] arr
;   [179:24] bounds check
;   [179:24] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [179:24] upper bound (--checks=upper)
    cmp r15, 4
    jg baz_bounds_panic
;   [179:5] size <= 16 B, use mov
;   [179:5] allocate named register rax
    mov rax, qword [rbp + r14 * 4 + 232]
    mov qword [rbp + 232], rax
;   [179:5] free named register rax
;   [179:5] free scratch register r14
;   [179:5] free scratch register r15
;   [180:5] assert(arr[0] == 2)
;   [180:12] allocate scratch register -> r15
;   [180:12] ? arr[0] == 2
;   [180:12] ? arr[0] == 2
    cmp.180.12:
    cmp dword [rbp + 232], 2
    sete r15b
    bool.180.12.end:
;   [32:6] assert(ok bool)
    func.assert.180.5:
;       [180:5] alias ok -> r15b
        if.32.27.180.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.180.5:
        cmp r15b, 0
        jne if.32.24.180.5.end
        if.32.27.180.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.180.5.end:
;       [180:5] free scratch register r15
    func.assert.180.5.end:
;   [183:5] var arr1[8] i32
;   [183:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [183:9] zero 8 * 4 B = 32 B
;   [183:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [184:5] array_copy(arr, arr1, 4)
;   [184:5] allocate scratch register -> r15
;   [184:27] 4
;   [184:27] 4
    mov r15, 4
;   [184:16] arr
;   [184:16] bounds check
;   [184:16] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [184:16] upper bound (--checks=upper)
    cmp r15, 4
    jg baz_bounds_panic
;   [184:21] arr1
;   [184:21] bounds check
;   [184:21] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [184:21] upper bound (--checks=upper)
    cmp r15, 8
    jg baz_bounds_panic
;   [184:5] size <= 16 B, use mov
;   [184:5] allocate named register rax
    mov rax, qword [rbp + 232]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 240]
    mov qword [rbp + 264], rax
;   [184:5] free named register rax
;   [184:5] free scratch register r15
;   [185:5] var eq bool = arrays_equal(arr[1], arr1[1], 3)
;   [185:9] eq: bool (1 B @ [rbp + 288])
;   [185:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [185:19] ? arrays_equal(arr[1], arr1[1], 3)
;   [185:19] ? shorthand: arrays_equal(arr[1], arr1[1], 3)
    cmp.185.19:
;       [185:19] arrays_equal(arr[1], arr1[1], 3)
;       [185:19] allocate named register rsi
;       [185:19] allocate named register rdi
;       [185:19] allocate named register rcx
;       [185:49] 3
;       [185:49] 3
        mov rcx, 3
;       [185:32] arr[1]
;       [185:36] allocate scratch register -> r15
;       [185:36] set array index
;       [185:36] 1
        mov r15, 1
;       [185:36] bounds check
;       [185:36] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
;       [185:36] upper bound (--checks=upper)
;       [185:36] allocate scratch register -> r14
        mov r14, rcx
        add r14, r15
        cmp r14, 4
;       [185:36] free scratch register r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 232]
;       [185:19] free scratch register r15
;       [185:40] arr1[1]
;       [185:45] allocate scratch register -> r15
;       [185:45] set array index
;       [185:45] 1
        mov r15, 1
;       [185:45] bounds check
;       [185:45] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
;       [185:45] upper bound (--checks=upper)
;       [185:45] allocate scratch register -> r14
        mov r14, rcx
        add r14, r15
        cmp r14, 8
;       [185:45] free scratch register r14
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 256]
;       [185:19] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [185:19] free named register rcx
;       [185:19] free named register rdi
;       [185:19] free named register rsi
        sete byte [rbp + 288]
    bool.185.19.end:
;   [188:5] assert(eq)
;   [188:12] allocate scratch register -> r15
;   [188:12] ? eq
;   [188:12] ? shorthand: eq
    cmp.188.12:
    mov r15b, byte [rbp + 288]
    bool.188.12.end:
;   [32:6] assert(ok bool)
    func.assert.188.5:
;       [188:5] alias ok -> r15b
        if.32.27.188.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.188.5:
        cmp r15b, 0
        jne if.32.24.188.5.end
        if.32.27.188.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.188.5.end:
;       [188:5] free scratch register r15
    func.assert.188.5.end:
;   [190:5] arr1[2] = -1
;   [190:15] instructions without scratch register 1, with 2
;   [190:16] -1
    mov dword [rbp + 264], -1
;   [191:5] assert(not arrays_equal(arr, arr1, 4))
;   [191:12] allocate scratch register -> r15
;   [191:12] ? not arrays_equal(arr, arr1, 4)
;   [191:12] ? shorthand: not arrays_equal(arr, arr1, 4)
    cmp.191.12:
;       [191:16] arrays_equal(arr, arr1, 4)
;       [191:16] allocate named register rsi
;       [191:16] allocate named register rdi
;       [191:16] allocate named register rcx
;       [191:40] 4
;       [191:40] 4
        mov rcx, 4
;       [191:29] arr
;       [191:29] bounds check
;       [191:29] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [191:29] upper bound (--checks=upper)
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
;       [191:34] arr1
;       [191:34] bounds check
;       [191:34] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [191:34] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [191:16] free named register rcx
;       [191:16] free named register rdi
;       [191:16] free named register rsi
        setne r15b
    bool.191.12.end:
;   [32:6] assert(ok bool)
    func.assert.191.5:
;       [191:5] alias ok -> r15b
        if.32.27.191.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.191.5:
        cmp r15b, 0
        jne if.32.24.191.5.end
        if.32.27.191.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.191.5.end:
;       [191:5] free scratch register r15
    func.assert.191.5.end:
;   [197:5] ix = 3
;   [197:10] 3
    mov qword [rbp + 248], 3
;   [198:5] var tmp i32 = ~inv(arr[ix - 1])
;   [198:9] tmp: i32 (4 B @ [rbp + 292])
;   [198:9] tmp = ~inv(arr[ix - 1])
;   [198:20] tmp = ~inv(arr[ix - 1])
;   [198:20] = expression
;   [198:20] ~inv(arr[ix - 1])
;   [198:20] instructions without scratch register 12, with 12
;   [198:28] allocate scratch register -> r15
;   [198:28] set array index
;   [198:28] ix
    mov r15, qword [rbp + 248]
;   [198:28] r15 - 1
;   [198:28] src: folded constant '- 1'
    sub r15, 1
;   [198:28] bounds check
;   [198:28] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [198:28] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [198:20] instructions without scratch register 6, with 7
;   [62:6] inv(i i32) res i32
    func.inv.198.20:
;       [198:20] alias res -> tmp
;       [198:20] alias i -> arr (lea: rbp + r15 * 4 + 232)
;       [63:5] res = ~i
;       [63:11] instructions without scratch register 3, with 3
;       [63:12] ~i
;       [63:12] allocate scratch register -> r14
        mov r14d, dword [rbp + r15 * 4 + 232]
        mov dword [rbp + 292], r14d
;       [63:12] free scratch register r14
        not dword [rbp + 292]
    func.inv.198.20.end:
    not dword [rbp + 292]
;       [198:20] free scratch register r15
;   [199:5] arr[ix] = tmp
;   [199:9] allocate scratch register -> r15
;   [199:9] set array index
;   [199:9] ix
    mov r15, qword [rbp + 248]
;   [199:9] bounds check
;   [199:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [199:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [199:15] tmp
;   [199:15] allocate scratch register -> r14
    mov r14d, dword [rbp + 292]
    mov dword [rbp + r15 * 4 + 232], r14d
;   [199:15] free scratch register r14
;   [199:5] free scratch register r15
;   [200:5] assert(arr[ix] == 2)
;   [200:12] allocate scratch register -> r15
;   [200:12] ? arr[ix] == 2
;   [200:12] ? arr[ix] == 2
    cmp.200.12:
;   [200:16] allocate scratch register -> r14
;   [200:16] set array index
;   [200:16] ix
    mov r14, qword [rbp + 248]
;   [200:16] bounds check
;   [200:16] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
;   [200:16] upper bound (--checks=upper)
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
;   [200:12] free scratch register r14
    sete r15b
    bool.200.12.end:
;   [32:6] assert(ok bool)
    func.assert.200.5:
;       [200:5] alias ok -> r15b
        if.32.27.200.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.200.5:
        cmp r15b, 0
        jne if.32.24.200.5.end
        if.32.27.200.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.200.5.end:
;       [200:5] free scratch register r15
    func.assert.200.5.end:
;   [202:5] faz(arr)
;   [72:6] faz(arg[] i32)
    func.faz.202.5:
;       [202:5] alias arg -> arr
;       [73:5] arg[1] = 0xfe
;       [73:14] 0xfe
        mov dword [rbp + 236], 254
    func.faz.202.5.end:
;   [203:5] assert(arr[1] == 0xfe)
;   [203:12] allocate scratch register -> r15
;   [203:12] ? arr[1] == 0xfe
;   [203:12] ? arr[1] == 0xfe
    cmp.203.12:
    cmp dword [rbp + 236], 254
    sete r15b
    bool.203.12.end:
;   [32:6] assert(ok bool)
    func.assert.203.5:
;       [203:5] alias ok -> r15b
        if.32.27.203.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.203.5:
        cmp r15b, 0
        jne if.32.24.203.5.end
        if.32.27.203.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.203.5.end:
;       [203:5] free scratch register r15
    func.assert.203.5.end:
;   [205:5] var arr3[] = { 3, 5 }
;   [205:9] arr3: i64[2] (16 B @ [rbp + 296])
;   [205:9] arr3= { 3, 5 }
;   [205:18] size <= 16 B, use immediates
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
;   [206:5] foo arr3
;   [206:9] allocate scratch register -> r15
;   [206:9] e: i64 (r15)
;   [206:9] i: i64 (8 B @ [rbp + 320])
;   [206:9] const n = 2
;   [206:9] initiate iterator e
    lea r15, [rbp + 296]
;   [206:9] initiate counter i
    mov qword [rbp + 320], 0
    foo.206.5:
;       [207:9] e = e + i + n
;       [207:13] instructions without scratch register 3, with 4
;       [207:13] e
;       [207:17] e + i
;       [207:17] src: operand
;       [207:17] allocate scratch register -> r14
        mov r14, qword [rbp + 320]
        add qword [r15], r14
;       [207:17] free scratch register r14
;       [207:13] e + 2
;       [207:13] src: folded constant '+ n'
        add qword [r15], 2
        foo.206.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.206.5
    foo.206.5.end:
;   [206:5] free scratch register r15
;   [209:5] assert(arr3[0] == 3 + 0 + 2)
;   [209:12] allocate scratch register -> r15
;   [209:12] ? arr3[0] == 3 + 0 + 2
;   [209:12] ? arr3[0] == 3 + 0 + 2
    cmp.209.12:
;   [209:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 296], 5
    sete r15b
    bool.209.12.end:
;   [32:6] assert(ok bool)
    func.assert.209.5:
;       [209:5] alias ok -> r15b
        if.32.27.209.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.209.5:
        cmp r15b, 0
        jne if.32.24.209.5.end
        if.32.27.209.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.209.5.end:
;       [209:5] free scratch register r15
    func.assert.209.5.end:
;   [210:5] assert(arr3[1] == 5 + 1 + 2)
;   [210:12] allocate scratch register -> r15
;   [210:12] ? arr3[1] == 5 + 1 + 2
;   [210:12] ? arr3[1] == 5 + 1 + 2
    cmp.210.12:
;   [210:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 304], 8
    sete r15b
    bool.210.12.end:
;   [32:6] assert(ok bool)
    func.assert.210.5:
;       [210:5] alias ok -> r15b
        if.32.27.210.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.210.5:
        cmp r15b, 0
        jne if.32.24.210.5.end
        if.32.27.210.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.210.5.end:
;       [210:5] free scratch register r15
    func.assert.210.5.end:
;   [216:5] var p point
;   [216:9] p: point (16 B @ [rbp + 312])
;   [216:9] zero 1 * 16 B = 16 B
;   [216:5] size <= 32 B, use mov
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
;   [219:7] p.fooz()
;   [46:6] point.fooz()
    func.point.fooz.219.7:
;       [219:7] alias self -> p
;       [47:5] self.x = 0b10
;       [47:14] 0b10
        mov qword [rbp + 312], 2
;       [48:5] self.y = 0xb
;       [48:14] 0xb
        mov qword [rbp + 320], 11
    func.point.fooz.219.7.end:
;   [222:5] assert(p.x == 2)
;   [222:12] allocate scratch register -> r15
;   [222:12] ? p.x == 2
;   [222:12] ? p.x == 2
    cmp.222.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.222.12.end:
;   [32:6] assert(ok bool)
    func.assert.222.5:
;       [222:5] alias ok -> r15b
        if.32.27.222.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.222.5:
        cmp r15b, 0
        jne if.32.24.222.5.end
        if.32.27.222.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.222.5.end:
;       [222:5] free scratch register r15
    func.assert.222.5.end:
;   [223:5] assert(p.y == 0xb)
;   [223:12] allocate scratch register -> r15
;   [223:12] ? p.y == 0xb
;   [223:12] ? p.y == 0xb
    cmp.223.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.223.12.end:
;   [32:6] assert(ok bool)
    func.assert.223.5:
;       [223:5] alias ok -> r15b
        if.32.27.223.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.223.5:
        cmp r15b, 0
        jne if.32.24.223.5.end
        if.32.27.223.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.223.5.end:
;       [223:5] free scratch register r15
    func.assert.223.5.end:
;   [225:5] var q point = p
;   [225:9] q: point (16 B @ [rbp + 328])
;   [225:9] q = p
;   [225:19] size <= 16 B, use mov
;   [225:19] allocate named register rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
;   [225:19] free named register rax
;   [228:5] assert(equal(p, q))
;   [228:12] allocate scratch register -> r15
;   [228:12] ? equal(p, q)
;   [228:12] ? shorthand: equal(p, q)
    cmp.228.12:
;       [228:12] equal(p, q)
;       [228:12] allocate named register rsi
;       [228:12] allocate named register rdi
;       [228:12] allocate named register rcx
;       [228:18] p
        lea rsi, [rbp + 312]
;       [228:21] q
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
;       [228:12] free named register rcx
;       [228:12] free named register rdi
;       [228:12] free named register rsi
        sete r15b
    bool.228.12.end:
;   [32:6] assert(ok bool)
    func.assert.228.5:
;       [228:5] alias ok -> r15b
        if.32.27.228.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.228.5:
        cmp r15b, 0
        jne if.32.24.228.5.end
        if.32.27.228.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.228.5.end:
;       [228:5] free scratch register r15
    func.assert.228.5.end:
;   [232:5] q.x = 3
;   [232:11] 3
    mov qword [rbp + 328], 3
;   [233:5] assert(not equal(p, q))
;   [233:12] allocate scratch register -> r15
;   [233:12] ? not equal(p, q)
;   [233:12] ? shorthand: not equal(p, q)
    cmp.233.12:
;       [233:16] equal(p, q)
;       [233:16] allocate named register rsi
;       [233:16] allocate named register rdi
;       [233:16] allocate named register rcx
;       [233:22] p
        lea rsi, [rbp + 312]
;       [233:25] q
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
;       [233:16] free named register rcx
;       [233:16] free named register rdi
;       [233:16] free named register rsi
        setne r15b
    bool.233.12.end:
;   [32:6] assert(ok bool)
    func.assert.233.5:
;       [233:5] alias ok -> r15b
        if.32.27.233.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.233.5:
        cmp r15b, 0
        jne if.32.24.233.5.end
        if.32.27.233.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.233.5.end:
;       [233:5] free scratch register r15
    func.assert.233.5.end:
;   [235:5] var i = 0
;   [235:9] i: i64 (8 B @ [rbp + 344])
;   [235:9] i = 0
;   [235:13] 0
    mov qword [rbp + 344], 0
;   [236:5] bar(i)
;   [54:6] bar(arg)
    func.bar.236.5:
;       [236:5] alias arg -> i
        if.55.8.236.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.236.5:
        cmp qword [rbp + 344], 0
        je func.bar.236.5.end
        if.55.8.236.5.code:
;           [55:17] return
        if.55.5.236.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.236.5.end:
;   [237:5] assert(i == 0)
;   [237:12] allocate scratch register -> r15
;   [237:12] ? i == 0
;   [237:12] ? i == 0
    cmp.237.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.237.12.end:
;   [32:6] assert(ok bool)
    func.assert.237.5:
;       [237:5] alias ok -> r15b
        if.32.27.237.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.237.5:
        cmp r15b, 0
        jne if.32.24.237.5.end
        if.32.27.237.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.237.5.end:
;       [237:5] free scratch register r15
    func.assert.237.5.end:
;   [239:5] i = 1
;   [239:9] 1
    mov qword [rbp + 344], 1
;   [240:5] bar(i)
;   [54:6] bar(arg)
    func.bar.240.5:
;       [240:5] alias arg -> i
        if.55.8.240.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.240.5:
        cmp qword [rbp + 344], 0
        je func.bar.240.5.end
        if.55.8.240.5.code:
;           [55:17] return
        if.55.5.240.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.240.5.end:
;   [241:5] assert(i == 0xff)
;   [241:12] allocate scratch register -> r15
;   [241:12] ? i == 0xff
;   [241:12] ? i == 0xff
    cmp.241.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.241.12.end:
;   [32:6] assert(ok bool)
    func.assert.241.5:
;       [241:5] alias ok -> r15b
        if.32.27.241.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.241.5:
        cmp r15b, 0
        jne if.32.24.241.5.end
        if.32.27.241.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.241.5.end:
;       [241:5] free scratch register r15
    func.assert.241.5.end:
;   [243:5] var j = 1
;   [243:9] j: i64 (8 B @ [rbp + 352])
;   [243:9] j = 1
;   [243:13] 1
    mov qword [rbp + 352], 1
;   [244:5] var k = baz(j)
;   [244:9] k: i64 (8 B @ [rbp + 360])
;   [244:9] k = baz(j)
;   [244:13] k = baz(j)
;   [244:13] = expression
;   [244:13] baz(j)
;   [66:6] baz(arg) res
    func.baz.244.13:
;       [244:13] alias res -> k
;       [244:13] alias arg -> j
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 3, with 3
;       [67:11] arg
;       [67:11] allocate scratch register -> r15
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
;       [67:11] free scratch register r15
;       [67:11] res * 2
;       [67:11] src: folded constant '* 2'
        sal qword [rbp + 360], 1
    func.baz.244.13.end:
;   [245:5] assert(k == 2)
;   [245:12] allocate scratch register -> r15
;   [245:12] ? k == 2
;   [245:12] ? k == 2
    cmp.245.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.245.12.end:
;   [32:6] assert(ok bool)
    func.assert.245.5:
;       [245:5] alias ok -> r15b
        if.32.27.245.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.245.5:
        cmp r15b, 0
        jne if.32.24.245.5.end
        if.32.27.245.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.245.5.end:
;       [245:5] free scratch register r15
    func.assert.245.5.end:
;   [247:5] k = baz(1)
;   [247:9] k = baz(1)
;   [247:9] = expression
;   [247:9] baz(1)
;   [66:6] baz(arg) res
    func.baz.247.9:
;       [247:9] alias res -> k
;       [247:9] alias arg -> 1
;       [67:5] res = arg * 2
;       [67:11] res = 2
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 360], 2
    func.baz.247.9.end:
;   [248:5] assert(k == 2)
;   [248:12] allocate scratch register -> r15
;   [248:12] ? k == 2
;   [248:12] ? k == 2
    cmp.248.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.248.12.end:
;   [32:6] assert(ok bool)
    func.assert.248.5:
;       [248:5] alias ok -> r15b
        if.32.27.248.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.248.5:
        cmp r15b, 0
        jne if.32.24.248.5.end
        if.32.27.248.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.248.5.end:
;       [248:5] free scratch register r15
    func.assert.248.5.end:
;   [250:5] var p0 point = {baz(3), 0}
;   [250:9] p0: point (16 B @ [rbp + 368])
;   [250:9] p0 = {baz(3), 0}
;   [250:21] copy field 'x'
;   [250:21] p0.x = baz(3)
;   [250:21] = expression
;   [250:21] baz(3)
;   [66:6] baz(arg) res
    func.baz.250.21:
;       [250:21] alias res -> p0.x (lea: rbp + 368)
;       [250:21] alias arg -> 3
;       [67:5] res = arg * 2
;       [67:11] res = 6
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 368], 6
    func.baz.250.21.end:
;   [250:29] copy field 'y'
    mov qword [rbp + 376], 0
;   [251:5] assert(p0.x == 6)
;   [251:12] allocate scratch register -> r15
;   [251:12] ? p0.x == 6
;   [251:12] ? p0.x == 6
    cmp.251.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.251.12.end:
;   [32:6] assert(ok bool)
    func.assert.251.5:
;       [251:5] alias ok -> r15b
        if.32.27.251.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.251.5:
        cmp r15b, 0
        jne if.32.24.251.5.end
        if.32.27.251.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.251.5.end:
;       [251:5] free scratch register r15
    func.assert.251.5.end:
;   [253:5] var pt = point.at(-1, -2)
;   [253:9] pt: point (16 B @ [rbp + 384])
;   [253:9] pt = point.at(-1, -2)
;   [253:14] point.at(-1, -2)
;   [90:6] point.at(x, y) self
    func.point.at.253.14:
;       [253:14] alias self -> pt
;       [253:14] alias x -> -1
;       [253:14] alias y -> -2
;       [91:5] self.x = x
;       [91:14] x
        mov qword [rbp + 384], -1
;       [92:5] self.y = y
;       [92:14] y
        mov qword [rbp + 392], -2
    func.point.at.253.14.end:
;   [257:5] assert(pt.x == -1)
;   [257:12] allocate scratch register -> r15
;   [257:12] ? pt.x == -1
;   [257:12] ? pt.x == -1
    cmp.257.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.257.12.end:
;   [32:6] assert(ok bool)
    func.assert.257.5:
;       [257:5] alias ok -> r15b
        if.32.27.257.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.257.5:
        cmp r15b, 0
        jne if.32.24.257.5.end
        if.32.27.257.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.257.5.end:
;       [257:5] free scratch register r15
    func.assert.257.5.end:
;   [258:5] assert(pt.y == -2)
;   [258:12] allocate scratch register -> r15
;   [258:12] ? pt.y == -2
;   [258:12] ? pt.y == -2
    cmp.258.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.258.12.end:
;   [32:6] assert(ok bool)
    func.assert.258.5:
;       [258:5] alias ok -> r15b
        if.32.27.258.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.258.5:
        cmp r15b, 0
        jne if.32.24.258.5.end
        if.32.27.258.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.258.5.end:
;       [258:5] free scratch register r15
    func.assert.258.5.end:
;   [260:5] var x = 1
;   [260:9] x: i64 (8 B @ [rbp + 400])
;   [260:9] x = 1
;   [260:13] 1
    mov qword [rbp + 400], 1
;   [261:5] var y = 2
;   [261:9] y: i64 (8 B @ [rbp + 408])
;   [261:9] y = 2
;   [261:13] 2
    mov qword [rbp + 408], 2
;   [263:5] var o1 object = {{x * 10, y}, 0xff0000}
;   [263:9] o1: object (24 B @ [rbp + 416])
;   [263:9] o1 = {{x * 10, y}, 0xff0000}
;   [263:22] copy field 'pos'
;   [263:23] copy field 'x'
;   [263:23] instructions without scratch register 5, with 3
;   [263:23] allocate scratch register -> r15
;   [263:23] x
    mov r15, qword [rbp + 400]
;   [263:23] r15 * 10
;   [263:23] src: folded constant '* 10'
    imul r15, 10
    mov qword [rbp + 416], r15
;   [263:23] free scratch register r15
;   [263:31] copy field 'y'
;   [263:31] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
;   [263:31] free scratch register r15
;   [263:35] copy field 'color'
    mov dword [rbp + 432], 16711680
;   [263:21] zero padding: 4 B
;   [263:21] size <= 32 B, use mov
    mov dword [rbp + 436], 0
;   [264:5] assert(o1.pos.x == 10)
;   [264:12] allocate scratch register -> r15
;   [264:12] ? o1.pos.x == 10
;   [264:12] ? o1.pos.x == 10
    cmp.264.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.264.12.end:
;   [32:6] assert(ok bool)
    func.assert.264.5:
;       [264:5] alias ok -> r15b
        if.32.27.264.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.264.5:
        cmp r15b, 0
        jne if.32.24.264.5.end
        if.32.27.264.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.264.5.end:
;       [264:5] free scratch register r15
    func.assert.264.5.end:
;   [265:5] assert(o1.pos.y == 2)
;   [265:12] allocate scratch register -> r15
;   [265:12] ? o1.pos.y == 2
;   [265:12] ? o1.pos.y == 2
    cmp.265.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.265.12.end:
;   [32:6] assert(ok bool)
    func.assert.265.5:
;       [265:5] alias ok -> r15b
        if.32.27.265.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.265.5:
        cmp r15b, 0
        jne if.32.24.265.5.end
        if.32.27.265.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.265.5.end:
;       [265:5] free scratch register r15
    func.assert.265.5.end:
;   [266:5] assert(o1.color == 0xff0000)
;   [266:12] allocate scratch register -> r15
;   [266:12] ? o1.color == 0xff0000
;   [266:12] ? o1.color == 0xff0000
    cmp.266.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.266.12.end:
;   [32:6] assert(ok bool)
    func.assert.266.5:
;       [266:5] alias ok -> r15b
        if.32.27.266.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.266.5:
        cmp r15b, 0
        jne if.32.24.266.5.end
        if.32.27.266.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.266.5.end:
;       [266:5] free scratch register r15
    func.assert.266.5.end:
;   [268:5] var p1 point = {-x, -y}
;   [268:9] p1: point (16 B @ [rbp + 440])
;   [268:9] p1 = {-x, -y}
;   [268:21] copy field 'x'
;   [268:21] instructions without scratch register 3, with 3
;   [268:21] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 440], r15
;   [268:21] free scratch register r15
    neg qword [rbp + 440]
;   [268:25] copy field 'y'
;   [268:25] instructions without scratch register 3, with 3
;   [268:25] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 448], r15
;   [268:25] free scratch register r15
    neg qword [rbp + 448]
;   [269:5] o1.pos = p1
;   [269:14] size <= 16 B, use mov
;   [269:14] allocate named register rax
    mov rax, qword [rbp + 440]
    mov qword [rbp + 416], rax
    mov rax, qword [rbp + 448]
    mov qword [rbp + 424], rax
;   [269:14] free named register rax
;   [270:5] assert(o1.pos.x == -1)
;   [270:12] allocate scratch register -> r15
;   [270:12] ? o1.pos.x == -1
;   [270:12] ? o1.pos.x == -1
    cmp.270.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.270.12.end:
;   [32:6] assert(ok bool)
    func.assert.270.5:
;       [270:5] alias ok -> r15b
        if.32.27.270.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.270.5:
        cmp r15b, 0
        jne if.32.24.270.5.end
        if.32.27.270.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.270.5.end:
;       [270:5] free scratch register r15
    func.assert.270.5.end:
;   [271:5] assert(o1.pos.y == -2)
;   [271:12] allocate scratch register -> r15
;   [271:12] ? o1.pos.y == -2
;   [271:12] ? o1.pos.y == -2
    cmp.271.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.271.12.end:
;   [32:6] assert(ok bool)
    func.assert.271.5:
;       [271:5] alias ok -> r15b
        if.32.27.271.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.271.5:
        cmp r15b, 0
        jne if.32.24.271.5.end
        if.32.27.271.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.271.5.end:
;       [271:5] free scratch register r15
    func.assert.271.5.end:
;   [273:5] var o2 object = o1
;   [273:9] o2: object (24 B @ [rbp + 456])
;   [273:9] o2 = o1
;   [273:21] allocate named register rsi
;   [273:21] allocate named register rdi
;   [273:21] allocate named register rcx
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
;   [273:21] free named register rcx
;   [273:21] free named register rdi
;   [273:21] free named register rsi
;   [274:5] assert(o2.pos.x == -1)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? o2.pos.x == -1
;   [274:12] ? o2.pos.x == -1
    cmp.274.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.274.12.end:
;   [32:6] assert(ok bool)
    func.assert.274.5:
;       [274:5] alias ok -> r15b
        if.32.27.274.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.274.5:
        cmp r15b, 0
        jne if.32.24.274.5.end
        if.32.27.274.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.274.5.end:
;       [274:5] free scratch register r15
    func.assert.274.5.end:
;   [275:5] assert(o2.pos.y == -2)
;   [275:12] allocate scratch register -> r15
;   [275:12] ? o2.pos.y == -2
;   [275:12] ? o2.pos.y == -2
    cmp.275.12:
    cmp qword [rbp + 464], -2
    sete r15b
    bool.275.12.end:
;   [32:6] assert(ok bool)
    func.assert.275.5:
;       [275:5] alias ok -> r15b
        if.32.27.275.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.275.5:
        cmp r15b, 0
        jne if.32.24.275.5.end
        if.32.27.275.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.275.5.end:
;       [275:5] free scratch register r15
    func.assert.275.5.end:
;   [276:5] assert(o2.color == 0xff0000)
;   [276:12] allocate scratch register -> r15
;   [276:12] ? o2.color == 0xff0000
;   [276:12] ? o2.color == 0xff0000
    cmp.276.12:
    cmp dword [rbp + 472], 16711680
    sete r15b
    bool.276.12.end:
;   [32:6] assert(ok bool)
    func.assert.276.5:
;       [276:5] alias ok -> r15b
        if.32.27.276.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.276.5:
        cmp r15b, 0
        jne if.32.24.276.5.end
        if.32.27.276.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.276.5.end:
;       [276:5] free scratch register r15
    func.assert.276.5.end:
;   [278:5] var o3[2] object
;   [278:9] o3: object[2] (48 B @ [rbp + 480])
;   [278:9] zero 2 * 24 B = 48 B
;   [278:5] allocate named register rax
;   [278:5] allocate named register rdi
;   [278:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
;   [278:5] free named register rcx
;   [278:5] free named register rdi
;   [278:5] free named register rax
;   [279:5] o3[0].pos.y = 73
;   [279:19] 73
    mov qword [rbp + 488], 73
;   [281:5] assert(o3[0].pos.y == 73)
;   [281:12] allocate scratch register -> r15
;   [281:12] ? o3[0].pos.y == 73
;   [281:12] ? o3[0].pos.y == 73
    cmp.281.12:
    cmp qword [rbp + 488], 73
    sete r15b
    bool.281.12.end:
;   [32:6] assert(ok bool)
    func.assert.281.5:
;       [281:5] alias ok -> r15b
        if.32.27.281.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.281.5:
        cmp r15b, 0
        jne if.32.24.281.5.end
        if.32.27.281.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.281.5.end:
;       [281:5] free scratch register r15
    func.assert.281.5.end:
;   [282:5] o3[1] = object.at(2, 74, 0xffffff)
;   [282:13] object.at(2, 74, 0xffffff)
;   [95:6] object.at(x, y, color i32) self
    func.object.at.282.13:
;       [282:13] alias self -> o3 (lea: rbp + 504)
;       [282:13] alias x -> 2
;       [282:13] alias y -> 74
;       [282:13] alias color -> 16777215
;       [96:5] self.pos = point.at(x, y)
;       [96:16] point.at(x, y)
;       [90:6] point.at(x, y) self
        func.point.at.96.16.282.13:
;           [96:16] alias self -> self.pos (lea: rbp + 504)
;           [96:16] alias x -> 2
;           [96:16] alias y -> 74
;           [91:5] self.x = x
;           [91:14] x
            mov qword [rbp + 504], 2
;           [92:5] self.y = y
;           [92:14] y
            mov qword [rbp + 512], 74
        func.point.at.96.16.282.13.end:
;       [97:5] self.color = color
;       [97:18] color
        mov dword [rbp + 520], 16777215
    func.object.at.282.13.end:
;   [283:5] assert(o3[1].pos.y == 74)
;   [283:12] allocate scratch register -> r15
;   [283:12] ? o3[1].pos.y == 74
;   [283:12] ? o3[1].pos.y == 74
    cmp.283.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.283.12.end:
;   [32:6] assert(ok bool)
    func.assert.283.5:
;       [283:5] alias ok -> r15b
        if.32.27.283.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.283.5:
        cmp r15b, 0
        jne if.32.24.283.5.end
        if.32.27.283.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.283.5.end:
;       [283:5] free scratch register r15
    func.assert.283.5.end:
;   [285:5] var worlds[8] world
;   [285:9] worlds: world[8] (512 B @ [rbp + 528])
;   [285:9] zero 8 * 64 B = 512 B
;   [285:5] allocate named register rax
;   [285:5] allocate named register rdi
;   [285:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
;   [285:5] free named register rcx
;   [285:5] free named register rdi
;   [285:5] free named register rax
;   [286:5] worlds[1].locations[1] = 0xffee
;   [286:30] 0xffee
    mov qword [rbp + 600], 65518
;   [287:5] assert(worlds[1].locations[1] == 0xffee)
;   [287:12] allocate scratch register -> r15
;   [287:12] ? worlds[1].locations[1] == 0xffee
;   [287:12] ? worlds[1].locations[1] == 0xffee
    cmp.287.12:
    cmp qword [rbp + 600], 65518
    sete r15b
    bool.287.12.end:
;   [32:6] assert(ok bool)
    func.assert.287.5:
;       [287:5] alias ok -> r15b
        if.32.27.287.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.287.5:
        cmp r15b, 0
        jne if.32.24.287.5.end
        if.32.27.287.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.287.5.end:
;       [287:5] free scratch register r15
    func.assert.287.5.end:
;   [289:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [289:5] allocate named register rsi
;   [289:5] allocate named register rdi
;   [289:5] allocate named register rcx
;   [292:9] array_length(worlds[0].locations)
;   [292:9] rcx = array_length(worlds[0].locations)
;   [292:9] = expression
;   [292:9] array_length(worlds[0].locations)
    mov rcx, 8
;   [290:9] worlds[1].locations
;   [290:9] bounds check
;   [290:9] lower bound (--checks=lower)
    test rcx, rcx
    js baz_bounds_panic
;   [290:9] upper bound (--checks=upper)
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 592]
;   [291:9] worlds[0].locations
;   [291:9] bounds check
;   [291:9] lower bound (--checks=lower)
    test rcx, rcx
    js baz_bounds_panic
;   [291:9] upper bound (--checks=upper)
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 528]
    shl rcx, 3
    rep movsb
;   [289:5] free named register rcx
;   [289:5] free named register rdi
;   [289:5] free named register rsi
;   [296:5] assert(worlds[0].locations[1] == 0xffee)
;   [296:12] allocate scratch register -> r15
;   [296:12] ? worlds[0].locations[1] == 0xffee
;   [296:12] ? worlds[0].locations[1] == 0xffee
    cmp.296.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.296.12.end:
;   [32:6] assert(ok bool)
    func.assert.296.5:
;       [296:5] alias ok -> r15b
        if.32.27.296.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.296.5:
        cmp r15b, 0
        jne if.32.24.296.5.end
        if.32.27.296.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.296.5.end:
;       [296:5] free scratch register r15
    func.assert.296.5.end:
;   [297:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [297:12] allocate scratch register -> r15
;   [297:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [297:12] ? shorthand: arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
    cmp.297.12:
;       [297:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;       [297:12] allocate named register rsi
;       [297:12] allocate named register rdi
;       [297:12] allocate named register rcx
;       [300:14] array_length(worlds[0].locations)
;       [300:14] rcx = array_length(worlds[0].locations)
;       [300:14] = expression
;       [300:14] array_length(worlds[0].locations)
        mov rcx, 8
;       [298:14] worlds[0].locations
;       [298:14] bounds check
;       [298:14] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [298:14] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 528]
;       [299:14] worlds[1].locations
;       [299:14] bounds check
;       [299:14] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [299:14] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 592]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [297:12] free named register rcx
;       [297:12] free named register rdi
;       [297:12] free named register rsi
        sete r15b
    bool.297.12.end:
;   [32:6] assert(ok bool)
    func.assert.297.5:
;       [297:5] alias ok -> r15b
        if.32.27.297.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.297.5:
        cmp r15b, 0
        jne if.32.24.297.5.end
        if.32.27.297.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.297.5.end:
;       [297:5] free scratch register r15
    func.assert.297.5.end:
;   [302:5] var arr2[] = { -1, 2 }
;   [302:9] arr2: i64[2] (16 B @ [rbp + 1040])
;   [302:9] arr2= { -1, 2 }
;   [302:18] size <= 16 B, use immediates
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
;   [303:5] assert(array_length(arr2) == 2)
;   [303:12] allocate scratch register -> r15
;   [303:12] ? array_length(arr2) == 2
;   [303:12] ? array_length(arr2) == 2
    cmp.303.12:
;   [303:12] allocate scratch register -> r14
;       [303:12] r14 = array_length(arr2)
;       [303:12] = expression
;       [303:12] array_length(arr2)
        mov r14, 2
    cmp r14, 2
;   [303:12] free scratch register r14
    sete r15b
    bool.303.12.end:
;   [32:6] assert(ok bool)
    func.assert.303.5:
;       [303:5] alias ok -> r15b
        if.32.27.303.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.303.5:
        cmp r15b, 0
        jne if.32.24.303.5.end
        if.32.27.303.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.303.5.end:
;       [303:5] free scratch register r15
    func.assert.303.5.end:
;   [304:5] assert(arr2[0] == -1)
;   [304:12] allocate scratch register -> r15
;   [304:12] ? arr2[0] == -1
;   [304:12] ? arr2[0] == -1
    cmp.304.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.304.12.end:
;   [32:6] assert(ok bool)
    func.assert.304.5:
;       [304:5] alias ok -> r15b
        if.32.27.304.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.304.5:
        cmp r15b, 0
        jne if.32.24.304.5.end
        if.32.27.304.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.304.5.end:
;       [304:5] free scratch register r15
    func.assert.304.5.end:
;   [305:5] assert(arr2[1] == 2)
;   [305:12] allocate scratch register -> r15
;   [305:12] ? arr2[1] == 2
;   [305:12] ? arr2[1] == 2
    cmp.305.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.305.12.end:
;   [32:6] assert(ok bool)
    func.assert.305.5:
;       [305:5] alias ok -> r15b
        if.32.27.305.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.305.5:
        cmp r15b, 0
        jne if.32.24.305.5.end
        if.32.27.305.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.305.5.end:
;       [305:5] free scratch register r15
    func.assert.305.5.end:
;   [307:5] var counter
;   [307:9] counter: i64 (8 B @ [rbp + 1056])
;   [307:9] zero 1 * 8 B = 8 B
;   [307:5] size <= 32 B, use mov
    mov qword [rbp + 1056], 0
;   [308:5] var nm str
;   [308:9] nm: str (128 B @ [rbp + 1064])
;   [308:9] zero 1 * 128 B = 128 B
;   [308:5] allocate named register rax
;   [308:5] allocate named register rdi
;   [308:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
;   [308:5] free named register rcx
;   [308:5] free named register rdi
;   [308:5] free named register rax
;   [309:5] print(hello)
;   [35:6] print(str[] i8)
    func.print.309.5:
;       [309:5] alias str -> hello
;       [36:5] write(1, str)
;       [36:5] allocate named register rdi
;       [36:5] allocate named register rsi
;       [36:5] allocate named register rdx
;       [36:11] 1
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
;       [36:5] allocate named register rax
        mov rax, 1
        syscall
;       [36:5] free named register rax
;       [36:5] free named register rdx
;       [36:5] free named register rsi
;       [36:5] free named register rdi
    func.print.309.5.end:
;   [310:5] label
    loop.310.5:
;       [311:9] counter = counter + 1
;       [311:19] instructions without scratch register 1, with 3
;       [311:19] counter
;       [311:19] counter + 1
;       [311:19] src: folded constant '+ 1'
        add qword [rbp + 1056], 1
;       [312:9] print_num(counter)
;       [312:9] frame capacity check (--checks=frame)
;       [312:9] allocate scratch register -> r15
;       [312:9] allocate scratch register -> r14
        lea r15, [rbp + 1192]
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
;       [312:9] free scratch register r14
;       [312:9] free scratch register r15
;       [312:9] address of argument 'counter' to parameter 'num'
;       [312:9] allocate scratch register -> r15
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
;       [312:9] free scratch register r15
;       [312:9] set function frame base
        lea rbx, [rbp + 1192]
        call func.print_num
;       [313:9] print(colon)
;       [35:6] print(str[] i8)
        func.print.313.9:
;           [313:9] alias str -> colon
;           [36:5] write(1, str)
;           [36:5] allocate named register rdi
;           [36:5] allocate named register rsi
;           [36:5] allocate named register rdx
;           [36:11] 1
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
;           [36:5] allocate named register rax
            mov rax, 1
            syscall
;           [36:5] free named register rax
;           [36:5] free named register rdx
;           [36:5] free named register rsi
;           [36:5] free named register rdi
        func.print.313.9.end:
;       [314:9] print(prompt1)
;       [35:6] print(str[] i8)
        func.print.314.9:
;           [314:9] alias str -> prompt1
;           [36:5] write(1, str)
;           [36:5] allocate named register rdi
;           [36:5] allocate named register rsi
;           [36:5] allocate named register rdx
;           [36:11] 1
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
;           [36:5] allocate named register rax
            mov rax, 1
            syscall
;           [36:5] free named register rax
;           [36:5] free named register rdx
;           [36:5] free named register rsi
;           [36:5] free named register rdi
        func.print.314.9.end:
;       [315:12] nm.input()
;       [76:6] str.input()
        func.str.input.315.12:
;           [315:12] alias self -> nm
;           [77:5] var nbytes = read(0, self.data)
;           [77:9] nbytes: i64 (8 B @ [rbp + 1192])
;           [77:9] nbytes = read(0, self.data)
;           [77:18] nbytes = read(0, self.data)
;           [77:18] = expression
;           [77:18] read(0, self.data)
;           [77:18] allocate named register rdi
;           [77:18] allocate named register rsi
;           [77:18] allocate named register rdx
;           [77:23] 0
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
;           [77:18] allocate named register rax
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
;           [77:18] free named register rax
;           [77:18] free named register rdx
;           [77:18] free named register rsi
;           [77:18] free named register rdi
;           [80:5] self.len = i8(nbytes - 1)
;           [80:16] self.len = i8(nbytes - 1)
;           [80:16] = expression
;           [80:16] instructions without scratch register 3, with 3
;           [80:19] instructions without scratch register 3, with 3
;           [80:19] nbytes
;           [80:19] allocate scratch register -> r15
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
;           [80:19] free scratch register r15
;           [80:19] self.len - 1
;           [80:19] src: folded constant '- 1'
            sub byte [rbp + 1064], 1
        func.str.input.315.12.end:
        if.317.12:
;       [317:12] ? nm.len <= 0
;       [317:12] ? nm.len <= 0
        cmp.317.12:
        cmp byte [rbp + 1064], 0
        jle loop.310.5.end
        if.317.12.code:
;           [318:13] break
        if.319.19:
;       [319:19] ? nm.len <= 4
;       [319:19] ? nm.len <= 4
        cmp.319.19:
        cmp byte [rbp + 1064], 4
        jg if.317.9.else
        if.319.19.code:
;           [320:13] print(prompt2)
;           [35:6] print(str[] i8)
            func.print.320.13:
;               [320:13] alias str -> prompt2
;               [36:5] write(1, str)
;               [36:5] allocate named register rdi
;               [36:5] allocate named register rsi
;               [36:5] allocate named register rdx
;               [36:11] 1
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
            func.print.320.13.end:
;           [321:13] continue
            jmp loop.310.5
        if.317.9.else:
;           [323:13] print(prompt3)
;           [35:6] print(str[] i8)
            func.print.323.13:
;               [323:13] alias str -> prompt3
;               [36:5] write(1, str)
;               [36:5] allocate named register rdi
;               [36:5] allocate named register rsi
;               [36:5] allocate named register rdx
;               [36:11] 1
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
            func.print.323.13.end:
;           [324:16] nm.output()
;           [83:6] str.output()
            func.str.output.324.16:
;               [324:16] alias self -> nm
;               [84:5] write(1, self.data, self.len)
;               [84:5] allocate named register rdi
;               [84:5] allocate named register rsi
;               [84:5] allocate named register rdx
;               [84:11] 1
                mov rdi, 1
;               [84:25] self.len
                movsx rdx, byte [rbp + 1064]
;               [84:14] bounds check
;               [84:14] lower bound (--checks=lower)
                test rdx, rdx
                js baz_bounds_panic
;               [84:14] upper bound (--checks=upper)
                cmp rdx, 127
                jg baz_bounds_panic
                lea rsi, [rbp + 1065]
;               [84:5] allocate named register rax
                mov rax, 1
                syscall
;               [84:5] free named register rax
;               [84:5] free named register rdx
;               [84:5] free named register rsi
;               [84:5] free named register rdi
            func.str.output.324.16.end:
;           [325:13] print(dot)
;           [35:6] print(str[] i8)
            func.print.325.13:
;               [325:13] alias str -> dot
;               [36:5] write(1, str)
;               [36:5] allocate named register rdi
;               [36:5] allocate named register rsi
;               [36:5] allocate named register rdx
;               [36:11] 1
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
            func.print.325.13.end:
;           [326:13] print(nl)
;           [35:6] print(str[] i8)
            func.print.326.13:
;               [326:13] alias str -> nl
;               [36:5] write(1, str)
;               [36:5] allocate named register rdi
;               [36:5] allocate named register rsi
;               [36:5] allocate named register rdx
;               [36:11] 1
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
;               [36:5] allocate named register rax
                mov rax, 1
                syscall
;               [36:5] free named register rax
;               [36:5] free named register rdx
;               [36:5] free named register rsi
;               [36:5] free named register rdi
            func.print.326.13.end:
        if.317.9.end:
    jmp loop.310.5
    loop.310.5.end:
    mov rdi, 0
    mov rax, 60
    syscall

;
;[110:15] noinline print_num(num)
func.print_num:
;   [110:25] num: i64 (8 B @ [rbx])
;   [112:11] const buf_count = 20
;   [114:5] var buf[buf_count] i8
;   [114:9] buf: i8[20] (20 B @ [rbx + 8])
;   [114:9] zero 20 * 1 B = 20 B
;   [114:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [115:5] var n = num
;   [115:9] n: i64 (8 B @ [rbx + 32])
;   [115:9] n = num
;   [115:13] num
;   [115:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [115:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [115:13] free scratch register r14
;   [115:13] free scratch register r15
;   [116:5] var is_negative bool
;   [116:9] is_negative: bool (1 B @ [rbx + 40])
;   [116:9] zero 1 * 1 B = 1 B
;   [116:5] size <= 32 B, use mov
    mov byte [rbx + 40], 0
    if.120.8:
;   [120:8] ? n < 0
;   [120:8] ? n < 0
    cmp.120.8:
    cmp qword [rbx + 32], 0
    jge if.120.5.end
    if.120.8.code:
;       [121:9] is_negative = true
        mov byte [rbx + 40], 1
    if.120.5.end:
    if.123.8:
;   [123:8] ? n > 0
;   [123:8] ? n > 0
    cmp.123.8:
    cmp qword [rbx + 32], 0
    jle if.123.5.end
    if.123.8.code:
;       [124:9] n = -n
;       [124:13] instructions without scratch register 1, with 3
;       [124:14] -n
        neg qword [rbx + 32]
    if.123.5.end:
;   [127:5] var i = buf_count
;   [127:9] i: i64 (8 B @ [rbx + 48])
;   [127:9] i = buf_count
;   [127:13] buf_count
    mov qword [rbx + 48], 20
;   [128:5] label
    loop.128.5:
;       [129:9] i = i - 1
;       [129:13] instructions without scratch register 1, with 3
;       [129:13] i
;       [129:13] i - 1
;       [129:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
;       [130:9] buf[i] = i8('0' - n % 10)
;       [130:13] allocate scratch register -> r15
;       [130:13] set array index
;       [130:13] i
        mov r15, qword [rbx + 48]
;       [130:13] bounds check
;       [130:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [130:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [130:18] buf = i8('0' - n % 10)
;       [130:18] = expression
;       [130:18] allocate scratch register -> r14
;           [130:21] r14 = 48
;           [130:21] src: folded constant '+ '0''
            mov r14, 48
;           [130:29] r14 - n % 10
;           [130:29] src: expression
;           [130:29] allocate scratch register -> r13
;           [130:27] n
            mov r13, qword [rbx + 32]
;           [130:31] r13 % 10
;           [130:31] src: constant
;           [130:31] allocate named register rax
            mov rax, r13
;           [130:31] allocate named register rdx
            cqo
;           [130:31] allocate scratch register -> r12
            mov r12, 10
            idiv r12
;           [130:31] free scratch register r12
            mov r13, rdx
;           [130:31] free named register rdx
;           [130:31] free named register rax
            sub r14, r13
;           [130:29] free scratch register r13
        mov byte [rbx + r15 + 8], r14b
;       [130:18] free scratch register r14
;       [130:9] free scratch register r15
;       [131:9] n = n / 10
;       [131:13] instructions without scratch register 5, with 7
;       [131:13] n
;       [131:17] n / 10
;       [131:17] src: constant
;       [131:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [131:17] allocate named register rdx
        cqo
;       [131:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [131:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [131:17] free named register rdx
;       [131:17] free named register rax
        if.132.12:
;       [132:12] ? n == 0
;       [132:12] ? n == 0
        cmp.132.12:
        cmp qword [rbx + 32], 0
        jne loop.128.5
        if.132.12.code:
;           [132:19] break
        if.132.9.end:
    loop.128.5.end:
    if.135.8:
;   [135:8] ? is_negative
;   [135:8] ? shorthand: is_negative
    cmp.135.8:
    cmp byte [rbx + 40], 0
    je if.135.5.end
    if.135.8.code:
;       [136:9] i = i - 1
;       [136:13] instructions without scratch register 1, with 3
;       [136:13] i
;       [136:13] i - 1
;       [136:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
;       [137:9] buf[i] = '-'
;       [137:13] allocate scratch register -> r15
;       [137:13] set array index
;       [137:13] i
        mov r15, qword [rbx + 48]
;       [137:13] bounds check
;       [137:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [137:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [137:18] '-'
        mov byte [rbx + r15 + 8], 45
;       [137:9] free scratch register r15
    if.135.5.end:
;   [140:5] var write_pos
;   [140:9] write_pos: i64 (8 B @ [rbx + 56])
;   [140:9] zero 1 * 8 B = 8 B
;   [140:5] size <= 32 B, use mov
    mov qword [rbx + 56], 0
;   [141:5] label
    loop.141.5:
;       [142:9] buf[write_pos] = buf[i]
;       [142:13] allocate scratch register -> r15
;       [142:13] set array index
;       [142:13] write_pos
        mov r15, qword [rbx + 56]
;       [142:13] bounds check
;       [142:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [142:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [142:26] buf[i]
;       [142:30] allocate scratch register -> r14
;       [142:30] set array index
;       [142:30] i
        mov r14, qword [rbx + 48]
;       [142:30] bounds check
;       [142:30] lower bound (--checks=lower)
        test r14, r14
        js baz_bounds_panic
;       [142:30] upper bound (--checks=upper)
        cmp r14, 20
        jge baz_bounds_panic
;       [142:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [142:26] free scratch register r13
;       [142:26] free scratch register r14
;       [142:9] free scratch register r15
;       [143:9] write_pos = write_pos + 1
;       [143:21] instructions without scratch register 1, with 3
;       [143:21] write_pos
;       [143:21] write_pos + 1
;       [143:21] src: folded constant '+ 1'
        add qword [rbx + 56], 1
;       [144:9] i = i + 1
;       [144:13] instructions without scratch register 1, with 3
;       [144:13] i
;       [144:13] i + 1
;       [144:13] src: folded constant '+ 1'
        add qword [rbx + 48], 1
        if.145.12:
;       [145:12] ? i == buf_count
;       [145:12] ? i == buf_count
        cmp.145.12:
        cmp qword [rbx + 48], 20
        jne loop.141.5
        if.145.12.code:
;           [145:27] break
        if.145.9.end:
    loop.141.5.end:
;   [148:5] write(1, buf, write_pos)
;   [148:5] allocate named register rdi
;   [148:5] allocate named register rsi
;   [148:5] allocate named register rdx
;   [148:11] 1
    mov rdi, 1
;   [148:19] write_pos
    mov rdx, qword [rbx + 56]
;   [148:14] bounds check
;   [148:14] lower bound (--checks=lower)
    test rdx, rdx
    js baz_bounds_panic
;   [148:14] upper bound (--checks=upper)
    cmp rdx, 20
    jg baz_bounds_panic
    lea rsi, [rbx + 8]
;   [148:5] allocate named register rax
    mov rax, 1
    syscall
;   [148:5] free named register rax
;   [148:5] free named register rdx
;   [148:5] free named register rsi
;   [148:5] free named register rdi
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
baz_bounds_panic:
;    exit with error code 255
    mov rax, 60
    mov rdi, 255
    syscall

section .data
align 16
dat:
;[20:7] hello
;[20:20] i8[21]
db `hello world from baz\n`
;[21:5] prompt1
;[21:20] i8[12]
db `enter name:\n`
;[22:5] prompt2
;[22:20] i8[20]
db `that is not a name.\n`
;[23:5] prompt3
;[23:20] i8[6]
db `hello `
;[24:9] dot
;[24:20] i8[1]
db `.`
;[25:10] nl
;[25:20] i8[1]
db `\n`
;[26:7] colon
;[26:20] i8[2]
db `: `
; padding 1 B
times 1 db 0
;[27:8] nums
; i64[4]
;[27:20] [0]
;[27:20] i64
dq 1
; pad 3 'i64' of size 8
times 24 db 0
;[28:8] str1
;[28:21] i8
db 3
;[28:19] zero remaining fields: 127 B
times 127 db 0
dat.end:

section .bss.vars nobits alloc write
align 16
vars:
resb 131072
vars.end:
; free named register rbp

;   removed jumps to next code: 88
;    removed unreachable jumps: 2
; removed same target branches: 33
; inverted branches over jumps: 7

; max scratch registers in use: 4
;            max frames in use: 8
;              dat var padding: 0 B
;                max vars size: 976 B
```
