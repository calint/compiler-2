# compiler-2: baz

Experimental compiler for a minimalistic, specialized language targeting x86_64
(Linux) via NASM assembler and RV32I via the LLVM assembler running in QEMU
(Linux).

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler
* super-loop program with non-reentrant inlined functions
* limited support for non-inlined functions
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
* optional bounds checking at runtime
  * optional line number
* inlined functions
* limited support for non-inlined functions
* methods on user defined types: `func list.add(x) { ... }` is called as
  `lst.add(x)` with `lst` as the implicit parameter `self`
* partial ub-free support
* keywords: `func`, `type`, `dat`, `var`, `const`, `foo`, `loop`, `if`, `else`,
  `continue`, `break`, `return`
* built-in functions: `array_copy`, `array_length`, `arrays_equal`, `equal`, `read`,
  `write`, `exit`, `i8`, `i16`, `i32`, `i64`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
  code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
  run `prog.baz`
  * optional parameters: _variable storage size_, _bounds check_, with _line number
    information_ and _jump optimizations_ in boolean expression e.g:
    * `./run-baz.sh myprogram.baz --vars=262144`: reserves 262144 bytes for
      variables, no runtime checks
    * `./run-baz.sh myprogram.baz --vars=262144 --checks=upper`: checks upper
      bounds without line number information and is often enough to ensure
      catching negative values (faster)
    * `./run-baz.sh myprogram.baz --vars=262144 --checks=upper,line`: checks
      upper bounds with line number information
    * `./run-baz.sh myprogram.baz --vars=262144 --checks=upper,lower,line`: checks
      bounds with line number information
    * option `--vars=SIZE` reserves variable storage in bytes (default: 65536,
      decimal or `0x` hex, positive multiple of 16)
    * option `--nopt` disables post processing jump optimizations in boolean
      expression
    * option `--reproduce-source` writes reproduced source to `diff.baz`
      and checks that it matches the input
    * to compile for rv32i and run in QEMU use `--target=rv32i`
* to run the tests `qa/coverage/test-all.sh` and see coverage report in
  `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Related

* rv32i soft core fpga implementation running `gen-rv32i.bin` compiled from
  `prog-uart.baz`
  * <https://github.com/calint/tang-nano-9k--riscv--cache-psram>
  * <https://github.com/calint/tang-nano-20k--riscv--cache-sdram>
  * to run in an emulator of fpga soft core rv32i: `./run-rv32i-fpga.sh`
  * to run on hardware use `scripts/fpga-connect-serial.sh`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    55           4696           1490          16201
C++                              1             58             10            273
-------------------------------------------------------------------------------
SUM:                            56           4754           1500          16474
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
    # write is a built-in functions writes to a file descriptor
    # it has 2 more optional arguments: count and start index
}

# function arguments and return are equivalent to mutable references

# functions can act on user types: `func point.fooz()` is called as
# `p.fooz()` with `p` as the implicit reference argument `self`

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
    var buf[20] i8
    var n = num
    var is_negative bool = false
 
    if n < 0 {
        is_negative = true
        n = -n
    }
 
    var i = 20
    loop {
        i = i - 1
        var ascii = 48 + (n % 10)
        # note: not buf[i] = 48 + ... because expression will be executed as
        #       byte sized and n overflows
        buf[i] = i8(ascii) 
        n = n / 10
        if n == 0 break
    }
 
    if is_negative {
        i = i - 1
        buf[i] = 45
    }
 
    var write_pos = 0
    loop {
        buf[write_pos] = buf[i]
        write_pos = write_pos + 1
        i = i + 1
        if i == 20 break
    }
 
    write(1, buf, write_pos)
}

func main() {
    var arr[4] i32
    # arrays are initialized to 0

    var answer
    assert(answer == 0)
    # variables without initializer are zeroed

    answer = maybe
    assert(answer == -1)

    {
        # a code block opens a new scope
        # constants and variables shadow outer scope
        const maybe = 33
        assert(maybe == 33)
    }

    assert(maybe == -1)

    var ix = 1
    # variables can have an initial value that can be an expression

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

    var p point = {0, 0}
    p.fooz()
    assert(p.x == 2)
    assert(p.y == 0xb)

    var q point = p
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
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    cmp.151.12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool.151.12.end:
    func.assert.151.5:
        if.32.27.151.5:
        cmp.32.27.151.5:
        cmp r15b, 0
        jne if.32.24.151.5.end
        if.32.27.151.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.151.5.end:
    func.assert.151.5.end:
    mov qword [rbp + 240], -1
    cmp.155.12:
    cmp qword [rbp + 240], -1
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
        cmp.161.16:
        bool.161.16.end:
        mov r15b, 1
        func.assert.161.9:
            if.32.27.161.9:
            cmp.32.27.161.9:
            cmp r15b, 0
            jne if.32.24.161.9.end
            if.32.27.161.9.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.32.24.161.9.end:
        func.assert.161.9.end:
    cmp.164.12:
    bool.164.12.end:
    mov r15b, 1
    func.assert.164.5:
        if.32.27.164.5:
        cmp.32.27.164.5:
        cmp r15b, 0
        jne if.32.24.164.5.end
        if.32.27.164.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.164.5.end:
    func.assert.164.5.end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 170
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 170
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp.171.12:
    mov r14, 1
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.171.12.end:
    func.assert.171.5:
        if.32.27.171.5:
        cmp.32.27.171.5:
        cmp r15b, 0
        jne if.32.24.171.5.end
        if.32.27.171.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.171.5.end:
    func.assert.171.5.end:
    cmp.172.12:
    mov r14, 2
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.172.12.end:
    func.assert.172.5:
        if.32.27.172.5:
        cmp.32.27.172.5:
        cmp r15b, 0
        jne if.32.24.172.5.end
        if.32.27.172.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.172.5.end:
    func.assert.172.5.end:
    mov r15, 2
    mov r14, 2
    mov r13, 174
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
    mov r13, 174
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
    mov rax, qword [rbp + r14 * 4 + 224]
    mov qword [rbp + 224], rax
    cmp.175.12:
    mov r14, 0
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
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
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov r15, 4
    mov r14, 179
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    mov r14, 179
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    mov rax, qword [rbp + 224]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 232]
    mov qword [rbp + 264], rax
    cmp.180.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 180
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
        lea rsi, [rbp + r15 * 4 + 224]
        mov r15, 1
        mov r14, 180
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
    bool.180.19.end:
    cmp.183.12:
    cmp byte [rbp + 288], 0
    setne r15b
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
    mov r14, 185
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp.186.12:
        mov rcx, 4
        mov r14, 186
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 186
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
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 189
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.189.16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    func.inv.189.16.end:
    not dword [rbp + r15 * 4 + 224]
    cmp.190.12:
    mov r14, qword [rbp + 248]
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.190.12.end:
    func.assert.190.5:
        if.32.27.190.5:
        cmp.32.27.190.5:
        cmp r15b, 0
        jne if.32.24.190.5.end
        if.32.27.190.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.190.5.end:
    func.assert.190.5.end:
    func.faz.192.5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    func.faz.192.5.end:
    cmp.193.12:
    mov r14, 1
    mov r13, 193
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
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
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.196.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.196.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.196.5
    foo.196.5.end:
    cmp.199.12:
    mov r14, 0
    mov r13, 199
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
    bool.199.12.end:
    func.assert.199.5:
        if.32.27.199.5:
        cmp.32.27.199.5:
        cmp r15b, 0
        jne if.32.24.199.5.end
        if.32.27.199.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.199.5.end:
    func.assert.199.5.end:
    cmp.200.12:
    mov r14, 1
    mov r13, 200
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
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.207.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.207.7.end:
    cmp.208.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.208.12.end:
    func.assert.208.5:
        if.32.27.208.5:
        cmp.32.27.208.5:
        cmp r15b, 0
        jne if.32.24.208.5.end
        if.32.27.208.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.208.5.end:
    func.assert.208.5.end:
    cmp.209.12:
    cmp qword [rbp + 320], 11
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
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.212.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool.212.12.end:
    func.assert.212.5:
        if.32.27.212.5:
        cmp.32.27.212.5:
        cmp r15b, 0
        jne if.32.24.212.5.end
        if.32.27.212.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.212.5.end:
    func.assert.212.5.end:
    mov qword [rbp + 328], 3
    cmp.217.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
        setne r15b
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
    mov qword [rbp + 344], 0
    func.bar.220.5:
        if.55.8.220.5:
        cmp.55.8.220.5:
        cmp qword [rbp + 344], 0
        jne if.55.5.220.5.end
        if.55.8.220.5.code:
            jmp func.bar.220.5.end
        if.55.5.220.5.end:
        mov qword [rbp + 344], 255
    func.bar.220.5.end:
    cmp.221.12:
    cmp qword [rbp + 344], 0
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
    mov qword [rbp + 344], 1
    func.bar.224.5:
        if.55.8.224.5:
        cmp.55.8.224.5:
        cmp qword [rbp + 344], 0
        jne if.55.5.224.5.end
        if.55.8.224.5.code:
            jmp func.bar.224.5.end
        if.55.5.224.5.end:
        mov qword [rbp + 344], 255
    func.bar.224.5.end:
    cmp.225.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.225.12.end:
    func.assert.225.5:
        if.32.27.225.5:
        cmp.32.27.225.5:
        cmp r15b, 0
        jne if.32.24.225.5.end
        if.32.27.225.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.225.5.end:
    func.assert.225.5.end:
    mov qword [rbp + 352], 1
    func.baz.228.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.228.13.end:
    cmp.229.12:
    cmp qword [rbp + 360], 2
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
    func.baz.231.9:
        mov qword [rbp + 360], 1
        sal qword [rbp + 360], 1
    func.baz.231.9.end:
    cmp.232.12:
    cmp qword [rbp + 360], 2
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
    func.baz.234.21:
        mov qword [rbp + 368], 3
        sal qword [rbp + 368], 1
    func.baz.234.21.end:
    mov qword [rbp + 376], 0
    cmp.235.12:
    cmp qword [rbp + 368], 6
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
    func.point_init.237.20:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point_init.237.20.end:
    cmp.238.12:
    cmp qword [rbp + 384], -1
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
    cmp.239.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.239.12.end:
    func.assert.239.5:
        if.32.27.239.5:
        cmp.32.27.239.5:
        cmp r15b, 0
        jne if.32.24.239.5.end
        if.32.27.239.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.239.5.end:
    func.assert.239.5.end:
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.245.12:
    cmp qword [rbp + 416], 10
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
    cmp.246.12:
    cmp qword [rbp + 424], 2
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
    cmp.247.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.247.12.end:
    func.assert.247.5:
        if.32.27.247.5:
        cmp.32.27.247.5:
        cmp r15b, 0
        jne if.32.24.247.5.end
        if.32.27.247.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.247.5.end:
    func.assert.247.5.end:
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
    cmp.251.12:
    cmp qword [rbp + 416], -1
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
    cmp.252.12:
    cmp qword [rbp + 424], -2
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
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.255.12:
    cmp qword [rbp + 456], -1
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
    cmp.256.12:
    cmp qword [rbp + 464], -2
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
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.263.12:
    mov r14, 0
    mov r13, 263
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 73
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
    mov r15, 1
    mov r14, 264
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
    imul r15, 24
    func.object_init.264.13:
        mov qword [rbp + r15 + 480], 2
        mov qword [rbp + r15 + 488], 74
        mov dword [rbp + r15 + 496], 16777215
    func.object_init.264.13.end:
    cmp.265.12:
    mov r14, 1
    mov r13, 265
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 74
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
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov r15, 1
    mov r14, 268
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    lea r15, [rbp + r15 + 528]
    mov r14, 1
    mov r13, 268
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp.269.12:
    mov r14, 1
    mov r13, 269
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 269
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    mov r15, 1
    mov r14, 272
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 272
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 + 528]
    mov r15, 0
    mov r14, 273
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 273
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [rbp + r15 + 528]
    shl rcx, 3
    rep movsb
    cmp.278.12:
    mov r14, 0
    mov r13, 278
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 278
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    cmp.279.12:
        mov rcx, 8
        mov r14, 0
        mov r13, 280
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 280
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 + 528]
        mov r14, 1
        mov r13, 281
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 281
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
    bool.279.12.end:
    func.assert.279.5:
        if.32.27.279.5:
        cmp.32.27.279.5:
        cmp r15b, 0
        jne if.32.24.279.5.end
        if.32.27.279.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.279.5.end:
    func.assert.279.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.285.12:
        mov r14, 2
    cmp r14, 2
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
    mov r14, 0
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], -1
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
    cmp.287.12:
    mov r14, 1
    mov r13, 287
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], 2
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
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.291.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.291.5.end:
    loop.292.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        PUSH_REGS
        lea rbx, [rbp + 1192]
        call func.print_num
        POP_REGS
        func.print.295.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.295.9.end:
        func.print.296.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.296.9.end:
        func.str.input.297.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.297.12.end:
        if.299.12:
        cmp.299.12:
        cmp byte [rbp + 1064], 0
        jg if.301.19
        if.299.12.code:
            jmp loop.292.5.end
        if.301.19:
        cmp.301.19:
        cmp byte [rbp + 1064], 4
        jg if.299.9.else
        if.301.19.code:
            func.print.302.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.302.13.end:
            jmp loop.292.5
        if.299.9.else:
            func.print.305.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.305.13.end:
            func.str.output.306.16:
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
            func.str.output.306.16.end:
            func.print.307.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.307.13.end:
            func.print.308.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.308.13.end:
        if.299.9.end:
    jmp loop.292.5
    loop.292.5.end:
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
    if.114.8:
    cmp.114.8:
    cmp qword [rbx + 32], 0
    jge if.114.5.end
    if.114.8.code:
        mov byte [rbx + 40], 1
        neg qword [rbx + 32]
    if.114.5.end:
    mov qword [rbx + 48], 20
    loop.120.5:
        sub qword [rbx + 48], 1
        mov qword [rbx + 56], 48
        mov r15, qword [rbx + 32]
        mov rax, r15
        cqo
        mov r14, 10
        idiv r14
        mov r15, rdx
        add qword [rbx + 56], r15
        mov r15, qword [rbx + 48]
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14b, byte [rbx + 56]
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 32]
        cqo
        mov r15, 10
        idiv r15
        mov qword [rbx + 32], rax
        if.127.12:
        cmp.127.12:
        cmp qword [rbx + 32], 0
        jne if.127.9.end
        if.127.12.code:
            jmp loop.120.5.end
        if.127.9.end:
    jmp loop.120.5
    loop.120.5.end:
    if.130.8:
    cmp.130.8:
    cmp byte [rbx + 40], 0
    je if.130.5.end
    if.130.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 132
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.130.5.end:
    mov qword [rbx + 56], 0
    loop.136.5:
        mov r15, qword [rbx + 56]
        mov r14, 137
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        mov r13, 137
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
        if.140.12:
        cmp.140.12:
        cmp qword [rbx + 48], 20
        jne if.140.9.end
        if.140.12.code:
            jmp loop.136.5.end
        if.140.9.end:
    jmp loop.136.5
    loop.136.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    mov r15, 143
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
;   [147:5] var arr[4] i32
;   [147:9] arr: i32[4] (16 B @ [rbp + 224])
;   [147:9] zero 4 * 4 B = 16 B
;   [147:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
;   [150:5] var answer
;   [150:9] answer: i64 (8 B @ [rbp + 240])
;   [150:9] zero 1 * 8 B = 8 B
;   [150:5] size <= 32 B, use mov
    mov qword [rbp + 240], 0
;   [151:5] assert(answer == 0)
;   [151:12] allocate scratch register -> r15
;   [151:12] ? answer == 0
;   [151:12] ? answer == 0
    cmp.151.12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool.151.12.end:
;   [32:6] assert(ok bool)
    func.assert.151.5:
;       [151:5] alias ok -> r15b
        if.32.27.151.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.151.5:
        cmp r15b, 0
        jne if.32.24.151.5.end
        if.32.27.151.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.151.5.end:
;       [151:5] free scratch register r15
    func.assert.151.5.end:
;   [154:5] answer = maybe
;   [154:14] maybe
    mov qword [rbp + 240], -1
;   [155:5] assert(answer == -1)
;   [155:12] allocate scratch register -> r15
;   [155:12] ? answer == -1
;   [155:12] ? answer == -1
    cmp.155.12:
    cmp qword [rbp + 240], -1
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
;       [161:16] allocate scratch register -> r15
;       [161:16] ? maybe == 33
;       [161:16] ? maybe == 33
        cmp.161.16:
;       [161:16] const eval to true
        bool.161.16.end:
        mov r15b, 1
;       [32:6] assert(ok bool)
        func.assert.161.9:
;           [161:9] alias ok -> r15b
            if.32.27.161.9:
;           [32:27] ? not ok
;           [32:27] ? not ok
            cmp.32.27.161.9:
            cmp r15b, 0
            jne if.32.24.161.9.end
            if.32.27.161.9.code:
;               [32:34] exit(1)
;               [32:34] allocate named register rdi
;               [32:39] 1
                mov rdi, 1
                mov rax, 60
                syscall
;               [32:34] free named register rdi
            if.32.24.161.9.end:
;           [161:9] free scratch register r15
        func.assert.161.9.end:
;   [164:5] assert(maybe == -1)
;   [164:12] allocate scratch register -> r15
;   [164:12] ? maybe == -1
;   [164:12] ? maybe == -1
    cmp.164.12:
;   [164:12] const eval to true
    bool.164.12.end:
    mov r15b, 1
;   [32:6] assert(ok bool)
    func.assert.164.5:
;       [164:5] alias ok -> r15b
        if.32.27.164.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.164.5:
        cmp r15b, 0
        jne if.32.24.164.5.end
        if.32.27.164.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.164.5.end:
;       [164:5] free scratch register r15
    func.assert.164.5.end:
;   [166:5] var ix = 1
;   [166:9] ix: i64 (8 B @ [rbp + 248])
;   [166:9] ix = 1
;   [166:14] 1
    mov qword [rbp + 248], 1
;   [169:5] arr[ix] = 2
;   [169:9] allocate scratch register -> r15
;   [169:9] set array index
;   [169:9] ix
    mov r15, qword [rbp + 248]
;   [169:9] bounds check
;   [169:9] allocate scratch register -> r14
;   [169:9] line number
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [169:9] free scratch register r14
;   [169:15] 2
    mov dword [rbp + r15 * 4 + 224], 2
;   [169:5] free scratch register r15
;   [170:5] arr[ix + 1] = arr[ix]
;   [170:9] allocate scratch register -> r15
;   [170:9] set array index
;   [170:9] ix
    mov r15, qword [rbp + 248]
;   [170:14] r15 + 1
    add r15, 1
;   [170:9] bounds check
;   [170:9] allocate scratch register -> r14
;   [170:9] line number
    mov r14, 170
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [170:9] free scratch register r14
;   [170:19] arr[ix]
;   [170:23] allocate scratch register -> r14
;   [170:23] set array index
;   [170:23] ix
    mov r14, qword [rbp + 248]
;   [170:23] bounds check
;   [170:23] allocate scratch register -> r13
;   [170:23] line number
    mov r13, 170
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [170:23] free scratch register r13
;   [170:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
;   [170:19] free scratch register r13
;   [170:19] free scratch register r14
;   [170:5] free scratch register r15
;   [171:5] assert(arr[1] == 2)
;   [171:12] allocate scratch register -> r15
;   [171:12] ? arr[1] == 2
;   [171:12] ? arr[1] == 2
    cmp.171.12:
;   [171:16] allocate scratch register -> r14
;   [171:16] set array index
;   [171:16] 1
    mov r14, 1
;   [171:16] bounds check
;   [171:16] allocate scratch register -> r13
;   [171:16] line number
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [171:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [171:12] free scratch register r14
    sete r15b
    bool.171.12.end:
;   [32:6] assert(ok bool)
    func.assert.171.5:
;       [171:5] alias ok -> r15b
        if.32.27.171.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.171.5:
        cmp r15b, 0
        jne if.32.24.171.5.end
        if.32.27.171.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.171.5.end:
;       [171:5] free scratch register r15
    func.assert.171.5.end:
;   [172:5] assert(arr[2] == 2)
;   [172:12] allocate scratch register -> r15
;   [172:12] ? arr[2] == 2
;   [172:12] ? arr[2] == 2
    cmp.172.12:
;   [172:16] allocate scratch register -> r14
;   [172:16] set array index
;   [172:16] 2
    mov r14, 2
;   [172:16] bounds check
;   [172:16] allocate scratch register -> r13
;   [172:16] line number
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [172:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [172:12] free scratch register r14
    sete r15b
    bool.172.12.end:
;   [32:6] assert(ok bool)
    func.assert.172.5:
;       [172:5] alias ok -> r15b
        if.32.27.172.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.172.5:
        cmp r15b, 0
        jne if.32.24.172.5.end
        if.32.27.172.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.172.5.end:
;       [172:5] free scratch register r15
    func.assert.172.5.end:
;   [174:5] array_copy(arr[2], arr, 2)
;   [174:5] allocate scratch register -> r15
;   [174:29] 2
;   [174:29] 2
    mov r15, 2
;   [174:16] arr[2]
;   [174:20] allocate scratch register -> r14
;   [174:20] set array index
;   [174:20] 2
    mov r14, 2
;   [174:20] bounds check
;   [174:20] allocate scratch register -> r13
;   [174:20] line number
    mov r13, 174
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
;   [174:20] allocate scratch register -> r12
    mov r12, r15
    add r12, r14
    cmp r12, 4
;   [174:20] free scratch register r12
    cmovg rbp, r13
    jg baz_bounds_panic
;   [174:20] free scratch register r13
;   [174:24] arr
;   [174:24] bounds check
;   [174:24] allocate scratch register -> r13
;   [174:24] line number
    mov r13, 174
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
;   [174:24] free scratch register r13
;   [174:5] size <= 16 B, use mov
;   [174:5] allocate named register rax
    mov rax, qword [rbp + r14 * 4 + 224]
    mov qword [rbp + 224], rax
;   [174:5] free named register rax
;   [174:5] free scratch register r14
;   [174:5] free scratch register r15
;   [175:5] assert(arr[0] == 2)
;   [175:12] allocate scratch register -> r15
;   [175:12] ? arr[0] == 2
;   [175:12] ? arr[0] == 2
    cmp.175.12:
;   [175:16] allocate scratch register -> r14
;   [175:16] set array index
;   [175:16] 0
    mov r14, 0
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
    cmp dword [rbp + r14 * 4 + 224], 2
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
;   [178:5] var arr1[8] i32
;   [178:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [178:9] zero 8 * 4 B = 32 B
;   [178:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [179:5] array_copy(arr, arr1, 4)
;   [179:5] allocate scratch register -> r15
;   [179:27] 4
;   [179:27] 4
    mov r15, 4
;   [179:16] arr
;   [179:16] bounds check
;   [179:16] allocate scratch register -> r14
;   [179:16] line number
    mov r14, 179
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
;   [179:16] free scratch register r14
;   [179:21] arr1
;   [179:21] bounds check
;   [179:21] allocate scratch register -> r14
;   [179:21] line number
    mov r14, 179
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [179:21] free scratch register r14
;   [179:5] size <= 16 B, use mov
;   [179:5] allocate named register rax
    mov rax, qword [rbp + 224]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 232]
    mov qword [rbp + 264], rax
;   [179:5] free named register rax
;   [179:5] free scratch register r15
;   [180:5] var eq bool = arrays_equal(arr[1], arr1[1], 3)
;   [180:9] eq: bool (1 B @ [rbp + 288])
;   [180:9] eq = arrays_equal(arr[1], arr1[1], 3)
;   [180:19] ? arrays_equal(arr[1], arr1[1], 3)
;   [180:19] ? arrays_equal(arr[1], arr1[1], 3)
    cmp.180.19:
;       [180:19] arrays_equal(arr[1], arr1[1], 3)
;       [180:19] allocate named register rsi
;       [180:19] allocate named register rdi
;       [180:19] allocate named register rcx
;       [180:49] 3
;       [180:49] 3
        mov rcx, 3
;       [180:32] arr[1]
;       [180:36] allocate scratch register -> r15
;       [180:36] set array index
;       [180:36] 1
        mov r15, 1
;       [180:36] bounds check
;       [180:36] allocate scratch register -> r14
;       [180:36] line number
        mov r14, 180
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [180:36] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 4
;       [180:36] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [180:36] free scratch register r14
        lea rsi, [rbp + r15 * 4 + 224]
;       [180:19] free scratch register r15
;       [180:40] arr1[1]
;       [180:45] allocate scratch register -> r15
;       [180:45] set array index
;       [180:45] 1
        mov r15, 1
;       [180:45] bounds check
;       [180:45] allocate scratch register -> r14
;       [180:45] line number
        mov r14, 180
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
;       [180:45] allocate scratch register -> r13
        mov r13, rcx
        add r13, r15
        cmp r13, 8
;       [180:45] free scratch register r13
        cmovg rbp, r14
        jg baz_bounds_panic
;       [180:45] free scratch register r14
        lea rdi, [rbp + r15 * 4 + 256]
;       [180:19] free scratch register r15
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [180:19] free named register rcx
;       [180:19] free named register rdi
;       [180:19] free named register rsi
        sete byte [rbp + 288]
    bool.180.19.end:
;   [183:5] assert(eq)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? eq
;   [183:12] ? eq
    cmp.183.12:
    cmp byte [rbp + 288], 0
    setne r15b
    bool.183.12.end:
;   [32:6] assert(ok bool)
    func.assert.183.5:
;       [183:5] alias ok -> r15b
        if.32.27.183.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [185:5] arr1[2] = -1
;   [185:10] allocate scratch register -> r15
;   [185:10] set array index
;   [185:10] 2
    mov r15, 2
;   [185:10] bounds check
;   [185:10] allocate scratch register -> r14
;   [185:10] line number
    mov r14, 185
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [185:10] free scratch register r14
;   [185:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [185:5] free scratch register r15
;   [186:5] assert(not arrays_equal(arr, arr1, 4))
;   [186:12] allocate scratch register -> r15
;   [186:12] ? not arrays_equal(arr, arr1, 4)
;   [186:12] ? not arrays_equal(arr, arr1, 4)
    cmp.186.12:
;       [186:16] arrays_equal(arr, arr1, 4)
;       [186:16] allocate named register rsi
;       [186:16] allocate named register rdi
;       [186:16] allocate named register rcx
;       [186:40] 4
;       [186:40] 4
        mov rcx, 4
;       [186:29] arr
;       [186:29] bounds check
;       [186:29] allocate scratch register -> r14
;       [186:29] line number
        mov r14, 186
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
;       [186:29] free scratch register r14
        lea rsi, [rbp + 224]
;       [186:34] arr1
;       [186:34] bounds check
;       [186:34] allocate scratch register -> r14
;       [186:34] line number
        mov r14, 186
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
;       [186:34] free scratch register r14
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
;       [186:16] free named register rcx
;       [186:16] free named register rdi
;       [186:16] free named register rsi
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
;   [188:5] ix = 3
;   [188:10] 3
    mov qword [rbp + 248], 3
;   [189:5] arr[ix] = ~inv(arr[ix - 1])
;   [189:9] allocate scratch register -> r15
;   [189:9] set array index
;   [189:9] ix
    mov r15, qword [rbp + 248]
;   [189:9] bounds check
;   [189:9] allocate scratch register -> r14
;   [189:9] line number
    mov r14, 189
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [189:9] free scratch register r14
;   [189:16] arr = ~inv(arr[ix - 1])
;   [189:16] = expression
;   [189:16] ~inv(arr[ix - 1])
;   [189:24] allocate scratch register -> r14
;   [189:24] set array index
;   [189:24] ix
    mov r14, qword [rbp + 248]
;   [189:29] r14 - 1
    sub r14, 1
;   [189:24] bounds check
;   [189:24] allocate scratch register -> r13
;   [189:24] line number
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [189:24] free scratch register r13
;   [62:6] inv(i i32) res i32
    func.inv.189.16:
;       [189:16] alias res -> arr (lea: rbp + r15 * 4 + 224)
;       [189:16] alias i -> arr (lea: rbp + r14 * 4 + 224)
;       [63:5] res = ~i
;       [63:12] ~i
;       [63:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
;       [63:12] free scratch register r13
        not dword [rbp + r15 * 4 + 224]
;       [189:16] free scratch register r14
    func.inv.189.16.end:
    not dword [rbp + r15 * 4 + 224]
;   [189:5] free scratch register r15
;   [190:5] assert(arr[ix] == 2)
;   [190:12] allocate scratch register -> r15
;   [190:12] ? arr[ix] == 2
;   [190:12] ? arr[ix] == 2
    cmp.190.12:
;   [190:16] allocate scratch register -> r14
;   [190:16] set array index
;   [190:16] ix
    mov r14, qword [rbp + 248]
;   [190:16] bounds check
;   [190:16] allocate scratch register -> r13
;   [190:16] line number
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [190:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [190:12] free scratch register r14
    sete r15b
    bool.190.12.end:
;   [32:6] assert(ok bool)
    func.assert.190.5:
;       [190:5] alias ok -> r15b
        if.32.27.190.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.190.5:
        cmp r15b, 0
        jne if.32.24.190.5.end
        if.32.27.190.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.190.5.end:
;       [190:5] free scratch register r15
    func.assert.190.5.end:
;   [192:5] faz(arr)
;   [72:6] faz(arg[] i32)
    func.faz.192.5:
;       [192:5] alias arg -> arr
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
        mov dword [rbp + r15 * 4 + 224], 254
;       [73:5] free scratch register r15
    func.faz.192.5.end:
;   [193:5] assert(arr[1] == 0xfe)
;   [193:12] allocate scratch register -> r15
;   [193:12] ? arr[1] == 0xfe
;   [193:12] ? arr[1] == 0xfe
    cmp.193.12:
;   [193:16] allocate scratch register -> r14
;   [193:16] set array index
;   [193:16] 1
    mov r14, 1
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
    cmp dword [rbp + r14 * 4 + 224], 254
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
;   [195:5] var arr3[] = { 3, 5 }
;   [195:9] arr3: i64[2] (16 B @ [rbp + 296])
;   [195:9] arr3= { 3, 5 }
;   [195:18] size <= 16 B, use immediates
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
;   [196:5] foo arr3
;   [196:9] allocate scratch register -> r15
;   [196:9] e: i64 (r15)
;   [196:9] i: i64 (8 B @ [rbp + 320])
;   [196:9] const n = 2
;   [196:9] initiate iterator e
    lea r15, [rbp + 296]
;   [196:9] initiate counter i
    mov qword [rbp + 320], 0
    foo.196.5:
;       [197:9] e = e + i + n
;       [197:13] instructions without scratch register 3, with 4
;       [197:13] e
;       [197:17] e + i
;       [197:17] allocate scratch register -> r14
        mov r14, qword [rbp + 320]
        add qword [r15], r14
;       [197:17] free scratch register r14
;       [197:21] e + n
        add qword [r15], 2
        foo.196.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.196.5
    foo.196.5.end:
;   [196:5] free scratch register r15
;   [199:5] assert(arr3[0] == 3 + 0 + 2)
;   [199:12] allocate scratch register -> r15
;   [199:12] ? arr3[0] == 3 + 0 + 2
;   [199:12] ? arr3[0] == 3 + 0 + 2
    cmp.199.12:
;   [199:17] allocate scratch register -> r14
;   [199:17] set array index
;   [199:17] 0
    mov r14, 0
;   [199:17] bounds check
;   [199:17] allocate scratch register -> r13
;   [199:17] line number
    mov r13, 199
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [199:17] free scratch register r13
;   [199:23] allocate scratch register -> r13
;       [199:23] 3
        mov r13, 3
;       [199:27] r13 + 0
        add r13, 0
;       [199:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
;   [199:12] free scratch register r13
;   [199:12] free scratch register r14
    sete r15b
    bool.199.12.end:
;   [32:6] assert(ok bool)
    func.assert.199.5:
;       [199:5] alias ok -> r15b
        if.32.27.199.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.199.5:
        cmp r15b, 0
        jne if.32.24.199.5.end
        if.32.27.199.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.199.5.end:
;       [199:5] free scratch register r15
    func.assert.199.5.end:
;   [200:5] assert(arr3[1] == 5 + 1 + 2)
;   [200:12] allocate scratch register -> r15
;   [200:12] ? arr3[1] == 5 + 1 + 2
;   [200:12] ? arr3[1] == 5 + 1 + 2
    cmp.200.12:
;   [200:17] allocate scratch register -> r14
;   [200:17] set array index
;   [200:17] 1
    mov r14, 1
;   [200:17] bounds check
;   [200:17] allocate scratch register -> r13
;   [200:17] line number
    mov r13, 200
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [200:17] free scratch register r13
;   [200:23] allocate scratch register -> r13
;       [200:23] 5
        mov r13, 5
;       [200:27] r13 + 1
        add r13, 1
;       [200:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
;   [200:12] free scratch register r13
;   [200:12] free scratch register r14
    sete r15b
    bool.200.12.end:
;   [32:6] assert(ok bool)
    func.assert.200.5:
;       [200:5] alias ok -> r15b
        if.32.27.200.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [206:5] var p point = {0, 0}
;   [206:9] p: point (16 B @ [rbp + 312])
;   [206:9] p = {0, 0}
;   [206:20] copy field 'x'
    mov qword [rbp + 312], 0
;   [206:23] copy field 'y'
    mov qword [rbp + 320], 0
;   [207:7] p.fooz()
;   [46:6] point.fooz()
    func.point.fooz.207.7:
;       [207:7] alias self -> p
;       [47:5] self.x = 0b10
;       [47:14] 0b10
        mov qword [rbp + 312], 2
;       [48:5] self.y = 0xb
;       [48:14] 0xb
        mov qword [rbp + 320], 11
    func.point.fooz.207.7.end:
;   [208:5] assert(p.x == 2)
;   [208:12] allocate scratch register -> r15
;   [208:12] ? p.x == 2
;   [208:12] ? p.x == 2
    cmp.208.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.208.12.end:
;   [32:6] assert(ok bool)
    func.assert.208.5:
;       [208:5] alias ok -> r15b
        if.32.27.208.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.208.5:
        cmp r15b, 0
        jne if.32.24.208.5.end
        if.32.27.208.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.208.5.end:
;       [208:5] free scratch register r15
    func.assert.208.5.end:
;   [209:5] assert(p.y == 0xb)
;   [209:12] allocate scratch register -> r15
;   [209:12] ? p.y == 0xb
;   [209:12] ? p.y == 0xb
    cmp.209.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.209.12.end:
;   [32:6] assert(ok bool)
    func.assert.209.5:
;       [209:5] alias ok -> r15b
        if.32.27.209.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [211:5] var q point = p
;   [211:9] q: point (16 B @ [rbp + 328])
;   [211:9] q = p
;   [211:19] size <= 16 B, use mov
;   [211:19] allocate named register rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
;   [211:19] free named register rax
;   [212:5] assert(equal(p, q))
;   [212:12] allocate scratch register -> r15
;   [212:12] ? equal(p, q)
;   [212:12] ? equal(p, q)
    cmp.212.12:
;       [212:12] equal(p, q)
;       [212:12] allocate named register rsi
;       [212:12] allocate named register rdi
;       [212:12] allocate named register rcx
;       [212:18] p
        lea rsi, [rbp + 312]
;       [212:21] q
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
;       [212:12] free named register rcx
;       [212:12] free named register rdi
;       [212:12] free named register rsi
        sete r15b
    bool.212.12.end:
;   [32:6] assert(ok bool)
    func.assert.212.5:
;       [212:5] alias ok -> r15b
        if.32.27.212.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.212.5:
        cmp r15b, 0
        jne if.32.24.212.5.end
        if.32.27.212.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.212.5.end:
;       [212:5] free scratch register r15
    func.assert.212.5.end:
;   [216:5] q.x = 3
;   [216:11] 3
    mov qword [rbp + 328], 3
;   [217:5] assert(not equal(p, q))
;   [217:12] allocate scratch register -> r15
;   [217:12] ? not equal(p, q)
;   [217:12] ? not equal(p, q)
    cmp.217.12:
;       [217:16] equal(p, q)
;       [217:16] allocate named register rsi
;       [217:16] allocate named register rdi
;       [217:16] allocate named register rcx
;       [217:22] p
        lea rsi, [rbp + 312]
;       [217:25] q
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
;       [217:16] free named register rcx
;       [217:16] free named register rdi
;       [217:16] free named register rsi
        setne r15b
    bool.217.12.end:
;   [32:6] assert(ok bool)
    func.assert.217.5:
;       [217:5] alias ok -> r15b
        if.32.27.217.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [219:5] var i = 0
;   [219:9] i: i64 (8 B @ [rbp + 344])
;   [219:9] i = 0
;   [219:13] 0
    mov qword [rbp + 344], 0
;   [220:5] bar(i)
;   [54:6] bar(arg)
    func.bar.220.5:
;       [220:5] alias arg -> i
        if.55.8.220.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.220.5:
        cmp qword [rbp + 344], 0
        jne if.55.5.220.5.end
        if.55.8.220.5.code:
;           [55:17] return
            jmp func.bar.220.5.end
        if.55.5.220.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.220.5.end:
;   [221:5] assert(i == 0)
;   [221:12] allocate scratch register -> r15
;   [221:12] ? i == 0
;   [221:12] ? i == 0
    cmp.221.12:
    cmp qword [rbp + 344], 0
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
;   [223:5] i = 1
;   [223:9] 1
    mov qword [rbp + 344], 1
;   [224:5] bar(i)
;   [54:6] bar(arg)
    func.bar.224.5:
;       [224:5] alias arg -> i
        if.55.8.224.5:
;       [55:8] ? arg == 0
;       [55:8] ? arg == 0
        cmp.55.8.224.5:
        cmp qword [rbp + 344], 0
        jne if.55.5.224.5.end
        if.55.8.224.5.code:
;           [55:17] return
            jmp func.bar.224.5.end
        if.55.5.224.5.end:
;       [56:5] arg = 0xff
;       [56:11] 0xff
        mov qword [rbp + 344], 255
    func.bar.224.5.end:
;   [225:5] assert(i == 0xff)
;   [225:12] allocate scratch register -> r15
;   [225:12] ? i == 0xff
;   [225:12] ? i == 0xff
    cmp.225.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.225.12.end:
;   [32:6] assert(ok bool)
    func.assert.225.5:
;       [225:5] alias ok -> r15b
        if.32.27.225.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.225.5:
        cmp r15b, 0
        jne if.32.24.225.5.end
        if.32.27.225.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.225.5.end:
;       [225:5] free scratch register r15
    func.assert.225.5.end:
;   [227:5] var j = 1
;   [227:9] j: i64 (8 B @ [rbp + 352])
;   [227:9] j = 1
;   [227:13] 1
    mov qword [rbp + 352], 1
;   [228:5] var k = baz(j)
;   [228:9] k: i64 (8 B @ [rbp + 360])
;   [228:9] k = baz(j)
;   [228:13] k = baz(j)
;   [228:13] = expression
;   [228:13] baz(j)
;   [66:6] baz(arg) res
    func.baz.228.13:
;       [228:13] alias res -> k
;       [228:13] alias arg -> j
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
    func.baz.228.13.end:
;   [229:5] assert(k == 2)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? k == 2
;   [229:12] ? k == 2
    cmp.229.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.229.12.end:
;   [32:6] assert(ok bool)
    func.assert.229.5:
;       [229:5] alias ok -> r15b
        if.32.27.229.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [231:5] k = baz(1)
;   [231:9] k = baz(1)
;   [231:9] = expression
;   [231:9] baz(1)
;   [66:6] baz(arg) res
    func.baz.231.9:
;       [231:9] alias res -> k
;       [231:9] alias arg -> 1
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 2, with 3
;       [67:11] arg
        mov qword [rbp + 360], 1
;       [67:17] res * 2
;       [67:17] dst is not reg, src is const
        sal qword [rbp + 360], 1
    func.baz.231.9.end:
;   [232:5] assert(k == 2)
;   [232:12] allocate scratch register -> r15
;   [232:12] ? k == 2
;   [232:12] ? k == 2
    cmp.232.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.232.12.end:
;   [32:6] assert(ok bool)
    func.assert.232.5:
;       [232:5] alias ok -> r15b
        if.32.27.232.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [234:5] var p0 point = {baz(3), 0}
;   [234:9] p0: point (16 B @ [rbp + 368])
;   [234:9] p0 = {baz(3), 0}
;   [234:21] copy field 'x'
;   [234:21] p0.x = baz(3)
;   [234:21] = expression
;   [234:21] baz(3)
;   [66:6] baz(arg) res
    func.baz.234.21:
;       [234:21] alias res -> p0.x (lea: rbp + 368)
;       [234:21] alias arg -> 3
;       [67:5] res = arg * 2
;       [67:11] instructions without scratch register 2, with 3
;       [67:11] arg
        mov qword [rbp + 368], 3
;       [67:17] res * 2
;       [67:17] dst is not reg, src is const
        sal qword [rbp + 368], 1
    func.baz.234.21.end:
;   [234:29] copy field 'y'
    mov qword [rbp + 376], 0
;   [235:5] assert(p0.x == 6)
;   [235:12] allocate scratch register -> r15
;   [235:12] ? p0.x == 6
;   [235:12] ? p0.x == 6
    cmp.235.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.235.12.end:
;   [32:6] assert(ok bool)
    func.assert.235.5:
;       [235:5] alias ok -> r15b
        if.32.27.235.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [237:5] var pt point = point_init()
;   [237:9] pt: point (16 B @ [rbp + 384])
;   [237:9] pt = point_init()
;   [237:20] point_init()
;   [87:6] point_init() res point
    func.point_init.237.20:
;       [237:20] alias res -> pt
;       [88:5] res.x = -1
;       [88:14] -1
        mov qword [rbp + 384], -1
;       [89:5] res.y = -2
;       [89:14] -2
        mov qword [rbp + 392], -2
    func.point_init.237.20.end:
;   [238:5] assert(pt.x == -1)
;   [238:12] allocate scratch register -> r15
;   [238:12] ? pt.x == -1
;   [238:12] ? pt.x == -1
    cmp.238.12:
    cmp qword [rbp + 384], -1
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
;   [239:5] assert(pt.y == -2)
;   [239:12] allocate scratch register -> r15
;   [239:12] ? pt.y == -2
;   [239:12] ? pt.y == -2
    cmp.239.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.239.12.end:
;   [32:6] assert(ok bool)
    func.assert.239.5:
;       [239:5] alias ok -> r15b
        if.32.27.239.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.239.5:
        cmp r15b, 0
        jne if.32.24.239.5.end
        if.32.27.239.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.239.5.end:
;       [239:5] free scratch register r15
    func.assert.239.5.end:
;   [241:5] var x = 1
;   [241:9] x: i64 (8 B @ [rbp + 400])
;   [241:9] x = 1
;   [241:13] 1
    mov qword [rbp + 400], 1
;   [242:5] var y = 2
;   [242:9] y: i64 (8 B @ [rbp + 408])
;   [242:9] y = 2
;   [242:13] 2
    mov qword [rbp + 408], 2
;   [244:5] var o1 object = {{x * 10, y}, 0xff0000}
;   [244:9] o1: object (24 B @ [rbp + 416])
;   [244:9] o1 = {{x * 10, y}, 0xff0000}
;   [244:22] copy field 'pos'
;   [244:23] copy field 'x'
;   [244:23] instructions without scratch register 5, with 3
;   [244:23] allocate scratch register -> r15
;   [244:23] x
    mov r15, qword [rbp + 400]
;   [244:27] r15 * 10
;   [244:27] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 416], r15
;   [244:23] free scratch register r15
;   [244:31] copy field 'y'
;   [244:31] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
;   [244:31] free scratch register r15
;   [244:35] copy field 'color'
    mov dword [rbp + 432], 16711680
;   [244:21] zero padding: 4 B
;   [244:21] size <= 32 B, use mov
    mov dword [rbp + 436], 0
;   [245:5] assert(o1.pos.x == 10)
;   [245:12] allocate scratch register -> r15
;   [245:12] ? o1.pos.x == 10
;   [245:12] ? o1.pos.x == 10
    cmp.245.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.245.12.end:
;   [32:6] assert(ok bool)
    func.assert.245.5:
;       [245:5] alias ok -> r15b
        if.32.27.245.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [246:5] assert(o1.pos.y == 2)
;   [246:12] allocate scratch register -> r15
;   [246:12] ? o1.pos.y == 2
;   [246:12] ? o1.pos.y == 2
    cmp.246.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.246.12.end:
;   [32:6] assert(ok bool)
    func.assert.246.5:
;       [246:5] alias ok -> r15b
        if.32.27.246.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [247:5] assert(o1.color == 0xff0000)
;   [247:12] allocate scratch register -> r15
;   [247:12] ? o1.color == 0xff0000
;   [247:12] ? o1.color == 0xff0000
    cmp.247.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.247.12.end:
;   [32:6] assert(ok bool)
    func.assert.247.5:
;       [247:5] alias ok -> r15b
        if.32.27.247.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.247.5:
        cmp r15b, 0
        jne if.32.24.247.5.end
        if.32.27.247.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.247.5.end:
;       [247:5] free scratch register r15
    func.assert.247.5.end:
;   [249:5] var p1 point = {-x, -y}
;   [249:9] p1: point (16 B @ [rbp + 440])
;   [249:9] p1 = {-x, -y}
;   [249:21] copy field 'x'
;   [249:21] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 440], r15
;   [249:21] free scratch register r15
    neg qword [rbp + 440]
;   [249:25] copy field 'y'
;   [249:25] allocate scratch register -> r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 448], r15
;   [249:25] free scratch register r15
    neg qword [rbp + 448]
;   [250:5] o1.pos = p1
;   [250:14] size <= 16 B, use mov
;   [250:14] allocate named register rax
    mov rax, qword [rbp + 440]
    mov qword [rbp + 416], rax
    mov rax, qword [rbp + 448]
    mov qword [rbp + 424], rax
;   [250:14] free named register rax
;   [251:5] assert(o1.pos.x == -1)
;   [251:12] allocate scratch register -> r15
;   [251:12] ? o1.pos.x == -1
;   [251:12] ? o1.pos.x == -1
    cmp.251.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.251.12.end:
;   [32:6] assert(ok bool)
    func.assert.251.5:
;       [251:5] alias ok -> r15b
        if.32.27.251.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [252:5] assert(o1.pos.y == -2)
;   [252:12] allocate scratch register -> r15
;   [252:12] ? o1.pos.y == -2
;   [252:12] ? o1.pos.y == -2
    cmp.252.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.252.12.end:
;   [32:6] assert(ok bool)
    func.assert.252.5:
;       [252:5] alias ok -> r15b
        if.32.27.252.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [254:5] var o2 object = o1
;   [254:9] o2: object (24 B @ [rbp + 456])
;   [254:9] o2 = o1
;   [254:21] allocate named register rsi
;   [254:21] allocate named register rdi
;   [254:21] allocate named register rcx
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
;   [254:21] free named register rcx
;   [254:21] free named register rdi
;   [254:21] free named register rsi
;   [255:5] assert(o2.pos.x == -1)
;   [255:12] allocate scratch register -> r15
;   [255:12] ? o2.pos.x == -1
;   [255:12] ? o2.pos.x == -1
    cmp.255.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.255.12.end:
;   [32:6] assert(ok bool)
    func.assert.255.5:
;       [255:5] alias ok -> r15b
        if.32.27.255.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [256:5] assert(o2.pos.y == -2)
;   [256:12] allocate scratch register -> r15
;   [256:12] ? o2.pos.y == -2
;   [256:12] ? o2.pos.y == -2
    cmp.256.12:
    cmp qword [rbp + 464], -2
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
;   [257:5] assert(o2.color == 0xff0000)
;   [257:12] allocate scratch register -> r15
;   [257:12] ? o2.color == 0xff0000
;   [257:12] ? o2.color == 0xff0000
    cmp.257.12:
    cmp dword [rbp + 472], 16711680
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
;   [259:5] var o3[2] object
;   [259:9] o3: object[2] (48 B @ [rbp + 480])
;   [259:9] zero 2 * 24 B = 48 B
;   [259:5] allocate named register rax
;   [259:5] allocate named register rdi
;   [259:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
;   [259:5] free named register rcx
;   [259:5] free named register rdi
;   [259:5] free named register rax
;   [260:5] o3.pos.y = 73
;   [260:16] 73
    mov qword [rbp + 488], 73
;   [263:5] assert(o3[0].pos.y == 73)
;   [263:12] allocate scratch register -> r15
;   [263:12] ? o3[0].pos.y == 73
;   [263:12] ? o3[0].pos.y == 73
    cmp.263.12:
;   [263:15] allocate scratch register -> r14
;   [263:15] set array index
;   [263:15] 0
    mov r14, 0
;   [263:15] bounds check
;   [263:15] allocate scratch register -> r13
;   [263:15] line number
    mov r13, 263
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [263:15] free scratch register r13
    imul r14, 24
    cmp qword [rbp + r14 + 488], 73
;   [263:12] free scratch register r14
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
;   [264:5] o3[1] = object_init()
;   [264:8] allocate scratch register -> r15
;   [264:8] set array index
;   [264:8] 1
    mov r15, 1
;   [264:8] bounds check
;   [264:8] allocate scratch register -> r14
;   [264:8] line number
    mov r14, 264
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
;   [264:8] free scratch register r14
    imul r15, 24
;   [264:13] object_init()
;   [92:6] object_init() res object
    func.object_init.264.13:
;       [264:13] alias res -> o3 (lea: rbp + r15 + 480)
;       [93:5] res.pos.x = 2
;       [93:17] 2
        mov qword [rbp + r15 + 480], 2
;       [94:5] res.pos.y = 74
;       [94:17] 74
        mov qword [rbp + r15 + 488], 74
;       [95:5] res.color = 0xffffff
;       [95:17] 0xffffff
        mov dword [rbp + r15 + 496], 16777215
    func.object_init.264.13.end:
;   [264:5] free scratch register r15
;   [265:5] assert(o3[1].pos.y == 74)
;   [265:12] allocate scratch register -> r15
;   [265:12] ? o3[1].pos.y == 74
;   [265:12] ? o3[1].pos.y == 74
    cmp.265.12:
;   [265:15] allocate scratch register -> r14
;   [265:15] set array index
;   [265:15] 1
    mov r14, 1
;   [265:15] bounds check
;   [265:15] allocate scratch register -> r13
;   [265:15] line number
    mov r13, 265
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [265:15] free scratch register r13
    imul r14, 24
    cmp qword [rbp + r14 + 488], 74
;   [265:12] free scratch register r14
    sete r15b
    bool.265.12.end:
;   [32:6] assert(ok bool)
    func.assert.265.5:
;       [265:5] alias ok -> r15b
        if.32.27.265.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [267:5] var worlds[8] world
;   [267:9] worlds: world[8] (512 B @ [rbp + 528])
;   [267:9] zero 8 * 64 B = 512 B
;   [267:5] allocate named register rax
;   [267:5] allocate named register rdi
;   [267:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
;   [267:5] free named register rcx
;   [267:5] free named register rdi
;   [267:5] free named register rax
;   [268:5] worlds[1].locations[1] = 0xffee
;   [268:12] allocate scratch register -> r15
;   [268:12] set array index
;   [268:12] 1
    mov r15, 1
;   [268:12] bounds check
;   [268:12] allocate scratch register -> r14
;   [268:12] line number
    mov r14, 268
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [268:12] free scratch register r14
    shl r15, 6
    lea r15, [rbp + r15 + 528]
;   [268:25] allocate scratch register -> r14
;   [268:25] set array index
;   [268:25] 1
    mov r14, 1
;   [268:25] bounds check
;   [268:25] allocate scratch register -> r13
;   [268:25] line number
    mov r13, 268
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [268:25] free scratch register r13
;   [268:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [268:5] free scratch register r14
;   [268:5] free scratch register r15
;   [269:5] assert(worlds[1].locations[1] == 0xffee)
;   [269:12] allocate scratch register -> r15
;   [269:12] ? worlds[1].locations[1] == 0xffee
;   [269:12] ? worlds[1].locations[1] == 0xffee
    cmp.269.12:
;   [269:19] allocate scratch register -> r14
;   [269:19] set array index
;   [269:19] 1
    mov r14, 1
;   [269:19] bounds check
;   [269:19] allocate scratch register -> r13
;   [269:19] line number
    mov r13, 269
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [269:19] free scratch register r13
    shl r14, 6
    lea r14, [rbp + r14 + 528]
;   [269:32] allocate scratch register -> r13
;   [269:32] set array index
;   [269:32] 1
    mov r13, 1
;   [269:32] bounds check
;   [269:32] allocate scratch register -> r12
;   [269:32] line number
    mov r12, 269
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [269:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [269:12] free scratch register r13
;   [269:12] free scratch register r14
    sete r15b
    bool.269.12.end:
;   [32:6] assert(ok bool)
    func.assert.269.5:
;       [269:5] alias ok -> r15b
        if.32.27.269.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [271:5] array_copy( worlds[1].locations, worlds[0].locations, array_length(worlds.locations) )
;   [271:5] allocate named register rsi
;   [271:5] allocate named register rdi
;   [271:5] allocate named register rcx
;   [274:9] array_length(worlds.locations)
;   [274:9] rcx = array_length(worlds.locations)
;   [274:9] = expression
;   [274:9] array_length(worlds.locations)
    mov rcx, 8
;   [272:9] worlds[1].locations
;   [272:16] allocate scratch register -> r15
;   [272:16] set array index
;   [272:16] 1
    mov r15, 1
;   [272:16] bounds check
;   [272:16] allocate scratch register -> r14
;   [272:16] line number
    mov r14, 272
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [272:16] free scratch register r14
    shl r15, 6
;   [272:9] bounds check
;   [272:9] allocate scratch register -> r14
;   [272:9] line number
    mov r14, 272
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [272:9] free scratch register r14
    lea rsi, [rbp + r15 + 528]
;   [271:5] free scratch register r15
;   [273:9] worlds[0].locations
;   [273:16] allocate scratch register -> r15
;   [273:16] set array index
;   [273:16] 0
    mov r15, 0
;   [273:16] bounds check
;   [273:16] allocate scratch register -> r14
;   [273:16] line number
    mov r14, 273
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [273:16] free scratch register r14
    shl r15, 6
;   [273:9] bounds check
;   [273:9] allocate scratch register -> r14
;   [273:9] line number
    mov r14, 273
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [273:9] free scratch register r14
    lea rdi, [rbp + r15 + 528]
;   [271:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [271:5] free named register rcx
;   [271:5] free named register rdi
;   [271:5] free named register rsi
;   [278:5] assert(worlds[0].locations[1] == 0xffee)
;   [278:12] allocate scratch register -> r15
;   [278:12] ? worlds[0].locations[1] == 0xffee
;   [278:12] ? worlds[0].locations[1] == 0xffee
    cmp.278.12:
;   [278:19] allocate scratch register -> r14
;   [278:19] set array index
;   [278:19] 0
    mov r14, 0
;   [278:19] bounds check
;   [278:19] allocate scratch register -> r13
;   [278:19] line number
    mov r13, 278
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [278:19] free scratch register r13
    shl r14, 6
    lea r14, [rbp + r14 + 528]
;   [278:32] allocate scratch register -> r13
;   [278:32] set array index
;   [278:32] 1
    mov r13, 1
;   [278:32] bounds check
;   [278:32] allocate scratch register -> r12
;   [278:32] line number
    mov r12, 278
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [278:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [278:12] free scratch register r13
;   [278:12] free scratch register r14
    sete r15b
    bool.278.12.end:
;   [32:6] assert(ok bool)
    func.assert.278.5:
;       [278:5] alias ok -> r15b
        if.32.27.278.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [279:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) ))
;   [279:12] allocate scratch register -> r15
;   [279:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) )
;   [279:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) )
    cmp.279.12:
;       [279:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_length(worlds.locations) )
;       [279:12] allocate named register rsi
;       [279:12] allocate named register rdi
;       [279:12] allocate named register rcx
;       [282:14] array_length(worlds.locations)
;       [282:14] rcx = array_length(worlds.locations)
;       [282:14] = expression
;       [282:14] array_length(worlds.locations)
        mov rcx, 8
;       [280:14] worlds[0].locations
;       [280:21] allocate scratch register -> r14
;       [280:21] set array index
;       [280:21] 0
        mov r14, 0
;       [280:21] bounds check
;       [280:21] allocate scratch register -> r13
;       [280:21] line number
        mov r13, 280
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
;       [280:21] free scratch register r13
        shl r14, 6
;       [280:14] bounds check
;       [280:14] allocate scratch register -> r13
;       [280:14] line number
        mov r13, 280
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [280:14] free scratch register r13
        lea rsi, [rbp + r14 + 528]
;       [279:12] free scratch register r14
;       [281:14] worlds[1].locations
;       [281:21] allocate scratch register -> r14
;       [281:21] set array index
;       [281:21] 1
        mov r14, 1
;       [281:21] bounds check
;       [281:21] allocate scratch register -> r13
;       [281:21] line number
        mov r13, 281
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
;       [281:21] free scratch register r13
        shl r14, 6
;       [281:14] bounds check
;       [281:14] allocate scratch register -> r13
;       [281:14] line number
        mov r13, 281
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [281:14] free scratch register r13
        lea rdi, [rbp + r14 + 528]
;       [279:12] free scratch register r14
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
;       [279:12] free named register rcx
;       [279:12] free named register rdi
;       [279:12] free named register rsi
        sete r15b
    bool.279.12.end:
;   [32:6] assert(ok bool)
    func.assert.279.5:
;       [279:5] alias ok -> r15b
        if.32.27.279.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
        cmp.32.27.279.5:
        cmp r15b, 0
        jne if.32.24.279.5.end
        if.32.27.279.5.code:
;           [32:34] exit(1)
;           [32:34] allocate named register rdi
;           [32:39] 1
            mov rdi, 1
            mov rax, 60
            syscall
;           [32:34] free named register rdi
        if.32.24.279.5.end:
;       [279:5] free scratch register r15
    func.assert.279.5.end:
;   [284:5] var arr2[] = { -1, 2 }
;   [284:9] arr2: i64[2] (16 B @ [rbp + 1040])
;   [284:9] arr2= { -1, 2 }
;   [284:18] size <= 16 B, use immediates
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
;   [285:5] assert(array_length(arr2) == 2)
;   [285:12] allocate scratch register -> r15
;   [285:12] ? array_length(arr2) == 2
;   [285:12] ? array_length(arr2) == 2
    cmp.285.12:
;   [285:12] allocate scratch register -> r14
;       [285:12] r14 = array_length(arr2)
;       [285:12] = expression
;       [285:12] array_length(arr2)
        mov r14, 2
    cmp r14, 2
;   [285:12] free scratch register r14
    sete r15b
    bool.285.12.end:
;   [32:6] assert(ok bool)
    func.assert.285.5:
;       [285:5] alias ok -> r15b
        if.32.27.285.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [286:5] assert(arr2[0] == -1)
;   [286:12] allocate scratch register -> r15
;   [286:12] ? arr2[0] == -1
;   [286:12] ? arr2[0] == -1
    cmp.286.12:
;   [286:17] allocate scratch register -> r14
;   [286:17] set array index
;   [286:17] 0
    mov r14, 0
;   [286:17] bounds check
;   [286:17] allocate scratch register -> r13
;   [286:17] line number
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [286:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1040], -1
;   [286:12] free scratch register r14
    sete r15b
    bool.286.12.end:
;   [32:6] assert(ok bool)
    func.assert.286.5:
;       [286:5] alias ok -> r15b
        if.32.27.286.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [287:5] assert(arr2[1] == 2)
;   [287:12] allocate scratch register -> r15
;   [287:12] ? arr2[1] == 2
;   [287:12] ? arr2[1] == 2
    cmp.287.12:
;   [287:17] allocate scratch register -> r14
;   [287:17] set array index
;   [287:17] 1
    mov r14, 1
;   [287:17] bounds check
;   [287:17] allocate scratch register -> r13
;   [287:17] line number
    mov r13, 287
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [287:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1040], 2
;   [287:12] free scratch register r14
    sete r15b
    bool.287.12.end:
;   [32:6] assert(ok bool)
    func.assert.287.5:
;       [287:5] alias ok -> r15b
        if.32.27.287.5:
;       [32:27] ? not ok
;       [32:27] ? not ok
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
;   [289:5] var counter
;   [289:9] counter: i64 (8 B @ [rbp + 1056])
;   [289:9] zero 1 * 8 B = 8 B
;   [289:5] size <= 32 B, use mov
    mov qword [rbp + 1056], 0
;   [290:5] var nm str
;   [290:9] nm: str (128 B @ [rbp + 1064])
;   [290:9] zero 1 * 128 B = 128 B
;   [290:5] allocate named register rax
;   [290:5] allocate named register rdi
;   [290:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
;   [290:5] free named register rcx
;   [290:5] free named register rdi
;   [290:5] free named register rax
;   [291:5] print(hello)
;   [35:6] print(str[] i8)
    func.print.291.5:
;       [291:5] alias str -> hello
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
    func.print.291.5.end:
;   [292:5] label
    loop.292.5:
;       [293:9] counter = counter + 1
;       [293:19] instructions without scratch register 1, with 3
;       [293:19] counter
;       [293:29] counter + 1
        add qword [rbp + 1056], 1
;       [294:9] print_num(counter)
;       [294:9] address of argument 'counter' to parameter 'num'
;       [294:9] allocate scratch register -> r15
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
;       [294:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1192]
        call func.print_num
        POP_REGS
;       [295:9] print(colon)
;       [35:6] print(str[] i8)
        func.print.295.9:
;           [295:9] alias str -> colon
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
        func.print.295.9.end:
;       [296:9] print(prompt1)
;       [35:6] print(str[] i8)
        func.print.296.9:
;           [296:9] alias str -> prompt1
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
        func.print.296.9.end:
;       [297:12] nm.input()
;       [76:6] str.input()
        func.str.input.297.12:
;           [297:12] alias self -> nm
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
        func.str.input.297.12.end:
        if.299.12:
;       [299:12] ? nm.len <= 0
;       [299:12] ? nm.len <= 0
        cmp.299.12:
        cmp byte [rbp + 1064], 0
        jg if.301.19
        if.299.12.code:
;           [300:13] break
            jmp loop.292.5.end
        if.301.19:
;       [301:19] ? nm.len <= 4
;       [301:19] ? nm.len <= 4
        cmp.301.19:
        cmp byte [rbp + 1064], 4
        jg if.299.9.else
        if.301.19.code:
;           [302:13] print(prompt2)
;           [35:6] print(str[] i8)
            func.print.302.13:
;               [302:13] alias str -> prompt2
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
            func.print.302.13.end:
;           [303:13] continue
            jmp loop.292.5
        if.299.9.else:
;           [305:13] print(prompt3)
;           [35:6] print(str[] i8)
            func.print.305.13:
;               [305:13] alias str -> prompt3
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
            func.print.305.13.end:
;           [306:16] nm.output()
;           [83:6] str.output()
            func.str.output.306.16:
;               [306:16] alias self -> nm
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
            func.str.output.306.16.end:
;           [307:13] print(dot)
;           [35:6] print(str[] i8)
            func.print.307.13:
;               [307:13] alias str -> dot
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
            func.print.307.13.end:
;           [308:13] print(nl)
;           [35:6] print(str[] i8)
            func.print.308.13:
;               [308:13] alias str -> nl
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
            func.print.308.13.end:
        if.299.9.end:
    jmp loop.292.5
    loop.292.5.end:
    mov rdi, 0
    mov rax, 60
    syscall

;
;[109:15] noinline print_num(num)
func.print_num:
;   [109:25] num: i64 (8 B @ [rbx])
;   [110:5] var buf[20] i8
;   [110:9] buf: i8[20] (20 B @ [rbx + 8])
;   [110:9] zero 20 * 1 B = 20 B
;   [110:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [111:5] var n = num
;   [111:9] n: i64 (8 B @ [rbx + 32])
;   [111:9] n = num
;   [111:13] num
;   [111:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [111:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
;   [111:13] free scratch register r14
;   [111:13] free scratch register r15
;   [112:5] var is_negative bool = false
;   [112:9] is_negative: bool (1 B @ [rbx + 40])
;   [112:9] is_negative = false
    mov byte [rbx + 40], 0
    if.114.8:
;   [114:8] ? n < 0
;   [114:8] ? n < 0
    cmp.114.8:
    cmp qword [rbx + 32], 0
    jge if.114.5.end
    if.114.8.code:
;       [115:9] is_negative = true
        mov byte [rbx + 40], 1
;       [116:9] n = -n
;       [116:14] -n
        neg qword [rbx + 32]
    if.114.5.end:
;   [119:5] var i = 20
;   [119:9] i: i64 (8 B @ [rbx + 48])
;   [119:9] i = 20
;   [119:13] 20
    mov qword [rbx + 48], 20
;   [120:5] label
    loop.120.5:
;       [121:9] i = i - 1
;       [121:13] instructions without scratch register 1, with 3
;       [121:13] i
;       [121:17] i - 1
        sub qword [rbx + 48], 1
;       [122:9] var ascii = 48 + (n % 10)
;       [122:13] ascii: i64 (8 B @ [rbx + 56])
;       [122:13] ascii = 48 + (n % 10)
;       [122:21] instructions without scratch register 8, with 9
;       [122:21] 48
        mov qword [rbx + 56], 48
;       [122:27] ascii + (n % 10)
;       [122:27] allocate scratch register -> r15
;       [122:27] n
        mov r15, qword [rbx + 32]
;       [122:31] r15 % 10
;       [122:31] div const
;       [122:31] allocate named register rax
        mov rax, r15
;       [122:31] allocate named register rdx
        cqo
;       [122:31] allocate scratch register -> r14
        mov r14, 10
        idiv r14
;       [122:31] free scratch register r14
        mov r15, rdx
;       [122:31] free named register rdx
;       [122:31] free named register rax
        add qword [rbx + 56], r15
;       [122:27] free scratch register r15
;       [125:9] buf[i] = i8(ascii)
;       [125:13] allocate scratch register -> r15
;       [125:13] set array index
;       [125:13] i
        mov r15, qword [rbx + 48]
;       [125:13] bounds check
;       [125:13] allocate scratch register -> r14
;       [125:13] line number
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [125:13] free scratch register r14
;       [125:18] buf = i8(ascii)
;       [125:18] = expression
;       [125:21] ascii
;       [125:21] allocate scratch register -> r14
        mov r14b, byte [rbx + 56]
        mov byte [rbx + r15 + 8], r14b
;       [125:21] free scratch register r14
;       [125:9] free scratch register r15
;       [126:9] n = n / 10
;       [126:13] instructions without scratch register 5, with 7
;       [126:13] n
;       [126:17] n / 10
;       [126:17] div const
;       [126:17] allocate named register rax
        mov rax, qword [rbx + 32]
;       [126:17] allocate named register rdx
        cqo
;       [126:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [126:17] free scratch register r15
        mov qword [rbx + 32], rax
;       [126:17] free named register rdx
;       [126:17] free named register rax
        if.127.12:
;       [127:12] ? n == 0
;       [127:12] ? n == 0
        cmp.127.12:
        cmp qword [rbx + 32], 0
        jne if.127.9.end
        if.127.12.code:
;           [127:19] break
            jmp loop.120.5.end
        if.127.9.end:
    jmp loop.120.5
    loop.120.5.end:
    if.130.8:
;   [130:8] ? is_negative
;   [130:8] ? is_negative
    cmp.130.8:
    cmp byte [rbx + 40], 0
    je if.130.5.end
    if.130.8.code:
;       [131:9] i = i - 1
;       [131:13] instructions without scratch register 1, with 3
;       [131:13] i
;       [131:17] i - 1
        sub qword [rbx + 48], 1
;       [132:9] buf[i] = 45
;       [132:13] allocate scratch register -> r15
;       [132:13] set array index
;       [132:13] i
        mov r15, qword [rbx + 48]
;       [132:13] bounds check
;       [132:13] allocate scratch register -> r14
;       [132:13] line number
        mov r14, 132
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [132:13] free scratch register r14
;       [132:18] 45
        mov byte [rbx + r15 + 8], 45
;       [132:9] free scratch register r15
    if.130.5.end:
;   [135:5] var write_pos = 0
;   [135:9] write_pos: i64 (8 B @ [rbx + 56])
;   [135:9] write_pos = 0
;   [135:21] 0
    mov qword [rbx + 56], 0
;   [136:5] label
    loop.136.5:
;       [137:9] buf[write_pos] = buf[i]
;       [137:13] allocate scratch register -> r15
;       [137:13] set array index
;       [137:13] write_pos
        mov r15, qword [rbx + 56]
;       [137:13] bounds check
;       [137:13] allocate scratch register -> r14
;       [137:13] line number
        mov r14, 137
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [137:13] free scratch register r14
;       [137:26] buf[i]
;       [137:30] allocate scratch register -> r14
;       [137:30] set array index
;       [137:30] i
        mov r14, qword [rbx + 48]
;       [137:30] bounds check
;       [137:30] allocate scratch register -> r13
;       [137:30] line number
        mov r13, 137
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [137:30] free scratch register r13
;       [137:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [137:26] free scratch register r13
;       [137:26] free scratch register r14
;       [137:9] free scratch register r15
;       [138:9] write_pos = write_pos + 1
;       [138:21] instructions without scratch register 1, with 3
;       [138:21] write_pos
;       [138:33] write_pos + 1
        add qword [rbx + 56], 1
;       [139:9] i = i + 1
;       [139:13] instructions without scratch register 1, with 3
;       [139:13] i
;       [139:17] i + 1
        add qword [rbx + 48], 1
        if.140.12:
;       [140:12] ? i == 20
;       [140:12] ? i == 20
        cmp.140.12:
        cmp qword [rbx + 48], 20
        jne if.140.9.end
        if.140.12.code:
;           [140:20] break
            jmp loop.136.5.end
        if.140.9.end:
    jmp loop.136.5
    loop.136.5.end:
;   [143:5] write(1, buf, write_pos)
;   [143:5] allocate named register rdi
;   [143:5] allocate named register rsi
;   [143:5] allocate named register rdx
;   [143:11] 1
    mov rdi, 1
;   [143:19] write_pos
    mov rdx, qword [rbx + 56]
;   [143:14] bounds check
;   [143:14] allocate scratch register -> r15
;   [143:14] line number
    mov r15, 143
    test rdx, rdx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rdx, 20
    cmovg rbp, r15
    jg baz_bounds_panic
;   [143:14] free scratch register r15
    lea rsi, [rbx + 8]
;   [143:5] allocate named register rax
    mov rax, 1
    syscall
;   [143:5] free named register rax
;   [143:5] free named register rdx
;   [143:5] free named register rsi
;   [143:5] free named register rdi
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

;   removed jumps to next code: 87
;    removed unreachable jumps: 2
; removed same target branches: 33
; inverted branches over jumps: 0

; max scratch registers in use: 4
;            max frames in use: 8
;              dat var padding: 0 B
;                max vars size: 976 B
```
