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

func inv(i : i32) : i32 res {
    res = ~i
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
    mov r15, 1
    lea r15, [rsp+r15*4-16]
    mov dword[r15], 2
    mov r15, 2
    lea r15, [rsp+r15*4-16]
    mov r14, 1
    lea r14, [rsp+r14*4-16]
    mov r13d, dword[r14]
    mov dword[r15], r13d
    cmp_71_12:
        mov r13, 1
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
    jne bool_false_71_12
    bool_true_71_12:
    mov r15, true
    jmp bool_end_71_12
    bool_false_71_12:
    mov r15, false
    bool_end_71_12:
    assert_71_5:
        if_36_8_71_5:
        cmp_36_8_71_5:
        cmp r15, false
        jne if_36_5_71_5_end
        if_36_8_71_5_code:
            mov rdi, 1
            exit_36_17_71_5:
                mov rax, 60
                syscall
            exit_36_17_71_5_end:
        if_36_5_71_5_end:
    assert_71_5_end:
    cmp_72_12:
        mov r13, 2
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
    jne bool_false_72_12
    bool_true_72_12:
    mov r15, true
    jmp bool_end_72_12
    bool_false_72_12:
    mov r15, false
    bool_end_72_12:
    assert_72_5:
        if_36_8_72_5:
        cmp_36_8_72_5:
        cmp r15, false
        jne if_36_5_72_5_end
        if_36_8_72_5_code:
            mov rdi, 1
            exit_36_17_72_5:
                mov rax, 60
                syscall
            exit_36_17_72_5_end:
        if_36_5_72_5_end:
    assert_72_5_end:
    mov qword[rsp-24], 3
    mov r15, qword[rsp-24]
    lea r15, [rsp+r15*4-16]
    mov r13, qword[rsp-24]
    sub r13, 1
    lea r13, [rsp+r13*4-16]
    movsx r14, dword[r13]
    inv_75_16:
        mov dword[r15], r14d
        not dword[r15]
    inv_75_16_end:
    not dword[r15]
    cmp_76_12:
        mov r13, qword[rsp-24]
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
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
    mov qword[rsp-40], 0
    mov qword[rsp-32], 0
    foo_79_5:
        mov qword[rsp-40], 0b10
        mov qword[rsp-32], 0xb
    foo_79_5_end:
    cmp_80_12:
    cmp qword[rsp-40], 2
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
    cmp_81_12:
    cmp qword[rsp-32], 0xb
    jne bool_false_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
    assert_81_5:
        if_36_8_81_5:
        cmp_36_8_81_5:
        cmp r15, false
        jne if_36_5_81_5_end
        if_36_8_81_5_code:
            mov rdi, 1
            exit_36_17_81_5:
                mov rax, 60
                syscall
            exit_36_17_81_5_end:
        if_36_5_81_5_end:
    assert_81_5_end:
    mov qword[rsp-48], 0
    bar_84_5:
        if_54_8_84_5:
        cmp_54_8_84_5:
        cmp qword[rsp-48], 0
        jne if_54_5_84_5_end
        if_54_8_84_5_code:
            jmp bar_84_5_end
        if_54_5_84_5_end:
        mov qword[rsp-48], 0xff
    bar_84_5_end:
    cmp_85_12:
    cmp qword[rsp-48], 0
    jne bool_false_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
    assert_85_5:
        if_36_8_85_5:
        cmp_36_8_85_5:
        cmp r15, false
        jne if_36_5_85_5_end
        if_36_8_85_5_code:
            mov rdi, 1
            exit_36_17_85_5:
                mov rax, 60
                syscall
            exit_36_17_85_5_end:
        if_36_5_85_5_end:
    assert_85_5_end:
    mov qword[rsp-48], 1
    bar_88_5:
        if_54_8_88_5:
        cmp_54_8_88_5:
        cmp qword[rsp-48], 0
        jne if_54_5_88_5_end
        if_54_8_88_5_code:
            jmp bar_88_5_end
        if_54_5_88_5_end:
        mov qword[rsp-48], 0xff
    bar_88_5_end:
    cmp_89_12:
    cmp qword[rsp-48], 0xff
    jne bool_false_89_12
    bool_true_89_12:
    mov r15, true
    jmp bool_end_89_12
    bool_false_89_12:
    mov r15, false
    bool_end_89_12:
    assert_89_5:
        if_36_8_89_5:
        cmp_36_8_89_5:
        cmp r15, false
        jne if_36_5_89_5_end
        if_36_8_89_5_code:
            mov rdi, 1
            exit_36_17_89_5:
                mov rax, 60
                syscall
            exit_36_17_89_5_end:
        if_36_5_89_5_end:
    assert_89_5_end:
    mov qword[rsp-56], 1
    baz_92_13:
        mov r15, qword[rsp-56]
        mov qword[rsp-64], r15
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
    baz_92_13_end:
    cmp_93_12:
    cmp qword[rsp-64], 2
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
    assert_93_5:
        if_36_8_93_5:
        cmp_36_8_93_5:
        cmp r15, false
        jne if_36_5_93_5_end
        if_36_8_93_5_code:
            mov rdi, 1
            exit_36_17_93_5:
                mov rax, 60
                syscall
            exit_36_17_93_5_end:
        if_36_5_93_5_end:
    assert_93_5_end:
    baz_95_9:
        mov qword[rsp-64], 1
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
    baz_95_9_end:
    cmp_96_12:
    cmp qword[rsp-64], 2
    jne bool_false_96_12
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
        if_36_8_96_5_code:
            mov rdi, 1
            exit_36_17_96_5:
                mov rax, 60
                syscall
            exit_36_17_96_5_end:
        if_36_5_96_5_end:
    assert_96_5_end:
    baz_98_23:
        mov qword[rsp-80], 2
        mov r15, qword[rsp-80]
        imul r15, 2
        mov qword[rsp-80], r15
    baz_98_23_end:
    mov qword[rsp-72], 0
    cmp_99_12:
    cmp qword[rsp-80], 4
    jne bool_false_99_12
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
        if_36_8_99_5_code:
            mov rdi, 1
            exit_36_17_99_5:
                mov rax, 60
                syscall
            exit_36_17_99_5_end:
        if_36_5_99_5_end:
    assert_99_5_end:
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
    cmp_105_12:
    cmp qword[rsp-116], 10
    jne bool_false_105_12
    bool_true_105_12:
    mov r15, true
    jmp bool_end_105_12
    bool_false_105_12:
    mov r15, false
    bool_end_105_12:
    assert_105_5:
        if_36_8_105_5:
        cmp_36_8_105_5:
        cmp r15, false
        jne if_36_5_105_5_end
        if_36_8_105_5_code:
            mov rdi, 1
            exit_36_17_105_5:
                mov rax, 60
                syscall
            exit_36_17_105_5_end:
        if_36_5_105_5_end:
    assert_105_5_end:
    cmp_106_12:
    cmp qword[rsp-108], 2
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
    cmp dword[rsp-100], 0xff0000
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
    cmp_111_12:
    cmp qword[rsp-116], -1
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
    cmp qword[rsp-108], -2
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
        mov r15, qword[rsp-116]
        mov qword[rsp-152], r15
        mov r15, qword[rsp-108]
        mov qword[rsp-144], r15
    mov r15d, dword[rsp-100]
    mov dword[rsp-136], r15d
    cmp_115_12:
    cmp qword[rsp-152], -1
    jne bool_false_115_12
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
        if_36_8_115_5_code:
            mov rdi, 1
            exit_36_17_115_5:
                mov rax, 60
                syscall
            exit_36_17_115_5_end:
        if_36_5_115_5_end:
    assert_115_5_end:
    cmp_116_12:
    cmp qword[rsp-144], -2
    jne bool_false_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
    assert_116_5:
        if_36_8_116_5:
        cmp_36_8_116_5:
        cmp r15, false
        jne if_36_5_116_5_end
        if_36_8_116_5_code:
            mov rdi, 1
            exit_36_17_116_5:
                mov rax, 60
                syscall
            exit_36_17_116_5_end:
        if_36_5_116_5_end:
    assert_116_5_end:
    cmp_117_12:
    cmp dword[rsp-136], 0xff0000
    jne bool_false_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
    assert_117_5:
        if_36_8_117_5:
        cmp_36_8_117_5:
        cmp r15, false
        jne if_36_5_117_5_end
        if_36_8_117_5_code:
            mov rdi, 1
            exit_36_17_117_5:
                mov rax, 60
                syscall
            exit_36_17_117_5_end:
        if_36_5_117_5_end:
    assert_117_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_119_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_119_5_end:
    loop_120_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_121_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_121_9_end:
        mov rdx, input.len
        mov rsi, input
        read_122_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword[rsp-160], rax
        read_122_19_end:
        sub qword[rsp-160], 1
        if_123_12:
        cmp_123_12:
        cmp qword[rsp-160], 0
        jne if_125_19
        if_123_12_code:
            jmp loop_120_5_end
        jmp if_123_9_end
        if_125_19:
        cmp_125_19:
        cmp qword[rsp-160], 4
        jg if_else_123_9
        if_125_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_126_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_126_13_end:
            jmp loop_120_5
        jmp if_123_9_end
        if_else_123_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_129_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_129_13_end:
            mov rdx, qword[rsp-160]
            mov rsi, input
            print_130_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_130_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_131_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_131_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_132_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_132_13_end:
        if_123_9_end:
    jmp loop_120_5
    loop_120_5_end:
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
;[62:1]  # return target is specified as a variable, in this case `res`
main:
;    var arr: i32[4] @ dword[rsp-16]
;    [68:5]  var arr : i32[4]
;    [69:5]  arr[1] = 2
;    alloc r15
;    [69:9] 1
;    [69:9] 1
;    [69:9] r15 = 1
    mov r15, 1
    lea r15, [rsp+r15*4-16]
;    [69:14]  2
;    [69:14] 2
;    [69:14] dword[r15] = 2
    mov dword[r15], 2
;    free r15
;    [70:5] arr[2] = arr[1]
;    alloc r15
;    [70:9] 2
;    [70:9] 2
;    [70:9] r15 = 2
    mov r15, 2
    lea r15, [rsp+r15*4-16]
;    [70:14]  arr[1]
;    [70:14] arr[1]
;    [70:14] dword[r15] = arr[1]
;    alloc r14
;    [70:18] 1
;    [70:18] 1
;    [70:18] r14 = 1
    mov r14, 1
    lea r14, [rsp+r14*4-16]
;    alloc r13
    mov r13d, dword[r14]
    mov dword[r15], r13d
;    free r13
;    free r14
;    free r15
;    [71:5]  assert(arr[1] == 2)
;    alloc r15
;    [71:12] arr[1] == 2
;    [71:12] ? arr[1] == 2
;    [71:12] ? arr[1] == 2
    cmp_71_12:
;    alloc r14
;        [71:12] arr[1]
;        [71:12] r14 = arr[1]
;        alloc r13
;        [71:16] 1
;        [71:16] 1
;        [71:16] r13 = 1
        mov r13, 1
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
;        free r13
    cmp r14, 2
;    free r14
    jne bool_false_71_12
    bool_true_71_12:
    mov r15, true
    jmp bool_end_71_12
    bool_false_71_12:
    mov r15, false
    bool_end_71_12:
;    assert(expr : bool) 
    assert_71_5:
;        alias expr -> r15
        if_36_8_71_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_71_5:
        cmp r15, false
        jne if_36_5_71_5_end
        if_36_8_71_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_71_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_71_5_end:
        if_36_5_71_5_end:
;        free r15
    assert_71_5_end:
;    [72:5] assert(arr[2] == 2)
;    alloc r15
;    [72:12] arr[2] == 2
;    [72:12] ? arr[2] == 2
;    [72:12] ? arr[2] == 2
    cmp_72_12:
;    alloc r14
;        [72:12] arr[2]
;        [72:12] r14 = arr[2]
;        alloc r13
;        [72:16] 2
;        [72:16] 2
;        [72:16] r13 = 2
        mov r13, 2
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
;        free r13
    cmp r14, 2
;    free r14
    jne bool_false_72_12
    bool_true_72_12:
    mov r15, true
    jmp bool_end_72_12
    bool_false_72_12:
    mov r15, false
    bool_end_72_12:
;    assert(expr : bool) 
    assert_72_5:
;        alias expr -> r15
        if_36_8_72_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_72_5:
        cmp r15, false
        jne if_36_5_72_5_end
        if_36_8_72_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_72_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_72_5_end:
        if_36_5_72_5_end:
;        free r15
    assert_72_5_end:
;    var ix: i64[1] @ qword[rsp-24]
;    [74:5] var ix = 3
;    [74:9] ix = 3
;    [74:14]  3
;    [74:14] 3
;    [74:14] ix = 3
    mov qword[rsp-24], 3
;    [75:5] arr[ix] = ~inv(arr[ix - 1])
;    alloc r15
;    [75:9] ix
;    [75:9] ix
;    [75:9] r15 = ix
    mov r15, qword[rsp-24]
    lea r15, [rsp+r15*4-16]
;    [75:15]  ~inv(arr[ix - 1])
;    [75:15] ~inv(arr[ix - 1])
;    [75:16] dword[r15] = ~inv(arr[ix - 1])
;    [75:16] ~inv(arr[ix - 1])
;    alloc r14
;    [75:20] arr[ix - 1]
;    [75:20] arr[ix - 1]
;    [75:20] r14 = arr[ix - 1]
;    alloc r13
;    [75:24] ix - 1
;    [75:24] ix - 1
;    [75:24] r13 = ix
    mov r13, qword[rsp-24]
;    [75:29] r13 - 1
    sub r13, 1
    lea r13, [rsp+r13*4-16]
    movsx r14, dword[r13]
;    free r13
;    inv(i : i32) : i32 res 
    inv_75_16:
;        alias res -> dword[r15]
;        alias i -> r14
;        [59:5]  res = ~i
;        [59:11]  ~i
;        [59:11] ~i
;        [59:12] res = ~i
        mov dword[r15], r14d
        not dword[r15]
;        free r14
    inv_75_16_end:
    not dword[r15]
;    free r15
;    [76:5] assert(arr[ix] == 2)
;    alloc r15
;    [76:12] arr[ix] == 2
;    [76:12] ? arr[ix] == 2
;    [76:12] ? arr[ix] == 2
    cmp_76_12:
;    alloc r14
;        [76:12] arr[ix]
;        [76:12] r14 = arr[ix]
;        alloc r13
;        [76:16] ix
;        [76:16] ix
;        [76:16] r13 = ix
        mov r13, qword[rsp-24]
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
;        free r13
    cmp r14, 2
;    free r14
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
;    var p: point[1] @ qword[rsp-40]
;    [78:5] var p : point = {0, 0}
;    [78:9] p = {0, 0}
;    [78:21]  {0, 0}
;    [78:21] {0, 0}
;    [78:22] 0
;    [78:22] 0
;    [78:22] p.x = 0
    mov qword[rsp-40], 0
;    [78:25]  0
;    [78:25] 0
;    [78:25] p.y = 0
    mov qword[rsp-32], 0
;    [79:5]  foo(p)
;    foo(pt : point) 
    foo_79_5:
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
    foo_79_5_end:
;    [80:5] assert(p.x == 2)
;    alloc r15
;    [80:12] p.x == 2
;    [80:12] ? p.x == 2
;    [80:12] ? p.x == 2
    cmp_80_12:
    cmp qword[rsp-40], 2
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
;    [81:5] assert(p.y == 0xb)
;    alloc r15
;    [81:12] p.y == 0xb
;    [81:12] ? p.y == 0xb
;    [81:12] ? p.y == 0xb
    cmp_81_12:
    cmp qword[rsp-32], 0xb
    jne bool_false_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
;    assert(expr : bool) 
    assert_81_5:
;        alias expr -> r15
        if_36_8_81_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_81_5:
        cmp r15, false
        jne if_36_5_81_5_end
        if_36_8_81_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_81_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_81_5_end:
        if_36_5_81_5_end:
;        free r15
    assert_81_5_end:
;    var i: i64[1] @ qword[rsp-48]
;    [83:5] var i = 0
;    [83:9] i = 0
;    [83:13]  0
;    [83:13] 0
;    [83:13] i = 0
    mov qword[rsp-48], 0
;    [84:5] bar(i)
;    bar(arg) 
    bar_84_5:
;        alias arg -> i
        if_54_8_84_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_84_5:
        cmp qword[rsp-48], 0
        jne if_54_5_84_5_end
        if_54_8_84_5_code:
;            [54:17] return
            jmp bar_84_5_end
        if_54_5_84_5_end:
;        [55:5] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [55:11] arg = 0xff
        mov qword[rsp-48], 0xff
    bar_84_5_end:
;    [85:5] assert(i == 0)
;    alloc r15
;    [85:12] i == 0
;    [85:12] ? i == 0
;    [85:12] ? i == 0
    cmp_85_12:
    cmp qword[rsp-48], 0
    jne bool_false_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
;    assert(expr : bool) 
    assert_85_5:
;        alias expr -> r15
        if_36_8_85_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_85_5:
        cmp r15, false
        jne if_36_5_85_5_end
        if_36_8_85_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_85_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_85_5_end:
        if_36_5_85_5_end:
;        free r15
    assert_85_5_end:
;    [87:5] i = 1
;    [87:9]  1
;    [87:9] 1
;    [87:9] i = 1
    mov qword[rsp-48], 1
;    [88:5] bar(i)
;    bar(arg) 
    bar_88_5:
;        alias arg -> i
        if_54_8_88_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_88_5:
        cmp qword[rsp-48], 0
        jne if_54_5_88_5_end
        if_54_8_88_5_code:
;            [54:17] return
            jmp bar_88_5_end
        if_54_5_88_5_end:
;        [55:5] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [55:11] arg = 0xff
        mov qword[rsp-48], 0xff
    bar_88_5_end:
;    [89:5] assert(i == 0xff)
;    alloc r15
;    [89:12] i == 0xff
;    [89:12] ? i == 0xff
;    [89:12] ? i == 0xff
    cmp_89_12:
    cmp qword[rsp-48], 0xff
    jne bool_false_89_12
    bool_true_89_12:
    mov r15, true
    jmp bool_end_89_12
    bool_false_89_12:
    mov r15, false
    bool_end_89_12:
;    assert(expr : bool) 
    assert_89_5:
;        alias expr -> r15
        if_36_8_89_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_89_5:
        cmp r15, false
        jne if_36_5_89_5_end
        if_36_8_89_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_89_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_89_5_end:
        if_36_5_89_5_end:
;        free r15
    assert_89_5_end:
;    var j: i64[1] @ qword[rsp-56]
;    [91:5] var j = 1
;    [91:9] j = 1
;    [91:13]  1
;    [91:13] 1
;    [91:13] j = 1
    mov qword[rsp-56], 1
;    var k: i64[1] @ qword[rsp-64]
;    [92:5] var k = baz(j)
;    [92:9] k = baz(j)
;    [92:13]  baz(j)
;    [92:13] baz(j)
;    [92:13] k = baz(j)
;    [92:13] baz(j)
;    baz(arg) : i64 res 
    baz_92_13:
;        alias res -> k
;        alias arg -> j
;        [64:5]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:11] res = arg
;        alloc r15
        mov r15, qword[rsp-56]
        mov qword[rsp-64], r15
;        free r15
;        [64:17] res * 2
;        alloc r15
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
;        free r15
    baz_92_13_end:
;    [93:5] assert(k == 2)
;    alloc r15
;    [93:12] k == 2
;    [93:12] ? k == 2
;    [93:12] ? k == 2
    cmp_93_12:
    cmp qword[rsp-64], 2
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
;    assert(expr : bool) 
    assert_93_5:
;        alias expr -> r15
        if_36_8_93_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_93_5:
        cmp r15, false
        jne if_36_5_93_5_end
        if_36_8_93_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_93_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_93_5_end:
        if_36_5_93_5_end:
;        free r15
    assert_93_5_end:
;    [95:5] k = baz(1)
;    [95:9]  baz(1)
;    [95:9] baz(1)
;    [95:9] k = baz(1)
;    [95:9] baz(1)
;    baz(arg) : i64 res 
    baz_95_9:
;        alias res -> k
;        alias arg -> 1
;        [64:5]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:11] res = arg
        mov qword[rsp-64], 1
;        [64:17] res * 2
;        alloc r15
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
;        free r15
    baz_95_9_end:
;    [96:5] assert(k == 2)
;    alloc r15
;    [96:12] k == 2
;    [96:12] ? k == 2
;    [96:12] ? k == 2
    cmp_96_12:
    cmp qword[rsp-64], 2
    jne bool_false_96_12
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
;    var p0: point[1] @ qword[rsp-80]
;    [98:5] var p0 : point = {baz(2), 0}
;    [98:9] p0 = {baz(2), 0}
;    [98:22]  {baz(2), 0}
;    [98:22] {baz(2), 0}
;    [98:23] baz(2)
;    [98:23] baz(2)
;    [98:23] p0.x = baz(2)
;    [98:23] baz(2)
;    baz(arg) : i64 res 
    baz_98_23:
;        alias res -> p0.x
;        alias arg -> 2
;        [64:5]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:11] res = arg
        mov qword[rsp-80], 2
;        [64:17] res * 2
;        alloc r15
        mov r15, qword[rsp-80]
        imul r15, 2
        mov qword[rsp-80], r15
;        free r15
    baz_98_23_end:
;    [98:31]  0
;    [98:31] 0
;    [98:31] p0.y = 0
    mov qword[rsp-72], 0
;    [99:5]  assert(p0.x == 4)
;    alloc r15
;    [99:12] p0.x == 4
;    [99:12] ? p0.x == 4
;    [99:12] ? p0.x == 4
    cmp_99_12:
    cmp qword[rsp-80], 4
    jne bool_false_99_12
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
;    var x: i64[1] @ qword[rsp-88]
;    [101:5] var x = 1
;    [101:9] x = 1
;    [101:13]  1
;    [101:13] 1
;    [101:13] x = 1
    mov qword[rsp-88], 1
;    var y: i64[1] @ qword[rsp-96]
;    [102:5] var y = 2
;    [102:9] y = 2
;    [102:13]  2
;    [102:13] 2
;    [102:13] y = 2
    mov qword[rsp-96], 2
;    var o1: object[1] @ qword[rsp-116]
;    [104:5] var o1 : object = {{x * 10, y}, 0xff0000}
;    [104:9] o1 = {{x * 10, y}, 0xff0000}
;    [104:23]  {{x * 10, y}, 0xff0000}
;    [104:23] {{x * 10, y}, 0xff0000}
;        [104:24] {x * 10, y}
;        [104:25] x * 10
;        [104:25] x * 10
;        [104:25] o1.pos.x = x
;        alloc r15
        mov r15, qword[rsp-88]
        mov qword[rsp-116], r15
;        free r15
;        [104:29] o1.pos.x * 10
;        alloc r15
        mov r15, qword[rsp-116]
        imul r15, 10
        mov qword[rsp-116], r15
;        free r15
;        [104:33]  y
;        [104:33] y
;        [104:33] o1.pos.y = y
;        alloc r15
        mov r15, qword[rsp-96]
        mov qword[rsp-108], r15
;        free r15
;    [104:37]  0xff0000
;    [104:37] 0xff0000
;    [104:37] o1.color = 0xff0000
    mov dword[rsp-100], 0xff0000
;    [105:5]  assert(o1.pos.x == 10)
;    alloc r15
;    [105:12] o1.pos.x == 10
;    [105:12] ? o1.pos.x == 10
;    [105:12] ? o1.pos.x == 10
    cmp_105_12:
    cmp qword[rsp-116], 10
    jne bool_false_105_12
    bool_true_105_12:
    mov r15, true
    jmp bool_end_105_12
    bool_false_105_12:
    mov r15, false
    bool_end_105_12:
;    assert(expr : bool) 
    assert_105_5:
;        alias expr -> r15
        if_36_8_105_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_105_5:
        cmp r15, false
        jne if_36_5_105_5_end
        if_36_8_105_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_105_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_105_5_end:
        if_36_5_105_5_end:
;        free r15
    assert_105_5_end:
;    [106:5] assert(o1.pos.y == 2)
;    alloc r15
;    [106:12] o1.pos.y == 2
;    [106:12] ? o1.pos.y == 2
;    [106:12] ? o1.pos.y == 2
    cmp_106_12:
    cmp qword[rsp-108], 2
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
;    [107:5] assert(o1.color == 0xff0000)
;    alloc r15
;    [107:12] o1.color == 0xff0000
;    [107:12] ? o1.color == 0xff0000
;    [107:12] ? o1.color == 0xff0000
    cmp_107_12:
    cmp dword[rsp-100], 0xff0000
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
;    var p1: point[1] @ qword[rsp-132]
;    [109:5] var p1 : point = {-x, -y}
;    [109:9] p1 = {-x, -y}
;    [109:22]  {-x, -y}
;    [109:22] {-x, -y}
;    [109:23] -x
;    [109:23] -x
;    [109:24] p1.x = -x
;    alloc r15
    mov r15, qword[rsp-88]
    mov qword[rsp-132], r15
;    free r15
    neg qword[rsp-132]
;    [109:27]  -y
;    [109:27] -y
;    [109:28] p1.y = -y
;    alloc r15
    mov r15, qword[rsp-96]
    mov qword[rsp-124], r15
;    free r15
    neg qword[rsp-124]
;    [110:5]  o1.pos = p1
;    [110:14]  p1
;    [110:14] p1
;    alloc r15
    mov r15, qword[rsp-132]
    mov qword[rsp-116], r15
;    free r15
;    alloc r15
    mov r15, qword[rsp-124]
    mov qword[rsp-108], r15
;    free r15
;    [111:5] assert(o1.pos.x == -1)
;    alloc r15
;    [111:12] o1.pos.x == -1
;    [111:12] ? o1.pos.x == -1
;    [111:12] ? o1.pos.x == -1
    cmp_111_12:
    cmp qword[rsp-116], -1
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
;    [112:5] assert(o1.pos.y == -2)
;    alloc r15
;    [112:12] o1.pos.y == -2
;    [112:12] ? o1.pos.y == -2
;    [112:12] ? o1.pos.y == -2
    cmp_112_12:
    cmp qword[rsp-108], -2
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
;    var o2: object[1] @ qword[rsp-152]
;    [114:5] var o2 : object = o1
;    [114:9] o2 = o1
;    [114:23]  o1
;    [114:23] o1
;        [114:23] o1
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
;    [115:5] assert(o2.pos.x == -1)
;    alloc r15
;    [115:12] o2.pos.x == -1
;    [115:12] ? o2.pos.x == -1
;    [115:12] ? o2.pos.x == -1
    cmp_115_12:
    cmp qword[rsp-152], -1
    jne bool_false_115_12
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
;    [116:5] assert(o2.pos.y == -2)
;    alloc r15
;    [116:12] o2.pos.y == -2
;    [116:12] ? o2.pos.y == -2
;    [116:12] ? o2.pos.y == -2
    cmp_116_12:
    cmp qword[rsp-144], -2
    jne bool_false_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
;    assert(expr : bool) 
    assert_116_5:
;        alias expr -> r15
        if_36_8_116_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_116_5:
        cmp r15, false
        jne if_36_5_116_5_end
        if_36_8_116_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_116_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_116_5_end:
        if_36_5_116_5_end:
;        free r15
    assert_116_5_end:
;    [117:5] assert(o2.color == 0xff0000)
;    alloc r15
;    [117:12] o2.color == 0xff0000
;    [117:12] ? o2.color == 0xff0000
;    [117:12] ? o2.color == 0xff0000
    cmp_117_12:
    cmp dword[rsp-136], 0xff0000
    jne bool_false_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
;    assert(expr : bool) 
    assert_117_5:
;        alias expr -> r15
        if_36_8_117_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_117_5:
        cmp r15, false
        jne if_36_5_117_5_end
        if_36_8_117_5_code:
;            [36:17] exit(1)
;            alloc rdi
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_117_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5]  mov(rdi, v)
;                [14:19] # return code
;                [15:5]  syscall()
                syscall
;                free rdi
            exit_36_17_117_5_end:
        if_36_5_117_5_end:
;        free r15
    assert_117_5_end:
;    [119:5] print(hello.len, hello)
;    alloc rdx
    mov rdx, hello.len
;    alloc rsi
    mov rsi, hello
;    print(len : reg_rdx, ptr : reg_rsi) 
    print_119_5:
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
    print_119_5_end:
;    [120:5] loop
    loop_120_5:
;        [121:9]  print(prompt1.len, prompt1)
;        alloc rdx
        mov rdx, prompt1.len
;        alloc rsi
        mov rsi, prompt1
;        print(len : reg_rdx, ptr : reg_rsi) 
        print_121_9:
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
        print_121_9_end:
;        var len: i64[1] @ qword[rsp-160]
;        [122:9] var len = read(input.len, input) - 1
;        [122:13] len = read(input.len, input) - 1
;        [122:19]  read(input.len, input) - 1
;        [122:19] read(input.len, input) - 1
;        [122:19] len = read(input.len, input)
;        [122:19] read(input.len, input)
;        alloc rdx
        mov rdx, input.len
;        alloc rsi
        mov rsi, input
;        read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_122_19:
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
        read_122_19_end:
;        [122:44] len - 1
        sub qword[rsp-160], 1
;        [122:49] # -1 don't include the '\n'
        if_123_12:
;        [123:12] ? len == 0
;        [123:12] ? len == 0
        cmp_123_12:
        cmp qword[rsp-160], 0
        jne if_125_19
        if_123_12_code:
;            [124:13]  break
            jmp loop_120_5_end
        jmp if_123_9_end
        if_125_19:
;        [125:19] ? len <= 4
;        [125:19] ? len <= 4
        cmp_125_19:
        cmp qword[rsp-160], 4
        jg if_else_123_9
        if_125_19_code:
;            [126:13]  print(prompt2.len, prompt2)
;            alloc rdx
            mov rdx, prompt2.len
;            alloc rsi
            mov rsi, prompt2
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
;            [127:13] continue
            jmp loop_120_5
        jmp if_123_9_end
        if_else_123_9:
;            [129:13]  print(prompt3.len, prompt3)
;            alloc rdx
            mov rdx, prompt3.len
;            alloc rsi
            mov rsi, prompt3
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
;            [130:13] print(len, input)
;            alloc rdx
            mov rdx, qword[rsp-160]
;            alloc rsi
            mov rsi, input
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_130_13:
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
            print_130_13_end:
;            [131:13] print(dot.len, dot)
;            alloc rdx
            mov rdx, dot.len
;            alloc rsi
            mov rsi, dot
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_131_13:
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
            print_131_13_end:
;            [132:13] print(nl.len, nl)
;            alloc rdx
            mov rdx, nl.len
;            alloc rsi
            mov rsi, nl
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
        if_123_9_end:
    jmp loop_120_5
    loop_120_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; max scratch registers in use: 3
;            max frames in use: 7
;               max stack size: 160 B
```
