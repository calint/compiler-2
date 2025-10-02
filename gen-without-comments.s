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
