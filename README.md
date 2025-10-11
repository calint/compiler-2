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
C/C++ Header                    39           1013            670           4964
C++                              1             65             75            431
-------------------------------------------------------------------------------
SUM:                            40           1078            745           5395
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
hello: db 'hello world from baz',10,''
hello.len equ $-hello
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
    cmp_80_12:
        mov r13, 1
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
    cmp_81_12:
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
    assert_81_5:
        if_19_8_81_5:
        cmp_19_8_81_5:
        cmp r15, false
        jne if_19_5_81_5_end
        if_19_8_81_5_code:
            mov rdi, 1
            exit_19_17_81_5:
                mov rax, 60
                syscall
            exit_19_17_81_5_end:
        if_19_5_81_5_end:
    assert_81_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_84_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_84_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_85_12:
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
    assert_85_5:
        if_19_8_85_5:
        cmp_19_8_85_5:
        cmp r15, false
        jne if_19_5_85_5_end
        if_19_8_85_5_code:
            mov rdi, 1
            exit_19_17_85_5:
                mov rax, 60
                syscall
            exit_19_17_85_5_end:
        if_19_5_85_5_end:
    assert_85_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_88_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_88_5_end:
    cmp_89_12:
    cmp qword [rsp - 40], 2
    jne bool_false_89_12
    bool_true_89_12:
    mov r15, true
    jmp bool_end_89_12
    bool_false_89_12:
    mov r15, false
    bool_end_89_12:
    assert_89_5:
        if_19_8_89_5:
        cmp_19_8_89_5:
        cmp r15, false
        jne if_19_5_89_5_end
        if_19_8_89_5_code:
            mov rdi, 1
            exit_19_17_89_5:
                mov rax, 60
                syscall
            exit_19_17_89_5_end:
        if_19_5_89_5_end:
    assert_89_5_end:
    cmp_90_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
    assert_90_5:
        if_19_8_90_5:
        cmp_19_8_90_5:
        cmp r15, false
        jne if_19_5_90_5_end
        if_19_8_90_5_code:
            mov rdi, 1
            exit_19_17_90_5:
                mov rax, 60
                syscall
            exit_19_17_90_5_end:
        if_19_5_90_5_end:
    assert_90_5_end:
    mov qword [rsp - 48], 0
    bar_93_5:
        if_57_8_93_5:
        cmp_57_8_93_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_93_5_end
        if_57_8_93_5_code:
            jmp bar_93_5_end
        if_57_5_93_5_end:
        mov qword [rsp - 48], 0xff
    bar_93_5_end:
    cmp_94_12:
    cmp qword [rsp - 48], 0
    jne bool_false_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
    assert_94_5:
        if_19_8_94_5:
        cmp_19_8_94_5:
        cmp r15, false
        jne if_19_5_94_5_end
        if_19_8_94_5_code:
            mov rdi, 1
            exit_19_17_94_5:
                mov rax, 60
                syscall
            exit_19_17_94_5_end:
        if_19_5_94_5_end:
    assert_94_5_end:
    mov qword [rsp - 48], 1
    bar_97_5:
        if_57_8_97_5:
        cmp_57_8_97_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_97_5_end
        if_57_8_97_5_code:
            jmp bar_97_5_end
        if_57_5_97_5_end:
        mov qword [rsp - 48], 0xff
    bar_97_5_end:
    cmp_98_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_98_12
    bool_true_98_12:
    mov r15, true
    jmp bool_end_98_12
    bool_false_98_12:
    mov r15, false
    bool_end_98_12:
    assert_98_5:
        if_19_8_98_5:
        cmp_19_8_98_5:
        cmp r15, false
        jne if_19_5_98_5_end
        if_19_8_98_5_code:
            mov rdi, 1
            exit_19_17_98_5:
                mov rax, 60
                syscall
            exit_19_17_98_5_end:
        if_19_5_98_5_end:
    assert_98_5_end:
    mov qword [rsp - 56], 1
    baz_101_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_101_13_end:
    cmp_102_12:
    cmp qword [rsp - 64], 2
    jne bool_false_102_12
    bool_true_102_12:
    mov r15, true
    jmp bool_end_102_12
    bool_false_102_12:
    mov r15, false
    bool_end_102_12:
    assert_102_5:
        if_19_8_102_5:
        cmp_19_8_102_5:
        cmp r15, false
        jne if_19_5_102_5_end
        if_19_8_102_5_code:
            mov rdi, 1
            exit_19_17_102_5:
                mov rax, 60
                syscall
            exit_19_17_102_5_end:
        if_19_5_102_5_end:
    assert_102_5_end:
    baz_104_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_104_9_end:
    cmp_105_12:
    cmp qword [rsp - 64], 2
    jne bool_false_105_12
    bool_true_105_12:
    mov r15, true
    jmp bool_end_105_12
    bool_false_105_12:
    mov r15, false
    bool_end_105_12:
    assert_105_5:
        if_19_8_105_5:
        cmp_19_8_105_5:
        cmp r15, false
        jne if_19_5_105_5_end
        if_19_8_105_5_code:
            mov rdi, 1
            exit_19_17_105_5:
                mov rax, 60
                syscall
            exit_19_17_105_5_end:
        if_19_5_105_5_end:
    assert_105_5_end:
    baz_107_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_107_23_end:
    mov qword [rsp - 72], 0
    cmp_108_12:
    cmp qword [rsp - 80], 4
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
    cmp_114_12:
    cmp qword [rsp - 116], 10
    jne bool_false_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
    assert_114_5:
        if_19_8_114_5:
        cmp_19_8_114_5:
        cmp r15, false
        jne if_19_5_114_5_end
        if_19_8_114_5_code:
            mov rdi, 1
            exit_19_17_114_5:
                mov rax, 60
                syscall
            exit_19_17_114_5_end:
        if_19_5_114_5_end:
    assert_114_5_end:
    cmp_115_12:
    cmp qword [rsp - 108], 2
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
    cmp_116_12:
cmp dword [rsp - 100], 0xff0000
    jne bool_false_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
    assert_116_5:
        if_19_8_116_5:
        cmp_19_8_116_5:
        cmp r15, false
        jne if_19_5_116_5_end
        if_19_8_116_5_code:
            mov rdi, 1
            exit_19_17_116_5:
                mov rax, 60
                syscall
            exit_19_17_116_5_end:
        if_19_5_116_5_end:
    assert_116_5_end:
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
    cmp_120_12:
    cmp qword [rsp - 116], -1
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_19_8_120_5:
        cmp_19_8_120_5:
        cmp r15, false
        jne if_19_5_120_5_end
        if_19_8_120_5_code:
            mov rdi, 1
            exit_19_17_120_5:
                mov rax, 60
                syscall
            exit_19_17_120_5_end:
        if_19_5_120_5_end:
    assert_120_5_end:
    cmp_121_12:
    cmp qword [rsp - 108], -2
    jne bool_false_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_19_8_121_5:
        cmp_19_8_121_5:
        cmp r15, false
        jne if_19_5_121_5_end
        if_19_8_121_5_code:
            mov rdi, 1
            exit_19_17_121_5:
                mov rax, 60
                syscall
            exit_19_17_121_5_end:
        if_19_5_121_5_end:
    assert_121_5_end:
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_124_12:
    cmp qword [rsp - 152], -1
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
    cmp qword [rsp - 144], -2
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
cmp dword [rsp - 136], 0xff0000
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
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 164], 73
    cmp_131_12:
        lea r13, [rsp - 172]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
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
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_135_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_135_5_end:
    loop_136_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_137_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_137_9_end:
        mov rdx, 80
        lea rsi, [rsp - 252]
        read_139_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 260], rax
        read_139_19_end:
        sub qword [rsp - 260], 1
        if_143_12:
        cmp_143_12:
        cmp qword [rsp - 260], 0
        jne if_145_19
        if_143_12_code:
            jmp loop_136_5_end
        jmp if_143_9_end
        if_145_19:
        cmp_145_19:
        cmp qword [rsp - 260], 4
        jg if_else_143_9
        if_145_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_146_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_146_13_end:
            jmp loop_136_5
        jmp if_143_9_end
        if_else_143_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_149_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_149_13_end:
            mov rdx, qword [rsp - 260]
            lea rsi, [rsp - 252]
            print_150_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_150_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_151_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_151_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_152_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_152_13_end:
        if_143_9_end:
    jmp loop_136_5
    loop_136_5_end:
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
;[2:1] field prompt1 = "enter name:\n"
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
;[3:1] field prompt2 = "that is not a name.\n"
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
;[4:1] field prompt3 = "hello "
prompt3: db 'hello '
prompt3.len equ $-prompt3
;[5:1] field dot = "."
dot: db '.'
dot.len equ $-dot
;[6:1] field nl = "\n"
nl: db '',10,''
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
;   [80:5] assert(arr[1] == 2)
;   allocate scratch register -> r15
;   [80:12] arr[1] == 2
;   [80:12] ? arr[1] == 2
;   [80:12] ? arr[1] == 2
    cmp_80_12:
;   allocate scratch register -> r14
;       [80:12] arr[1]
;       [80:15] r14 = arr[1]
;       [80:15] arr[1]
;       allocate scratch register -> r13
;       [80:16] 1
;       [80:16] 1
;       [80:17] r13 = 1
        mov r13, 1
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
;   [81:5] assert(arr[2] == 2)
;   allocate scratch register -> r15
;   [81:12] arr[2] == 2
;   [81:12] ? arr[2] == 2
;   [81:12] ? arr[2] == 2
    cmp_81_12:
;   allocate scratch register -> r14
;       [81:12] arr[2]
;       [81:15] r14 = arr[2]
;       [81:15] arr[2]
;       allocate scratch register -> r13
;       [81:16] 2
;       [81:16] 2
;       [81:17] r13 = 2
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
;       free scratch register 'r13'
    cmp r14, 2
;   free scratch register 'r14'
    jne bool_false_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
;   assert(expr : bool) 
    assert_81_5:
;       alias expr -> r15
        if_19_8_81_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_81_5:
        cmp r15, false
        jne if_19_5_81_5_end
        if_19_8_81_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_81_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_81_5_end:
        if_19_5_81_5_end:
;       free scratch register 'r15'
    assert_81_5_end:
;   [83:8] ix = 3
;   [83:10] 3
;   [83:10] 3
;   [84:5] ix = 3
    mov qword [rsp - 24], 3
;   [84:8] arr[ix] = ~inv(arr[ix - 1])
;   allocate scratch register -> r15
;   [84:9] ix
;   [84:9] ix
;   [84:11] r15 = ix
    mov r15, qword [rsp - 24]
;   [84:15] ~inv(arr[ix - 1])
;   [84:15] ~inv(arr[ix - 1])
;   [84:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;   [84:16] ~inv(arr[ix - 1])
;   allocate scratch register -> r14
;   [84:20] arr[ix - 1]
;   [84:20] arr[ix - 1]
;   [84:23] r14 = arr[ix - 1]
;   [84:23] arr[ix - 1]
;   allocate scratch register -> r13
;   [84:24] ix - 1
;   [84:24] ix - 1
;   [84:27] r13 = ix
    mov r13, qword [rsp - 24]
;   [84:30] r13 - 1
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
;   free scratch register 'r13'
;   inv(i : i32) : i32 res 
    inv_84_16:
;       alias res -> dword [rsp + r15 * 4 - 16]
;       alias i -> r14
;       [64:9] res = ~i
;       [64:11] ~i
;       [64:11] ~i
;       [65:1] res = ~i
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
;       free scratch register 'r14'
    inv_84_16_end:
    not dword [rsp + r15 * 4 - 16]
;   free scratch register 'r15'
;   [85:5] assert(arr[ix] == 2)
;   allocate scratch register -> r15
;   [85:12] arr[ix] == 2
;   [85:12] ? arr[ix] == 2
;   [85:12] ? arr[ix] == 2
    cmp_85_12:
;   allocate scratch register -> r14
;       [85:12] arr[ix]
;       [85:15] r14 = arr[ix]
;       [85:15] arr[ix]
;       allocate scratch register -> r13
;       [85:16] ix
;       [85:16] ix
;       [85:18] r13 = ix
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
;       free scratch register 'r13'
    cmp r14, 2
;   free scratch register 'r14'
    jne bool_false_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
;   assert(expr : bool) 
    assert_85_5:
;       alias expr -> r15
        if_19_8_85_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_85_5:
        cmp r15, false
        jne if_19_5_85_5_end
        if_19_8_85_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_85_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_85_5_end:
        if_19_5_85_5_end:
;       free scratch register 'r15'
    assert_85_5_end:
;   var p: point @ qword [rsp - 40]
;   [87:5] var p : point = {0, 0}
;   [87:21] p ={0, 0}
;   [87:21] {0, 0}
;   [87:21] {0, 0}
;   [87:22] 0
;   [87:22] 0
;   [87:23] p.x = 0
    mov qword [rsp - 40], 0
;   [87:25] 0
;   [87:25] 0
;   [87:26] p.y = 0
    mov qword [rsp - 32], 0
;   [88:5] foo(p)
;   foo(pt : point) 
    foo_88_5:
;       alias pt -> p
;       [50:7] pt.x = 0b10
;       [50:12] 0b10
;       [50:12] 0b10
;       [50:20] pt.x = 0b10
        mov qword [rsp - 40], 0b10
;       [50:20] # binary value 2
;       [51:7] pt.y = 0xb
;       [51:12] 0xb
;       [51:12] 0xb
;       [51:20] pt.y = 0xb
        mov qword [rsp - 32], 0xb
;       [51:20] # hex value 11
    foo_88_5_end:
;   [89:5] assert(p.x == 2)
;   allocate scratch register -> r15
;   [89:12] p.x == 2
;   [89:12] ? p.x == 2
;   [89:12] ? p.x == 2
    cmp_89_12:
    cmp qword [rsp - 40], 2
    jne bool_false_89_12
    bool_true_89_12:
    mov r15, true
    jmp bool_end_89_12
    bool_false_89_12:
    mov r15, false
    bool_end_89_12:
;   assert(expr : bool) 
    assert_89_5:
;       alias expr -> r15
        if_19_8_89_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_89_5:
        cmp r15, false
        jne if_19_5_89_5_end
        if_19_8_89_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_89_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_89_5_end:
        if_19_5_89_5_end:
;       free scratch register 'r15'
    assert_89_5_end:
;   [90:5] assert(p.y == 0xb)
;   allocate scratch register -> r15
;   [90:12] p.y == 0xb
;   [90:12] ? p.y == 0xb
;   [90:12] ? p.y == 0xb
    cmp_90_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
;   assert(expr : bool) 
    assert_90_5:
;       alias expr -> r15
        if_19_8_90_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_90_5:
        cmp r15, false
        jne if_19_5_90_5_end
        if_19_8_90_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_90_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_90_5_end:
        if_19_5_90_5_end:
;       free scratch register 'r15'
    assert_90_5_end:
;   var i: i64 @ qword [rsp - 48]
;   [92:5] var i = 0
;   [92:13] i =0
;   [92:13] 0
;   [92:13] 0
;   [93:5] i = 0
    mov qword [rsp - 48], 0
;   [93:5] bar(i)
;   bar(arg) 
    bar_93_5:
;       alias arg -> i
        if_57_8_93_5:
;       [57:8] ? arg == 0
;       [57:8] ? arg == 0
        cmp_57_8_93_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_93_5_end
        if_57_8_93_5_code:
;           [57:17] return
            jmp bar_93_5_end
        if_57_5_93_5_end:
;       [58:9] arg = 0xff
;       [58:11] 0xff
;       [58:11] 0xff
;       [59:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_93_5_end:
;   [94:5] assert(i == 0)
;   allocate scratch register -> r15
;   [94:12] i == 0
;   [94:12] ? i == 0
;   [94:12] ? i == 0
    cmp_94_12:
    cmp qword [rsp - 48], 0
    jne bool_false_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
;   assert(expr : bool) 
    assert_94_5:
;       alias expr -> r15
        if_19_8_94_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_94_5:
        cmp r15, false
        jne if_19_5_94_5_end
        if_19_8_94_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_94_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_94_5_end:
        if_19_5_94_5_end:
;       free scratch register 'r15'
    assert_94_5_end:
;   [96:7] i = 1
;   [96:9] 1
;   [96:9] 1
;   [97:5] i = 1
    mov qword [rsp - 48], 1
;   [97:5] bar(i)
;   bar(arg) 
    bar_97_5:
;       alias arg -> i
        if_57_8_97_5:
;       [57:8] ? arg == 0
;       [57:8] ? arg == 0
        cmp_57_8_97_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_97_5_end
        if_57_8_97_5_code:
;           [57:17] return
            jmp bar_97_5_end
        if_57_5_97_5_end:
;       [58:9] arg = 0xff
;       [58:11] 0xff
;       [58:11] 0xff
;       [59:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_97_5_end:
;   [98:5] assert(i == 0xff)
;   allocate scratch register -> r15
;   [98:12] i == 0xff
;   [98:12] ? i == 0xff
;   [98:12] ? i == 0xff
    cmp_98_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_98_12
    bool_true_98_12:
    mov r15, true
    jmp bool_end_98_12
    bool_false_98_12:
    mov r15, false
    bool_end_98_12:
;   assert(expr : bool) 
    assert_98_5:
;       alias expr -> r15
        if_19_8_98_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_98_5:
        cmp r15, false
        jne if_19_5_98_5_end
        if_19_8_98_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_98_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_98_5_end:
        if_19_5_98_5_end:
;       free scratch register 'r15'
    assert_98_5_end:
;   var j: i64 @ qword [rsp - 56]
;   [100:5] var j = 1
;   [100:13] j =1
;   [100:13] 1
;   [100:13] 1
;   [101:5] j = 1
    mov qword [rsp - 56], 1
;   var k: i64 @ qword [rsp - 64]
;   [101:5] var k = baz(j)
;   [101:13] k =baz(j)
;   [101:13] baz(j)
;   [101:13] baz(j)
;   [101:13] k = baz(j)
;   [101:13] baz(j)
;   baz(arg) : i64 res 
    baz_101_13:
;       alias res -> k
;       alias arg -> j
;       [68:9] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:15] res = arg
;       allocate scratch register -> r15
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
;       free scratch register 'r15'
;       [69:1] res * 2
;       allocate scratch register -> r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;       free scratch register 'r15'
    baz_101_13_end:
;   [102:5] assert(k == 2)
;   allocate scratch register -> r15
;   [102:12] k == 2
;   [102:12] ? k == 2
;   [102:12] ? k == 2
    cmp_102_12:
    cmp qword [rsp - 64], 2
    jne bool_false_102_12
    bool_true_102_12:
    mov r15, true
    jmp bool_end_102_12
    bool_false_102_12:
    mov r15, false
    bool_end_102_12:
;   assert(expr : bool) 
    assert_102_5:
;       alias expr -> r15
        if_19_8_102_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_102_5:
        cmp r15, false
        jne if_19_5_102_5_end
        if_19_8_102_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_102_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_102_5_end:
        if_19_5_102_5_end:
;       free scratch register 'r15'
    assert_102_5_end:
;   [104:7] k = baz(1)
;   [104:9] baz(1)
;   [104:9] baz(1)
;   [104:9] k = baz(1)
;   [104:9] baz(1)
;   baz(arg) : i64 res 
    baz_104_9:
;       alias res -> k
;       alias arg -> 1
;       [68:9] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:15] res = arg
        mov qword [rsp - 64], 1
;       [69:1] res * 2
;       allocate scratch register -> r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;       free scratch register 'r15'
    baz_104_9_end:
;   [105:5] assert(k == 2)
;   allocate scratch register -> r15
;   [105:12] k == 2
;   [105:12] ? k == 2
;   [105:12] ? k == 2
    cmp_105_12:
    cmp qword [rsp - 64], 2
    jne bool_false_105_12
    bool_true_105_12:
    mov r15, true
    jmp bool_end_105_12
    bool_false_105_12:
    mov r15, false
    bool_end_105_12:
;   assert(expr : bool) 
    assert_105_5:
;       alias expr -> r15
        if_19_8_105_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_105_5:
        cmp r15, false
        jne if_19_5_105_5_end
        if_19_8_105_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_105_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_105_5_end:
        if_19_5_105_5_end:
;       free scratch register 'r15'
    assert_105_5_end:
;   var p0: point @ qword [rsp - 80]
;   [107:5] var p0 : point = {baz(2), 0}
;   [107:22] p0 ={baz(2), 0}
;   [107:22] {baz(2), 0}
;   [107:22] {baz(2), 0}
;   [107:23] baz(2)
;   [107:23] baz(2)
;   [107:23] p0.x = baz(2)
;   [107:23] baz(2)
;   baz(arg) : i64 res 
    baz_107_23:
;       alias res -> p0.x
;       alias arg -> 2
;       [68:9] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:15] res = arg
        mov qword [rsp - 80], 2
;       [69:1] res * 2
;       allocate scratch register -> r15
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
;       free scratch register 'r15'
    baz_107_23_end:
;   [107:31] 0
;   [107:31] 0
;   [107:32] p0.y = 0
    mov qword [rsp - 72], 0
;   [108:5] assert(p0.x == 4)
;   allocate scratch register -> r15
;   [108:12] p0.x == 4
;   [108:12] ? p0.x == 4
;   [108:12] ? p0.x == 4
    cmp_108_12:
    cmp qword [rsp - 80], 4
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
;   var x: i64 @ qword [rsp - 88]
;   [110:5] var x = 1
;   [110:13] x =1
;   [110:13] 1
;   [110:13] 1
;   [111:5] x = 1
    mov qword [rsp - 88], 1
;   var y: i64 @ qword [rsp - 96]
;   [111:5] var y = 2
;   [111:13] y =2
;   [111:13] 2
;   [111:13] 2
;   [113:5] y = 2
    mov qword [rsp - 96], 2
;   var o1: object @ qword [rsp - 116]
;   [113:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [113:23] o1 ={{x * 10, y}, 0xff0000}
;   [113:23] {{x * 10, y}, 0xff0000}
;   [113:23] {{x * 10, y}, 0xff0000}
;       [113:24] {x * 10, y}
;       [113:25] x * 10
;       [113:25] x * 10
;       [113:27] o1.pos.x = x
;       allocate scratch register -> r15
        mov r15, qword [rsp - 88]
        mov qword [rsp - 116], r15
;       free scratch register 'r15'
;       [113:31] o1.pos.x * 10
;       allocate scratch register -> r15
        mov r15, qword [rsp - 116]
        imul r15, 10
        mov qword [rsp - 116], r15
;       free scratch register 'r15'
;       [113:33] y
;       [113:33] y
;       [113:34] o1.pos.y = y
;       allocate scratch register -> r15
        mov r15, qword [rsp - 96]
        mov qword [rsp - 108], r15
;       free scratch register 'r15'
;   [113:37] 0xff0000
;   [113:37] 0xff0000
;   [113:45] o1.color = 0xff0000
mov dword [rsp - 100], 0xff0000
;   [114:5] assert(o1.pos.x == 10)
;   allocate scratch register -> r15
;   [114:12] o1.pos.x == 10
;   [114:12] ? o1.pos.x == 10
;   [114:12] ? o1.pos.x == 10
    cmp_114_12:
    cmp qword [rsp - 116], 10
    jne bool_false_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
;   assert(expr : bool) 
    assert_114_5:
;       alias expr -> r15
        if_19_8_114_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_114_5:
        cmp r15, false
        jne if_19_5_114_5_end
        if_19_8_114_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_114_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_114_5_end:
        if_19_5_114_5_end:
;       free scratch register 'r15'
    assert_114_5_end:
;   [115:5] assert(o1.pos.y == 2)
;   allocate scratch register -> r15
;   [115:12] o1.pos.y == 2
;   [115:12] ? o1.pos.y == 2
;   [115:12] ? o1.pos.y == 2
    cmp_115_12:
    cmp qword [rsp - 108], 2
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
;   [116:5] assert(o1.color == 0xff0000)
;   allocate scratch register -> r15
;   [116:12] o1.color == 0xff0000
;   [116:12] ? o1.color == 0xff0000
;   [116:12] ? o1.color == 0xff0000
    cmp_116_12:
cmp dword [rsp - 100], 0xff0000
    jne bool_false_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
;   assert(expr : bool) 
    assert_116_5:
;       alias expr -> r15
        if_19_8_116_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_116_5:
        cmp r15, false
        jne if_19_5_116_5_end
        if_19_8_116_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_116_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_116_5_end:
        if_19_5_116_5_end:
;       free scratch register 'r15'
    assert_116_5_end:
;   var p1: point @ qword [rsp - 132]
;   [118:5] var p1 : point = {-x, -y}
;   [118:22] p1 ={-x, -y}
;   [118:22] {-x, -y}
;   [118:22] {-x, -y}
;   [118:23] -x
;   [118:23] -x
;   [118:25] p1.x = -x
;   allocate scratch register -> r15
    mov r15, qword [rsp - 88]
    mov qword [rsp - 132], r15
;   free scratch register 'r15'
    neg qword [rsp - 132]
;   [118:27] -y
;   [118:27] -y
;   [118:29] p1.y = -y
;   allocate scratch register -> r15
    mov r15, qword [rsp - 96]
    mov qword [rsp - 124], r15
;   free scratch register 'r15'
    neg qword [rsp - 124]
;   [119:7] o1.pos = p1
;   allocate named register 'rsi'
;   allocate named register 'rdi'
;   allocate named register 'rcx'
    lea rdi, [rsp - 116]
;   [119:14] p1
    lea rsi, [rsp - 132]
    mov rcx, 16
    rep movsb
;   free named register 'rcx'
;   free named register 'rdi'
;   free named register 'rsi'
;   [120:5] assert(o1.pos.x == -1)
;   allocate scratch register -> r15
;   [120:12] o1.pos.x == -1
;   [120:12] ? o1.pos.x == -1
;   [120:12] ? o1.pos.x == -1
    cmp_120_12:
    cmp qword [rsp - 116], -1
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
;   assert(expr : bool) 
    assert_120_5:
;       alias expr -> r15
        if_19_8_120_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_120_5:
        cmp r15, false
        jne if_19_5_120_5_end
        if_19_8_120_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_120_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_120_5_end:
        if_19_5_120_5_end:
;       free scratch register 'r15'
    assert_120_5_end:
;   [121:5] assert(o1.pos.y == -2)
;   allocate scratch register -> r15
;   [121:12] o1.pos.y == -2
;   [121:12] ? o1.pos.y == -2
;   [121:12] ? o1.pos.y == -2
    cmp_121_12:
    cmp qword [rsp - 108], -2
    jne bool_false_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
;   assert(expr : bool) 
    assert_121_5:
;       alias expr -> r15
        if_19_8_121_5:
;       [19:8] ? not expr
;       [19:8] ? not expr
        cmp_19_8_121_5:
        cmp r15, false
        jne if_19_5_121_5_end
        if_19_8_121_5_code:
;           [19:17] exit(1)
;           allocate named register 'rdi'
            mov rdi, 1
;           exit(v : reg_rdi) 
            exit_19_17_121_5:
;               alias v -> rdi
;               [13:5] mov(rax, 60)
                mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               free named register 'rdi'
            exit_19_17_121_5_end:
        if_19_5_121_5_end:
;       free scratch register 'r15'
    assert_121_5_end:
;   var o2: object @ qword [rsp - 152]
;   [123:5] var o2 : object = o1
;   [123:23] o2 =o1
;   allocate named register 'rsi'
;   allocate named register 'rdi'
;   allocate named register 'rcx'
    lea rdi, [rsp - 152]
;   [123:23] o1
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
;   free named register 'rcx'
;   free named register 'rdi'
;   free named register 'rsi'
;   [124:5] assert(o2.pos.x == -1)
;   allocate scratch register -> r15
;   [124:12] o2.pos.x == -1
;   [124:12] ? o2.pos.x == -1
;   [124:12] ? o2.pos.x == -1
    cmp_124_12:
    cmp qword [rsp - 152], -1
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
;   [125:5] assert(o2.pos.y == -2)
;   allocate scratch register -> r15
;   [125:12] o2.pos.y == -2
;   [125:12] ? o2.pos.y == -2
;   [125:12] ? o2.pos.y == -2
    cmp_125_12:
    cmp qword [rsp - 144], -2
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
;   [126:5] assert(o2.color == 0xff0000)
;   allocate scratch register -> r15
;   [126:12] o2.color == 0xff0000
;   [126:12] ? o2.color == 0xff0000
;   [126:12] ? o2.color == 0xff0000
    cmp_126_12:
cmp dword [rsp - 136], 0xff0000
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
;   var o3: object[1] @ qword [rsp - 172]
;   [128:5] var o3 : object[1]
;   clear array 1 * 20 B = 20 B
;   allocate named register 'rdi'
;   allocate named register 'rcx'
;   allocate named register 'rax'
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
;   free named register 'rax'
;   free named register 'rcx'
;   free named register 'rdi'
;   [129:5] # index 0 in an array can be accessed without array index
;   [130:7] o3.pos.y = 73
;   [130:16] 73
;   [130:16] 73
;   [131:5] o3.pos.y = 73
    mov qword [rsp - 164], 73
;   [131:5] assert(o3[0].pos.y == 73)
;   allocate scratch register -> r15
;   [131:12] o3[0].pos.y == 73
;   [131:12] ? o3[0].pos.y == 73
;   [131:12] ? o3[0].pos.y == 73
    cmp_131_12:
;   allocate scratch register -> r14
;       [131:12] o3[0].pos.y
;       [131:14] r14 = o3[0].pos.y
;       [131:14] o3[0].pos.y
;       allocate scratch register -> r13
        lea r13, [rsp - 172]
;       allocate scratch register -> r12
;       [131:15] 0
;       [131:15] 0
;       [131:16] r12 = 0
        mov r12, 0
        imul r12, 20
        add r13, r12
;       free scratch register 'r12'
        add r13, 8
        mov r14, qword [r13]
;       free scratch register 'r13'
    cmp r14, 73
;   free scratch register 'r14'
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
;   var input_buffer: i8[80] @ byte [rsp - 252]
;   [133:5] var input_buffer : i8[80]
;   clear array 80 * 1 B = 80 B
;   allocate named register 'rdi'
;   allocate named register 'rcx'
;   allocate named register 'rax'
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
;   free named register 'rax'
;   free named register 'rcx'
;   free named register 'rdi'
;   [135:5] print(hello.len, hello)
;   allocate named register 'rdx'
    mov rdx, hello.len
;   allocate named register 'rsi'
    mov rsi, hello
;   print(len : reg_rdx, ptr : reg_rsi) 
    print_135_5:
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
    print_135_5_end:
;   [136:5] loop
    loop_136_5:
;       [137:9] print(prompt1.len, prompt1)
;       allocate named register 'rdx'
        mov rdx, prompt1.len
;       allocate named register 'rsi'
        mov rsi, prompt1
;       print(len : reg_rdx, ptr : reg_rsi) 
        print_137_9:
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
        print_137_9_end:
;       var len: i64 @ qword [rsp - 260]
;       [139:9] var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [139:19] len =read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [139:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [139:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [139:19] len = read(array_size_of(input_buffer), address_of(input_buffer))
;       [139:19] read(array_size_of(input_buffer), address_of(input_buffer))
;       allocate named register 'rdx'
;       [139:24] array_size_of(input_buffer)
;       [139:24] array_size_of(input_buffer)
;       [139:24] rdx = array_size_of(input_buffer)
;       [139:24] array_size_of(input_buffer)
        mov rdx, 80
;       allocate named register 'rsi'
;       [139:53] address_of(input_buffer)
;       [139:53] address_of(input_buffer)
;       [139:53] rsi = address_of(input_buffer)
;       [139:53] address_of(input_buffer)
        lea rsi, [rsp - 252]
;       read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_139_19:
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
            mov qword [rsp - 260], rax
;           [36:22] # return value
;           free named register 'rsi'
;           free named register 'rdx'
        read_139_19_end:
;       [140:9] len - 1
        sub qword [rsp - 260], 1
;       [140:9] # note: `array_size_of` and `address_of` are built-in functions
;       [141:9] # -1 to not include the trailing '\n'
        if_143_12:
;       [143:12] ? len == 0
;       [143:12] ? len == 0
        cmp_143_12:
        cmp qword [rsp - 260], 0
        jne if_145_19
        if_143_12_code:
;           [144:13] break
            jmp loop_136_5_end
        jmp if_143_9_end
        if_145_19:
;       [145:19] ? len <= 4
;       [145:19] ? len <= 4
        cmp_145_19:
        cmp qword [rsp - 260], 4
        jg if_else_143_9
        if_145_19_code:
;           [146:13] print(prompt2.len, prompt2)
;           allocate named register 'rdx'
            mov rdx, prompt2.len
;           allocate named register 'rsi'
            mov rsi, prompt2
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_146_13:
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
            print_146_13_end:
;           [147:13] continue
            jmp loop_136_5
        jmp if_143_9_end
        if_else_143_9:
;           [149:13] print(prompt3.len, prompt3)
;           allocate named register 'rdx'
            mov rdx, prompt3.len
;           allocate named register 'rsi'
            mov rsi, prompt3
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_149_13:
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
            print_149_13_end:
;           [150:13] print(len, address_of(input_buffer))
;           allocate named register 'rdx'
            mov rdx, qword [rsp - 260]
;           allocate named register 'rsi'
;           [150:24] address_of(input_buffer)
;           [150:24] address_of(input_buffer)
;           [150:24] rsi = address_of(input_buffer)
;           [150:24] address_of(input_buffer)
            lea rsi, [rsp - 252]
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_150_13:
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
            print_150_13_end:
;           [151:13] print(dot.len, dot)
;           allocate named register 'rdx'
            mov rdx, dot.len
;           allocate named register 'rsi'
            mov rsi, dot
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_151_13:
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
            print_151_13_end:
;           [152:13] print(nl.len, nl)
;           allocate named register 'rdx'
            mov rdx, nl.len
;           allocate named register 'rsi'
            mov rsi, nl
;           print(len : reg_rdx, ptr : reg_rsi) 
            print_152_13:
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
            print_152_13_end:
        if_143_9_end:
    jmp loop_136_5
    loop_136_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; max scratch registers in use: 4
;            max frames in use: 7
;               max stack size: 260 B
```
