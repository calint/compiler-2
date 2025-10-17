# compiler-2: baz

Experimental compiler for a minimalistic, specialized language that targets NASM
x86_64 assembly on Linux.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by NASM for x86_64
* super loop program with non-reentrant inlined functions

## Supports

* built-in integer types (64, 32, 16, 8 bit)
* built-in boolean type
* user defined types
* arrays
* optional bounds checking at runtime
  * optional line number
* inlined functions
* keywords: `func`, `type`, `field`, `var`, `loop`, `if`, `else`, `continue`,
`break`, `return`
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`, `address_of`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
run `prog.baz`
  * optional parameters: _stack size_, _bounds check_, with _line number information_ e.g:
    * `./run-baz.sh myprogram.baz --stack=262144`: stack size, no runtime checks
    * `./run-baz.sh myprogram.baz --stack=262144 --check=bounds`: checks bounds without
    line number information (faster)
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=bounds,line`: checks
    bounds with line number information
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    40           1216            679           5814
C++                              1             75             83            520
-------------------------------------------------------------------------------
SUM:                            41           1291            762           6334
-------------------------------------------------------------------------------
```

## Sample

```text
field   hello = "hello world from baz\n"
field prompt1 = "enter name:\n"
field prompt2 = "that is not a name.\n"
field prompt3 = "hello "
field     dot = "."
field      nl = "\n"

# all functions are inlined

# arguments can be placed in specified register using `reg_...` syntax

func exit(v : reg_rdi) {
    mov(rax, 60)  # exit system call
    mov(rdi, v)   # return code
    syscall()
}

func assert(expr : bool) {
    if not expr exit(1)
}

func print(len : reg_rdx, ptr : reg_rsi) {
    mov(rax, 1)   # write system call
    mov(rdi, 1)   # file descriptor for standard out
    mov(rsi, ptr) # buffer address 
    mov(rdx, len) # buffer size
    syscall()
}

func read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes {
    mov(rax, 0)   # read system call
    mov(rdi, 0)   # file descriptor for standard input
    mov(rsi, ptr) # buffer address
    mov(rdx, len) # buffer size
    syscall()
    mov(nbytes, rax) # return value
}

# user types are defined using keyword `type`

# default type is `i64` and does not need to be specified

type point {x, y}

type object {pos : point, color : i32}

type world { locations : i64[8] }

# function arguments are equivalent to mutable references

func foo(pt : point) {
    pt.x = 0b10    # binary value 2
    pt.y = 0xb     # hex value 11
}

# default argument type is `i64`

func bar(arg) {
    if arg == 0 return
    arg = 0xff
}

# return target is specified as a variable, in this case `res`

func inv(i : i32) : i32 res {
    res = ~i
}

func baz(arg) : i64 res {
    res = arg * 2
}

# array arguments are declared with type and []

func faz(arg : i32[]) {
    arg[1] = 0xfe
}

func main() {
    var arr : i32[4]
    # arrays are initialized to 0

    var ix = 1

    arr[ix] = 2
    arr[ix + 1] = arr[ix]
    assert(arr[1] == 2)
    assert(arr[2] == 2)

    array_copy(arr[2], arr, 2)
    # copy from, to, number of elements
    assert(arr[0] == 2)

    var arr1 : i32[8]
    array_copy(arr, arr1, 4)
    assert(arrays_equal(arr, arr1, 4))

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

    ix = 3
    arr[ix] = ~inv(arr[ix - 1])
    assert(arr[ix] == 2)

    faz(arr)
    assert(arr[1] == 0xfe)

    var p : point = {0, 0}
    foo(p)
    assert(p.x == 2)
    assert(p.y == 0xb)

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

    var p0 : point = {baz(2), 0}
    assert(p0.x == 4)

    var x = 1
    var y = 2

    var o1 : object = {{x * 10, y}, 0xff0000}
    assert(o1.pos.x == 10)
    assert(o1.pos.y == 2)
    assert(o1.color == 0xff0000)
    
    var p1 : point = {-x, -y}
    o1.pos = p1
    assert(o1.pos.x == -1)
    assert(o1.pos.y == -2)

    var o2 : object = o1
    assert(o2.pos.x == -1)
    assert(o2.pos.y == -2)
    assert(o2.color == 0xff0000)

    var o3 : object[1]
    # index 0 in an array can be accessed without array index
    o3.pos.y = 73
    assert(o3[0].pos.y == 73)

    var input_buffer : i8[80]

    var worlds : world[8]
    worlds[1].locations[1] = 0xffee
    assert(worlds[1].locations[1] == 0xffee)

    array_copy(
        worlds[1].locations,
        worlds[0].locations,
        array_size_of(worlds[1].locations)
    )
    assert(worlds[0].locations[1] == 0xffee)

    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        
        var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
        # note: `array_size_of` and `address_of` are built-in functions
        #       -1 to not include the trailing '\n'

        if len == 0 {
            break
        } else if len <= 4 {
            print(prompt2.len, prompt2)
            continue
        } else {
            print(prompt3.len, prompt3)
            print(len, address_of(input_buffer))
            print(dot.len, dot)
            print(nl.len, nl)
        }
    }
}
```

## Generates

```nasm
DEFAULT REL
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $ - hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $ - prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $ - prompt2
prompt3: db 'hello '
prompt3.len equ $ - prompt3
dot: db '.'
dot.len equ $ - dot
nl: db '', 10,''
nl.len equ $ - nl
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    cmp r15, 4
    mov r14, 85
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    cmp r15, 4
    mov r14, 86
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    cmp r14, 4
    mov r13, 86
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_87_12:
        mov r13, 1
        cmp r13, 4
        mov r12, 87
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_87_12
    jmp bool_true_87_12
    bool_true_87_12:
    mov r15, true
    jmp bool_end_87_12
    bool_false_87_12:
    mov r15, false
    bool_end_87_12:
    assert_87_5:
        if_19_8_87_5:
        cmp_19_8_87_5:
        test r15, r15
        jne if_19_5_87_5_end
        jmp if_19_8_87_5_code
        if_19_8_87_5_code:
            mov rdi, 1
            exit_19_17_87_5:
                    mov rax, 60
                syscall
            exit_19_17_87_5_end:
        if_19_5_87_5_end:
    assert_87_5_end:
    cmp_88_12:
        mov r13, 2
        cmp r13, 4
        mov r12, 88
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_88_12
    jmp bool_true_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
    assert_88_5:
        if_19_8_88_5:
        cmp_19_8_88_5:
        test r15, r15
        jne if_19_5_88_5_end
        jmp if_19_8_88_5_code
        if_19_8_88_5_code:
            mov rdi, 1
            exit_19_17_88_5:
                    mov rax, 60
                syscall
            exit_19_17_88_5_end:
        if_19_5_88_5_end:
    assert_88_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, rcx
    add r14, r15
    cmp r14, 4
    mov r13, 90
    cmovg rbp, r13
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    cmp rcx, 4
    mov r15, 90
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_92_12:
        mov r13, 0
        cmp r13, 4
        mov r12, 92
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_92_12
    jmp bool_true_92_12
    bool_true_92_12:
    mov r15, true
    jmp bool_end_92_12
    bool_false_92_12:
    mov r15, false
    bool_end_92_12:
    assert_92_5:
        if_19_8_92_5:
        cmp_19_8_92_5:
        test r15, r15
        jne if_19_5_92_5_end
        jmp if_19_8_92_5_code
        if_19_8_92_5_code:
            mov rdi, 1
            exit_19_17_92_5:
                    mov rax, 60
                syscall
            exit_19_17_92_5_end:
        if_19_5_92_5_end:
    assert_92_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    cmp rcx, 4
    mov r15, 95
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    cmp rcx, 8
    mov r15, 95
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_96_12:
        mov rcx, 4
        cmp rcx, 4
        mov r13, 96
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        mov r13, 96
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_96_12
        mov r14, false
        jmp cmps_end_96_12
        cmps_eq_96_12:
        mov r14, true
        cmps_end_96_12:
    test r14, r14
    je bool_false_96_12
    jmp bool_true_96_12
    bool_true_96_12:
    mov r15, true
    jmp bool_end_96_12
    bool_false_96_12:
    mov r15, false
    bool_end_96_12:
    assert_96_5:
        if_19_8_96_5:
        cmp_19_8_96_5:
        test r15, r15
        jne if_19_5_96_5_end
        jmp if_19_8_96_5_code
        if_19_8_96_5_code:
            mov rdi, 1
            exit_19_17_96_5:
                    mov rax, 60
                syscall
            exit_19_17_96_5_end:
        if_19_5_96_5_end:
    assert_96_5_end:
    mov r15, 2
    cmp r15, 8
    mov r14, 98
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_99_12:
        mov rcx, 4
        cmp rcx, 4
        mov r13, 99
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        mov r13, 99
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_99_16
        mov r14, false
        jmp cmps_end_99_16
        cmps_eq_99_16:
        mov r14, true
        cmps_end_99_16:
    test r14, r14
    jne bool_false_99_12
    jmp bool_true_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
    assert_99_5:
        if_19_8_99_5:
        cmp_19_8_99_5:
        test r15, r15
        jne if_19_5_99_5_end
        jmp if_19_8_99_5_code
        if_19_8_99_5_code:
            mov rdi, 1
            exit_19_17_99_5:
                    mov rax, 60
                syscall
            exit_19_17_99_5_end:
        if_19_5_99_5_end:
    assert_99_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    cmp r15, 4
    mov r14, 102
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    cmp r14, 4
    mov r13, 102
    cmovge rbp, r13
    jge panic_bounds
    inv_102_16:
        lea r13, [rsp + r14 * 4 - 16]
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
        not dword [rsp + r15 * 4 - 16]
    inv_102_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_103_12:
        mov r13, qword [rsp - 24]
        cmp r13, 4
        mov r12, 103
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_103_12
    jmp bool_true_103_12
    bool_true_103_12:
    mov r15, true
    jmp bool_end_103_12
    bool_false_103_12:
    mov r15, false
    bool_end_103_12:
    assert_103_5:
        if_19_8_103_5:
        cmp_19_8_103_5:
        test r15, r15
        jne if_19_5_103_5_end
        jmp if_19_8_103_5_code
        if_19_8_103_5_code:
            mov rdi, 1
            exit_19_17_103_5:
                    mov rax, 60
                syscall
            exit_19_17_103_5_end:
        if_19_5_103_5_end:
    assert_103_5_end:
    faz_105_5:
        mov r15, 1
        cmp r15, 4
        mov r14, 76
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_105_5_end:
    cmp_106_12:
        mov r13, 1
        cmp r13, 4
        mov r12, 106
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 0xfe
    jne bool_false_106_12
    jmp bool_true_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
    assert_106_5:
        if_19_8_106_5:
        cmp_19_8_106_5:
        test r15, r15
        jne if_19_5_106_5_end
        jmp if_19_8_106_5_code
        if_19_8_106_5_code:
            mov rdi, 1
            exit_19_17_106_5:
                    mov rax, 60
                syscall
            exit_19_17_106_5_end:
        if_19_5_106_5_end:
    assert_106_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_109_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_109_5_end:
    cmp_110_12:
    cmp qword [rsp - 72], 2
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_19_8_110_5:
        cmp_19_8_110_5:
        test r15, r15
        jne if_19_5_110_5_end
        jmp if_19_8_110_5_code
        if_19_8_110_5_code:
            mov rdi, 1
            exit_19_17_110_5:
                    mov rax, 60
                syscall
            exit_19_17_110_5_end:
        if_19_5_110_5_end:
    assert_110_5_end:
    cmp_111_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_111_12
    jmp bool_true_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_19_8_111_5:
        cmp_19_8_111_5:
        test r15, r15
        jne if_19_5_111_5_end
        jmp if_19_8_111_5_code
        if_19_8_111_5_code:
            mov rdi, 1
            exit_19_17_111_5:
                    mov rax, 60
                syscall
            exit_19_17_111_5_end:
        if_19_5_111_5_end:
    assert_111_5_end:
    mov qword [rsp - 80], 0
    bar_114_5:
        if_59_8_114_5:
        cmp_59_8_114_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_114_5_end
        jmp if_59_8_114_5_code
        if_59_8_114_5_code:
            jmp bar_114_5_end
        if_59_5_114_5_end:
        mov qword [rsp - 80], 0xff
    bar_114_5_end:
    cmp_115_12:
    cmp qword [rsp - 80], 0
    jne bool_false_115_12
    jmp bool_true_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_19_8_115_5:
        cmp_19_8_115_5:
        test r15, r15
        jne if_19_5_115_5_end
        jmp if_19_8_115_5_code
        if_19_8_115_5_code:
            mov rdi, 1
            exit_19_17_115_5:
                    mov rax, 60
                syscall
            exit_19_17_115_5_end:
        if_19_5_115_5_end:
    assert_115_5_end:
    mov qword [rsp - 80], 1
    bar_118_5:
        if_59_8_118_5:
        cmp_59_8_118_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_118_5_end
        jmp if_59_8_118_5_code
        if_59_8_118_5_code:
            jmp bar_118_5_end
        if_59_5_118_5_end:
        mov qword [rsp - 80], 0xff
    bar_118_5_end:
    cmp_119_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_119_12
    jmp bool_true_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
    assert_119_5:
        if_19_8_119_5:
        cmp_19_8_119_5:
        test r15, r15
        jne if_19_5_119_5_end
        jmp if_19_8_119_5_code
        if_19_8_119_5_code:
            mov rdi, 1
            exit_19_17_119_5:
                    mov rax, 60
                syscall
            exit_19_17_119_5_end:
        if_19_5_119_5_end:
    assert_119_5_end:
    mov qword [rsp - 88], 1
    baz_122_13:
        mov r15, qword [rsp - 88]
        imul r15, 2
    baz_122_13_end:
    mov qword [rsp - 96], r15
    cmp_123_12:
    cmp qword [rsp - 96], 2
    jne bool_false_123_12
    jmp bool_true_123_12
    bool_true_123_12:
    mov r15, true
    jmp bool_end_123_12
    bool_false_123_12:
    mov r15, false
    bool_end_123_12:
    assert_123_5:
        if_19_8_123_5:
        cmp_19_8_123_5:
        test r15, r15
        jne if_19_5_123_5_end
        jmp if_19_8_123_5_code
        if_19_8_123_5_code:
            mov rdi, 1
            exit_19_17_123_5:
                    mov rax, 60
                syscall
            exit_19_17_123_5_end:
        if_19_5_123_5_end:
    assert_123_5_end:
    baz_125_9:
        mov r15, 1
        imul r15, 2
    baz_125_9_end:
    mov qword [rsp - 96], r15
    cmp_126_12:
    cmp qword [rsp - 96], 2
    jne bool_false_126_12
    jmp bool_true_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_19_8_126_5:
        cmp_19_8_126_5:
        test r15, r15
        jne if_19_5_126_5_end
        jmp if_19_8_126_5_code
        if_19_8_126_5_code:
            mov rdi, 1
            exit_19_17_126_5:
                    mov rax, 60
                syscall
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
    assert_126_5_end:
    baz_128_23:
        mov r15, 2
        imul r15, 2
    baz_128_23_end:
    mov qword [rsp - 112], r15
    mov qword [rsp - 104], 0
    cmp_129_12:
    cmp qword [rsp - 112], 4
    jne bool_false_129_12
    jmp bool_true_129_12
    bool_true_129_12:
    mov r15, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15, false
    bool_end_129_12:
    assert_129_5:
        if_19_8_129_5:
        cmp_19_8_129_5:
        test r15, r15
        jne if_19_5_129_5_end
        jmp if_19_8_129_5_code
        if_19_8_129_5_code:
            mov rdi, 1
            exit_19_17_129_5:
                    mov rax, 60
                syscall
            exit_19_17_129_5_end:
        if_19_5_129_5_end:
    assert_129_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_135_12:
    cmp qword [rsp - 148], 10
    jne bool_false_135_12
    jmp bool_true_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
    assert_135_5:
        if_19_8_135_5:
        cmp_19_8_135_5:
        test r15, r15
        jne if_19_5_135_5_end
        jmp if_19_8_135_5_code
        if_19_8_135_5_code:
            mov rdi, 1
            exit_19_17_135_5:
                    mov rax, 60
                syscall
            exit_19_17_135_5_end:
        if_19_5_135_5_end:
    assert_135_5_end:
    cmp_136_12:
    cmp qword [rsp - 140], 2
    jne bool_false_136_12
    jmp bool_true_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_19_8_136_5:
        cmp_19_8_136_5:
        test r15, r15
        jne if_19_5_136_5_end
        jmp if_19_8_136_5_code
        if_19_8_136_5_code:
            mov rdi, 1
            exit_19_17_136_5:
                    mov rax, 60
                syscall
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
    assert_136_5_end:
    cmp_137_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_137_12
    jmp bool_true_137_12
    bool_true_137_12:
    mov r15, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15, false
    bool_end_137_12:
    assert_137_5:
        if_19_8_137_5:
        cmp_19_8_137_5:
        test r15, r15
        jne if_19_5_137_5_end
        jmp if_19_8_137_5_code
        if_19_8_137_5_code:
            mov rdi, 1
            exit_19_17_137_5:
                    mov rax, 60
                syscall
            exit_19_17_137_5_end:
        if_19_5_137_5_end:
    assert_137_5_end:
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
    neg qword [rsp - 164]
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
    neg qword [rsp - 156]
    lea rdi, [rsp - 148]
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
    cmp_141_12:
    cmp qword [rsp - 148], -1
    jne bool_false_141_12
    jmp bool_true_141_12
    bool_true_141_12:
    mov r15, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15, false
    bool_end_141_12:
    assert_141_5:
        if_19_8_141_5:
        cmp_19_8_141_5:
        test r15, r15
        jne if_19_5_141_5_end
        jmp if_19_8_141_5_code
        if_19_8_141_5_code:
            mov rdi, 1
            exit_19_17_141_5:
                    mov rax, 60
                syscall
            exit_19_17_141_5_end:
        if_19_5_141_5_end:
    assert_141_5_end:
    cmp_142_12:
    cmp qword [rsp - 140], -2
    jne bool_false_142_12
    jmp bool_true_142_12
    bool_true_142_12:
    mov r15, true
    jmp bool_end_142_12
    bool_false_142_12:
    mov r15, false
    bool_end_142_12:
    assert_142_5:
        if_19_8_142_5:
        cmp_19_8_142_5:
        test r15, r15
        jne if_19_5_142_5_end
        jmp if_19_8_142_5_code
        if_19_8_142_5_code:
            mov rdi, 1
            exit_19_17_142_5:
                    mov rax, 60
                syscall
            exit_19_17_142_5_end:
        if_19_5_142_5_end:
    assert_142_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_145_12:
    cmp qword [rsp - 184], -1
    jne bool_false_145_12
    jmp bool_true_145_12
    bool_true_145_12:
    mov r15, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15, false
    bool_end_145_12:
    assert_145_5:
        if_19_8_145_5:
        cmp_19_8_145_5:
        test r15, r15
        jne if_19_5_145_5_end
        jmp if_19_8_145_5_code
        if_19_8_145_5_code:
            mov rdi, 1
            exit_19_17_145_5:
                    mov rax, 60
                syscall
            exit_19_17_145_5_end:
        if_19_5_145_5_end:
    assert_145_5_end:
    cmp_146_12:
    cmp qword [rsp - 176], -2
    jne bool_false_146_12
    jmp bool_true_146_12
    bool_true_146_12:
    mov r15, true
    jmp bool_end_146_12
    bool_false_146_12:
    mov r15, false
    bool_end_146_12:
    assert_146_5:
        if_19_8_146_5:
        cmp_19_8_146_5:
        test r15, r15
        jne if_19_5_146_5_end
        jmp if_19_8_146_5_code
        if_19_8_146_5_code:
            mov rdi, 1
            exit_19_17_146_5:
                    mov rax, 60
                syscall
            exit_19_17_146_5_end:
        if_19_5_146_5_end:
    assert_146_5_end:
    cmp_147_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_147_12
    jmp bool_true_147_12
    bool_true_147_12:
    mov r15, true
    jmp bool_end_147_12
    bool_false_147_12:
    mov r15, false
    bool_end_147_12:
    assert_147_5:
        if_19_8_147_5:
        cmp_19_8_147_5:
        test r15, r15
        jne if_19_5_147_5_end
        jmp if_19_8_147_5_code
        if_19_8_147_5_code:
            mov rdi, 1
            exit_19_17_147_5:
                    mov rax, 60
                syscall
            exit_19_17_147_5_end:
        if_19_5_147_5_end:
    assert_147_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_152_12:
        lea r13, [rsp - 204]
        mov r12, 0
        cmp r12, 1
        mov r11, 152
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        mov r14, qword [r13 + 8]
    cmp r14, 73
    jne bool_false_152_12
    jmp bool_true_152_12
    bool_true_152_12:
    mov r15, true
    jmp bool_end_152_12
    bool_false_152_12:
    mov r15, false
    bool_end_152_12:
    assert_152_5:
        if_19_8_152_5:
        cmp_19_8_152_5:
        test r15, r15
        jne if_19_5_152_5_end
        jmp if_19_8_152_5_code
        if_19_8_152_5_code:
            mov rdi, 1
            exit_19_17_152_5:
                    mov rax, 60
                syscall
            exit_19_17_152_5_end:
        if_19_5_152_5_end:
    assert_152_5_end:
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 8
    mov r13, 157
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    cmp r14, 8
    mov r13, 157
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8 + 0], 0xffee
    cmp_158_12:
        lea r13, [rsp - 796]
        mov r12, 1
        cmp r12, 8
        mov r11, 158
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 8
        mov r11, 158
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_158_12
    jmp bool_true_158_12
    bool_true_158_12:
    mov r15, true
    jmp bool_end_158_12
    bool_false_158_12:
    mov r15, false
    bool_end_158_12:
    assert_158_5:
        if_19_8_158_5:
        cmp_19_8_158_5:
        test r15, r15
        jne if_19_5_158_5_end
        jmp if_19_8_158_5_code
        if_19_8_158_5_code:
            mov rdi, 1
            exit_19_17_158_5:
                    mov rax, 60
                syscall
            exit_19_17_158_5_end:
        if_19_5_158_5_end:
    assert_158_5_end:
    mov rcx, 8
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 8
    mov r13, 161
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 8
    mov r14, 161
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 796]
    mov r14, 0
    cmp r14, 8
    mov r13, 162
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 8
    mov r14, 162
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_165_12:
        lea r13, [rsp - 796]
        mov r12, 0
        cmp r12, 8
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 8
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_165_12
    jmp bool_true_165_12
    bool_true_165_12:
    mov r15, true
    jmp bool_end_165_12
    bool_false_165_12:
    mov r15, false
    bool_end_165_12:
    assert_165_5:
        if_19_8_165_5:
        cmp_19_8_165_5:
        test r15, r15
        jne if_19_5_165_5_end
        jmp if_19_8_165_5_code
        if_19_8_165_5_code:
            mov rdi, 1
            exit_19_17_165_5:
                    mov rax, 60
                syscall
            exit_19_17_165_5_end:
        if_19_5_165_5_end:
    assert_165_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_167_5:
            mov rax, 1
            mov rdi, 1
        syscall
    print_167_5_end:
    loop_168_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_169_9:
                mov rax, 1
                mov rdi, 1
            syscall
        print_169_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_171_19:
                mov rax, 0
                mov rdi, 0
            syscall
                mov qword [rsp - 804], rax
        read_171_19_end:
        sub qword [rsp - 804], 1
        if_175_12:
        cmp_175_12:
        cmp qword [rsp - 804], 0
        jne if_177_19
        jmp if_175_12_code
        if_175_12_code:
            jmp loop_168_5_end
        jmp if_175_9_end
        if_177_19:
        cmp_177_19:
        cmp qword [rsp - 804], 4
        jg if_else_175_9
        jmp if_177_19_code
        if_177_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_178_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_178_13_end:
            jmp loop_168_5
        jmp if_175_9_end
        if_else_175_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_181_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_181_13_end:
            mov rdx, qword [rsp - 804]
            lea rsi, [rsp - 284]
            print_182_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_182_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_183_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_183_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_184_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_184_13_end:
        if_175_9_end:
    jmp loop_168_5
    loop_168_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    lea rdi, [num_buffer + 19]
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
    lea rdx, [num_buffer + 20]
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
    msg_panic: db 'panic: bounds at line '
    msg_panic_len equ $ - msg_panic
section .bss
    num_buffer: resb 21
```

## With comments

```nasm

;
; generated by baz
;

DEFAULT REL

section .bss
stk resd 131072
stk.end:

true equ 1
false equ 0

section .data
;[1:1] field hello = "hello world from baz\n"
hello: db 'hello world from baz', 10,''
hello.len equ $ - hello
;[2:1] field prompt1 = "enter name:\n"
prompt1: db 'enter name:', 10,''
prompt1.len equ $ - prompt1
;[3:1] field prompt2 = "that is not a name.\n"
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $ - prompt2
;[4:1] field prompt3 = "hello "
prompt3: db 'hello '
prompt3.len equ $ - prompt3
;[5:1] field dot = "."
dot: db '.'
dot.len equ $ - dot
;[6:1] field nl = "\n"
nl: db '', 10,''
nl.len equ $ - nl

section .text
bits 64
global _start
_start:
mov rsp,stk.end
;
; program
;
;[8:1] # all functions are inlined
;[10:1] # arguments can be placed in specified register using `reg_...` syntax
;[39:1] # user types are defined using keyword `type`
;[41:1] # default type is `i64` and does not need to be specified
;[43:1] point : 16 B    fields: 
;[43:1]       name :  size :  offset :  array? : array size
;[43:1]          x :     8 :       0 :      no :           
;[43:1]          y :     8 :       8 :      no :           

;[45:1] object : 20 B    fields: 
;[45:1]       name :  size :  offset :  array? : array size
;[45:1]        pos :    16 :       0 :      no :           
;[45:1]      color :     4 :      16 :      no :           

;[47:1] world : 64 B    fields: 
;[47:1]       name :  size :  offset :  array? : array size
;[47:1]  locations :    64 :       0 :     yes :          8

;[49:1] # function arguments are equivalent to mutable references
;[56:1] # default argument type is `i64`
;[63:1] # return target is specified as a variable, in this case `res`
;[73:1] # array arguments are declared with type and []
main:
;   [80:5] var arr : i32[4]
;   [80:9] arr: i32[4] @ dword [rsp - 16]
;   [80:9] clear array 4 * 4 B = 16 B
;   [80:5] allocate named register 'rdi'
;   [80:5] allocate named register 'rcx'
;   [80:5] allocate named register 'rax'
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
;   [80:5] free named register 'rax'
;   [80:5] free named register 'rcx'
;   [80:5] free named register 'rdi'
;   [81:5] # arrays are initialized to 0
;   [83:5] var ix = 1
;   [83:9] ix: i64 @ qword [rsp - 24]
;   [83:9] ix =1
;   [83:14] 1
;   [83:14] 1
;   [83:14] 1
    mov qword [rsp - 24], 1
;   [85:5] arr[ix] = 2
;   [85:5] allocate scratch register -> r15
;   [85:9] set array index
;   [85:9] ix
;   [85:9] ix
;   [85:9] ix
    mov r15, qword [rsp - 24]
;   [85:9] bounds check
    cmp r15, 4
;   [85:9] allocate scratch register -> r14
;   [85:9] line number
    mov r14, 85
    cmovge rbp, r14
;   [85:9] free scratch register 'r14'
    jge panic_bounds
;   [85:15] 2
;   [85:15] 2
;   [85:15] 2
    mov dword [rsp + r15 * 4 - 16], 2
;   [85:5] free scratch register 'r15'
;   [86:5] arr[ix + 1] = arr[ix]
;   [86:5] allocate scratch register -> r15
;   [86:9] set array index
;   [86:9] ix + 1
;   [86:9] ix + 1
;   [86:9] ix
    mov r15, qword [rsp - 24]
;   [86:14] r15 + 1
    add r15, 1
;   [86:9] bounds check
    cmp r15, 4
;   [86:9] allocate scratch register -> r14
;   [86:9] line number
    mov r14, 86
    cmovge rbp, r14
;   [86:9] free scratch register 'r14'
    jge panic_bounds
;   [86:19] arr[ix]
;   [86:19] arr[ix]
;   [86:19] arr[ix]
;   [86:19] allocate scratch register -> r14
;   [86:23] set array index
;   [86:23] ix
;   [86:23] ix
;   [86:23] ix
    mov r14, qword [rsp - 24]
;   [86:23] bounds check
    cmp r14, 4
;   [86:23] allocate scratch register -> r13
;   [86:23] line number
    mov r13, 86
    cmovge rbp, r13
;   [86:23] free scratch register 'r13'
    jge panic_bounds
;   [86:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
;   [86:19] free scratch register 'r13'
;   [86:19] free scratch register 'r14'
;   [86:5] free scratch register 'r15'
;   [87:5] assert(arr[1] == 2)
;   [87:12] allocate scratch register -> r15
;   [87:12] arr[1] == 2
;   [87:12] ? arr[1] == 2
;   [87:12] ? arr[1] == 2
    cmp_87_12:
;   [87:12] allocate scratch register -> r14
;       [87:12] arr[1]
;       [87:12] arr[1]
;       [87:12] allocate scratch register -> r13
;       [87:16] set array index
;       [87:16] 1
;       [87:16] 1
;       [87:16] 1
        mov r13, 1
;       [87:16] bounds check
        cmp r13, 4
;       [87:16] allocate scratch register -> r12
;       [87:16] line number
        mov r12, 87
        cmovge rbp, r12
;       [87:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [87:12] free scratch register 'r13'
    cmp r14, 2
;   [87:12] free scratch register 'r14'
    jne bool_false_87_12
    jmp bool_true_87_12
    bool_true_87_12:
    mov r15, true
    jmp bool_end_87_12
    bool_false_87_12:
    mov r15, false
    bool_end_87_12:
;   [18:6] assert(expr : bool) 
    assert_87_5:
;       [87:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_87_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_87_5:
        test r15, r15
        jne if_19_5_87_5_end
        jmp if_19_8_87_5_code
        if_19_8_87_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_87_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_87_5_end:
        if_19_5_87_5_end:
;       [87:5] free scratch register 'r15'
    assert_87_5_end:
;   [88:5] assert(arr[2] == 2)
;   [88:12] allocate scratch register -> r15
;   [88:12] arr[2] == 2
;   [88:12] ? arr[2] == 2
;   [88:12] ? arr[2] == 2
    cmp_88_12:
;   [88:12] allocate scratch register -> r14
;       [88:12] arr[2]
;       [88:12] arr[2]
;       [88:12] allocate scratch register -> r13
;       [88:16] set array index
;       [88:16] 2
;       [88:16] 2
;       [88:16] 2
        mov r13, 2
;       [88:16] bounds check
        cmp r13, 4
;       [88:16] allocate scratch register -> r12
;       [88:16] line number
        mov r12, 88
        cmovge rbp, r12
;       [88:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [88:12] free scratch register 'r13'
    cmp r14, 2
;   [88:12] free scratch register 'r14'
    jne bool_false_88_12
    jmp bool_true_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
;   [18:6] assert(expr : bool) 
    assert_88_5:
;       [88:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_88_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_88_5:
        test r15, r15
        jne if_19_5_88_5_end
        jmp if_19_8_88_5_code
        if_19_8_88_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_88_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_88_5_end:
        if_19_5_88_5_end:
;       [88:5] free scratch register 'r15'
    assert_88_5_end:
;   [90:5] array_copy(arr[2], arr, 2)
;   [90:5] allocate named register 'rsi'
;   [90:5] allocate named register 'rdi'
;   [90:5] allocate named register 'rcx'
;   [90:29] 2
;   [90:29] 2
;   [90:29] 2
;   [90:29] 2
    mov rcx, 2
;   [90:16] arr[2]
;   [90:16] allocate scratch register -> r15
;   [90:20] set array index
;   [90:20] 2
;   [90:20] 2
;   [90:20] 2
    mov r15, 2
;   [90:20] bounds check
;   [90:20] allocate scratch register -> r14
    mov r14, rcx
    add r14, r15
    cmp r14, 4
;   [90:20] allocate scratch register -> r13
;   [90:20] line number
    mov r13, 90
    cmovg rbp, r13
;   [90:20] free scratch register 'r13'
;   [90:20] free scratch register 'r14'
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
;   [90:5] free scratch register 'r15'
;   [90:24] arr
;   [90:24] bounds check
    cmp rcx, 4
;   [90:24] allocate scratch register -> r15
;   [90:24] line number
    mov r15, 90
    cmovg rbp, r15
;   [90:24] free scratch register 'r15'
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
;   [90:5] free named register 'rcx'
;   [90:5] free named register 'rdi'
;   [90:5] free named register 'rsi'
;   [91:5] # copy from, to, number of elements
;   [92:5] assert(arr[0] == 2)
;   [92:12] allocate scratch register -> r15
;   [92:12] arr[0] == 2
;   [92:12] ? arr[0] == 2
;   [92:12] ? arr[0] == 2
    cmp_92_12:
;   [92:12] allocate scratch register -> r14
;       [92:12] arr[0]
;       [92:12] arr[0]
;       [92:12] allocate scratch register -> r13
;       [92:16] set array index
;       [92:16] 0
;       [92:16] 0
;       [92:16] 0
        mov r13, 0
;       [92:16] bounds check
        cmp r13, 4
;       [92:16] allocate scratch register -> r12
;       [92:16] line number
        mov r12, 92
        cmovge rbp, r12
;       [92:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [92:12] free scratch register 'r13'
    cmp r14, 2
;   [92:12] free scratch register 'r14'
    jne bool_false_92_12
    jmp bool_true_92_12
    bool_true_92_12:
    mov r15, true
    jmp bool_end_92_12
    bool_false_92_12:
    mov r15, false
    bool_end_92_12:
;   [18:6] assert(expr : bool) 
    assert_92_5:
;       [92:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_92_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_92_5:
        test r15, r15
        jne if_19_5_92_5_end
        jmp if_19_8_92_5_code
        if_19_8_92_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_92_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_92_5_end:
        if_19_5_92_5_end:
;       [92:5] free scratch register 'r15'
    assert_92_5_end:
;   [94:5] var arr1 : i32[8]
;   [94:9] arr1: i32[8] @ dword [rsp - 56]
;   [94:9] clear array 8 * 4 B = 32 B
;   [94:5] allocate named register 'rdi'
;   [94:5] allocate named register 'rcx'
;   [94:5] allocate named register 'rax'
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
;   [94:5] free named register 'rax'
;   [94:5] free named register 'rcx'
;   [94:5] free named register 'rdi'
;   [95:5] array_copy(arr, arr1, 4)
;   [95:5] allocate named register 'rsi'
;   [95:5] allocate named register 'rdi'
;   [95:5] allocate named register 'rcx'
;   [95:27] 4
;   [95:27] 4
;   [95:27] 4
;   [95:27] 4
    mov rcx, 4
;   [95:16] arr
;   [95:16] bounds check
    cmp rcx, 4
;   [95:16] allocate scratch register -> r15
;   [95:16] line number
    mov r15, 95
    cmovg rbp, r15
;   [95:16] free scratch register 'r15'
    jg panic_bounds
    lea rsi, [rsp - 16]
;   [95:21] arr1
;   [95:21] bounds check
    cmp rcx, 8
;   [95:21] allocate scratch register -> r15
;   [95:21] line number
    mov r15, 95
    cmovg rbp, r15
;   [95:21] free scratch register 'r15'
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
;   [95:5] free named register 'rcx'
;   [95:5] free named register 'rdi'
;   [95:5] free named register 'rsi'
;   [96:5] assert(arrays_equal(arr, arr1, 4))
;   [96:12] allocate scratch register -> r15
;   [96:12] arrays_equal(arr, arr1, 4)
;   [96:12] ? arrays_equal(arr, arr1, 4)
;   [96:12] ? arrays_equal(arr, arr1, 4)
    cmp_96_12:
;   [96:12] allocate scratch register -> r14
;       [96:12] arrays_equal(arr, arr1, 4)
;       [96:12] r14 = arrays_equal(arr, arr1, 4)
;       [96:12] arrays_equal(arr, arr1, 4)
;       [96:12] allocate named register 'rsi'
;       [96:12] allocate named register 'rdi'
;       [96:12] allocate named register 'rcx'
;       [96:36] 4
;       [96:36] 4
;       [96:36] 4
;       [96:36] 4
        mov rcx, 4
;       [96:25] arr
;       [96:25] bounds check
        cmp rcx, 4
;       [96:25] allocate scratch register -> r13
;       [96:25] line number
        mov r13, 96
        cmovg rbp, r13
;       [96:25] free scratch register 'r13'
        jg panic_bounds
        lea rsi, [rsp - 16]
;       [96:30] arr1
;       [96:30] bounds check
        cmp rcx, 8
;       [96:30] allocate scratch register -> r13
;       [96:30] line number
        mov r13, 96
        cmovg rbp, r13
;       [96:30] free scratch register 'r13'
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [96:12] free named register 'rcx'
;       [96:12] free named register 'rdi'
;       [96:12] free named register 'rsi'
        je cmps_eq_96_12
        mov r14, false
        jmp cmps_end_96_12
        cmps_eq_96_12:
        mov r14, true
        cmps_end_96_12:
    test r14, r14
;   [96:12] free scratch register 'r14'
    je bool_false_96_12
    jmp bool_true_96_12
    bool_true_96_12:
    mov r15, true
    jmp bool_end_96_12
    bool_false_96_12:
    mov r15, false
    bool_end_96_12:
;   [18:6] assert(expr : bool) 
    assert_96_5:
;       [96:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_96_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_96_5:
        test r15, r15
        jne if_19_5_96_5_end
        jmp if_19_8_96_5_code
        if_19_8_96_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_96_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_96_5_end:
        if_19_5_96_5_end:
;       [96:5] free scratch register 'r15'
    assert_96_5_end:
;   [98:5] arr1[2] = -1
;   [98:5] allocate scratch register -> r15
;   [98:10] set array index
;   [98:10] 2
;   [98:10] 2
;   [98:10] 2
    mov r15, 2
;   [98:10] bounds check
    cmp r15, 8
;   [98:10] allocate scratch register -> r14
;   [98:10] line number
    mov r14, 98
    cmovge rbp, r14
;   [98:10] free scratch register 'r14'
    jge panic_bounds
;   [98:15] -1
;   [98:15] -1
;   [98:16] -1
    mov dword [rsp + r15 * 4 - 56], -1
;   [98:5] free scratch register 'r15'
;   [99:5] assert(not arrays_equal(arr, arr1, 4))
;   [99:12] allocate scratch register -> r15
;   [99:12] not arrays_equal(arr, arr1, 4)
;   [99:12] ? not arrays_equal(arr, arr1, 4)
;   [99:12] ? not arrays_equal(arr, arr1, 4)
    cmp_99_12:
;   [99:16] allocate scratch register -> r14
;       [99:16] arrays_equal(arr, arr1, 4)
;       [99:16] r14 = arrays_equal(arr, arr1, 4)
;       [99:16] arrays_equal(arr, arr1, 4)
;       [99:16] allocate named register 'rsi'
;       [99:16] allocate named register 'rdi'
;       [99:16] allocate named register 'rcx'
;       [99:40] 4
;       [99:40] 4
;       [99:40] 4
;       [99:40] 4
        mov rcx, 4
;       [99:29] arr
;       [99:29] bounds check
        cmp rcx, 4
;       [99:29] allocate scratch register -> r13
;       [99:29] line number
        mov r13, 99
        cmovg rbp, r13
;       [99:29] free scratch register 'r13'
        jg panic_bounds
        lea rsi, [rsp - 16]
;       [99:34] arr1
;       [99:34] bounds check
        cmp rcx, 8
;       [99:34] allocate scratch register -> r13
;       [99:34] line number
        mov r13, 99
        cmovg rbp, r13
;       [99:34] free scratch register 'r13'
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [99:16] free named register 'rcx'
;       [99:16] free named register 'rdi'
;       [99:16] free named register 'rsi'
        je cmps_eq_99_16
        mov r14, false
        jmp cmps_end_99_16
        cmps_eq_99_16:
        mov r14, true
        cmps_end_99_16:
    test r14, r14
;   [99:12] free scratch register 'r14'
    jne bool_false_99_12
    jmp bool_true_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
;   [18:6] assert(expr : bool) 
    assert_99_5:
;       [99:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_99_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_99_5:
        test r15, r15
        jne if_19_5_99_5_end
        jmp if_19_8_99_5_code
        if_19_8_99_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_99_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_99_5_end:
        if_19_5_99_5_end:
;       [99:5] free scratch register 'r15'
    assert_99_5_end:
;   [101:5] ix = 3
;   [101:10] 3
;   [101:10] 3
;   [101:10] 3
    mov qword [rsp - 24], 3
;   [102:5] arr[ix] = ~inv(arr[ix - 1])
;   [102:5] allocate scratch register -> r15
;   [102:9] set array index
;   [102:9] ix
;   [102:9] ix
;   [102:9] ix
    mov r15, qword [rsp - 24]
;   [102:9] bounds check
    cmp r15, 4
;   [102:9] allocate scratch register -> r14
;   [102:9] line number
    mov r14, 102
    cmovge rbp, r14
;   [102:9] free scratch register 'r14'
    jge panic_bounds
;   [102:15] ~inv(arr[ix - 1])
;   [102:15] ~inv(arr[ix - 1])
;   [102:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;   [102:16] ~inv(arr[ix - 1])
;   [102:20] allocate scratch register -> r14
;   [102:24] set array index
;   [102:24] ix - 1
;   [102:24] ix - 1
;   [102:24] ix
    mov r14, qword [rsp - 24]
;   [102:29] r14 - 1
    sub r14, 1
;   [102:24] bounds check
    cmp r14, 4
;   [102:24] allocate scratch register -> r13
;   [102:24] line number
    mov r13, 102
    cmovge rbp, r13
;   [102:24] free scratch register 'r13'
    jge panic_bounds
;   [65:6] inv(i : i32) : i32 res 
    inv_102_16:
;       [102:16] alias res -> dword [rsp + r15 * 4 - 16]  (lea: , len: 0)
;       [102:16] alias i -> arr  (lea: rsp + r14 * 4 - 16, len: 4)
;       [66:5] res = ~i
;       [66:11] ~i
;       [66:11] ~i
;       [66:12] ~i
;       [66:12] allocate scratch register -> r13
        lea r13, [rsp + r14 * 4 - 16]
;       [66:12] allocate scratch register -> r12
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
;       [66:12] free scratch register 'r12'
        not dword [rsp + r15 * 4 - 16]
;       [66:12] free scratch register 'r13'
;       [102:16] free scratch register 'r14'
    inv_102_16_end:
    not dword [rsp + r15 * 4 - 16]
;   [102:5] free scratch register 'r15'
;   [103:5] assert(arr[ix] == 2)
;   [103:12] allocate scratch register -> r15
;   [103:12] arr[ix] == 2
;   [103:12] ? arr[ix] == 2
;   [103:12] ? arr[ix] == 2
    cmp_103_12:
;   [103:12] allocate scratch register -> r14
;       [103:12] arr[ix]
;       [103:12] arr[ix]
;       [103:12] allocate scratch register -> r13
;       [103:16] set array index
;       [103:16] ix
;       [103:16] ix
;       [103:16] ix
        mov r13, qword [rsp - 24]
;       [103:16] bounds check
        cmp r13, 4
;       [103:16] allocate scratch register -> r12
;       [103:16] line number
        mov r12, 103
        cmovge rbp, r12
;       [103:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [103:12] free scratch register 'r13'
    cmp r14, 2
;   [103:12] free scratch register 'r14'
    jne bool_false_103_12
    jmp bool_true_103_12
    bool_true_103_12:
    mov r15, true
    jmp bool_end_103_12
    bool_false_103_12:
    mov r15, false
    bool_end_103_12:
;   [18:6] assert(expr : bool) 
    assert_103_5:
;       [103:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_103_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_103_5:
        test r15, r15
        jne if_19_5_103_5_end
        jmp if_19_8_103_5_code
        if_19_8_103_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_103_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_103_5_end:
        if_19_5_103_5_end:
;       [103:5] free scratch register 'r15'
    assert_103_5_end:
;   [105:5] faz(arr)
;   [75:6] faz(arg : i32[]) 
    faz_105_5:
;       [105:5] alias arg -> arr  (lea: , len: 0)
;       [76:5] arg[1] = 0xfe
;       [76:5] allocate scratch register -> r15
;       [76:9] set array index
;       [76:9] 1
;       [76:9] 1
;       [76:9] 1
        mov r15, 1
;       [76:9] bounds check
        cmp r15, 4
;       [76:9] allocate scratch register -> r14
;       [76:9] line number
        mov r14, 76
        cmovge rbp, r14
;       [76:9] free scratch register 'r14'
        jge panic_bounds
;       [76:14] 0xfe
;       [76:14] 0xfe
;       [76:14] 0xfe
        mov dword [rsp + r15 * 4 - 16], 0xfe
;       [76:5] free scratch register 'r15'
    faz_105_5_end:
;   [106:5] assert(arr[1] == 0xfe)
;   [106:12] allocate scratch register -> r15
;   [106:12] arr[1] == 0xfe
;   [106:12] ? arr[1] == 0xfe
;   [106:12] ? arr[1] == 0xfe
    cmp_106_12:
;   [106:12] allocate scratch register -> r14
;       [106:12] arr[1]
;       [106:12] arr[1]
;       [106:12] allocate scratch register -> r13
;       [106:16] set array index
;       [106:16] 1
;       [106:16] 1
;       [106:16] 1
        mov r13, 1
;       [106:16] bounds check
        cmp r13, 4
;       [106:16] allocate scratch register -> r12
;       [106:16] line number
        mov r12, 106
        cmovge rbp, r12
;       [106:16] free scratch register 'r12'
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [106:12] free scratch register 'r13'
    cmp r14, 0xfe
;   [106:12] free scratch register 'r14'
    jne bool_false_106_12
    jmp bool_true_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
;   [18:6] assert(expr : bool) 
    assert_106_5:
;       [106:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_106_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_106_5:
        test r15, r15
        jne if_19_5_106_5_end
        jmp if_19_8_106_5_code
        if_19_8_106_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_106_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_106_5_end:
        if_19_5_106_5_end:
;       [106:5] free scratch register 'r15'
    assert_106_5_end:
;   [108:5] var p : point = {0, 0}
;   [108:9] p: point @ qword [rsp - 72]
;   [108:9] p ={0, 0}
;   [108:21] {0, 0}
;   [108:21] {0, 0}
;   [108:22] 0
;   [108:22] 0
;   [108:22] 0
    mov qword [rsp - 72], 0
;   [108:25] 0
;   [108:25] 0
;   [108:25] 0
    mov qword [rsp - 64], 0
;   [109:5] foo(p)
;   [51:6] foo(pt : point) 
    foo_109_5:
;       [109:5] alias pt -> p  (lea: , len: 0)
;       [52:5] pt.x = 0b10
;       [52:12] 0b10
;       [52:12] 0b10
;       [52:12] 0b10
        mov qword [rsp - 72], 0b10
;       [52:20] # binary value 2
;       [53:5] pt.y = 0xb
;       [53:12] 0xb
;       [53:12] 0xb
;       [53:12] 0xb
        mov qword [rsp - 64], 0xb
;       [53:20] # hex value 11
    foo_109_5_end:
;   [110:5] assert(p.x == 2)
;   [110:12] allocate scratch register -> r15
;   [110:12] p.x == 2
;   [110:12] ? p.x == 2
;   [110:12] ? p.x == 2
    cmp_110_12:
    cmp qword [rsp - 72], 2
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
;   [18:6] assert(expr : bool) 
    assert_110_5:
;       [110:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_110_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_110_5:
        test r15, r15
        jne if_19_5_110_5_end
        jmp if_19_8_110_5_code
        if_19_8_110_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_110_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_110_5_end:
        if_19_5_110_5_end:
;       [110:5] free scratch register 'r15'
    assert_110_5_end:
;   [111:5] assert(p.y == 0xb)
;   [111:12] allocate scratch register -> r15
;   [111:12] p.y == 0xb
;   [111:12] ? p.y == 0xb
;   [111:12] ? p.y == 0xb
    cmp_111_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_111_12
    jmp bool_true_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
;   [18:6] assert(expr : bool) 
    assert_111_5:
;       [111:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_111_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_111_5:
        test r15, r15
        jne if_19_5_111_5_end
        jmp if_19_8_111_5_code
        if_19_8_111_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_111_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_111_5_end:
        if_19_5_111_5_end:
;       [111:5] free scratch register 'r15'
    assert_111_5_end:
;   [113:5] var i = 0
;   [113:9] i: i64 @ qword [rsp - 80]
;   [113:9] i =0
;   [113:13] 0
;   [113:13] 0
;   [113:13] 0
    mov qword [rsp - 80], 0
;   [114:5] bar(i)
;   [58:6] bar(arg) 
    bar_114_5:
;       [114:5] alias arg -> i  (lea: , len: 0)
        if_59_8_114_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_114_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_114_5_end
        jmp if_59_8_114_5_code
        if_59_8_114_5_code:
;           [59:17] return
            jmp bar_114_5_end
        if_59_5_114_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
;       [60:11] 0xff
;       [60:11] 0xff
        mov qword [rsp - 80], 0xff
    bar_114_5_end:
;   [115:5] assert(i == 0)
;   [115:12] allocate scratch register -> r15
;   [115:12] i == 0
;   [115:12] ? i == 0
;   [115:12] ? i == 0
    cmp_115_12:
    cmp qword [rsp - 80], 0
    jne bool_false_115_12
    jmp bool_true_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
;   [18:6] assert(expr : bool) 
    assert_115_5:
;       [115:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_115_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_115_5:
        test r15, r15
        jne if_19_5_115_5_end
        jmp if_19_8_115_5_code
        if_19_8_115_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_115_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_115_5_end:
        if_19_5_115_5_end:
;       [115:5] free scratch register 'r15'
    assert_115_5_end:
;   [117:5] i = 1
;   [117:9] 1
;   [117:9] 1
;   [117:9] 1
    mov qword [rsp - 80], 1
;   [118:5] bar(i)
;   [58:6] bar(arg) 
    bar_118_5:
;       [118:5] alias arg -> i  (lea: , len: 0)
        if_59_8_118_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_118_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_118_5_end
        jmp if_59_8_118_5_code
        if_59_8_118_5_code:
;           [59:17] return
            jmp bar_118_5_end
        if_59_5_118_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
;       [60:11] 0xff
;       [60:11] 0xff
        mov qword [rsp - 80], 0xff
    bar_118_5_end:
;   [119:5] assert(i == 0xff)
;   [119:12] allocate scratch register -> r15
;   [119:12] i == 0xff
;   [119:12] ? i == 0xff
;   [119:12] ? i == 0xff
    cmp_119_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_119_12
    jmp bool_true_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
;   [18:6] assert(expr : bool) 
    assert_119_5:
;       [119:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_119_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_119_5:
        test r15, r15
        jne if_19_5_119_5_end
        jmp if_19_8_119_5_code
        if_19_8_119_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_119_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_119_5_end:
        if_19_5_119_5_end:
;       [119:5] free scratch register 'r15'
    assert_119_5_end:
;   [121:5] var j = 1
;   [121:9] j: i64 @ qword [rsp - 88]
;   [121:9] j =1
;   [121:13] 1
;   [121:13] 1
;   [121:13] 1
    mov qword [rsp - 88], 1
;   [122:5] var k = baz(j)
;   [122:9] k: i64 @ qword [rsp - 96]
;   [122:9] k =baz(j)
;   [122:13] baz(j)
;   [122:13] allocate scratch register -> r15
;   [122:13] baz(j)
;   [122:13] r15 = baz(j)
;   [122:13] baz(j)
;   [69:6] baz(arg) : i64 res 
    baz_122_13:
;       [122:13] alias res -> r15  (lea: , len: 0)
;       [122:13] alias arg -> j  (lea: , len: 0)
;       [70:5] res = arg * 2
;       [70:11] arg * 2
;       [70:11] arg * 2
;       [70:11] arg
        mov r15, qword [rsp - 88]
;       [70:17] res * 2
        imul r15, 2
    baz_122_13_end:
    mov qword [rsp - 96], r15
;   [122:13] free scratch register 'r15'
;   [123:5] assert(k == 2)
;   [123:12] allocate scratch register -> r15
;   [123:12] k == 2
;   [123:12] ? k == 2
;   [123:12] ? k == 2
    cmp_123_12:
    cmp qword [rsp - 96], 2
    jne bool_false_123_12
    jmp bool_true_123_12
    bool_true_123_12:
    mov r15, true
    jmp bool_end_123_12
    bool_false_123_12:
    mov r15, false
    bool_end_123_12:
;   [18:6] assert(expr : bool) 
    assert_123_5:
;       [123:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_123_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_123_5:
        test r15, r15
        jne if_19_5_123_5_end
        jmp if_19_8_123_5_code
        if_19_8_123_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_123_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_123_5_end:
        if_19_5_123_5_end:
;       [123:5] free scratch register 'r15'
    assert_123_5_end:
;   [125:5] k = baz(1)
;   [125:9] baz(1)
;   [125:9] allocate scratch register -> r15
;   [125:9] baz(1)
;   [125:9] r15 = baz(1)
;   [125:9] baz(1)
;   [69:6] baz(arg) : i64 res 
    baz_125_9:
;       [125:9] alias res -> r15  (lea: , len: 0)
;       [125:9] alias arg -> 1  (lea: , len: 0)
;       [70:5] res = arg * 2
;       [70:11] arg * 2
;       [70:11] arg * 2
;       [70:11] arg
        mov r15, 1
;       [70:17] res * 2
        imul r15, 2
    baz_125_9_end:
    mov qword [rsp - 96], r15
;   [125:9] free scratch register 'r15'
;   [126:5] assert(k == 2)
;   [126:12] allocate scratch register -> r15
;   [126:12] k == 2
;   [126:12] ? k == 2
;   [126:12] ? k == 2
    cmp_126_12:
    cmp qword [rsp - 96], 2
    jne bool_false_126_12
    jmp bool_true_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
;   [18:6] assert(expr : bool) 
    assert_126_5:
;       [126:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_126_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_126_5:
        test r15, r15
        jne if_19_5_126_5_end
        jmp if_19_8_126_5_code
        if_19_8_126_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_126_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
;       [126:5] free scratch register 'r15'
    assert_126_5_end:
;   [128:5] var p0 : point = {baz(2), 0}
;   [128:9] p0: point @ qword [rsp - 112]
;   [128:9] p0 ={baz(2), 0}
;   [128:22] {baz(2), 0}
;   [128:22] {baz(2), 0}
;   [128:23] baz(2)
;   [128:23] allocate scratch register -> r15
;   [128:23] baz(2)
;   [128:23] r15 = baz(2)
;   [128:23] baz(2)
;   [69:6] baz(arg) : i64 res 
    baz_128_23:
;       [128:23] alias res -> r15  (lea: , len: 0)
;       [128:23] alias arg -> 2  (lea: , len: 0)
;       [70:5] res = arg * 2
;       [70:11] arg * 2
;       [70:11] arg * 2
;       [70:11] arg
        mov r15, 2
;       [70:17] res * 2
        imul r15, 2
    baz_128_23_end:
    mov qword [rsp - 112], r15
;   [128:23] free scratch register 'r15'
;   [128:31] 0
;   [128:31] 0
;   [128:31] 0
    mov qword [rsp - 104], 0
;   [129:5] assert(p0.x == 4)
;   [129:12] allocate scratch register -> r15
;   [129:12] p0.x == 4
;   [129:12] ? p0.x == 4
;   [129:12] ? p0.x == 4
    cmp_129_12:
    cmp qword [rsp - 112], 4
    jne bool_false_129_12
    jmp bool_true_129_12
    bool_true_129_12:
    mov r15, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15, false
    bool_end_129_12:
;   [18:6] assert(expr : bool) 
    assert_129_5:
;       [129:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_129_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_129_5:
        test r15, r15
        jne if_19_5_129_5_end
        jmp if_19_8_129_5_code
        if_19_8_129_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_129_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_129_5_end:
        if_19_5_129_5_end:
;       [129:5] free scratch register 'r15'
    assert_129_5_end:
;   [131:5] var x = 1
;   [131:9] x: i64 @ qword [rsp - 120]
;   [131:9] x =1
;   [131:13] 1
;   [131:13] 1
;   [131:13] 1
    mov qword [rsp - 120], 1
;   [132:5] var y = 2
;   [132:9] y: i64 @ qword [rsp - 128]
;   [132:9] y =2
;   [132:13] 2
;   [132:13] 2
;   [132:13] 2
    mov qword [rsp - 128], 2
;   [134:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [134:9] o1: object @ qword [rsp - 148]
;   [134:9] o1 ={{x * 10, y}, 0xff0000}
;   [134:23] {{x * 10, y}, 0xff0000}
;   [134:23] {{x * 10, y}, 0xff0000}
;       [134:24] {x * 10, y}
;       [134:25] x * 10
;       [134:25] allocate scratch register -> r15
;       [134:25] x * 10
;       [134:25] x
        mov r15, qword [rsp - 120]
;       [134:29] r15 * 10
        imul r15, 10
        mov qword [rsp - 148], r15
;       [134:25] free scratch register 'r15'
;       [134:33] y
;       [134:33] y
;       [134:33] y
;       [134:33] allocate scratch register -> r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
;       [134:33] free scratch register 'r15'
;   [134:37] 0xff0000
;   [134:37] 0xff0000
;   [134:37] 0xff0000
    mov dword [rsp - 132], 0xff0000
;   [135:5] assert(o1.pos.x == 10)
;   [135:12] allocate scratch register -> r15
;   [135:12] o1.pos.x == 10
;   [135:12] ? o1.pos.x == 10
;   [135:12] ? o1.pos.x == 10
    cmp_135_12:
    cmp qword [rsp - 148], 10
    jne bool_false_135_12
    jmp bool_true_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
;   [18:6] assert(expr : bool) 
    assert_135_5:
;       [135:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_135_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_135_5:
        test r15, r15
        jne if_19_5_135_5_end
        jmp if_19_8_135_5_code
        if_19_8_135_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_135_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_135_5_end:
        if_19_5_135_5_end:
;       [135:5] free scratch register 'r15'
    assert_135_5_end:
;   [136:5] assert(o1.pos.y == 2)
;   [136:12] allocate scratch register -> r15
;   [136:12] o1.pos.y == 2
;   [136:12] ? o1.pos.y == 2
;   [136:12] ? o1.pos.y == 2
    cmp_136_12:
    cmp qword [rsp - 140], 2
    jne bool_false_136_12
    jmp bool_true_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
;   [18:6] assert(expr : bool) 
    assert_136_5:
;       [136:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_136_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_136_5:
        test r15, r15
        jne if_19_5_136_5_end
        jmp if_19_8_136_5_code
        if_19_8_136_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_136_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
;       [136:5] free scratch register 'r15'
    assert_136_5_end:
;   [137:5] assert(o1.color == 0xff0000)
;   [137:12] allocate scratch register -> r15
;   [137:12] o1.color == 0xff0000
;   [137:12] ? o1.color == 0xff0000
;   [137:12] ? o1.color == 0xff0000
    cmp_137_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_137_12
    jmp bool_true_137_12
    bool_true_137_12:
    mov r15, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15, false
    bool_end_137_12:
;   [18:6] assert(expr : bool) 
    assert_137_5:
;       [137:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_137_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_137_5:
        test r15, r15
        jne if_19_5_137_5_end
        jmp if_19_8_137_5_code
        if_19_8_137_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_137_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_137_5_end:
        if_19_5_137_5_end:
;       [137:5] free scratch register 'r15'
    assert_137_5_end:
;   [139:5] var p1 : point = {-x, -y}
;   [139:9] p1: point @ qword [rsp - 164]
;   [139:9] p1 ={-x, -y}
;   [139:22] {-x, -y}
;   [139:22] {-x, -y}
;   [139:23] -x
;   [139:23] -x
;   [139:24] -x
;   [139:24] allocate scratch register -> r15
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
;   [139:24] free scratch register 'r15'
    neg qword [rsp - 164]
;   [139:27] -y
;   [139:27] -y
;   [139:28] -y
;   [139:28] allocate scratch register -> r15
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
;   [139:28] free scratch register 'r15'
    neg qword [rsp - 156]
;   [140:5] o1.pos = p1
;   [140:5] allocate named register 'rsi'
;   [140:5] allocate named register 'rdi'
;   [140:5] allocate named register 'rcx'
    lea rdi, [rsp - 148]
;   [140:14] p1
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
;   [140:5] free named register 'rcx'
;   [140:5] free named register 'rdi'
;   [140:5] free named register 'rsi'
;   [141:5] assert(o1.pos.x == -1)
;   [141:12] allocate scratch register -> r15
;   [141:12] o1.pos.x == -1
;   [141:12] ? o1.pos.x == -1
;   [141:12] ? o1.pos.x == -1
    cmp_141_12:
    cmp qword [rsp - 148], -1
    jne bool_false_141_12
    jmp bool_true_141_12
    bool_true_141_12:
    mov r15, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15, false
    bool_end_141_12:
;   [18:6] assert(expr : bool) 
    assert_141_5:
;       [141:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_141_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_141_5:
        test r15, r15
        jne if_19_5_141_5_end
        jmp if_19_8_141_5_code
        if_19_8_141_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_141_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_141_5_end:
        if_19_5_141_5_end:
;       [141:5] free scratch register 'r15'
    assert_141_5_end:
;   [142:5] assert(o1.pos.y == -2)
;   [142:12] allocate scratch register -> r15
;   [142:12] o1.pos.y == -2
;   [142:12] ? o1.pos.y == -2
;   [142:12] ? o1.pos.y == -2
    cmp_142_12:
    cmp qword [rsp - 140], -2
    jne bool_false_142_12
    jmp bool_true_142_12
    bool_true_142_12:
    mov r15, true
    jmp bool_end_142_12
    bool_false_142_12:
    mov r15, false
    bool_end_142_12:
;   [18:6] assert(expr : bool) 
    assert_142_5:
;       [142:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_142_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_142_5:
        test r15, r15
        jne if_19_5_142_5_end
        jmp if_19_8_142_5_code
        if_19_8_142_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_142_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_142_5_end:
        if_19_5_142_5_end:
;       [142:5] free scratch register 'r15'
    assert_142_5_end:
;   [144:5] var o2 : object = o1
;   [144:9] o2: object @ qword [rsp - 184]
;   [144:9] o2 =o1
;   [144:9] allocate named register 'rsi'
;   [144:9] allocate named register 'rdi'
;   [144:9] allocate named register 'rcx'
    lea rdi, [rsp - 184]
;   [144:23] o1
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
;   [144:9] free named register 'rcx'
;   [144:9] free named register 'rdi'
;   [144:9] free named register 'rsi'
;   [145:5] assert(o2.pos.x == -1)
;   [145:12] allocate scratch register -> r15
;   [145:12] o2.pos.x == -1
;   [145:12] ? o2.pos.x == -1
;   [145:12] ? o2.pos.x == -1
    cmp_145_12:
    cmp qword [rsp - 184], -1
    jne bool_false_145_12
    jmp bool_true_145_12
    bool_true_145_12:
    mov r15, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15, false
    bool_end_145_12:
;   [18:6] assert(expr : bool) 
    assert_145_5:
;       [145:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_145_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_145_5:
        test r15, r15
        jne if_19_5_145_5_end
        jmp if_19_8_145_5_code
        if_19_8_145_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_145_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_145_5_end:
        if_19_5_145_5_end:
;       [145:5] free scratch register 'r15'
    assert_145_5_end:
;   [146:5] assert(o2.pos.y == -2)
;   [146:12] allocate scratch register -> r15
;   [146:12] o2.pos.y == -2
;   [146:12] ? o2.pos.y == -2
;   [146:12] ? o2.pos.y == -2
    cmp_146_12:
    cmp qword [rsp - 176], -2
    jne bool_false_146_12
    jmp bool_true_146_12
    bool_true_146_12:
    mov r15, true
    jmp bool_end_146_12
    bool_false_146_12:
    mov r15, false
    bool_end_146_12:
;   [18:6] assert(expr : bool) 
    assert_146_5:
;       [146:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_146_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_146_5:
        test r15, r15
        jne if_19_5_146_5_end
        jmp if_19_8_146_5_code
        if_19_8_146_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_146_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_146_5_end:
        if_19_5_146_5_end:
;       [146:5] free scratch register 'r15'
    assert_146_5_end:
;   [147:5] assert(o2.color == 0xff0000)
;   [147:12] allocate scratch register -> r15
;   [147:12] o2.color == 0xff0000
;   [147:12] ? o2.color == 0xff0000
;   [147:12] ? o2.color == 0xff0000
    cmp_147_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_147_12
    jmp bool_true_147_12
    bool_true_147_12:
    mov r15, true
    jmp bool_end_147_12
    bool_false_147_12:
    mov r15, false
    bool_end_147_12:
;   [18:6] assert(expr : bool) 
    assert_147_5:
;       [147:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_147_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_147_5:
        test r15, r15
        jne if_19_5_147_5_end
        jmp if_19_8_147_5_code
        if_19_8_147_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_147_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_147_5_end:
        if_19_5_147_5_end:
;       [147:5] free scratch register 'r15'
    assert_147_5_end:
;   [149:5] var o3 : object[1]
;   [149:9] o3: object[1] @ qword [rsp - 204]
;   [149:9] clear array 1 * 20 B = 20 B
;   [149:5] allocate named register 'rdi'
;   [149:5] allocate named register 'rcx'
;   [149:5] allocate named register 'rax'
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
;   [149:5] free named register 'rax'
;   [149:5] free named register 'rcx'
;   [149:5] free named register 'rdi'
;   [150:5] # index 0 in an array can be accessed without array index
;   [151:5] o3.pos.y = 73
;   [151:16] 73
;   [151:16] 73
;   [151:16] 73
    mov qword [rsp - 196], 73
;   [152:5] assert(o3[0].pos.y == 73)
;   [152:12] allocate scratch register -> r15
;   [152:12] o3[0].pos.y == 73
;   [152:12] ? o3[0].pos.y == 73
;   [152:12] ? o3[0].pos.y == 73
    cmp_152_12:
;   [152:12] allocate scratch register -> r14
;       [152:12] o3[0].pos.y
;       [152:12] o3[0].pos.y
;       [152:12] allocate scratch register -> r13
        lea r13, [rsp - 204]
;       [152:12] allocate scratch register -> r12
;       [152:15] set array index
;       [152:15] 0
;       [152:15] 0
;       [152:15] 0
        mov r12, 0
;       [152:15] bounds check
        cmp r12, 1
;       [152:15] allocate scratch register -> r11
;       [152:15] line number
        mov r11, 152
        cmovge rbp, r11
;       [152:15] free scratch register 'r11'
        jge panic_bounds
        imul r12, 20
        add r13, r12
;       [152:12] free scratch register 'r12'
        mov r14, qword [r13 + 8]
;       [152:12] free scratch register 'r13'
    cmp r14, 73
;   [152:12] free scratch register 'r14'
    jne bool_false_152_12
    jmp bool_true_152_12
    bool_true_152_12:
    mov r15, true
    jmp bool_end_152_12
    bool_false_152_12:
    mov r15, false
    bool_end_152_12:
;   [18:6] assert(expr : bool) 
    assert_152_5:
;       [152:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_152_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_152_5:
        test r15, r15
        jne if_19_5_152_5_end
        jmp if_19_8_152_5_code
        if_19_8_152_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_152_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_152_5_end:
        if_19_5_152_5_end:
;       [152:5] free scratch register 'r15'
    assert_152_5_end:
;   [154:5] var input_buffer : i8[80]
;   [154:9] input_buffer: i8[80] @ byte [rsp - 284]
;   [154:9] clear array 80 * 1 B = 80 B
;   [154:5] allocate named register 'rdi'
;   [154:5] allocate named register 'rcx'
;   [154:5] allocate named register 'rax'
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
;   [154:5] free named register 'rax'
;   [154:5] free named register 'rcx'
;   [154:5] free named register 'rdi'
;   [156:5] var worlds : world[8]
;   [156:9] worlds: world[8] @ qword [rsp - 796]
;   [156:9] clear array 8 * 64 B = 512 B
;   [156:5] allocate named register 'rdi'
;   [156:5] allocate named register 'rcx'
;   [156:5] allocate named register 'rax'
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
;   [156:5] free named register 'rax'
;   [156:5] free named register 'rcx'
;   [156:5] free named register 'rdi'
;   [157:5] worlds[1].locations[1] = 0xffee
;   [157:5] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [157:5] allocate scratch register -> r14
;   [157:12] set array index
;   [157:12] 1
;   [157:12] 1
;   [157:12] 1
    mov r14, 1
;   [157:12] bounds check
    cmp r14, 8
;   [157:12] allocate scratch register -> r13
;   [157:12] line number
    mov r13, 157
    cmovge rbp, r13
;   [157:12] free scratch register 'r13'
    jge panic_bounds
    shl r14, 6
    add r15, r14
;   [157:5] free scratch register 'r14'
;   [157:5] allocate scratch register -> r14
;   [157:25] set array index
;   [157:25] 1
;   [157:25] 1
;   [157:25] 1
    mov r14, 1
;   [157:25] bounds check
    cmp r14, 8
;   [157:25] allocate scratch register -> r13
;   [157:25] line number
    mov r13, 157
    cmovge rbp, r13
;   [157:25] free scratch register 'r13'
    jge panic_bounds
;   [157:30] 0xffee
;   [157:30] 0xffee
;   [157:30] 0xffee
    mov qword [r15 + r14 * 8 + 0], 0xffee
;   [157:5] free scratch register 'r14'
;   [157:5] free scratch register 'r15'
;   [158:5] assert(worlds[1].locations[1] == 0xffee)
;   [158:12] allocate scratch register -> r15
;   [158:12] worlds[1].locations[1] == 0xffee
;   [158:12] ? worlds[1].locations[1] == 0xffee
;   [158:12] ? worlds[1].locations[1] == 0xffee
    cmp_158_12:
;   [158:12] allocate scratch register -> r14
;       [158:12] worlds[1].locations[1]
;       [158:12] worlds[1].locations[1]
;       [158:12] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [158:12] allocate scratch register -> r12
;       [158:19] set array index
;       [158:19] 1
;       [158:19] 1
;       [158:19] 1
        mov r12, 1
;       [158:19] bounds check
        cmp r12, 8
;       [158:19] allocate scratch register -> r11
;       [158:19] line number
        mov r11, 158
        cmovge rbp, r11
;       [158:19] free scratch register 'r11'
        jge panic_bounds
        shl r12, 6
        add r13, r12
;       [158:12] free scratch register 'r12'
;       [158:12] allocate scratch register -> r12
;       [158:32] set array index
;       [158:32] 1
;       [158:32] 1
;       [158:32] 1
        mov r12, 1
;       [158:32] bounds check
        cmp r12, 8
;       [158:32] allocate scratch register -> r11
;       [158:32] line number
        mov r11, 158
        cmovge rbp, r11
;       [158:32] free scratch register 'r11'
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
;       [158:12] free scratch register 'r12'
;       [158:12] free scratch register 'r13'
    cmp r14, 0xffee
;   [158:12] free scratch register 'r14'
    jne bool_false_158_12
    jmp bool_true_158_12
    bool_true_158_12:
    mov r15, true
    jmp bool_end_158_12
    bool_false_158_12:
    mov r15, false
    bool_end_158_12:
;   [18:6] assert(expr : bool) 
    assert_158_5:
;       [158:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_158_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_158_5:
        test r15, r15
        jne if_19_5_158_5_end
        jmp if_19_8_158_5_code
        if_19_8_158_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_158_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_158_5_end:
        if_19_5_158_5_end:
;       [158:5] free scratch register 'r15'
    assert_158_5_end:
;   [160:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds[1].locations) )
;   [160:5] allocate named register 'rsi'
;   [160:5] allocate named register 'rdi'
;   [160:5] allocate named register 'rcx'
;   [162:9] array_size_of(worlds[1].locations)
;   [162:9] array_size_of(worlds[1].locations)
;   [163:9] array_size_of(worlds[1].locations)
;   [163:9] rcx = array_size_of(worlds[1].locations)
;   [163:9] array_size_of(worlds[1].locations)
    mov rcx, 8
;   [161:9] worlds[1].locations
;   [161:9] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [161:9] allocate scratch register -> r14
;   [161:16] set array index
;   [161:16] 1
;   [161:16] 1
;   [161:16] 1
    mov r14, 1
;   [161:16] bounds check
    cmp r14, 8
;   [161:16] allocate scratch register -> r13
;   [161:16] line number
    mov r13, 161
    cmovge rbp, r13
;   [161:16] free scratch register 'r13'
    jge panic_bounds
    shl r14, 6
    add r15, r14
;   [161:9] free scratch register 'r14'
;   [161:9] bounds check
    cmp rcx, 8
;   [161:9] allocate scratch register -> r14
;   [161:9] line number
    mov r14, 161
    cmovg rbp, r14
;   [161:9] free scratch register 'r14'
    jg panic_bounds
    lea rsi, [r15]
;   [160:5] free scratch register 'r15'
;   [162:9] worlds[0].locations
;   [162:9] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [162:9] allocate scratch register -> r14
;   [162:16] set array index
;   [162:16] 0
;   [162:16] 0
;   [162:16] 0
    mov r14, 0
;   [162:16] bounds check
    cmp r14, 8
;   [162:16] allocate scratch register -> r13
;   [162:16] line number
    mov r13, 162
    cmovge rbp, r13
;   [162:16] free scratch register 'r13'
    jge panic_bounds
    shl r14, 6
    add r15, r14
;   [162:9] free scratch register 'r14'
;   [162:9] bounds check
    cmp rcx, 8
;   [162:9] allocate scratch register -> r14
;   [162:9] line number
    mov r14, 162
    cmovg rbp, r14
;   [162:9] free scratch register 'r14'
    jg panic_bounds
    lea rdi, [r15]
;   [160:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [160:5] free named register 'rcx'
;   [160:5] free named register 'rdi'
;   [160:5] free named register 'rsi'
;   [165:5] assert(worlds[0].locations[1] == 0xffee)
;   [165:12] allocate scratch register -> r15
;   [165:12] worlds[0].locations[1] == 0xffee
;   [165:12] ? worlds[0].locations[1] == 0xffee
;   [165:12] ? worlds[0].locations[1] == 0xffee
    cmp_165_12:
;   [165:12] allocate scratch register -> r14
;       [165:12] worlds[0].locations[1]
;       [165:12] worlds[0].locations[1]
;       [165:12] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [165:12] allocate scratch register -> r12
;       [165:19] set array index
;       [165:19] 0
;       [165:19] 0
;       [165:19] 0
        mov r12, 0
;       [165:19] bounds check
        cmp r12, 8
;       [165:19] allocate scratch register -> r11
;       [165:19] line number
        mov r11, 165
        cmovge rbp, r11
;       [165:19] free scratch register 'r11'
        jge panic_bounds
        shl r12, 6
        add r13, r12
;       [165:12] free scratch register 'r12'
;       [165:12] allocate scratch register -> r12
;       [165:32] set array index
;       [165:32] 1
;       [165:32] 1
;       [165:32] 1
        mov r12, 1
;       [165:32] bounds check
        cmp r12, 8
;       [165:32] allocate scratch register -> r11
;       [165:32] line number
        mov r11, 165
        cmovge rbp, r11
;       [165:32] free scratch register 'r11'
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
;       [165:12] free scratch register 'r12'
;       [165:12] free scratch register 'r13'
    cmp r14, 0xffee
;   [165:12] free scratch register 'r14'
    jne bool_false_165_12
    jmp bool_true_165_12
    bool_true_165_12:
    mov r15, true
    jmp bool_end_165_12
    bool_false_165_12:
    mov r15, false
    bool_end_165_12:
;   [18:6] assert(expr : bool) 
    assert_165_5:
;       [165:5] alias expr -> r15  (lea: , len: 0)
        if_19_8_165_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_165_5:
        test r15, r15
        jne if_19_5_165_5_end
        jmp if_19_8_165_5_code
        if_19_8_165_5_code:
;           [19:17] exit(1)
;           [19:22] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_19_17_165_5:
;               [19:17] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [19:17] free named register 'rdi'
            exit_19_17_165_5_end:
        if_19_5_165_5_end:
;       [165:5] free scratch register 'r15'
    assert_165_5_end:
;   [167:5] print(hello.len, hello)
;   [167:11] allocate named register 'rdx'
    mov rdx, hello.len
;   [167:22] allocate named register 'rsi'
    mov rsi, hello
;   [22:6] print(len : reg_rdx, ptr : reg_rsi) 
    print_167_5:
;       [167:5] alias len -> rdx  (lea: , len: 0)
;       [167:5] alias ptr -> rsi  (lea: , len: 0)
;       [23:5] mov(rax, 1)
;           [23:14] 1
;           [23:14] 1
;           [23:14] 1
            mov rax, 1
;       [23:19] # write system call
;       [24:5] mov(rdi, 1)
;           [24:14] 1
;           [24:14] 1
;           [24:14] 1
            mov rdi, 1
;       [24:19] # file descriptor for standard out
;       [25:5] mov(rsi, ptr)
;           [25:14] ptr
;           [25:14] ptr
;           [25:14] ptr
;       [25:19] # buffer address
;       [26:5] mov(rdx, len)
;           [26:14] len
;           [26:14] len
;           [26:14] len
;       [26:19] # buffer size
;       [27:5] syscall()
        syscall
;       [167:5] free named register 'rsi'
;       [167:5] free named register 'rdx'
    print_167_5_end:
;   [168:5] loop
    loop_168_5:
;       [169:9] print(prompt1.len, prompt1)
;       [169:15] allocate named register 'rdx'
        mov rdx, prompt1.len
;       [169:28] allocate named register 'rsi'
        mov rsi, prompt1
;       [22:6] print(len : reg_rdx, ptr : reg_rsi) 
        print_169_9:
;           [169:9] alias len -> rdx  (lea: , len: 0)
;           [169:9] alias ptr -> rsi  (lea: , len: 0)
;           [23:5] mov(rax, 1)
;               [23:14] 1
;               [23:14] 1
;               [23:14] 1
                mov rax, 1
;           [23:19] # write system call
;           [24:5] mov(rdi, 1)
;               [24:14] 1
;               [24:14] 1
;               [24:14] 1
                mov rdi, 1
;           [24:19] # file descriptor for standard out
;           [25:5] mov(rsi, ptr)
;               [25:14] ptr
;               [25:14] ptr
;               [25:14] ptr
;           [25:19] # buffer address
;           [26:5] mov(rdx, len)
;               [26:14] len
;               [26:14] len
;               [26:14] len
;           [26:19] # buffer size
;           [27:5] syscall()
            syscall
;           [169:9] free named register 'rsi'
;           [169:9] free named register 'rdx'
        print_169_9_end:
;       [171:9] var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [171:13] len: i64 @ qword [rsp - 804]
;       [171:13] len =read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [171:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [171:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [171:19] len = read(array_size_of(input_buffer), address_of(input_buffer))
;       [171:19] read(array_size_of(input_buffer), address_of(input_buffer))
;       [171:24] allocate named register 'rdx'
;       [171:24] array_size_of(input_buffer)
;       [171:24] array_size_of(input_buffer)
;       [171:24] rdx = array_size_of(input_buffer)
;       [171:24] array_size_of(input_buffer)
        mov rdx, 80
;       [171:53] allocate named register 'rsi'
;       [171:53] address_of(input_buffer)
;       [171:53] address_of(input_buffer)
;       [171:53] rsi = address_of(input_buffer)
;       [171:53] address_of(input_buffer)
        lea rsi, [rsp - 284]
;       [30:6] read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_171_19:
;           [171:19] alias nbytes -> qword [rsp - 804]  (lea: , len: 0)
;           [171:19] alias len -> rdx  (lea: , len: 0)
;           [171:19] alias ptr -> rsi  (lea: , len: 0)
;           [31:5] mov(rax, 0)
;               [31:14] 0
;               [31:14] 0
;               [31:14] 0
                mov rax, 0
;           [31:19] # read system call
;           [32:5] mov(rdi, 0)
;               [32:14] 0
;               [32:14] 0
;               [32:14] 0
                mov rdi, 0
;           [32:19] # file descriptor for standard input
;           [33:5] mov(rsi, ptr)
;               [33:14] ptr
;               [33:14] ptr
;               [33:14] ptr
;           [33:19] # buffer address
;           [34:5] mov(rdx, len)
;               [34:14] len
;               [34:14] len
;               [34:14] len
;           [34:19] # buffer size
;           [35:5] syscall()
            syscall
;           [36:5] mov(nbytes, rax)
;               [36:17] rax
;               [36:17] rax
;               [36:17] rax
                mov qword [rsp - 804], rax
;           [36:22] # return value
;           [171:19] free named register 'rsi'
;           [171:19] free named register 'rdx'
        read_171_19_end:
;       [171:81] len - 1
        sub qword [rsp - 804], 1
;       [172:9] # note: `array_size_of` and `address_of` are built-in functions
;       [173:9] # -1 to not include the trailing '\n'
        if_175_12:
;       [175:12] ? len == 0
;       [175:12] ? len == 0
        cmp_175_12:
        cmp qword [rsp - 804], 0
        jne if_177_19
        jmp if_175_12_code
        if_175_12_code:
;           [176:13] break
            jmp loop_168_5_end
        jmp if_175_9_end
        if_177_19:
;       [177:19] ? len <= 4
;       [177:19] ? len <= 4
        cmp_177_19:
        cmp qword [rsp - 804], 4
        jg if_else_175_9
        jmp if_177_19_code
        if_177_19_code:
;           [178:13] print(prompt2.len, prompt2)
;           [178:19] allocate named register 'rdx'
            mov rdx, prompt2.len
;           [178:32] allocate named register 'rsi'
            mov rsi, prompt2
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_178_13:
;               [178:13] alias len -> rdx  (lea: , len: 0)
;               [178:13] alias ptr -> rsi  (lea: , len: 0)
;               [23:5] mov(rax, 1)
;                   [23:14] 1
;                   [23:14] 1
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
;                   [24:14] 1
;                   [24:14] 1
;                   [24:14] 1
                    mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;                   [25:14] ptr
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;                   [26:14] len
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [178:13] free named register 'rsi'
;               [178:13] free named register 'rdx'
            print_178_13_end:
;           [179:13] continue
            jmp loop_168_5
        jmp if_175_9_end
        if_else_175_9:
;           [181:13] print(prompt3.len, prompt3)
;           [181:19] allocate named register 'rdx'
            mov rdx, prompt3.len
;           [181:32] allocate named register 'rsi'
            mov rsi, prompt3
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_181_13:
;               [181:13] alias len -> rdx  (lea: , len: 0)
;               [181:13] alias ptr -> rsi  (lea: , len: 0)
;               [23:5] mov(rax, 1)
;                   [23:14] 1
;                   [23:14] 1
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
;                   [24:14] 1
;                   [24:14] 1
;                   [24:14] 1
                    mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;                   [25:14] ptr
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;                   [26:14] len
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [181:13] free named register 'rsi'
;               [181:13] free named register 'rdx'
            print_181_13_end:
;           [182:13] print(len, address_of(input_buffer))
;           [182:19] allocate named register 'rdx'
            mov rdx, qword [rsp - 804]
;           [182:24] allocate named register 'rsi'
;           [182:24] address_of(input_buffer)
;           [182:24] address_of(input_buffer)
;           [182:24] rsi = address_of(input_buffer)
;           [182:24] address_of(input_buffer)
            lea rsi, [rsp - 284]
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_182_13:
;               [182:13] alias len -> rdx  (lea: , len: 0)
;               [182:13] alias ptr -> rsi  (lea: , len: 0)
;               [23:5] mov(rax, 1)
;                   [23:14] 1
;                   [23:14] 1
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
;                   [24:14] 1
;                   [24:14] 1
;                   [24:14] 1
                    mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;                   [25:14] ptr
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;                   [26:14] len
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [182:13] free named register 'rsi'
;               [182:13] free named register 'rdx'
            print_182_13_end:
;           [183:13] print(dot.len, dot)
;           [183:19] allocate named register 'rdx'
            mov rdx, dot.len
;           [183:28] allocate named register 'rsi'
            mov rsi, dot
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_183_13:
;               [183:13] alias len -> rdx  (lea: , len: 0)
;               [183:13] alias ptr -> rsi  (lea: , len: 0)
;               [23:5] mov(rax, 1)
;                   [23:14] 1
;                   [23:14] 1
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
;                   [24:14] 1
;                   [24:14] 1
;                   [24:14] 1
                    mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;                   [25:14] ptr
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;                   [26:14] len
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [183:13] free named register 'rsi'
;               [183:13] free named register 'rdx'
            print_183_13_end:
;           [184:13] print(nl.len, nl)
;           [184:19] allocate named register 'rdx'
            mov rdx, nl.len
;           [184:27] allocate named register 'rsi'
            mov rsi, nl
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_184_13:
;               [184:13] alias len -> rdx  (lea: , len: 0)
;               [184:13] alias ptr -> rsi  (lea: , len: 0)
;               [23:5] mov(rax, 1)
;                   [23:14] 1
;                   [23:14] 1
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
;                   [24:14] 1
;                   [24:14] 1
;                   [24:14] 1
                    mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;                   [25:14] ptr
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;                   [26:14] len
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [184:13] free named register 'rsi'
;               [184:13] free named register 'rdx'
            print_184_13_end:
        if_175_9_end:
    jmp loop_168_5
    loop_168_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

panic_bounds:
;   print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
;   line number is in `rbp`
    mov rax, rbp
;   convert to string
    lea rdi, [num_buffer + 19]
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
;   print line number to stderr
    mov rax, 1
    mov rsi, rdi
    lea rdx, [num_buffer + 20]
    sub rdx, rdi
    mov rdi, 2
    syscall
;   exit with error code 255
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
    msg_panic: db 'panic: bounds at line '
    msg_panic_len equ $ - msg_panic
section .bss
    num_buffer: resb 21

; max scratch registers in use: 5
;            max frames in use: 7
;               max stack size: 804 B
```
