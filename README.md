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
* inlined functions
* keywords: `func`, `field`, `var`, `loop`, `if`, `else`, `continue`, `break`, `return`

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
C/C++ Header                    37            860            607           4617
C++                              1             52             68            393
-------------------------------------------------------------------------------
SUM:                            38            912            675           5010
-------------------------------------------------------------------------------
```

## Sample

```text
field   hello = "hello world from baz\n"
field   input = "............................................................"
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

func assert(expr : bool) {
    if not expr exit(1)
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

func inv(i : i32) : i32 res {
    res = ~i
}

# return target is specified as a variable, in this case `res`
func baz(arg) : i64 res {
    res = arg * 2
}

func main() {
    var arr : i32[4]
    # arrays are initialized to 0

    arr[1] = 2
    arr[2] = arr[1]
    
    assert(arr[1] == 2)
    assert(arr[2] == 2)

    var ix = 3
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

    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        var len = read(input.len, input) - 1    # -1 don't include the '\n'
        if len == 0 {
            break
        } else if len <= 4 {
            print(prompt2.len, prompt2)
            continue
        } else {
            print(prompt3.len, prompt3)
            print(len, input)
            print(dot.len, dot)
            print(nl.len, nl)
        }
    }
}
```

## Generates

```nasm
section .bss
stk resd 1024
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz',10,''
hello.len equ $-hello
input: db '............................................................'
input.len equ $-input
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
nl: db '',10,''
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
    mov r15, 1
    lea r15, [rsp + r15 * 4 - 16]
    mov dword [r15], 2
    mov r15, 2
    lea r15, [rsp + r15 * 4 - 16]
    mov r14, 1
    lea r14, [rsp + r14 * 4 - 16]
    mov r13d, dword [r14]
    mov dword [r15], r13d
    cmp_74_12:
        mov r13, 1
        lea r13, [rsp + r13 * 4 - 16]
        movsx r14, dword [r13]
    cmp r14, 2
    jne bool_false_74_12
    jmp bool_true_74_12
    bool_true_74_12:
    mov r15, true
    jmp bool_end_74_12
    bool_false_74_12:
    mov r15, false
    bool_end_74_12:
    assert_74_5:
        if_36_8_74_5:
        cmp_36_8_74_5:
        cmp r15, false
        jne if_36_5_74_5_end
        jmp if_36_8_74_5_code
        if_36_8_74_5_code:
            mov rdi, 1
            exit_36_17_74_5:
                mov rax, 60
                syscall
            exit_36_17_74_5_end:
        if_36_5_74_5_end:
    assert_74_5_end:
    cmp_75_12:
        mov r13, 2
        lea r13, [rsp + r13 * 4 - 16]
        movsx r14, dword [r13]
    cmp r14, 2
    jne bool_false_75_12
    jmp bool_true_75_12
    bool_true_75_12:
    mov r15, true
    jmp bool_end_75_12
    bool_false_75_12:
    mov r15, false
    bool_end_75_12:
    assert_75_5:
        if_36_8_75_5:
        cmp_36_8_75_5:
        cmp r15, false
        jne if_36_5_75_5_end
        jmp if_36_8_75_5_code
        if_36_8_75_5_code:
            mov rdi, 1
            exit_36_17_75_5:
                mov rax, 60
                syscall
            exit_36_17_75_5_end:
        if_36_5_75_5_end:
    assert_75_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    lea r15, [rsp + r15 * 4 - 16]
    mov r13, qword [rsp - 24]
    sub r13, 1
    lea r13, [rsp + r13 * 4 - 16]
    movsx r14, dword [r13]
    inv_78_16:
        mov dword [r15], r14d
        not dword [r15]
    inv_78_16_end:
    not dword [r15]
    cmp_79_12:
        mov r13, qword [rsp - 24]
        lea r13, [rsp + r13 * 4 - 16]
        movsx r14, dword [r13]
    cmp r14, 2
    jne bool_false_79_12
    jmp bool_true_79_12
    bool_true_79_12:
    mov r15, true
    jmp bool_end_79_12
    bool_false_79_12:
    mov r15, false
    bool_end_79_12:
    assert_79_5:
        if_36_8_79_5:
        cmp_36_8_79_5:
        cmp r15, false
        jne if_36_5_79_5_end
        jmp if_36_8_79_5_code
        if_36_8_79_5_code:
            mov rdi, 1
            exit_36_17_79_5:
                mov rax, 60
                syscall
            exit_36_17_79_5_end:
        if_36_5_79_5_end:
    assert_79_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_82_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_82_5_end:
    cmp_83_12:
    cmp qword [rsp - 40], 2
    jne bool_false_83_12
    jmp bool_true_83_12
    bool_true_83_12:
    mov r15, true
    jmp bool_end_83_12
    bool_false_83_12:
    mov r15, false
    bool_end_83_12:
    assert_83_5:
        if_36_8_83_5:
        cmp_36_8_83_5:
        cmp r15, false
        jne if_36_5_83_5_end
        jmp if_36_8_83_5_code
        if_36_8_83_5_code:
            mov rdi, 1
            exit_36_17_83_5:
                mov rax, 60
                syscall
            exit_36_17_83_5_end:
        if_36_5_83_5_end:
    assert_83_5_end:
    cmp_84_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_84_12
    jmp bool_true_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
    assert_84_5:
        if_36_8_84_5:
        cmp_36_8_84_5:
        cmp r15, false
        jne if_36_5_84_5_end
        jmp if_36_8_84_5_code
        if_36_8_84_5_code:
            mov rdi, 1
            exit_36_17_84_5:
                mov rax, 60
                syscall
            exit_36_17_84_5_end:
        if_36_5_84_5_end:
    assert_84_5_end:
    mov qword [rsp - 48], 0
    bar_87_5:
        if_54_8_87_5:
        cmp_54_8_87_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_87_5_end
        jmp if_54_8_87_5_code
        if_54_8_87_5_code:
            jmp bar_87_5_end
        if_54_5_87_5_end:
        mov qword [rsp - 48], 0xff
    bar_87_5_end:
    cmp_88_12:
    cmp qword [rsp - 48], 0
    jne bool_false_88_12
    jmp bool_true_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
    assert_88_5:
        if_36_8_88_5:
        cmp_36_8_88_5:
        cmp r15, false
        jne if_36_5_88_5_end
        jmp if_36_8_88_5_code
        if_36_8_88_5_code:
            mov rdi, 1
            exit_36_17_88_5:
                mov rax, 60
                syscall
            exit_36_17_88_5_end:
        if_36_5_88_5_end:
    assert_88_5_end:
    mov qword [rsp - 48], 1
    bar_91_5:
        if_54_8_91_5:
        cmp_54_8_91_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_91_5_end
        jmp if_54_8_91_5_code
        if_54_8_91_5_code:
            jmp bar_91_5_end
        if_54_5_91_5_end:
        mov qword [rsp - 48], 0xff
    bar_91_5_end:
    cmp_92_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_92_12
    jmp bool_true_92_12
    bool_true_92_12:
    mov r15, true
    jmp bool_end_92_12
    bool_false_92_12:
    mov r15, false
    bool_end_92_12:
    assert_92_5:
        if_36_8_92_5:
        cmp_36_8_92_5:
        cmp r15, false
        jne if_36_5_92_5_end
        jmp if_36_8_92_5_code
        if_36_8_92_5_code:
            mov rdi, 1
            exit_36_17_92_5:
                mov rax, 60
                syscall
            exit_36_17_92_5_end:
        if_36_5_92_5_end:
    assert_92_5_end:
    mov qword [rsp - 56], 1
    baz_95_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_95_13_end:
    cmp_96_12:
    cmp qword [rsp - 64], 2
    jne bool_false_96_12
    jmp bool_true_96_12
    bool_true_96_12:
    mov r15, true
    jmp bool_end_96_12
    bool_false_96_12:
    mov r15, false
    bool_end_96_12:
    assert_96_5:
        if_36_8_96_5:
        cmp_36_8_96_5:
        cmp r15, false
        jne if_36_5_96_5_end
        jmp if_36_8_96_5_code
        if_36_8_96_5_code:
            mov rdi, 1
            exit_36_17_96_5:
                mov rax, 60
                syscall
            exit_36_17_96_5_end:
        if_36_5_96_5_end:
    assert_96_5_end:
    baz_98_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_98_9_end:
    cmp_99_12:
    cmp qword [rsp - 64], 2
    jne bool_false_99_12
    jmp bool_true_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
    assert_99_5:
        if_36_8_99_5:
        cmp_36_8_99_5:
        cmp r15, false
        jne if_36_5_99_5_end
        jmp if_36_8_99_5_code
        if_36_8_99_5_code:
            mov rdi, 1
            exit_36_17_99_5:
                mov rax, 60
                syscall
            exit_36_17_99_5_end:
        if_36_5_99_5_end:
    assert_99_5_end:
    baz_101_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_101_23_end:
    mov qword [rsp - 72], 0
    cmp_102_12:
    cmp qword [rsp - 80], 4
    jne bool_false_102_12
    jmp bool_true_102_12
    bool_true_102_12:
    mov r15, true
    jmp bool_end_102_12
    bool_false_102_12:
    mov r15, false
    bool_end_102_12:
    assert_102_5:
        if_36_8_102_5:
        cmp_36_8_102_5:
        cmp r15, false
        jne if_36_5_102_5_end
        jmp if_36_8_102_5_code
        if_36_8_102_5_code:
            mov rdi, 1
            exit_36_17_102_5:
                mov rax, 60
                syscall
            exit_36_17_102_5_end:
        if_36_5_102_5_end:
    assert_102_5_end:
    mov qword [rsp - 88], 1
    mov qword [rsp - 96], 2
        mov r15, qword [rsp - 88]
        mov qword [rsp - 116], r15
        mov r15, qword [rsp - 116]
        imul r15, 10
        mov qword [rsp - 116], r15
        mov r15, qword [rsp - 96]
        mov qword [rsp - 108], r15
    mov dword [rsp - 100], 0xff0000
    cmp_108_12:
    cmp qword [rsp - 116], 10
    jne bool_false_108_12
    jmp bool_true_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
    assert_108_5:
        if_36_8_108_5:
        cmp_36_8_108_5:
        cmp r15, false
        jne if_36_5_108_5_end
        jmp if_36_8_108_5_code
        if_36_8_108_5_code:
            mov rdi, 1
            exit_36_17_108_5:
                mov rax, 60
                syscall
            exit_36_17_108_5_end:
        if_36_5_108_5_end:
    assert_108_5_end:
    cmp_109_12:
    cmp qword [rsp - 108], 2
    jne bool_false_109_12
    jmp bool_true_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
    assert_109_5:
        if_36_8_109_5:
        cmp_36_8_109_5:
        cmp r15, false
        jne if_36_5_109_5_end
        jmp if_36_8_109_5_code
        if_36_8_109_5_code:
            mov rdi, 1
            exit_36_17_109_5:
                mov rax, 60
                syscall
            exit_36_17_109_5_end:
        if_36_5_109_5_end:
    assert_109_5_end:
    cmp_110_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_36_8_110_5:
        cmp_36_8_110_5:
        cmp r15, false
        jne if_36_5_110_5_end
        jmp if_36_8_110_5_code
        if_36_8_110_5_code:
            mov rdi, 1
            exit_36_17_110_5:
                mov rax, 60
                syscall
            exit_36_17_110_5_end:
        if_36_5_110_5_end:
    assert_110_5_end:
    mov r15, qword [rsp - 88]
    mov qword [rsp - 132], r15
    neg qword [rsp - 132]
    mov r15, qword [rsp - 96]
    mov qword [rsp - 124], r15
    neg qword [rsp - 124]
    mov r15, qword [rsp - 132]
    mov qword [rsp - 116], r15
    mov r15, qword [rsp - 124]
    mov qword [rsp - 108], r15
    cmp_114_12:
    cmp qword [rsp - 116], -1
    jne bool_false_114_12
    jmp bool_true_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
    assert_114_5:
        if_36_8_114_5:
        cmp_36_8_114_5:
        cmp r15, false
        jne if_36_5_114_5_end
        jmp if_36_8_114_5_code
        if_36_8_114_5_code:
            mov rdi, 1
            exit_36_17_114_5:
                mov rax, 60
                syscall
            exit_36_17_114_5_end:
        if_36_5_114_5_end:
    assert_114_5_end:
    cmp_115_12:
    cmp qword [rsp - 108], -2
    jne bool_false_115_12
    jmp bool_true_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_36_8_115_5:
        cmp_36_8_115_5:
        cmp r15, false
        jne if_36_5_115_5_end
        jmp if_36_8_115_5_code
        if_36_8_115_5_code:
            mov rdi, 1
            exit_36_17_115_5:
                mov rax, 60
                syscall
            exit_36_17_115_5_end:
        if_36_5_115_5_end:
    assert_115_5_end:
        mov r15, qword [rsp - 116]
        mov qword [rsp - 152], r15
        mov r15, qword [rsp - 108]
        mov qword [rsp - 144], r15
    mov r15d, dword [rsp - 100]
    mov dword [rsp - 136], r15d
    cmp_118_12:
    cmp qword [rsp - 152], -1
    jne bool_false_118_12
    jmp bool_true_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
    assert_118_5:
        if_36_8_118_5:
        cmp_36_8_118_5:
        cmp r15, false
        jne if_36_5_118_5_end
        jmp if_36_8_118_5_code
        if_36_8_118_5_code:
            mov rdi, 1
            exit_36_17_118_5:
                mov rax, 60
                syscall
            exit_36_17_118_5_end:
        if_36_5_118_5_end:
    assert_118_5_end:
    cmp_119_12:
    cmp qword [rsp - 144], -2
    jne bool_false_119_12
    jmp bool_true_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
    assert_119_5:
        if_36_8_119_5:
        cmp_36_8_119_5:
        cmp r15, false
        jne if_36_5_119_5_end
        jmp if_36_8_119_5_code
        if_36_8_119_5_code:
            mov rdi, 1
            exit_36_17_119_5:
                mov rax, 60
                syscall
            exit_36_17_119_5_end:
        if_36_5_119_5_end:
    assert_119_5_end:
    cmp_120_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_120_12
    jmp bool_true_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_36_8_120_5:
        cmp_36_8_120_5:
        cmp r15, false
        jne if_36_5_120_5_end
        jmp if_36_8_120_5_code
        if_36_8_120_5_code:
            mov rdi, 1
            exit_36_17_120_5:
                mov rax, 60
                syscall
            exit_36_17_120_5_end:
        if_36_5_120_5_end:
    assert_120_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_122_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_122_5_end:
    loop_123_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_124_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_124_9_end:
        mov rdx, input.len
        mov rsi, input
        read_125_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 160], rax
        read_125_19_end:
        sub qword [rsp - 160], 1
        if_126_12:
        cmp_126_12:
        cmp qword [rsp - 160], 0
        jne if_128_19
        jmp if_126_12_code
        if_126_12_code:
            jmp loop_123_5_end
        jmp if_126_9_end
        if_128_19:
        cmp_128_19:
        cmp qword [rsp - 160], 4
        jg if_else_126_9
        jmp if_128_19_code
        if_128_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_129_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_129_13_end:
            jmp loop_123_5
        jmp if_126_9_end
        if_else_126_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_132_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_132_13_end:
            mov rdx, qword [rsp - 160]
            mov rsi, input
            print_133_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_133_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_134_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_134_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_135_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_135_13_end:
        if_126_9_end:
    jmp loop_123_5
    loop_123_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
```

## With comments

```nasm

; generated by baz

section .bss
stk resd 1024
stk.end:

true equ 1
false equ 0

section .data
;[1:1] field hello = "hello world from baz\n"
hello: db 'hello world from baz',10,''
hello.len equ $-hello
;[2:1] field input = "............................................................"
input: db '............................................................'
input.len equ $-input
;[3:1] field prompt1 = "enter name:\n"
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
;[4:1] field prompt2 = "that is not a name.\n"
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
;[5:1] field prompt3 = "hello "
prompt3: db 'hello '
prompt3.len equ $-prompt3
;[6:1] field dot = "."
dot: db '.'
dot.len equ $-dot
;[7:1] field nl = "\n"
nl: db '',10,''
nl.len equ $-nl

section .text
bits 64
global _start
_start:
mov rsp,stk.end
; program
;[9:1] # all functions are inlined
;[11:1]  # arguments can be placed in specified register using `reg_...` syntax
;[39:1] # user types are defined using keyword `type`
;[41:1]  # default type is `i64` and does not need to be specified
;[46:1]  # function arguments are equivalent to mutable references
;[52:1] # default argument type is `i64`
;[62:1] # return target is specified as a variable, in this case `res`
main:
;    var arr: i32[4] @ dword [rsp - 16]
;    [68:5]  var arr : i32[4]
;    clear array 4 * 4 B = 16 B
;    alloc rdi
;    alloc rcx
;    alloc rax
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
;    free rax
;    free rcx
;    free rdi
;    [69:5] # arrays are initialized to 0
;    [71:8]  arr[1] = 2
;    alloc r15
;    [71:9] 1
;    [71:9] 1
;    [71:10] r15 = 1
    mov r15, 1
    lea r15, [rsp + r15 * 4 - 16]
;    [71:14]  2
;    [71:14] 2
;    [72:5] dword [r15] = 2
    mov dword [r15], 2
;    free r15
;    [72:8] arr[2] = arr[1]
;    alloc r15
;    [72:9] 2
;    [72:9] 2
;    [72:10] r15 = 2
    mov r15, 2
    lea r15, [rsp + r15 * 4 - 16]
;    [72:14]  arr[1]
;    [72:14] arr[1]
;    [72:17] dword [r15] = arr[1]
;    alloc r14
;    [72:18] 1
;    [72:18] 1
;    [72:19] r14 = 1
    mov r14, 1
    lea r14, [rsp + r14 * 4 - 16]
;    alloc r13
    mov r13d, dword [r14]
    mov dword [r15], r13d
;    free r13
;    free r14
;    free r15
;    [74:5] assert(arr[1] == 2)
;    alloc r15
;    [74:12] arr[1] == 2
;    [74:12] ? arr[1] == 2
;    [74:12] ? arr[1] == 2
    cmp_74_12:
;    alloc r14
;        [74:12] arr[1]
;        [74:15] r14 = arr[1]
;        alloc r13
;        [74:16] 1
;        [74:16] 1
;        [74:17] r13 = 1
        mov r13, 1
        lea r13, [rsp + r13 * 4 - 16]
        movsx r14, dword [r13]
;        free r13
    cmp r14, 2
;    free r14
    jne bool_false_74_12
    jmp bool_true_74_12
    bool_true_74_12:
    mov r15, true
    jmp bool_end_74_12
    bool_false_74_12:
    mov r15, false
    bool_end_74_12:
;    assert(expr : bool) 
    assert_74_5:
;        alias expr -> r15
        if_36_8_74_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_74_5:
        cmp r15, false
        jne if_36_5_74_5_end
        jmp if_36_8_74_5_code
        if_36_8_74_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_74_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_74_5_end:
        if_36_5_74_5_end:
;        free r15
    assert_74_5_end:
;    [75:5] assert(arr[2] == 2)
;    alloc r15
;    [75:12] arr[2] == 2
;    [75:12] ? arr[2] == 2
;    [75:12] ? arr[2] == 2
    cmp_75_12:
;    alloc r14
;        [75:12] arr[2]
;        [75:15] r14 = arr[2]
;        alloc r13
;        [75:16] 2
;        [75:16] 2
;        [75:17] r13 = 2
        mov r13, 2
        lea r13, [rsp + r13 * 4 - 16]
        movsx r14, dword [r13]
;        free r13
    cmp r14, 2
;    free r14
    jne bool_false_75_12
    jmp bool_true_75_12
    bool_true_75_12:
    mov r15, true
    jmp bool_end_75_12
    bool_false_75_12:
    mov r15, false
    bool_end_75_12:
;    assert(expr : bool) 
    assert_75_5:
;        alias expr -> r15
        if_36_8_75_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_75_5:
        cmp r15, false
        jne if_36_5_75_5_end
        jmp if_36_8_75_5_code
        if_36_8_75_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_75_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_75_5_end:
        if_36_5_75_5_end:
;        free r15
    assert_75_5_end:
;    var ix: i64 @ qword [rsp - 24]
;    [77:5] var ix = 3
;    [77:14] ix =3
;    [77:14] 3
;    [77:14] 3
;    [78:5] ix = 3
    mov qword [rsp - 24], 3
;    [78:8] arr[ix] = ~inv(arr[ix - 1])
;    alloc r15
;    [78:9] ix
;    [78:9] ix
;    [78:11] r15 = ix
    mov r15, qword [rsp - 24]
    lea r15, [rsp + r15 * 4 - 16]
;    [78:15]  ~inv(arr[ix - 1])
;    [78:15] ~inv(arr[ix - 1])
;    [78:16] dword [r15] = ~inv(arr[ix - 1])
;    [78:16] ~inv(arr[ix - 1])
;    alloc r14
;    [78:20] arr[ix - 1]
;    [78:20] arr[ix - 1]
;    [78:23] r14 = arr[ix - 1]
;    alloc r13
;    [78:24] ix - 1
;    [78:24] ix - 1
;    [78:27] r13 = ix
    mov r13, qword [rsp - 24]
;    [78:30] r13 - 1
    sub r13, 1
    lea r13, [rsp + r13 * 4 - 16]
    movsx r14, dword [r13]
;    free r13
;    inv(i : i32) : i32 res 
    inv_78_16:
;        alias res -> dword [r15]
;        alias i -> r14
;        [59:9]  res = ~i
;        [59:11]  ~i
;        [59:11] ~i
;        [60:1] res = ~i
        mov dword [r15], r14d
        not dword [r15]
;        free r14
    inv_78_16_end:
    not dword [r15]
;    free r15
;    [79:5] assert(arr[ix] == 2)
;    alloc r15
;    [79:12] arr[ix] == 2
;    [79:12] ? arr[ix] == 2
;    [79:12] ? arr[ix] == 2
    cmp_79_12:
;    alloc r14
;        [79:12] arr[ix]
;        [79:15] r14 = arr[ix]
;        alloc r13
;        [79:16] ix
;        [79:16] ix
;        [79:18] r13 = ix
        mov r13, qword [rsp - 24]
        lea r13, [rsp + r13 * 4 - 16]
        movsx r14, dword [r13]
;        free r13
    cmp r14, 2
;    free r14
    jne bool_false_79_12
    jmp bool_true_79_12
    bool_true_79_12:
    mov r15, true
    jmp bool_end_79_12
    bool_false_79_12:
    mov r15, false
    bool_end_79_12:
;    assert(expr : bool) 
    assert_79_5:
;        alias expr -> r15
        if_36_8_79_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_79_5:
        cmp r15, false
        jne if_36_5_79_5_end
        jmp if_36_8_79_5_code
        if_36_8_79_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_79_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_79_5_end:
        if_36_5_79_5_end:
;        free r15
    assert_79_5_end:
;    var p: point @ qword [rsp - 40]
;    [81:5] var p : point = {0, 0}
;    [81:21] p ={0, 0}
;    [81:21] {0, 0}
;    [81:21] {0, 0}
;    [81:22] 0
;    [81:22] 0
;    [81:23] p.x = 0
    mov qword [rsp - 40], 0
;    [81:25]  0
;    [81:25] 0
;    [81:26] p.y = 0
    mov qword [rsp - 32], 0
;    [82:5]  foo(p)
;    foo(pt : point) 
    foo_82_5:
;        alias pt -> p
;        [48:7]  pt.x = 0b10
;        [48:12]  0b10
;        [48:12] 0b10
;        [48:20] pt.x = 0b10
        mov qword [rsp - 40], 0b10
;        [48:20] # binary value 2
;        [49:7]  pt.y = 0xb
;        [49:12]  0xb
;        [49:12] 0xb
;        [49:20] pt.y = 0xb
        mov qword [rsp - 32], 0xb
;        [49:20] # hex value 11
    foo_82_5_end:
;    [83:5] assert(p.x == 2)
;    alloc r15
;    [83:12] p.x == 2
;    [83:12] ? p.x == 2
;    [83:12] ? p.x == 2
    cmp_83_12:
    cmp qword [rsp - 40], 2
    jne bool_false_83_12
    jmp bool_true_83_12
    bool_true_83_12:
    mov r15, true
    jmp bool_end_83_12
    bool_false_83_12:
    mov r15, false
    bool_end_83_12:
;    assert(expr : bool) 
    assert_83_5:
;        alias expr -> r15
        if_36_8_83_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_83_5:
        cmp r15, false
        jne if_36_5_83_5_end
        jmp if_36_8_83_5_code
        if_36_8_83_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_83_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_83_5_end:
        if_36_5_83_5_end:
;        free r15
    assert_83_5_end:
;    [84:5] assert(p.y == 0xb)
;    alloc r15
;    [84:12] p.y == 0xb
;    [84:12] ? p.y == 0xb
;    [84:12] ? p.y == 0xb
    cmp_84_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_84_12
    jmp bool_true_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
;    assert(expr : bool) 
    assert_84_5:
;        alias expr -> r15
        if_36_8_84_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_84_5:
        cmp r15, false
        jne if_36_5_84_5_end
        jmp if_36_8_84_5_code
        if_36_8_84_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_84_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_84_5_end:
        if_36_5_84_5_end:
;        free r15
    assert_84_5_end:
;    var i: i64 @ qword [rsp - 48]
;    [86:5] var i = 0
;    [86:13] i =0
;    [86:13] 0
;    [86:13] 0
;    [87:5] i = 0
    mov qword [rsp - 48], 0
;    [87:5] bar(i)
;    bar(arg) 
    bar_87_5:
;        alias arg -> i
        if_54_8_87_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_87_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_87_5_end
        jmp if_54_8_87_5_code
        if_54_8_87_5_code:
;            [54:17] return
            jmp bar_87_5_end
        if_54_5_87_5_end:
;        [55:9] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [56:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_87_5_end:
;    [88:5] assert(i == 0)
;    alloc r15
;    [88:12] i == 0
;    [88:12] ? i == 0
;    [88:12] ? i == 0
    cmp_88_12:
    cmp qword [rsp - 48], 0
    jne bool_false_88_12
    jmp bool_true_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
;    assert(expr : bool) 
    assert_88_5:
;        alias expr -> r15
        if_36_8_88_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_88_5:
        cmp r15, false
        jne if_36_5_88_5_end
        jmp if_36_8_88_5_code
        if_36_8_88_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_88_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_88_5_end:
        if_36_5_88_5_end:
;        free r15
    assert_88_5_end:
;    [90:7] i = 1
;    [90:9]  1
;    [90:9] 1
;    [91:5] i = 1
    mov qword [rsp - 48], 1
;    [91:5] bar(i)
;    bar(arg) 
    bar_91_5:
;        alias arg -> i
        if_54_8_91_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_91_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_91_5_end
        jmp if_54_8_91_5_code
        if_54_8_91_5_code:
;            [54:17] return
            jmp bar_91_5_end
        if_54_5_91_5_end:
;        [55:9] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [56:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_91_5_end:
;    [92:5] assert(i == 0xff)
;    alloc r15
;    [92:12] i == 0xff
;    [92:12] ? i == 0xff
;    [92:12] ? i == 0xff
    cmp_92_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_92_12
    jmp bool_true_92_12
    bool_true_92_12:
    mov r15, true
    jmp bool_end_92_12
    bool_false_92_12:
    mov r15, false
    bool_end_92_12:
;    assert(expr : bool) 
    assert_92_5:
;        alias expr -> r15
        if_36_8_92_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_92_5:
        cmp r15, false
        jne if_36_5_92_5_end
        jmp if_36_8_92_5_code
        if_36_8_92_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_92_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_92_5_end:
        if_36_5_92_5_end:
;        free r15
    assert_92_5_end:
;    var j: i64 @ qword [rsp - 56]
;    [94:5] var j = 1
;    [94:13] j =1
;    [94:13] 1
;    [94:13] 1
;    [95:5] j = 1
    mov qword [rsp - 56], 1
;    var k: i64 @ qword [rsp - 64]
;    [95:5] var k = baz(j)
;    [95:13] k =baz(j)
;    [95:13] baz(j)
;    [95:13] baz(j)
;    [95:13] k = baz(j)
;    [95:13] baz(j)
;    baz(arg) : i64 res 
    baz_95_13:
;        alias res -> k
;        alias arg -> j
;        [64:9]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:15] res = arg
;        alloc r15
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
;        free r15
;        [65:1] res * 2
;        alloc r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;        free r15
    baz_95_13_end:
;    [96:5] assert(k == 2)
;    alloc r15
;    [96:12] k == 2
;    [96:12] ? k == 2
;    [96:12] ? k == 2
    cmp_96_12:
    cmp qword [rsp - 64], 2
    jne bool_false_96_12
    jmp bool_true_96_12
    bool_true_96_12:
    mov r15, true
    jmp bool_end_96_12
    bool_false_96_12:
    mov r15, false
    bool_end_96_12:
;    assert(expr : bool) 
    assert_96_5:
;        alias expr -> r15
        if_36_8_96_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_96_5:
        cmp r15, false
        jne if_36_5_96_5_end
        jmp if_36_8_96_5_code
        if_36_8_96_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_96_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_96_5_end:
        if_36_5_96_5_end:
;        free r15
    assert_96_5_end:
;    [98:7] k = baz(1)
;    [98:9]  baz(1)
;    [98:9] baz(1)
;    [98:9] k = baz(1)
;    [98:9] baz(1)
;    baz(arg) : i64 res 
    baz_98_9:
;        alias res -> k
;        alias arg -> 1
;        [64:9]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:15] res = arg
        mov qword [rsp - 64], 1
;        [65:1] res * 2
;        alloc r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;        free r15
    baz_98_9_end:
;    [99:5] assert(k == 2)
;    alloc r15
;    [99:12] k == 2
;    [99:12] ? k == 2
;    [99:12] ? k == 2
    cmp_99_12:
    cmp qword [rsp - 64], 2
    jne bool_false_99_12
    jmp bool_true_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
;    assert(expr : bool) 
    assert_99_5:
;        alias expr -> r15
        if_36_8_99_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_99_5:
        cmp r15, false
        jne if_36_5_99_5_end
        jmp if_36_8_99_5_code
        if_36_8_99_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_99_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_99_5_end:
        if_36_5_99_5_end:
;        free r15
    assert_99_5_end:
;    var p0: point @ qword [rsp - 80]
;    [101:5] var p0 : point = {baz(2), 0}
;    [101:22] p0 ={baz(2), 0}
;    [101:22] {baz(2), 0}
;    [101:22] {baz(2), 0}
;    [101:23] baz(2)
;    [101:23] baz(2)
;    [101:23] p0.x = baz(2)
;    [101:23] baz(2)
;    baz(arg) : i64 res 
    baz_101_23:
;        alias res -> p0.x
;        alias arg -> 2
;        [64:9]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:15] res = arg
        mov qword [rsp - 80], 2
;        [65:1] res * 2
;        alloc r15
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
;        free r15
    baz_101_23_end:
;    [101:31]  0
;    [101:31] 0
;    [101:32] p0.y = 0
    mov qword [rsp - 72], 0
;    [102:5]  assert(p0.x == 4)
;    alloc r15
;    [102:12] p0.x == 4
;    [102:12] ? p0.x == 4
;    [102:12] ? p0.x == 4
    cmp_102_12:
    cmp qword [rsp - 80], 4
    jne bool_false_102_12
    jmp bool_true_102_12
    bool_true_102_12:
    mov r15, true
    jmp bool_end_102_12
    bool_false_102_12:
    mov r15, false
    bool_end_102_12:
;    assert(expr : bool) 
    assert_102_5:
;        alias expr -> r15
        if_36_8_102_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_102_5:
        cmp r15, false
        jne if_36_5_102_5_end
        jmp if_36_8_102_5_code
        if_36_8_102_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_102_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_102_5_end:
        if_36_5_102_5_end:
;        free r15
    assert_102_5_end:
;    var x: i64 @ qword [rsp - 88]
;    [104:5] var x = 1
;    [104:13] x =1
;    [104:13] 1
;    [104:13] 1
;    [105:5] x = 1
    mov qword [rsp - 88], 1
;    var y: i64 @ qword [rsp - 96]
;    [105:5] var y = 2
;    [105:13] y =2
;    [105:13] 2
;    [105:13] 2
;    [107:5] y = 2
    mov qword [rsp - 96], 2
;    var o1: object @ qword [rsp - 116]
;    [107:5] var o1 : object = {{x * 10, y}, 0xff0000}
;    [107:23] o1 ={{x * 10, y}, 0xff0000}
;    [107:23] {{x * 10, y}, 0xff0000}
;    [107:23] {{x * 10, y}, 0xff0000}
;        [107:24] {x * 10, y}
;        [107:25] x * 10
;        [107:25] x * 10
;        [107:27] o1.pos.x = x
;        alloc r15
        mov r15, qword [rsp - 88]
        mov qword [rsp - 116], r15
;        free r15
;        [107:31] o1.pos.x * 10
;        alloc r15
        mov r15, qword [rsp - 116]
        imul r15, 10
        mov qword [rsp - 116], r15
;        free r15
;        [107:33]  y
;        [107:33] y
;        [107:34] o1.pos.y = y
;        alloc r15
        mov r15, qword [rsp - 96]
        mov qword [rsp - 108], r15
;        free r15
;    [107:37]  0xff0000
;    [107:37] 0xff0000
;    [107:45] o1.color = 0xff0000
    mov dword [rsp - 100], 0xff0000
;    [108:5]  assert(o1.pos.x == 10)
;    alloc r15
;    [108:12] o1.pos.x == 10
;    [108:12] ? o1.pos.x == 10
;    [108:12] ? o1.pos.x == 10
    cmp_108_12:
    cmp qword [rsp - 116], 10
    jne bool_false_108_12
    jmp bool_true_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
;    assert(expr : bool) 
    assert_108_5:
;        alias expr -> r15
        if_36_8_108_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_108_5:
        cmp r15, false
        jne if_36_5_108_5_end
        jmp if_36_8_108_5_code
        if_36_8_108_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_108_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_108_5_end:
        if_36_5_108_5_end:
;        free r15
    assert_108_5_end:
;    [109:5] assert(o1.pos.y == 2)
;    alloc r15
;    [109:12] o1.pos.y == 2
;    [109:12] ? o1.pos.y == 2
;    [109:12] ? o1.pos.y == 2
    cmp_109_12:
    cmp qword [rsp - 108], 2
    jne bool_false_109_12
    jmp bool_true_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
;    assert(expr : bool) 
    assert_109_5:
;        alias expr -> r15
        if_36_8_109_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_109_5:
        cmp r15, false
        jne if_36_5_109_5_end
        jmp if_36_8_109_5_code
        if_36_8_109_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_109_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_109_5_end:
        if_36_5_109_5_end:
;        free r15
    assert_109_5_end:
;    [110:5] assert(o1.color == 0xff0000)
;    alloc r15
;    [110:12] o1.color == 0xff0000
;    [110:12] ? o1.color == 0xff0000
;    [110:12] ? o1.color == 0xff0000
    cmp_110_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
;    assert(expr : bool) 
    assert_110_5:
;        alias expr -> r15
        if_36_8_110_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_110_5:
        cmp r15, false
        jne if_36_5_110_5_end
        jmp if_36_8_110_5_code
        if_36_8_110_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_110_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_110_5_end:
        if_36_5_110_5_end:
;        free r15
    assert_110_5_end:
;    var p1: point @ qword [rsp - 132]
;    [112:5] var p1 : point = {-x, -y}
;    [112:22] p1 ={-x, -y}
;    [112:22] {-x, -y}
;    [112:22] {-x, -y}
;    [112:23] -x
;    [112:23] -x
;    [112:25] p1.x = -x
;    alloc r15
    mov r15, qword [rsp - 88]
    mov qword [rsp - 132], r15
;    free r15
    neg qword [rsp - 132]
;    [112:27]  -y
;    [112:27] -y
;    [112:29] p1.y = -y
;    alloc r15
    mov r15, qword [rsp - 96]
    mov qword [rsp - 124], r15
;    free r15
    neg qword [rsp - 124]
;    [113:7]  o1.pos = p1
;    [113:14]  p1
;    [113:14] p1
;    alloc r15
    mov r15, qword [rsp - 132]
    mov qword [rsp - 116], r15
;    free r15
;    alloc r15
    mov r15, qword [rsp - 124]
    mov qword [rsp - 108], r15
;    free r15
;    [114:5] assert(o1.pos.x == -1)
;    alloc r15
;    [114:12] o1.pos.x == -1
;    [114:12] ? o1.pos.x == -1
;    [114:12] ? o1.pos.x == -1
    cmp_114_12:
    cmp qword [rsp - 116], -1
    jne bool_false_114_12
    jmp bool_true_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
;    assert(expr : bool) 
    assert_114_5:
;        alias expr -> r15
        if_36_8_114_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_114_5:
        cmp r15, false
        jne if_36_5_114_5_end
        jmp if_36_8_114_5_code
        if_36_8_114_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_114_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_114_5_end:
        if_36_5_114_5_end:
;        free r15
    assert_114_5_end:
;    [115:5] assert(o1.pos.y == -2)
;    alloc r15
;    [115:12] o1.pos.y == -2
;    [115:12] ? o1.pos.y == -2
;    [115:12] ? o1.pos.y == -2
    cmp_115_12:
    cmp qword [rsp - 108], -2
    jne bool_false_115_12
    jmp bool_true_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
;    assert(expr : bool) 
    assert_115_5:
;        alias expr -> r15
        if_36_8_115_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_115_5:
        cmp r15, false
        jne if_36_5_115_5_end
        jmp if_36_8_115_5_code
        if_36_8_115_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_115_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_115_5_end:
        if_36_5_115_5_end:
;        free r15
    assert_115_5_end:
;    var o2: object @ qword [rsp - 152]
;    [117:5] var o2 : object = o1
;    [117:23] o2 =o1
;    [117:23] o1
;    [117:23] o1
;        [117:23] o1
;        alloc r15
        mov r15, qword [rsp - 116]
        mov qword [rsp - 152], r15
;        free r15
;        alloc r15
        mov r15, qword [rsp - 108]
        mov qword [rsp - 144], r15
;        free r15
;    alloc r15
    mov r15d, dword [rsp - 100]
    mov dword [rsp - 136], r15d
;    free r15
;    [118:5] assert(o2.pos.x == -1)
;    alloc r15
;    [118:12] o2.pos.x == -1
;    [118:12] ? o2.pos.x == -1
;    [118:12] ? o2.pos.x == -1
    cmp_118_12:
    cmp qword [rsp - 152], -1
    jne bool_false_118_12
    jmp bool_true_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
;    assert(expr : bool) 
    assert_118_5:
;        alias expr -> r15
        if_36_8_118_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_118_5:
        cmp r15, false
        jne if_36_5_118_5_end
        jmp if_36_8_118_5_code
        if_36_8_118_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_118_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_118_5_end:
        if_36_5_118_5_end:
;        free r15
    assert_118_5_end:
;    [119:5] assert(o2.pos.y == -2)
;    alloc r15
;    [119:12] o2.pos.y == -2
;    [119:12] ? o2.pos.y == -2
;    [119:12] ? o2.pos.y == -2
    cmp_119_12:
    cmp qword [rsp - 144], -2
    jne bool_false_119_12
    jmp bool_true_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
;    assert(expr : bool) 
    assert_119_5:
;        alias expr -> r15
        if_36_8_119_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_119_5:
        cmp r15, false
        jne if_36_5_119_5_end
        jmp if_36_8_119_5_code
        if_36_8_119_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_119_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_119_5_end:
        if_36_5_119_5_end:
;        free r15
    assert_119_5_end:
;    [120:5] assert(o2.color == 0xff0000)
;    alloc r15
;    [120:12] o2.color == 0xff0000
;    [120:12] ? o2.color == 0xff0000
;    [120:12] ? o2.color == 0xff0000
    cmp_120_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_120_12
    jmp bool_true_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
;    assert(expr : bool) 
    assert_120_5:
;        alias expr -> r15
        if_36_8_120_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_120_5:
        cmp r15, false
        jne if_36_5_120_5_end
        jmp if_36_8_120_5_code
        if_36_8_120_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_120_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_120_5_end:
        if_36_5_120_5_end:
;        free r15
    assert_120_5_end:
;    [122:5] print(hello.len, hello)
;    alloc rdx
    mov rdx, hello.len
;    alloc rsi
    mov rsi, hello
;    print(len : reg_rdx, ptr : reg_rsi) 
    print_122_5:
;        alias len -> rdx
;        alias ptr -> rsi
;        [19:5]  mov(rax, 1)
        mov rax, 1
;        [19:19] # write system call
;        [20:5]  mov(rdi, 1)
        mov rdi, 1
;        [20:19] # file descriptor for standard out
;        [21:5]  mov(rsi, ptr)
;        [21:19] # buffer address
;        [22:5]  mov(rdx, len)
;        [22:19] # buffer size
;        [23:5]  syscall()
        syscall
;        free rsi
;        free rdx
    print_122_5_end:
;    [123:5] loop
    loop_123_5:
;        [124:9]  print(prompt1.len, prompt1)
;        alloc rdx
        mov rdx, prompt1.len
;        alloc rsi
        mov rsi, prompt1
;        print(len : reg_rdx, ptr : reg_rsi) 
        print_124_9:
;            alias len -> rdx
;            alias ptr -> rsi
;            [19:5]  mov(rax, 1)
            mov rax, 1
;            [19:19] # write system call
;            [20:5]  mov(rdi, 1)
            mov rdi, 1
;            [20:19] # file descriptor for standard out
;            [21:5]  mov(rsi, ptr)
;            [21:19] # buffer address
;            [22:5]  mov(rdx, len)
;            [22:19] # buffer size
;            [23:5]  syscall()
            syscall
;            free rsi
;            free rdx
        print_124_9_end:
;        var len: i64 @ qword [rsp - 160]
;        [125:9] var len = read(input.len, input) - 1
;        [125:19] len =read(input.len, input) - 1
;        [125:19] read(input.len, input) - 1
;        [125:19] read(input.len, input) - 1
;        [125:19] len = read(input.len, input)
;        [125:19] read(input.len, input)
;        alloc rdx
        mov rdx, input.len
;        alloc rsi
        mov rsi, input
;        read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_125_19:
;            alias nbytes -> len
;            alias len -> rdx
;            alias ptr -> rsi
;            [27:5]  mov(rax, 0)
            mov rax, 0
;            [27:19] # read system call
;            [28:5]  mov(rdi, 0)
            mov rdi, 0
;            [28:19] # file descriptor for standard input
;            [29:5]  mov(rsi, ptr)
;            [29:19] # buffer address
;            [30:5]  mov(rdx, len)
;            [30:19] # buffer size
;            [31:5]  syscall()
            syscall
;            [32:5] mov(nbytes, rax)
            mov qword [rsp - 160], rax
;            [32:22] # return value
;            free rsi
;            free rdx
        read_125_19_end:
;        [125:49] len - 1
        sub qword [rsp - 160], 1
;        [125:49] # -1 don't include the '\n'
        if_126_12:
;        [126:12] ? len == 0
;        [126:12] ? len == 0
        cmp_126_12:
        cmp qword [rsp - 160], 0
        jne if_128_19
        jmp if_126_12_code
        if_126_12_code:
;            [127:13]  break
            jmp loop_123_5_end
        jmp if_126_9_end
        if_128_19:
;        [128:19] ? len <= 4
;        [128:19] ? len <= 4
        cmp_128_19:
        cmp qword [rsp - 160], 4
        jg if_else_126_9
        jmp if_128_19_code
        if_128_19_code:
;            [129:13]  print(prompt2.len, prompt2)
;            alloc rdx
            mov rdx, prompt2.len
;            alloc rsi
            mov rsi, prompt2
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_129_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5]  mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5]  mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5]  mov(rdx, len)
;                [22:19] # buffer size
;                [23:5]  syscall()
                syscall
;                free rsi
;                free rdx
            print_129_13_end:
;            [130:13] continue
            jmp loop_123_5
        jmp if_126_9_end
        if_else_126_9:
;            [132:13]  print(prompt3.len, prompt3)
;            alloc rdx
            mov rdx, prompt3.len
;            alloc rsi
            mov rsi, prompt3
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_132_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5]  mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5]  mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5]  mov(rdx, len)
;                [22:19] # buffer size
;                [23:5]  syscall()
                syscall
;                free rsi
;                free rdx
            print_132_13_end:
;            [133:13] print(len, input)
;            alloc rdx
            mov rdx, qword [rsp - 160]
;            alloc rsi
            mov rsi, input
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_133_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5]  mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5]  mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5]  mov(rdx, len)
;                [22:19] # buffer size
;                [23:5]  syscall()
                syscall
;                free rsi
;                free rdx
            print_133_13_end:
;            [134:13] print(dot.len, dot)
;            alloc rdx
            mov rdx, dot.len
;            alloc rsi
            mov rsi, dot
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_134_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5]  mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5]  mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5]  mov(rdx, len)
;                [22:19] # buffer size
;                [23:5]  syscall()
                syscall
;                free rsi
;                free rdx
            print_134_13_end:
;            [135:13] print(nl.len, nl)
;            alloc rdx
            mov rdx, nl.len
;            alloc rsi
            mov rsi, nl
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_135_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5]  mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5]  mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5]  mov(rdx, len)
;                [22:19] # buffer size
;                [23:5]  syscall()
                syscall
;                free rsi
;                free rdx
            print_135_13_end:
        if_126_9_end:
    jmp loop_123_5
    loop_123_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; max scratch registers in use: 3
;            max frames in use: 7
;               max stack size: 160 B
```
