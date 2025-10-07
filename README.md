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
* keywords: `func`, `type`, `field`, `var`, `loop`, `if`, `else`, `continue`,
`break`, `return`

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
C/C++ Header                    36            934            667           4765
C++                              1             65             76            420
-------------------------------------------------------------------------------
SUM:                            37            999            743           5185
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

    var ix = 1

    arr[ix] = 2
    arr[ix + 1] = arr[ix]
    
    assert(arr[1] == 2)
    assert(arr[2] == 2)

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
stk resd 0x10000
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
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, qword [rsp - 24]
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_76_12:
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_76_12
    jmp bool_true_76_12
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
        jmp if_36_8_76_5_code
        if_36_8_76_5_code:
            mov rdi, 1
            exit_36_17_76_5:
                mov rax, 60
                syscall
            exit_36_17_76_5_end:
        if_36_5_76_5_end:
    assert_76_5_end:
    cmp_77_12:
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_77_12
    jmp bool_true_77_12
    bool_true_77_12:
    mov r15, true
    jmp bool_end_77_12
    bool_false_77_12:
    mov r15, false
    bool_end_77_12:
    assert_77_5:
        if_36_8_77_5:
        cmp_36_8_77_5:
        cmp r15, false
        jne if_36_5_77_5_end
        jmp if_36_8_77_5_code
        if_36_8_77_5_code:
            mov rdi, 1
            exit_36_17_77_5:
                mov rax, 60
                syscall
            exit_36_17_77_5_end:
        if_36_5_77_5_end:
    assert_77_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_80_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_80_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_81_12:
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_81_12
    jmp bool_true_81_12
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
        jmp if_36_8_81_5_code
        if_36_8_81_5_code:
            mov rdi, 1
            exit_36_17_81_5:
                mov rax, 60
                syscall
            exit_36_17_81_5_end:
        if_36_5_81_5_end:
    assert_81_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_84_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_84_5_end:
    cmp_85_12:
    cmp qword [rsp - 40], 2
    jne bool_false_85_12
    jmp bool_true_85_12
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
        jmp if_36_8_85_5_code
        if_36_8_85_5_code:
            mov rdi, 1
            exit_36_17_85_5:
                mov rax, 60
                syscall
            exit_36_17_85_5_end:
        if_36_5_85_5_end:
    assert_85_5_end:
    cmp_86_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_86_12
    jmp bool_true_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
    assert_86_5:
        if_36_8_86_5:
        cmp_36_8_86_5:
        cmp r15, false
        jne if_36_5_86_5_end
        jmp if_36_8_86_5_code
        if_36_8_86_5_code:
            mov rdi, 1
            exit_36_17_86_5:
                mov rax, 60
                syscall
            exit_36_17_86_5_end:
        if_36_5_86_5_end:
    assert_86_5_end:
    mov qword [rsp - 48], 0
    bar_89_5:
        if_54_8_89_5:
        cmp_54_8_89_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_89_5_end
        jmp if_54_8_89_5_code
        if_54_8_89_5_code:
            jmp bar_89_5_end
        if_54_5_89_5_end:
        mov qword [rsp - 48], 0xff
    bar_89_5_end:
    cmp_90_12:
    cmp qword [rsp - 48], 0
    jne bool_false_90_12
    jmp bool_true_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
    assert_90_5:
        if_36_8_90_5:
        cmp_36_8_90_5:
        cmp r15, false
        jne if_36_5_90_5_end
        jmp if_36_8_90_5_code
        if_36_8_90_5_code:
            mov rdi, 1
            exit_36_17_90_5:
                mov rax, 60
                syscall
            exit_36_17_90_5_end:
        if_36_5_90_5_end:
    assert_90_5_end:
    mov qword [rsp - 48], 1
    bar_93_5:
        if_54_8_93_5:
        cmp_54_8_93_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_93_5_end
        jmp if_54_8_93_5_code
        if_54_8_93_5_code:
            jmp bar_93_5_end
        if_54_5_93_5_end:
        mov qword [rsp - 48], 0xff
    bar_93_5_end:
    cmp_94_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_94_12
    jmp bool_true_94_12
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
        jmp if_36_8_94_5_code
        if_36_8_94_5_code:
            mov rdi, 1
            exit_36_17_94_5:
                mov rax, 60
                syscall
            exit_36_17_94_5_end:
        if_36_5_94_5_end:
    assert_94_5_end:
    mov qword [rsp - 56], 1
    baz_97_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_97_13_end:
    cmp_98_12:
    cmp qword [rsp - 64], 2
    jne bool_false_98_12
    jmp bool_true_98_12
    bool_true_98_12:
    mov r15, true
    jmp bool_end_98_12
    bool_false_98_12:
    mov r15, false
    bool_end_98_12:
    assert_98_5:
        if_36_8_98_5:
        cmp_36_8_98_5:
        cmp r15, false
        jne if_36_5_98_5_end
        jmp if_36_8_98_5_code
        if_36_8_98_5_code:
            mov rdi, 1
            exit_36_17_98_5:
                mov rax, 60
                syscall
            exit_36_17_98_5_end:
        if_36_5_98_5_end:
    assert_98_5_end:
    baz_100_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_100_9_end:
    cmp_101_12:
    cmp qword [rsp - 64], 2
    jne bool_false_101_12
    jmp bool_true_101_12
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
        jmp if_36_8_101_5_code
        if_36_8_101_5_code:
            mov rdi, 1
            exit_36_17_101_5:
                mov rax, 60
                syscall
            exit_36_17_101_5_end:
        if_36_5_101_5_end:
    assert_101_5_end:
    baz_103_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_103_23_end:
    mov qword [rsp - 72], 0
    cmp_104_12:
    cmp qword [rsp - 80], 4
    jne bool_false_104_12
    jmp bool_true_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_36_8_104_5:
        cmp_36_8_104_5:
        cmp r15, false
        jne if_36_5_104_5_end
        jmp if_36_8_104_5_code
        if_36_8_104_5_code:
            mov rdi, 1
            exit_36_17_104_5:
                mov rax, 60
                syscall
            exit_36_17_104_5_end:
        if_36_5_104_5_end:
    assert_104_5_end:
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
    cmp_110_12:
    cmp qword [rsp - 116], 10
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
    cmp_111_12:
    cmp qword [rsp - 108], 2
    jne bool_false_111_12
    jmp bool_true_111_12
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
        jmp if_36_8_111_5_code
        if_36_8_111_5_code:
            mov rdi, 1
            exit_36_17_111_5:
                mov rax, 60
                syscall
            exit_36_17_111_5_end:
        if_36_5_111_5_end:
    assert_111_5_end:
    cmp_112_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_112_12
    jmp bool_true_112_12
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
        jmp if_36_8_112_5_code
        if_36_8_112_5_code:
            mov rdi, 1
            exit_36_17_112_5:
                mov rax, 60
                syscall
            exit_36_17_112_5_end:
        if_36_5_112_5_end:
    assert_112_5_end:
    mov r15, qword [rsp - 88]
    mov qword [rsp - 132], r15
    neg qword [rsp - 132]
    mov r15, qword [rsp - 96]
    mov qword [rsp - 124], r15
    neg qword [rsp - 124]
    lea rdi, [rsp - 116]
    lea rsi, [rsp - 132]
    mov rcx, 16
    rep movsb
    cmp_116_12:
    cmp qword [rsp - 116], -1
    jne bool_false_116_12
    jmp bool_true_116_12
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
        jmp if_36_8_116_5_code
        if_36_8_116_5_code:
            mov rdi, 1
            exit_36_17_116_5:
                mov rax, 60
                syscall
            exit_36_17_116_5_end:
        if_36_5_116_5_end:
    assert_116_5_end:
    cmp_117_12:
    cmp qword [rsp - 108], -2
    jne bool_false_117_12
    jmp bool_true_117_12
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
        jmp if_36_8_117_5_code
        if_36_8_117_5_code:
            mov rdi, 1
            exit_36_17_117_5:
                mov rax, 60
                syscall
            exit_36_17_117_5_end:
        if_36_5_117_5_end:
    assert_117_5_end:
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_120_12:
    cmp qword [rsp - 152], -1
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
    cmp_121_12:
    cmp qword [rsp - 144], -2
    jne bool_false_121_12
    jmp bool_true_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_36_8_121_5:
        cmp_36_8_121_5:
        cmp r15, false
        jne if_36_5_121_5_end
        jmp if_36_8_121_5_code
        if_36_8_121_5_code:
            mov rdi, 1
            exit_36_17_121_5:
                mov rax, 60
                syscall
            exit_36_17_121_5_end:
        if_36_5_121_5_end:
    assert_121_5_end:
    cmp_122_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_122_12
    jmp bool_true_122_12
    bool_true_122_12:
    mov r15, true
    jmp bool_end_122_12
    bool_false_122_12:
    mov r15, false
    bool_end_122_12:
    assert_122_5:
        if_36_8_122_5:
        cmp_36_8_122_5:
        cmp r15, false
        jne if_36_5_122_5_end
        jmp if_36_8_122_5_code
        if_36_8_122_5_code:
            mov rdi, 1
            exit_36_17_122_5:
                mov rax, 60
                syscall
            exit_36_17_122_5_end:
        if_36_5_122_5_end:
    assert_122_5_end:
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 164], 73
    cmp_127_12:
        lea r13, [rsp - 172]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_127_12
    jmp bool_true_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
    assert_127_5:
        if_36_8_127_5:
        cmp_36_8_127_5:
        cmp r15, false
        jne if_36_5_127_5_end
        jmp if_36_8_127_5_code
        if_36_8_127_5_code:
            mov rdi, 1
            exit_36_17_127_5:
                mov rax, 60
                syscall
            exit_36_17_127_5_end:
        if_36_5_127_5_end:
    assert_127_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_129_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_129_5_end:
    loop_130_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_131_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_131_9_end:
        mov rdx, input.len
        mov rsi, input
        read_132_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 180], rax
        read_132_19_end:
        sub qword [rsp - 180], 1
        if_133_12:
        cmp_133_12:
        cmp qword [rsp - 180], 0
        jne if_135_19
        jmp if_133_12_code
        if_133_12_code:
            jmp loop_130_5_end
        jmp if_133_9_end
        if_135_19:
        cmp_135_19:
        cmp qword [rsp - 180], 4
        jg if_else_133_9
        jmp if_135_19_code
        if_135_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_136_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_136_13_end:
            jmp loop_130_5
        jmp if_133_9_end
        if_else_133_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_139_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_139_13_end:
            mov rdx, qword [rsp - 180]
            mov rsi, input
            print_140_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_140_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_141_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_141_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_142_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_142_13_end:
        if_133_9_end:
    jmp loop_130_5
    loop_130_5_end:
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
;[11:1] # arguments can be placed in specified register using `reg_...` syntax
;[39:1] # user types are defined using keyword `type`
;[41:1] # default type is `i64` and does not need to be specified
;[46:1]  # function arguments are equivalent to mutable references
;[52:1] # default argument type is `i64`
;[62:1] # return target is specified as a variable, in this case `res`
main:
;    var arr: i32[4] @ dword [rsp - 16]
;    [68:5]  var arr : i32[4]
;    clear array 4 * 4 B = 16 B
;    allocate named register 'rdi'
;    allocate named register 'rcx'
;    allocate named register 'rax'
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
;    free named register 'rax'
;    free named register 'rcx'
;    free named register 'rdi'
;    [69:5] # arrays are initialized to 0
;    var ix: i64 @ qword [rsp - 24]
;    [71:5] var ix = 1
;    [71:14] ix =1
;    [71:14] 1
;    [71:14] 1
;    [73:5] ix = 1
    mov qword [rsp - 24], 1
;    [73:8] arr[ix] = 2
;    allocate scratch register -> r15
;    [73:9] ix
;    [73:9] ix
;    [73:11] r15 = ix
    mov r15, qword [rsp - 24]
;    [73:15]  2
;    [73:15] 2
;    [74:5] dword [rsp + r15 * 4 - 16] = 2
    mov dword [rsp + r15 * 4 - 16], 2
;    free scratch register 'r15'
;    [74:8] arr[ix + 1] = arr[ix]
;    allocate scratch register -> r15
;    [74:9] ix + 1
;    [74:9] ix + 1
;    [74:12] r15 = ix
    mov r15, qword [rsp - 24]
;    [74:15] r15 + 1
    add r15, 1
;    [74:19]  arr[ix]
;    [74:19] arr[ix]
;    [74:22] dword [rsp + r15 * 4 - 16] = arr[ix]
;    [74:22] arr[ix]
;    allocate scratch register -> r14
;    [74:23] ix
;    [74:23] ix
;    [74:25] r14 = ix
    mov r14, qword [rsp - 24]
;    allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
;    free scratch register 'r13'
;    free scratch register 'r14'
;    free scratch register 'r15'
;    [76:5] assert(arr[1] == 2)
;    allocate scratch register -> r15
;    [76:12] arr[1] == 2
;    [76:12] ? arr[1] == 2
;    [76:12] ? arr[1] == 2
    cmp_76_12:
;    allocate scratch register -> r14
;        [76:12] arr[1]
;        [76:15] r14 = arr[1]
;        [76:15] arr[1]
;        allocate scratch register -> r13
;        [76:16] 1
;        [76:16] 1
;        [76:17] r13 = 1
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
;        free scratch register 'r13'
    cmp r14, 2
;    free scratch register 'r14'
    jne bool_false_76_12
    jmp bool_true_76_12
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
        jmp if_36_8_76_5_code
        if_36_8_76_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_76_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_76_5_end:
        if_36_5_76_5_end:
;        free scratch register 'r15'
    assert_76_5_end:
;    [77:5] assert(arr[2] == 2)
;    allocate scratch register -> r15
;    [77:12] arr[2] == 2
;    [77:12] ? arr[2] == 2
;    [77:12] ? arr[2] == 2
    cmp_77_12:
;    allocate scratch register -> r14
;        [77:12] arr[2]
;        [77:15] r14 = arr[2]
;        [77:15] arr[2]
;        allocate scratch register -> r13
;        [77:16] 2
;        [77:16] 2
;        [77:17] r13 = 2
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
;        free scratch register 'r13'
    cmp r14, 2
;    free scratch register 'r14'
    jne bool_false_77_12
    jmp bool_true_77_12
    bool_true_77_12:
    mov r15, true
    jmp bool_end_77_12
    bool_false_77_12:
    mov r15, false
    bool_end_77_12:
;    assert(expr : bool) 
    assert_77_5:
;        alias expr -> r15
        if_36_8_77_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_77_5:
        cmp r15, false
        jne if_36_5_77_5_end
        jmp if_36_8_77_5_code
        if_36_8_77_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_77_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_77_5_end:
        if_36_5_77_5_end:
;        free scratch register 'r15'
    assert_77_5_end:
;    [79:8] ix = 3
;    [79:10]  3
;    [79:10] 3
;    [80:5] ix = 3
    mov qword [rsp - 24], 3
;    [80:8] arr[ix] = ~inv(arr[ix - 1])
;    allocate scratch register -> r15
;    [80:9] ix
;    [80:9] ix
;    [80:11] r15 = ix
    mov r15, qword [rsp - 24]
;    [80:15]  ~inv(arr[ix - 1])
;    [80:15] ~inv(arr[ix - 1])
;    [80:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;    [80:16] ~inv(arr[ix - 1])
;    allocate scratch register -> r14
;    [80:20] arr[ix - 1]
;    [80:20] arr[ix - 1]
;    [80:23] r14 = arr[ix - 1]
;    [80:23] arr[ix - 1]
;    allocate scratch register -> r13
;    [80:24] ix - 1
;    [80:24] ix - 1
;    [80:27] r13 = ix
    mov r13, qword [rsp - 24]
;    [80:30] r13 - 1
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
;    free scratch register 'r13'
;    inv(i : i32) : i32 res 
    inv_80_16:
;        alias res -> dword [rsp + r15 * 4 - 16]
;        alias i -> r14
;        [59:9]  res = ~i
;        [59:11]  ~i
;        [59:11] ~i
;        [60:1] res = ~i
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
;        free scratch register 'r14'
    inv_80_16_end:
    not dword [rsp + r15 * 4 - 16]
;    free scratch register 'r15'
;    [81:5] assert(arr[ix] == 2)
;    allocate scratch register -> r15
;    [81:12] arr[ix] == 2
;    [81:12] ? arr[ix] == 2
;    [81:12] ? arr[ix] == 2
    cmp_81_12:
;    allocate scratch register -> r14
;        [81:12] arr[ix]
;        [81:15] r14 = arr[ix]
;        [81:15] arr[ix]
;        allocate scratch register -> r13
;        [81:16] ix
;        [81:16] ix
;        [81:18] r13 = ix
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
;        free scratch register 'r13'
    cmp r14, 2
;    free scratch register 'r14'
    jne bool_false_81_12
    jmp bool_true_81_12
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
        jmp if_36_8_81_5_code
        if_36_8_81_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_81_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_81_5_end:
        if_36_5_81_5_end:
;        free scratch register 'r15'
    assert_81_5_end:
;    var p: point @ qword [rsp - 40]
;    [83:5] var p : point = {0, 0}
;    [83:21] p ={0, 0}
;    [83:21] {0, 0}
;    [83:21] {0, 0}
;    [83:22] 0
;    [83:22] 0
;    [83:23] p.x = 0
    mov qword [rsp - 40], 0
;    [83:25]  0
;    [83:25] 0
;    [83:26] p.y = 0
    mov qword [rsp - 32], 0
;    [84:5]  foo(p)
;    foo(pt : point) 
    foo_84_5:
;        alias pt -> p
;        [48:7]  pt.x = 0b10
;        [48:12]  0b10
;        [48:12] 0b10
;        [48:20] pt.x = 0b10
        mov qword [rsp - 40], 0b10
;        [48:20] # binary value 2
;        [49:7] pt.y = 0xb
;        [49:12]  0xb
;        [49:12] 0xb
;        [49:20] pt.y = 0xb
        mov qword [rsp - 32], 0xb
;        [49:20] # hex value 11
    foo_84_5_end:
;    [85:5] assert(p.x == 2)
;    allocate scratch register -> r15
;    [85:12] p.x == 2
;    [85:12] ? p.x == 2
;    [85:12] ? p.x == 2
    cmp_85_12:
    cmp qword [rsp - 40], 2
    jne bool_false_85_12
    jmp bool_true_85_12
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
        jmp if_36_8_85_5_code
        if_36_8_85_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_85_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_85_5_end:
        if_36_5_85_5_end:
;        free scratch register 'r15'
    assert_85_5_end:
;    [86:5] assert(p.y == 0xb)
;    allocate scratch register -> r15
;    [86:12] p.y == 0xb
;    [86:12] ? p.y == 0xb
;    [86:12] ? p.y == 0xb
    cmp_86_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_86_12
    jmp bool_true_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
;    assert(expr : bool) 
    assert_86_5:
;        alias expr -> r15
        if_36_8_86_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_86_5:
        cmp r15, false
        jne if_36_5_86_5_end
        jmp if_36_8_86_5_code
        if_36_8_86_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_86_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_86_5_end:
        if_36_5_86_5_end:
;        free scratch register 'r15'
    assert_86_5_end:
;    var i: i64 @ qword [rsp - 48]
;    [88:5] var i = 0
;    [88:13] i =0
;    [88:13] 0
;    [88:13] 0
;    [89:5] i = 0
    mov qword [rsp - 48], 0
;    [89:5] bar(i)
;    bar(arg) 
    bar_89_5:
;        alias arg -> i
        if_54_8_89_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_89_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_89_5_end
        jmp if_54_8_89_5_code
        if_54_8_89_5_code:
;            [54:17] return
            jmp bar_89_5_end
        if_54_5_89_5_end:
;        [55:9] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [56:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_89_5_end:
;    [90:5] assert(i == 0)
;    allocate scratch register -> r15
;    [90:12] i == 0
;    [90:12] ? i == 0
;    [90:12] ? i == 0
    cmp_90_12:
    cmp qword [rsp - 48], 0
    jne bool_false_90_12
    jmp bool_true_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
;    assert(expr : bool) 
    assert_90_5:
;        alias expr -> r15
        if_36_8_90_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_90_5:
        cmp r15, false
        jne if_36_5_90_5_end
        jmp if_36_8_90_5_code
        if_36_8_90_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_90_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_90_5_end:
        if_36_5_90_5_end:
;        free scratch register 'r15'
    assert_90_5_end:
;    [92:7] i = 1
;    [92:9]  1
;    [92:9] 1
;    [93:5] i = 1
    mov qword [rsp - 48], 1
;    [93:5] bar(i)
;    bar(arg) 
    bar_93_5:
;        alias arg -> i
        if_54_8_93_5:
;        [54:8] ? arg == 0
;        [54:8] ? arg == 0
        cmp_54_8_93_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_93_5_end
        jmp if_54_8_93_5_code
        if_54_8_93_5_code:
;            [54:17] return
            jmp bar_93_5_end
        if_54_5_93_5_end:
;        [55:9] arg = 0xff
;        [55:11]  0xff
;        [55:11] 0xff
;        [56:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_93_5_end:
;    [94:5] assert(i == 0xff)
;    allocate scratch register -> r15
;    [94:12] i == 0xff
;    [94:12] ? i == 0xff
;    [94:12] ? i == 0xff
    cmp_94_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_94_12
    jmp bool_true_94_12
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
        jmp if_36_8_94_5_code
        if_36_8_94_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_94_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_94_5_end:
        if_36_5_94_5_end:
;        free scratch register 'r15'
    assert_94_5_end:
;    var j: i64 @ qword [rsp - 56]
;    [96:5] var j = 1
;    [96:13] j =1
;    [96:13] 1
;    [96:13] 1
;    [97:5] j = 1
    mov qword [rsp - 56], 1
;    var k: i64 @ qword [rsp - 64]
;    [97:5] var k = baz(j)
;    [97:13] k =baz(j)
;    [97:13] baz(j)
;    [97:13] baz(j)
;    [97:13] k = baz(j)
;    [97:13] baz(j)
;    baz(arg) : i64 res 
    baz_97_13:
;        alias res -> k
;        alias arg -> j
;        [64:9]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:15] res = arg
;        allocate scratch register -> r15
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
;        free scratch register 'r15'
;        [65:1] res * 2
;        allocate scratch register -> r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;        free scratch register 'r15'
    baz_97_13_end:
;    [98:5] assert(k == 2)
;    allocate scratch register -> r15
;    [98:12] k == 2
;    [98:12] ? k == 2
;    [98:12] ? k == 2
    cmp_98_12:
    cmp qword [rsp - 64], 2
    jne bool_false_98_12
    jmp bool_true_98_12
    bool_true_98_12:
    mov r15, true
    jmp bool_end_98_12
    bool_false_98_12:
    mov r15, false
    bool_end_98_12:
;    assert(expr : bool) 
    assert_98_5:
;        alias expr -> r15
        if_36_8_98_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_98_5:
        cmp r15, false
        jne if_36_5_98_5_end
        jmp if_36_8_98_5_code
        if_36_8_98_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_98_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_98_5_end:
        if_36_5_98_5_end:
;        free scratch register 'r15'
    assert_98_5_end:
;    [100:7] k = baz(1)
;    [100:9]  baz(1)
;    [100:9] baz(1)
;    [100:9] k = baz(1)
;    [100:9] baz(1)
;    baz(arg) : i64 res 
    baz_100_9:
;        alias res -> k
;        alias arg -> 1
;        [64:9]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:15] res = arg
        mov qword [rsp - 64], 1
;        [65:1] res * 2
;        allocate scratch register -> r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;        free scratch register 'r15'
    baz_100_9_end:
;    [101:5] assert(k == 2)
;    allocate scratch register -> r15
;    [101:12] k == 2
;    [101:12] ? k == 2
;    [101:12] ? k == 2
    cmp_101_12:
    cmp qword [rsp - 64], 2
    jne bool_false_101_12
    jmp bool_true_101_12
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
        jmp if_36_8_101_5_code
        if_36_8_101_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_101_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_101_5_end:
        if_36_5_101_5_end:
;        free scratch register 'r15'
    assert_101_5_end:
;    var p0: point @ qword [rsp - 80]
;    [103:5] var p0 : point = {baz(2), 0}
;    [103:22] p0 ={baz(2), 0}
;    [103:22] {baz(2), 0}
;    [103:22] {baz(2), 0}
;    [103:23] baz(2)
;    [103:23] baz(2)
;    [103:23] p0.x = baz(2)
;    [103:23] baz(2)
;    baz(arg) : i64 res 
    baz_103_23:
;        alias res -> p0.x
;        alias arg -> 2
;        [64:9]  res = arg * 2
;        [64:11]  arg * 2
;        [64:11] arg * 2
;        [64:15] res = arg
        mov qword [rsp - 80], 2
;        [65:1] res * 2
;        allocate scratch register -> r15
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
;        free scratch register 'r15'
    baz_103_23_end:
;    [103:31]  0
;    [103:31] 0
;    [103:32] p0.y = 0
    mov qword [rsp - 72], 0
;    [104:5]  assert(p0.x == 4)
;    allocate scratch register -> r15
;    [104:12] p0.x == 4
;    [104:12] ? p0.x == 4
;    [104:12] ? p0.x == 4
    cmp_104_12:
    cmp qword [rsp - 80], 4
    jne bool_false_104_12
    jmp bool_true_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
;    assert(expr : bool) 
    assert_104_5:
;        alias expr -> r15
        if_36_8_104_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_104_5:
        cmp r15, false
        jne if_36_5_104_5_end
        jmp if_36_8_104_5_code
        if_36_8_104_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_104_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_104_5_end:
        if_36_5_104_5_end:
;        free scratch register 'r15'
    assert_104_5_end:
;    var x: i64 @ qword [rsp - 88]
;    [106:5] var x = 1
;    [106:13] x =1
;    [106:13] 1
;    [106:13] 1
;    [107:5] x = 1
    mov qword [rsp - 88], 1
;    var y: i64 @ qword [rsp - 96]
;    [107:5] var y = 2
;    [107:13] y =2
;    [107:13] 2
;    [107:13] 2
;    [109:5] y = 2
    mov qword [rsp - 96], 2
;    var o1: object @ qword [rsp - 116]
;    [109:5] var o1 : object = {{x * 10, y}, 0xff0000}
;    [109:23] o1 ={{x * 10, y}, 0xff0000}
;    [109:23] {{x * 10, y}, 0xff0000}
;    [109:23] {{x * 10, y}, 0xff0000}
;        [109:24] {x * 10, y}
;        [109:25] x * 10
;        [109:25] x * 10
;        [109:27] o1.pos.x = x
;        allocate scratch register -> r15
        mov r15, qword [rsp - 88]
        mov qword [rsp - 116], r15
;        free scratch register 'r15'
;        [109:31] o1.pos.x * 10
;        allocate scratch register -> r15
        mov r15, qword [rsp - 116]
        imul r15, 10
        mov qword [rsp - 116], r15
;        free scratch register 'r15'
;        [109:33]  y
;        [109:33] y
;        [109:34] o1.pos.y = y
;        allocate scratch register -> r15
        mov r15, qword [rsp - 96]
        mov qword [rsp - 108], r15
;        free scratch register 'r15'
;    [109:37]  0xff0000
;    [109:37] 0xff0000
;    [109:45] o1.color = 0xff0000
    mov dword [rsp - 100], 0xff0000
;    [110:5]  assert(o1.pos.x == 10)
;    allocate scratch register -> r15
;    [110:12] o1.pos.x == 10
;    [110:12] ? o1.pos.x == 10
;    [110:12] ? o1.pos.x == 10
    cmp_110_12:
    cmp qword [rsp - 116], 10
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
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_110_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_110_5_end:
        if_36_5_110_5_end:
;        free scratch register 'r15'
    assert_110_5_end:
;    [111:5] assert(o1.pos.y == 2)
;    allocate scratch register -> r15
;    [111:12] o1.pos.y == 2
;    [111:12] ? o1.pos.y == 2
;    [111:12] ? o1.pos.y == 2
    cmp_111_12:
    cmp qword [rsp - 108], 2
    jne bool_false_111_12
    jmp bool_true_111_12
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
        jmp if_36_8_111_5_code
        if_36_8_111_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_111_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_111_5_end:
        if_36_5_111_5_end:
;        free scratch register 'r15'
    assert_111_5_end:
;    [112:5] assert(o1.color == 0xff0000)
;    allocate scratch register -> r15
;    [112:12] o1.color == 0xff0000
;    [112:12] ? o1.color == 0xff0000
;    [112:12] ? o1.color == 0xff0000
    cmp_112_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_112_12
    jmp bool_true_112_12
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
        jmp if_36_8_112_5_code
        if_36_8_112_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_112_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_112_5_end:
        if_36_5_112_5_end:
;        free scratch register 'r15'
    assert_112_5_end:
;    var p1: point @ qword [rsp - 132]
;    [114:5] var p1 : point = {-x, -y}
;    [114:22] p1 ={-x, -y}
;    [114:22] {-x, -y}
;    [114:22] {-x, -y}
;    [114:23] -x
;    [114:23] -x
;    [114:25] p1.x = -x
;    allocate scratch register -> r15
    mov r15, qword [rsp - 88]
    mov qword [rsp - 132], r15
;    free scratch register 'r15'
    neg qword [rsp - 132]
;    [114:27]  -y
;    [114:27] -y
;    [114:29] p1.y = -y
;    allocate scratch register -> r15
    mov r15, qword [rsp - 96]
    mov qword [rsp - 124], r15
;    free scratch register 'r15'
    neg qword [rsp - 124]
;    [115:7]  o1.pos = p1
;    allocate named register 'rsi'
;    allocate named register 'rdi'
;    allocate named register 'rcx'
    lea rdi, [rsp - 116]
;    [115:14]  p1
    lea rsi, [rsp - 132]
    mov rcx, 16
    rep movsb
;    free named register 'rcx'
;    free named register 'rdi'
;    free named register 'rsi'
;    [116:5] assert(o1.pos.x == -1)
;    allocate scratch register -> r15
;    [116:12] o1.pos.x == -1
;    [116:12] ? o1.pos.x == -1
;    [116:12] ? o1.pos.x == -1
    cmp_116_12:
    cmp qword [rsp - 116], -1
    jne bool_false_116_12
    jmp bool_true_116_12
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
        jmp if_36_8_116_5_code
        if_36_8_116_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_116_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_116_5_end:
        if_36_5_116_5_end:
;        free scratch register 'r15'
    assert_116_5_end:
;    [117:5] assert(o1.pos.y == -2)
;    allocate scratch register -> r15
;    [117:12] o1.pos.y == -2
;    [117:12] ? o1.pos.y == -2
;    [117:12] ? o1.pos.y == -2
    cmp_117_12:
    cmp qword [rsp - 108], -2
    jne bool_false_117_12
    jmp bool_true_117_12
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
        jmp if_36_8_117_5_code
        if_36_8_117_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_117_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_117_5_end:
        if_36_5_117_5_end:
;        free scratch register 'r15'
    assert_117_5_end:
;    var o2: object @ qword [rsp - 152]
;    [119:5] var o2 : object = o1
;    [119:23] o2 =o1
;    allocate named register 'rsi'
;    allocate named register 'rdi'
;    allocate named register 'rcx'
    lea rdi, [rsp - 152]
;    [119:23] o1
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
;    free named register 'rcx'
;    free named register 'rdi'
;    free named register 'rsi'
;    [120:5] assert(o2.pos.x == -1)
;    allocate scratch register -> r15
;    [120:12] o2.pos.x == -1
;    [120:12] ? o2.pos.x == -1
;    [120:12] ? o2.pos.x == -1
    cmp_120_12:
    cmp qword [rsp - 152], -1
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
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_120_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_120_5_end:
        if_36_5_120_5_end:
;        free scratch register 'r15'
    assert_120_5_end:
;    [121:5] assert(o2.pos.y == -2)
;    allocate scratch register -> r15
;    [121:12] o2.pos.y == -2
;    [121:12] ? o2.pos.y == -2
;    [121:12] ? o2.pos.y == -2
    cmp_121_12:
    cmp qword [rsp - 144], -2
    jne bool_false_121_12
    jmp bool_true_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
;    assert(expr : bool) 
    assert_121_5:
;        alias expr -> r15
        if_36_8_121_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_121_5:
        cmp r15, false
        jne if_36_5_121_5_end
        jmp if_36_8_121_5_code
        if_36_8_121_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_121_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_121_5_end:
        if_36_5_121_5_end:
;        free scratch register 'r15'
    assert_121_5_end:
;    [122:5] assert(o2.color == 0xff0000)
;    allocate scratch register -> r15
;    [122:12] o2.color == 0xff0000
;    [122:12] ? o2.color == 0xff0000
;    [122:12] ? o2.color == 0xff0000
    cmp_122_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_122_12
    jmp bool_true_122_12
    bool_true_122_12:
    mov r15, true
    jmp bool_end_122_12
    bool_false_122_12:
    mov r15, false
    bool_end_122_12:
;    assert(expr : bool) 
    assert_122_5:
;        alias expr -> r15
        if_36_8_122_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_122_5:
        cmp r15, false
        jne if_36_5_122_5_end
        jmp if_36_8_122_5_code
        if_36_8_122_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_122_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_122_5_end:
        if_36_5_122_5_end:
;        free scratch register 'r15'
    assert_122_5_end:
;    var o3: object[1] @ qword [rsp - 172]
;    [124:5] var o3 : object[1]
;    clear array 1 * 20 B = 20 B
;    allocate named register 'rdi'
;    allocate named register 'rcx'
;    allocate named register 'rax'
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
;    free named register 'rax'
;    free named register 'rcx'
;    free named register 'rdi'
;    [125:5] # index 0 in an array can be accessed without array index
;    [126:7] o3.pos.y = 73
;    [126:16]  73
;    [126:16] 73
;    [127:5] o3.pos.y = 73
    mov qword [rsp - 164], 73
;    [127:5] assert(o3[0].pos.y == 73)
;    allocate scratch register -> r15
;    [127:12] o3[0].pos.y == 73
;    [127:12] ? o3[0].pos.y == 73
;    [127:12] ? o3[0].pos.y == 73
    cmp_127_12:
;    allocate scratch register -> r14
;        [127:12] o3[0].pos.y
;        [127:14] r14 = o3[0].pos.y
;        [127:14] o3[0].pos.y
;        allocate scratch register -> r13
        lea r13, [rsp - 172]
;        allocate scratch register -> r12
;        [127:15] 0
;        [127:15] 0
;        [127:16] r12 = 0
        mov r12, 0
        imul r12, 20
        add r13, r12
;        free scratch register 'r12'
        add r13, 8
        mov r14, qword [r13]
;        free scratch register 'r13'
    cmp r14, 73
;    free scratch register 'r14'
    jne bool_false_127_12
    jmp bool_true_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
;    assert(expr : bool) 
    assert_127_5:
;        alias expr -> r15
        if_36_8_127_5:
;        [36:8] ? not expr
;        [36:8] ? not expr
        cmp_36_8_127_5:
        cmp r15, false
        jne if_36_5_127_5_end
        jmp if_36_8_127_5_code
        if_36_8_127_5_code:
;            [36:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_36_17_127_5:
;                alias v -> rdi
;                [13:5]  mov(rax, 60)
                mov rax, 60
;                [13:19] # exit system call
;                [14:5] mov(rdi, v)
;                [14:19] # return code
;                [15:5] syscall()
                syscall
;                free named register 'rdi'
            exit_36_17_127_5_end:
        if_36_5_127_5_end:
;        free scratch register 'r15'
    assert_127_5_end:
;    [129:5] print(hello.len, hello)
;    allocate named register 'rdx'
    mov rdx, hello.len
;    allocate named register 'rsi'
    mov rsi, hello
;    print(len : reg_rdx, ptr : reg_rsi) 
    print_129_5:
;        alias len -> rdx
;        alias ptr -> rsi
;        [19:5]  mov(rax, 1)
        mov rax, 1
;        [19:19] # write system call
;        [20:5] mov(rdi, 1)
        mov rdi, 1
;        [20:19] # file descriptor for standard out
;        [21:5] mov(rsi, ptr)
;        [21:19] # buffer address
;        [22:5] mov(rdx, len)
;        [22:19] # buffer size
;        [23:5] syscall()
        syscall
;        free named register 'rsi'
;        free named register 'rdx'
    print_129_5_end:
;    [130:5] loop
    loop_130_5:
;        [131:9]  print(prompt1.len, prompt1)
;        allocate named register 'rdx'
        mov rdx, prompt1.len
;        allocate named register 'rsi'
        mov rsi, prompt1
;        print(len : reg_rdx, ptr : reg_rsi) 
        print_131_9:
;            alias len -> rdx
;            alias ptr -> rsi
;            [19:5]  mov(rax, 1)
            mov rax, 1
;            [19:19] # write system call
;            [20:5] mov(rdi, 1)
            mov rdi, 1
;            [20:19] # file descriptor for standard out
;            [21:5] mov(rsi, ptr)
;            [21:19] # buffer address
;            [22:5] mov(rdx, len)
;            [22:19] # buffer size
;            [23:5] syscall()
            syscall
;            free named register 'rsi'
;            free named register 'rdx'
        print_131_9_end:
;        var len: i64 @ qword [rsp - 180]
;        [132:9] var len = read(input.len, input) - 1
;        [132:19] len =read(input.len, input) - 1
;        [132:19] read(input.len, input) - 1
;        [132:19] read(input.len, input) - 1
;        [132:19] len = read(input.len, input)
;        [132:19] read(input.len, input)
;        allocate named register 'rdx'
        mov rdx, input.len
;        allocate named register 'rsi'
        mov rsi, input
;        read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_132_19:
;            alias nbytes -> len
;            alias len -> rdx
;            alias ptr -> rsi
;            [27:5]  mov(rax, 0)
            mov rax, 0
;            [27:19] # read system call
;            [28:5] mov(rdi, 0)
            mov rdi, 0
;            [28:19] # file descriptor for standard input
;            [29:5] mov(rsi, ptr)
;            [29:19] # buffer address
;            [30:5] mov(rdx, len)
;            [30:19] # buffer size
;            [31:5] syscall()
            syscall
;            [32:5] mov(nbytes, rax)
            mov qword [rsp - 180], rax
;            [32:22] # return value
;            free named register 'rsi'
;            free named register 'rdx'
        read_132_19_end:
;        [132:49] len - 1
        sub qword [rsp - 180], 1
;        [132:49] # -1 don't include the '\n'
        if_133_12:
;        [133:12] ? len == 0
;        [133:12] ? len == 0
        cmp_133_12:
        cmp qword [rsp - 180], 0
        jne if_135_19
        jmp if_133_12_code
        if_133_12_code:
;            [134:13]  break
            jmp loop_130_5_end
        jmp if_133_9_end
        if_135_19:
;        [135:19] ? len <= 4
;        [135:19] ? len <= 4
        cmp_135_19:
        cmp qword [rsp - 180], 4
        jg if_else_133_9
        jmp if_135_19_code
        if_135_19_code:
;            [136:13]  print(prompt2.len, prompt2)
;            allocate named register 'rdx'
            mov rdx, prompt2.len
;            allocate named register 'rsi'
            mov rsi, prompt2
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_136_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5] mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5] mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5] mov(rdx, len)
;                [22:19] # buffer size
;                [23:5] syscall()
                syscall
;                free named register 'rsi'
;                free named register 'rdx'
            print_136_13_end:
;            [137:13] continue
            jmp loop_130_5
        jmp if_133_9_end
        if_else_133_9:
;            [139:13]  print(prompt3.len, prompt3)
;            allocate named register 'rdx'
            mov rdx, prompt3.len
;            allocate named register 'rsi'
            mov rsi, prompt3
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_139_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5] mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5] mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5] mov(rdx, len)
;                [22:19] # buffer size
;                [23:5] syscall()
                syscall
;                free named register 'rsi'
;                free named register 'rdx'
            print_139_13_end:
;            [140:13] print(len, input)
;            allocate named register 'rdx'
            mov rdx, qword [rsp - 180]
;            allocate named register 'rsi'
            mov rsi, input
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_140_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5] mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5] mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5] mov(rdx, len)
;                [22:19] # buffer size
;                [23:5] syscall()
                syscall
;                free named register 'rsi'
;                free named register 'rdx'
            print_140_13_end:
;            [141:13] print(dot.len, dot)
;            allocate named register 'rdx'
            mov rdx, dot.len
;            allocate named register 'rsi'
            mov rsi, dot
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_141_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5] mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5] mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5] mov(rdx, len)
;                [22:19] # buffer size
;                [23:5] syscall()
                syscall
;                free named register 'rsi'
;                free named register 'rdx'
            print_141_13_end:
;            [142:13] print(nl.len, nl)
;            allocate named register 'rdx'
            mov rdx, nl.len
;            allocate named register 'rsi'
            mov rsi, nl
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_142_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [19:5]  mov(rax, 1)
                mov rax, 1
;                [19:19] # write system call
;                [20:5] mov(rdi, 1)
                mov rdi, 1
;                [20:19] # file descriptor for standard out
;                [21:5] mov(rsi, ptr)
;                [21:19] # buffer address
;                [22:5] mov(rdx, len)
;                [22:19] # buffer size
;                [23:5] syscall()
                syscall
;                free named register 'rsi'
;                free named register 'rdx'
            print_142_13_end:
        if_133_9_end:
    jmp loop_130_5
    loop_130_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; max scratch registers in use: 4
;            max frames in use: 7
;               max stack size: 180 B
```
