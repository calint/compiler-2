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
