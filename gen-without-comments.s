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
    lea rdi, [rsp-16]
    mov rcx, 16
    xor rax, rax
    rep stosb
    mov r15, 1
    lea r15, [rsp+r15*4-16]
    mov dword[r15], 2
    mov r15, 2
    lea r15, [rsp+r15*4-16]
    mov r14, 1
    lea r14, [rsp+r14*4-16]
    mov r13d, dword[r14]
    mov dword[r15], r13d
    cmp_74_12:
        mov r13, 1
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
    jne bool_false_74_12
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
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
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
    mov qword[rsp-24], 3
    mov r15, qword[rsp-24]
    lea r15, [rsp+r15*4-16]
    mov r13, qword[rsp-24]
    sub r13, 1
    lea r13, [rsp+r13*4-16]
    movsx r14, dword[r13]
    inv_78_16:
        mov dword[r15], r14d
        not dword[r15]
    inv_78_16_end:
    not dword[r15]
    cmp_79_12:
        mov r13, qword[rsp-24]
        lea r13, [rsp+r13*4-16]
        movsx r14, dword[r13]
    cmp r14, 2
    jne bool_false_79_12
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
        if_36_8_79_5_code:
            mov rdi, 1
            exit_36_17_79_5:
                mov rax, 60
                syscall
            exit_36_17_79_5_end:
        if_36_5_79_5_end:
    assert_79_5_end:
    mov qword[rsp-40], 0
    mov qword[rsp-32], 0
    foo_82_5:
        mov qword[rsp-40], 0b10
        mov qword[rsp-32], 0xb
    foo_82_5_end:
    cmp_83_12:
    cmp qword[rsp-40], 2
    jne bool_false_83_12
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
        if_36_8_83_5_code:
            mov rdi, 1
            exit_36_17_83_5:
                mov rax, 60
                syscall
            exit_36_17_83_5_end:
        if_36_5_83_5_end:
    assert_83_5_end:
    cmp_84_12:
    cmp qword[rsp-32], 0xb
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
    mov qword[rsp-48], 0
    bar_87_5:
        if_54_8_87_5:
        cmp_54_8_87_5:
        cmp qword[rsp-48], 0
        jne if_54_5_87_5_end
        if_54_8_87_5_code:
            jmp bar_87_5_end
        if_54_5_87_5_end:
        mov qword[rsp-48], 0xff
    bar_87_5_end:
    cmp_88_12:
    cmp qword[rsp-48], 0
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
    mov qword[rsp-48], 1
    bar_91_5:
        if_54_8_91_5:
        cmp_54_8_91_5:
        cmp qword[rsp-48], 0
        jne if_54_5_91_5_end
        if_54_8_91_5_code:
            jmp bar_91_5_end
        if_54_5_91_5_end:
        mov qword[rsp-48], 0xff
    bar_91_5_end:
    cmp_92_12:
    cmp qword[rsp-48], 0xff
    jne bool_false_92_12
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
        if_36_8_92_5_code:
            mov rdi, 1
            exit_36_17_92_5:
                mov rax, 60
                syscall
            exit_36_17_92_5_end:
        if_36_5_92_5_end:
    assert_92_5_end:
    mov qword[rsp-56], 1
    baz_95_13:
        mov r15, qword[rsp-56]
        mov qword[rsp-64], r15
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
    baz_95_13_end:
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
    baz_98_9:
        mov qword[rsp-64], 1
        mov r15, qword[rsp-64]
        imul r15, 2
        mov qword[rsp-64], r15
    baz_98_9_end:
    cmp_99_12:
    cmp qword[rsp-64], 2
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
    baz_101_23:
        mov qword[rsp-80], 2
        mov r15, qword[rsp-80]
        imul r15, 2
        mov qword[rsp-80], r15
    baz_101_23_end:
    mov qword[rsp-72], 0
    cmp_102_12:
    cmp qword[rsp-80], 4
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
    cmp_108_12:
    cmp qword[rsp-116], 10
    jne bool_false_108_12
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
        if_36_8_108_5_code:
            mov rdi, 1
            exit_36_17_108_5:
                mov rax, 60
                syscall
            exit_36_17_108_5_end:
        if_36_5_108_5_end:
    assert_108_5_end:
    cmp_109_12:
    cmp qword[rsp-108], 2
    jne bool_false_109_12
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
        if_36_8_109_5_code:
            mov rdi, 1
            exit_36_17_109_5:
                mov rax, 60
                syscall
            exit_36_17_109_5_end:
        if_36_5_109_5_end:
    assert_109_5_end:
    cmp_110_12:
    cmp dword[rsp-100], 0xff0000
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
    cmp_114_12:
    cmp qword[rsp-116], -1
    jne bool_false_114_12
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
        if_36_8_114_5_code:
            mov rdi, 1
            exit_36_17_114_5:
                mov rax, 60
                syscall
            exit_36_17_114_5_end:
        if_36_5_114_5_end:
    assert_114_5_end:
    cmp_115_12:
    cmp qword[rsp-108], -2
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
        mov r15, qword[rsp-116]
        mov qword[rsp-152], r15
        mov r15, qword[rsp-108]
        mov qword[rsp-144], r15
    mov r15d, dword[rsp-100]
    mov dword[rsp-136], r15d
    cmp_118_12:
    cmp qword[rsp-152], -1
    jne bool_false_118_12
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
        if_36_8_118_5_code:
            mov rdi, 1
            exit_36_17_118_5:
                mov rax, 60
                syscall
            exit_36_17_118_5_end:
        if_36_5_118_5_end:
    assert_118_5_end:
    cmp_119_12:
    cmp qword[rsp-144], -2
    jne bool_false_119_12
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
        if_36_8_119_5_code:
            mov rdi, 1
            exit_36_17_119_5:
                mov rax, 60
                syscall
            exit_36_17_119_5_end:
        if_36_5_119_5_end:
    assert_119_5_end:
    cmp_120_12:
    cmp dword[rsp-136], 0xff0000
    jne bool_false_120_12
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
            mov qword[rsp-160], rax
        read_125_19_end:
        sub qword[rsp-160], 1
        if_126_12:
        cmp_126_12:
        cmp qword[rsp-160], 0
        jne if_128_19
        if_126_12_code:
            jmp loop_123_5_end
        jmp if_126_9_end
        if_128_19:
        cmp_128_19:
        cmp qword[rsp-160], 4
        jg if_else_126_9
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
            mov rdx, qword[rsp-160]
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
