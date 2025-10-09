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
C/C++ Header                    38            979            628           4846
C++                              1             65             78            430
-------------------------------------------------------------------------------
SUM:                            39           1044            706           5276
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

    var input_buffer : i8[80]

    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
        # note: array_size_of and address_of are built-in functions
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
    cmp_75_12:
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_75_12
    bool_true_75_12:
    mov r15, true
    jmp bool_end_75_12
    bool_false_75_12:
    mov r15, false
    bool_end_75_12:
    assert_75_5:
        if_18_8_75_5:
        cmp_18_8_75_5:
        cmp r15, false
        jne if_18_5_75_5_end
        if_18_8_75_5_code:
            mov rdi, 1
            exit_18_17_75_5:
                mov rax, 60
                syscall
            exit_18_17_75_5_end:
        if_18_5_75_5_end:
    assert_75_5_end:
    cmp_76_12:
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_76_12
    bool_true_76_12:
    mov r15, true
    jmp bool_end_76_12
    bool_false_76_12:
    mov r15, false
    bool_end_76_12:
    assert_76_5:
        if_18_8_76_5:
        cmp_18_8_76_5:
        cmp r15, false
        jne if_18_5_76_5_end
        if_18_8_76_5_code:
            mov rdi, 1
            exit_18_17_76_5:
                mov rax, 60
                syscall
            exit_18_17_76_5_end:
        if_18_5_76_5_end:
    assert_76_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_79_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_79_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_80_12:
        mov r13, qword [rsp - 24]
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
        if_18_8_80_5:
        cmp_18_8_80_5:
        cmp r15, false
        jne if_18_5_80_5_end
        if_18_8_80_5_code:
            mov rdi, 1
            exit_18_17_80_5:
                mov rax, 60
                syscall
            exit_18_17_80_5_end:
        if_18_5_80_5_end:
    assert_80_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_83_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_83_5_end:
    cmp_84_12:
    cmp qword [rsp - 40], 2
    jne bool_false_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
    assert_84_5:
        if_18_8_84_5:
        cmp_18_8_84_5:
        cmp r15, false
        jne if_18_5_84_5_end
        if_18_8_84_5_code:
            mov rdi, 1
            exit_18_17_84_5:
                mov rax, 60
                syscall
            exit_18_17_84_5_end:
        if_18_5_84_5_end:
    assert_84_5_end:
    cmp_85_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
    assert_85_5:
        if_18_8_85_5:
        cmp_18_8_85_5:
        cmp r15, false
        jne if_18_5_85_5_end
        if_18_8_85_5_code:
            mov rdi, 1
            exit_18_17_85_5:
                mov rax, 60
                syscall
            exit_18_17_85_5_end:
        if_18_5_85_5_end:
    assert_85_5_end:
    mov qword [rsp - 48], 0
    bar_88_5:
        if_53_8_88_5:
        cmp_53_8_88_5:
        cmp qword [rsp - 48], 0
        jne if_53_5_88_5_end
        if_53_8_88_5_code:
            jmp bar_88_5_end
        if_53_5_88_5_end:
        mov qword [rsp - 48], 0xff
    bar_88_5_end:
    cmp_89_12:
    cmp qword [rsp - 48], 0
    jne bool_false_89_12
    bool_true_89_12:
    mov r15, true
    jmp bool_end_89_12
    bool_false_89_12:
    mov r15, false
    bool_end_89_12:
    assert_89_5:
        if_18_8_89_5:
        cmp_18_8_89_5:
        cmp r15, false
        jne if_18_5_89_5_end
        if_18_8_89_5_code:
            mov rdi, 1
            exit_18_17_89_5:
                mov rax, 60
                syscall
            exit_18_17_89_5_end:
        if_18_5_89_5_end:
    assert_89_5_end:
    mov qword [rsp - 48], 1
    bar_92_5:
        if_53_8_92_5:
        cmp_53_8_92_5:
        cmp qword [rsp - 48], 0
        jne if_53_5_92_5_end
        if_53_8_92_5_code:
            jmp bar_92_5_end
        if_53_5_92_5_end:
        mov qword [rsp - 48], 0xff
    bar_92_5_end:
    cmp_93_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
    assert_93_5:
        if_18_8_93_5:
        cmp_18_8_93_5:
        cmp r15, false
        jne if_18_5_93_5_end
        if_18_8_93_5_code:
            mov rdi, 1
            exit_18_17_93_5:
                mov rax, 60
                syscall
            exit_18_17_93_5_end:
        if_18_5_93_5_end:
    assert_93_5_end:
    mov qword [rsp - 56], 1
    baz_96_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_96_13_end:
    cmp_97_12:
    cmp qword [rsp - 64], 2
    jne bool_false_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
    assert_97_5:
        if_18_8_97_5:
        cmp_18_8_97_5:
        cmp r15, false
        jne if_18_5_97_5_end
        if_18_8_97_5_code:
            mov rdi, 1
            exit_18_17_97_5:
                mov rax, 60
                syscall
            exit_18_17_97_5_end:
        if_18_5_97_5_end:
    assert_97_5_end:
    baz_99_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_99_9_end:
    cmp_100_12:
    cmp qword [rsp - 64], 2
    jne bool_false_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
    assert_100_5:
        if_18_8_100_5:
        cmp_18_8_100_5:
        cmp r15, false
        jne if_18_5_100_5_end
        if_18_8_100_5_code:
            mov rdi, 1
            exit_18_17_100_5:
                mov rax, 60
                syscall
            exit_18_17_100_5_end:
        if_18_5_100_5_end:
    assert_100_5_end:
    baz_102_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_102_23_end:
    mov qword [rsp - 72], 0
    cmp_103_12:
    cmp qword [rsp - 80], 4
    jne bool_false_103_12
    bool_true_103_12:
    mov r15, true
    jmp bool_end_103_12
    bool_false_103_12:
    mov r15, false
    bool_end_103_12:
    assert_103_5:
        if_18_8_103_5:
        cmp_18_8_103_5:
        cmp r15, false
        jne if_18_5_103_5_end
        if_18_8_103_5_code:
            mov rdi, 1
            exit_18_17_103_5:
                mov rax, 60
                syscall
            exit_18_17_103_5_end:
        if_18_5_103_5_end:
    assert_103_5_end:
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
    cmp_109_12:
    cmp qword [rsp - 116], 10
    jne bool_false_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
    assert_109_5:
        if_18_8_109_5:
        cmp_18_8_109_5:
        cmp r15, false
        jne if_18_5_109_5_end
        if_18_8_109_5_code:
            mov rdi, 1
            exit_18_17_109_5:
                mov rax, 60
                syscall
            exit_18_17_109_5_end:
        if_18_5_109_5_end:
    assert_109_5_end:
    cmp_110_12:
    cmp qword [rsp - 108], 2
    jne bool_false_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_18_8_110_5:
        cmp_18_8_110_5:
        cmp r15, false
        jne if_18_5_110_5_end
        if_18_8_110_5_code:
            mov rdi, 1
            exit_18_17_110_5:
                mov rax, 60
                syscall
            exit_18_17_110_5_end:
        if_18_5_110_5_end:
    assert_110_5_end:
    cmp_111_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_18_8_111_5:
        cmp_18_8_111_5:
        cmp r15, false
        jne if_18_5_111_5_end
        if_18_8_111_5_code:
            mov rdi, 1
            exit_18_17_111_5:
                mov rax, 60
                syscall
            exit_18_17_111_5_end:
        if_18_5_111_5_end:
    assert_111_5_end:
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
    cmp_115_12:
    cmp qword [rsp - 116], -1
    jne bool_false_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_18_8_115_5:
        cmp_18_8_115_5:
        cmp r15, false
        jne if_18_5_115_5_end
        if_18_8_115_5_code:
            mov rdi, 1
            exit_18_17_115_5:
                mov rax, 60
                syscall
            exit_18_17_115_5_end:
        if_18_5_115_5_end:
    assert_115_5_end:
    cmp_116_12:
    cmp qword [rsp - 108], -2
    jne bool_false_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
    assert_116_5:
        if_18_8_116_5:
        cmp_18_8_116_5:
        cmp r15, false
        jne if_18_5_116_5_end
        if_18_8_116_5_code:
            mov rdi, 1
            exit_18_17_116_5:
                mov rax, 60
                syscall
            exit_18_17_116_5_end:
        if_18_5_116_5_end:
    assert_116_5_end:
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_119_12:
    cmp qword [rsp - 152], -1
    jne bool_false_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
    assert_119_5:
        if_18_8_119_5:
        cmp_18_8_119_5:
        cmp r15, false
        jne if_18_5_119_5_end
        if_18_8_119_5_code:
            mov rdi, 1
            exit_18_17_119_5:
                mov rax, 60
                syscall
            exit_18_17_119_5_end:
        if_18_5_119_5_end:
    assert_119_5_end:
    cmp_120_12:
    cmp qword [rsp - 144], -2
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_18_8_120_5:
        cmp_18_8_120_5:
        cmp r15, false
        jne if_18_5_120_5_end
        if_18_8_120_5_code:
            mov rdi, 1
            exit_18_17_120_5:
                mov rax, 60
                syscall
            exit_18_17_120_5_end:
        if_18_5_120_5_end:
    assert_120_5_end:
    cmp_121_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_18_8_121_5:
        cmp_18_8_121_5:
        cmp r15, false
        jne if_18_5_121_5_end
        if_18_8_121_5_code:
            mov rdi, 1
            exit_18_17_121_5:
                mov rax, 60
                syscall
            exit_18_17_121_5_end:
        if_18_5_121_5_end:
    assert_121_5_end:
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 164], 73
    cmp_126_12:
        lea r13, [rsp - 172]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_18_8_126_5:
        cmp_18_8_126_5:
        cmp r15, false
        jne if_18_5_126_5_end
        if_18_8_126_5_code:
            mov rdi, 1
            exit_18_17_126_5:
                mov rax, 60
                syscall
            exit_18_17_126_5_end:
        if_18_5_126_5_end:
    assert_126_5_end:
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_130_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_130_5_end:
    loop_131_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_132_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_132_9_end:
        mov rdx, 80
        lea rsi, [rsp - 252]
        read_133_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 260], rax
        read_133_19_end:
        sub qword [rsp - 260], 1
        if_136_12:
        cmp_136_12:
        cmp qword [rsp - 260], 0
        jne if_138_19
        if_136_12_code:
            jmp loop_131_5_end
        jmp if_136_9_end
        if_138_19:
        cmp_138_19:
        cmp qword [rsp - 260], 4
        jg if_else_136_9
        if_138_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_139_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_139_13_end:
            jmp loop_131_5
        jmp if_136_9_end
        if_else_136_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_142_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_142_13_end:
            mov rdx, qword [rsp - 260]
            lea rsi, [rsp - 252]
            print_143_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_143_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_144_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_144_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_145_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_145_13_end:
        if_136_9_end:
    jmp loop_131_5
    loop_131_5_end:
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
;[38:1] # user types are defined using keyword `type`
;[40:1] # default type is `i64` and does not need to be specified
;[45:1]  # function arguments are equivalent to mutable references
;[51:1] # default argument type is `i64`
;[61:1] # return target is specified as a variable, in this case `res`
main:
;    var arr: i32[4] @ dword [rsp - 16]
;    [67:5]  var arr : i32[4]
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
;    [68:5] # arrays are initialized to 0
;    var ix: i64 @ qword [rsp - 24]
;    [70:5] var ix = 1
;    [70:14] ix =1
;    [70:14] 1
;    [70:14] 1
;    [72:5] ix = 1
    mov qword [rsp - 24], 1
;    [72:8] arr[ix] = 2
;    allocate scratch register -> r15
;    [72:9] ix
;    [72:9] ix
;    [72:11] r15 = ix
    mov r15, qword [rsp - 24]
;    [72:15]  2
;    [72:15] 2
;    [73:5] dword [rsp + r15 * 4 - 16] = 2
    mov dword [rsp + r15 * 4 - 16], 2
;    free scratch register 'r15'
;    [73:8] arr[ix + 1] = arr[ix]
;    allocate scratch register -> r15
;    [73:9] ix + 1
;    [73:9] ix + 1
;    [73:12] r15 = ix
    mov r15, qword [rsp - 24]
;    [73:15] r15 + 1
    add r15, 1
;    [73:19]  arr[ix]
;    [73:19] arr[ix]
;    [73:22] dword [rsp + r15 * 4 - 16] = arr[ix]
;    [73:22] arr[ix]
;    allocate scratch register -> r14
;    [73:23] ix
;    [73:23] ix
;    [73:25] r14 = ix
    mov r14, qword [rsp - 24]
;    allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
;    free scratch register 'r13'
;    free scratch register 'r14'
;    free scratch register 'r15'
;    [75:5] assert(arr[1] == 2)
;    allocate scratch register -> r15
;    [75:12] arr[1] == 2
;    [75:12] ? arr[1] == 2
;    [75:12] ? arr[1] == 2
    cmp_75_12:
;    allocate scratch register -> r14
;        [75:12] arr[1]
;        [75:15] r14 = arr[1]
;        [75:15] arr[1]
;        allocate scratch register -> r13
;        [75:16] 1
;        [75:16] 1
;        [75:17] r13 = 1
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
;        free scratch register 'r13'
    cmp r14, 2
;    free scratch register 'r14'
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
        if_18_8_75_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_75_5:
        cmp r15, false
        jne if_18_5_75_5_end
        if_18_8_75_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_75_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_75_5_end:
;                free named register 'rdi'
        if_18_5_75_5_end:
    assert_75_5_end:
;        free scratch register 'r15'
;    [76:5] assert(arr[2] == 2)
;    allocate scratch register -> r15
;    [76:12] arr[2] == 2
;    [76:12] ? arr[2] == 2
;    [76:12] ? arr[2] == 2
    cmp_76_12:
;    allocate scratch register -> r14
;        [76:12] arr[2]
;        [76:15] r14 = arr[2]
;        [76:15] arr[2]
;        allocate scratch register -> r13
;        [76:16] 2
;        [76:16] 2
;        [76:17] r13 = 2
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
;        free scratch register 'r13'
    cmp r14, 2
;    free scratch register 'r14'
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
        if_18_8_76_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_76_5:
        cmp r15, false
        jne if_18_5_76_5_end
        if_18_8_76_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_76_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_76_5_end:
;                free named register 'rdi'
        if_18_5_76_5_end:
    assert_76_5_end:
;        free scratch register 'r15'
;    [78:8] ix = 3
;    [78:10]  3
;    [78:10] 3
;    [79:5] ix = 3
    mov qword [rsp - 24], 3
;    [79:8] arr[ix] = ~inv(arr[ix - 1])
;    allocate scratch register -> r15
;    [79:9] ix
;    [79:9] ix
;    [79:11] r15 = ix
    mov r15, qword [rsp - 24]
;    [79:15]  ~inv(arr[ix - 1])
;    [79:15] ~inv(arr[ix - 1])
;    [79:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;    [79:16] ~inv(arr[ix - 1])
;    allocate scratch register -> r14
;    [79:20] arr[ix - 1]
;    [79:20] arr[ix - 1]
;    [79:23] r14 = arr[ix - 1]
;    [79:23] arr[ix - 1]
;    allocate scratch register -> r13
;    [79:24] ix - 1
;    [79:24] ix - 1
;    [79:27] r13 = ix
    mov r13, qword [rsp - 24]
;    [79:30] r13 - 1
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
;    free scratch register 'r13'
;    inv(i : i32) : i32 res 
    inv_79_16:
;        alias res -> dword [rsp + r15 * 4 - 16]
;        alias i -> r14
;        [58:9]  res = ~i
;        [58:11]  ~i
;        [58:11] ~i
;        [59:1] res = ~i
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_79_16_end:
;        free scratch register 'r14'
    not dword [rsp + r15 * 4 - 16]
;    free scratch register 'r15'
;    [80:5] assert(arr[ix] == 2)
;    allocate scratch register -> r15
;    [80:12] arr[ix] == 2
;    [80:12] ? arr[ix] == 2
;    [80:12] ? arr[ix] == 2
    cmp_80_12:
;    allocate scratch register -> r14
;        [80:12] arr[ix]
;        [80:15] r14 = arr[ix]
;        [80:15] arr[ix]
;        allocate scratch register -> r13
;        [80:16] ix
;        [80:16] ix
;        [80:18] r13 = ix
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
;        free scratch register 'r13'
    cmp r14, 2
;    free scratch register 'r14'
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
        if_18_8_80_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_80_5:
        cmp r15, false
        jne if_18_5_80_5_end
        if_18_8_80_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_80_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_80_5_end:
;                free named register 'rdi'
        if_18_5_80_5_end:
    assert_80_5_end:
;        free scratch register 'r15'
;    var p: point @ qword [rsp - 40]
;    [82:5] var p : point = {0, 0}
;    [82:21] p ={0, 0}
;    [82:21] {0, 0}
;    [82:21] {0, 0}
;    [82:22] 0
;    [82:22] 0
;    [82:23] p.x = 0
    mov qword [rsp - 40], 0
;    [82:25]  0
;    [82:25] 0
;    [82:26] p.y = 0
    mov qword [rsp - 32], 0
;    [83:5]  foo(p)
;    foo(pt : point) 
    foo_83_5:
;        alias pt -> p
;        [47:7]  pt.x = 0b10
;        [47:12]  0b10
;        [47:12] 0b10
;        [47:20] pt.x = 0b10
        mov qword [rsp - 40], 0b10
;        [47:20] # binary value 2
;        [48:7] pt.y = 0xb
;        [48:12]  0xb
;        [48:12] 0xb
;        [48:20] pt.y = 0xb
        mov qword [rsp - 32], 0xb
;        [48:20] # hex value 11
    foo_83_5_end:
;    [84:5] assert(p.x == 2)
;    allocate scratch register -> r15
;    [84:12] p.x == 2
;    [84:12] ? p.x == 2
;    [84:12] ? p.x == 2
    cmp_84_12:
    cmp qword [rsp - 40], 2
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
        if_18_8_84_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_84_5:
        cmp r15, false
        jne if_18_5_84_5_end
        if_18_8_84_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_84_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_84_5_end:
;                free named register 'rdi'
        if_18_5_84_5_end:
    assert_84_5_end:
;        free scratch register 'r15'
;    [85:5] assert(p.y == 0xb)
;    allocate scratch register -> r15
;    [85:12] p.y == 0xb
;    [85:12] ? p.y == 0xb
;    [85:12] ? p.y == 0xb
    cmp_85_12:
    cmp qword [rsp - 32], 0xb
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
        if_18_8_85_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_85_5:
        cmp r15, false
        jne if_18_5_85_5_end
        if_18_8_85_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_85_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_85_5_end:
;                free named register 'rdi'
        if_18_5_85_5_end:
    assert_85_5_end:
;        free scratch register 'r15'
;    var i: i64 @ qword [rsp - 48]
;    [87:5] var i = 0
;    [87:13] i =0
;    [87:13] 0
;    [87:13] 0
;    [88:5] i = 0
    mov qword [rsp - 48], 0
;    [88:5] bar(i)
;    bar(arg) 
    bar_88_5:
;        alias arg -> i
        if_53_8_88_5:
;        [53:8] ? arg == 0
;        [53:8] ? arg == 0
        cmp_53_8_88_5:
        cmp qword [rsp - 48], 0
        jne if_53_5_88_5_end
        if_53_8_88_5_code:
;            [53:17] return
            jmp bar_88_5_end
        if_53_5_88_5_end:
;        [54:9] arg = 0xff
;        [54:11]  0xff
;        [54:11] 0xff
;        [55:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_88_5_end:
;    [89:5] assert(i == 0)
;    allocate scratch register -> r15
;    [89:12] i == 0
;    [89:12] ? i == 0
;    [89:12] ? i == 0
    cmp_89_12:
    cmp qword [rsp - 48], 0
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
        if_18_8_89_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_89_5:
        cmp r15, false
        jne if_18_5_89_5_end
        if_18_8_89_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_89_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_89_5_end:
;                free named register 'rdi'
        if_18_5_89_5_end:
    assert_89_5_end:
;        free scratch register 'r15'
;    [91:7] i = 1
;    [91:9]  1
;    [91:9] 1
;    [92:5] i = 1
    mov qword [rsp - 48], 1
;    [92:5] bar(i)
;    bar(arg) 
    bar_92_5:
;        alias arg -> i
        if_53_8_92_5:
;        [53:8] ? arg == 0
;        [53:8] ? arg == 0
        cmp_53_8_92_5:
        cmp qword [rsp - 48], 0
        jne if_53_5_92_5_end
        if_53_8_92_5_code:
;            [53:17] return
            jmp bar_92_5_end
        if_53_5_92_5_end:
;        [54:9] arg = 0xff
;        [54:11]  0xff
;        [54:11] 0xff
;        [55:1] arg = 0xff
        mov qword [rsp - 48], 0xff
    bar_92_5_end:
;    [93:5] assert(i == 0xff)
;    allocate scratch register -> r15
;    [93:12] i == 0xff
;    [93:12] ? i == 0xff
;    [93:12] ? i == 0xff
    cmp_93_12:
    cmp qword [rsp - 48], 0xff
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
        if_18_8_93_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_93_5:
        cmp r15, false
        jne if_18_5_93_5_end
        if_18_8_93_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_93_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_93_5_end:
;                free named register 'rdi'
        if_18_5_93_5_end:
    assert_93_5_end:
;        free scratch register 'r15'
;    var j: i64 @ qword [rsp - 56]
;    [95:5] var j = 1
;    [95:13] j =1
;    [95:13] 1
;    [95:13] 1
;    [96:5] j = 1
    mov qword [rsp - 56], 1
;    var k: i64 @ qword [rsp - 64]
;    [96:5] var k = baz(j)
;    [96:13] k =baz(j)
;    [96:13] baz(j)
;    [96:13] baz(j)
;    [96:13] k = baz(j)
;    [96:13] baz(j)
;    baz(arg) : i64 res 
    baz_96_13:
;        alias res -> k
;        alias arg -> j
;        [63:9]  res = arg * 2
;        [63:11]  arg * 2
;        [63:11] arg * 2
;        [63:15] res = arg
;        allocate scratch register -> r15
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
;        free scratch register 'r15'
;        [64:1] res * 2
;        allocate scratch register -> r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;        free scratch register 'r15'
    baz_96_13_end:
;    [97:5] assert(k == 2)
;    allocate scratch register -> r15
;    [97:12] k == 2
;    [97:12] ? k == 2
;    [97:12] ? k == 2
    cmp_97_12:
    cmp qword [rsp - 64], 2
    jne bool_false_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
;    assert(expr : bool) 
    assert_97_5:
;        alias expr -> r15
        if_18_8_97_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_97_5:
        cmp r15, false
        jne if_18_5_97_5_end
        if_18_8_97_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_97_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_97_5_end:
;                free named register 'rdi'
        if_18_5_97_5_end:
    assert_97_5_end:
;        free scratch register 'r15'
;    [99:7] k = baz(1)
;    [99:9]  baz(1)
;    [99:9] baz(1)
;    [99:9] k = baz(1)
;    [99:9] baz(1)
;    baz(arg) : i64 res 
    baz_99_9:
;        alias res -> k
;        alias arg -> 1
;        [63:9]  res = arg * 2
;        [63:11]  arg * 2
;        [63:11] arg * 2
;        [63:15] res = arg
        mov qword [rsp - 64], 1
;        [64:1] res * 2
;        allocate scratch register -> r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
;        free scratch register 'r15'
    baz_99_9_end:
;    [100:5] assert(k == 2)
;    allocate scratch register -> r15
;    [100:12] k == 2
;    [100:12] ? k == 2
;    [100:12] ? k == 2
    cmp_100_12:
    cmp qword [rsp - 64], 2
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
        if_18_8_100_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_100_5:
        cmp r15, false
        jne if_18_5_100_5_end
        if_18_8_100_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_100_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_100_5_end:
;                free named register 'rdi'
        if_18_5_100_5_end:
    assert_100_5_end:
;        free scratch register 'r15'
;    var p0: point @ qword [rsp - 80]
;    [102:5] var p0 : point = {baz(2), 0}
;    [102:22] p0 ={baz(2), 0}
;    [102:22] {baz(2), 0}
;    [102:22] {baz(2), 0}
;    [102:23] baz(2)
;    [102:23] baz(2)
;    [102:23] p0.x = baz(2)
;    [102:23] baz(2)
;    baz(arg) : i64 res 
    baz_102_23:
;        alias res -> p0.x
;        alias arg -> 2
;        [63:9]  res = arg * 2
;        [63:11]  arg * 2
;        [63:11] arg * 2
;        [63:15] res = arg
        mov qword [rsp - 80], 2
;        [64:1] res * 2
;        allocate scratch register -> r15
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
;        free scratch register 'r15'
    baz_102_23_end:
;    [102:31]  0
;    [102:31] 0
;    [102:32] p0.y = 0
    mov qword [rsp - 72], 0
;    [103:5]  assert(p0.x == 4)
;    allocate scratch register -> r15
;    [103:12] p0.x == 4
;    [103:12] ? p0.x == 4
;    [103:12] ? p0.x == 4
    cmp_103_12:
    cmp qword [rsp - 80], 4
    jne bool_false_103_12
    bool_true_103_12:
    mov r15, true
    jmp bool_end_103_12
    bool_false_103_12:
    mov r15, false
    bool_end_103_12:
;    assert(expr : bool) 
    assert_103_5:
;        alias expr -> r15
        if_18_8_103_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_103_5:
        cmp r15, false
        jne if_18_5_103_5_end
        if_18_8_103_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_103_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_103_5_end:
;                free named register 'rdi'
        if_18_5_103_5_end:
    assert_103_5_end:
;        free scratch register 'r15'
;    var x: i64 @ qword [rsp - 88]
;    [105:5] var x = 1
;    [105:13] x =1
;    [105:13] 1
;    [105:13] 1
;    [106:5] x = 1
    mov qword [rsp - 88], 1
;    var y: i64 @ qword [rsp - 96]
;    [106:5] var y = 2
;    [106:13] y =2
;    [106:13] 2
;    [106:13] 2
;    [108:5] y = 2
    mov qword [rsp - 96], 2
;    var o1: object @ qword [rsp - 116]
;    [108:5] var o1 : object = {{x * 10, y}, 0xff0000}
;    [108:23] o1 ={{x * 10, y}, 0xff0000}
;    [108:23] {{x * 10, y}, 0xff0000}
;    [108:23] {{x * 10, y}, 0xff0000}
;        [108:24] {x * 10, y}
;        [108:25] x * 10
;        [108:25] x * 10
;        [108:27] o1.pos.x = x
;        allocate scratch register -> r15
        mov r15, qword [rsp - 88]
        mov qword [rsp - 116], r15
;        free scratch register 'r15'
;        [108:31] o1.pos.x * 10
;        allocate scratch register -> r15
        mov r15, qword [rsp - 116]
        imul r15, 10
        mov qword [rsp - 116], r15
;        free scratch register 'r15'
;        [108:33]  y
;        [108:33] y
;        [108:34] o1.pos.y = y
;        allocate scratch register -> r15
        mov r15, qword [rsp - 96]
        mov qword [rsp - 108], r15
;        free scratch register 'r15'
;    [108:37]  0xff0000
;    [108:37] 0xff0000
;    [108:45] o1.color = 0xff0000
    mov dword [rsp - 100], 0xff0000
;    [109:5]  assert(o1.pos.x == 10)
;    allocate scratch register -> r15
;    [109:12] o1.pos.x == 10
;    [109:12] ? o1.pos.x == 10
;    [109:12] ? o1.pos.x == 10
    cmp_109_12:
    cmp qword [rsp - 116], 10
    jne bool_false_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
;    assert(expr : bool) 
    assert_109_5:
;        alias expr -> r15
        if_18_8_109_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_109_5:
        cmp r15, false
        jne if_18_5_109_5_end
        if_18_8_109_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_109_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_109_5_end:
;                free named register 'rdi'
        if_18_5_109_5_end:
    assert_109_5_end:
;        free scratch register 'r15'
;    [110:5] assert(o1.pos.y == 2)
;    allocate scratch register -> r15
;    [110:12] o1.pos.y == 2
;    [110:12] ? o1.pos.y == 2
;    [110:12] ? o1.pos.y == 2
    cmp_110_12:
    cmp qword [rsp - 108], 2
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
        if_18_8_110_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_110_5:
        cmp r15, false
        jne if_18_5_110_5_end
        if_18_8_110_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_110_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_110_5_end:
;                free named register 'rdi'
        if_18_5_110_5_end:
    assert_110_5_end:
;        free scratch register 'r15'
;    [111:5] assert(o1.color == 0xff0000)
;    allocate scratch register -> r15
;    [111:12] o1.color == 0xff0000
;    [111:12] ? o1.color == 0xff0000
;    [111:12] ? o1.color == 0xff0000
    cmp_111_12:
    cmp dword [rsp - 100], 0xff0000
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
        if_18_8_111_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_111_5:
        cmp r15, false
        jne if_18_5_111_5_end
        if_18_8_111_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_111_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_111_5_end:
;                free named register 'rdi'
        if_18_5_111_5_end:
    assert_111_5_end:
;        free scratch register 'r15'
;    var p1: point @ qword [rsp - 132]
;    [113:5] var p1 : point = {-x, -y}
;    [113:22] p1 ={-x, -y}
;    [113:22] {-x, -y}
;    [113:22] {-x, -y}
;    [113:23] -x
;    [113:23] -x
;    [113:25] p1.x = -x
;    allocate scratch register -> r15
    mov r15, qword [rsp - 88]
    mov qword [rsp - 132], r15
;    free scratch register 'r15'
    neg qword [rsp - 132]
;    [113:27]  -y
;    [113:27] -y
;    [113:29] p1.y = -y
;    allocate scratch register -> r15
    mov r15, qword [rsp - 96]
    mov qword [rsp - 124], r15
;    free scratch register 'r15'
    neg qword [rsp - 124]
;    [114:7]  o1.pos = p1
;    allocate named register 'rsi'
;    allocate named register 'rdi'
;    allocate named register 'rcx'
    lea rdi, [rsp - 116]
;    [114:14]  p1
    lea rsi, [rsp - 132]
    mov rcx, 16
    rep movsb
;    free named register 'rcx'
;    free named register 'rdi'
;    free named register 'rsi'
;    [115:5] assert(o1.pos.x == -1)
;    allocate scratch register -> r15
;    [115:12] o1.pos.x == -1
;    [115:12] ? o1.pos.x == -1
;    [115:12] ? o1.pos.x == -1
    cmp_115_12:
    cmp qword [rsp - 116], -1
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
        if_18_8_115_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_115_5:
        cmp r15, false
        jne if_18_5_115_5_end
        if_18_8_115_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_115_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_115_5_end:
;                free named register 'rdi'
        if_18_5_115_5_end:
    assert_115_5_end:
;        free scratch register 'r15'
;    [116:5] assert(o1.pos.y == -2)
;    allocate scratch register -> r15
;    [116:12] o1.pos.y == -2
;    [116:12] ? o1.pos.y == -2
;    [116:12] ? o1.pos.y == -2
    cmp_116_12:
    cmp qword [rsp - 108], -2
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
        if_18_8_116_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_116_5:
        cmp r15, false
        jne if_18_5_116_5_end
        if_18_8_116_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_116_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_116_5_end:
;                free named register 'rdi'
        if_18_5_116_5_end:
    assert_116_5_end:
;        free scratch register 'r15'
;    var o2: object @ qword [rsp - 152]
;    [118:5] var o2 : object = o1
;    [118:23] o2 =o1
;    allocate named register 'rsi'
;    allocate named register 'rdi'
;    allocate named register 'rcx'
    lea rdi, [rsp - 152]
;    [118:23] o1
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
;    free named register 'rcx'
;    free named register 'rdi'
;    free named register 'rsi'
;    [119:5] assert(o2.pos.x == -1)
;    allocate scratch register -> r15
;    [119:12] o2.pos.x == -1
;    [119:12] ? o2.pos.x == -1
;    [119:12] ? o2.pos.x == -1
    cmp_119_12:
    cmp qword [rsp - 152], -1
    jne bool_false_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
;    assert(expr : bool) 
    assert_119_5:
;        alias expr -> r15
        if_18_8_119_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_119_5:
        cmp r15, false
        jne if_18_5_119_5_end
        if_18_8_119_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_119_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_119_5_end:
;                free named register 'rdi'
        if_18_5_119_5_end:
    assert_119_5_end:
;        free scratch register 'r15'
;    [120:5] assert(o2.pos.y == -2)
;    allocate scratch register -> r15
;    [120:12] o2.pos.y == -2
;    [120:12] ? o2.pos.y == -2
;    [120:12] ? o2.pos.y == -2
    cmp_120_12:
    cmp qword [rsp - 144], -2
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
;    assert(expr : bool) 
    assert_120_5:
;        alias expr -> r15
        if_18_8_120_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_120_5:
        cmp r15, false
        jne if_18_5_120_5_end
        if_18_8_120_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_120_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_120_5_end:
;                free named register 'rdi'
        if_18_5_120_5_end:
    assert_120_5_end:
;        free scratch register 'r15'
;    [121:5] assert(o2.color == 0xff0000)
;    allocate scratch register -> r15
;    [121:12] o2.color == 0xff0000
;    [121:12] ? o2.color == 0xff0000
;    [121:12] ? o2.color == 0xff0000
    cmp_121_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
;    assert(expr : bool) 
    assert_121_5:
;        alias expr -> r15
        if_18_8_121_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_121_5:
        cmp r15, false
        jne if_18_5_121_5_end
        if_18_8_121_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_121_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_121_5_end:
;                free named register 'rdi'
        if_18_5_121_5_end:
    assert_121_5_end:
;        free scratch register 'r15'
;    var o3: object[1] @ qword [rsp - 172]
;    [123:5] var o3 : object[1]
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
;    [124:5] # index 0 in an array can be accessed without array index
;    [125:7] o3.pos.y = 73
;    [125:16]  73
;    [125:16] 73
;    [126:5] o3.pos.y = 73
    mov qword [rsp - 164], 73
;    [126:5] assert(o3[0].pos.y == 73)
;    allocate scratch register -> r15
;    [126:12] o3[0].pos.y == 73
;    [126:12] ? o3[0].pos.y == 73
;    [126:12] ? o3[0].pos.y == 73
    cmp_126_12:
;    allocate scratch register -> r14
;        [126:12] o3[0].pos.y
;        [126:14] r14 = o3[0].pos.y
;        [126:14] o3[0].pos.y
;        allocate scratch register -> r13
        lea r13, [rsp - 172]
;        allocate scratch register -> r12
;        [126:15] 0
;        [126:15] 0
;        [126:16] r12 = 0
        mov r12, 0
        imul r12, 20
        add r13, r12
;        free scratch register 'r12'
        add r13, 8
        mov r14, qword [r13]
;        free scratch register 'r13'
    cmp r14, 73
;    free scratch register 'r14'
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
;    assert(expr : bool) 
    assert_126_5:
;        alias expr -> r15
        if_18_8_126_5:
;        [18:8] ? not expr
;        [18:8] ? not expr
        cmp_18_8_126_5:
        cmp r15, false
        jne if_18_5_126_5_end
        if_18_8_126_5_code:
;            [18:17] exit(1)
;            allocate named register 'rdi'
            mov rdi, 1
;            exit(v : reg_rdi) 
            exit_18_17_126_5:
;                alias v -> rdi
;                [12:5]  mov(rax, 60)
                mov rax, 60
;                [12:19] # exit system call
;                [13:5] mov(rdi, v)
;                [13:19] # return code
;                [14:5] syscall()
                syscall
            exit_18_17_126_5_end:
;                free named register 'rdi'
        if_18_5_126_5_end:
    assert_126_5_end:
;        free scratch register 'r15'
;    var input_buffer: i8[80] @ byte [rsp - 252]
;    [128:5] var input_buffer : i8[80]
;    clear array 80 * 1 B = 80 B
;    allocate named register 'rdi'
;    allocate named register 'rcx'
;    allocate named register 'rax'
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
;    free named register 'rax'
;    free named register 'rcx'
;    free named register 'rdi'
;    [130:5] print(hello.len, hello)
;    allocate named register 'rdx'
    mov rdx, hello.len
;    allocate named register 'rsi'
    mov rsi, hello
;    print(len : reg_rdx, ptr : reg_rsi) 
    print_130_5:
;        alias len -> rdx
;        alias ptr -> rsi
;        [22:5]  mov(rax, 1)
        mov rax, 1
;        [22:19] # write system call
;        [23:5] mov(rdi, 1)
        mov rdi, 1
;        [23:19] # file descriptor for standard out
;        [24:5] mov(rsi, ptr)
;        [24:19] # buffer address
;        [25:5] mov(rdx, len)
;        [25:19] # buffer size
;        [26:5] syscall()
        syscall
    print_130_5_end:
;        free named register 'rsi'
;        free named register 'rdx'
;    [131:5] loop
    loop_131_5:
;        [132:9]  print(prompt1.len, prompt1)
;        allocate named register 'rdx'
        mov rdx, prompt1.len
;        allocate named register 'rsi'
        mov rsi, prompt1
;        print(len : reg_rdx, ptr : reg_rsi) 
        print_132_9:
;            alias len -> rdx
;            alias ptr -> rsi
;            [22:5]  mov(rax, 1)
            mov rax, 1
;            [22:19] # write system call
;            [23:5] mov(rdi, 1)
            mov rdi, 1
;            [23:19] # file descriptor for standard out
;            [24:5] mov(rsi, ptr)
;            [24:19] # buffer address
;            [25:5] mov(rdx, len)
;            [25:19] # buffer size
;            [26:5] syscall()
            syscall
        print_132_9_end:
;            free named register 'rsi'
;            free named register 'rdx'
;        var len: i64 @ qword [rsp - 260]
;        [133:9] var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;        [133:19] len =read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;        [133:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;        [133:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;        [133:19] len = read(array_size_of(input_buffer), address_of(input_buffer))
;        [133:19] read(array_size_of(input_buffer), address_of(input_buffer))
;        allocate named register 'rdx'
;        [133:24] array_size_of(input_buffer)
;        [133:24] array_size_of(input_buffer)
;        [133:24] rdx = array_size_of(input_buffer)
;        [133:24] array_size_of(input_buffer)
        mov rdx, 80
;        allocate named register 'rsi'
;        [133:53]  address_of(input_buffer)
;        [133:53] address_of(input_buffer)
;        [133:53] rsi = address_of(input_buffer)
;        [133:53] address_of(input_buffer)
        lea rsi, [rsp - 252]
;        read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_133_19:
;            alias nbytes -> len
;            alias len -> rdx
;            alias ptr -> rsi
;            [30:5]  mov(rax, 0)
            mov rax, 0
;            [30:19] # read system call
;            [31:5] mov(rdi, 0)
            mov rdi, 0
;            [31:19] # file descriptor for standard input
;            [32:5] mov(rsi, ptr)
;            [32:19] # buffer address
;            [33:5] mov(rdx, len)
;            [33:19] # buffer size
;            [34:5] syscall()
            syscall
;            [35:5] mov(nbytes, rax)
            mov qword [rsp - 260], rax
;            [35:22] # return value
        read_133_19_end:
;            free named register 'rsi'
;            free named register 'rdx'
;        [134:9] len - 1
        sub qword [rsp - 260], 1
;        [134:9] # note: array_size_of and address_of are built-in functions
;        [135:9] # -1 to not include the trailing '\n'
        if_136_12:
;        [136:12] ? len == 0
;        [136:12] ? len == 0
        cmp_136_12:
        cmp qword [rsp - 260], 0
        jne if_138_19
        if_136_12_code:
;            [137:13]  break
            jmp loop_131_5_end
        jmp if_136_9_end
        if_138_19:
;        [138:19] ? len <= 4
;        [138:19] ? len <= 4
        cmp_138_19:
        cmp qword [rsp - 260], 4
        jg if_else_136_9
        if_138_19_code:
;            [139:13]  print(prompt2.len, prompt2)
;            allocate named register 'rdx'
            mov rdx, prompt2.len
;            allocate named register 'rsi'
            mov rsi, prompt2
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_139_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [22:5]  mov(rax, 1)
                mov rax, 1
;                [22:19] # write system call
;                [23:5] mov(rdi, 1)
                mov rdi, 1
;                [23:19] # file descriptor for standard out
;                [24:5] mov(rsi, ptr)
;                [24:19] # buffer address
;                [25:5] mov(rdx, len)
;                [25:19] # buffer size
;                [26:5] syscall()
                syscall
            print_139_13_end:
;                free named register 'rsi'
;                free named register 'rdx'
;            [140:13] continue
            jmp loop_131_5
        jmp if_136_9_end
        if_else_136_9:
;            [142:13]  print(prompt3.len, prompt3)
;            allocate named register 'rdx'
            mov rdx, prompt3.len
;            allocate named register 'rsi'
            mov rsi, prompt3
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_142_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [22:5]  mov(rax, 1)
                mov rax, 1
;                [22:19] # write system call
;                [23:5] mov(rdi, 1)
                mov rdi, 1
;                [23:19] # file descriptor for standard out
;                [24:5] mov(rsi, ptr)
;                [24:19] # buffer address
;                [25:5] mov(rdx, len)
;                [25:19] # buffer size
;                [26:5] syscall()
                syscall
            print_142_13_end:
;                free named register 'rsi'
;                free named register 'rdx'
;            [143:13] print(len, address_of(input_buffer))
;            allocate named register 'rdx'
            mov rdx, qword [rsp - 260]
;            allocate named register 'rsi'
;            [143:24]  address_of(input_buffer)
;            [143:24] address_of(input_buffer)
;            [143:24] rsi = address_of(input_buffer)
;            [143:24] address_of(input_buffer)
            lea rsi, [rsp - 252]
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_143_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [22:5]  mov(rax, 1)
                mov rax, 1
;                [22:19] # write system call
;                [23:5] mov(rdi, 1)
                mov rdi, 1
;                [23:19] # file descriptor for standard out
;                [24:5] mov(rsi, ptr)
;                [24:19] # buffer address
;                [25:5] mov(rdx, len)
;                [25:19] # buffer size
;                [26:5] syscall()
                syscall
            print_143_13_end:
;                free named register 'rsi'
;                free named register 'rdx'
;            [144:13] print(dot.len, dot)
;            allocate named register 'rdx'
            mov rdx, dot.len
;            allocate named register 'rsi'
            mov rsi, dot
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_144_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [22:5]  mov(rax, 1)
                mov rax, 1
;                [22:19] # write system call
;                [23:5] mov(rdi, 1)
                mov rdi, 1
;                [23:19] # file descriptor for standard out
;                [24:5] mov(rsi, ptr)
;                [24:19] # buffer address
;                [25:5] mov(rdx, len)
;                [25:19] # buffer size
;                [26:5] syscall()
                syscall
            print_144_13_end:
;                free named register 'rsi'
;                free named register 'rdx'
;            [145:13] print(nl.len, nl)
;            allocate named register 'rdx'
            mov rdx, nl.len
;            allocate named register 'rsi'
            mov rsi, nl
;            print(len : reg_rdx, ptr : reg_rsi) 
            print_145_13:
;                alias len -> rdx
;                alias ptr -> rsi
;                [22:5]  mov(rax, 1)
                mov rax, 1
;                [22:19] # write system call
;                [23:5] mov(rdi, 1)
                mov rdi, 1
;                [23:19] # file descriptor for standard out
;                [24:5] mov(rsi, ptr)
;                [24:19] # buffer address
;                [25:5] mov(rdx, len)
;                [25:19] # buffer size
;                [26:5] syscall()
                syscall
            print_145_13_end:
;                free named register 'rsi'
;                free named register 'rdx'
        if_136_9_end:
    jmp loop_131_5
    loop_131_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; max scratch registers in use: 4
;            max frames in use: 7
;               max stack size: 260 B
```
