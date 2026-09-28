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
C/C++ Header                    55           5551           2114          17653
C++                              1             66             19            329
-------------------------------------------------------------------------------
SUM:                            56           5617           2133          17982
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
dat      nums[4] = { 1 } # remaining elements are zeroed
dat     str1 str = { 3 } # remaining fields are zeroed

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

# types can have methods with same name as fields

func point.x(x) {
    self.x = x
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
#   and argument refer to same memory range (`arr`)
#   same possible ub if anyt function arguments share the same memory region

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

    pt.x(2)
    assert(pt.x == 2)

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
    cmp.160.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.160.12.end:
    func.assert.160.5:
        if.32.27.160.5:
        cmp.32.27.160.5:
        cmp r15b, 0
        jne if.32.24.160.5.end
        if.32.27.160.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.160.5.end:
    func.assert.160.5.end:
    mov qword [rbp + 224], -1
    cmp.163.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.163.12.end:
    func.assert.163.5:
        if.32.27.163.5:
        cmp.32.27.163.5:
        cmp r15b, 0
        jne if.32.24.163.5.end
        if.32.27.163.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.163.5.end:
    func.assert.163.5.end:
        func.assert.169.9:
            if.32.27.169.9:
            cmp.32.27.169.9:
            if.32.24.169.9.end:
        func.assert.169.9.end:
    func.assert.172.5:
        if.32.27.172.5:
        cmp.32.27.172.5:
        if.32.24.172.5.end:
    func.assert.172.5.end:
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
    cmp.182.12:
    cmp dword [rbp + 236], 2
    sete r15b
    bool.182.12.end:
    func.assert.182.5:
        if.32.27.182.5:
        cmp.32.27.182.5:
        cmp r15b, 0
        jne if.32.24.182.5.end
        if.32.27.182.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.182.5.end:
    func.assert.182.5.end:
    cmp.183.12:
    cmp dword [rbp + 240], 2
    sete r15b
    bool.183.12.end:
    func.assert.183.5:
        if.32.27.183.5:
        cmp.32.27.183.5:
        cmp r15b, 0
        jne if.32.24.183.5.end
        if.32.27.183.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.183.5.end:
    func.assert.183.5.end:
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
    cmp.186.12:
    cmp dword [rbp + 232], 2
    sete r15b
    bool.186.12.end:
    func.assert.186.5:
        if.32.27.186.5:
        cmp.32.27.186.5:
        cmp r15b, 0
        jne if.32.24.186.5.end
        if.32.27.186.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.186.5.end:
    func.assert.186.5.end:
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
    cmp.191.19:
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
    bool.191.19.end:
    cmp.194.12:
    mov r15b, byte [rbp + 288]
    bool.194.12.end:
    func.assert.194.5:
        if.32.27.194.5:
        cmp.32.27.194.5:
        cmp r15b, 0
        jne if.32.24.194.5.end
        if.32.27.194.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.194.5.end:
    func.assert.194.5.end:
    mov dword [rbp + 264], -1
    cmp.197.12:
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
    bool.197.12.end:
    func.assert.197.5:
        if.32.27.197.5:
        cmp.32.27.197.5:
        cmp r15b, 0
        jne if.32.24.197.5.end
        if.32.27.197.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.197.5.end:
    func.assert.197.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.205.20:
        mov r14d, dword [rbp + r15 * 4 + 232]
        mov dword [rbp + 292], r14d
        not dword [rbp + 292]
    func.inv.205.20.end:
    not dword [rbp + 292]
    mov r15, qword [rbp + 248]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 292]
    mov dword [rbp + r15 * 4 + 232], r14d
    cmp.207.12:
    mov r14, qword [rbp + 248]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.207.12.end:
    func.assert.207.5:
        if.32.27.207.5:
        cmp.32.27.207.5:
        cmp r15b, 0
        jne if.32.24.207.5.end
        if.32.27.207.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.207.5.end:
    func.assert.207.5.end:
    func.faz.209.5:
        mov dword [rbp + 236], 254
    func.faz.209.5.end:
    cmp.210.12:
    cmp dword [rbp + 236], 254
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
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.213.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.213.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.213.5
    foo.213.5.end:
    cmp.216.12:
    cmp qword [rbp + 296], 5
    sete r15b
    bool.216.12.end:
    func.assert.216.5:
        if.32.27.216.5:
        cmp.32.27.216.5:
        cmp r15b, 0
        jne if.32.24.216.5.end
        if.32.27.216.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.216.5.end:
    func.assert.216.5.end:
    cmp.217.12:
    cmp qword [rbp + 304], 8
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
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.226.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.226.7.end:
    cmp.229.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.229.12.end:
    func.assert.229.5:
        if.32.27.229.5:
        cmp.32.27.229.5:
        cmp r15b, 0
        jne if.32.24.229.5.end
        if.32.27.229.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.229.5.end:
    func.assert.229.5.end:
    cmp.230.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.230.12.end:
    func.assert.230.5:
        if.32.27.230.5:
        cmp.32.27.230.5:
        cmp r15b, 0
        jne if.32.24.230.5.end
        if.32.27.230.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.230.5.end:
    func.assert.230.5.end:
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.235.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.235.12.end:
    func.assert.235.5:
        if.32.27.235.5:
        cmp.32.27.235.5:
        cmp r15b, 0
        jne if.32.24.235.5.end
        if.32.27.235.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.235.5.end:
    func.assert.235.5.end:
    mov qword [rbp + 328], 3
    cmp.240.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        setne r15b
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
    mov qword [rbp + 344], 0
    func.bar.243.5:
        if.55.8.243.5:
        cmp.55.8.243.5:
        cmp qword [rbp + 344], 0
        je func.bar.243.5.end
        if.55.8.243.5.code:
        if.55.5.243.5.end:
        mov qword [rbp + 344], 255
    func.bar.243.5.end:
    cmp.244.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.244.12.end:
    func.assert.244.5:
        if.32.27.244.5:
        cmp.32.27.244.5:
        cmp r15b, 0
        jne if.32.24.244.5.end
        if.32.27.244.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.244.5.end:
    func.assert.244.5.end:
    mov qword [rbp + 344], 1
    func.bar.247.5:
        if.55.8.247.5:
        cmp.55.8.247.5:
        cmp qword [rbp + 344], 0
        je func.bar.247.5.end
        if.55.8.247.5.code:
        if.55.5.247.5.end:
        mov qword [rbp + 344], 255
    func.bar.247.5.end:
    cmp.248.12:
    cmp qword [rbp + 344], 255
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
    mov qword [rbp + 352], 1
    func.baz.251.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.251.13.end:
    cmp.252.12:
    cmp qword [rbp + 360], 2
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
    func.baz.254.9:
        mov qword [rbp + 360], 2
    func.baz.254.9.end:
    cmp.255.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.255.12.end:
    func.assert.255.5:
        if.32.27.255.5:
        cmp.32.27.255.5:
        cmp r15b, 0
        jne if.32.24.255.5.end
        if.32.27.255.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.255.5.end:
    func.assert.255.5.end:
    func.baz.257.21:
        mov qword [rbp + 368], 6
    func.baz.257.21.end:
    mov qword [rbp + 376], 0
    cmp.258.12:
    cmp qword [rbp + 368], 6
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
    func.point.at.260.14:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point.at.260.14.end:
    cmp.264.12:
    cmp qword [rbp + 384], -1
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
    cmp qword [rbp + 392], -2
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
    func.point.x.267.8:
        mov qword [rbp + 384], 2
    func.point.x.267.8.end:
    cmp.268.12:
    cmp qword [rbp + 384], 2
    sete r15b
    bool.268.12.end:
    func.assert.268.5:
        if.32.27.268.5:
        cmp.32.27.268.5:
        cmp r15b, 0
        jne if.32.24.268.5.end
        if.32.27.268.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.268.5.end:
    func.assert.268.5.end:
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.274.12:
    cmp qword [rbp + 416], 10
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
    cmp qword [rbp + 424], 2
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
    cmp dword [rbp + 432], 16711680
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
    cmp.280.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.280.12.end:
    func.assert.280.5:
        if.32.27.280.5:
        cmp.32.27.280.5:
        cmp r15b, 0
        jne if.32.24.280.5.end
        if.32.27.280.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.280.5.end:
    func.assert.280.5.end:
    cmp.281.12:
    cmp qword [rbp + 424], -2
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
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.284.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.284.12.end:
    func.assert.284.5:
        if.32.27.284.5:
        cmp.32.27.284.5:
        cmp r15b, 0
        jne if.32.24.284.5.end
        if.32.27.284.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.284.5.end:
    func.assert.284.5.end:
    cmp.285.12:
    cmp qword [rbp + 464], -2
    sete r15b
    bool.285.12.end:
    func.assert.285.5:
        if.32.27.285.5:
        cmp.32.27.285.5:
        cmp r15b, 0
        jne if.32.24.285.5.end
        if.32.27.285.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.285.5.end:
    func.assert.285.5.end:
    cmp.286.12:
    cmp dword [rbp + 472], 16711680
    sete r15b
    bool.286.12.end:
    func.assert.286.5:
        if.32.27.286.5:
        cmp.32.27.286.5:
        cmp r15b, 0
        jne if.32.24.286.5.end
        if.32.27.286.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.286.5.end:
    func.assert.286.5.end:
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.291.12:
    cmp qword [rbp + 488], 73
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
    func.object.at.292.13:
        func.point.at.102.16.292.13:
            mov qword [rbp + 504], 2
            mov qword [rbp + 512], 74
        func.point.at.102.16.292.13.end:
        mov dword [rbp + 520], 16777215
    func.object.at.292.13.end:
    cmp.293.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.293.12.end:
    func.assert.293.5:
        if.32.27.293.5:
        cmp.32.27.293.5:
        cmp r15b, 0
        jne if.32.24.293.5.end
        if.32.27.293.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.293.5.end:
    func.assert.293.5.end:
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 600], 65518
    cmp.297.12:
    cmp qword [rbp + 600], 65518
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
    cmp.306.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.306.12.end:
    func.assert.306.5:
        if.32.27.306.5:
        cmp.32.27.306.5:
        cmp r15b, 0
        jne if.32.24.306.5.end
        if.32.27.306.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.306.5.end:
    func.assert.306.5.end:
    cmp.307.12:
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
    bool.307.12.end:
    func.assert.307.5:
        if.32.27.307.5:
        cmp.32.27.307.5:
        cmp r15b, 0
        jne if.32.24.307.5.end
        if.32.27.307.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.307.5.end:
    func.assert.307.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.313.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.313.12.end:
    func.assert.313.5:
        if.32.27.313.5:
        cmp.32.27.313.5:
        cmp r15b, 0
        jne if.32.24.313.5.end
        if.32.27.313.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.313.5.end:
    func.assert.313.5.end:
    cmp.314.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.314.12.end:
    func.assert.314.5:
        if.32.27.314.5:
        cmp.32.27.314.5:
        cmp r15b, 0
        jne if.32.24.314.5.end
        if.32.27.314.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.314.5.end:
    func.assert.314.5.end:
    cmp.315.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.315.12.end:
    func.assert.315.5:
        if.32.27.315.5:
        cmp.32.27.315.5:
        cmp r15b, 0
        jne if.32.24.315.5.end
        if.32.27.315.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.315.5.end:
    func.assert.315.5.end:
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.319.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.319.5.end:
    loop.320.5:
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
        func.print.323.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.323.9.end:
        func.print.324.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.324.9.end:
        func.str.input.325.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.325.12.end:
        if.327.12:
        cmp.327.12:
        cmp byte [rbp + 1064], 0
        jle loop.320.5.end
        if.327.12.code:
        if.329.19:
        cmp.329.19:
        cmp byte [rbp + 1064], 4
        jg if.327.9.else
        if.329.19.code:
            func.print.330.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.330.13.end:
            jmp loop.320.5
        if.327.9.else:
            func.print.333.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.333.13.end:
            func.str.output.334.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 1064]
                test rdx, rdx
                js baz_bounds_panic
                cmp rdx, 127
                jg baz_bounds_panic
                lea rsi, [rbp + 1065]
                mov rax, 1
                syscall
            func.str.output.334.16.end:
            func.print.335.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.335.13.end:
            func.print.336.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.336.13.end:
        if.327.9.end:
    jmp loop.320.5
    loop.320.5.end:
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
    if.126.8:
    cmp.126.8:
    cmp qword [rbx + 32], 0
    jge if.126.5.end
    if.126.8.code:
        mov byte [rbx + 40], 1
    if.126.5.end:
    if.129.8:
    cmp.129.8:
    cmp qword [rbx + 32], 0
    jle if.129.5.end
    if.129.8.code:
        neg qword [rbx + 32]
    if.129.5.end:
    mov qword [rbx + 48], 20
    loop.134.5:
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
        if.138.12:
        cmp.138.12:
        cmp qword [rbx + 32], 0
        jne loop.134.5
        if.138.12.code:
        if.138.9.end:
    loop.134.5.end:
    if.141.8:
    cmp.141.8:
    cmp byte [rbx + 40], 0
    je if.141.5.end
    if.141.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.141.5.end:
    mov qword [rbx + 56], 0
    loop.147.5:
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
        if.151.12:
        cmp.151.12:
        cmp qword [rbx + 48], 20
        jne loop.147.5
        if.151.12.code:
        if.151.9.end:
    loop.147.5.end:
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
;[27:10] nums: i64[4] (32 B @ [rbp + 64])
;[28:1] dat str1 str = { 3 }
;[28:9] str1: str (128 B @ [rbp + 96])
;[106:7] const yes = 1
;[107:7] const no = 0
;[108:7] const maybe = -1
;
main:
;   [158:5] var answer
;   [158:9] answer: i64 (8 B @ [rbp + 224])
;   [158:9] zero 1 * 8 B = 8 B
;   [158:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
;   [160:5] assert(answer == 0)
;   [160:12] allocate scratch register -> r15
;   [160:12] ? answer == 0
;   [160:12] ? answer == 0
    cmp.160.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.160.12.end:
;   [32:6] assert(ok bool)
    func.assert.160.5:
;       [160:5] alias ok -> r15b
        if.32.27.160.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.160.5:
        cmp r15b, 0
        jne if.32.24.160.5.end
        if.32.27.160.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.160.5.end:
;       [160:5] free scratch register r15
    func.assert.160.5.end:
;   [162:5] answer = maybe
;   [162:14] maybe
    mov qword [rbp + 224], -1
;   [163:5] assert(answer == -1)
;   [163:12] allocate scratch register -> r15
;   [163:12] ? answer == -1
;   [163:12] ? answer == -1
    cmp.163.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.163.12.end:
;   [32:6] assert(ok bool)
    func.assert.163.5:
;       [163:5] alias ok -> r15b
        if.32.27.163.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.163.5:
        cmp r15b, 0
        jne if.32.24.163.5.end
        if.32.27.163.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.163.5.end:
;       [163:5] free scratch register r15
    func.assert.163.5.end:
;       [168:15] const maybe = 33
;       [169:9] assert(maybe == 33)
;       [32:6] assert(ok bool)
        func.assert.169.9:
;           [169:9] alias ok -> 1
            if.32.27.169.9:
;           [32:27] ? not ok
;           [32:27] ? shorthand: not ok
            cmp.32.27.169.9:
;           [32:31] const eval to false
            if.32.24.169.9.end:
        func.assert.169.9.end:
;   [172:5] assert(maybe == -1)
;   [32:6] assert(ok bool)
    func.assert.172.5:
;       [172:5] alias ok -> 1
        if.32.27.172.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.172.5:
;       [32:31] const eval to false
        if.32.24.172.5.end:
    func.assert.172.5.end:
;   [174:5] var arr[4] i32
;   [174:9] arr: i32[4] (16 B @ [rbp + 232])
;   [174:9] zero 4 * 4 B = 16 B
;   [174:5] size <= 32 B, use mov
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
;   [177:5] var ix = 1
;   [177:9] ix: i64 (8 B @ [rbp + 248])
;   [177:9] ix = 1
;   [177:14] 1
    mov qword [rbp + 248], 1
;   [180:5] arr[ix] = 2
;   [180:9] allocate scratch register -> r15
;   [180:9] set array index
;   [180:9] ix
    mov r15, qword [rbp + 248]
;   [180:9] bounds check
;   [180:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [180:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [180:15] 2
    mov dword [rbp + r15 * 4 + 232], 2
;   [180:5] free scratch register r15
;   [181:5] arr[ix + 1] = arr[ix]
;   [181:9] allocate scratch register -> r15
;   [181:9] set array index
;   [181:9] ix
    mov r15, qword [rbp + 248]
;   [181:9] r15 + 1
;   [181:9] src: folded constant '+ 1'
    add r15, 1
;   [181:9] bounds check
;   [181:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [181:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [181:19] arr[ix]
;   [181:23] allocate scratch register -> r14
;   [181:23] set array index
;   [181:23] ix
    mov r14, qword [rbp + 248]
;   [181:23] bounds check
;   [181:23] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
;   [181:23] upper bound (--checks=upper)
    cmp r14, 4
    jge baz_bounds_panic
;   [181:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 232]
    mov dword [rbp + r15 * 4 + 232], r13d
;   [181:19] free scratch register r13
;   [181:19] free scratch register r14
;   [181:5] free scratch register r15
;   [182:5] assert(arr[1] == 2)
;   [182:12] allocate scratch register -> r15
;   [182:12] ? arr[1] == 2
;   [182:12] ? arr[1] == 2
    cmp.182.12:
    cmp dword [rbp + 236], 2
    sete r15b
    bool.182.12.end:
;   [32:6] assert(ok bool)
    func.assert.182.5:
;       [182:5] alias ok -> r15b
        if.32.27.182.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.182.5:
        cmp r15b, 0
        jne if.32.24.182.5.end
        if.32.27.182.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.182.5.end:
;       [182:5] free scratch register r15
    func.assert.182.5.end:
;   [183:5] assert(arr[2] == 2)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? arr[2] == 2
;   [183:12] ? arr[2] == 2
    cmp.183.12:
    cmp dword [rbp + 240], 2
    sete r15b
    bool.183.12.end:
;   [32:6] assert(ok bool)
    func.assert.183.5:
;       [183:5] alias ok -> r15b
        if.32.27.183.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.183.5:
        cmp r15b, 0
        jne if.32.24.183.5.end
        if.32.27.183.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.183.5.end:
;       [183:5] free scratch register r15
    func.assert.183.5.end:
;   [185:5] array_copy(arr[2], arr, 2)
;   [185:5] allocate scratch register -> r15
;   [185:29] 2
;   [185:29] 2
    mov r15, 2
;   [185:16] arr[2]
;   [185:20] allocate scratch register -> r14
;   [185:20] set array index
;   [185:20] 2
    mov r14, 2
;   [185:20] bounds check
;   [185:20] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
;   [185:20] upper bound (--checks=upper)
;   [185:20] allocate scratch register -> r13
    mov r13, r15
    add r13, r14
    cmp r13, 4
;   [185:20] free scratch register r13
    jg baz_bounds_panic
;   [185:24] arr
;   [185:24] bounds check
;   [185:24] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [185:24] upper bound (--checks=upper)
    cmp r15, 4
    jg baz_bounds_panic
;   [185:5] size <= 16 B, use mov
;   [185:5] allocate named register rax
    mov rax, qword [rbp + r14 * 4 + 232]
    mov qword [rbp + 232], rax
;   [185:5] free named register rax
;   [185:5] free scratch register r14
;   [185:5] free scratch register r15
;   [186:5] assert(arr[0] == 2)
;   [186:12] allocate scratch register -> r15
;   [186:12] ? arr[0] == 2
;   [186:12] ? arr[0] == 2
    cmp.186.12:
    cmp dword [rbp + 232], 2
    sete r15b
    bool.186.12.end:
;   [32:6] assert(ok bool)
    func.assert.186.5:
;       [186:5] alias ok -> r15b
        if.32.27.186.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.186.5:
        cmp r15b, 0
        jne if.32.24.186.5.end
        if.32.27.186.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.186.5.end:
;       [186:5] free scratch register r15
    func.assert.186.5.end:
;   [189:5] var arr1[8] i32
;   [189:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [189:9] zero 8 * 4 B = 32 B
;   [189:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [190:5] array_copy(arr, arr1, 4)
;   [190:5] allocate scratch register -> r15
;   [190:27] 4
;   [190:27] 4
    mov r15, 4
;   [190:16] arr
;   [190:16] bounds check
;   [190:16] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [190:16] upper bound (--checks=upper)
    cmp r15, 4
    jg baz_bounds_panic
;   [190:21] arr1
;   [190:21] bounds check
;   [190:21] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [190:21] upper bound (--checks=upper)
    cmp r15, 8
    jg baz_bounds_panic
;   [190:5] size <= 16 B, use mov
;   [190:5] allocate named register rax
    mov rax, qword [rbp + 232]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 240]
    mov qword [rbp + 264], rax
;   [190:5] free named register rax
;   [190:5] free scratch register r15
;   [191:5] var eq bool = arrays_equal(arr[1], arr1[1], 3)
;   [191:9] eq: bool (1 B @ [rbp + 288])
;   [191:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [191:19] ? arrays_equal(arr[1], arr1[1], 3)
;   [191:19] ? shorthand: arrays_equal(arr[1], arr1[1], 3)
    cmp.191.19:
;       [191:19] arrays_equal(arr[1], arr1[1], 3)
;       [191:19] allocate named register rsi
;       [191:19] allocate named register rdi
;       [191:19] allocate named register rcx
;       [191:49] 3
;       [191:49] 3
        mov rcx, 3
;       [191:32] arr[1]
;       [191:36] allocate scratch register -> r15
;       [191:36] set array index
;       [191:36] 1
        mov r15, 1
;       [191:36] bounds check
;       [191:36] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
;       [191:36] upper bound (--checks=upper)
;       [191:36] allocate scratch register -> r14
        mov r14, rcx
        add r14, r15
        cmp r14, 4
;       [191:36] free scratch register r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 232]
;       [191:19] free scratch register r15
;       [191:40] arr1[1]
;       [191:45] allocate scratch register -> r15
;       [191:45] set array index
;       [191:45] 1
        mov r15, 1
;       [191:45] bounds check
;       [191:45] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
;       [191:45] upper bound (--checks=upper)
;       [191:45] allocate scratch register -> r14
        mov r14, rcx
        add r14, r15
        cmp r14, 8
;       [191:45] free scratch register r14
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 256]
;       [191:19] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [191:19] free named register rcx
;       [191:19] free named register rdi
;       [191:19] free named register rsi
        sete byte [rbp + 288]
    bool.191.19.end:
;   [194:5] assert(eq)
;   [194:12] allocate scratch register -> r15
;   [194:12] ? eq
;   [194:12] ? shorthand: eq
    cmp.194.12:
    mov r15b, byte [rbp + 288]
    bool.194.12.end:
;   [32:6] assert(ok bool)
    func.assert.194.5:
;       [194:5] alias ok -> r15b
        if.32.27.194.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.194.5:
        cmp r15b, 0
        jne if.32.24.194.5.end
        if.32.27.194.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.194.5.end:
;       [194:5] free scratch register r15
    func.assert.194.5.end:
;   [196:5] arr1[2] = -1
;   [196:15] instructions without scratch register 1, with 2
;   [196:16] -1
    mov dword [rbp + 264], -1
;   [197:5] assert(not arrays_equal(arr, arr1, 4))
;   [197:12] allocate scratch register -> r15
;   [197:12] ? not arrays_equal(arr, arr1, 4)
;   [197:12] ? shorthand: not arrays_equal(arr, arr1, 4)
    cmp.197.12:
;       [197:16] arrays_equal(arr, arr1, 4)
;       [197:16] allocate named register rsi
;       [197:16] allocate named register rdi
;       [197:16] allocate named register rcx
;       [197:40] 4
;       [197:40] 4
        mov rcx, 4
;       [197:29] arr
;       [197:29] bounds check
;       [197:29] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [197:29] upper bound (--checks=upper)
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
;       [197:34] arr1
;       [197:34] bounds check
;       [197:34] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [197:34] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [197:16] free named register rcx
;       [197:16] free named register rdi
;       [197:16] free named register rsi
        setne r15b
    bool.197.12.end:
;   [32:6] assert(ok bool)
    func.assert.197.5:
;       [197:5] alias ok -> r15b
        if.32.27.197.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.197.5:
        cmp r15b, 0
        jne if.32.24.197.5.end
        if.32.27.197.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.197.5.end:
;       [197:5] free scratch register r15
    func.assert.197.5.end:
;   [204:5] ix = 3
;   [204:10] 3
    mov qword [rbp + 248], 3
;   [205:5] var tmp i32 = ~inv(arr[ix - 1])
;   [205:9] tmp: i32 (4 B @ [rbp + 292])
;   [205:9] tmp = ~inv(arr[ix - 1])
;   [205:20] tmp = ~inv(arr[ix - 1])
;   [205:20] = expression
;   [205:20] ~inv(arr[ix - 1])
;   [205:20] instructions without scratch register 12, with 12
;   [205:28] allocate scratch register -> r15
;   [205:28] set array index
;   [205:28] ix
    mov r15, qword [rbp + 248]
;   [205:28] r15 - 1
;   [205:28] src: folded constant '- 1'
    sub r15, 1
;   [205:28] bounds check
;   [205:28] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [205:28] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [205:20] instructions without scratch register 6, with 7
;   [62:6] inv(i i32) res i32
    func.inv.205.20:
;       [205:20] alias res -> tmp
;       [205:20] alias i -> arr (lea: rbp + r15 * 4 + 232)
;       [63:5] res = ~i
;       [63:11] instructions without scratch register 3, with 3
;       [63:12] ~i
;       [63:12] allocate scratch register -> r14
        mov r14d, dword [rbp + r15 * 4 + 232]
        mov dword [rbp + 292], r14d
;       [63:12] free scratch register r14
        not dword [rbp + 292]
    func.inv.205.20.end:
    not dword [rbp + 292]
;       [205:20] free scratch register r15
;   [206:5] arr[ix] = tmp
;   [206:9] allocate scratch register -> r15
;   [206:9] set array index
;   [206:9] ix
    mov r15, qword [rbp + 248]
;   [206:9] bounds check
;   [206:9] lower bound (--checks=lower)
    test r15, r15
    js baz_bounds_panic
;   [206:9] upper bound (--checks=upper)
    cmp r15, 4
    jge baz_bounds_panic
;   [206:15] tmp
;   [206:15] allocate scratch register -> r14
    mov r14d, dword [rbp + 292]
    mov dword [rbp + r15 * 4 + 232], r14d
;   [206:15] free scratch register r14
;   [206:5] free scratch register r15
;   [207:5] assert(arr[ix] == 2)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? arr[ix] == 2
;   [207:12] ? arr[ix] == 2
    cmp.207.12:
;   [207:16] allocate scratch register -> r14
;   [207:16] set array index
;   [207:16] ix
    mov r14, qword [rbp + 248]
;   [207:16] bounds check
;   [207:16] lower bound (--checks=lower)
    test r14, r14
    js baz_bounds_panic
;   [207:16] upper bound (--checks=upper)
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
;   [207:12] free scratch register r14
    sete r15b
    bool.207.12.end:
;   [32:6] assert(ok bool)
    func.assert.207.5:
;       [207:5] alias ok -> r15b
        if.32.27.207.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.207.5:
        cmp r15b, 0
        jne if.32.24.207.5.end
        if.32.27.207.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.207.5.end:
;       [207:5] free scratch register r15
    func.assert.207.5.end:
;   [209:5] faz(arr)
;   [72:6] faz(arg[] i32)
    func.faz.209.5:
;       [209:5] alias arg -> arr
;       [73:5] arg[1] = 0xfe
;       [73:14] 0xfe
        mov dword [rbp + 236], 254
    func.faz.209.5.end:
;   [210:5] assert(arr[1] == 0xfe)
;   [210:12] allocate scratch register -> r15
;   [210:12] ? arr[1] == 0xfe
;   [210:12] ? arr[1] == 0xfe
    cmp.210.12:
    cmp dword [rbp + 236], 254
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
;   [212:5] var arr3[] = { 3, 5 }
;   [212:9] arr3: i64[2] (16 B @ [rbp + 296])
;   [212:9] arr3= { 3, 5 }
;   [212:18] size <= 16 B, use immediates
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
;   [213:5] foo arr3
;   [213:9] allocate scratch register -> r15
;   [213:9] e: i64 (r15)
;   [213:9] i: i64 (8 B @ [rbp + 320])
;   [213:9] const n = 2
;   [213:9] initiate iterator e
    lea r15, [rbp + 296]
;   [213:9] initiate counter i
    mov qword [rbp + 320], 0
    foo.213.5:
;       [214:9] e = e + i + n
;       [214:13] instructions without scratch register 3, with 4
;       [214:13] e
;       [214:17] e + i
;       [214:17] src: operand
;       [214:17] allocate scratch register -> r14
        mov r14, qword [rbp + 320]
        add qword [r15], r14
;       [214:17] free scratch register r14
;       [214:13] e + 2
;       [214:13] src: folded constant '+ n'
        add qword [r15], 2
        foo.213.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.213.5
    foo.213.5.end:
;   [213:5] free scratch register r15
;   [216:5] assert(arr3[0] == 3 + 0 + 2)
;   [216:12] allocate scratch register -> r15
;   [216:12] ? arr3[0] == 3 + 0 + 2
;   [216:12] ? arr3[0] == 3 + 0 + 2
    cmp.216.12:
;   [216:23] src: folded constant '3 + 0 + 2'
    cmp qword [rbp + 296], 5
    sete r15b
    bool.216.12.end:
;   [32:6] assert(ok bool)
    func.assert.216.5:
;       [216:5] alias ok -> r15b
        if.32.27.216.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.216.5:
        cmp r15b, 0
        jne if.32.24.216.5.end
        if.32.27.216.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.216.5.end:
;       [216:5] free scratch register r15
    func.assert.216.5.end:
;   [217:5] assert(arr3[1] == 5 + 1 + 2)
;   [217:12] allocate scratch register -> r15
;   [217:12] ? arr3[1] == 5 + 1 + 2
;   [217:12] ? arr3[1] == 5 + 1 + 2
    cmp.217.12:
;   [217:23] src: folded constant '5 + 1 + 2'
    cmp qword [rbp + 304], 8
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
;   [223:5] var p point
;   [223:9] p: point (16 B @ [rbp + 312])
;   [223:9] zero 1 * 16 B = 16 B
;   [223:5] size <= 32 B, use mov
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
;   [226:7] p.fooz()
;   [46:6] point.fooz()
    func.point.fooz.226.7:
;       [226:7] alias self -> p
;       [47:5] self.x = 0b10
;       [47:14] 0b10
        mov qword [rbp + 312], 2
;       [48:5] self.y = 0xb
;       [48:14] 0xb
        mov qword [rbp + 320], 11
    func.point.fooz.226.7.end:
;   [229:5] assert(p.x == 2)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? p.x == 2
;   [229:12] ? p.x == 2
    cmp.229.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.229.12.end:
;   [32:6] assert(ok bool)
    func.assert.229.5:
;       [229:5] alias ok -> r15b
        if.32.27.229.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.229.5:
        cmp r15b, 0
        jne if.32.24.229.5.end
        if.32.27.229.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.229.5.end:
;       [229:5] free scratch register r15
    func.assert.229.5.end:
;   [230:5] assert(p.y == 0xb)
;   [230:12] allocate scratch register -> r15
;   [230:12] ? p.y == 0xb
;   [230:12] ? p.y == 0xb
    cmp.230.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.230.12.end:
;   [32:6] assert(ok bool)
    func.assert.230.5:
;       [230:5] alias ok -> r15b
        if.32.27.230.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.230.5:
        cmp r15b, 0
        jne if.32.24.230.5.end
        if.32.27.230.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.230.5.end:
;       [230:5] free scratch register r15
    func.assert.230.5.end:
;   [232:5] var q point = p
;   [232:9] q: point (16 B @ [rbp + 328])
;   [232:9] q = p
;   [232:19] size <= 16 B, use mov
;   [232:19] allocate named register rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
;   [232:19] free named register rax
;   [235:5] assert(equal(p, q))
;   [235:12] allocate scratch register -> r15
;   [235:12] ? equal(p, q)
;   [235:12] ? shorthand: equal(p, q)
    cmp.235.12:
;       [235:12] equal(p, q)
;       [235:12] allocate named register rsi
;       [235:12] allocate named register rdi
;       [235:12] allocate named register rcx
;       [235:18] p
        lea rsi, [rbp + 312]
;       [235:21] q
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
;       [235:12] free named register rcx
;       [235:12] free named register rdi
;       [235:12] free named register rsi
        sete r15b
    bool.235.12.end:
;   [32:6] assert(ok bool)
    func.assert.235.5:
;       [235:5] alias ok -> r15b
        if.32.27.235.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.235.5:
        cmp r15b, 0
        jne if.32.24.235.5.end
        if.32.27.235.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.235.5.end:
;       [235:5] free scratch register r15
    func.assert.235.5.end:
;   [239:5] q.x = 3
;   [239:11] 3
    mov qword [rbp + 328], 3
;   [240:5] assert(not equal(p, q))
;   [240:12] allocate scratch register -> r15
;   [240:12] ? not equal(p, q)
;   [240:12] ? shorthand: not equal(p, q)
    cmp.240.12:
;       [240:16] equal(p, q)
;       [240:16] allocate named register rsi
;       [240:16] allocate named register rdi
;       [240:16] allocate named register rcx
;       [240:22] p
        lea rsi, [rbp + 312]
;       [240:25] q
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
;       [240:16] free named register rcx
;       [240:16] free named register rdi
;       [240:16] free named register rsi
        setne r15b
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
;   [242:5] var i = 0
;   [242:9] i: i64 (8 B @ [rbp + 344])
;   [242:9] i = 0
;   [242:13] 0
    mov qword [rbp + 344], 0
;   [243:5] bar(i)
;   [54:6] bar(arg)
    func.bar.243.5:
;       [243:5] alias arg -> i
        if.55.8.243.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.243.5:
        cmp qword [rbp + 344], 0
        je func.bar.243.5.end
        if.55.8.243.5.code:
;           [55:17] return
        if.55.5.243.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.243.5.end:
;   [244:5] assert(i == 0)
;   [244:12] allocate scratch register -> r15
;   [244:12] ? i == 0
;   [244:12] ? i == 0
    cmp.244.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.244.12.end:
;   [32:6] assert(ok bool)
    func.assert.244.5:
;       [244:5] alias ok -> r15b
        if.32.27.244.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.244.5:
        cmp r15b, 0
        jne if.32.24.244.5.end
        if.32.27.244.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.244.5.end:
;       [244:5] free scratch register r15
    func.assert.244.5.end:
;   [246:5] i = 1
;   [246:9] 1
    mov qword [rbp + 344], 1
;   [247:5] bar(i)
;   [54:6] bar(arg)
    func.bar.247.5:
;       [247:5] alias arg -> i
        if.55.8.247.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.247.5:
        cmp qword [rbp + 344], 0
        je func.bar.247.5.end
        if.55.8.247.5.code:
;           [55:17] return
        if.55.5.247.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.247.5.end:
;   [248:5] assert(i == 0xff)
;   [248:12] allocate scratch register -> r15
;   [248:12] ? i == 0xff
;   [248:12] ? i == 0xff
    cmp.248.12:
    cmp qword [rbp + 344], 255
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
;   [250:5] var j = 1
;   [250:9] j: i64 (8 B @ [rbp + 352])
;   [250:9] j = 1
;   [250:13] 1
    mov qword [rbp + 352], 1
;   [251:5] var k = baz(j)
;   [251:9] k: i64 (8 B @ [rbp + 360])
;   [251:9] k = baz(j)
;   [251:13] k = baz(j)
;   [251:13] = expression
;   [251:13] baz(j)
;   [66:6] baz(arg) res
    func.baz.251.13:
;       [251:13] alias res -> k
;       [251:13] alias arg -> j
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
    func.baz.251.13.end:
;   [252:5] assert(k == 2)
;   [252:12] allocate scratch register -> r15
;   [252:12] ? k == 2
;   [252:12] ? k == 2
    cmp.252.12:
    cmp qword [rbp + 360], 2
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
;   [254:5] k = baz(1)
;   [254:9] k = baz(1)
;   [254:9] = expression
;   [254:9] baz(1)
;   [66:6] baz(arg) res
    func.baz.254.9:
;       [254:9] alias res -> k
;       [254:9] alias arg -> 1
;       [67:5] res = arg * 2
;       [67:11] res = 2
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 360], 2
    func.baz.254.9.end:
;   [255:5] assert(k == 2)
;   [255:12] allocate scratch register -> r15
;   [255:12] ? k == 2
;   [255:12] ? k == 2
    cmp.255.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.255.12.end:
;   [32:6] assert(ok bool)
    func.assert.255.5:
;       [255:5] alias ok -> r15b
        if.32.27.255.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.255.5:
        cmp r15b, 0
        jne if.32.24.255.5.end
        if.32.27.255.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.255.5.end:
;       [255:5] free scratch register r15
    func.assert.255.5.end:
;   [257:5] var p0 point = {baz(3), 0}
;   [257:9] p0: point (16 B @ [rbp + 368])
;   [257:9] p0 = {baz(3), 0}
;   [257:21] copy field 'x'
;   [257:21] p0.x = baz(3)
;   [257:21] = expression
;   [257:21] baz(3)
;   [66:6] baz(arg) res
    func.baz.257.21:
;       [257:21] alias res -> p0.x (lea: rbp + 368)
;       [257:21] alias arg -> 3
;       [67:5] res = arg * 2
;       [67:11] res = 6
;       [67:11] src: folded constant 'arg * 2'
        mov qword [rbp + 368], 6
    func.baz.257.21.end:
;   [257:29] copy field 'y'
    mov qword [rbp + 376], 0
;   [258:5] assert(p0.x == 6)
;   [258:12] allocate scratch register -> r15
;   [258:12] ? p0.x == 6
;   [258:12] ? p0.x == 6
    cmp.258.12:
    cmp qword [rbp + 368], 6
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
;   [260:5] var pt = point.at(-1, -2)
;   [260:9] pt: point (16 B @ [rbp + 384])
;   [260:9] pt = point.at(-1, -2)
;   [260:14] point.at(-1, -2)
;   [90:6] point.at(x, y) self
    func.point.at.260.14:
;       [260:14] alias self -> pt
;       [260:14] alias x -> -1
;       [260:14] alias y -> -2
;       [91:5] self.x = x
;       [91:14] x
        mov qword [rbp + 384], -1
;       [92:5] self.y = y
;       [92:14] y
        mov qword [rbp + 392], -2
    func.point.at.260.14.end:
;   [264:5] assert(pt.x == -1)
;   [264:12] allocate scratch register -> r15
;   [264:12] ? pt.x == -1
;   [264:12] ? pt.x == -1
    cmp.264.12:
    cmp qword [rbp + 384], -1
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
;   [265:5] assert(pt.y == -2)
;   [265:12] allocate scratch register -> r15
;   [265:12] ? pt.y == -2
;   [265:12] ? pt.y == -2
    cmp.265.12:
    cmp qword [rbp + 392], -2
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
;   [267:8] pt.x(2)
;   [97:6] point.x(x)
    func.point.x.267.8:
;       [267:8] alias self -> pt
;       [267:8] alias x -> 2
;       [98:5] self.x = x
;       [98:14] x
        mov qword [rbp + 384], 2
    func.point.x.267.8.end:
;   [268:5] assert(pt.x == 2)
;   [268:12] allocate scratch register -> r15
;   [268:12] ? pt.x == 2
;   [268:12] ? pt.x == 2
    cmp.268.12:
    cmp qword [rbp + 384], 2
    sete r15b
    bool.268.12.end:
;   [32:6] assert(ok bool)
    func.assert.268.5:
;       [268:5] alias ok -> r15b
        if.32.27.268.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.268.5:
        cmp r15b, 0
        jne if.32.24.268.5.end
        if.32.27.268.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.268.5.end:
;       [268:5] free scratch register r15
    func.assert.268.5.end:
;   [270:5] var x = 1
;   [270:9] x: i64 (8 B @ [rbp + 400])
;   [270:9] x = 1
;   [270:13] 1
    mov qword [rbp + 400], 1
;   [271:5] var y = 2
;   [271:9] y: i64 (8 B @ [rbp + 408])
;   [271:9] y = 2
;   [271:13] 2
    mov qword [rbp + 408], 2
;   [273:5] var o1 object = {{x * 10, y}, 0xff0000}
;   [273:9] o1: object (24 B @ [rbp + 416])
;   [273:9] o1 = {{x * 10, y}, 0xff0000}
;   [273:22] copy field 'pos'
;   [273:23] copy field 'x'
;   [273:23] instructions without scratch register 5, with 3
;   [273:23] allocate scratch register -> r15
;   [273:23] x
    mov r15, qword [rbp + 400]
;   [273:23] r15 * 10
;   [273:23] src: folded constant '* 10'
    imul r15, 10
    mov qword [rbp + 416], r15
;   [273:23] free scratch register r15
;   [273:31] copy field 'y'
;   [273:31] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
;   [273:31] free scratch register r15
;   [273:35] copy field 'color'
    mov dword [rbp + 432], 16711680
;   [273:21] zero padding: 4 B
;   [273:21] size <= 32 B, use mov
    mov dword [rbp + 436], 0
;   [274:5] assert(o1.pos.x == 10)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? o1.pos.x == 10
;   [274:12] ? o1.pos.x == 10
    cmp.274.12:
    cmp qword [rbp + 416], 10
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
;   [275:5] assert(o1.pos.y == 2)
;   [275:12] allocate scratch register -> r15
;   [275:12] ? o1.pos.y == 2
;   [275:12] ? o1.pos.y == 2
    cmp.275.12:
    cmp qword [rbp + 424], 2
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
;   [276:5] assert(o1.color == 0xff0000)
;   [276:12] allocate scratch register -> r15
;   [276:12] ? o1.color == 0xff0000
;   [276:12] ? o1.color == 0xff0000
    cmp.276.12:
    cmp dword [rbp + 432], 16711680
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
;   [278:5] var p1 point = {-x, -y}
;   [278:9] p1: point (16 B @ [rbp + 440])
;   [278:9] p1 = {-x, -y}
;   [278:21] copy field 'x'
;   [278:21] instructions without scratch register 3, with 3
;   [278:21] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 440], r15
;   [278:21] free scratch register r15
    neg qword [rbp + 440]
;   [278:25] copy field 'y'
;   [278:25] instructions without scratch register 3, with 3
;   [278:25] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 448], r15
;   [278:25] free scratch register r15
    neg qword [rbp + 448]
;   [279:5] o1.pos = p1
;   [279:14] size <= 16 B, use mov
;   [279:14] allocate named register rax
    mov rax, qword [rbp + 440]
    mov qword [rbp + 416], rax
    mov rax, qword [rbp + 448]
    mov qword [rbp + 424], rax
;   [279:14] free named register rax
;   [280:5] assert(o1.pos.x == -1)
;   [280:12] allocate scratch register -> r15
;   [280:12] ? o1.pos.x == -1
;   [280:12] ? o1.pos.x == -1
    cmp.280.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.280.12.end:
;   [32:6] assert(ok bool)
    func.assert.280.5:
;       [280:5] alias ok -> r15b
        if.32.27.280.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.280.5:
        cmp r15b, 0
        jne if.32.24.280.5.end
        if.32.27.280.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.280.5.end:
;       [280:5] free scratch register r15
    func.assert.280.5.end:
;   [281:5] assert(o1.pos.y == -2)
;   [281:12] allocate scratch register -> r15
;   [281:12] ? o1.pos.y == -2
;   [281:12] ? o1.pos.y == -2
    cmp.281.12:
    cmp qword [rbp + 424], -2
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
;   [283:5] var o2 object = o1
;   [283:9] o2: object (24 B @ [rbp + 456])
;   [283:9] o2 = o1
;   [283:21] allocate named register rsi
;   [283:21] allocate named register rdi
;   [283:21] allocate named register rcx
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
;   [283:21] free named register rcx
;   [283:21] free named register rdi
;   [283:21] free named register rsi
;   [284:5] assert(o2.pos.x == -1)
;   [284:12] allocate scratch register -> r15
;   [284:12] ? o2.pos.x == -1
;   [284:12] ? o2.pos.x == -1
    cmp.284.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.284.12.end:
;   [32:6] assert(ok bool)
    func.assert.284.5:
;       [284:5] alias ok -> r15b
        if.32.27.284.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.284.5:
        cmp r15b, 0
        jne if.32.24.284.5.end
        if.32.27.284.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.284.5.end:
;       [284:5] free scratch register r15
    func.assert.284.5.end:
;   [285:5] assert(o2.pos.y == -2)
;   [285:12] allocate scratch register -> r15
;   [285:12] ? o2.pos.y == -2
;   [285:12] ? o2.pos.y == -2
    cmp.285.12:
    cmp qword [rbp + 464], -2
    sete r15b
    bool.285.12.end:
;   [32:6] assert(ok bool)
    func.assert.285.5:
;       [285:5] alias ok -> r15b
        if.32.27.285.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.285.5:
        cmp r15b, 0
        jne if.32.24.285.5.end
        if.32.27.285.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.285.5.end:
;       [285:5] free scratch register r15
    func.assert.285.5.end:
;   [286:5] assert(o2.color == 0xff0000)
;   [286:12] allocate scratch register -> r15
;   [286:12] ? o2.color == 0xff0000
;   [286:12] ? o2.color == 0xff0000
    cmp.286.12:
    cmp dword [rbp + 472], 16711680
    sete r15b
    bool.286.12.end:
;   [32:6] assert(ok bool)
    func.assert.286.5:
;       [286:5] alias ok -> r15b
        if.32.27.286.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.286.5:
        cmp r15b, 0
        jne if.32.24.286.5.end
        if.32.27.286.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.286.5.end:
;       [286:5] free scratch register r15
    func.assert.286.5.end:
;   [288:5] var o3[2] object
;   [288:9] o3: object[2] (48 B @ [rbp + 480])
;   [288:9] zero 2 * 24 B = 48 B
;   [288:5] allocate named register rax
;   [288:5] allocate named register rdi
;   [288:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
;   [288:5] free named register rcx
;   [288:5] free named register rdi
;   [288:5] free named register rax
;   [289:5] o3[0].pos.y = 73
;   [289:19] 73
    mov qword [rbp + 488], 73
;   [291:5] assert(o3[0].pos.y == 73)
;   [291:12] allocate scratch register -> r15
;   [291:12] ? o3[0].pos.y == 73
;   [291:12] ? o3[0].pos.y == 73
    cmp.291.12:
    cmp qword [rbp + 488], 73
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
;   [292:5] o3[1] = object.at(2, 74, 0xffffff)
;   [292:13] object.at(2, 74, 0xffffff)
;   [101:6] object.at(x, y, color i32) self
    func.object.at.292.13:
;       [292:13] alias self -> o3 (lea: rbp + 504)
;       [292:13] alias x -> 2
;       [292:13] alias y -> 74
;       [292:13] alias color -> 16777215
;       [102:5] self.pos = point.at(x, y)
;       [102:16] point.at(x, y)
;       [90:6] point.at(x, y) self
        func.point.at.102.16.292.13:
;           [102:16] alias self -> self.pos (lea: rbp + 504)
;           [102:16] alias x -> 2
;           [102:16] alias y -> 74
;           [91:5] self.x = x
;           [91:14] x
            mov qword [rbp + 504], 2
;           [92:5] self.y = y
;           [92:14] y
            mov qword [rbp + 512], 74
        func.point.at.102.16.292.13.end:
;       [103:5] self.color = color
;       [103:18] color
        mov dword [rbp + 520], 16777215
    func.object.at.292.13.end:
;   [293:5] assert(o3[1].pos.y == 74)
;   [293:12] allocate scratch register -> r15
;   [293:12] ? o3[1].pos.y == 74
;   [293:12] ? o3[1].pos.y == 74
    cmp.293.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.293.12.end:
;   [32:6] assert(ok bool)
    func.assert.293.5:
;       [293:5] alias ok -> r15b
        if.32.27.293.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.293.5:
        cmp r15b, 0
        jne if.32.24.293.5.end
        if.32.27.293.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.293.5.end:
;       [293:5] free scratch register r15
    func.assert.293.5.end:
;   [295:5] var worlds[8] world
;   [295:9] worlds: world[8] (512 B @ [rbp + 528])
;   [295:9] zero 8 * 64 B = 512 B
;   [295:5] allocate named register rax
;   [295:5] allocate named register rdi
;   [295:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
;   [295:5] free named register rcx
;   [295:5] free named register rdi
;   [295:5] free named register rax
;   [296:5] worlds[1].locations[1] = 0xffee
;   [296:30] 0xffee
    mov qword [rbp + 600], 65518
;   [297:5] assert(worlds[1].locations[1] == 0xffee)
;   [297:12] allocate scratch register -> r15
;   [297:12] ? worlds[1].locations[1] == 0xffee
;   [297:12] ? worlds[1].locations[1] == 0xffee
    cmp.297.12:
    cmp qword [rbp + 600], 65518
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
;   [299:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds[0].locations) )
;   [299:5] allocate named register rsi
;   [299:5] allocate named register rdi
;   [299:5] allocate named register rcx
;   [302:9] array_length(worlds[0].locations)
;   [302:9] rcx = array_length(worlds[0].locations)
;   [302:9] = expression
;   [302:9] array_length(worlds[0].locations)
    mov rcx, 8
;   [300:9] worlds[1].locations
;   [300:9] bounds check
;   [300:9] lower bound (--checks=lower)
    test rcx, rcx
    js baz_bounds_panic
;   [300:9] upper bound (--checks=upper)
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 592]
;   [301:9] worlds[0].locations
;   [301:9] bounds check
;   [301:9] lower bound (--checks=lower)
    test rcx, rcx
    js baz_bounds_panic
;   [301:9] upper bound (--checks=upper)
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 528]
    shl rcx, 3
    rep movsb
;   [299:5] free named register rcx
;   [299:5] free named register rdi
;   [299:5] free named register rsi
;   [306:5] assert(worlds[0].locations[1] == 0xffee)
;   [306:12] allocate scratch register -> r15
;   [306:12] ? worlds[0].locations[1] == 0xffee
;   [306:12] ? worlds[0].locations[1] == 0xffee
    cmp.306.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.306.12.end:
;   [32:6] assert(ok bool)
    func.assert.306.5:
;       [306:5] alias ok -> r15b
        if.32.27.306.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.306.5:
        cmp r15b, 0
        jne if.32.24.306.5.end
        if.32.27.306.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.306.5.end:
;       [306:5] free scratch register r15
    func.assert.306.5.end:
;   [307:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) ))
;   [307:12] allocate scratch register -> r15
;   [307:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;   [307:12] ? shorthand: arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
    cmp.307.12:
;       [307:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds[0].locations) )
;       [307:12] allocate named register rsi
;       [307:12] allocate named register rdi
;       [307:12] allocate named register rcx
;       [310:14] array_length(worlds[0].locations)
;       [310:14] rcx = array_length(worlds[0].locations)
;       [310:14] = expression
;       [310:14] array_length(worlds[0].locations)
        mov rcx, 8
;       [308:14] worlds[0].locations
;       [308:14] bounds check
;       [308:14] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [308:14] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 528]
;       [309:14] worlds[1].locations
;       [309:14] bounds check
;       [309:14] lower bound (--checks=lower)
        test rcx, rcx
        js baz_bounds_panic
;       [309:14] upper bound (--checks=upper)
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 592]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [307:12] free named register rcx
;       [307:12] free named register rdi
;       [307:12] free named register rsi
        sete r15b
    bool.307.12.end:
;   [32:6] assert(ok bool)
    func.assert.307.5:
;       [307:5] alias ok -> r15b
        if.32.27.307.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.307.5:
        cmp r15b, 0
        jne if.32.24.307.5.end
        if.32.27.307.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.307.5.end:
;       [307:5] free scratch register r15
    func.assert.307.5.end:
;   [312:5] var arr2[] = { -1, 2 }
;   [312:9] arr2: i64[2] (16 B @ [rbp + 1040])
;   [312:9] arr2= { -1, 2 }
;   [312:18] size <= 16 B, use immediates
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
;   [313:5] assert(array_length(arr2) == 2)
;   [313:12] allocate scratch register -> r15
;   [313:12] ? array_length(arr2) == 2
;   [313:12] ? array_length(arr2) == 2
    cmp.313.12:
;   [313:12] allocate scratch register -> r14
;       [313:12] r14 = array_length(arr2)
;       [313:12] = expression
;       [313:12] array_length(arr2)
        mov r14, 2
    cmp r14, 2
;   [313:12] free scratch register r14
    sete r15b
    bool.313.12.end:
;   [32:6] assert(ok bool)
    func.assert.313.5:
;       [313:5] alias ok -> r15b
        if.32.27.313.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.313.5:
        cmp r15b, 0
        jne if.32.24.313.5.end
        if.32.27.313.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.313.5.end:
;       [313:5] free scratch register r15
    func.assert.313.5.end:
;   [314:5] assert(arr2[0] == -1)
;   [314:12] allocate scratch register -> r15
;   [314:12] ? arr2[0] == -1
;   [314:12] ? arr2[0] == -1
    cmp.314.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.314.12.end:
;   [32:6] assert(ok bool)
    func.assert.314.5:
;       [314:5] alias ok -> r15b
        if.32.27.314.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.314.5:
        cmp r15b, 0
        jne if.32.24.314.5.end
        if.32.27.314.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.314.5.end:
;       [314:5] free scratch register r15
    func.assert.314.5.end:
;   [315:5] assert(arr2[1] == 2)
;   [315:12] allocate scratch register -> r15
;   [315:12] ? arr2[1] == 2
;   [315:12] ? arr2[1] == 2
    cmp.315.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.315.12.end:
;   [32:6] assert(ok bool)
    func.assert.315.5:
;       [315:5] alias ok -> r15b
        if.32.27.315.5:
;       [32:27] ? not ok
;       [32:27] ? shorthand: not ok
        cmp.32.27.315.5:
        cmp r15b, 0
        jne if.32.24.315.5.end
        if.32.27.315.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.315.5.end:
;       [315:5] free scratch register r15
    func.assert.315.5.end:
;   [317:5] var counter
;   [317:9] counter: i64 (8 B @ [rbp + 1056])
;   [317:9] zero 1 * 8 B = 8 B
;   [317:5] size <= 32 B, use mov
    mov qword [rbp + 1056], 0
;   [318:5] var nm str
;   [318:9] nm: str (128 B @ [rbp + 1064])
;   [318:9] zero 1 * 128 B = 128 B
;   [318:5] allocate named register rax
;   [318:5] allocate named register rdi
;   [318:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
;   [318:5] free named register rcx
;   [318:5] free named register rdi
;   [318:5] free named register rax
;   [319:5] print(hello)
;   [35:6] print(str[] i8)
    func.print.319.5:
;       [319:5] alias str -> hello
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
    func.print.319.5.end:
;   [320:5] label
    loop.320.5:
;       [321:9] counter = counter + 1
;       [321:19] instructions without scratch register 1, with 3
;       [321:19] counter
;       [321:19] counter + 1
;       [321:19] src: folded constant '+ 1'
        add qword [rbp + 1056], 1
;       [322:9] print_num(counter)
;       [322:9] frame capacity check (--checks=frame)
;       [322:9] allocate scratch register -> r15
;       [322:9] allocate scratch register -> r14
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
;       [322:9] free scratch register r14
;       [322:9] free scratch register r15
;       [322:9] address of argument 'counter' to parameter 'num'
;       [322:9] allocate scratch register -> r15
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
;       [322:9] free scratch register r15
;       [322:9] set function frame base
        lea rbx, [rbp + 1192]
        call func.print_num
;       [323:9] print(colon)
;       [35:6] print(str[] i8)
        func.print.323.9:
;           [323:9] alias str -> colon
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
        func.print.323.9.end:
;       [324:9] print(prompt1)
;       [35:6] print(str[] i8)
        func.print.324.9:
;           [324:9] alias str -> prompt1
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
        func.print.324.9.end:
;       [325:12] nm.input()
;       [76:6] str.input()
        func.str.input.325.12:
;           [325:12] alias self -> nm
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
        func.str.input.325.12.end:
        if.327.12:
;       [327:12] ? nm.len <= 0
;       [327:12] ? nm.len <= 0
        cmp.327.12:
        cmp byte [rbp + 1064], 0
        jle loop.320.5.end
        if.327.12.code:
;           [328:13] break
        if.329.19:
;       [329:19] ? nm.len <= 4
;       [329:19] ? nm.len <= 4
        cmp.329.19:
        cmp byte [rbp + 1064], 4
        jg if.327.9.else
        if.329.19.code:
;           [330:13] print(prompt2)
;           [35:6] print(str[] i8)
            func.print.330.13:
;               [330:13] alias str -> prompt2
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
            func.print.330.13.end:
;           [331:13] continue
            jmp loop.320.5
        if.327.9.else:
;           [333:13] print(prompt3)
;           [35:6] print(str[] i8)
            func.print.333.13:
;               [333:13] alias str -> prompt3
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
            func.print.333.13.end:
;           [334:16] nm.output()
;           [83:6] str.output()
            func.str.output.334.16:
;               [334:16] alias self -> nm
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
            func.str.output.334.16.end:
;           [335:13] print(dot)
;           [35:6] print(str[] i8)
            func.print.335.13:
;               [335:13] alias str -> dot
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
            func.print.335.13.end:
;           [336:13] print(nl)
;           [35:6] print(str[] i8)
            func.print.336.13:
;               [336:13] alias str -> nl
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
            func.print.336.13.end:
        if.327.9.end:
    jmp loop.320.5
    loop.320.5.end:
    mov rdi, 0
    mov rax, 60
    syscall

;
;[116:15] noinline print_num(num)
func.print_num:
;   [116:25] num: i64 (8 B @ [rbx])
;   [118:11] const buf_count = 20
;   [120:5] var buf[buf_count] i8
;   [120:9] buf: i8[20] (20 B @ [rbx + 8])
;   [120:9] zero 20 * 1 B = 20 B
;   [120:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [121:5] var n = num
;   [121:9] n: i64 (8 B @ [rbx + 32])
;   [121:9] n = num
;   [121:13] num
;   [121:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [121:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [121:13] free scratch register r14
;   [121:13] free scratch register r15
;   [122:5] var is_negative bool
;   [122:9] is_negative: bool (1 B @ [rbx + 40])
;   [122:9] zero 1 * 1 B = 1 B
;   [122:5] size <= 32 B, use mov
    mov byte [rbx + 40], 0
    if.126.8:
;   [126:8] ? n < 0
;   [126:8] ? n < 0
    cmp.126.8:
    cmp qword [rbx + 32], 0
    jge if.126.5.end
    if.126.8.code:
;       [127:9] is_negative = true
        mov byte [rbx + 40], 1
    if.126.5.end:
    if.129.8:
;   [129:8] ? n > 0
;   [129:8] ? n > 0
    cmp.129.8:
    cmp qword [rbx + 32], 0
    jle if.129.5.end
    if.129.8.code:
;       [130:9] n = -n
;       [130:13] instructions without scratch register 1, with 3
;       [130:14] -n
        neg qword [rbx + 32]
    if.129.5.end:
;   [133:5] var i = buf_count
;   [133:9] i: i64 (8 B @ [rbx + 48])
;   [133:9] i = buf_count
;   [133:13] buf_count
    mov qword [rbx + 48], 20
;   [134:5] label
    loop.134.5:
;       [135:9] i = i - 1
;       [135:13] instructions without scratch register 1, with 3
;       [135:13] i
;       [135:13] i - 1
;       [135:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
;       [136:9] buf[i] = i8('0' - n % 10)
;       [136:13] allocate scratch register -> r15
;       [136:13] set array index
;       [136:13] i
        mov r15, qword [rbx + 48]
;       [136:13] bounds check
;       [136:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [136:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [136:18] buf = i8('0' - n % 10)
;       [136:18] = expression
;       [136:18] allocate scratch register -> r14
;           [136:21] r14 = 48
;           [136:21] src: folded constant '+ '0''
            mov r14, 48
;           [136:29] r14 - n % 10
;           [136:29] src: expression
;           [136:29] allocate scratch register -> r13
;           [136:27] n
            mov r13, qword [rbx + 32]
;           [136:31] r13 % 10
;           [136:31] src: constant
;           [136:31] allocate named register rax
            mov rax, r13
;           [136:31] allocate named register rdx
            cqo
;           [136:31] allocate scratch register -> r12
            mov r12, 10
            idiv r12
;           [136:31] free scratch register r12
            mov r13, rdx
;           [136:31] free named register rdx
;           [136:31] free named register rax
            sub r14, r13
;           [136:29] free scratch register r13
        mov byte [rbx + r15 + 8], r14b
;       [136:18] free scratch register r14
;       [136:9] free scratch register r15
;       [137:9] n = n / 10
;       [137:13] instructions without scratch register 5, with 7
;       [137:13] n
;       [137:17] n / 10
;       [137:17] src: constant
;       [137:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [137:17] allocate named register rdx
        cqo
;       [137:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [137:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [137:17] free named register rdx
;       [137:17] free named register rax
        if.138.12:
;       [138:12] ? n == 0
;       [138:12] ? n == 0
        cmp.138.12:
        cmp qword [rbx + 32], 0
        jne loop.134.5
        if.138.12.code:
;           [138:19] break
        if.138.9.end:
    loop.134.5.end:
    if.141.8:
;   [141:8] ? is_negative
;   [141:8] ? shorthand: is_negative
    cmp.141.8:
    cmp byte [rbx + 40], 0
    je if.141.5.end
    if.141.8.code:
;       [142:9] i = i - 1
;       [142:13] instructions without scratch register 1, with 3
;       [142:13] i
;       [142:13] i - 1
;       [142:13] src: folded constant '- 1'
        sub qword [rbx + 48], 1
;       [143:9] buf[i] = '-'
;       [143:13] allocate scratch register -> r15
;       [143:13] set array index
;       [143:13] i
        mov r15, qword [rbx + 48]
;       [143:13] bounds check
;       [143:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [143:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [143:18] '-'
        mov byte [rbx + r15 + 8], 45
;       [143:9] free scratch register r15
    if.141.5.end:
;   [146:5] var write_pos
;   [146:9] write_pos: i64 (8 B @ [rbx + 56])
;   [146:9] zero 1 * 8 B = 8 B
;   [146:5] size <= 32 B, use mov
    mov qword [rbx + 56], 0
;   [147:5] label
    loop.147.5:
;       [148:9] buf[write_pos] = buf[i]
;       [148:13] allocate scratch register -> r15
;       [148:13] set array index
;       [148:13] write_pos
        mov r15, qword [rbx + 56]
;       [148:13] bounds check
;       [148:13] lower bound (--checks=lower)
        test r15, r15
        js baz_bounds_panic
;       [148:13] upper bound (--checks=upper)
        cmp r15, 20
        jge baz_bounds_panic
;       [148:26] buf[i]
;       [148:30] allocate scratch register -> r14
;       [148:30] set array index
;       [148:30] i
        mov r14, qword [rbx + 48]
;       [148:30] bounds check
;       [148:30] lower bound (--checks=lower)
        test r14, r14
        js baz_bounds_panic
;       [148:30] upper bound (--checks=upper)
        cmp r14, 20
        jge baz_bounds_panic
;       [148:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [148:26] free scratch register r13
;       [148:26] free scratch register r14
;       [148:9] free scratch register r15
;       [149:9] write_pos = write_pos + 1
;       [149:21] instructions without scratch register 1, with 3
;       [149:21] write_pos
;       [149:21] write_pos + 1
;       [149:21] src: folded constant '+ 1'
        add qword [rbx + 56], 1
;       [150:9] i = i + 1
;       [150:13] instructions without scratch register 1, with 3
;       [150:13] i
;       [150:13] i + 1
;       [150:13] src: folded constant '+ 1'
        add qword [rbx + 48], 1
        if.151.12:
;       [151:12] ? i == buf_count
;       [151:12] ? i == buf_count
        cmp.151.12:
        cmp qword [rbx + 48], 20
        jne loop.147.5
        if.151.12.code:
;           [151:27] break
        if.151.9.end:
    loop.147.5.end:
;   [154:5] write(1, buf, write_pos)
;   [154:5] allocate named register rdi
;   [154:5] allocate named register rsi
;   [154:5] allocate named register rdx
;   [154:11] 1
    mov rdi, 1
;   [154:19] write_pos
    mov rdx, qword [rbx + 56]
;   [154:14] bounds check
;   [154:14] lower bound (--checks=lower)
    test rdx, rdx
    js baz_bounds_panic
;   [154:14] upper bound (--checks=upper)
    cmp rdx, 20
    jg baz_bounds_panic
    lea rsi, [rbx + 8]
;   [154:5] allocate named register rax
    mov rax, 1
    syscall
;   [154:5] free named register rax
;   [154:5] free named register rdx
;   [154:5] free named register rsi
;   [154:5] free named register rdi
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
;[27:10] nums
; i64[4]
;[27:22] [0]
;[27:22] i64
dq 1
; pad 3 'i64' of size 8
times 24 db 0
;[28:9] str1
;[28:22] i8
db 3
;[28:20] zero remaining fields: 127 B
times 127 db 0
dat.end:

section .bss.vars nobits alloc write
align 16
vars:
resb 131072
vars.end:
; free named register rbp

;   removed jumps to next code: 90
;    removed unreachable jumps: 2
; removed same target branches: 34
; inverted branches over jumps: 7

; max scratch registers in use: 4
;            max frames in use: 8
;              dat var padding: 0 B
;                max vars size: 976 B
```
