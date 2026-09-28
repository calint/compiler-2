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

    ix = 3
    arr[ix] = ~inv(arr[ix - 1])
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
    mov r14, 174
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 232], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 175
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
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
    mov r13, 179
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    mov r12, r15
    add r12, r14
    cmp r12, 4
    cmovg rbp, r13
    jg baz_bounds_panic
    mov r13, 179
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
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
    mov r14, 184
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    mov r14, 184
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    mov rax, qword [rbp + 232]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 240]
    mov qword [rbp + 264], rax
    cmp.185.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 185
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 232]
        mov r15, 1
        mov r14, 185
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 8
        cmovg rbp, r14
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
        mov r14, 191
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
        mov r14, 191
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
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
    mov r14, 194
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 194
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.194.16:
        mov r13d, dword [rbp + r14 * 4 + 232]
        mov dword [rbp + r15 * 4 + 232], r13d
        not dword [rbp + r15 * 4 + 232]
    func.inv.194.16.end:
    not dword [rbp + r15 * 4 + 232]
    cmp.195.12:
    mov r14, qword [rbp + 248]
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.195.12.end:
    func.assert.195.5:
        if.32.27.195.5:
        cmp.32.27.195.5:
        cmp r15b, 0
        jne if.32.24.195.5.end
        if.32.27.195.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.195.5.end:
    func.assert.195.5.end:
    func.faz.197.5:
        mov dword [rbp + 236], 254
    func.faz.197.5.end:
    cmp.198.12:
    cmp dword [rbp + 236], 254
    sete r15b
    bool.198.12.end:
    func.assert.198.5:
        if.32.27.198.5:
        cmp.32.27.198.5:
        cmp r15b, 0
        jne if.32.24.198.5.end
        if.32.27.198.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.198.5.end:
    func.assert.198.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.201.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.201.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.201.5
    foo.201.5.end:
    cmp.204.12:
    cmp qword [rbp + 296], 5
    sete r15b
    bool.204.12.end:
    func.assert.204.5:
        if.32.27.204.5:
        cmp.32.27.204.5:
        cmp r15b, 0
        jne if.32.24.204.5.end
        if.32.27.204.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.204.5.end:
    func.assert.204.5.end:
    cmp.205.12:
    cmp qword [rbp + 304], 8
    sete r15b
    bool.205.12.end:
    func.assert.205.5:
        if.32.27.205.5:
        cmp.32.27.205.5:
        cmp r15b, 0
        jne if.32.24.205.5.end
        if.32.27.205.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.205.5.end:
    func.assert.205.5.end:
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.214.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.214.7.end:
    cmp.217.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.217.12.end:
    func.assert.217.5:
        if.32.27.217.5:
        cmp.32.27.217.5:
        cmp r15b, 0
        jne if.32.24.217.5.end
        if.32.27.217.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.217.5.end:
    func.assert.217.5.end:
    cmp.218.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.218.12.end:
    func.assert.218.5:
        if.32.27.218.5:
        cmp.32.27.218.5:
        cmp r15b, 0
        jne if.32.24.218.5.end
        if.32.27.218.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.218.5.end:
    func.assert.218.5.end:
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.223.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
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
    mov qword [rbp + 328], 3
    cmp.228.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        setne r15b
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
    mov qword [rbp + 344], 0
    func.bar.231.5:
        if.55.8.231.5:
        cmp.55.8.231.5:
        cmp qword [rbp + 344], 0
        je func.bar.231.5.end
        if.55.8.231.5.code:
        if.55.5.231.5.end:
        mov qword [rbp + 344], 255
    func.bar.231.5.end:
    cmp.232.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.232.12.end:
    func.assert.232.5:
        if.32.27.232.5:
        cmp.32.27.232.5:
        cmp r15b, 0
        jne if.32.24.232.5.end
        if.32.27.232.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.232.5.end:
    func.assert.232.5.end:
    mov qword [rbp + 344], 1
    func.bar.235.5:
        if.55.8.235.5:
        cmp.55.8.235.5:
        cmp qword [rbp + 344], 0
        je func.bar.235.5.end
        if.55.8.235.5.code:
        if.55.5.235.5.end:
        mov qword [rbp + 344], 255
    func.bar.235.5.end:
    cmp.236.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.236.12.end:
    func.assert.236.5:
        if.32.27.236.5:
        cmp.32.27.236.5:
        cmp r15b, 0
        jne if.32.24.236.5.end
        if.32.27.236.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.236.5.end:
    func.assert.236.5.end:
    mov qword [rbp + 352], 1
    func.baz.239.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.239.13.end:
    cmp.240.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.240.12.end:
    func.assert.240.5:
        if.32.27.240.5:
        cmp.32.27.240.5:
        cmp r15b, 0
        jne if.32.24.240.5.end
        if.32.27.240.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.240.5.end:
    func.assert.240.5.end:
    func.baz.242.9:
        mov qword [rbp + 360], 2
    func.baz.242.9.end:
    cmp.243.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.243.12.end:
    func.assert.243.5:
        if.32.27.243.5:
        cmp.32.27.243.5:
        cmp r15b, 0
        jne if.32.24.243.5.end
        if.32.27.243.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.243.5.end:
    func.assert.243.5.end:
    func.baz.245.21:
        mov qword [rbp + 368], 6
    func.baz.245.21.end:
    mov qword [rbp + 376], 0
    cmp.246.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.246.12.end:
    func.assert.246.5:
        if.32.27.246.5:
        cmp.32.27.246.5:
        cmp r15b, 0
        jne if.32.24.246.5.end
        if.32.27.246.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.246.5.end:
    func.assert.246.5.end:
    func.point.at.248.14:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point.at.248.14.end:
    cmp.252.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.252.12.end:
    func.assert.252.5:
        if.32.27.252.5:
        cmp.32.27.252.5:
        cmp r15b, 0
        jne if.32.24.252.5.end
        if.32.27.252.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.252.5.end:
    func.assert.252.5.end:
    cmp.253.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.253.12.end:
    func.assert.253.5:
        if.32.27.253.5:
        cmp.32.27.253.5:
        cmp r15b, 0
        jne if.32.24.253.5.end
        if.32.27.253.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.253.5.end:
    func.assert.253.5.end:
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.259.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.259.12.end:
    func.assert.259.5:
        if.32.27.259.5:
        cmp.32.27.259.5:
        cmp r15b, 0
        jne if.32.24.259.5.end
        if.32.27.259.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.259.5.end:
    func.assert.259.5.end:
    cmp.260.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.260.12.end:
    func.assert.260.5:
        if.32.27.260.5:
        cmp.32.27.260.5:
        cmp r15b, 0
        jne if.32.24.260.5.end
        if.32.27.260.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.260.5.end:
    func.assert.260.5.end:
    cmp.261.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.261.12.end:
    func.assert.261.5:
        if.32.27.261.5:
        cmp.32.27.261.5:
        cmp r15b, 0
        jne if.32.24.261.5.end
        if.32.27.261.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.261.5.end:
    func.assert.261.5.end:
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
    cmp.265.12:
    cmp qword [rbp + 416], -1
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
    cmp qword [rbp + 424], -2
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
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.269.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.269.12.end:
    func.assert.269.5:
        if.32.27.269.5:
        cmp.32.27.269.5:
        cmp r15b, 0
        jne if.32.24.269.5.end
        if.32.27.269.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.269.5.end:
    func.assert.269.5.end:
    cmp.270.12:
    cmp qword [rbp + 464], -2
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
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.276.12:
    cmp qword [rbp + 488], 73
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
    func.object.at.277.13:
        func.point.at.96.16.277.13:
            mov qword [rbp + 504], 2
            mov qword [rbp + 512], 74
        func.point.at.96.16.277.13.end:
        mov dword [rbp + 520], 16777215
    func.object.at.277.13.end:
    cmp.278.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.278.12.end:
    func.assert.278.5:
        if.32.27.278.5:
        cmp.32.27.278.5:
        cmp r15b, 0
        jne if.32.24.278.5.end
        if.32.27.278.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.278.5.end:
    func.assert.278.5.end:
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 600], 65518
    cmp.282.12:
    cmp qword [rbp + 600], 65518
    sete r15b
    bool.282.12.end:
    func.assert.282.5:
        if.32.27.282.5:
        cmp.32.27.282.5:
        cmp r15b, 0
        jne if.32.24.282.5.end
        if.32.27.282.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.282.5.end:
    func.assert.282.5.end:
    mov rcx, 8
    mov r15, 285
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 592]
    mov r15, 286
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 528]
    shl rcx, 3
    rep movsb
    cmp.291.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.291.12.end:
    func.assert.291.5:
        if.32.27.291.5:
        cmp.32.27.291.5:
        cmp r15b, 0
        jne if.32.24.291.5.end
        if.32.27.291.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.291.5.end:
    func.assert.291.5.end:
    cmp.292.12:
        mov rcx, 8
        mov r14, 293
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 528]
        mov r14, 294
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + 592]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.292.12.end:
    func.assert.292.5:
        if.32.27.292.5:
        cmp.32.27.292.5:
        cmp r15b, 0
        jne if.32.24.292.5.end
        if.32.27.292.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.292.5.end:
    func.assert.292.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.298.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.298.12.end:
    func.assert.298.5:
        if.32.27.298.5:
        cmp.32.27.298.5:
        cmp r15b, 0
        jne if.32.24.298.5.end
        if.32.27.298.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.298.5.end:
    func.assert.298.5.end:
    cmp.299.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.299.12.end:
    func.assert.299.5:
        if.32.27.299.5:
        cmp.32.27.299.5:
        cmp r15b, 0
        jne if.32.24.299.5.end
        if.32.27.299.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.299.5.end:
    func.assert.299.5.end:
    cmp.300.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.300.12.end:
    func.assert.300.5:
        if.32.27.300.5:
        cmp.32.27.300.5:
        cmp r15b, 0
        jne if.32.24.300.5.end
        if.32.27.300.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.300.5.end:
    func.assert.300.5.end:
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.304.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.304.5.end:
    loop.305.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        lea rbx, [rbp + 1192]
        call func.print_num
        func.print.308.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.308.9.end:
        func.print.309.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.309.9.end:
        func.str.input.310.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.310.12.end:
        if.312.12:
        cmp.312.12:
        cmp byte [rbp + 1064], 0
        jle loop.305.5.end
        if.312.12.code:
        if.314.19:
        cmp.314.19:
        cmp byte [rbp + 1064], 4
        jg if.312.9.else
        if.314.19.code:
            func.print.315.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.315.13.end:
            jmp loop.305.5
        if.312.9.else:
            func.print.318.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.318.13.end:
            func.str.output.319.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 1064]
                mov r15, 84
                test rdx, rdx
                cmovs rbp, r15
                js baz_bounds_panic
                cmp rdx, 127
                cmovg rbp, r15
                jg baz_bounds_panic
                lea rsi, [rbp + 1065]
                mov rax, 1
                syscall
            func.str.output.319.16.end:
            func.print.320.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.320.13.end:
            func.print.321.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.321.13.end:
        if.312.9.end:
    jmp loop.305.5
    loop.305.5.end:
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
        mov r14, 130
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
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
        mov r14, 137
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.135.5.end:
    mov qword [rbx + 56], 0
    loop.141.5:
        mov r15, qword [rbx + 56]
        mov r14, 142
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        mov r13, 142
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
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
    mov r15, 148
    test rdx, rdx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rdx, 20
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
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
;   [174:9] allocate scratch register -> r14
;   [174:9] line number (--checks=line)
    mov r14, 174
;   [174:9] lower bound (--checks=lower)
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [174:9] upper bound (--checks=upper)
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [174:9] free scratch register r14
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
;   [175:9] allocate scratch register -> r14
;   [175:9] line number (--checks=line)
    mov r14, 175
;   [175:9] lower bound (--checks=lower)
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [175:9] upper bound (--checks=upper)
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [175:9] free scratch register r14
;   [175:19] arr[ix]
;   [175:23] allocate scratch register -> r14
;   [175:23] set array index
;   [175:23] ix
    mov r14, qword [rbp + 248]
;   [175:23] bounds check
;   [175:23] allocate scratch register -> r13
;   [175:23] line number (--checks=line)
    mov r13, 175
;   [175:23] lower bound (--checks=lower)
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
;   [175:23] upper bound (--checks=upper)
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [175:23] free scratch register r13
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
;   [179:20] allocate scratch register -> r13
;   [179:20] line number (--checks=line)
    mov r13, 179
;   [179:20] lower bound (--checks=lower)
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
;   [179:20] upper bound (--checks=upper)
;   [179:20] allocate scratch register -> r12
    mov r12, r15
    add r12, r14
    cmp r12, 4
;   [179:20] free scratch register r12
    cmovg rbp, r13
    jg baz_bounds_panic
;   [179:20] free scratch register r13
;   [179:24] arr
;   [179:24] bounds check
;   [179:24] allocate scratch register -> r13
;   [179:24] line number (--checks=line)
    mov r13, 179
;   [179:24] lower bound (--checks=lower)
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
;   [179:24] upper bound (--checks=upper)
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
;   [179:24] free scratch register r13
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
;   [184:16] allocate scratch register -> r14
;   [184:16] line number (--checks=line)
    mov r14, 184
;   [184:16] lower bound (--checks=lower)
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [184:16] upper bound (--checks=upper)
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
;   [184:16] free scratch register r14
;   [184:21] arr1
;   [184:21] bounds check
;   [184:21] allocate scratch register -> r14
;   [184:21] line number (--checks=line)
    mov r14, 184
;   [184:21] lower bound (--checks=lower)
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [184:21] upper bound (--checks=upper)
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [184:21] free scratch register r14
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
;       [185:36] allocate scratch register -> r14
;       [185:36] line number (--checks=line)
        mov r14, 185
;       [185:36] lower bound (--checks=lower)
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [185:36] upper bound (--checks=upper)
;       [185:36] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 4
;       [185:36] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [185:36] free scratch register r14
        lea rsi, [rbp + r15 * 4 + 232]
;       [185:19] free scratch register r15
;       [185:40] arr1[1]
;       [185:45] allocate scratch register -> r15
;       [185:45] set array index
;       [185:45] 1
        mov r15, 1
;       [185:45] bounds check
;       [185:45] allocate scratch register -> r14
;       [185:45] line number (--checks=line)
        mov r14, 185
;       [185:45] lower bound (--checks=lower)
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [185:45] upper bound (--checks=upper)
;       [185:45] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 8
;       [185:45] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [185:45] free scratch register r14
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
;       [191:29] allocate scratch register -> r14
;       [191:29] line number (--checks=line)
        mov r14, 191
;       [191:29] lower bound (--checks=lower)
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [191:29] upper bound (--checks=upper)
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
;       [191:29] free scratch register r14
        lea rsi, [rbp + 232]
;       [191:34] arr1
;       [191:34] bounds check
;       [191:34] allocate scratch register -> r14
;       [191:34] line number (--checks=line)
        mov r14, 191
;       [191:34] lower bound (--checks=lower)
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [191:34] upper bound (--checks=upper)
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
;       [191:34] free scratch register r14
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
;   [193:5] ix = 3
;   [193:10] 3
    mov qword [rbp + 248], 3
;   [194:5] arr[ix] = ~inv(arr[ix - 1])
;   [194:9] allocate scratch register -> r15
;   [194:9] set array index
;   [194:9] ix
    mov r15, qword [rbp + 248]
;   [194:9] bounds check
;   [194:9] allocate scratch register -> r14
;   [194:9] line number (--checks=line)
    mov r14, 194
;   [194:9] lower bound (--checks=lower)
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [194:9] upper bound (--checks=upper)
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [194:9] free scratch register r14
;   [194:16] arr = ~inv(arr[ix - 1])
;   [194:16] = expression
;   [194:16] ~inv(arr[ix - 1])
;   [194:16] instructions without scratch register 15, with 15
;   [194:24] allocate scratch register -> r14
;   [194:24] set array index
;   [194:24] ix
    mov r14, qword [rbp + 248]
;   [194:24] r14 - 1
;   [194:24] src: folded constant '- 1'
    sub r14, 1
;   [194:24] bounds check
;   [194:24] allocate scratch register -> r13
;   [194:24] line number (--checks=line)
    mov r13, 194
;   [194:24] lower bound (--checks=lower)
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
;   [194:24] upper bound (--checks=upper)
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [194:24] free scratch register r13
;   [194:16] instructions without scratch register 6, with 7
;   [62:6] inv(i i32) res i32
    func.inv.194.16:
;       [194:16] alias res -> arr (lea: rbp + r15 * 4 + 232)
;       [194:16] alias i -> arr (lea: rbp + r14 * 4 + 232)
;       [63:5] res = ~i
;       [63:11] instructions without scratch register 3, with 3
;       [63:12] ~i
;       [63:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 232]
        mov dword [rbp + r15 * 4 + 232], r13d
;       [63:12] free scratch register r13
        not dword [rbp + r15 * 4 + 232]
    func.inv.194.16.end:
    not dword [rbp + r15 * 4 + 232]
;       [194:16] free scratch register r14
;   [194:5] free scratch register r15
;   [195:5] assert(arr[ix] == 2)
;   [195:12] allocate scratch register -> r15
;   [195:12] ? arr[ix] == 2
;   [195:12] ? arr[ix] == 2
    cmp.195.12:
;   [195:16] allocate scratch register -> r14
;   [195:16] set array index
;   [195:16] ix
    mov r14, qword [rbp + 248]
;   [195:16] bounds check
;   [195:16] allocate scratch register -> r13
;   [195:16] line number (--checks=line)
    mov r13, 195
;   [195:16] lower bound (--checks=lower)
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
;   [195:16] upper bound (--checks=upper)
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [195:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 232], 2
;   [195:12] free scratch register r14
    sete r15b
    bool.195.12.end:
;   [32:6] assert(ok bool)
    func.assert.195.5:
;       [195:5] alias ok -> r15b
        if.32.27.195.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.195.5:
        cmp r15b, 0
        jne if.32.24.195.5.end
        if.32.27.195.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.195.5.end:
;       [195:5] free scratch register r15
    func.assert.195.5.end:
;   [197:5] faz(arr)
;   [72:6] faz(arg[] i32)
    func.faz.197.5:
;       [197:5] alias arg -> arr
;       [73:5] arg[1] = 0xfe
;       [73:14] 0xfe
        mov dword [rbp + 236], 254
    func.faz.197.5.end:
;   [198:5] assert(arr[1] == 0xfe)
;   [198:12] allocate scratch register -> r15
;   [198:12] ? arr[1] == 0xfe
;   [198:12] ? arr[1] == 0xfe
    cmp.198.12:
    cmp dword [rbp + 236], 254
    sete r15b
    bool.198.12.end:
;   [32:6] assert(ok bool)
    func.assert.198.5:
;       [198:5] alias ok -> r15b
        if.32.27.198.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.198.5:
        cmp r15b, 0
        jne if.32.24.198.5.end
        if.32.27.198.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.198.5.end:
;       [198:5] free scratch register r15
    func.assert.198.5.end:
;   [200:5] var arr3[] = { 3, 5 }
;   [200:9] arr3: i64[2] (16 B @ [rbp + 296])
;   [200:9] arr3= { 3, 5 }
;   [200:18] size <= 16 B, use immediates
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
;   [201:5] foo arr3
;   [201:9] allocate scratch register -> r15
;   [201:9] e: i64 (r15)
;   [201:9] i: i64 (8 B @ [rbp + 320])
;   [201:9] const n = 2
;   [201:9] initiate iterator e
    lea r15, [rbp + 296]
;   [201:9] initiate counter i
    mov qword [rbp + 320], 0
    foo.201.5:
;       [202:9] e = e + i + n
;       [202:13] instructions without scratch register 3, with 4
;       [202:13] e
;       [202:17] e + i
;       [202:17] src: operand
;       [202:17] allocate scratch register -> r14
        mov r14, qword [rbp + 320]
        add qword [r15], r14
;       [202:17] free scratch register r14
;       [202:13] e + 2
;       [202:13] src: folded constant '+ n'
        add qword [r15], 2
        foo.201.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.201.5
    foo.201.5.end:
;   [201:5] free scratch register r15
;   [204:5] assert(arr3[0] == 3 + 0 + 2)
;   [204:12] allocate scratch register -> r15
;   [204:12] ? arr3[0] == 3 + 0 + 2
;   [204:12] ? arr3[0] == 3 + 0 + 2
    cmp.204.12:
;   [204:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 296], 5
    sete r15b
    bool.204.12.end:
;   [32:6] assert(ok bool)
    func.assert.204.5:
;       [204:5] alias ok -> r15b
        if.32.27.204.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.204.5:
        cmp r15b, 0
        jne if.32.24.204.5.end
        if.32.27.204.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.204.5.end:
;       [204:5] free scratch register r15
    func.assert.204.5.end:
;   [205:5] assert(arr3[1] == 5 + 1 + 2)
;   [205:12] allocate scratch register -> r15
;   [205:12] ? arr3[1] == 5 + 1 + 2
;   [205:12] ? arr3[1] == 5 + 1 + 2
    cmp.205.12:
;   [205:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 304], 8
    sete r15b
    bool.205.12.end:
;   [32:6] assert(ok bool)
    func.assert.205.5:
;       [205:5] alias ok -> r15b
        if.32.27.205.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.205.5:
        cmp r15b, 0
        jne if.32.24.205.5.end
        if.32.27.205.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.205.5.end:
;       [205:5] free scratch register r15
    func.assert.205.5.end:
;   [211:5] var p point
;   [211:9] p: point (16 B @ [rbp + 312])
;   [211:9] zero 1 * 16 B = 16 B
;   [211:5] size <= 32 B, use mov
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
;   [214:7] p.fooz()
;   [46:6] point.fooz()
    func.point.fooz.214.7:
;       [214:7] alias self -> p
;       [47:5] self.x = 0b10
;       [47:14] 0b10
        mov qword [rbp + 312], 2
;       [48:5] self.y = 0xb
;       [48:14] 0xb
        mov qword [rbp + 320], 11
    func.point.fooz.214.7.end:
;   [217:5] assert(p.x == 2)
;   [217:12] allocate scratch register -> r15
;   [217:12] ? p.x == 2
;   [217:12] ? p.x == 2
    cmp.217.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.217.12.end:
;   [32:6] assert(ok bool)
    func.assert.217.5:
;       [217:5] alias ok -> r15b
        if.32.27.217.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.217.5:
        cmp r15b, 0
        jne if.32.24.217.5.end
        if.32.27.217.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.217.5.end:
;       [217:5] free scratch register r15
    func.assert.217.5.end:
;   [218:5] assert(p.y == 0xb)
;   [218:12] allocate scratch register -> r15
;   [218:12] ? p.y == 0xb
;   [218:12] ? p.y == 0xb
    cmp.218.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.218.12.end:
;   [32:6] assert(ok bool)
    func.assert.218.5:
;       [218:5] alias ok -> r15b
        if.32.27.218.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.218.5:
        cmp r15b, 0
        jne if.32.24.218.5.end
        if.32.27.218.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.218.5.end:
;       [218:5] free scratch register r15
    func.assert.218.5.end:
;   [220:5] var q point = p
;   [220:9] q: point (16 B @ [rbp + 328])
;   [220:9] q = p
;   [220:19] size <= 16 B, use mov
;   [220:19] allocate named register rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
;   [220:19] free named register rax
;   [223:5] assert(equal(p, q))
;   [223:12] allocate scratch register -> r15
;   [223:12] ? equal(p, q)
;   [223:12] ? shorthand: equal(p, q)
    cmp.223.12:
;       [223:12] equal(p, q)
;       [223:12] allocate named register rsi
;       [223:12] allocate named register rdi
;       [223:12] allocate named register rcx
;       [223:18] p
        lea rsi, [rbp + 312]
;       [223:21] q
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
;       [223:12] free named register rcx
;       [223:12] free named register rdi
;       [223:12] free named register rsi
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
;   [227:5] q.x = 3
;   [227:11] 3
    mov qword [rbp + 328], 3
;   [228:5] assert(not equal(p, q))
;   [228:12] allocate scratch register -> r15
;   [228:12] ? not equal(p, q)
;   [228:12] ? shorthand: not equal(p, q)
    cmp.228.12:
;       [228:16] equal(p, q)
;       [228:16] allocate named register rsi
;       [228:16] allocate named register rdi
;       [228:16] allocate named register rcx
;       [228:22] p
        lea rsi, [rbp + 312]
;       [228:25] q
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
;       [228:16] free named register rcx
;       [228:16] free named register rdi
;       [228:16] free named register rsi
        setne r15b
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
;   [230:5] var i = 0
;   [230:9] i: i64 (8 B @ [rbp + 344])
;   [230:9] i = 0
;   [230:13] 0
    mov qword [rbp + 344], 0
;   [231:5] bar(i)
;   [54:6] bar(arg)
    func.bar.231.5:
;       [231:5] alias arg -> i
        if.55.8.231.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.231.5:
        cmp qword [rbp + 344], 0
        je func.bar.231.5.end
        if.55.8.231.5.code:
;           [55:17] return
        if.55.5.231.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.231.5.end:
;   [232:5] assert(i == 0)
;   [232:12] allocate scratch register -> r15
;   [232:12] ? i == 0
;   [232:12] ? i == 0
    cmp.232.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.232.12.end:
;   [32:6] assert(ok bool)
    func.assert.232.5:
;       [232:5] alias ok -> r15b
        if.32.27.232.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.232.5:
        cmp r15b, 0
        jne if.32.24.232.5.end
        if.32.27.232.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.232.5.end:
;       [232:5] free scratch register r15
    func.assert.232.5.end:
;   [234:5] i = 1
;   [234:9] 1
    mov qword [rbp + 344], 1
;   [235:5] bar(i)
;   [54:6] bar(arg)
    func.bar.235.5:
;       [235:5] alias arg -> i
        if.55.8.235.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.235.5:
        cmp qword [rbp + 344], 0
        je func.bar.235.5.end
        if.55.8.235.5.code:
;           [55:17] return
        if.55.5.235.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.235.5.end:
;   [236:5] assert(i == 0xff)
;   [236:12] allocate scratch register -> r15
;   [236:12] ? i == 0xff
;   [236:12] ? i == 0xff
    cmp.236.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.236.12.end:
;   [32:6] assert(ok bool)
    func.assert.236.5:
;       [236:5] alias ok -> r15b
        if.32.27.236.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.236.5:
        cmp r15b, 0
        jne if.32.24.236.5.end
        if.32.27.236.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.236.5.end:
;       [236:5] free scratch register r15
    func.assert.236.5.end:
;   [238:5] var j = 1
;   [238:9] j: i64 (8 B @ [rbp + 352])
;   [238:9] j = 1
;   [238:13] 1
    mov qword [rbp + 352], 1
;   [239:5] var k = baz(j)
;   [239:9] k: i64 (8 B @ [rbp + 360])
;   [239:9] k = baz(j)
;   [239:13] k = baz(j)
;   [239:13] = expression
;   [239:13] baz(j)
;   [66:6] baz(arg) res
    func.baz.239.13:
;       [239:13] alias res -> k
;       [239:13] alias arg -> j
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
    func.baz.239.13.end:
;   [240:5] assert(k == 2)
;   [240:12] allocate scratch register -> r15
;   [240:12] ? k == 2
;   [240:12] ? k == 2
    cmp.240.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.240.12.end:
;   [32:6] assert(ok bool)
    func.assert.240.5:
;       [240:5] alias ok -> r15b
        if.32.27.240.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.240.5:
        cmp r15b, 0
        jne if.32.24.240.5.end
        if.32.27.240.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.240.5.end:
;       [240:5] free scratch register r15
    func.assert.240.5.end:
;   [242:5] k = baz(1)
;   [242:9] k = baz(1)
;   [242:9] = expression
;   [242:9] baz(1)
;   [66:6] baz(arg) res
    func.baz.242.9:
;       [242:9] alias res -> k
;       [242:9] alias arg -> 1
;       [67:5] res = arg * 2
;       [67:11] res = 2
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 360], 2
    func.baz.242.9.end:
;   [243:5] assert(k == 2)
;   [243:12] allocate scratch register -> r15
;   [243:12] ? k == 2
;   [243:12] ? k == 2
    cmp.243.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.243.12.end:
;   [32:6] assert(ok bool)
    func.assert.243.5:
;       [243:5] alias ok -> r15b
        if.32.27.243.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.243.5:
        cmp r15b, 0
        jne if.32.24.243.5.end
        if.32.27.243.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.243.5.end:
;       [243:5] free scratch register r15
    func.assert.243.5.end:
;   [245:5] var p0 point = {baz(3), 0}
;   [245:9] p0: point (16 B @ [rbp + 368])
;   [245:9] p0 = {baz(3), 0}
;   [245:21] copy field 'x'
;   [245:21] p0.x = baz(3)
;   [245:21] = expression
;   [245:21] baz(3)
;   [66:6] baz(arg) res
    func.baz.245.21:
;       [245:21] alias res -> p0.x (lea: rbp + 368)
;       [245:21] alias arg -> 3
;       [67:5] res = arg * 2
;       [67:11] res = 6
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 368], 6
    func.baz.245.21.end:
;   [245:29] copy field 'y'
    mov qword [rbp + 376], 0
;   [246:5] assert(p0.x == 6)
;   [246:12] allocate scratch register -> r15
;   [246:12] ? p0.x == 6
;   [246:12] ? p0.x == 6
    cmp.246.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.246.12.end:
;   [32:6] assert(ok bool)
    func.assert.246.5:
;       [246:5] alias ok -> r15b
        if.32.27.246.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.246.5:
        cmp r15b, 0
        jne if.32.24.246.5.end
        if.32.27.246.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.246.5.end:
;       [246:5] free scratch register r15
    func.assert.246.5.end:
;   [248:5] var pt = point.at(-1, -2)
;   [248:9] pt: point (16 B @ [rbp + 384])
;   [248:9] pt = point.at(-1, -2)
;   [248:14] point.at(-1, -2)
;   [90:6] point.at(x, y) self
    func.point.at.248.14:
;       [248:14] alias self -> pt
;       [248:14] alias x -> -1
;       [248:14] alias y -> -2
;       [91:5] self.x = x
;       [91:14] x
        mov qword [rbp + 384], -1
;       [92:5] self.y = y
;       [92:14] y
        mov qword [rbp + 392], -2
    func.point.at.248.14.end:
;   [252:5] assert(pt.x == -1)
;   [252:12] allocate scratch register -> r15
;   [252:12] ? pt.x == -1
;   [252:12] ? pt.x == -1
    cmp.252.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.252.12.end:
;   [32:6] assert(ok bool)
    func.assert.252.5:
;       [252:5] alias ok -> r15b
        if.32.27.252.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.252.5:
        cmp r15b, 0
        jne if.32.24.252.5.end
        if.32.27.252.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.252.5.end:
;       [252:5] free scratch register r15
    func.assert.252.5.end:
;   [253:5] assert(pt.y == -2)
;   [253:12] allocate scratch register -> r15
;   [253:12] ? pt.y == -2
;   [253:12] ? pt.y == -2
    cmp.253.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.253.12.end:
;   [32:6] assert(ok bool)
    func.assert.253.5:
;       [253:5] alias ok -> r15b
        if.32.27.253.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.253.5:
        cmp r15b, 0
        jne if.32.24.253.5.end
        if.32.27.253.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.253.5.end:
;       [253:5] free scratch register r15
    func.assert.253.5.end:
;   [255:5] var x = 1
;   [255:9] x: i64 (8 B @ [rbp + 400])
;   [255:9] x = 1
;   [255:13] 1
    mov qword [rbp + 400], 1
;   [256:5] var y = 2
;   [256:9] y: i64 (8 B @ [rbp + 408])
;   [256:9] y = 2
;   [256:13] 2
    mov qword [rbp + 408], 2
;   [258:5] var o1 object = {{x * 10, y}, 0xff0000}
;   [258:9] o1: object (24 B @ [rbp + 416])
;   [258:9] o1 = {{x * 10, y}, 0xff0000}
;   [258:22] copy field 'pos'
;   [258:23] copy field 'x'
;   [258:23] instructions without scratch register 5, with 3
;   [258:23] allocate scratch register -> r15
;   [258:23] x
    mov r15, qword [rbp + 400]
;   [258:23] r15 * 10
;   [258:23] src: folded constant '* 10'
    imul r15, 10
    mov qword [rbp + 416], r15
;   [258:23] free scratch register r15
;   [258:31] copy field 'y'
;   [258:31] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
;   [258:31] free scratch register r15
;   [258:35] copy field 'color'
    mov dword [rbp + 432], 16711680
;   [258:21] zero padding: 4 B
;   [258:21] size <= 32 B, use mov
    mov dword [rbp + 436], 0
;   [259:5] assert(o1.pos.x == 10)
;   [259:12] allocate scratch register -> r15
;   [259:12] ? o1.pos.x == 10
;   [259:12] ? o1.pos.x == 10
    cmp.259.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.259.12.end:
;   [32:6] assert(ok bool)
    func.assert.259.5:
;       [259:5] alias ok -> r15b
        if.32.27.259.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.259.5:
        cmp r15b, 0
        jne if.32.24.259.5.end
        if.32.27.259.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.259.5.end:
;       [259:5] free scratch register r15
    func.assert.259.5.end:
;   [260:5] assert(o1.pos.y == 2)
;   [260:12] allocate scratch register -> r15
;   [260:12] ? o1.pos.y == 2
;   [260:12] ? o1.pos.y == 2
    cmp.260.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.260.12.end:
;   [32:6] assert(ok bool)
    func.assert.260.5:
;       [260:5] alias ok -> r15b
        if.32.27.260.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.260.5:
        cmp r15b, 0
        jne if.32.24.260.5.end
        if.32.27.260.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.260.5.end:
;       [260:5] free scratch register r15
    func.assert.260.5.end:
;   [261:5] assert(o1.color == 0xff0000)
;   [261:12] allocate scratch register -> r15
;   [261:12] ? o1.color == 0xff0000
;   [261:12] ? o1.color == 0xff0000
    cmp.261.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.261.12.end:
;   [32:6] assert(ok bool)
    func.assert.261.5:
;       [261:5] alias ok -> r15b
        if.32.27.261.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.261.5:
        cmp r15b, 0
        jne if.32.24.261.5.end
        if.32.27.261.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.261.5.end:
;       [261:5] free scratch register r15
    func.assert.261.5.end:
;   [263:5] var p1 point = {-x, -y}
;   [263:9] p1: point (16 B @ [rbp + 440])
;   [263:9] p1 = {-x, -y}
;   [263:21] copy field 'x'
;   [263:21] instructions without scratch register 3, with 3
;   [263:21] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 440], r15
;   [263:21] free scratch register r15
    neg qword [rbp + 440]
;   [263:25] copy field 'y'
;   [263:25] instructions without scratch register 3, with 3
;   [263:25] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 448], r15
;   [263:25] free scratch register r15
    neg qword [rbp + 448]
;   [264:5] o1.pos = p1
;   [264:14] size <= 16 B, use mov
;   [264:14] allocate named register rax
    mov rax, qword [rbp + 440]
    mov qword [rbp + 416], rax
    mov rax, qword [rbp + 448]
    mov qword [rbp + 424], rax
;   [264:14] free named register rax
;   [265:5] assert(o1.pos.x == -1)
;   [265:12] allocate scratch register -> r15
;   [265:12] ? o1.pos.x == -1
;   [265:12] ? o1.pos.x == -1
    cmp.265.12:
    cmp qword [rbp + 416], -1
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
;   [266:5] assert(o1.pos.y == -2)
;   [266:12] allocate scratch register -> r15
;   [266:12] ? o1.pos.y == -2
;   [266:12] ? o1.pos.y == -2
    cmp.266.12:
    cmp qword [rbp + 424], -2
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
;   [268:5] var o2 object = o1
;   [268:9] o2: object (24 B @ [rbp + 456])
;   [268:9] o2 = o1
;   [268:21] allocate named register rsi
;   [268:21] allocate named register rdi
;   [268:21] allocate named register rcx
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
;   [268:21] free named register rcx
;   [268:21] free named register rdi
;   [268:21] free named register rsi
;   [269:5] assert(o2.pos.x == -1)
;   [269:12] allocate scratch register -> r15
;   [269:12] ? o2.pos.x == -1
;   [269:12] ? o2.pos.x == -1
    cmp.269.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.269.12.end:
;   [32:6] assert(ok bool)
    func.assert.269.5:
;       [269:5] alias ok -> r15b
        if.32.27.269.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.269.5:
        cmp r15b, 0
        jne if.32.24.269.5.end
        if.32.27.269.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.269.5.end:
;       [269:5] free scratch register r15
    func.assert.269.5.end:
;   [270:5] assert(o2.pos.y == -2)
;   [270:12] allocate scratch register -> r15
;   [270:12] ? o2.pos.y == -2
;   [270:12] ? o2.pos.y == -2
    cmp.270.12:
    cmp qword [rbp + 464], -2
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
;   [271:5] assert(o2.color == 0xff0000)
;   [271:12] allocate scratch register -> r15
;   [271:12] ? o2.color == 0xff0000
;   [271:12] ? o2.color == 0xff0000
    cmp.271.12:
    cmp dword [rbp + 472], 16711680
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
;   [273:5] var o3[2] object
;   [273:9] o3: object[2] (48 B @ [rbp + 480])
;   [273:9] zero 2 * 24 B = 48 B
;   [273:5] allocate named register rax
;   [273:5] allocate named register rdi
;   [273:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
;   [273:5] free named register rcx
;   [273:5] free named register rdi
;   [273:5] free named register rax
;   [274:5] o3[0].pos.y = 73
;   [274:19] 73
    mov qword [rbp + 488], 73
;   [276:5] assert(o3[0].pos.y == 73)
;   [276:12] allocate scratch register -> r15
;   [276:12] ? o3[0].pos.y == 73
;   [276:12] ? o3[0].pos.y == 73
    cmp.276.12:
    cmp qword [rbp + 488], 73
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
;   [277:5] o3[1] = object.at(2, 74, 0xffffff)
;   [277:13] object.at(2, 74, 0xffffff)
;   [95:6] object.at(x, y, color i32) self
    func.object.at.277.13:
;       [277:13] alias self -> o3 (lea: rbp + 504)
;       [277:13] alias x -> 2
;       [277:13] alias y -> 74
;       [277:13] alias color -> 16777215
;       [96:5] self.pos = point.at(x, y)
;       [96:16] point.at(x, y)
;       [90:6] point.at(x, y) self
        func.point.at.96.16.277.13:
;           [96:16] alias self -> self.pos (lea: rbp + 504)
;           [96:16] alias x -> 2
;           [96:16] alias y -> 74
;           [91:5] self.x = x
;           [91:14] x
            mov qword [rbp + 504], 2
;           [92:5] self.y = y
;           [92:14] y
            mov qword [rbp + 512], 74
        func.point.at.96.16.277.13.end:
;       [97:5] self.color = color
;       [97:18] color
        mov dword [rbp + 520], 16777215
    func.object.at.277.13.end:
;   [278:5] assert(o3[1].pos.y == 74)
;   [278:12] allocate scratch register -> r15
;   [278:12] ? o3[1].pos.y == 74
;   [278:12] ? o3[1].pos.y == 74
    cmp.278.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.278.12.end:
;   [32:6] assert(ok bool)
    func.assert.278.5:
;       [278:5] alias ok -> r15b
        if.32.27.278.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.278.5:
        cmp r15b, 0
        jne if.32.24.278.5.end
        if.32.27.278.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.278.5.end:
;       [278:5] free scratch register r15
    func.assert.278.5.end:
;   [280:5] var worlds[8] world
;   [280:9] worlds: world[8] (512 B @ [rbp + 528])
;   [280:9] zero 8 * 64 B = 512 B
;   [280:5] allocate named register rax
;   [280:5] allocate named register rdi
;   [280:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
;   [280:5] free named register rcx
;   [280:5] free named register rdi
;   [280:5] free named register rax
;   [281:5] worlds[1].locations[1] = 0xffee
;   [281:30] 0xffee
    mov qword [rbp + 600], 65518
;   [282:5] assert(worlds[1].locations[1] == 0xffee)
;   [282:12] allocate scratch register -> r15
;   [282:12] ? worlds[1].locations[1] == 0xffee
;   [282:12] ? worlds[1].locations[1] == 0xffee
    cmp.282.12:
    cmp qword [rbp + 600], 65518
    sete r15b
    bool.282.12.end:
;   [32:6] assert(ok bool)
    func.assert.282.5:
;       [282:5] alias ok -> r15b
        if.32.27.282.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.282.5:
        cmp r15b, 0
        jne if.32.24.282.5.end
        if.32.27.282.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.282.5.end:
;       [282:5] free scratch register r15
    func.assert.282.5.end:
;   [284:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [284:5] allocate named register rsi
;   [284:5] allocate named register rdi
;   [284:5] allocate named register rcx
;   [287:9] array_length(worlds[0].locations)
;   [287:9] rcx = array_length(worlds[0].locations)
;   [287:9] = expression
;   [287:9] array_length(worlds[0].locations)
    mov rcx, 8
;   [285:9] worlds[1].locations
;   [285:9] bounds check
;   [285:9] allocate scratch register -> r15
;   [285:9] line number (--checks=line)
    mov r15, 285
;   [285:9] lower bound (--checks=lower)
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
;   [285:9] upper bound (--checks=upper)
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [285:9] free scratch register r15
    lea rsi, [rbp + 592]
;   [286:9] worlds[0].locations
;   [286:9] bounds check
;   [286:9] allocate scratch register -> r15
;   [286:9] line number (--checks=line)
    mov r15, 286
;   [286:9] lower bound (--checks=lower)
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
;   [286:9] upper bound (--checks=upper)
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [286:9] free scratch register r15
    lea rdi, [rbp + 528]
    shl rcx, 3
    rep movsb
;   [284:5] free named register rcx
;   [284:5] free named register rdi
;   [284:5] free named register rsi
;   [291:5] assert(worlds[0].locations[1] == 0xffee)
;   [291:12] allocate scratch register -> r15
;   [291:12] ? worlds[0].locations[1] == 0xffee
;   [291:12] ? worlds[0].locations[1] == 0xffee
    cmp.291.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.291.12.end:
;   [32:6] assert(ok bool)
    func.assert.291.5:
;       [291:5] alias ok -> r15b
        if.32.27.291.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.291.5:
        cmp r15b, 0
        jne if.32.24.291.5.end
        if.32.27.291.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.291.5.end:
;       [291:5] free scratch register r15
    func.assert.291.5.end:
;   [292:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [292:12] allocate scratch register -> r15
;   [292:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [292:12] ? shorthand: arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
    cmp.292.12:
;       [292:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;       [292:12] allocate named register rsi
;       [292:12] allocate named register rdi
;       [292:12] allocate named register rcx
;       [295:14] array_length(worlds[0].locations)
;       [295:14] rcx = array_length(worlds[0].locations)
;       [295:14] = expression
;       [295:14] array_length(worlds[0].locations)
        mov rcx, 8
;       [293:14] worlds[0].locations
;       [293:14] bounds check
;       [293:14] allocate scratch register -> r14
;       [293:14] line number (--checks=line)
        mov r14, 293
;       [293:14] lower bound (--checks=lower)
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [293:14] upper bound (--checks=upper)
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
;       [293:14] free scratch register r14
        lea rsi, [rbp + 528]
;       [294:14] worlds[1].locations
;       [294:14] bounds check
;       [294:14] allocate scratch register -> r14
;       [294:14] line number (--checks=line)
        mov r14, 294
;       [294:14] lower bound (--checks=lower)
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [294:14] upper bound (--checks=upper)
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
;       [294:14] free scratch register r14
        lea rdi, [rbp + 592]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [292:12] free named register rcx
;       [292:12] free named register rdi
;       [292:12] free named register rsi
        sete r15b
    bool.292.12.end:
;   [32:6] assert(ok bool)
    func.assert.292.5:
;       [292:5] alias ok -> r15b
        if.32.27.292.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.292.5:
        cmp r15b, 0
        jne if.32.24.292.5.end
        if.32.27.292.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.292.5.end:
;       [292:5] free scratch register r15
    func.assert.292.5.end:
;   [297:5] var arr2[] = { -1, 2 }
;   [297:9] arr2: i64[2] (16 B @ [rbp + 1040])
;   [297:9] arr2= { -1, 2 }
;   [297:18] size <= 16 B, use immediates
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
;   [298:5] assert(array_length(arr2) == 2)
;   [298:12] allocate scratch register -> r15
;   [298:12] ? array_length(arr2) == 2
;   [298:12] ? array_length(arr2) == 2
    cmp.298.12:
;   [298:12] allocate scratch register -> r14
;       [298:12] r14 = array_length(arr2)
;       [298:12] = expression
;       [298:12] array_length(arr2)
        mov r14, 2
    cmp r14, 2
;   [298:12] free scratch register r14
    sete r15b
    bool.298.12.end:
;   [32:6] assert(ok bool)
    func.assert.298.5:
;       [298:5] alias ok -> r15b
        if.32.27.298.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.298.5:
        cmp r15b, 0
        jne if.32.24.298.5.end
        if.32.27.298.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.298.5.end:
;       [298:5] free scratch register r15
    func.assert.298.5.end:
;   [299:5] assert(arr2[0] == -1)
;   [299:12] allocate scratch register -> r15
;   [299:12] ? arr2[0] == -1
;   [299:12] ? arr2[0] == -1
    cmp.299.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.299.12.end:
;   [32:6] assert(ok bool)
    func.assert.299.5:
;       [299:5] alias ok -> r15b
        if.32.27.299.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.299.5:
        cmp r15b, 0
        jne if.32.24.299.5.end
        if.32.27.299.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.299.5.end:
;       [299:5] free scratch register r15
    func.assert.299.5.end:
;   [300:5] assert(arr2[1] == 2)
;   [300:12] allocate scratch register -> r15
;   [300:12] ? arr2[1] == 2
;   [300:12] ? arr2[1] == 2
    cmp.300.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.300.12.end:
;   [32:6] assert(ok bool)
    func.assert.300.5:
;       [300:5] alias ok -> r15b
        if.32.27.300.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.300.5:
        cmp r15b, 0
        jne if.32.24.300.5.end
        if.32.27.300.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.300.5.end:
;       [300:5] free scratch register r15
    func.assert.300.5.end:
;   [302:5] var counter
;   [302:9] counter: i64 (8 B @ [rbp + 1056])
;   [302:9] zero 1 * 8 B = 8 B
;   [302:5] size <= 32 B, use mov
    mov qword [rbp + 1056], 0
;   [303:5] var nm str
;   [303:9] nm: str (128 B @ [rbp + 1064])
;   [303:9] zero 1 * 128 B = 128 B
;   [303:5] allocate named register rax
;   [303:5] allocate named register rdi
;   [303:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
;   [303:5] free named register rcx
;   [303:5] free named register rdi
;   [303:5] free named register rax
;   [304:5] print(hello)
;   [35:6] print(str[] i8)
    func.print.304.5:
;       [304:5] alias str -> hello
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
    func.print.304.5.end:
;   [305:5] label
    loop.305.5:
;       [306:9] counter = counter + 1
;       [306:19] instructions without scratch register 1, with 3
;       [306:19] counter
;       [306:19] counter + 1
;       [306:19] src: folded constant '+ 1'
        add qword [rbp + 1056], 1
;       [307:9] print_num(counter)
;       [307:9] address of argument 'counter' to parameter 'num'
;       [307:9] allocate scratch register -> r15
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
;       [307:9] free scratch register r15
;       [307:9] set function frame base
        lea rbx, [rbp + 1192]
        call func.print_num
;       [308:9] print(colon)
;       [35:6] print(str[] i8)
        func.print.308.9:
;           [308:9] alias str -> colon
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
        func.print.308.9.end:
;       [309:9] print(prompt1)
;       [35:6] print(str[] i8)
        func.print.309.9:
;           [309:9] alias str -> prompt1
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
        func.print.309.9.end:
;       [310:12] nm.input()
;       [76:6] str.input()
        func.str.input.310.12:
;           [310:12] alias self -> nm
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
        func.str.input.310.12.end:
        if.312.12:
;       [312:12] ? nm.len <= 0
;       [312:12] ? nm.len <= 0
        cmp.312.12:
        cmp byte [rbp + 1064], 0
        jle loop.305.5.end
        if.312.12.code:
;           [313:13] break
        if.314.19:
;       [314:19] ? nm.len <= 4
;       [314:19] ? nm.len <= 4
        cmp.314.19:
        cmp byte [rbp + 1064], 4
        jg if.312.9.else
        if.314.19.code:
;           [315:13] print(prompt2)
;           [35:6] print(str[] i8)
            func.print.315.13:
;               [315:13] alias str -> prompt2
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
            func.print.315.13.end:
;           [316:13] continue
            jmp loop.305.5
        if.312.9.else:
;           [318:13] print(prompt3)
;           [35:6] print(str[] i8)
            func.print.318.13:
;               [318:13] alias str -> prompt3
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
            func.print.318.13.end:
;           [319:16] nm.output()
;           [83:6] str.output()
            func.str.output.319.16:
;               [319:16] alias self -> nm
;               [84:5] write(1, self.data, self.len)
;               [84:5] allocate named register rdi
;               [84:5] allocate named register rsi
;               [84:5] allocate named register rdx
;               [84:11] 1
                mov rdi, 1
;               [84:25] self.len
                movsx rdx, byte [rbp + 1064]
;               [84:14] bounds check
;               [84:14] allocate scratch register -> r15
;               [84:14] line number (--checks=line)
                mov r15, 84
;               [84:14] lower bound (--checks=lower)
                test rdx, rdx
                cmovs rbp, r15
                js baz_bounds_panic
;               [84:14] upper bound (--checks=upper)
                cmp rdx, 127
                cmovg rbp, r15
                jg baz_bounds_panic
;               [84:14] free scratch register r15
                lea rsi, [rbp + 1065]
;               [84:5] allocate named register rax
                mov rax, 1
                syscall
;               [84:5] free named register rax
;               [84:5] free named register rdx
;               [84:5] free named register rsi
;               [84:5] free named register rdi
            func.str.output.319.16.end:
;           [320:13] print(dot)
;           [35:6] print(str[] i8)
            func.print.320.13:
;               [320:13] alias str -> dot
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
            func.print.320.13.end:
;           [321:13] print(nl)
;           [35:6] print(str[] i8)
            func.print.321.13:
;               [321:13] alias str -> nl
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
            func.print.321.13.end:
        if.312.9.end:
    jmp loop.305.5
    loop.305.5.end:
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
;       [130:13] allocate scratch register -> r14
;       [130:13] line number (--checks=line)
        mov r14, 130
;       [130:13] lower bound (--checks=lower)
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
;       [130:13] upper bound (--checks=upper)
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [130:13] free scratch register r14
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
;       [137:13] allocate scratch register -> r14
;       [137:13] line number (--checks=line)
        mov r14, 137
;       [137:13] lower bound (--checks=lower)
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
;       [137:13] upper bound (--checks=upper)
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [137:13] free scratch register r14
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
;       [142:13] allocate scratch register -> r14
;       [142:13] line number (--checks=line)
        mov r14, 142
;       [142:13] lower bound (--checks=lower)
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
;       [142:13] upper bound (--checks=upper)
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [142:13] free scratch register r14
;       [142:26] buf[i]
;       [142:30] allocate scratch register -> r14
;       [142:30] set array index
;       [142:30] i
        mov r14, qword [rbx + 48]
;       [142:30] bounds check
;       [142:30] allocate scratch register -> r13
;       [142:30] line number (--checks=line)
        mov r13, 142
;       [142:30] lower bound (--checks=lower)
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
;       [142:30] upper bound (--checks=upper)
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [142:30] free scratch register r13
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
;   [148:14] allocate scratch register -> r15
;   [148:14] line number (--checks=line)
    mov r15, 148
;   [148:14] lower bound (--checks=lower)
    test rdx, rdx
    cmovs rbp, r15
    js baz_bounds_panic
;   [148:14] upper bound (--checks=upper)
    cmp rdx, 20
    cmovg rbp, r15
    jg baz_bounds_panic
;   [148:14] free scratch register r15
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
; bounds failure handler (--checks=upper or --checks=lower)
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
