section .bss
stk resd 131072
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
    cmp r15, 4
    mov r14, 79
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    cmp r15, 4
    mov r14, 80
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    cmp r14, 4
    mov r13, 80
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_81_12:
        mov r13, 1
        cmp r13, 4
        mov r12, 81
        cmovge rbp, r12
        jge panic_bounds
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
    cmp_82_12:
        mov r13, 2
        cmp r13, 4
        mov r12, 82
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_82_12
    bool_true_82_12:
    mov r15, true
    jmp bool_end_82_12
    bool_false_82_12:
    mov r15, false
    bool_end_82_12:
    assert_82_5:
        if_19_8_82_5:
        cmp_19_8_82_5:
        cmp r15, false
        jne if_19_5_82_5_end
        if_19_8_82_5_code:
            mov rdi, 1
            exit_19_17_82_5:
                mov rax, 60
                syscall
            exit_19_17_82_5_end:
        if_19_5_82_5_end:
    assert_82_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, rcx
    add r14, r15
    cmp r14, 4
    mov r13, 84
    cmovg rbp, r13
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    cmp rcx, 4
    mov r15, 84
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_86_12:
        mov r13, 0
        cmp r13, 4
        mov r12, 86
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
    assert_86_5:
        if_19_8_86_5:
        cmp_19_8_86_5:
        cmp r15, false
        jne if_19_5_86_5_end
        if_19_8_86_5_code:
            mov rdi, 1
            exit_19_17_86_5:
                mov rax, 60
                syscall
            exit_19_17_86_5_end:
        if_19_5_86_5_end:
    assert_86_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    cmp rcx, 4
    mov r15, 89
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    cmp rcx, 8
    mov r15, 89
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_90_12:
        mov rcx, 4
        cmp rcx, 4
        mov r13, 90
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        mov r13, 90
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_90_12
        mov r14, false
        jmp cmps_end_90_12
        cmps_eq_90_12:
        mov r14, true
        cmps_end_90_12:
    cmp r14, false
    je bool_false_90_12
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
    mov r15, 2
    cmp r15, 8
    mov r14, 92
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_93_12:
        mov rcx, 4
        cmp rcx, 4
        mov r13, 93
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        mov r13, 93
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_93_16
        mov r14, false
        jmp cmps_end_93_16
        cmps_eq_93_16:
        mov r14, true
        cmps_end_93_16:
    cmp r14, false
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
    assert_93_5:
        if_19_8_93_5:
        cmp_19_8_93_5:
        cmp r15, false
        jne if_19_5_93_5_end
        if_19_8_93_5_code:
            mov rdi, 1
            exit_19_17_93_5:
                mov rax, 60
                syscall
            exit_19_17_93_5_end:
        if_19_5_93_5_end:
    assert_93_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    cmp r15, 4
    mov r14, 96
    cmovge rbp, r14
    jge panic_bounds
    mov r13, qword [rsp - 24]
    sub r13, 1
    cmp r13, 4
    mov r12, 96
    cmovge rbp, r12
    jge panic_bounds
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_96_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_96_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_97_12:
        mov r13, qword [rsp - 24]
        cmp r13, 4
        mov r12, 97
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
    assert_97_5:
        if_19_8_97_5:
        cmp_19_8_97_5:
        cmp r15, false
        jne if_19_5_97_5_end
        if_19_8_97_5_code:
            mov rdi, 1
            exit_19_17_97_5:
                mov rax, 60
                syscall
            exit_19_17_97_5_end:
        if_19_5_97_5_end:
    assert_97_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_100_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_100_5_end:
    cmp_101_12:
    cmp qword [rsp - 72], 2
    jne bool_false_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
    assert_101_5:
        if_19_8_101_5:
        cmp_19_8_101_5:
        cmp r15, false
        jne if_19_5_101_5_end
        if_19_8_101_5_code:
            mov rdi, 1
            exit_19_17_101_5:
                mov rax, 60
                syscall
            exit_19_17_101_5_end:
        if_19_5_101_5_end:
    assert_101_5_end:
    cmp_102_12:
    cmp qword [rsp - 64], 0xb
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
    mov qword [rsp - 80], 0
    bar_105_5:
        if_59_8_105_5:
        cmp_59_8_105_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_105_5_end
        if_59_8_105_5_code:
            jmp bar_105_5_end
        if_59_5_105_5_end:
        mov qword [rsp - 80], 0xff
    bar_105_5_end:
    cmp_106_12:
    cmp qword [rsp - 80], 0
    jne bool_false_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
    assert_106_5:
        if_19_8_106_5:
        cmp_19_8_106_5:
        cmp r15, false
        jne if_19_5_106_5_end
        if_19_8_106_5_code:
            mov rdi, 1
            exit_19_17_106_5:
                mov rax, 60
                syscall
            exit_19_17_106_5_end:
        if_19_5_106_5_end:
    assert_106_5_end:
    mov qword [rsp - 80], 1
    bar_109_5:
        if_59_8_109_5:
        cmp_59_8_109_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_109_5_end
        if_59_8_109_5_code:
            jmp bar_109_5_end
        if_59_5_109_5_end:
        mov qword [rsp - 80], 0xff
    bar_109_5_end:
    cmp_110_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_19_8_110_5:
        cmp_19_8_110_5:
        cmp r15, false
        jne if_19_5_110_5_end
        if_19_8_110_5_code:
            mov rdi, 1
            exit_19_17_110_5:
                mov rax, 60
                syscall
            exit_19_17_110_5_end:
        if_19_5_110_5_end:
    assert_110_5_end:
    mov qword [rsp - 88], 1
    baz_113_13:
        mov r15, qword [rsp - 88]
        mov qword [rsp - 96], r15
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
    baz_113_13_end:
    cmp_114_12:
    cmp qword [rsp - 96], 2
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
    baz_116_9:
        mov qword [rsp - 96], 1
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
    baz_116_9_end:
    cmp_117_12:
    cmp qword [rsp - 96], 2
    jne bool_false_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
    assert_117_5:
        if_19_8_117_5:
        cmp_19_8_117_5:
        cmp r15, false
        jne if_19_5_117_5_end
        if_19_8_117_5_code:
            mov rdi, 1
            exit_19_17_117_5:
                mov rax, 60
                syscall
            exit_19_17_117_5_end:
        if_19_5_117_5_end:
    assert_117_5_end:
    baz_119_23:
        mov qword [rsp - 112], 2
        mov r15, qword [rsp - 112]
        imul r15, 2
        mov qword [rsp - 112], r15
    baz_119_23_end:
    mov qword [rsp - 104], 0
    cmp_120_12:
    cmp qword [rsp - 112], 4
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
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 148]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_126_12:
    cmp qword [rsp - 148], 10
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
    cmp_127_12:
    cmp qword [rsp - 140], 2
    jne bool_false_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
    assert_127_5:
        if_19_8_127_5:
        cmp_19_8_127_5:
        cmp r15, false
        jne if_19_5_127_5_end
        if_19_8_127_5_code:
            mov rdi, 1
            exit_19_17_127_5:
                mov rax, 60
                syscall
            exit_19_17_127_5_end:
        if_19_5_127_5_end:
    assert_127_5_end:
    cmp_128_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_128_12
    bool_true_128_12:
    mov r15, true
    jmp bool_end_128_12
    bool_false_128_12:
    mov r15, false
    bool_end_128_12:
    assert_128_5:
        if_19_8_128_5:
        cmp_19_8_128_5:
        cmp r15, false
        jne if_19_5_128_5_end
        if_19_8_128_5_code:
            mov rdi, 1
            exit_19_17_128_5:
                mov rax, 60
                syscall
            exit_19_17_128_5_end:
        if_19_5_128_5_end:
    assert_128_5_end:
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
    neg qword [rsp - 164]
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
    neg qword [rsp - 156]
    lea rdi, [rsp - 148]
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
    cmp_132_12:
    cmp qword [rsp - 148], -1
    jne bool_false_132_12
    bool_true_132_12:
    mov r15, true
    jmp bool_end_132_12
    bool_false_132_12:
    mov r15, false
    bool_end_132_12:
    assert_132_5:
        if_19_8_132_5:
        cmp_19_8_132_5:
        cmp r15, false
        jne if_19_5_132_5_end
        if_19_8_132_5_code:
            mov rdi, 1
            exit_19_17_132_5:
                mov rax, 60
                syscall
            exit_19_17_132_5_end:
        if_19_5_132_5_end:
    assert_132_5_end:
    cmp_133_12:
    cmp qword [rsp - 140], -2
    jne bool_false_133_12
    bool_true_133_12:
    mov r15, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15, false
    bool_end_133_12:
    assert_133_5:
        if_19_8_133_5:
        cmp_19_8_133_5:
        cmp r15, false
        jne if_19_5_133_5_end
        if_19_8_133_5_code:
            mov rdi, 1
            exit_19_17_133_5:
                mov rax, 60
                syscall
            exit_19_17_133_5_end:
        if_19_5_133_5_end:
    assert_133_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_136_12:
    cmp qword [rsp - 184], -1
    jne bool_false_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_19_8_136_5:
        cmp_19_8_136_5:
        cmp r15, false
        jne if_19_5_136_5_end
        if_19_8_136_5_code:
            mov rdi, 1
            exit_19_17_136_5:
                mov rax, 60
                syscall
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
    assert_136_5_end:
    cmp_137_12:
    cmp qword [rsp - 176], -2
    jne bool_false_137_12
    bool_true_137_12:
    mov r15, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15, false
    bool_end_137_12:
    assert_137_5:
        if_19_8_137_5:
        cmp_19_8_137_5:
        cmp r15, false
        jne if_19_5_137_5_end
        if_19_8_137_5_code:
            mov rdi, 1
            exit_19_17_137_5:
                mov rax, 60
                syscall
            exit_19_17_137_5_end:
        if_19_5_137_5_end:
    assert_137_5_end:
    cmp_138_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_138_12
    bool_true_138_12:
    mov r15, true
    jmp bool_end_138_12
    bool_false_138_12:
    mov r15, false
    bool_end_138_12:
    assert_138_5:
        if_19_8_138_5:
        cmp_19_8_138_5:
        cmp r15, false
        jne if_19_5_138_5_end
        if_19_8_138_5_code:
            mov rdi, 1
            exit_19_17_138_5:
                mov rax, 60
                syscall
            exit_19_17_138_5_end:
        if_19_5_138_5_end:
    assert_138_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_143_12:
        lea r13, [rsp - 204]
        mov r12, 0
        cmp r12, 1
        mov r11, 143
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
    assert_143_5:
        if_19_8_143_5:
        cmp_19_8_143_5:
        cmp r15, false
        jne if_19_5_143_5_end
        if_19_8_143_5_code:
            mov rdi, 1
            exit_19_17_143_5:
                mov rax, 60
                syscall
            exit_19_17_143_5_end:
        if_19_5_143_5_end:
    assert_143_5_end:
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 8
    mov r13, 148
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    cmp r14, 8
    mov r13, 148
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8 + 0], 0xffee
    cmp_149_12:
        lea r13, [rsp - 796]
        mov r12, 1
        cmp r12, 8
        mov r11, 149
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 8
        mov r11, 149
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_149_12
    bool_true_149_12:
    mov r15, true
    jmp bool_end_149_12
    bool_false_149_12:
    mov r15, false
    bool_end_149_12:
    assert_149_5:
        if_19_8_149_5:
        cmp_19_8_149_5:
        cmp r15, false
        jne if_19_5_149_5_end
        if_19_8_149_5_code:
            mov rdi, 1
            exit_19_17_149_5:
                mov rax, 60
                syscall
            exit_19_17_149_5_end:
        if_19_5_149_5_end:
    assert_149_5_end:
    mov rcx, 8
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 8
    mov r13, 152
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 8
    mov r14, 152
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 796]
    mov r14, 0
    cmp r14, 8
    mov r13, 153
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 8
    mov r14, 153
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_156_12:
        lea r13, [rsp - 796]
        mov r12, 0
        cmp r12, 8
        mov r11, 156
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 8
        mov r11, 156
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_156_12
    bool_true_156_12:
    mov r15, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15, false
    bool_end_156_12:
    assert_156_5:
        if_19_8_156_5:
        cmp_19_8_156_5:
        cmp r15, false
        jne if_19_5_156_5_end
        if_19_8_156_5_code:
            mov rdi, 1
            exit_19_17_156_5:
                mov rax, 60
                syscall
            exit_19_17_156_5_end:
        if_19_5_156_5_end:
    assert_156_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_158_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_158_5_end:
    loop_159_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_160_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_160_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_162_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 804], rax
        read_162_19_end:
        sub qword [rsp - 804], 1
        if_166_12:
        cmp_166_12:
        cmp qword [rsp - 804], 0
        jne if_168_19
        if_166_12_code:
            jmp loop_159_5_end
        jmp if_166_9_end
        if_168_19:
        cmp_168_19:
        cmp qword [rsp - 804], 4
        jg if_else_166_9
        if_168_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_169_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_169_13_end:
            jmp loop_159_5
        jmp if_166_9_end
        if_else_166_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_172_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_172_13_end:
            mov rdx, qword [rsp - 804]
            lea rsi, [rsp - 284]
            print_173_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_173_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_174_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_174_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_175_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_175_13_end:
        if_166_9_end:
    jmp loop_159_5
    loop_159_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 1
    mov rdi, 2
    lea rsi, [rel msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    lea rdi, [rel num_buffer + 19]
    mov byte [rdi], 10
    dec rdi
.convert_loop:
    xor rdx, rdx
    mov rcx, 10
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jnz .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    lea rdx, [rel num_buffer + 20]
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
    msg_panic: db 'panic: bounds at line '
    msg_panic_len equ $ - msg_panic
section .bss
    num_buffer: resb 21
