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
