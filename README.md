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
C/C++ Header                    36            811            583           4479
C++                              1             51             67            393
-------------------------------------------------------------------------------
SUM:                            37            862            650           4872
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

# return target is specified as a variable, in this case `res`
func baz(arg) : i64 res {
    res = arg * 2
}

func main() {
    var arr : i32[4]
    arr[1] = 2
    arr[2] = arr[1]
    assert(arr[1] == 2)
    assert(arr[2] == 2)

    var ix = 3
    arr[ix] = arr[ix - 1]

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
    mov r15, 1
    lea r15, [rsp+r15*4-16]
    mov dword[r15], 2
    mov r15, 2
    lea r15, [rsp+r15*4-16]
    mov r14, 1
    lea r14, [rsp+r14*4-16]
    mov r13d, dword[r14]
    mov dword[r15], r13d
    cmp_67_12:
        mov r13, 1
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
    jne bool_false_67_12
    bool_true_67_12:
    mov r15, true
    jmp bool_end_67_12
    bool_false_67_12:
    mov r15, false
    bool_end_67_12:
    assert_67_5:
        if_36_8_67_5:
        cmp_36_8_67_5:
        cmp r15, false
        jne if_36_5_67_5_end
        if_36_8_67_5_code:
            mov rdi, 1
            exit_36_17_67_5:
                mov rax, 60
                syscall
            exit_36_17_67_5_end:
        if_36_5_67_5_end:
    assert_67_5_end:
    cmp_68_12:
        mov r13, 2
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
    jne bool_false_68_12
    bool_true_68_12:
    mov r15, true
    jmp bool_end_68_12
    bool_false_68_12:
    mov r15, false
    bool_end_68_12:
    assert_68_5:
        if_36_8_68_5:
        cmp_36_8_68_5:
        cmp r15, false
        jne if_36_5_68_5_end
        if_36_8_68_5_code:
            mov rdi, 1
            exit_36_17_68_5:
                mov rax, 60
                syscall
            exit_36_17_68_5_end:
        if_36_5_68_5_end:
    assert_68_5_end:
    mov qword[rsp-24], 3
    mov r15, qword[rsp-24]
    lea r15, [rsp+r15*4-16]
    mov r14, qword[rsp-24]
    sub r14, 1
    lea r14, [rsp+r14*4-16]
    mov r13d, dword[r14]
    mov dword[r15], r13d
    mov qword[rsp-40], 0
    mov qword[rsp-32], 0
    foo_74_5:
        mov qword[rsp-40], 0b10
        mov qword[rsp-32], 0xb
    foo_74_5_end:
    cmp_75_12:
    cmp qword[rsp-40], 2
    jne bool_false_75_12
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
        if_36_8_75_5_code:
            mov rdi, 1
            exit_36_17_75_5:
                mov rax, 60
                syscall
            exit_36_17_75_5_end:
        if_36_5_75_5_end:
    assert_75_5_end:
    cmp_76_12:
    cmp qword[rsp-32], 0xb
    jne bool_false_76_12
    bool_true_76_12:
    mov r15, true
    jmp bool_end_76_12
    bool_false_76_12:
    mov r15, false
    bool_end_76_12:
    assert_76_5:
        if_36_8_76_5:
        cmp_36_8_76_5:
        cmp r15, false
        jne if_36_5_76_5_end
        if_36_8_76_5_code:
            mov rdi, 1
            exit_36_17_76_5:
                mov rax, 60
                syscall
            exit_36_17_76_5_end:
        if_36_5_76_5_end:
    assert_76_5_end:
    mov qword[rsp-48], 0
    bar_79_5:
        if_54_8_79_5:
        cmp_54_8_79_5:
        cmp qword[rsp-48], 0
        jne if_54_5_79_5_end
        if_54_8_79_5_code:
            jmp bar_79_5_end
        if_54_5_79_5_end:
        mov qword[rsp-48], 0xff
    bar_79_5_end:
    cmp_80_12:
    cmp qword[rsp-48], 0
    jne bool_false_80_12
    bool_true_80_12:
    mov r15, true
    jmp bool_end_80_12
    bool_false_80_12:
    mov r15, false
    bool_end_80_12:
    assert_80_5:
        if_36_8_80_5:
        cmp_36_8_80_5:
        cmp r15, false
        jne if_36_5_80_5_end
        if_36_8_80_5_code:
            mov rdi, 1
            exit_36_17_80_5:
                mov rax, 60
                syscall
            exit_36_17_80_5_end:
        if_36_5_80_5_end:
    assert_80_5_end:
    mov qword[rsp-48], 1
    bar_83_5:
        if_54_8_83_5:
        cmp_54_8_83_5:
        cmp qword[rsp-48], 0
        jne if_54_5_83_5_end
        if_54_8_83_5_code:
            jmp bar_83_5_end
        if_54_5_83_5_end:
        mov qword[rsp-48], 0xff
    bar_83_5_end:
    cmp_84_12:
    cmp qword[rsp-48], 0xff
    jne bool_false_84_12
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
        if_36_8_84_5_code:
            mov rdi, 1
            exit_36_17_84_5:
                mov rax, 60
                syscall
            exit_36_17_84_5_end:
        if_36_5_84_5_end:
    assert_84_5_end:
    mov qword[rsp-56], 1
    baz_87_13:
        mov r15, qword[rsp-56]
        mov qword[rsp-64], r15
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
    baz_87_13_end:
    cmp_88_12:
    cmp qword[rsp-64], 2
    jne bool_false_88_12
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
        if_36_8_88_5_code:
            mov rdi, 1
            exit_36_17_88_5:
                mov rax, 60
                syscall
            exit_36_17_88_5_end:
        if_36_5_88_5_end:
    assert_88_5_end:
    baz_90_9:
        mov qword[rsp-64], 1
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
    baz_90_9_end:
    cmp_91_12:
    cmp qword[rsp-64], 2
    jne bool_false_91_12
    bool_true_91_12:
    mov r15, true
    jmp bool_end_91_12
    bool_false_91_12:
    mov r15, false
    bool_end_91_12:
    assert_91_5:
        if_36_8_91_5:
        cmp_36_8_91_5:
        cmp r15, false
        jne if_36_5_91_5_end
        if_36_8_91_5_code:
            mov rdi, 1
            exit_36_17_91_5:
                mov rax, 60
                syscall
            exit_36_17_91_5_end:
        if_36_5_91_5_end:
    assert_91_5_end:
    baz_93_23:
        mov qword[rsp-80], 2
        mov r15, qword[rsp-80]
        imul r15, 2
        mov qword[rsp-80], r15
    baz_93_23_end:
    mov qword[rsp-72], 0
    cmp_94_12:
    cmp qword[rsp-80], 4
    jne bool_false_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
    assert_94_5:
        if_36_8_94_5:
        cmp_36_8_94_5:
        cmp r15, false
        jne if_36_5_94_5_end
        if_36_8_94_5_code:
            mov rdi, 1
            exit_36_17_94_5:
                mov rax, 60
                syscall
            exit_36_17_94_5_end:
        if_36_5_94_5_end:
    assert_94_5_end:
    mov qword[rsp-88], 1
    mov qword[rsp-96], 2
        mov r15, qword[rsp-88]
        mov qword[rsp-116], r15
        mov r15, qword[rsp-116]
        imul r15, 10
        mov qword[rsp-116], r15
        mov r15, qword[rsp-96]
        mov qword[rsp-108], r15
    mov dword[rsp-100], 0xff0000
    cmp_100_12:
    cmp qword[rsp-116], 10
    jne bool_false_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
    assert_100_5:
        if_36_8_100_5:
        cmp_36_8_100_5:
        cmp r15, false
        jne if_36_5_100_5_end
        if_36_8_100_5_code:
            mov rdi, 1
            exit_36_17_100_5:
                mov rax, 60
                syscall
            exit_36_17_100_5_end:
        if_36_5_100_5_end:
    assert_100_5_end:
    cmp_101_12:
    cmp qword[rsp-108], 2
    jne bool_false_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
    assert_101_5:
        if_36_8_101_5:
        cmp_36_8_101_5:
        cmp r15, false
        jne if_36_5_101_5_end
        if_36_8_101_5_code:
            mov rdi, 1
            exit_36_17_101_5:
                mov rax, 60
                syscall
            exit_36_17_101_5_end:
        if_36_5_101_5_end:
    assert_101_5_end:
    cmp_102_12:
    cmp dword[rsp-100], 0xff0000
    jne bool_false_102_12
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
        if_36_8_102_5_code:
            mov rdi, 1
            exit_36_17_102_5:
                mov rax, 60
                syscall
            exit_36_17_102_5_end:
        if_36_5_102_5_end:
    assert_102_5_end:
    mov r15, qword[rsp-88]
    mov qword[rsp-132], r15
    neg qword[rsp-132]
    mov r15, qword[rsp-96]
    mov qword[rsp-124], r15
    neg qword[rsp-124]
    mov r15, qword[rsp-132]
    mov qword[rsp-116], r15
    mov r15, qword[rsp-124]
    mov qword[rsp-108], r15
    cmp_106_12:
    cmp qword[rsp-116], -1
    jne bool_false_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
    assert_106_5:
        if_36_8_106_5:
        cmp_36_8_106_5:
        cmp r15, false
        jne if_36_5_106_5_end
        if_36_8_106_5_code:
            mov rdi, 1
            exit_36_17_106_5:
                mov rax, 60
                syscall
            exit_36_17_106_5_end:
        if_36_5_106_5_end:
    assert_106_5_end:
    cmp_107_12:
    cmp qword[rsp-108], -2
    jne bool_false_107_12
    bool_true_107_12:
    mov r15, true
    jmp bool_end_107_12
    bool_false_107_12:
    mov r15, false
    bool_end_107_12:
    assert_107_5:
        if_36_8_107_5:
        cmp_36_8_107_5:
        cmp r15, false
        jne if_36_5_107_5_end
        if_36_8_107_5_code:
            mov rdi, 1
            exit_36_17_107_5:
                mov rax, 60
                syscall
            exit_36_17_107_5_end:
        if_36_5_107_5_end:
    assert_107_5_end:
        mov r15, qword[rsp-116]
        mov qword[rsp-152], r15
        mov r15, qword[rsp-108]
        mov qword[rsp-144], r15
    mov r15d, dword[rsp-100]
    mov dword[rsp-136], r15d
    cmp_110_12:
    cmp qword[rsp-152], -1
    jne bool_false_110_12
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
        if_36_8_110_5_code:
            mov rdi, 1
            exit_36_17_110_5:
                mov rax, 60
                syscall
            exit_36_17_110_5_end:
        if_36_5_110_5_end:
    assert_110_5_end:
    cmp_111_12:
    cmp qword[rsp-144], -2
    jne bool_false_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_36_8_111_5:
        cmp_36_8_111_5:
        cmp r15, false
        jne if_36_5_111_5_end
        if_36_8_111_5_code:
            mov rdi, 1
            exit_36_17_111_5:
                mov rax, 60
                syscall
            exit_36_17_111_5_end:
        if_36_5_111_5_end:
    assert_111_5_end:
    cmp_112_12:
    cmp dword[rsp-136], 0xff0000
    jne bool_false_112_12
    bool_true_112_12:
    mov r15, true
    jmp bool_end_112_12
    bool_false_112_12:
    mov r15, false
    bool_end_112_12:
    assert_112_5:
        if_36_8_112_5:
        cmp_36_8_112_5:
        cmp r15, false
        jne if_36_5_112_5_end
        if_36_8_112_5_code:
            mov rdi, 1
            exit_36_17_112_5:
                mov rax, 60
                syscall
            exit_36_17_112_5_end:
        if_36_5_112_5_end:
    assert_112_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_114_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_114_5_end:
    loop_115_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_116_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_116_9_end:
        mov rdx, input.len
        mov rsi, input
        read_117_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword[rsp-160], rax
        read_117_19_end:
        sub qword[rsp-160], 1
        if_118_12:
        cmp_118_12:
        cmp qword[rsp-160], 0
        jne if_120_19
        if_118_12_code:
            jmp loop_115_5_end
        jmp if_118_9_end
        if_120_19:
        cmp_120_19:
        cmp qword[rsp-160], 4
        jg if_else_118_9
        if_120_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_121_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_121_13_end:
            jmp loop_115_5
        jmp if_118_9_end
        if_else_118_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_124_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_124_13_end:
            mov rdx, qword[rsp-160]
            mov rsi, input
            print_125_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_125_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_126_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_126_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_127_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_127_13_end:
        if_118_9_end:
    jmp loop_115_5
    loop_115_5_end:
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
;[39:1]  # user types are defined using keyword `type`
;[41:1]  # default type is `i64` and does not need to be specified
;[46:1]  # function arguments are equivalent to mutable references
;[52:1]  # default argument type is `i64`
;[58:1]  # return target is specified as a variable, in this case `res`
main:
;    var arr: i32[4] @ dword[rsp-16]
;    [64:5]  var arr : i32[4]
;    [65:5]  arr[1] = 2
;    alloc r15
;    [65:9] 1
;    [65:9] 1
;    [65:9] r15 = 1
    mov r15, 1
    lea r15, [rsp+r15*4-16]
;    [65:14]  2
;    [65:14] 2
;    [65:14] dword[r15] = 2
    mov dword[r15], 2
;    free r15
;    [66:5] arr[2] = arr[1]
;    alloc r15
;    [66:9] 2
;    [66:9] 2
;    [66:9] r15 = 2
    mov r15, 2
    lea r15, [rsp+r15*4-16]
;    [66:14]  arr[1]
;    [66:14] arr[1]
;    [66:14] dword[r15] = arr[1]
;    alloc r14
;    [66:18] 1
;    [66:18] 1
;    [66:18] r14 = 1
    mov r14, 1
    lea r14, [rsp+r14*4-16]
;    alloc r13
    mov r13d, dword[r14]
    mov dword[r15], r13d
;    free r13
;    free r14
;    free r15
;    [67:5]  assert(arr[1] == 2)
;    alloc r15
;    [67:12] arr[1] == 2
;    [67:12] ? arr[1] == 2
;    [67:12] ? arr[1] == 2
    cmp_67_12:
;    alloc r14
;        [67:12] arr[1]
;        [67:12] r14 = arr[1]
;        alloc r13
;        [67:16] 1
;        [67:16] 1
;        [67:16] r13 = 1
        mov r13, 1
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
;        free r13
    cmp r14, 2
;    free r14
    jne bool_false_67_12
    bool_true_67_12:
    mov r15, true
    jmp bool_end_67_12
    bool_false_67_12:
    mov r15, false
    bool_end_67_12:
;    assert(expr : bool) 
    assert_67_5:
;        alias expr -> r15
        if_36_8_67_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_67_5:
        cmp r15, false
        jne if_36_5_67_5_end
        if_36_8_67_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_67_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_67_5_end:
        if_36_5_67_5_end:
;        free r15
    assert_67_5_end:
;    [68:5] assert(arr[2] == 2)
;    alloc r15
;    [68:12] arr[2] == 2
;    [68:12] ? arr[2] == 2
;    [68:12] ? arr[2] == 2
    cmp_68_12:
;    alloc r14
;        [68:12] arr[2]
;        [68:12] r14 = arr[2]
;        alloc r13
;        [68:16] 2
;        [68:16] 2
;        [68:16] r13 = 2
        mov r13, 2
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
;        free r13
    cmp r14, 2
;    free r14
    jne bool_false_68_12
    bool_true_68_12:
    mov r15, true
    jmp bool_end_68_12
    bool_false_68_12:
    mov r15, false
    bool_end_68_12:
;    assert(expr : bool) 
    assert_68_5:
;        alias expr -> r15
        if_36_8_68_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_68_5:
        cmp r15, false
        jne if_36_5_68_5_end
        if_36_8_68_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_68_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_68_5_end:
        if_36_5_68_5_end:
;        free r15
    assert_68_5_end:
;    var ix: i64[1] @ qword[rsp-24]
;    [70:5] var ix = 3
;    [70:9] ix = 3
;    [70:14]  3
;    [70:14] 3
;    [70:14] ix = 3
    mov qword[rsp-24], 3
;    [71:5] arr[ix] = arr[ix - 1]
;    alloc r15
;    [71:9] ix
;    [71:9] ix
;    [71:9] r15 = ix
    mov r15, qword[rsp-24]
    lea r15, [rsp+r15*4-16]
;    [71:15]  arr[ix - 1]
;    [71:15] arr[ix - 1]
;    [71:15] dword[r15] = arr[ix - 1]
;    alloc r14
;    [71:19] ix - 1
;    [71:19] ix - 1
;    [71:19] r14 = ix
    mov r14, qword[rsp-24]
;    [71:24] r14 - 1
    sub r14, 1
    lea r14, [rsp+r14*4-16]
;    alloc r13
    mov r13d, dword[r14]
    mov dword[r15], r13d
;    free r13
;    free r14
;    free r15
;    var p: point[1] @ qword[rsp-40]
;    [73:5]  var p : point = {0, 0}
;    [73:9] p = {0, 0}
;    [73:21]  {0, 0}
;    [73:21] {0, 0}
;    [73:22] 0
;    [73:22] 0
;    [73:22] p.x = 0
    mov qword[rsp-40], 0
;    [73:25]  0
;    [73:25] 0
;    [73:25] p.y = 0
    mov qword[rsp-32], 0
;    [74:5]  foo(p)
;    foo(pt : point) 
    foo_74_5:
;        alias pt -> p
;        [48:5]  pt.x = 0b10
;        [48:12]  0b10
;        [48:12] 0b10
;        [48:12] pt.x = 0b10
        mov qword[rsp-40], 0b10
;        [48:20] # binary value 2
;        [49:5]  pt.y = 0xb
;        [49:12]  0xb
;        [49:12] 0xb
;        [49:12] pt.y = 0xb
        mov qword[rsp-32], 0xb
;        [49:20] # hex value 11
    foo_74_5_end:
;    [75:5] assert(p.x == 2)
;    alloc r15
;    [75:12] p.x == 2
;    [75:12] ? p.x == 2
;    [75:12] ? p.x == 2
    cmp_75_12:
    cmp qword[rsp-40], 2
    jne bool_false_75_12
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
;    [76:5] assert(p.y == 0xb)
;    alloc r15
;    [76:12] p.y == 0xb
;    [76:12] ? p.y == 0xb
;    [76:12] ? p.y == 0xb
    cmp_76_12:
    cmp qword[rsp-32], 0xb
    jne bool_false_76_12
    bool_true_76_12:
    mov r15, true
    jmp bool_end_76_12
    bool_false_76_12:
    mov r15, false
    bool_end_76_12:
;    assert(expr : bool) 
    assert_76_5:
;        alias expr -> r15
        if_36_8_76_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_76_5:
        cmp r15, false
        jne if_36_5_76_5_end
        if_36_8_76_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_76_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_76_5_end:
        if_36_5_76_5_end:
;        free r15
    assert_76_5_end:
;    var i: i64[1] @ qword[rsp-48]
;    [78:5] var i = 0
;    [78:9] i = 0
;    [78:13]  0
;    [78:13] 0
;    [78:13] i = 0
    mov qword[rsp-48], 0
;    [79:5] bar(i)
;    bar(arg) 
    bar_79_5:
;        alias arg -> i
        if_54_8_79_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_79_5:
        cmp qword[rsp-48], 0
        jne if_54_5_79_5_end
        if_54_8_79_5_code:
;            [54:17] return
            jmp bar_79_5_end
        if_54_5_79_5_end:
;        [55:5] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [55:11] arg = 0xff
        mov qword[rsp-48], 0xff
    bar_79_5_end:
;    [80:5] assert(i == 0)
;    alloc r15
;    [80:12] i == 0
;    [80:12] ? i == 0
;    [80:12] ? i == 0
    cmp_80_12:
    cmp qword[rsp-48], 0
    jne bool_false_80_12
    bool_true_80_12:
    mov r15, true
    jmp bool_end_80_12
    bool_false_80_12:
    mov r15, false
    bool_end_80_12:
;    assert(expr : bool) 
    assert_80_5:
;        alias expr -> r15
        if_36_8_80_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_80_5:
        cmp r15, false
        jne if_36_5_80_5_end
        if_36_8_80_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_80_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_80_5_end:
        if_36_5_80_5_end:
;        free r15
    assert_80_5_end:
;    [82:5] i = 1
;    [82:9]  1
;    [82:9] 1
;    [82:9] i = 1
    mov qword[rsp-48], 1
;    [83:5] bar(i)
;    bar(arg) 
    bar_83_5:
;        alias arg -> i
        if_54_8_83_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_83_5:
        cmp qword[rsp-48], 0
        jne if_54_5_83_5_end
        if_54_8_83_5_code:
;            [54:17] return
            jmp bar_83_5_end
        if_54_5_83_5_end:
;        [55:5] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [55:11] arg = 0xff
        mov qword[rsp-48], 0xff
    bar_83_5_end:
;    [84:5] assert(i == 0xff)
;    alloc r15
;    [84:12] i == 0xff
;    [84:12] ? i == 0xff
;    [84:12] ? i == 0xff
    cmp_84_12:
    cmp qword[rsp-48], 0xff
    jne bool_false_84_12
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
;    var j: i64[1] @ qword[rsp-56]
;    [86:5] var j = 1
;    [86:9] j = 1
;    [86:13]  1
;    [86:13] 1
;    [86:13] j = 1
    mov qword[rsp-56], 1
;    var k: i64[1] @ qword[rsp-64]
;    [87:5] var k = baz(j)
;    [87:9] k = baz(j)
;    [87:13]  baz(j)
;    [87:13] baz(j)
;    [87:13] k = baz(j)
;    [87:13] baz(j)
;    baz(arg) : i64 res 
    baz_87_13:
;        alias res -> k
;        alias arg -> j
;        [60:5]  res = arg * 2
;        [60:11]  arg * 2
;        [60:11] arg * 2
;        [60:11] res = arg
;        alloc r15
        mov r15, qword[rsp-56]
        mov qword[rsp-64], r15
;        free r15
;        [60:17] res * 2
;        alloc r15
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
;        free r15
    baz_87_13_end:
;    [88:5] assert(k == 2)
;    alloc r15
;    [88:12] k == 2
;    [88:12] ? k == 2
;    [88:12] ? k == 2
    cmp_88_12:
    cmp qword[rsp-64], 2
    jne bool_false_88_12
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
;    [90:5] k = baz(1)
;    [90:9]  baz(1)
;    [90:9] baz(1)
;    [90:9] k = baz(1)
;    [90:9] baz(1)
;    baz(arg) : i64 res 
    baz_90_9:
;        alias res -> k
;        alias arg -> 1
;        [60:5]  res = arg * 2
;        [60:11]  arg * 2
;        [60:11] arg * 2
;        [60:11] res = arg
        mov qword[rsp-64], 1
;        [60:17] res * 2
;        alloc r15
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
;        free r15
    baz_90_9_end:
;    [91:5] assert(k == 2)
;    alloc r15
;    [91:12] k == 2
;    [91:12] ? k == 2
;    [91:12] ? k == 2
    cmp_91_12:
    cmp qword[rsp-64], 2
    jne bool_false_91_12
    bool_true_91_12:
    mov r15, true
    jmp bool_end_91_12
    bool_false_91_12:
    mov r15, false
    bool_end_91_12:
;    assert(expr : bool) 
    assert_91_5:
;        alias expr -> r15
        if_36_8_91_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_91_5:
        cmp r15, false
        jne if_36_5_91_5_end
        if_36_8_91_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_91_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_91_5_end:
        if_36_5_91_5_end:
;        free r15
    assert_91_5_end:
;    var p0: point[1] @ qword[rsp-80]
;    [93:5] var p0 : point = {baz(2), 0}
;    [93:9] p0 = {baz(2), 0}
;    [93:22]  {baz(2), 0}
;    [93:22] {baz(2), 0}
;    [93:23] baz(2)
;    [93:23] baz(2)
;    [93:23] p0.x = baz(2)
;    [93:23] baz(2)
;    baz(arg) : i64 res 
    baz_93_23:
;        alias res -> p0.x
;        alias arg -> 2
;        [60:5]  res = arg * 2
;        [60:11]  arg * 2
;        [60:11] arg * 2
;        [60:11] res = arg
        mov qword[rsp-80], 2
;        [60:17] res * 2
;        alloc r15
        mov r15, qword[rsp-80]
        imul r15, 2
        mov qword[rsp-80], r15
;        free r15
    baz_93_23_end:
;    [93:31]  0
;    [93:31] 0
;    [93:31] p0.y = 0
    mov qword[rsp-72], 0
;    [94:5]  assert(p0.x == 4)
;    alloc r15
;    [94:12] p0.x == 4
;    [94:12] ? p0.x == 4
;    [94:12] ? p0.x == 4
    cmp_94_12:
    cmp qword[rsp-80], 4
    jne bool_false_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
;    assert(expr : bool) 
    assert_94_5:
;        alias expr -> r15
        if_36_8_94_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_94_5:
        cmp r15, false
        jne if_36_5_94_5_end
        if_36_8_94_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_94_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_94_5_end:
        if_36_5_94_5_end:
;        free r15
    assert_94_5_end:
;    var x: i64[1] @ qword[rsp-88]
;    [96:5] var x = 1
;    [96:9] x = 1
;    [96:13]  1
;    [96:13] 1
;    [96:13] x = 1
    mov qword[rsp-88], 1
;    var y: i64[1] @ qword[rsp-96]
;    [97:5] var y = 2
;    [97:9] y = 2
;    [97:13]  2
;    [97:13] 2
;    [97:13] y = 2
    mov qword[rsp-96], 2
;    var o1: object[1] @ qword[rsp-116]
;    [99:5] var o1 : object = {{x * 10, y}, 0xff0000}
;    [99:9] o1 = {{x * 10, y}, 0xff0000}
;    [99:23]  {{x * 10, y}, 0xff0000}
;    [99:23] {{x * 10, y}, 0xff0000}
;        [99:24] {x * 10, y}
;        [99:25] x * 10
;        [99:25] x * 10
;        [99:25] o1.pos.x = x
;        alloc r15
        mov r15, qword[rsp-88]
        mov qword[rsp-116], r15
;        free r15
;        [99:29] o1.pos.x * 10
;        alloc r15
        mov r15, qword[rsp-116]
        imul r15, 10
        mov qword[rsp-116], r15
;        free r15
;        [99:33]  y
;        [99:33] y
;        [99:33] o1.pos.y = y
;        alloc r15
        mov r15, qword[rsp-96]
        mov qword[rsp-108], r15
;        free r15
;    [99:37]  0xff0000
;    [99:37] 0xff0000
;    [99:37] o1.color = 0xff0000
    mov dword[rsp-100], 0xff0000
;    [100:5]  assert(o1.pos.x == 10)
;    alloc r15
;    [100:12] o1.pos.x == 10
;    [100:12] ? o1.pos.x == 10
;    [100:12] ? o1.pos.x == 10
    cmp_100_12:
    cmp qword[rsp-116], 10
    jne bool_false_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
;    assert(expr : bool) 
    assert_100_5:
;        alias expr -> r15
        if_36_8_100_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_100_5:
        cmp r15, false
        jne if_36_5_100_5_end
        if_36_8_100_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_100_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_100_5_end:
        if_36_5_100_5_end:
;        free r15
    assert_100_5_end:
;    [101:5] assert(o1.pos.y == 2)
;    alloc r15
;    [101:12] o1.pos.y == 2
;    [101:12] ? o1.pos.y == 2
;    [101:12] ? o1.pos.y == 2
    cmp_101_12:
    cmp qword[rsp-108], 2
    jne bool_false_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
;    assert(expr : bool) 
    assert_101_5:
;        alias expr -> r15
        if_36_8_101_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_101_5:
        cmp r15, false
        jne if_36_5_101_5_end
        if_36_8_101_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_101_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_101_5_end:
        if_36_5_101_5_end:
;        free r15
    assert_101_5_end:
;    [102:5] assert(o1.color == 0xff0000)
;    alloc r15
;    [102:12] o1.color == 0xff0000
;    [102:12] ? o1.color == 0xff0000
;    [102:12] ? o1.color == 0xff0000
    cmp_102_12:
    cmp dword[rsp-100], 0xff0000
    jne bool_false_102_12
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
;    var p1: point[1] @ qword[rsp-132]
;    [104:5] var p1 : point = {-x, -y}
;    [104:9] p1 = {-x, -y}
;    [104:22]  {-x, -y}
;    [104:22] {-x, -y}
;    [104:23] -x
;    [104:23] -x
;    [104:24] p1.x = -x
;    alloc r15
    mov r15, qword[rsp-88]
    mov qword[rsp-132], r15
;    free r15
    neg qword[rsp-132]
;    [104:27]  -y
;    [104:27] -y
;    [104:28] p1.y = -y
;    alloc r15
    mov r15, qword[rsp-96]
    mov qword[rsp-124], r15
;    free r15
    neg qword[rsp-124]
;    [105:5]  o1.pos = p1
;    [105:14]  p1
;    [105:14] p1
;    alloc r15
    mov r15, qword[rsp-132]
    mov qword[rsp-116], r15
;    free r15
;    alloc r15
    mov r15, qword[rsp-124]
    mov qword[rsp-108], r15
;    free r15
;    [106:5] assert(o1.pos.x == -1)
;    alloc r15
;    [106:12] o1.pos.x == -1
;    [106:12] ? o1.pos.x == -1
;    [106:12] ? o1.pos.x == -1
    cmp_106_12:
    cmp qword[rsp-116], -1
    jne bool_false_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
;    assert(expr : bool) 
    assert_106_5:
;        alias expr -> r15
        if_36_8_106_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_106_5:
        cmp r15, false
        jne if_36_5_106_5_end
        if_36_8_106_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_106_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_106_5_end:
        if_36_5_106_5_end:
;        free r15
    assert_106_5_end:
;    [107:5] assert(o1.pos.y == -2)
;    alloc r15
;    [107:12] o1.pos.y == -2
;    [107:12] ? o1.pos.y == -2
;    [107:12] ? o1.pos.y == -2
    cmp_107_12:
    cmp qword[rsp-108], -2
    jne bool_false_107_12
    bool_true_107_12:
    mov r15, true
    jmp bool_end_107_12
    bool_false_107_12:
    mov r15, false
    bool_end_107_12:
;    assert(expr : bool) 
    assert_107_5:
;        alias expr -> r15
        if_36_8_107_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_107_5:
        cmp r15, false
        jne if_36_5_107_5_end
        if_36_8_107_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_107_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_107_5_end:
        if_36_5_107_5_end:
;        free r15
    assert_107_5_end:
;    var o2: object[1] @ qword[rsp-152]
;    [109:5] var o2 : object = o1
;    [109:9] o2 = o1
;    [109:23]  o1
;    [109:23] o1
;        [109:23] o1
;        alloc r15
        mov r15, qword[rsp-116]
        mov qword[rsp-152], r15
;        free r15
;        alloc r15
        mov r15, qword[rsp-108]
        mov qword[rsp-144], r15
;        free r15
;    alloc r15
    mov r15d, dword[rsp-100]
    mov dword[rsp-136], r15d
;    free r15
;    [110:5] assert(o2.pos.x == -1)
;    alloc r15
;    [110:12] o2.pos.x == -1
;    [110:12] ? o2.pos.x == -1
;    [110:12] ? o2.pos.x == -1
    cmp_110_12:
    cmp qword[rsp-152], -1
    jne bool_false_110_12
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
;    [111:5] assert(o2.pos.y == -2)
;    alloc r15
;    [111:12] o2.pos.y == -2
;    [111:12] ? o2.pos.y == -2
;    [111:12] ? o2.pos.y == -2
    cmp_111_12:
    cmp qword[rsp-144], -2
    jne bool_false_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
;    assert(expr : bool) 
    assert_111_5:
;        alias expr -> r15
        if_36_8_111_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_111_5:
        cmp r15, false
        jne if_36_5_111_5_end
        if_36_8_111_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_111_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_111_5_end:
        if_36_5_111_5_end:
;        free r15
    assert_111_5_end:
;    [112:5] assert(o2.color == 0xff0000)
;    alloc r15
;    [112:12] o2.color == 0xff0000
;    [112:12] ? o2.color == 0xff0000
;    [112:12] ? o2.color == 0xff0000
    cmp_112_12:
    cmp dword[rsp-136], 0xff0000
    jne bool_false_112_12
    bool_true_112_12:
    mov r15, true
    jmp bool_end_112_12
    bool_false_112_12:
    mov r15, false
    bool_end_112_12:
;    assert(expr : bool) 
    assert_112_5:
;        alias expr -> r15
        if_36_8_112_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_112_5:
        cmp r15, false
        jne if_36_5_112_5_end
        if_36_8_112_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_112_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_112_5_end:
        if_36_5_112_5_end:
;        free r15
    assert_112_5_end:
;    [114:5] print(hello.len, hello)
;    alloc rdx
    mov rdx, hello.len
;    alloc rsi
    mov rsi, hello
;    print(len : reg_rdx, ptr : reg_rsi) 
    print_114_5:
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
    print_114_5_end:
;    [115:5] loop
    loop_115_5:
;        [116:9]  print(prompt1.len, prompt1)
;        alloc rdx
        mov rdx, prompt1.len
;        alloc rsi
        mov rsi, prompt1
;        print(len : reg_rdx, ptr : reg_rsi) 
        print_116_9:
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
        print_116_9_end:
;        var len: i64[1] @ qword[rsp-160]
;        [117:9] var len = read(input.len, input) - 1
;        [117:13] len = read(input.len, input) - 1
;        [117:19]  read(input.len, input) - 1
;        [117:19] read(input.len, input) - 1
;        [117:19] len = read(input.len, input)
;        [117:19] read(input.len, input)
;        alloc rdx
        mov rdx, input.len
;        alloc rsi
        mov rsi, input
;        read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_117_19:
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
            mov qword[rsp-160], rax
;            [32:22] # return value
;            free rsi
;            free rdx
        read_117_19_end:
;        [117:44] len - 1
        sub qword[rsp-160], 1
;        [117:49] # -1 don't include the '\n'
        if_118_12:
;        [118:12] ? len == 0
;        [118:12] ? len == 0
        cmp_118_12:
        cmp qword[rsp-160], 0
        jne if_120_19
        if_118_12_code:
;            [119:13]  break
            jmp loop_115_5_end
        jmp if_118_9_end
        if_120_19:
;        [120:19] ? len <= 4
;        [120:19] ? len <= 4
        cmp_120_19:
        cmp qword[rsp-160], 4
        jg if_else_118_9
        if_120_19_code:
;            [121:13]  print(prompt2.len, prompt2)
;            alloc rdx
            mov rdx, prompt2.len
;            alloc rsi
            mov rsi, prompt2
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_121_13:
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
            print_121_13_end:
;            [122:13] continue
            jmp loop_115_5
        jmp if_118_9_end
        if_else_118_9:
;            [124:13]  print(prompt3.len, prompt3)
;            alloc rdx
            mov rdx, prompt3.len
;            alloc rsi
            mov rsi, prompt3
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_124_13:
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
            print_124_13_end:
;            [125:13] print(len, input)
;            alloc rdx
            mov rdx, qword[rsp-160]
;            alloc rsi
            mov rsi, input
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_125_13:
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
            print_125_13_end:
;            [126:13] print(dot.len, dot)
;            alloc rdx
            mov rdx, dot.len
;            alloc rsi
            mov rsi, dot
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_126_13:
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
            print_126_13_end:
;            [127:13] print(nl.len, nl)
;            alloc rdx
            mov rdx, nl.len
;            alloc rsi
            mov rsi, nl
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_127_13:
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
            print_127_13_end:
        if_118_9_end:
    jmp loop_115_5
    loop_115_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; max scratch registers in use: 3
;            max frames in use: 7
;               max stack size: 160 B
```
