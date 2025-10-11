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
* inlined functions
* keywords: `func`, `type`, `field`, `var`, `loop`, `if`, `else`, `continue`,
`break`, `return`
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`, `address_of`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
run `prog.baz`
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    41           1075            653           5174
C++                              1             65             75            435
-------------------------------------------------------------------------------
SUM:                            42           1140            728           5609
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

    var arr1 : i32[4]
    array_copy(arr, arr1, 4)
    assert(arrays_equal(arr, arr1, 4))

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

    ix = 3
    arr[ix] = ~inv(arr[ix - 1])
    assert(arr[ix] == 2)

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
section .bss
stk resd 0x10000
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $-hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
nl: db '', 10,''
nl.len equ $-nl
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
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, qword [rsp - 24]
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_79_12:
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_79_12
    bool_true_79_12:
    mov r15, true
    jmp bool_end_79_12
    bool_false_79_12:
    mov r15, false
    bool_end_79_12:
    assert_79_5:
        if_19_8_79_5:
        cmp_19_8_79_5:
        cmp r15, false
        jne if_19_5_79_5_end
        if_19_8_79_5_code:
            mov rdi, 1
            exit_19_17_79_5:
                mov rax, 60
                syscall
            exit_19_17_79_5_end:
        if_19_5_79_5_end:
    assert_79_5_end:
    cmp_80_12:
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_80_12
    bool_true_80_12:
    mov r15, true
    jmp bool_end_80_12
    bool_false_80_12:
    mov r15, false
    bool_end_80_12:
    assert_80_5:
        if_19_8_80_5:
        cmp_19_8_80_5:
        cmp r15, false
        jne if_19_5_80_5_end
        if_19_8_80_5_code:
            mov rdi, 1
            exit_19_17_80_5:
                mov rax, 60
                syscall
            exit_19_17_80_5_end:
        if_19_5_80_5_end:
    assert_80_5_end:
    mov r15, 2
    lea rsi, [rsp + r15 * 4 - 16]
    lea rdi, [rsp - 16]
    mov rcx, 2
    shl rcx, 2
    rep movsb
    cmp_84_12:
        mov r13, 0
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
    assert_84_5:
        if_19_8_84_5:
        cmp_19_8_84_5:
        cmp r15, false
        jne if_19_5_84_5_end
        if_19_8_84_5_code:
            mov rdi, 1
            exit_19_17_84_5:
                mov rax, 60
                syscall
            exit_19_17_84_5_end:
        if_19_5_84_5_end:
    assert_84_5_end:
    lea rdi, [rsp - 40]
    mov rcx, 16
    xor rax, rax
    rep stosb
    lea rsi, [rsp - 16]
    lea rdi, [rsp - 40]
    mov rcx, 4
    shl rcx, 2
    rep movsb
    cmp_88_12:
        lea rsi, [rsp - 16]
        lea rdi, [rsp - 40]
        mov rcx, 4
        shl rcx, 2
        repe cmpsb
        je cmps_eq_88_12
        mov r14, false
        jmp cmps_end_88_12
        cmps_eq_88_12:
        mov r14, true
        cmps_end_88_12:
    cmp r14, false
    je bool_false_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
    assert_88_5:
        if_19_8_88_5:
        cmp_19_8_88_5:
        cmp r15, false
        jne if_19_5_88_5_end
        if_19_8_88_5_code:
            mov rdi, 1
            exit_19_17_88_5:
                mov rax, 60
                syscall
            exit_19_17_88_5_end:
        if_19_5_88_5_end:
    assert_88_5_end:
    mov r15, 2
    mov dword [rsp + r15 * 4 - 40], -1
    cmp_91_12:
        lea rsi, [rsp - 16]
        lea rdi, [rsp - 40]
        mov rcx, 4
        shl rcx, 2
        repe cmpsb
        je cmps_eq_91_16
        mov r14, false
        jmp cmps_end_91_16
        cmps_eq_91_16:
        mov r14, true
        cmps_end_91_16:
    cmp r14, false
    jne bool_false_91_12
    bool_true_91_12:
    mov r15, true
    jmp bool_end_91_12
    bool_false_91_12:
    mov r15, false
    bool_end_91_12:
    assert_91_5:
        if_19_8_91_5:
        cmp_19_8_91_5:
        cmp r15, false
        jne if_19_5_91_5_end
        if_19_8_91_5_code:
            mov rdi, 1
            exit_19_17_91_5:
                mov rax, 60
                syscall
            exit_19_17_91_5_end:
        if_19_5_91_5_end:
    assert_91_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_94_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_94_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_95_12:
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_95_12
    bool_true_95_12:
    mov r15, true
    jmp bool_end_95_12
    bool_false_95_12:
    mov r15, false
    bool_end_95_12:
    assert_95_5:
        if_19_8_95_5:
        cmp_19_8_95_5:
        cmp r15, false
        jne if_19_5_95_5_end
        if_19_8_95_5_code:
            mov rdi, 1
            exit_19_17_95_5:
                mov rax, 60
                syscall
            exit_19_17_95_5_end:
        if_19_5_95_5_end:
    assert_95_5_end:
    mov qword [rsp - 56], 0
    mov qword [rsp - 48], 0
    foo_98_5:
        mov qword [rsp - 56], 0b10
        mov qword [rsp - 48], 0xb
    foo_98_5_end:
    cmp_99_12:
    cmp qword [rsp - 56], 2
    jne bool_false_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
    assert_99_5:
        if_19_8_99_5:
        cmp_19_8_99_5:
        cmp r15, false
        jne if_19_5_99_5_end
        if_19_8_99_5_code:
            mov rdi, 1
            exit_19_17_99_5:
                mov rax, 60
                syscall
            exit_19_17_99_5_end:
        if_19_5_99_5_end:
    assert_99_5_end:
    cmp_100_12:
    cmp qword [rsp - 48], 0xb
    jne bool_false_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
    assert_100_5:
        if_19_8_100_5:
        cmp_19_8_100_5:
        cmp r15, false
        jne if_19_5_100_5_end
        if_19_8_100_5_code:
            mov rdi, 1
            exit_19_17_100_5:
                mov rax, 60
                syscall
            exit_19_17_100_5_end:
        if_19_5_100_5_end:
    assert_100_5_end:
    mov qword [rsp - 64], 0
    bar_103_5:
        if_57_8_103_5:
        cmp_57_8_103_5:
        cmp qword [rsp - 64], 0
        jne if_57_5_103_5_end
        if_57_8_103_5_code:
            jmp bar_103_5_end
        if_57_5_103_5_end:
        mov qword [rsp - 64], 0xff
    bar_103_5_end:
    cmp_104_12:
    cmp qword [rsp - 64], 0
    jne bool_false_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_19_8_104_5:
        cmp_19_8_104_5:
        cmp r15, false
        jne if_19_5_104_5_end
        if_19_8_104_5_code:
            mov rdi, 1
            exit_19_17_104_5:
                mov rax, 60
                syscall
            exit_19_17_104_5_end:
        if_19_5_104_5_end:
    assert_104_5_end:
    mov qword [rsp - 64], 1
    bar_107_5:
        if_57_8_107_5:
        cmp_57_8_107_5:
        cmp qword [rsp - 64], 0
        jne if_57_5_107_5_end
        if_57_8_107_5_code:
            jmp bar_107_5_end
        if_57_5_107_5_end:
        mov qword [rsp - 64], 0xff
    bar_107_5_end:
    cmp_108_12:
    cmp qword [rsp - 64], 0xff
    jne bool_false_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
    assert_108_5:
        if_19_8_108_5:
        cmp_19_8_108_5:
        cmp r15, false
        jne if_19_5_108_5_end
        if_19_8_108_5_code:
            mov rdi, 1
            exit_19_17_108_5:
                mov rax, 60
                syscall
            exit_19_17_108_5_end:
        if_19_5_108_5_end:
    assert_108_5_end:
    mov qword [rsp - 72], 1
    baz_111_13:
        mov r15, qword [rsp - 72]
        mov qword [rsp - 80], r15
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_111_13_end:
    cmp_112_12:
    cmp qword [rsp - 80], 2
    jne bool_false_112_12
    bool_true_112_12:
    mov r15, true
    jmp bool_end_112_12
    bool_false_112_12:
    mov r15, false
    bool_end_112_12:
    assert_112_5:
        if_19_8_112_5:
        cmp_19_8_112_5:
        cmp r15, false
        jne if_19_5_112_5_end
        if_19_8_112_5_code:
            mov rdi, 1
            exit_19_17_112_5:
                mov rax, 60
                syscall
            exit_19_17_112_5_end:
        if_19_5_112_5_end:
    assert_112_5_end:
    baz_114_9:
        mov qword [rsp - 80], 1
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_114_9_end:
    cmp_115_12:
    cmp qword [rsp - 80], 2
    jne bool_false_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_19_8_115_5:
        cmp_19_8_115_5:
        cmp r15, false
        jne if_19_5_115_5_end
        if_19_8_115_5_code:
            mov rdi, 1
            exit_19_17_115_5:
                mov rax, 60
                syscall
            exit_19_17_115_5_end:
        if_19_5_115_5_end:
    assert_115_5_end:
    baz_117_23:
        mov qword [rsp - 96], 2
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
    baz_117_23_end:
    mov qword [rsp - 88], 0
    cmp_118_12:
    cmp qword [rsp - 96], 4
    jne bool_false_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
    assert_118_5:
        if_19_8_118_5:
        cmp_19_8_118_5:
        cmp r15, false
        jne if_19_5_118_5_end
        if_19_8_118_5_code:
            mov rdi, 1
            exit_19_17_118_5:
                mov rax, 60
                syscall
            exit_19_17_118_5_end:
        if_19_5_118_5_end:
    assert_118_5_end:
    mov qword [rsp - 104], 1
    mov qword [rsp - 112], 2
        mov r15, qword [rsp - 104]
        mov qword [rsp - 132], r15
        mov r15, qword [rsp - 132]
        imul r15, 10
        mov qword [rsp - 132], r15
        mov r15, qword [rsp - 112]
        mov qword [rsp - 124], r15
    mov dword [rsp - 116], 0xff0000
    cmp_124_12:
    cmp qword [rsp - 132], 10
    jne bool_false_124_12
    bool_true_124_12:
    mov r15, true
    jmp bool_end_124_12
    bool_false_124_12:
    mov r15, false
    bool_end_124_12:
    assert_124_5:
        if_19_8_124_5:
        cmp_19_8_124_5:
        cmp r15, false
        jne if_19_5_124_5_end
        if_19_8_124_5_code:
            mov rdi, 1
            exit_19_17_124_5:
                mov rax, 60
                syscall
            exit_19_17_124_5_end:
        if_19_5_124_5_end:
    assert_124_5_end:
    cmp_125_12:
    cmp qword [rsp - 124], 2
    jne bool_false_125_12
    bool_true_125_12:
    mov r15, true
    jmp bool_end_125_12
    bool_false_125_12:
    mov r15, false
    bool_end_125_12:
    assert_125_5:
        if_19_8_125_5:
        cmp_19_8_125_5:
        cmp r15, false
        jne if_19_5_125_5_end
        if_19_8_125_5_code:
            mov rdi, 1
            exit_19_17_125_5:
                mov rax, 60
                syscall
            exit_19_17_125_5_end:
        if_19_5_125_5_end:
    assert_125_5_end:
    cmp_126_12:
    cmp dword [rsp - 116], 0xff0000
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_19_8_126_5:
        cmp_19_8_126_5:
        cmp r15, false
        jne if_19_5_126_5_end
        if_19_8_126_5_code:
            mov rdi, 1
            exit_19_17_126_5:
                mov rax, 60
                syscall
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
    assert_126_5_end:
    mov r15, qword [rsp - 104]
    mov qword [rsp - 148], r15
    neg qword [rsp - 148]
    mov r15, qword [rsp - 112]
    mov qword [rsp - 140], r15
    neg qword [rsp - 140]
    lea rdi, [rsp - 132]
    lea rsi, [rsp - 148]
    mov rcx, 16
    rep movsb
    cmp_130_12:
    cmp qword [rsp - 132], -1
    jne bool_false_130_12
    bool_true_130_12:
    mov r15, true
    jmp bool_end_130_12
    bool_false_130_12:
    mov r15, false
    bool_end_130_12:
    assert_130_5:
        if_19_8_130_5:
        cmp_19_8_130_5:
        cmp r15, false
        jne if_19_5_130_5_end
        if_19_8_130_5_code:
            mov rdi, 1
            exit_19_17_130_5:
                mov rax, 60
                syscall
            exit_19_17_130_5_end:
        if_19_5_130_5_end:
    assert_130_5_end:
    cmp_131_12:
    cmp qword [rsp - 124], -2
    jne bool_false_131_12
    bool_true_131_12:
    mov r15, true
    jmp bool_end_131_12
    bool_false_131_12:
    mov r15, false
    bool_end_131_12:
    assert_131_5:
        if_19_8_131_5:
        cmp_19_8_131_5:
        cmp r15, false
        jne if_19_5_131_5_end
        if_19_8_131_5_code:
            mov rdi, 1
            exit_19_17_131_5:
                mov rax, 60
                syscall
            exit_19_17_131_5_end:
        if_19_5_131_5_end:
    assert_131_5_end:
    lea rdi, [rsp - 168]
    lea rsi, [rsp - 132]
    mov rcx, 20
    rep movsb
    cmp_134_12:
    cmp qword [rsp - 168], -1
    jne bool_false_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
    assert_134_5:
        if_19_8_134_5:
        cmp_19_8_134_5:
        cmp r15, false
        jne if_19_5_134_5_end
        if_19_8_134_5_code:
            mov rdi, 1
            exit_19_17_134_5:
                mov rax, 60
                syscall
            exit_19_17_134_5_end:
        if_19_5_134_5_end:
    assert_134_5_end:
    cmp_135_12:
    cmp qword [rsp - 160], -2
    jne bool_false_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
    assert_135_5:
        if_19_8_135_5:
        cmp_19_8_135_5:
        cmp r15, false
        jne if_19_5_135_5_end
        if_19_8_135_5_code:
            mov rdi, 1
            exit_19_17_135_5:
                mov rax, 60
                syscall
            exit_19_17_135_5_end:
        if_19_5_135_5_end:
    assert_135_5_end:
    cmp_136_12:
    cmp dword [rsp - 152], 0xff0000
    jne bool_false_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_19_8_136_5:
        cmp_19_8_136_5:
        cmp r15, false
        jne if_19_5_136_5_end
        if_19_8_136_5_code:
            mov rdi, 1
            exit_19_17_136_5:
                mov rax, 60
                syscall
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
    assert_136_5_end:
    lea rdi, [rsp - 188]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 180], 73
    cmp_141_12:
        lea r13, [rsp - 188]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_141_12
    bool_true_141_12:
    mov r15, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15, false
    bool_end_141_12:
    assert_141_5:
        if_19_8_141_5:
        cmp_19_8_141_5:
        cmp r15, false
        jne if_19_5_141_5_end
        if_19_8_141_5_code:
            mov rdi, 1
            exit_19_17_141_5:
                mov rax, 60
                syscall
            exit_19_17_141_5_end:
        if_19_5_141_5_end:
    assert_141_5_end:
    lea rdi, [rsp - 268]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_145_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_145_5_end:
    loop_146_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_147_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_147_9_end:
        mov rdx, 80
        lea rsi, [rsp - 268]
        read_149_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 276], rax
        read_149_19_end:
        sub qword [rsp - 276], 1
        if_153_12:
        cmp_153_12:
        cmp qword [rsp - 276], 0
        jne if_155_19
        if_153_12_code:
            jmp loop_146_5_end
        jmp if_153_9_end
        if_155_19:
        cmp_155_19:
        cmp qword [rsp - 276], 4
        jg if_else_153_9
        if_155_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_156_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_156_13_end:
            jmp loop_146_5
        jmp if_153_9_end
        if_else_153_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_159_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_159_13_end:
            mov rdx, qword [rsp - 276]
            lea rsi, [rsp - 268]
            print_160_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_160_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_161_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_161_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_162_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_162_13_end:
        if_153_9_end:
    jmp loop_146_5
    loop_146_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
```

## With comments

```nasm

; generated by baz

section .bss
stk resd 0x10000
stk.end:

true equ 1
false equ 0

section .data
;[1:1] field hello = "hello world from baz\n"
hello: db 'hello world from baz', 10,''
hello.len equ $-hello
;[2:1] field prompt1 = "enter name:\n"
prompt1: db 'enter name:', 10,''
prompt1.len equ $-prompt1
;[3:1] field prompt2 = "that is not a name.\n"
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $-prompt2
;[4:1] field prompt3 = "hello "
prompt3: db 'hello '
prompt3.len equ $-prompt3
;[5:1] field dot = "."
dot: db '.'
dot.len equ $-dot
;[6:1] field nl = "\n"
nl: db '', 10,''
nl.len equ $-nl

section .text
bits 64
global _start
_start:
mov rsp,stk.end
; program
;[8:1] # all functions are inlined
;[10:1] # arguments can be placed in specified register using `reg_...` syntax
;[39:1] # user types are defined using keyword `type`
;[41:1] # default type is `i64` and does not need to be specified
;[47:1] # function arguments are equivalent to mutable references
;[54:1] # default argument type is `i64`
;[61:1] # return target is specified as a variable, in this case `res`
main:
;   var arr: i32[4] @ dword [rsp - 16]
;   [72:5] var arr : i32[4]
;   clear array 4 * 4 B = 16 B
;   allocate named register 'rdi'
;   allocate named register 'rcx'
;   allocate named register 'rax'
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
;   free named register 'rax'
;   free named register 'rcx'
;   free named register 'rdi'
;   [73:5] # arrays are initialized to 0
;   var ix: i64 @ qword [rsp - 24]
;   [75:5] var ix = 1
;   [75:14] ix =1
;   [75:14] 1
;   [75:14] 1
;   [77:5] ix = 1
    mov qword [rsp - 24], 1
;   [77:8] arr[ix] = 2
;   allocate scratch register -> r15
;   [77:9] ix
;   [77:9] ix
;   [77:11] r15 = ix
    mov r15, qword [rsp - 24]
;   [77:15] 2
;   [77:15] 2
;   [78:5] dword [rsp + r15 * 4 - 16] = 2
    mov dword [rsp + r15 * 4 - 16], 2
;   free scratch register 'r15'
;   [78:8] arr[ix + 1] = arr[ix]
;   allocate scratch register -> r15
;   [78:9] ix + 1
;   [78:9] ix + 1
;   [78:12] r15 = ix
    mov r15, qword [rsp - 24]
;   [78:15] r15 + 1
    add r15, 1
;   [78:19] arr[ix]
;   [78:19] arr[ix]
;   [78:22] dword [rsp + r15 * 4 - 16] = arr[ix]
;   [78:22] arr[ix]
;   allocate scratch register -> r14
;   [78:23] ix
;   [78:23] ix
;   [78:25] r14 = ix
    mov r14, qword [rsp - 24]
;   allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
;   free scratch register 'r13'
;   free scratch register 'r14'
;   free scratch register 'r15'
;   [79:5] assert(arr[1] == 2)
;   allocate scratch register -> r15
;   [79:12] arr[1] == 2
;   [79:12] ? arr[1] == 2
;   [79:12] ? arr[1] == 2
    cmp_79_12:
;   allocate scratch register -> r14
;       [79:12] arr[1]
;       [79:15] r14 = arr[1]
;       [79:15] arr[1]
;       allocate scratch register -> r13
;       [79:16] 1
;       [79:16] 1
;       [79:17] r13 = 1
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
;       free scratch register 'r13'
    cmp r14, 2
;   free scratch register 'r14'
    jne bool_false_79_12
    bool_true_79_12:
    mov r15, true
    jmp bool_end_79_12
    bool_false_79_12:
    mov r15, false
    bool_end_79_12:
;   assert(expr : bool) 
    assert_79_5:
;       alias expr -> r15
        if_19_8_79_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_79_5:
        cmp r15, false
        jne if_19_5_79_5_end
        if_19_8_79_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_79_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_79_5_end:
        if_19_5_79_5_end:
;       free scratch register 'r15'
    assert_79_5_end:
;   [80:5] assert(arr[2] == 2)
;   allocate scratch register -> r15
;   [80:12] arr[2] == 2
;   [80:12] ? arr[2] == 2
;   [80:12] ? arr[2] == 2
    cmp_80_12:
;   allocate scratch register -> r14
;       [80:12] arr[2]
;       [80:15] r14 = arr[2]
;       [80:15] arr[2]
;       allocate scratch register -> r13
;       [80:16] 2
;       [80:16] 2
;       [80:17] r13 = 2
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
;       free scratch register 'r13'
    cmp r14, 2
;   free scratch register 'r14'
    jne bool_false_80_12
    bool_true_80_12:
    mov r15, true
    jmp bool_end_80_12
    bool_false_80_12:
    mov r15, false
    bool_end_80_12:
;   assert(expr : bool) 
    assert_80_5:
;       alias expr -> r15
        if_19_8_80_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_80_5:
        cmp r15, false
        jne if_19_5_80_5_end
        if_19_8_80_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_80_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_80_5_end:
        if_19_5_80_5_end:
;       free scratch register 'r15'
    assert_80_5_end:
;   [82:5] array_copy(arr[2], arr, 2)
;   allocate named register 'rsi'
;   allocate named register 'rdi'
;   allocate named register 'rcx'
;   allocate scratch register -> r15
;   [82:20] 2
;   [82:20] 2
;   [82:21] r15 = 2
    mov r15, 2
    lea rsi, [rsp + r15 * 4 - 16]
;   free scratch register 'r15'
    lea rdi, [rsp - 16]
;   [82:29] 2
;   [82:29] 2
;   [82:30] rcx = 2
    mov rcx, 2
    shl rcx, 2
    rep movsb
;   free named register 'rcx'
;   free named register 'rdi'
;   free named register 'rsi'
;   [83:5] # copy from, to, number of elements
;   [84:5] assert(arr[0] == 2)
;   allocate scratch register -> r15
;   [84:12] arr[0] == 2
;   [84:12] ? arr[0] == 2
;   [84:12] ? arr[0] == 2
    cmp_84_12:
;   allocate scratch register -> r14
;       [84:12] arr[0]
;       [84:15] r14 = arr[0]
;       [84:15] arr[0]
;       allocate scratch register -> r13
;       [84:16] 0
;       [84:16] 0
;       [84:17] r13 = 0
        mov r13, 0
        movsx r14, dword [rsp + r13 * 4 - 16]
;       free scratch register 'r13'
    cmp r14, 2
;   free scratch register 'r14'
    jne bool_false_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
;   assert(expr : bool) 
    assert_84_5:
;       alias expr -> r15
        if_19_8_84_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_84_5:
        cmp r15, false
        jne if_19_5_84_5_end
        if_19_8_84_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_84_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_84_5_end:
        if_19_5_84_5_end:
;       free scratch register 'r15'
    assert_84_5_end:
;   var arr1: i32[4] @ dword [rsp - 40]
;   [86:5] var arr1 : i32[4]
;   clear array 4 * 4 B = 16 B
;   allocate named register 'rdi'
;   allocate named register 'rcx'
;   allocate named register 'rax'
    lea rdi, [rsp - 40]
    mov rcx, 16
    xor rax, rax
    rep stosb
;   free named register 'rax'
;   free named register 'rcx'
;   free named register 'rdi'
;   [87:5] array_copy(arr, arr1, 4)
;   allocate named register 'rsi'
;   allocate named register 'rdi'
;   allocate named register 'rcx'
    lea rsi, [rsp - 16]
    lea rdi, [rsp - 40]
;   [87:27] 4
;   [87:27] 4
;   [87:28] rcx = 4
    mov rcx, 4
    shl rcx, 2
    rep movsb
;   free named register 'rcx'
;   free named register 'rdi'
;   free named register 'rsi'
;   [88:5] assert(arrays_equal(arr, arr1, 4))
;   allocate scratch register -> r15
;   [88:12] arrays_equal(arr, arr1, 4)
;   [88:12] ? arrays_equal(arr, arr1, 4)
;   [88:12] ? arrays_equal(arr, arr1, 4)
    cmp_88_12:
;   allocate scratch register -> r14
;       [88:12] arrays_equal(arr, arr1, 4)
;       [88:12] r14 = arrays_equal(arr, arr1, 4)
;       [88:12] arrays_equal(arr, arr1, 4)
;       allocate named register 'rsi'
;       allocate named register 'rdi'
;       allocate named register 'rcx'
        lea rsi, [rsp - 16]
        lea rdi, [rsp - 40]
;       [88:36] 4
;       [88:36] 4
;       [88:37] rcx = 4
        mov rcx, 4
        shl rcx, 2
        repe cmpsb
;       free named register 'rcx'
;       free named register 'rdi'
;       free named register 'rsi'
        je cmps_eq_88_12
        mov r14, false
        jmp cmps_end_88_12
        cmps_eq_88_12:
        mov r14, true
        cmps_end_88_12:
    cmp r14, false
;   free scratch register 'r14'
    je bool_false_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
;   assert(expr : bool) 
    assert_88_5:
;       alias expr -> r15
        if_19_8_88_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_88_5:
        cmp r15, false
        jne if_19_5_88_5_end
        if_19_8_88_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_88_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_88_5_end:
        if_19_5_88_5_end:
;       free scratch register 'r15'
    assert_88_5_end:
;   [90:9] arr1[2] = -1
;   allocate scratch register -> r15
;   [90:10] 2
;   [90:10] 2
;   [90:11] r15 = 2
    mov r15, 2
;   [90:15] -1
;   [90:15] -1
;   [91:5] dword [rsp + r15 * 4 - 40] = -1
    mov dword [rsp + r15 * 4 - 40], -1
;   free scratch register 'r15'
;   [91:5] assert(not arrays_equal(arr, arr1, 4))
;   allocate scratch register -> r15
;   [91:12] not arrays_equal(arr, arr1, 4)
;   [91:12] ? not arrays_equal(arr, arr1, 4)
;   [91:12] ? not arrays_equal(arr, arr1, 4)
    cmp_91_12:
;   allocate scratch register -> r14
;       [91:16] arrays_equal(arr, arr1, 4)
;       [91:16] r14 = arrays_equal(arr, arr1, 4)
;       [91:16] arrays_equal(arr, arr1, 4)
;       allocate named register 'rsi'
;       allocate named register 'rdi'
;       allocate named register 'rcx'
        lea rsi, [rsp - 16]
        lea rdi, [rsp - 40]
;       [91:40] 4
;       [91:40] 4
;       [91:41] rcx = 4
        mov rcx, 4
        shl rcx, 2
        repe cmpsb
;       free named register 'rcx'
;       free named register 'rdi'
;       free named register 'rsi'
        je cmps_eq_91_16
        mov r14, false
        jmp cmps_end_91_16
        cmps_eq_91_16:
        mov r14, true
        cmps_end_91_16:
    cmp r14, false
;   free scratch register 'r14'
    jne bool_false_91_12
    bool_true_91_12:
    mov r15, true
    jmp bool_end_91_12
    bool_false_91_12:
    mov r15, false
    bool_end_91_12:
;   assert(expr : bool) 
    assert_91_5:
;       alias expr -> r15
        if_19_8_91_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_91_5:
        cmp r15, false
        jne if_19_5_91_5_end
        if_19_8_91_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_91_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_91_5_end:
        if_19_5_91_5_end:
;       free scratch register 'r15'
    assert_91_5_end:
;   [93:8] ix = 3
;   [93:10] 3
;   [93:10] 3
;   [94:5] ix = 3
    mov qword [rsp - 24], 3
;   [94:8] arr[ix] = ~inv(arr[ix - 1])
;   allocate scratch register -> r15
;   [94:9] ix
;   [94:9] ix
;   [94:11] r15 = ix
    mov r15, qword [rsp - 24]
;   [94:15] ~inv(arr[ix - 1])
;   [94:15] ~inv(arr[ix - 1])
;   [94:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;   [94:16] ~inv(arr[ix - 1])
;   allocate scratch register -> r14
;   [94:20] arr[ix - 1]
;   [94:20] arr[ix - 1]
;   [94:23] r14 = arr[ix - 1]
;   [94:23] arr[ix - 1]
;   allocate scratch register -> r13
;   [94:24] ix - 1
;   [94:24] ix - 1
;   [94:27] r13 = ix
    mov r13, qword [rsp - 24]
;   [94:30] r13 - 1
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
;   free scratch register 'r13'
;   inv(i : i32) : i32 res 
    inv_94_16:
;       alias res -> dword [rsp + r15 * 4 - 16]
;       alias i -> r14
;       [64:9] res = ~i
;       [64:11] ~i
;       [64:11] ~i
;       [65:1] res = ~i
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
;       free scratch register 'r14'
    inv_94_16_end:
    not dword [rsp + r15 * 4 - 16]
;   free scratch register 'r15'
;   [95:5] assert(arr[ix] == 2)
;   allocate scratch register -> r15
;   [95:12] arr[ix] == 2
;   [95:12] ? arr[ix] == 2
;   [95:12] ? arr[ix] == 2
    cmp_95_12:
;   allocate scratch register -> r14
;       [95:12] arr[ix]
;       [95:15] r14 = arr[ix]
;       [95:15] arr[ix]
;       allocate scratch register -> r13
;       [95:16] ix
;       [95:16] ix
;       [95:18] r13 = ix
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
;       free scratch register 'r13'
    cmp r14, 2
;   free scratch register 'r14'
    jne bool_false_95_12
    bool_true_95_12:
    mov r15, true
    jmp bool_end_95_12
    bool_false_95_12:
    mov r15, false
    bool_end_95_12:
;   assert(expr : bool) 
    assert_95_5:
;       alias expr -> r15
        if_19_8_95_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_95_5:
        cmp r15, false
        jne if_19_5_95_5_end
        if_19_8_95_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_95_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_95_5_end:
        if_19_5_95_5_end:
;       free scratch register 'r15'
    assert_95_5_end:
;   var p: point @ qword [rsp - 56]
;   [97:5] var p : point = {0, 0}
;   [97:21] p ={0, 0}
;   [97:21] {0, 0}
;   [97:21] {0, 0}
;   [97:22] 0
;   [97:22] 0
;   [97:23] p.x = 0
    mov qword [rsp - 56], 0
;   [97:25] 0
;   [97:25] 0
;   [97:26] p.y = 0
    mov qword [rsp - 48], 0
;   [98:5] foo(p)
;   foo(pt : point) 
    foo_98_5:
;       alias pt -> p
;       [50:7] pt.x = 0b10
;       [50:12] 0b10
;       [50:12] 0b10
;       [50:20] pt.x = 0b10
        mov qword [rsp - 56], 0b10
;       [50:20] # binary value 2
;       [51:7] pt.y = 0xb
;       [51:12] 0xb
;       [51:12] 0xb
;       [51:20] pt.y = 0xb
        mov qword [rsp - 48], 0xb
;       [51:20] # hex value 11
    foo_98_5_end:
;   [99:5] assert(p.x == 2)
;   allocate scratch register -> r15
;   [99:12] p.x == 2
;   [99:12] ? p.x == 2
;   [99:12] ? p.x == 2
    cmp_99_12:
    cmp qword [rsp - 56], 2
    jne bool_false_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
;   assert(expr : bool) 
    assert_99_5:
;       alias expr -> r15
        if_19_8_99_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_99_5:
        cmp r15, false
        jne if_19_5_99_5_end
        if_19_8_99_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_99_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_99_5_end:
        if_19_5_99_5_end:
;       free scratch register 'r15'
    assert_99_5_end:
;   [100:5] assert(p.y == 0xb)
;   allocate scratch register -> r15
;   [100:12] p.y == 0xb
;   [100:12] ? p.y == 0xb
;   [100:12] ? p.y == 0xb
    cmp_100_12:
    cmp qword [rsp - 48], 0xb
    jne bool_false_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
;   assert(expr : bool) 
    assert_100_5:
;       alias expr -> r15
        if_19_8_100_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_100_5:
        cmp r15, false
        jne if_19_5_100_5_end
        if_19_8_100_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_100_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_100_5_end:
        if_19_5_100_5_end:
;       free scratch register 'r15'
    assert_100_5_end:
;   var i: i64 @ qword [rsp - 64]
;   [102:5] var i = 0
;   [102:13] i =0
;   [102:13] 0
;   [102:13] 0
;   [103:5] i = 0
    mov qword [rsp - 64], 0
;   [103:5] bar(i)
;   bar(arg) 
    bar_103_5:
;       alias arg -> i
        if_57_8_103_5:
;       [57:8] ? arg == 0
;       [57:8] ? arg == 0
        cmp_57_8_103_5:
        cmp qword [rsp - 64], 0
        jne if_57_5_103_5_end
        if_57_8_103_5_code:
;           [57:17] return
            jmp bar_103_5_end
        if_57_5_103_5_end:
;       [58:9] arg = 0xff
;       [58:11] 0xff
;       [58:11] 0xff
;       [59:1] arg = 0xff
        mov qword [rsp - 64], 0xff
    bar_103_5_end:
;   [104:5] assert(i == 0)
;   allocate scratch register -> r15
;   [104:12] i == 0
;   [104:12] ? i == 0
;   [104:12] ? i == 0
    cmp_104_12:
    cmp qword [rsp - 64], 0
    jne bool_false_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
;   assert(expr : bool) 
    assert_104_5:
;       alias expr -> r15
        if_19_8_104_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_104_5:
        cmp r15, false
        jne if_19_5_104_5_end
        if_19_8_104_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_104_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_104_5_end:
        if_19_5_104_5_end:
;       free scratch register 'r15'
    assert_104_5_end:
;   [106:7] i = 1
;   [106:9] 1
;   [106:9] 1
;   [107:5] i = 1
    mov qword [rsp - 64], 1
;   [107:5] bar(i)
;   bar(arg) 
    bar_107_5:
;       alias arg -> i
        if_57_8_107_5:
;       [57:8] ? arg == 0
;       [57:8] ? arg == 0
        cmp_57_8_107_5:
        cmp qword [rsp - 64], 0
        jne if_57_5_107_5_end
        if_57_8_107_5_code:
;           [57:17] return
            jmp bar_107_5_end
        if_57_5_107_5_end:
;       [58:9] arg = 0xff
;       [58:11] 0xff
;       [58:11] 0xff
;       [59:1] arg = 0xff
        mov qword [rsp - 64], 0xff
    bar_107_5_end:
;   [108:5] assert(i == 0xff)
;   allocate scratch register -> r15
;   [108:12] i == 0xff
;   [108:12] ? i == 0xff
;   [108:12] ? i == 0xff
    cmp_108_12:
    cmp qword [rsp - 64], 0xff
    jne bool_false_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
;   assert(expr : bool) 
    assert_108_5:
;       alias expr -> r15
        if_19_8_108_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_108_5:
        cmp r15, false
        jne if_19_5_108_5_end
        if_19_8_108_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_108_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_108_5_end:
        if_19_5_108_5_end:
;       free scratch register 'r15'
    assert_108_5_end:
;   var j: i64 @ qword [rsp - 72]
;   [110:5] var j = 1
;   [110:13] j =1
;   [110:13] 1
;   [110:13] 1
;   [111:5] j = 1
    mov qword [rsp - 72], 1
;   var k: i64 @ qword [rsp - 80]
;   [111:5] var k = baz(j)
;   [111:13] k =baz(j)
;   [111:13] baz(j)
;   [111:13] baz(j)
;   [111:13] k = baz(j)
;   [111:13] baz(j)
;   baz(arg) : i64 res 
    baz_111_13:
;       alias res -> k
;       alias arg -> j
;       [68:9] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:15] res = arg
;       allocate scratch register -> r15
        mov r15, qword [rsp - 72]
        mov qword [rsp - 80], r15
;       free scratch register 'r15'
;       [69:1] res * 2
;       allocate scratch register -> r15
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
;       free scratch register 'r15'
    baz_111_13_end:
;   [112:5] assert(k == 2)
;   allocate scratch register -> r15
;   [112:12] k == 2
;   [112:12] ? k == 2
;   [112:12] ? k == 2
    cmp_112_12:
    cmp qword [rsp - 80], 2
    jne bool_false_112_12
    bool_true_112_12:
    mov r15, true
    jmp bool_end_112_12
    bool_false_112_12:
    mov r15, false
    bool_end_112_12:
;   assert(expr : bool) 
    assert_112_5:
;       alias expr -> r15
        if_19_8_112_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_112_5:
        cmp r15, false
        jne if_19_5_112_5_end
        if_19_8_112_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_112_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_112_5_end:
        if_19_5_112_5_end:
;       free scratch register 'r15'
    assert_112_5_end:
;   [114:7] k = baz(1)
;   [114:9] baz(1)
;   [114:9] baz(1)
;   [114:9] k = baz(1)
;   [114:9] baz(1)
;   baz(arg) : i64 res 
    baz_114_9:
;       alias res -> k
;       alias arg -> 1
;       [68:9] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:15] res = arg
        mov qword [rsp - 80], 1
;       [69:1] res * 2
;       allocate scratch register -> r15
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
;       free scratch register 'r15'
    baz_114_9_end:
;   [115:5] assert(k == 2)
;   allocate scratch register -> r15
;   [115:12] k == 2
;   [115:12] ? k == 2
;   [115:12] ? k == 2
    cmp_115_12:
    cmp qword [rsp - 80], 2
    jne bool_false_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
;   assert(expr : bool) 
    assert_115_5:
;       alias expr -> r15
        if_19_8_115_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_115_5:
        cmp r15, false
        jne if_19_5_115_5_end
        if_19_8_115_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_115_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_115_5_end:
        if_19_5_115_5_end:
;       free scratch register 'r15'
    assert_115_5_end:
;   var p0: point @ qword [rsp - 96]
;   [117:5] var p0 : point = {baz(2), 0}
;   [117:22] p0 ={baz(2), 0}
;   [117:22] {baz(2), 0}
;   [117:22] {baz(2), 0}
;   [117:23] baz(2)
;   [117:23] baz(2)
;   [117:23] p0.x = baz(2)
;   [117:23] baz(2)
;   baz(arg) : i64 res 
    baz_117_23:
;       alias res -> p0.x
;       alias arg -> 2
;       [68:9] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:15] res = arg
        mov qword [rsp - 96], 2
;       [69:1] res * 2
;       allocate scratch register -> r15
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
;       free scratch register 'r15'
    baz_117_23_end:
;   [117:31] 0
;   [117:31] 0
;   [117:32] p0.y = 0
    mov qword [rsp - 88], 0
;   [118:5] assert(p0.x == 4)
;   allocate scratch register -> r15
;   [118:12] p0.x == 4
;   [118:12] ? p0.x == 4
;   [118:12] ? p0.x == 4
    cmp_118_12:
    cmp qword [rsp - 96], 4
    jne bool_false_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
;   assert(expr : bool) 
    assert_118_5:
;       alias expr -> r15
        if_19_8_118_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_118_5:
        cmp r15, false
        jne if_19_5_118_5_end
        if_19_8_118_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_118_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_118_5_end:
        if_19_5_118_5_end:
;       free scratch register 'r15'
    assert_118_5_end:
;   var x: i64 @ qword [rsp - 104]
;   [120:5] var x = 1
;   [120:13] x =1
;   [120:13] 1
;   [120:13] 1
;   [121:5] x = 1
    mov qword [rsp - 104], 1
;   var y: i64 @ qword [rsp - 112]
;   [121:5] var y = 2
;   [121:13] y =2
;   [121:13] 2
;   [121:13] 2
;   [123:5] y = 2
    mov qword [rsp - 112], 2
;   var o1: object @ qword [rsp - 132]
;   [123:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [123:23] o1 ={{x * 10, y}, 0xff0000}
;   [123:23] {{x * 10, y}, 0xff0000}
;   [123:23] {{x * 10, y}, 0xff0000}
;       [123:24] {x * 10, y}
;       [123:25] x * 10
;       [123:25] x * 10
;       [123:27] o1.pos.x = x
;       allocate scratch register -> r15
        mov r15, qword [rsp - 104]
        mov qword [rsp - 132], r15
;       free scratch register 'r15'
;       [123:31] o1.pos.x * 10
;       allocate scratch register -> r15
        mov r15, qword [rsp - 132]
        imul r15, 10
        mov qword [rsp - 132], r15
;       free scratch register 'r15'
;       [123:33] y
;       [123:33] y
;       [123:34] o1.pos.y = y
;       allocate scratch register -> r15
        mov r15, qword [rsp - 112]
        mov qword [rsp - 124], r15
;       free scratch register 'r15'
;   [123:37] 0xff0000
;   [123:37] 0xff0000
;   [123:45] o1.color = 0xff0000
    mov dword [rsp - 116], 0xff0000
;   [124:5] assert(o1.pos.x == 10)
;   allocate scratch register -> r15
;   [124:12] o1.pos.x == 10
;   [124:12] ? o1.pos.x == 10
;   [124:12] ? o1.pos.x == 10
    cmp_124_12:
    cmp qword [rsp - 132], 10
    jne bool_false_124_12
    bool_true_124_12:
    mov r15, true
    jmp bool_end_124_12
    bool_false_124_12:
    mov r15, false
    bool_end_124_12:
;   assert(expr : bool) 
    assert_124_5:
;       alias expr -> r15
        if_19_8_124_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_124_5:
        cmp r15, false
        jne if_19_5_124_5_end
        if_19_8_124_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_124_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_124_5_end:
        if_19_5_124_5_end:
;       free scratch register 'r15'
    assert_124_5_end:
;   [125:5] assert(o1.pos.y == 2)
;   allocate scratch register -> r15
;   [125:12] o1.pos.y == 2
;   [125:12] ? o1.pos.y == 2
;   [125:12] ? o1.pos.y == 2
    cmp_125_12:
    cmp qword [rsp - 124], 2
    jne bool_false_125_12
    bool_true_125_12:
    mov r15, true
    jmp bool_end_125_12
    bool_false_125_12:
    mov r15, false
    bool_end_125_12:
;   assert(expr : bool) 
    assert_125_5:
;       alias expr -> r15
        if_19_8_125_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_125_5:
        cmp r15, false
        jne if_19_5_125_5_end
        if_19_8_125_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_125_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_125_5_end:
        if_19_5_125_5_end:
;       free scratch register 'r15'
    assert_125_5_end:
;   [126:5] assert(o1.color == 0xff0000)
;   allocate scratch register -> r15
;   [126:12] o1.color == 0xff0000
;   [126:12] ? o1.color == 0xff0000
;   [126:12] ? o1.color == 0xff0000
    cmp_126_12:
    cmp dword [rsp - 116], 0xff0000
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
;   assert(expr : bool) 
    assert_126_5:
;       alias expr -> r15
        if_19_8_126_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_126_5:
        cmp r15, false
        jne if_19_5_126_5_end
        if_19_8_126_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_126_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
;       free scratch register 'r15'
    assert_126_5_end:
;   var p1: point @ qword [rsp - 148]
;   [128:5] var p1 : point = {-x, -y}
;   [128:22] p1 ={-x, -y}
;   [128:22] {-x, -y}
;   [128:22] {-x, -y}
;   [128:23] -x
;   [128:23] -x
;   [128:25] p1.x = -x
;   allocate scratch register -> r15
    mov r15, qword [rsp - 104]
    mov qword [rsp - 148], r15
;   free scratch register 'r15'
    neg qword [rsp - 148]
;   [128:27] -y
;   [128:27] -y
;   [128:29] p1.y = -y
;   allocate scratch register -> r15
    mov r15, qword [rsp - 112]
    mov qword [rsp - 140], r15
;   free scratch register 'r15'
    neg qword [rsp - 140]
;   [129:7] o1.pos = p1
;   allocate named register 'rsi'
;   allocate named register 'rdi'
;   allocate named register 'rcx'
    lea rdi, [rsp - 132]
;   [129:14] p1
    lea rsi, [rsp - 148]
    mov rcx, 16
    rep movsb
;   free named register 'rcx'
;   free named register 'rdi'
;   free named register 'rsi'
;   [130:5] assert(o1.pos.x == -1)
;   allocate scratch register -> r15
;   [130:12] o1.pos.x == -1
;   [130:12] ? o1.pos.x == -1
;   [130:12] ? o1.pos.x == -1
    cmp_130_12:
    cmp qword [rsp - 132], -1
    jne bool_false_130_12
    bool_true_130_12:
    mov r15, true
    jmp bool_end_130_12
    bool_false_130_12:
    mov r15, false
    bool_end_130_12:
;   assert(expr : bool) 
    assert_130_5:
;       alias expr -> r15
        if_19_8_130_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_130_5:
        cmp r15, false
        jne if_19_5_130_5_end
        if_19_8_130_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_130_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_130_5_end:
        if_19_5_130_5_end:
;       free scratch register 'r15'
    assert_130_5_end:
;   [131:5] assert(o1.pos.y == -2)
;   allocate scratch register -> r15
;   [131:12] o1.pos.y == -2
;   [131:12] ? o1.pos.y == -2
;   [131:12] ? o1.pos.y == -2
    cmp_131_12:
    cmp qword [rsp - 124], -2
    jne bool_false_131_12
    bool_true_131_12:
    mov r15, true
    jmp bool_end_131_12
    bool_false_131_12:
    mov r15, false
    bool_end_131_12:
;   assert(expr : bool) 
    assert_131_5:
;       alias expr -> r15
        if_19_8_131_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_131_5:
        cmp r15, false
        jne if_19_5_131_5_end
        if_19_8_131_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_131_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_131_5_end:
        if_19_5_131_5_end:
;       free scratch register 'r15'
    assert_131_5_end:
;   var o2: object @ qword [rsp - 168]
;   [133:5] var o2 : object = o1
;   [133:23] o2 =o1
;   allocate named register 'rsi'
;   allocate named register 'rdi'
;   allocate named register 'rcx'
    lea rdi, [rsp - 168]
;   [133:23] o1
    lea rsi, [rsp - 132]
    mov rcx, 20
    rep movsb
;   free named register 'rcx'
;   free named register 'rdi'
;   free named register 'rsi'
;   [134:5] assert(o2.pos.x == -1)
;   allocate scratch register -> r15
;   [134:12] o2.pos.x == -1
;   [134:12] ? o2.pos.x == -1
;   [134:12] ? o2.pos.x == -1
    cmp_134_12:
    cmp qword [rsp - 168], -1
    jne bool_false_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
;   assert(expr : bool) 
    assert_134_5:
;       alias expr -> r15
        if_19_8_134_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_134_5:
        cmp r15, false
        jne if_19_5_134_5_end
        if_19_8_134_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_134_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_134_5_end:
        if_19_5_134_5_end:
;       free scratch register 'r15'
    assert_134_5_end:
;   [135:5] assert(o2.pos.y == -2)
;   allocate scratch register -> r15
;   [135:12] o2.pos.y == -2
;   [135:12] ? o2.pos.y == -2
;   [135:12] ? o2.pos.y == -2
    cmp_135_12:
    cmp qword [rsp - 160], -2
    jne bool_false_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
;   assert(expr : bool) 
    assert_135_5:
;       alias expr -> r15
        if_19_8_135_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_135_5:
        cmp r15, false
        jne if_19_5_135_5_end
        if_19_8_135_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_135_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_135_5_end:
        if_19_5_135_5_end:
;       free scratch register 'r15'
    assert_135_5_end:
;   [136:5] assert(o2.color == 0xff0000)
;   allocate scratch register -> r15
;   [136:12] o2.color == 0xff0000
;   [136:12] ? o2.color == 0xff0000
;   [136:12] ? o2.color == 0xff0000
    cmp_136_12:
    cmp dword [rsp - 152], 0xff0000
    jne bool_false_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
;   assert(expr : bool) 
    assert_136_5:
;       alias expr -> r15
        if_19_8_136_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_136_5:
        cmp r15, false
        jne if_19_5_136_5_end
        if_19_8_136_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_136_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
;       free scratch register 'r15'
    assert_136_5_end:
;   var o3: object[1] @ qword [rsp - 188]
;   [138:5] var o3 : object[1]
;   clear array 1 * 20 B = 20 B
;   allocate named register 'rdi'
;   allocate named register 'rcx'
;   allocate named register 'rax'
    lea rdi, [rsp - 188]
    mov rcx, 20
    xor rax, rax
    rep stosb
;   free named register 'rax'
;   free named register 'rcx'
;   free named register 'rdi'
;   [139:5] # index 0 in an array can be accessed without array index
;   [140:7] o3.pos.y = 73
;   [140:16] 73
;   [140:16] 73
;   [141:5] o3.pos.y = 73
    mov qword [rsp - 180], 73
;   [141:5] assert(o3[0].pos.y == 73)
;   allocate scratch register -> r15
;   [141:12] o3[0].pos.y == 73
;   [141:12] ? o3[0].pos.y == 73
;   [141:12] ? o3[0].pos.y == 73
    cmp_141_12:
;   allocate scratch register -> r14
;       [141:12] o3[0].pos.y
;       [141:14] r14 = o3[0].pos.y
;       [141:14] o3[0].pos.y
;       allocate scratch register -> r13
        lea r13, [rsp - 188]
;       allocate scratch register -> r12
;       [141:15] 0
;       [141:15] 0
;       [141:16] r12 = 0
        mov r12, 0
        imul r12, 20
        add r13, r12
;       free scratch register 'r12'
        add r13, 8
        mov r14, qword [r13]
;       free scratch register 'r13'
    cmp r14, 73
;   free scratch register 'r14'
    jne bool_false_141_12
    bool_true_141_12:
    mov r15, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15, false
    bool_end_141_12:
;   assert(expr : bool) 
    assert_141_5:
;       alias expr -> r15
        if_19_8_141_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_141_5:
        cmp r15, false
        jne if_19_5_141_5_end
        if_19_8_141_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_141_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_141_5_end:
        if_19_5_141_5_end:
;       free scratch register 'r15'
    assert_141_5_end:
;   var input_buffer: i8[80] @ byte [rsp - 268]
;   [143:5] var input_buffer : i8[80]
;   clear array 80 * 1 B = 80 B
;   allocate named register 'rdi'
;   allocate named register 'rcx'
;   allocate named register 'rax'
    lea rdi, [rsp - 268]
    mov rcx, 80
    xor rax, rax
    rep stosb
;   free named register 'rax'
;   free named register 'rcx'
;   free named register 'rdi'
;   [145:5] print(hello.len, hello)
;   allocate named register 'rdx'
    mov rdx, hello.len
;   allocate named register 'rsi'
    mov rsi, hello
;   print(len : reg_rdx, ptr : reg_rsi) 
    print_145_5:
;       alias len -> rdx
;       alias ptr -> rsi
;       [23:5] mov(rax, 1)
        mov rax, 1
;       [23:19] # write system call
;       [24:5] mov(rdi, 1)
        mov rdi, 1
;       [24:19] # file descriptor for standard out
;       [25:5] mov(rsi, ptr)
;       [25:19] # buffer address
;       [26:5] mov(rdx, len)
;       [26:19] # buffer size
;       [27:5] syscall()
        syscall
;       free named register 'rsi'
;       free named register 'rdx'
    print_145_5_end:
;   [146:5] loop
    loop_146_5:
;       [147:9] print(prompt1.len, prompt1)
;       allocate named register 'rdx'
        mov rdx, prompt1.len
;       allocate named register 'rsi'
        mov rsi, prompt1
;       print(len : reg_rdx, ptr : reg_rsi) 
        print_147_9:
;           alias len -> rdx
;           alias ptr -> rsi
;           [23:5] mov(rax, 1)
            mov rax, 1
;           [23:19] # write system call
;           [24:5] mov(rdi, 1)
            mov rdi, 1
;           [24:19] # file descriptor for standard out
;           [25:5] mov(rsi, ptr)
;           [25:19] # buffer address
;           [26:5] mov(rdx, len)
;           [26:19] # buffer size
;           [27:5] syscall()
            syscall
;           free named register 'rsi'
;           free named register 'rdx'
        print_147_9_end:
;       var len: i64 @ qword [rsp - 276]
;       [149:9] var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [149:19] len =read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [149:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [149:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [149:19] len = read(array_size_of(input_buffer), address_of(input_buffer))
;       [149:19] read(array_size_of(input_buffer), address_of(input_buffer))
;       allocate named register 'rdx'
;       [149:24] array_size_of(input_buffer)
;       [149:24] array_size_of(input_buffer)
;       [149:24] rdx = array_size_of(input_buffer)
;       [149:24] array_size_of(input_buffer)
        mov rdx, 80
;       allocate named register 'rsi'
;       [149:53] address_of(input_buffer)
;       [149:53] address_of(input_buffer)
;       [149:53] rsi = address_of(input_buffer)
;       [149:53] address_of(input_buffer)
        lea rsi, [rsp - 268]
;       read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_149_19:
;           alias nbytes -> len
;           alias len -> rdx
;           alias ptr -> rsi
;           [31:5] mov(rax, 0)
            mov rax, 0
;           [31:19] # read system call
;           [32:5] mov(rdi, 0)
            mov rdi, 0
;           [32:19] # file descriptor for standard input
;           [33:5] mov(rsi, ptr)
;           [33:19] # buffer address
;           [34:5] mov(rdx, len)
;           [34:19] # buffer size
;           [35:5] syscall()
            syscall
;           [36:5] mov(nbytes, rax)
            mov qword [rsp - 276], rax
;           [36:22] # return value
;           free named register 'rsi'
;           free named register 'rdx'
        read_149_19_end:
;       [150:9] len - 1
        sub qword [rsp - 276], 1
;       [150:9] # note: `array_size_of` and `address_of` are built-in functions
;       [151:9] # -1 to not include the trailing '\n'
        if_153_12:
;       [153:12] ? len == 0
;       [153:12] ? len == 0
        cmp_153_12:
        cmp qword [rsp - 276], 0
        jne if_155_19
        if_153_12_code:
;           [154:13] break
            jmp loop_146_5_end
        jmp if_153_9_end
        if_155_19:
;       [155:19] ? len <= 4
;       [155:19] ? len <= 4
        cmp_155_19:
        cmp qword [rsp - 276], 4
        jg if_else_153_9
        if_155_19_code:
;           [156:13] print(prompt2.len, prompt2)
;           allocate named register 'rdx'
            mov rdx, prompt2.len
;           allocate named register 'rsi'
            mov rsi, prompt2
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_156_13:
;               alias len -> rdx
;               alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               free named register 'rsi'
;               free named register 'rdx'
            print_156_13_end:
;           [157:13] continue
            jmp loop_146_5
        jmp if_153_9_end
        if_else_153_9:
;           [159:13] print(prompt3.len, prompt3)
;           allocate named register 'rdx'
            mov rdx, prompt3.len
;           allocate named register 'rsi'
            mov rsi, prompt3
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_159_13:
;               alias len -> rdx
;               alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               free named register 'rsi'
;               free named register 'rdx'
            print_159_13_end:
;           [160:13] print(len, address_of(input_buffer))
;           allocate named register 'rdx'
            mov rdx, qword [rsp - 276]
;           allocate named register 'rsi'
;           [160:24] address_of(input_buffer)
;           [160:24] address_of(input_buffer)
;           [160:24] rsi = address_of(input_buffer)
;           [160:24] address_of(input_buffer)
            lea rsi, [rsp - 268]
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_160_13:
;               alias len -> rdx
;               alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               free named register 'rsi'
;               free named register 'rdx'
            print_160_13_end:
;           [161:13] print(dot.len, dot)
;           allocate named register 'rdx'
            mov rdx, dot.len
;           allocate named register 'rsi'
            mov rsi, dot
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_161_13:
;               alias len -> rdx
;               alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               free named register 'rsi'
;               free named register 'rdx'
            print_161_13_end:
;           [162:13] print(nl.len, nl)
;           allocate named register 'rdx'
            mov rdx, nl.len
;           allocate named register 'rsi'
            mov rsi, nl
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_162_13:
;               alias len -> rdx
;               alias ptr -> rsi
;               [23:5] mov(rax, 1)
                mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 1)
                mov rdi, 1
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               free named register 'rsi'
;               free named register 'rdx'
            print_162_13_end:
        if_153_9_end:
    jmp loop_146_5
    loop_146_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; max scratch registers in use: 4
;            max frames in use: 7
;               max stack size: 276 B
```
