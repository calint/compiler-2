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
  `continue`, `break`, `return`
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `equal`, `read`,
  `write`, `exit`, `i8`, `i16`, `i32`, `i64`

## Howto

* to compile the compiler, then compile and run `prog.baz` run `./make.sh`
  (`./make.sh build` only compiles the compiler)
* after that use `./run.sh myprogram.baz` to compile, assemble and run a
  program, `./run.sh` alone uses `prog.baz`
  * writes `myprogram.s`, `myprogram-without-comments.s`, `myprogram.o` and the
    binary `myprogram`
  * optional parameters: _variable storage size_, _bounds check_, with _line number
    information_ and _jump optimizations_ e.g:
    * `./run.sh myprogram.baz --vars=262144`: reserves 262144 bytes for
      variables, no runtime checks
    * `./run.sh myprogram.baz --vars=262144 --checks=upper`: checks upper
      bounds without line number information and is often enough to ensure
      catching negative values (faster)
    * `./run.sh myprogram.baz --vars=262144 --checks=upper,line`: checks
      upper bounds with line number information
    * `./run.sh myprogram.baz --vars=262144 --checks=upper,lower,line`: checks
      bounds with line number information
    * option `--vars=SIZE` reserves variable storage in bytes (default: 65536,
      decimal or `0x` hex, positive multiple of 16)
    * option `--checks=TYPE` also accepts `frame` (non-inlined function frame
      capacity) and `alias` (calls where a result or argument may share
      storage)
    * option `--nopt` disables post processing jump optimizations
    * option `--reproduce-source` writes reproduced source to `diff.baz`
      and checks that it matches the input
    * to compile for rv32i and run in QEMU user mode use `--target=rv32i`
    * to compile a bare-metal image `gen-rv32i.bin` and run it on the QEMU
      `virt` machine use `--target=rv32i-qemu` (option `--stack=SIZE` sets
      the stack size, default: 65536, multiple of 16)
    * to compile a bare-metal image `gen-rv32i.bin` and run it in the fpga
      soft core emulator use `--target=rv32i-fpga`
* to run the tests `qa/coverage/test-all.sh` and see coverage report in
  `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Related

* rv32i soft core fpga implementation running `gen-rv32i.bin` compiled from
  `prog-uart.baz`
  * <https://github.com/calint/tang-nano-9k--riscv--cache-psram>
  * <https://github.com/calint/tang-nano-20k--riscv--cache-sdram>
  * to compile and run in an emulator of fpga soft core rv32i:
    `./run.sh prog-uart.baz --target=rv32i-fpga`
  * to run on hardware use `scripts/fpga-connect-serial.sh`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    55           4739           1510          16343
C++                              1             58             10            273
-------------------------------------------------------------------------------
SUM:                            56           4797           1520          16616
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
    data[127] i8
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

func point_init() res point {
    res.x = -1
    res.y = -2
}

func object_init() res object {
    res.pos.x = 2
    res.pos.y = 74
    res.color = 0xffffff
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

    var pt point = point_init()
    # "return" from functions are writing to the destination through a reference
 
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
    o3.pos.y = 73
    # index 0 in an array can be accessed without array index

    assert(o3[0].pos.y == 73)
    o3[1] = object_init()
    assert(o3[1].pos.y == 74)

    var worlds[8] world
    worlds[1].locations[1] = 0xffee
    assert(worlds[1].locations[1] == 0xffee)

    array_copy(
        worlds[1].locations,
        worlds[0].locations,
        array_length(worlds.locations)
    )
    # `array_length` is built-in

    assert(worlds[0].locations[1] == 0xffee)
    assert(arrays_equal(
             worlds[0].locations,
             worlds[1].locations,
             array_length(worlds.locations)
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
%macro PUSH_REGS 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro POP_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 224], 0
    cmp.152.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.152.12.end:
    func.assert.152.5:
        if.32.27.152.5:
        cmp.32.27.152.5:
        cmp r15b, 0
        jne if.32.24.152.5.end
        if.32.27.152.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.152.5.end:
    func.assert.152.5.end:
    mov qword [rbp + 224], -1
    cmp.155.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.155.12.end:
    func.assert.155.5:
        if.32.27.155.5:
        cmp.32.27.155.5:
        cmp r15b, 0
        jne if.32.24.155.5.end
        if.32.27.155.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.155.5.end:
    func.assert.155.5.end:
        func.assert.161.9:
            if.32.27.161.9:
            cmp.32.27.161.9:
            if.32.24.161.9.end:
        func.assert.161.9.end:
    func.assert.164.5:
        if.32.27.164.5:
        cmp.32.27.164.5:
        if.32.24.164.5.end:
    func.assert.164.5.end:
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 172
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 232], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 173
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 232]
    mov dword [rbp + r15 * 4 + 232], r13d
    cmp.174.12:
    mov r14, 1
    mov r13, 174
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.174.12.end:
    func.assert.174.5:
        if.32.27.174.5:
        cmp.32.27.174.5:
        cmp r15b, 0
        jne if.32.24.174.5.end
        if.32.27.174.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.174.5.end:
    func.assert.174.5.end:
    cmp.175.12:
    mov r14, 2
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.175.12.end:
    func.assert.175.5:
        if.32.27.175.5:
        cmp.32.27.175.5:
        cmp r15b, 0
        jne if.32.24.175.5.end
        if.32.27.175.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.175.5.end:
    func.assert.175.5.end:
    mov r15, 2
    mov r14, 2
    mov r13, 177
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
    mov r13, 177
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
    mov rax, qword [rbp + r14 * 4 + 232]
    mov qword [rbp + 232], rax
    cmp.178.12:
    mov r14, 0
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.178.12.end:
    func.assert.178.5:
        if.32.27.178.5:
        cmp.32.27.178.5:
        cmp r15b, 0
        jne if.32.24.178.5.end
        if.32.27.178.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.178.5.end:
    func.assert.178.5.end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov r15, 4
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    mov r14, 182
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
    cmp.183.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 183
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
        mov r14, 183
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
    bool.183.19.end:
    cmp.186.12:
    cmp byte [rbp + 288], 0
    setne r15b
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
    mov r15, 2
    mov r14, 188
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp.189.12:
        mov rcx, 4
        mov r14, 189
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
        mov r14, 189
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
    bool.189.12.end:
    func.assert.189.5:
        if.32.27.189.5:
        cmp.32.27.189.5:
        cmp r15b, 0
        jne if.32.24.189.5.end
        if.32.27.189.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.189.5.end:
    func.assert.189.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 192
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 192
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.192.16:
        mov r13d, dword [rbp + r14 * 4 + 232]
        mov dword [rbp + r15 * 4 + 232], r13d
        not dword [rbp + r15 * 4 + 232]
    func.inv.192.16.end:
    not dword [rbp + r15 * 4 + 232]
    cmp.193.12:
    mov r14, qword [rbp + 248]
    mov r13, 193
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.193.12.end:
    func.assert.193.5:
        if.32.27.193.5:
        cmp.32.27.193.5:
        cmp r15b, 0
        jne if.32.24.193.5.end
        if.32.27.193.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.193.5.end:
    func.assert.193.5.end:
    func.faz.195.5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 232], 254
    func.faz.195.5.end:
    cmp.196.12:
    mov r14, 1
    mov r13, 196
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 254
    sete r15b
    bool.196.12.end:
    func.assert.196.5:
        if.32.27.196.5:
        cmp.32.27.196.5:
        cmp r15b, 0
        jne if.32.24.196.5.end
        if.32.27.196.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.196.5.end:
    func.assert.196.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.199.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.199.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.199.5
    foo.199.5.end:
    cmp.202.12:
    mov r14, 0
    mov r13, 202
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
    sete r15b
    bool.202.12.end:
    func.assert.202.5:
        if.32.27.202.5:
        cmp.32.27.202.5:
        cmp r15b, 0
        jne if.32.24.202.5.end
        if.32.27.202.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.202.5.end:
    func.assert.202.5.end:
    cmp.203.12:
    mov r14, 1
    mov r13, 203
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
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
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.212.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.212.7.end:
    cmp.215.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.215.12.end:
    func.assert.215.5:
        if.32.27.215.5:
        cmp.32.27.215.5:
        cmp r15b, 0
        jne if.32.24.215.5.end
        if.32.27.215.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.215.5.end:
    func.assert.215.5.end:
    cmp.216.12:
    cmp qword [rbp + 320], 11
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
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.221.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool.221.12.end:
    func.assert.221.5:
        if.32.27.221.5:
        cmp.32.27.221.5:
        cmp r15b, 0
        jne if.32.24.221.5.end
        if.32.27.221.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.221.5.end:
    func.assert.221.5.end:
    mov qword [rbp + 328], 3
    cmp.226.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool.226.12.end:
    func.assert.226.5:
        if.32.27.226.5:
        cmp.32.27.226.5:
        cmp r15b, 0
        jne if.32.24.226.5.end
        if.32.27.226.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.226.5.end:
    func.assert.226.5.end:
    mov qword [rbp + 344], 0
    func.bar.229.5:
        if.55.8.229.5:
        cmp.55.8.229.5:
        cmp qword [rbp + 344], 0
        je func.bar.229.5.end
        if.55.8.229.5.code:
        if.55.5.229.5.end:
        mov qword [rbp + 344], 255
    func.bar.229.5.end:
    cmp.230.12:
    cmp qword [rbp + 344], 0
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
    mov qword [rbp + 344], 1
    func.bar.233.5:
        if.55.8.233.5:
        cmp.55.8.233.5:
        cmp qword [rbp + 344], 0
        je func.bar.233.5.end
        if.55.8.233.5.code:
        if.55.5.233.5.end:
        mov qword [rbp + 344], 255
    func.bar.233.5.end:
    cmp.234.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.234.12.end:
    func.assert.234.5:
        if.32.27.234.5:
        cmp.32.27.234.5:
        cmp r15b, 0
        jne if.32.24.234.5.end
        if.32.27.234.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.234.5.end:
    func.assert.234.5.end:
    mov qword [rbp + 352], 1
    func.baz.237.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.237.13.end:
    cmp.238.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.238.12.end:
    func.assert.238.5:
        if.32.27.238.5:
        cmp.32.27.238.5:
        cmp r15b, 0
        jne if.32.24.238.5.end
        if.32.27.238.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.238.5.end:
    func.assert.238.5.end:
    func.baz.240.9:
        mov qword [rbp + 360], 1
        sal qword [rbp + 360], 1
    func.baz.240.9.end:
    cmp.241.12:
    cmp qword [rbp + 360], 2
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
    func.baz.243.21:
        mov qword [rbp + 368], 3
        sal qword [rbp + 368], 1
    func.baz.243.21.end:
    mov qword [rbp + 376], 0
    cmp.244.12:
    cmp qword [rbp + 368], 6
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
    func.point_init.246.20:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point_init.246.20.end:
    cmp.249.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.249.12.end:
    func.assert.249.5:
        if.32.27.249.5:
        cmp.32.27.249.5:
        cmp r15b, 0
        jne if.32.24.249.5.end
        if.32.27.249.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.249.5.end:
    func.assert.249.5.end:
    cmp.250.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.250.12.end:
    func.assert.250.5:
        if.32.27.250.5:
        cmp.32.27.250.5:
        cmp r15b, 0
        jne if.32.24.250.5.end
        if.32.27.250.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.250.5.end:
    func.assert.250.5.end:
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.256.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.256.12.end:
    func.assert.256.5:
        if.32.27.256.5:
        cmp.32.27.256.5:
        cmp r15b, 0
        jne if.32.24.256.5.end
        if.32.27.256.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.256.5.end:
    func.assert.256.5.end:
    cmp.257.12:
    cmp qword [rbp + 424], 2
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
    cmp dword [rbp + 432], 16711680
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
    cmp.262.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.262.12.end:
    func.assert.262.5:
        if.32.27.262.5:
        cmp.32.27.262.5:
        cmp r15b, 0
        jne if.32.24.262.5.end
        if.32.27.262.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.262.5.end:
    func.assert.262.5.end:
    cmp.263.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.263.12.end:
    func.assert.263.5:
        if.32.27.263.5:
        cmp.32.27.263.5:
        cmp r15b, 0
        jne if.32.24.263.5.end
        if.32.27.263.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.263.5.end:
    func.assert.263.5.end:
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.266.12:
    cmp qword [rbp + 456], -1
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
    cmp.267.12:
    cmp qword [rbp + 464], -2
    sete r15b
    bool.267.12.end:
    func.assert.267.5:
        if.32.27.267.5:
        cmp.32.27.267.5:
        cmp r15b, 0
        jne if.32.24.267.5.end
        if.32.27.267.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.267.5.end:
    func.assert.267.5.end:
    cmp.268.12:
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.274.12:
    mov r14, 0
    mov r13, 274
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 73
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
    mov r15, 1
    mov r14, 275
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
    imul r15, 24
    func.object_init.275.13:
        mov qword [rbp + r15 + 480], 2
        mov qword [rbp + r15 + 488], 74
        mov dword [rbp + r15 + 496], 16777215
    func.object_init.275.13.end:
    cmp.276.12:
    mov r14, 1
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 74
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
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov r15, 1
    mov r14, 279
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    lea r15, [rbp + r15 + 528]
    mov r14, 1
    mov r13, 279
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp.280.12:
    mov r14, 1
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 280
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    mov r15, 1
    mov r14, 283
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 283
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 + 528]
    mov r15, 0
    mov r14, 284
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 284
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [rbp + r15 + 528]
    shl rcx, 3
    rep movsb
    cmp.289.12:
    mov r14, 0
    mov r13, 289
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 289
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool.289.12.end:
    func.assert.289.5:
        if.32.27.289.5:
        cmp.32.27.289.5:
        cmp r15b, 0
        jne if.32.24.289.5.end
        if.32.27.289.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.289.5.end:
    func.assert.289.5.end:
    cmp.290.12:
        mov rcx, 8
        mov r14, 0
        mov r13, 291
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 291
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 + 528]
        mov r14, 1
        mov r13, 292
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 292
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + r14 + 528]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.290.12.end:
    func.assert.290.5:
        if.32.27.290.5:
        cmp.32.27.290.5:
        cmp r15b, 0
        jne if.32.24.290.5.end
        if.32.27.290.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.290.5.end:
    func.assert.290.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.296.12:
        mov r14, 2
    cmp r14, 2
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
    mov r14, 0
    mov r13, 297
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], -1
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
    cmp.298.12:
    mov r14, 1
    mov r13, 298
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], 2
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
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.302.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.302.5.end:
    loop.303.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        PUSH_REGS
        lea rbx, [rbp + 1192]
        call func.print_num
        POP_REGS
        func.print.306.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.306.9.end:
        func.print.307.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.307.9.end:
        func.str.input.308.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.308.12.end:
        if.310.12:
        cmp.310.12:
        cmp byte [rbp + 1064], 0
        jle loop.303.5.end
        if.310.12.code:
        if.312.19:
        cmp.312.19:
        cmp byte [rbp + 1064], 4
        jg if.310.9.else
        if.312.19.code:
            func.print.313.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.313.13.end:
            jmp loop.303.5
        if.310.9.else:
            func.print.316.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.316.13.end:
            func.str.output.317.16:
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
            func.str.output.317.16.end:
            func.print.318.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.318.13.end:
            func.print.319.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.319.13.end:
        if.310.9.end:
    jmp loop.303.5
    loop.303.5.end:
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
    if.118.8:
    cmp.118.8:
    cmp qword [rbx + 32], 0
    jge if.118.5.end
    if.118.8.code:
        mov byte [rbx + 40], 1
    if.118.5.end:
    if.121.8:
    cmp.121.8:
    cmp qword [rbx + 32], 0
    jle if.121.5.end
    if.121.8.code:
        neg qword [rbx + 32]
    if.121.5.end:
    mov qword [rbx + 48], 20
    loop.126.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 128
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
        if.130.12:
        cmp.130.12:
        cmp qword [rbx + 32], 0
        jne loop.126.5
        if.130.12.code:
        if.130.9.end:
    loop.126.5.end:
    if.133.8:
    cmp.133.8:
    cmp byte [rbx + 40], 0
    je if.133.5.end
    if.133.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 135
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.133.5.end:
    mov qword [rbx + 56], 0
    loop.139.5:
        mov r15, qword [rbx + 56]
        mov r14, 140
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        mov r13, 140
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
        if.143.12:
        cmp.143.12:
        cmp qword [rbx + 48], 20
        jne loop.139.5
        if.143.12.code:
        if.143.9.end:
    loop.139.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    mov r15, 146
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

%macro PUSH_REGS 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

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
;[98:7] const yes = 1
;[99:7] const no = 0
;[100:7] const maybe = -1
;
main:
;   [150:5] var answer
;   [150:9] answer: i64 (8 B @ [rbp + 224])
;   [150:9] zero 1 * 8 B = 8 B
;   [150:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
;   [152:5] assert(answer == 0)
;   [152:12] allocate scratch register -> r15
;   [152:12] ? answer == 0
;   [152:12] ? answer == 0
    cmp.152.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.152.12.end:
;   [32:6] assert(ok bool)
    func.assert.152.5:
;       [152:5] alias ok -> r15b
        if.32.27.152.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.152.5:
        cmp r15b, 0
        jne if.32.24.152.5.end
        if.32.27.152.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.152.5.end:
;       [152:5] free scratch register r15
    func.assert.152.5.end:
;   [154:5] answer = maybe
;   [154:14] maybe
    mov qword [rbp + 224], -1
;   [155:5] assert(answer == -1)
;   [155:12] allocate scratch register -> r15
;   [155:12] ? answer == -1
;   [155:12] ? answer == -1
    cmp.155.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.155.12.end:
;   [32:6] assert(ok bool)
    func.assert.155.5:
;       [155:5] alias ok -> r15b
        if.32.27.155.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.155.5:
        cmp r15b, 0
        jne if.32.24.155.5.end
        if.32.27.155.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.155.5.end:
;       [155:5] free scratch register r15
    func.assert.155.5.end:
;       [160:15] const maybe = 33
;       [161:9] assert(maybe == 33)
;       [32:6] assert(ok bool)
        func.assert.161.9:
;           [161:9] alias ok -> 1
            if.32.27.161.9:
;           [32:27] ? not ok
;           [32:27] ? not ok
            cmp.32.27.161.9:
;           [32:31] const eval to false
            if.32.24.161.9.end:
        func.assert.161.9.end:
;   [164:5] assert(maybe == -1)
;   [32:6] assert(ok bool)
    func.assert.164.5:
;       [164:5] alias ok -> 1
        if.32.27.164.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.164.5:
;       [32:31] const eval to false
        if.32.24.164.5.end:
    func.assert.164.5.end:
;   [166:5] var arr[4] i32
;   [166:9] arr: i32[4] (16 B @ [rbp + 232])
;   [166:9] zero 4 * 4 B = 16 B
;   [166:5] size <= 32 B, use mov
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
;   [169:5] var ix = 1
;   [169:9] ix: i64 (8 B @ [rbp + 248])
;   [169:9] ix = 1
;   [169:14] 1
    mov qword [rbp + 248], 1
;   [172:5] arr[ix] = 2
;   [172:9] allocate scratch register -> r15
;   [172:9] set array index
;   [172:9] ix
    mov r15, qword [rbp + 248]
;   [172:9] bounds check
;   [172:9] allocate scratch register -> r14
;   [172:9] line number
    mov r14, 172
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [172:9] free scratch register r14
;   [172:15] 2
    mov dword [rbp + r15 * 4 + 232], 2
;   [172:5] free scratch register r15
;   [173:5] arr[ix + 1] = arr[ix]
;   [173:9] allocate scratch register -> r15
;   [173:9] set array index
;   [173:9] ix
    mov r15, qword [rbp + 248]
;   [173:14] r15 + 1
    add r15, 1
;   [173:9] bounds check
;   [173:9] allocate scratch register -> r14
;   [173:9] line number
    mov r14, 173
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [173:9] free scratch register r14
;   [173:19] arr[ix]
;   [173:23] allocate scratch register -> r14
;   [173:23] set array index
;   [173:23] ix
    mov r14, qword [rbp + 248]
;   [173:23] bounds check
;   [173:23] allocate scratch register -> r13
;   [173:23] line number
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [173:23] free scratch register r13
;   [173:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 232]
    mov dword [rbp + r15 * 4 + 232], r13d
;   [173:19] free scratch register r13
;   [173:19] free scratch register r14
;   [173:5] free scratch register r15
;   [174:5] assert(arr[1] == 2)
;   [174:12] allocate scratch register -> r15
;   [174:12] ? arr[1] == 2
;   [174:12] ? arr[1] == 2
    cmp.174.12:
;   [174:16] allocate scratch register -> r14
;   [174:16] set array index
;   [174:16] 1
    mov r14, 1
;   [174:16] bounds check
;   [174:16] allocate scratch register -> r13
;   [174:16] line number
    mov r13, 174
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [174:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 232], 2
;   [174:12] free scratch register r14
    sete r15b
    bool.174.12.end:
;   [32:6] assert(ok bool)
    func.assert.174.5:
;       [174:5] alias ok -> r15b
        if.32.27.174.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.174.5:
        cmp r15b, 0
        jne if.32.24.174.5.end
        if.32.27.174.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.174.5.end:
;       [174:5] free scratch register r15
    func.assert.174.5.end:
;   [175:5] assert(arr[2] == 2)
;   [175:12] allocate scratch register -> r15
;   [175:12] ? arr[2] == 2
;   [175:12] ? arr[2] == 2
    cmp.175.12:
;   [175:16] allocate scratch register -> r14
;   [175:16] set array index
;   [175:16] 2
    mov r14, 2
;   [175:16] bounds check
;   [175:16] allocate scratch register -> r13
;   [175:16] line number
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [175:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 232], 2
;   [175:12] free scratch register r14
    sete r15b
    bool.175.12.end:
;   [32:6] assert(ok bool)
    func.assert.175.5:
;       [175:5] alias ok -> r15b
        if.32.27.175.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.175.5:
        cmp r15b, 0
        jne if.32.24.175.5.end
        if.32.27.175.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.175.5.end:
;       [175:5] free scratch register r15
    func.assert.175.5.end:
;   [177:5] array_copy(arr[2], arr, 2)
;   [177:5] allocate scratch register -> r15
;   [177:29] 2
;   [177:29] 2
    mov r15, 2
;   [177:16] arr[2]
;   [177:20] allocate scratch register -> r14
;   [177:20] set array index
;   [177:20] 2
    mov r14, 2
;   [177:20] bounds check
;   [177:20] allocate scratch register -> r13
;   [177:20] line number
    mov r13, 177
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
;   [177:20] allocate scratch register -> r12
    mov r12, r15
    add r12, r14
    cmp r12, 4
;   [177:20] free scratch register r12
    cmovg rbp, r13
    jg baz_bounds_panic
;   [177:20] free scratch register r13
;   [177:24] arr
;   [177:24] bounds check
;   [177:24] allocate scratch register -> r13
;   [177:24] line number
    mov r13, 177
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
;   [177:24] free scratch register r13
;   [177:5] size <= 16 B, use mov
;   [177:5] allocate named register rax
    mov rax, qword [rbp + r14 * 4 + 232]
    mov qword [rbp + 232], rax
;   [177:5] free named register rax
;   [177:5] free scratch register r14
;   [177:5] free scratch register r15
;   [178:5] assert(arr[0] == 2)
;   [178:12] allocate scratch register -> r15
;   [178:12] ? arr[0] == 2
;   [178:12] ? arr[0] == 2
    cmp.178.12:
;   [178:16] allocate scratch register -> r14
;   [178:16] set array index
;   [178:16] 0
    mov r14, 0
;   [178:16] bounds check
;   [178:16] allocate scratch register -> r13
;   [178:16] line number
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [178:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 232], 2
;   [178:12] free scratch register r14
    sete r15b
    bool.178.12.end:
;   [32:6] assert(ok bool)
    func.assert.178.5:
;       [178:5] alias ok -> r15b
        if.32.27.178.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.178.5:
        cmp r15b, 0
        jne if.32.24.178.5.end
        if.32.27.178.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.178.5.end:
;       [178:5] free scratch register r15
    func.assert.178.5.end:
;   [181:5] var arr1[8] i32
;   [181:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [181:9] zero 8 * 4 B = 32 B
;   [181:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [182:5] array_copy(arr, arr1, 4)
;   [182:5] allocate scratch register -> r15
;   [182:27] 4
;   [182:27] 4
    mov r15, 4
;   [182:16] arr
;   [182:16] bounds check
;   [182:16] allocate scratch register -> r14
;   [182:16] line number
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
;   [182:16] free scratch register r14
;   [182:21] arr1
;   [182:21] bounds check
;   [182:21] allocate scratch register -> r14
;   [182:21] line number
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [182:21] free scratch register r14
;   [182:5] size <= 16 B, use mov
;   [182:5] allocate named register rax
    mov rax, qword [rbp + 232]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 240]
    mov qword [rbp + 264], rax
;   [182:5] free named register rax
;   [182:5] free scratch register r15
;   [183:5] var eq bool = arrays_equal(arr[1], arr1[1], 3)
;   [183:9] eq: bool (1 B @ [rbp + 288])
;   [183:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [183:19] ? arrays_equal(arr[1], arr1[1], 3)
;   [183:19] ? arrays_equal(arr[1], arr1[1], 3)
    cmp.183.19:
;       [183:19] arrays_equal(arr[1], arr1[1], 3)
;       [183:19] allocate named register rsi
;       [183:19] allocate named register rdi
;       [183:19] allocate named register rcx
;       [183:49] 3
;       [183:49] 3
        mov rcx, 3
;       [183:32] arr[1]
;       [183:36] allocate scratch register -> r15
;       [183:36] set array index
;       [183:36] 1
        mov r15, 1
;       [183:36] bounds check
;       [183:36] allocate scratch register -> r14
;       [183:36] line number
        mov r14, 183
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [183:36] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 4
;       [183:36] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [183:36] free scratch register r14
        lea rsi, [rbp + r15 * 4 + 232]
;       [183:19] free scratch register r15
;       [183:40] arr1[1]
;       [183:45] allocate scratch register -> r15
;       [183:45] set array index
;       [183:45] 1
        mov r15, 1
;       [183:45] bounds check
;       [183:45] allocate scratch register -> r14
;       [183:45] line number
        mov r14, 183
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [183:45] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 8
;       [183:45] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [183:45] free scratch register r14
        lea rdi, [rbp + r15 * 4 + 256]
;       [183:19] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [183:19] free named register rcx
;       [183:19] free named register rdi
;       [183:19] free named register rsi
        sete byte [rbp + 288]
    bool.183.19.end:
;   [186:5] assert(eq)
;   [186:12] allocate scratch register -> r15
;   [186:12] ? eq
;   [186:12] ? eq
    cmp.186.12:
    cmp byte [rbp + 288], 0
    setne r15b
    bool.186.12.end:
;   [32:6] assert(ok bool)
    func.assert.186.5:
;       [186:5] alias ok -> r15b
        if.32.27.186.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [188:5] arr1[2] = -1
;   [188:10] allocate scratch register -> r15
;   [188:10] set array index
;   [188:10] 2
    mov r15, 2
;   [188:10] bounds check
;   [188:10] allocate scratch register -> r14
;   [188:10] line number
    mov r14, 188
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [188:10] free scratch register r14
;   [188:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [188:5] free scratch register r15
;   [189:5] assert(not arrays_equal(arr, arr1, 4))
;   [189:12] allocate scratch register -> r15
;   [189:12] ? not arrays_equal(arr, arr1, 4)
;   [189:12] ? not arrays_equal(arr, arr1, 4)
    cmp.189.12:
;       [189:16] arrays_equal(arr, arr1, 4)
;       [189:16] allocate named register rsi
;       [189:16] allocate named register rdi
;       [189:16] allocate named register rcx
;       [189:40] 4
;       [189:40] 4
        mov rcx, 4
;       [189:29] arr
;       [189:29] bounds check
;       [189:29] allocate scratch register -> r14
;       [189:29] line number
        mov r14, 189
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
;       [189:29] free scratch register r14
        lea rsi, [rbp + 232]
;       [189:34] arr1
;       [189:34] bounds check
;       [189:34] allocate scratch register -> r14
;       [189:34] line number
        mov r14, 189
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
;       [189:34] free scratch register r14
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [189:16] free named register rcx
;       [189:16] free named register rdi
;       [189:16] free named register rsi
        setne r15b
    bool.189.12.end:
;   [32:6] assert(ok bool)
    func.assert.189.5:
;       [189:5] alias ok -> r15b
        if.32.27.189.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.189.5:
        cmp r15b, 0
        jne if.32.24.189.5.end
        if.32.27.189.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.189.5.end:
;       [189:5] free scratch register r15
    func.assert.189.5.end:
;   [191:5] ix = 3
;   [191:10] 3
    mov qword [rbp + 248], 3
;   [192:5] arr[ix] = ~inv(arr[ix - 1])
;   [192:9] allocate scratch register -> r15
;   [192:9] set array index
;   [192:9] ix
    mov r15, qword [rbp + 248]
;   [192:9] bounds check
;   [192:9] allocate scratch register -> r14
;   [192:9] line number
    mov r14, 192
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [192:9] free scratch register r14
;   [192:16] arr = ~inv(arr[ix - 1])
;   [192:16] = expression
;   [192:16] ~inv(arr[ix - 1])
;   [192:24] allocate scratch register -> r14
;   [192:24] set array index
;   [192:24] ix
    mov r14, qword [rbp + 248]
;   [192:29] r14 - 1
    sub r14, 1
;   [192:24] bounds check
;   [192:24] allocate scratch register -> r13
;   [192:24] line number
    mov r13, 192
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [192:24] free scratch register r13
;   [62:6] inv(i i32) res i32
    func.inv.192.16:
;       [192:16] alias res -> arr (lea: rbp + r15 * 4 + 232)
;       [192:16] alias i -> arr (lea: rbp + r14 * 4 + 232)
;       [63:5] res = ~i
;       [63:12] ~i
;       [63:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 232]
        mov dword [rbp + r15 * 4 + 232], r13d
;       [63:12] free scratch register r13
        not dword [rbp + r15 * 4 + 232]
;       [192:16] free scratch register r14
    func.inv.192.16.end:
    not dword [rbp + r15 * 4 + 232]
;   [192:5] free scratch register r15
;   [193:5] assert(arr[ix] == 2)
;   [193:12] allocate scratch register -> r15
;   [193:12] ? arr[ix] == 2
;   [193:12] ? arr[ix] == 2
    cmp.193.12:
;   [193:16] allocate scratch register -> r14
;   [193:16] set array index
;   [193:16] ix
    mov r14, qword [rbp + 248]
;   [193:16] bounds check
;   [193:16] allocate scratch register -> r13
;   [193:16] line number
    mov r13, 193
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [193:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 232], 2
;   [193:12] free scratch register r14
    sete r15b
    bool.193.12.end:
;   [32:6] assert(ok bool)
    func.assert.193.5:
;       [193:5] alias ok -> r15b
        if.32.27.193.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.193.5:
        cmp r15b, 0
        jne if.32.24.193.5.end
        if.32.27.193.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.193.5.end:
;       [193:5] free scratch register r15
    func.assert.193.5.end:
;   [195:5] faz(arr)
;   [72:6] faz(arg[] i32)
    func.faz.195.5:
;       [195:5] alias arg -> arr
;       [73:5] arg[1] = 0xfe
;       [73:9] allocate scratch register -> r15
;       [73:9] set array index
;       [73:9] 1
        mov r15, 1
;       [73:9] bounds check
;       [73:9] allocate scratch register -> r14
;       [73:9] line number
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
;       [73:9] free scratch register r14
;       [73:14] 0xfe
        mov dword [rbp + r15 * 4 + 232], 254
;       [73:5] free scratch register r15
    func.faz.195.5.end:
;   [196:5] assert(arr[1] == 0xfe)
;   [196:12] allocate scratch register -> r15
;   [196:12] ? arr[1] == 0xfe
;   [196:12] ? arr[1] == 0xfe
    cmp.196.12:
;   [196:16] allocate scratch register -> r14
;   [196:16] set array index
;   [196:16] 1
    mov r14, 1
;   [196:16] bounds check
;   [196:16] allocate scratch register -> r13
;   [196:16] line number
    mov r13, 196
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [196:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 232], 254
;   [196:12] free scratch register r14
    sete r15b
    bool.196.12.end:
;   [32:6] assert(ok bool)
    func.assert.196.5:
;       [196:5] alias ok -> r15b
        if.32.27.196.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.196.5:
        cmp r15b, 0
        jne if.32.24.196.5.end
        if.32.27.196.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.196.5.end:
;       [196:5] free scratch register r15
    func.assert.196.5.end:
;   [198:5] var arr3[] = { 3, 5 }
;   [198:9] arr3: i64[2] (16 B @ [rbp + 296])
;   [198:9] arr3= { 3, 5 }
;   [198:18] size <= 16 B, use immediates
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
;   [199:5] foo arr3
;   [199:9] allocate scratch register -> r15
;   [199:9] e: i64 (r15)
;   [199:9] i: i64 (8 B @ [rbp + 320])
;   [199:9] const n = 2
;   [199:9] initiate iterator e
    lea r15, [rbp + 296]
;   [199:9] initiate counter i
    mov qword [rbp + 320], 0
    foo.199.5:
;       [200:9] e = e + i + n
;       [200:13] instructions without scratch register 3, with 4
;       [200:13] e
;       [200:17] e + i
;       [200:17] allocate scratch register -> r14
        mov r14, qword [rbp + 320]
        add qword [r15], r14
;       [200:17] free scratch register r14
;       [200:21] e + n
        add qword [r15], 2
        foo.199.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.199.5
    foo.199.5.end:
;   [199:5] free scratch register r15
;   [202:5] assert(arr3[0] == 3 + 0 + 2)
;   [202:12] allocate scratch register -> r15
;   [202:12] ? arr3[0] == 3 + 0 + 2
;   [202:12] ? arr3[0] == 3 + 0 + 2
    cmp.202.12:
;   [202:17] allocate scratch register -> r14
;   [202:17] set array index
;   [202:17] 0
    mov r14, 0
;   [202:17] bounds check
;   [202:17] allocate scratch register -> r13
;   [202:17] line number
    mov r13, 202
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [202:17] free scratch register r13
;   [202:23] allocate scratch register -> r13
;       [202:23] 3
        mov r13, 3
;       [202:27] r13 + 0
        add r13, 0
;       [202:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
;   [202:12] free scratch register r13
;   [202:12] free scratch register r14
    sete r15b
    bool.202.12.end:
;   [32:6] assert(ok bool)
    func.assert.202.5:
;       [202:5] alias ok -> r15b
        if.32.27.202.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.202.5:
        cmp r15b, 0
        jne if.32.24.202.5.end
        if.32.27.202.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.202.5.end:
;       [202:5] free scratch register r15
    func.assert.202.5.end:
;   [203:5] assert(arr3[1] == 5 + 1 + 2)
;   [203:12] allocate scratch register -> r15
;   [203:12] ? arr3[1] == 5 + 1 + 2
;   [203:12] ? arr3[1] == 5 + 1 + 2
    cmp.203.12:
;   [203:17] allocate scratch register -> r14
;   [203:17] set array index
;   [203:17] 1
    mov r14, 1
;   [203:17] bounds check
;   [203:17] allocate scratch register -> r13
;   [203:17] line number
    mov r13, 203
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [203:17] free scratch register r13
;   [203:23] allocate scratch register -> r13
;       [203:23] 5
        mov r13, 5
;       [203:27] r13 + 1
        add r13, 1
;       [203:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
;   [203:12] free scratch register r13
;   [203:12] free scratch register r14
    sete r15b
    bool.203.12.end:
;   [32:6] assert(ok bool)
    func.assert.203.5:
;       [203:5] alias ok -> r15b
        if.32.27.203.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [209:5] var p point
;   [209:9] p: point (16 B @ [rbp + 312])
;   [209:9] zero 1 * 16 B = 16 B
;   [209:5] size <= 32 B, use mov
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
;   [212:7] p.fooz()
;   [46:6] point.fooz()
    func.point.fooz.212.7:
;       [212:7] alias self -> p
;       [47:5] self.x = 0b10
;       [47:14] 0b10
        mov qword [rbp + 312], 2
;       [48:5] self.y = 0xb
;       [48:14] 0xb
        mov qword [rbp + 320], 11
    func.point.fooz.212.7.end:
;   [215:5] assert(p.x == 2)
;   [215:12] allocate scratch register -> r15
;   [215:12] ? p.x == 2
;   [215:12] ? p.x == 2
    cmp.215.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.215.12.end:
;   [32:6] assert(ok bool)
    func.assert.215.5:
;       [215:5] alias ok -> r15b
        if.32.27.215.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.215.5:
        cmp r15b, 0
        jne if.32.24.215.5.end
        if.32.27.215.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.215.5.end:
;       [215:5] free scratch register r15
    func.assert.215.5.end:
;   [216:5] assert(p.y == 0xb)
;   [216:12] allocate scratch register -> r15
;   [216:12] ? p.y == 0xb
;   [216:12] ? p.y == 0xb
    cmp.216.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.216.12.end:
;   [32:6] assert(ok bool)
    func.assert.216.5:
;       [216:5] alias ok -> r15b
        if.32.27.216.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [218:5] var q point = p
;   [218:9] q: point (16 B @ [rbp + 328])
;   [218:9] q = p
;   [218:19] size <= 16 B, use mov
;   [218:19] allocate named register rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
;   [218:19] free named register rax
;   [221:5] assert(equal(p, q))
;   [221:12] allocate scratch register -> r15
;   [221:12] ? equal(p, q)
;   [221:12] ? equal(p, q)
    cmp.221.12:
;       [221:12] equal(p, q)
;       [221:12] allocate named register rsi
;       [221:12] allocate named register rdi
;       [221:12] allocate named register rcx
;       [221:18] p
        lea rsi, [rbp + 312]
;       [221:21] q
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
;       [221:12] free named register rcx
;       [221:12] free named register rdi
;       [221:12] free named register rsi
        sete r15b
    bool.221.12.end:
;   [32:6] assert(ok bool)
    func.assert.221.5:
;       [221:5] alias ok -> r15b
        if.32.27.221.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.221.5:
        cmp r15b, 0
        jne if.32.24.221.5.end
        if.32.27.221.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.221.5.end:
;       [221:5] free scratch register r15
    func.assert.221.5.end:
;   [225:5] q.x = 3
;   [225:11] 3
    mov qword [rbp + 328], 3
;   [226:5] assert(not equal(p, q))
;   [226:12] allocate scratch register -> r15
;   [226:12] ? not equal(p, q)
;   [226:12] ? not equal(p, q)
    cmp.226.12:
;       [226:16] equal(p, q)
;       [226:16] allocate named register rsi
;       [226:16] allocate named register rdi
;       [226:16] allocate named register rcx
;       [226:22] p
        lea rsi, [rbp + 312]
;       [226:25] q
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
;       [226:16] free named register rcx
;       [226:16] free named register rdi
;       [226:16] free named register rsi
        setne r15b
    bool.226.12.end:
;   [32:6] assert(ok bool)
    func.assert.226.5:
;       [226:5] alias ok -> r15b
        if.32.27.226.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.226.5:
        cmp r15b, 0
        jne if.32.24.226.5.end
        if.32.27.226.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.226.5.end:
;       [226:5] free scratch register r15
    func.assert.226.5.end:
;   [228:5] var i = 0
;   [228:9] i: i64 (8 B @ [rbp + 344])
;   [228:9] i = 0
;   [228:13] 0
    mov qword [rbp + 344], 0
;   [229:5] bar(i)
;   [54:6] bar(arg)
    func.bar.229.5:
;       [229:5] alias arg -> i
        if.55.8.229.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.229.5:
        cmp qword [rbp + 344], 0
        je func.bar.229.5.end
        if.55.8.229.5.code:
;           [55:17] return
        if.55.5.229.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.229.5.end:
;   [230:5] assert(i == 0)
;   [230:12] allocate scratch register -> r15
;   [230:12] ? i == 0
;   [230:12] ? i == 0
    cmp.230.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.230.12.end:
;   [32:6] assert(ok bool)
    func.assert.230.5:
;       [230:5] alias ok -> r15b
        if.32.27.230.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [232:5] i = 1
;   [232:9] 1
    mov qword [rbp + 344], 1
;   [233:5] bar(i)
;   [54:6] bar(arg)
    func.bar.233.5:
;       [233:5] alias arg -> i
        if.55.8.233.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.233.5:
        cmp qword [rbp + 344], 0
        je func.bar.233.5.end
        if.55.8.233.5.code:
;           [55:17] return
        if.55.5.233.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.233.5.end:
;   [234:5] assert(i == 0xff)
;   [234:12] allocate scratch register -> r15
;   [234:12] ? i == 0xff
;   [234:12] ? i == 0xff
    cmp.234.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.234.12.end:
;   [32:6] assert(ok bool)
    func.assert.234.5:
;       [234:5] alias ok -> r15b
        if.32.27.234.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.234.5:
        cmp r15b, 0
        jne if.32.24.234.5.end
        if.32.27.234.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.234.5.end:
;       [234:5] free scratch register r15
    func.assert.234.5.end:
;   [236:5] var j = 1
;   [236:9] j: i64 (8 B @ [rbp + 352])
;   [236:9] j = 1
;   [236:13] 1
    mov qword [rbp + 352], 1
;   [237:5] var k = baz(j)
;   [237:9] k: i64 (8 B @ [rbp + 360])
;   [237:9] k = baz(j)
;   [237:13] k = baz(j)
;   [237:13] = expression
;   [237:13] baz(j)
;   [66:6] baz(arg) res
    func.baz.237.13:
;       [237:13] alias res -> k
;       [237:13] alias arg -> j
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 3, with 3
;       [67:11] arg
;       [67:11] allocate scratch register -> r15
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
;       [67:11] free scratch register r15
;       [67:17] res * 2
;       [67:17] dst is not reg, src is const
        sal qword [rbp + 360], 1
    func.baz.237.13.end:
;   [238:5] assert(k == 2)
;   [238:12] allocate scratch register -> r15
;   [238:12] ? k == 2
;   [238:12] ? k == 2
    cmp.238.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.238.12.end:
;   [32:6] assert(ok bool)
    func.assert.238.5:
;       [238:5] alias ok -> r15b
        if.32.27.238.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.238.5:
        cmp r15b, 0
        jne if.32.24.238.5.end
        if.32.27.238.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.238.5.end:
;       [238:5] free scratch register r15
    func.assert.238.5.end:
;   [240:5] k = baz(1)
;   [240:9] k = baz(1)
;   [240:9] = expression
;   [240:9] baz(1)
;   [66:6] baz(arg) res
    func.baz.240.9:
;       [240:9] alias res -> k
;       [240:9] alias arg -> 1
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 2, with 3
;       [67:11] arg
        mov qword [rbp + 360], 1
;       [67:17] res * 2
;       [67:17] dst is not reg, src is const
        sal qword [rbp + 360], 1
    func.baz.240.9.end:
;   [241:5] assert(k == 2)
;   [241:12] allocate scratch register -> r15
;   [241:12] ? k == 2
;   [241:12] ? k == 2
    cmp.241.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.241.12.end:
;   [32:6] assert(ok bool)
    func.assert.241.5:
;       [241:5] alias ok -> r15b
        if.32.27.241.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [243:5] var p0 point = {baz(3), 0}
;   [243:9] p0: point (16 B @ [rbp + 368])
;   [243:9] p0 = {baz(3), 0}
;   [243:21] copy field 'x'
;   [243:21] p0.x = baz(3)
;   [243:21] = expression
;   [243:21] baz(3)
;   [66:6] baz(arg) res
    func.baz.243.21:
;       [243:21] alias res -> p0.x (lea: rbp + 368)
;       [243:21] alias arg -> 3
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 2, with 3
;       [67:11] arg
        mov qword [rbp + 368], 3
;       [67:17] res * 2
;       [67:17] dst is not reg, src is const
        sal qword [rbp + 368], 1
    func.baz.243.21.end:
;   [243:29] copy field 'y'
    mov qword [rbp + 376], 0
;   [244:5] assert(p0.x == 6)
;   [244:12] allocate scratch register -> r15
;   [244:12] ? p0.x == 6
;   [244:12] ? p0.x == 6
    cmp.244.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.244.12.end:
;   [32:6] assert(ok bool)
    func.assert.244.5:
;       [244:5] alias ok -> r15b
        if.32.27.244.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [246:5] var pt point = point_init()
;   [246:9] pt: point (16 B @ [rbp + 384])
;   [246:9] pt = point_init()
;   [246:20] point_init()
;   [87:6] point_init() res point
    func.point_init.246.20:
;       [246:20] alias res -> pt
;       [88:5] res.x = -1
;       [88:14] -1
        mov qword [rbp + 384], -1
;       [89:5] res.y = -2
;       [89:14] -2
        mov qword [rbp + 392], -2
    func.point_init.246.20.end:
;   [249:5] assert(pt.x == -1)
;   [249:12] allocate scratch register -> r15
;   [249:12] ? pt.x == -1
;   [249:12] ? pt.x == -1
    cmp.249.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.249.12.end:
;   [32:6] assert(ok bool)
    func.assert.249.5:
;       [249:5] alias ok -> r15b
        if.32.27.249.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.249.5:
        cmp r15b, 0
        jne if.32.24.249.5.end
        if.32.27.249.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.249.5.end:
;       [249:5] free scratch register r15
    func.assert.249.5.end:
;   [250:5] assert(pt.y == -2)
;   [250:12] allocate scratch register -> r15
;   [250:12] ? pt.y == -2
;   [250:12] ? pt.y == -2
    cmp.250.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.250.12.end:
;   [32:6] assert(ok bool)
    func.assert.250.5:
;       [250:5] alias ok -> r15b
        if.32.27.250.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.250.5:
        cmp r15b, 0
        jne if.32.24.250.5.end
        if.32.27.250.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.250.5.end:
;       [250:5] free scratch register r15
    func.assert.250.5.end:
;   [252:5] var x = 1
;   [252:9] x: i64 (8 B @ [rbp + 400])
;   [252:9] x = 1
;   [252:13] 1
    mov qword [rbp + 400], 1
;   [253:5] var y = 2
;   [253:9] y: i64 (8 B @ [rbp + 408])
;   [253:9] y = 2
;   [253:13] 2
    mov qword [rbp + 408], 2
;   [255:5] var o1 object = {{x * 10, y}, 0xff0000}
;   [255:9] o1: object (24 B @ [rbp + 416])
;   [255:9] o1 = {{x * 10, y}, 0xff0000}
;   [255:22] copy field 'pos'
;   [255:23] copy field 'x'
;   [255:23] instructions without scratch register 5, with 3
;   [255:23] allocate scratch register -> r15
;   [255:23] x
    mov r15, qword [rbp + 400]
;   [255:27] r15 * 10
;   [255:27] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 416], r15
;   [255:23] free scratch register r15
;   [255:31] copy field 'y'
;   [255:31] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
;   [255:31] free scratch register r15
;   [255:35] copy field 'color'
    mov dword [rbp + 432], 16711680
;   [255:21] zero padding: 4 B
;   [255:21] size <= 32 B, use mov
    mov dword [rbp + 436], 0
;   [256:5] assert(o1.pos.x == 10)
;   [256:12] allocate scratch register -> r15
;   [256:12] ? o1.pos.x == 10
;   [256:12] ? o1.pos.x == 10
    cmp.256.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.256.12.end:
;   [32:6] assert(ok bool)
    func.assert.256.5:
;       [256:5] alias ok -> r15b
        if.32.27.256.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.256.5:
        cmp r15b, 0
        jne if.32.24.256.5.end
        if.32.27.256.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.256.5.end:
;       [256:5] free scratch register r15
    func.assert.256.5.end:
;   [257:5] assert(o1.pos.y == 2)
;   [257:12] allocate scratch register -> r15
;   [257:12] ? o1.pos.y == 2
;   [257:12] ? o1.pos.y == 2
    cmp.257.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.257.12.end:
;   [32:6] assert(ok bool)
    func.assert.257.5:
;       [257:5] alias ok -> r15b
        if.32.27.257.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [258:5] assert(o1.color == 0xff0000)
;   [258:12] allocate scratch register -> r15
;   [258:12] ? o1.color == 0xff0000
;   [258:12] ? o1.color == 0xff0000
    cmp.258.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.258.12.end:
;   [32:6] assert(ok bool)
    func.assert.258.5:
;       [258:5] alias ok -> r15b
        if.32.27.258.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [260:5] var p1 point = {-x, -y}
;   [260:9] p1: point (16 B @ [rbp + 440])
;   [260:9] p1 = {-x, -y}
;   [260:21] copy field 'x'
;   [260:21] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 440], r15
;   [260:21] free scratch register r15
    neg qword [rbp + 440]
;   [260:25] copy field 'y'
;   [260:25] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 448], r15
;   [260:25] free scratch register r15
    neg qword [rbp + 448]
;   [261:5] o1.pos = p1
;   [261:14] size <= 16 B, use mov
;   [261:14] allocate named register rax
    mov rax, qword [rbp + 440]
    mov qword [rbp + 416], rax
    mov rax, qword [rbp + 448]
    mov qword [rbp + 424], rax
;   [261:14] free named register rax
;   [262:5] assert(o1.pos.x == -1)
;   [262:12] allocate scratch register -> r15
;   [262:12] ? o1.pos.x == -1
;   [262:12] ? o1.pos.x == -1
    cmp.262.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.262.12.end:
;   [32:6] assert(ok bool)
    func.assert.262.5:
;       [262:5] alias ok -> r15b
        if.32.27.262.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.262.5:
        cmp r15b, 0
        jne if.32.24.262.5.end
        if.32.27.262.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.262.5.end:
;       [262:5] free scratch register r15
    func.assert.262.5.end:
;   [263:5] assert(o1.pos.y == -2)
;   [263:12] allocate scratch register -> r15
;   [263:12] ? o1.pos.y == -2
;   [263:12] ? o1.pos.y == -2
    cmp.263.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.263.12.end:
;   [32:6] assert(ok bool)
    func.assert.263.5:
;       [263:5] alias ok -> r15b
        if.32.27.263.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.263.5:
        cmp r15b, 0
        jne if.32.24.263.5.end
        if.32.27.263.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.263.5.end:
;       [263:5] free scratch register r15
    func.assert.263.5.end:
;   [265:5] var o2 object = o1
;   [265:9] o2: object (24 B @ [rbp + 456])
;   [265:9] o2 = o1
;   [265:21] allocate named register rsi
;   [265:21] allocate named register rdi
;   [265:21] allocate named register rcx
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
;   [265:21] free named register rcx
;   [265:21] free named register rdi
;   [265:21] free named register rsi
;   [266:5] assert(o2.pos.x == -1)
;   [266:12] allocate scratch register -> r15
;   [266:12] ? o2.pos.x == -1
;   [266:12] ? o2.pos.x == -1
    cmp.266.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.266.12.end:
;   [32:6] assert(ok bool)
    func.assert.266.5:
;       [266:5] alias ok -> r15b
        if.32.27.266.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [267:5] assert(o2.pos.y == -2)
;   [267:12] allocate scratch register -> r15
;   [267:12] ? o2.pos.y == -2
;   [267:12] ? o2.pos.y == -2
    cmp.267.12:
    cmp qword [rbp + 464], -2
    sete r15b
    bool.267.12.end:
;   [32:6] assert(ok bool)
    func.assert.267.5:
;       [267:5] alias ok -> r15b
        if.32.27.267.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.267.5:
        cmp r15b, 0
        jne if.32.24.267.5.end
        if.32.27.267.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.267.5.end:
;       [267:5] free scratch register r15
    func.assert.267.5.end:
;   [268:5] assert(o2.color == 0xff0000)
;   [268:12] allocate scratch register -> r15
;   [268:12] ? o2.color == 0xff0000
;   [268:12] ? o2.color == 0xff0000
    cmp.268.12:
    cmp dword [rbp + 472], 16711680
    sete r15b
    bool.268.12.end:
;   [32:6] assert(ok bool)
    func.assert.268.5:
;       [268:5] alias ok -> r15b
        if.32.27.268.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [270:5] var o3[2] object
;   [270:9] o3: object[2] (48 B @ [rbp + 480])
;   [270:9] zero 2 * 24 B = 48 B
;   [270:5] allocate named register rax
;   [270:5] allocate named register rdi
;   [270:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
;   [270:5] free named register rcx
;   [270:5] free named register rdi
;   [270:5] free named register rax
;   [271:5] o3.pos.y = 73
;   [271:16] 73
    mov qword [rbp + 488], 73
;   [274:5] assert(o3[0].pos.y == 73)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? o3[0].pos.y == 73
;   [274:12] ? o3[0].pos.y == 73
    cmp.274.12:
;   [274:15] allocate scratch register -> r14
;   [274:15] set array index
;   [274:15] 0
    mov r14, 0
;   [274:15] bounds check
;   [274:15] allocate scratch register -> r13
;   [274:15] line number
    mov r13, 274
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [274:15] free scratch register r13
    imul r14, 24
    cmp qword [rbp + r14 + 488], 73
;   [274:12] free scratch register r14
    sete r15b
    bool.274.12.end:
;   [32:6] assert(ok bool)
    func.assert.274.5:
;       [274:5] alias ok -> r15b
        if.32.27.274.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [275:5] o3[1] = object_init()
;   [275:8] allocate scratch register -> r15
;   [275:8] set array index
;   [275:8] 1
    mov r15, 1
;   [275:8] bounds check
;   [275:8] allocate scratch register -> r14
;   [275:8] line number
    mov r14, 275
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
;   [275:8] free scratch register r14
    imul r15, 24
;   [275:13] object_init()
;   [92:6] object_init() res object
    func.object_init.275.13:
;       [275:13] alias res -> o3 (lea: rbp + r15 + 480)
;       [93:5] res.pos.x = 2
;       [93:17] 2
        mov qword [rbp + r15 + 480], 2
;       [94:5] res.pos.y = 74
;       [94:17] 74
        mov qword [rbp + r15 + 488], 74
;       [95:5] res.color = 0xffffff
;       [95:17] 0xffffff
        mov dword [rbp + r15 + 496], 16777215
    func.object_init.275.13.end:
;   [275:5] free scratch register r15
;   [276:5] assert(o3[1].pos.y == 74)
;   [276:12] allocate scratch register -> r15
;   [276:12] ? o3[1].pos.y == 74
;   [276:12] ? o3[1].pos.y == 74
    cmp.276.12:
;   [276:15] allocate scratch register -> r14
;   [276:15] set array index
;   [276:15] 1
    mov r14, 1
;   [276:15] bounds check
;   [276:15] allocate scratch register -> r13
;   [276:15] line number
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [276:15] free scratch register r13
    imul r14, 24
    cmp qword [rbp + r14 + 488], 74
;   [276:12] free scratch register r14
    sete r15b
    bool.276.12.end:
;   [32:6] assert(ok bool)
    func.assert.276.5:
;       [276:5] alias ok -> r15b
        if.32.27.276.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [278:5] var worlds[8] world
;   [278:9] worlds: world[8] (512 B @ [rbp + 528])
;   [278:9] zero 8 * 64 B = 512 B
;   [278:5] allocate named register rax
;   [278:5] allocate named register rdi
;   [278:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
;   [278:5] free named register rcx
;   [278:5] free named register rdi
;   [278:5] free named register rax
;   [279:5] worlds[1].locations[1] = 0xffee
;   [279:12] allocate scratch register -> r15
;   [279:12] set array index
;   [279:12] 1
    mov r15, 1
;   [279:12] bounds check
;   [279:12] allocate scratch register -> r14
;   [279:12] line number
    mov r14, 279
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [279:12] free scratch register r14
    shl r15, 6
    lea r15, [rbp + r15 + 528]
;   [279:25] allocate scratch register -> r14
;   [279:25] set array index
;   [279:25] 1
    mov r14, 1
;   [279:25] bounds check
;   [279:25] allocate scratch register -> r13
;   [279:25] line number
    mov r13, 279
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [279:25] free scratch register r13
;   [279:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [279:5] free scratch register r14
;   [279:5] free scratch register r15
;   [280:5] assert(worlds[1].locations[1] == 0xffee)
;   [280:12] allocate scratch register -> r15
;   [280:12] ? worlds[1].locations[1] == 0xffee
;   [280:12] ? worlds[1].locations[1] == 0xffee
    cmp.280.12:
;   [280:19] allocate scratch register -> r14
;   [280:19] set array index
;   [280:19] 1
    mov r14, 1
;   [280:19] bounds check
;   [280:19] allocate scratch register -> r13
;   [280:19] line number
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [280:19] free scratch register r13
    shl r14, 6
    lea r14, [rbp + r14 + 528]
;   [280:32] allocate scratch register -> r13
;   [280:32] set array index
;   [280:32] 1
    mov r13, 1
;   [280:32] bounds check
;   [280:32] allocate scratch register -> r12
;   [280:32] line number
    mov r12, 280
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [280:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [280:12] free scratch register r13
;   [280:12] free scratch register r14
    sete r15b
    bool.280.12.end:
;   [32:6] assert(ok bool)
    func.assert.280.5:
;       [280:5] alias ok -> r15b
        if.32.27.280.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [282:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds.locations) )
;   [282:5] allocate named register rsi
;   [282:5] allocate named register rdi
;   [282:5] allocate named register rcx
;   [285:9] array_length(worlds.locations)
;   [285:9] rcx = array_length(worlds.locations)
;   [285:9] = expression
;   [285:9] array_length(worlds.locations)
    mov rcx, 8
;   [283:9] worlds[1].locations
;   [283:16] allocate scratch register -> r15
;   [283:16] set array index
;   [283:16] 1
    mov r15, 1
;   [283:16] bounds check
;   [283:16] allocate scratch register -> r14
;   [283:16] line number
    mov r14, 283
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [283:16] free scratch register r14
    shl r15, 6
;   [283:9] bounds check
;   [283:9] allocate scratch register -> r14
;   [283:9] line number
    mov r14, 283
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [283:9] free scratch register r14
    lea rsi, [rbp + r15 + 528]
;   [282:5] free scratch register r15
;   [284:9] worlds[0].locations
;   [284:16] allocate scratch register -> r15
;   [284:16] set array index
;   [284:16] 0
    mov r15, 0
;   [284:16] bounds check
;   [284:16] allocate scratch register -> r14
;   [284:16] line number
    mov r14, 284
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [284:16] free scratch register r14
    shl r15, 6
;   [284:9] bounds check
;   [284:9] allocate scratch register -> r14
;   [284:9] line number
    mov r14, 284
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [284:9] free scratch register r14
    lea rdi, [rbp + r15 + 528]
;   [282:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [282:5] free named register rcx
;   [282:5] free named register rdi
;   [282:5] free named register rsi
;   [289:5] assert(worlds[0].locations[1] == 0xffee)
;   [289:12] allocate scratch register -> r15
;   [289:12] ? worlds[0].locations[1] == 0xffee
;   [289:12] ? worlds[0].locations[1] == 0xffee
    cmp.289.12:
;   [289:19] allocate scratch register -> r14
;   [289:19] set array index
;   [289:19] 0
    mov r14, 0
;   [289:19] bounds check
;   [289:19] allocate scratch register -> r13
;   [289:19] line number
    mov r13, 289
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [289:19] free scratch register r13
    shl r14, 6
    lea r14, [rbp + r14 + 528]
;   [289:32] allocate scratch register -> r13
;   [289:32] set array index
;   [289:32] 1
    mov r13, 1
;   [289:32] bounds check
;   [289:32] allocate scratch register -> r12
;   [289:32] line number
    mov r12, 289
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [289:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [289:12] free scratch register r13
;   [289:12] free scratch register r14
    sete r15b
    bool.289.12.end:
;   [32:6] assert(ok bool)
    func.assert.289.5:
;       [289:5] alias ok -> r15b
        if.32.27.289.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.289.5:
        cmp r15b, 0
        jne if.32.24.289.5.end
        if.32.27.289.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.289.5.end:
;       [289:5] free scratch register r15
    func.assert.289.5.end:
;   [290:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) ))
;   [290:12] allocate scratch register -> r15
;   [290:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) )
;   [290:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) )
    cmp.290.12:
;       [290:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) )
;       [290:12] allocate named register rsi
;       [290:12] allocate named register rdi
;       [290:12] allocate named register rcx
;       [293:14] array_length(worlds.locations)
;       [293:14] rcx = array_length(worlds.locations)
;       [293:14] = expression
;       [293:14] array_length(worlds.locations)
        mov rcx, 8
;       [291:14] worlds[0].locations
;       [291:21] allocate scratch register -> r14
;       [291:21] set array index
;       [291:21] 0
        mov r14, 0
;       [291:21] bounds check
;       [291:21] allocate scratch register -> r13
;       [291:21] line number
        mov r13, 291
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
;       [291:21] free scratch register r13
        shl r14, 6
;       [291:14] bounds check
;       [291:14] allocate scratch register -> r13
;       [291:14] line number
        mov r13, 291
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [291:14] free scratch register r13
        lea rsi, [rbp + r14 + 528]
;       [290:12] free scratch register r14
;       [292:14] worlds[1].locations
;       [292:21] allocate scratch register -> r14
;       [292:21] set array index
;       [292:21] 1
        mov r14, 1
;       [292:21] bounds check
;       [292:21] allocate scratch register -> r13
;       [292:21] line number
        mov r13, 292
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
;       [292:21] free scratch register r13
        shl r14, 6
;       [292:14] bounds check
;       [292:14] allocate scratch register -> r13
;       [292:14] line number
        mov r13, 292
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [292:14] free scratch register r13
        lea rdi, [rbp + r14 + 528]
;       [290:12] free scratch register r14
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [290:12] free named register rcx
;       [290:12] free named register rdi
;       [290:12] free named register rsi
        sete r15b
    bool.290.12.end:
;   [32:6] assert(ok bool)
    func.assert.290.5:
;       [290:5] alias ok -> r15b
        if.32.27.290.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.290.5:
        cmp r15b, 0
        jne if.32.24.290.5.end
        if.32.27.290.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.290.5.end:
;       [290:5] free scratch register r15
    func.assert.290.5.end:
;   [295:5] var arr2[] = { -1, 2 }
;   [295:9] arr2: i64[2] (16 B @ [rbp + 1040])
;   [295:9] arr2= { -1, 2 }
;   [295:18] size <= 16 B, use immediates
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
;   [296:5] assert(array_length(arr2) == 2)
;   [296:12] allocate scratch register -> r15
;   [296:12] ? array_length(arr2) == 2
;   [296:12] ? array_length(arr2) == 2
    cmp.296.12:
;   [296:12] allocate scratch register -> r14
;       [296:12] r14 = array_length(arr2)
;       [296:12] = expression
;       [296:12] array_length(arr2)
        mov r14, 2
    cmp r14, 2
;   [296:12] free scratch register r14
    sete r15b
    bool.296.12.end:
;   [32:6] assert(ok bool)
    func.assert.296.5:
;       [296:5] alias ok -> r15b
        if.32.27.296.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [297:5] assert(arr2[0] == -1)
;   [297:12] allocate scratch register -> r15
;   [297:12] ? arr2[0] == -1
;   [297:12] ? arr2[0] == -1
    cmp.297.12:
;   [297:17] allocate scratch register -> r14
;   [297:17] set array index
;   [297:17] 0
    mov r14, 0
;   [297:17] bounds check
;   [297:17] allocate scratch register -> r13
;   [297:17] line number
    mov r13, 297
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [297:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1040], -1
;   [297:12] free scratch register r14
    sete r15b
    bool.297.12.end:
;   [32:6] assert(ok bool)
    func.assert.297.5:
;       [297:5] alias ok -> r15b
        if.32.27.297.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [298:5] assert(arr2[1] == 2)
;   [298:12] allocate scratch register -> r15
;   [298:12] ? arr2[1] == 2
;   [298:12] ? arr2[1] == 2
    cmp.298.12:
;   [298:17] allocate scratch register -> r14
;   [298:17] set array index
;   [298:17] 1
    mov r14, 1
;   [298:17] bounds check
;   [298:17] allocate scratch register -> r13
;   [298:17] line number
    mov r13, 298
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [298:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1040], 2
;   [298:12] free scratch register r14
    sete r15b
    bool.298.12.end:
;   [32:6] assert(ok bool)
    func.assert.298.5:
;       [298:5] alias ok -> r15b
        if.32.27.298.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [300:5] var counter
;   [300:9] counter: i64 (8 B @ [rbp + 1056])
;   [300:9] zero 1 * 8 B = 8 B
;   [300:5] size <= 32 B, use mov
    mov qword [rbp + 1056], 0
;   [301:5] var nm str
;   [301:9] nm: str (128 B @ [rbp + 1064])
;   [301:9] zero 1 * 128 B = 128 B
;   [301:5] allocate named register rax
;   [301:5] allocate named register rdi
;   [301:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
;   [301:5] free named register rcx
;   [301:5] free named register rdi
;   [301:5] free named register rax
;   [302:5] print(hello)
;   [35:6] print(str[] i8)
    func.print.302.5:
;       [302:5] alias str -> hello
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
    func.print.302.5.end:
;   [303:5] label
    loop.303.5:
;       [304:9] counter = counter + 1
;       [304:19] instructions without scratch register 1, with 3
;       [304:19] counter
;       [304:29] counter + 1
        add qword [rbp + 1056], 1
;       [305:9] print_num(counter)
;       [305:9] address of argument 'counter' to parameter 'num'
;       [305:9] allocate scratch register -> r15
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
;       [305:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1192]
        call func.print_num
        POP_REGS
;       [306:9] print(colon)
;       [35:6] print(str[] i8)
        func.print.306.9:
;           [306:9] alias str -> colon
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
        func.print.306.9.end:
;       [307:9] print(prompt1)
;       [35:6] print(str[] i8)
        func.print.307.9:
;           [307:9] alias str -> prompt1
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
        func.print.307.9.end:
;       [308:12] nm.input()
;       [76:6] str.input()
        func.str.input.308.12:
;           [308:12] alias self -> nm
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
;           [80:19] instructions without scratch register 3, with 3
;           [80:19] nbytes
;           [80:19] allocate scratch register -> r15
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
;           [80:19] free scratch register r15
;           [80:28] self.len - 1
            sub byte [rbp + 1064], 1
        func.str.input.308.12.end:
        if.310.12:
;       [310:12] ? nm.len <= 0
;       [310:12] ? nm.len <= 0
        cmp.310.12:
        cmp byte [rbp + 1064], 0
        jle loop.303.5.end
        if.310.12.code:
;           [311:13] break
        if.312.19:
;       [312:19] ? nm.len <= 4
;       [312:19] ? nm.len <= 4
        cmp.312.19:
        cmp byte [rbp + 1064], 4
        jg if.310.9.else
        if.312.19.code:
;           [313:13] print(prompt2)
;           [35:6] print(str[] i8)
            func.print.313.13:
;               [313:13] alias str -> prompt2
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
            func.print.313.13.end:
;           [314:13] continue
            jmp loop.303.5
        if.310.9.else:
;           [316:13] print(prompt3)
;           [35:6] print(str[] i8)
            func.print.316.13:
;               [316:13] alias str -> prompt3
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
            func.print.316.13.end:
;           [317:16] nm.output()
;           [83:6] str.output()
            func.str.output.317.16:
;               [317:16] alias self -> nm
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
;               [84:14] line number
                mov r15, 84
                test rdx, rdx
                cmovs rbp, r15
                js baz_bounds_panic
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
            func.str.output.317.16.end:
;           [318:13] print(dot)
;           [35:6] print(str[] i8)
            func.print.318.13:
;               [318:13] alias str -> dot
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
            func.print.318.13.end:
;           [319:13] print(nl)
;           [35:6] print(str[] i8)
            func.print.319.13:
;               [319:13] alias str -> nl
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
            func.print.319.13.end:
        if.310.9.end:
    jmp loop.303.5
    loop.303.5.end:
    mov rdi, 0
    mov rax, 60
    syscall

;
;[108:15] noinline print_num(num)
func.print_num:
;   [108:25] num: i64 (8 B @ [rbx])
;   [110:11] const buf_count = 20
;   [112:5] var buf[buf_count] i8
;   [112:9] buf: i8[20] (20 B @ [rbx + 8])
;   [112:9] zero 20 * 1 B = 20 B
;   [112:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [113:5] var n = num
;   [113:9] n: i64 (8 B @ [rbx + 32])
;   [113:9] n = num
;   [113:13] num
;   [113:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [113:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [113:13] free scratch register r14
;   [113:13] free scratch register r15
;   [114:5] var is_negative bool
;   [114:9] is_negative: bool (1 B @ [rbx + 40])
;   [114:9] zero 1 * 1 B = 1 B
;   [114:5] size <= 32 B, use mov
    mov byte [rbx + 40], 0
    if.118.8:
;   [118:8] ? n < 0
;   [118:8] ? n < 0
    cmp.118.8:
    cmp qword [rbx + 32], 0
    jge if.118.5.end
    if.118.8.code:
;       [119:9] is_negative = true
        mov byte [rbx + 40], 1
    if.118.5.end:
    if.121.8:
;   [121:8] ? n > 0
;   [121:8] ? n > 0
    cmp.121.8:
    cmp qword [rbx + 32], 0
    jle if.121.5.end
    if.121.8.code:
;       [122:9] n = -n
;       [122:14] -n
        neg qword [rbx + 32]
    if.121.5.end:
;   [125:5] var i = buf_count
;   [125:9] i: i64 (8 B @ [rbx + 48])
;   [125:9] i = buf_count
;   [125:13] buf_count
    mov qword [rbx + 48], 20
;   [126:5] label
    loop.126.5:
;       [127:9] i = i - 1
;       [127:13] instructions without scratch register 1, with 3
;       [127:13] i
;       [127:17] i - 1
        sub qword [rbx + 48], 1
;       [128:9] buf[i] = i8('0' - n % 10)
;       [128:13] allocate scratch register -> r15
;       [128:13] set array index
;       [128:13] i
        mov r15, qword [rbx + 48]
;       [128:13] bounds check
;       [128:13] allocate scratch register -> r14
;       [128:13] line number
        mov r14, 128
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [128:13] free scratch register r14
;       [128:18] buf = i8('0' - n % 10)
;       [128:18] = expression
;       [128:18] allocate scratch register -> r14
;           [128:21] '0'
            mov r14, 48
;           [128:29] r14 - n % 10
;           [128:29] allocate scratch register -> r13
;           [128:27] n
            mov r13, qword [rbx + 32]
;           [128:31] r13 % 10
;           [128:31] div const
;           [128:31] allocate named register rax
            mov rax, r13
;           [128:31] allocate named register rdx
            cqo
;           [128:31] allocate scratch register -> r12
            mov r12, 10
            idiv r12
;           [128:31] free scratch register r12
            mov r13, rdx
;           [128:31] free named register rdx
;           [128:31] free named register rax
            sub r14, r13
;           [128:29] free scratch register r13
        mov byte [rbx + r15 + 8], r14b
;       [128:18] free scratch register r14
;       [128:9] free scratch register r15
;       [129:9] n = n / 10
;       [129:13] instructions without scratch register 5, with 7
;       [129:13] n
;       [129:17] n / 10
;       [129:17] div const
;       [129:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [129:17] allocate named register rdx
        cqo
;       [129:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [129:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [129:17] free named register rdx
;       [129:17] free named register rax
        if.130.12:
;       [130:12] ? n == 0
;       [130:12] ? n == 0
        cmp.130.12:
        cmp qword [rbx + 32], 0
        jne loop.126.5
        if.130.12.code:
;           [130:19] break
        if.130.9.end:
    loop.126.5.end:
    if.133.8:
;   [133:8] ? is_negative
;   [133:8] ? is_negative
    cmp.133.8:
    cmp byte [rbx + 40], 0
    je if.133.5.end
    if.133.8.code:
;       [134:9] i = i - 1
;       [134:13] instructions without scratch register 1, with 3
;       [134:13] i
;       [134:17] i - 1
        sub qword [rbx + 48], 1
;       [135:9] buf[i] = '-'
;       [135:13] allocate scratch register -> r15
;       [135:13] set array index
;       [135:13] i
        mov r15, qword [rbx + 48]
;       [135:13] bounds check
;       [135:13] allocate scratch register -> r14
;       [135:13] line number
        mov r14, 135
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [135:13] free scratch register r14
;       [135:18] '-'
        mov byte [rbx + r15 + 8], 45
;       [135:9] free scratch register r15
    if.133.5.end:
;   [138:5] var write_pos
;   [138:9] write_pos: i64 (8 B @ [rbx + 56])
;   [138:9] zero 1 * 8 B = 8 B
;   [138:5] size <= 32 B, use mov
    mov qword [rbx + 56], 0
;   [139:5] label
    loop.139.5:
;       [140:9] buf[write_pos] = buf[i]
;       [140:13] allocate scratch register -> r15
;       [140:13] set array index
;       [140:13] write_pos
        mov r15, qword [rbx + 56]
;       [140:13] bounds check
;       [140:13] allocate scratch register -> r14
;       [140:13] line number
        mov r14, 140
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [140:13] free scratch register r14
;       [140:26] buf[i]
;       [140:30] allocate scratch register -> r14
;       [140:30] set array index
;       [140:30] i
        mov r14, qword [rbx + 48]
;       [140:30] bounds check
;       [140:30] allocate scratch register -> r13
;       [140:30] line number
        mov r13, 140
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [140:30] free scratch register r13
;       [140:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [140:26] free scratch register r13
;       [140:26] free scratch register r14
;       [140:9] free scratch register r15
;       [141:9] write_pos = write_pos + 1
;       [141:21] instructions without scratch register 1, with 3
;       [141:21] write_pos
;       [141:33] write_pos + 1
        add qword [rbx + 56], 1
;       [142:9] i = i + 1
;       [142:13] instructions without scratch register 1, with 3
;       [142:13] i
;       [142:17] i + 1
        add qword [rbx + 48], 1
        if.143.12:
;       [143:12] ? i == buf_count
;       [143:12] ? i == buf_count
        cmp.143.12:
        cmp qword [rbx + 48], 20
        jne loop.139.5
        if.143.12.code:
;           [143:27] break
        if.143.9.end:
    loop.139.5.end:
;   [146:5] write(1, buf, write_pos)
;   [146:5] allocate named register rdi
;   [146:5] allocate named register rsi
;   [146:5] allocate named register rdx
;   [146:11] 1
    mov rdi, 1
;   [146:19] write_pos
    mov rdx, qword [rbx + 56]
;   [146:14] bounds check
;   [146:14] allocate scratch register -> r15
;   [146:14] line number
    mov r15, 146
    test rdx, rdx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rdx, 20
    cmovg rbp, r15
    jg baz_bounds_panic
;   [146:14] free scratch register r15
    lea rsi, [rbx + 8]
;   [146:5] allocate named register rax
    mov rax, 1
    syscall
;   [146:5] free named register rax
;   [146:5] free named register rdx
;   [146:5] free named register rsi
;   [146:5] free named register rdi
    ret
size.func.print_num equ 64
;
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
