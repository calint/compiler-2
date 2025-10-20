DEFAULT REL
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $ - hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $ - prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $ - prompt2
prompt3: db 'hello '
prompt3.len equ $ - prompt3
dot: db '.'
dot.len equ $ - dot
nl: db '', 10,''
nl.len equ $ - nl
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    mov qword [rsp - 16], 0
    mov qword [rsp - 8], 0
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    mov r14, 107
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, 108
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    mov r13, 108
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_109_12:
        mov r13, 1
        mov r12, 109
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_109_12
    jmp bool_true_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
    assert_109_5:
        if_20_29_109_5:
        cmp_20_29_109_5:
        test r15, r15
        jne if_20_26_109_5_end
        jmp if_20_29_109_5_code
        if_20_29_109_5_code:
            mov rdi, 1
            exit_20_38_109_5:
                    mov rax, 60
                syscall
            exit_20_38_109_5_end:
        if_20_26_109_5_end:
    assert_109_5_end:
    cmp_110_12:
        mov r13, 2
        mov r12, 110
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_20_29_110_5:
        cmp_20_29_110_5:
        test r15, r15
        jne if_20_26_110_5_end
        jmp if_20_29_110_5_code
        if_20_29_110_5_code:
            mov rdi, 1
            exit_20_38_110_5:
                    mov rax, 60
                syscall
            exit_20_38_110_5_end:
        if_20_26_110_5_end:
    assert_110_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 112
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    mov r15, 112
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_114_12:
        mov r13, 0
        mov r12, 114
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_114_12
    jmp bool_true_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
    assert_114_5:
        if_20_29_114_5:
        cmp_20_29_114_5:
        test r15, r15
        jne if_20_26_114_5_end
        jmp if_20_29_114_5_code
        if_20_29_114_5_code:
            mov rdi, 1
            exit_20_38_114_5:
                    mov rax, 60
                syscall
            exit_20_38_114_5_end:
        if_20_26_114_5_end:
    assert_114_5_end:
    mov qword [rsp - 56], 0
    mov qword [rsp - 48], 0
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    mov rcx, 4
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_118_12:
        mov rcx, 4
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_118_12
        mov r14, false
        jmp cmps_end_118_12
        cmps_eq_118_12:
        mov r14, true
        cmps_end_118_12:
    test r14, r14
    je bool_false_118_12
    jmp bool_true_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
    assert_118_5:
        if_20_29_118_5:
        cmp_20_29_118_5:
        test r15, r15
        jne if_20_26_118_5_end
        jmp if_20_29_118_5_code
        if_20_29_118_5_code:
            mov rdi, 1
            exit_20_38_118_5:
                    mov rax, 60
                syscall
            exit_20_38_118_5_end:
        if_20_26_118_5_end:
    assert_118_5_end:
    mov r15, 2
    mov r14, 121
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_122_12:
        mov rcx, 4
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_122_16
        mov r14, false
        jmp cmps_end_122_16
        cmps_eq_122_16:
        mov r14, true
        cmps_end_122_16:
    test r14, r14
    jne bool_false_122_12
    jmp bool_true_122_12
    bool_true_122_12:
    mov r15, true
    jmp bool_end_122_12
    bool_false_122_12:
    mov r15, false
    bool_end_122_12:
    assert_122_5:
        if_20_29_122_5:
        cmp_20_29_122_5:
        test r15, r15
        jne if_20_26_122_5_end
        jmp if_20_29_122_5_code
        if_20_29_122_5_code:
            mov rdi, 1
            exit_20_38_122_5:
                    mov rax, 60
                syscall
            exit_20_38_122_5_end:
        if_20_26_122_5_end:
    assert_122_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    mov r13, 125
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_125_16:
        lea r13, [rsp + r14 * 4 - 16]
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
        not dword [rsp + r15 * 4 - 16]
    inv_125_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_126_12:
        mov r13, qword [rsp - 24]
        mov r12, 126
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_126_12
    jmp bool_true_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_20_29_126_5:
        cmp_20_29_126_5:
        test r15, r15
        jne if_20_26_126_5_end
        jmp if_20_29_126_5_code
        if_20_29_126_5_code:
            mov rdi, 1
            exit_20_38_126_5:
                    mov rax, 60
                syscall
            exit_20_38_126_5_end:
        if_20_26_126_5_end:
    assert_126_5_end:
    faz_128_5:
        mov r15, 1
        mov r14, 76
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_128_5_end:
    cmp_129_12:
        mov r13, 1
        mov r12, 129
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 0xfe
    jne bool_false_129_12
    jmp bool_true_129_12
    bool_true_129_12:
    mov r15, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15, false
    bool_end_129_12:
    assert_129_5:
        if_20_29_129_5:
        cmp_20_29_129_5:
        test r15, r15
        jne if_20_26_129_5_end
        jmp if_20_29_129_5_code
        if_20_29_129_5_code:
            mov rdi, 1
            exit_20_38_129_5:
                    mov rax, 60
                syscall
            exit_20_38_129_5_end:
        if_20_26_129_5_end:
    assert_129_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_132_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_132_5_end:
    cmp_133_12:
    cmp qword [rsp - 72], 2
    jne bool_false_133_12
    jmp bool_true_133_12
    bool_true_133_12:
    mov r15, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15, false
    bool_end_133_12:
    assert_133_5:
        if_20_29_133_5:
        cmp_20_29_133_5:
        test r15, r15
        jne if_20_26_133_5_end
        jmp if_20_29_133_5_code
        if_20_29_133_5_code:
            mov rdi, 1
            exit_20_38_133_5:
                    mov rax, 60
                syscall
            exit_20_38_133_5_end:
        if_20_26_133_5_end:
    assert_133_5_end:
    cmp_134_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_134_12
    jmp bool_true_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
    assert_134_5:
        if_20_29_134_5:
        cmp_20_29_134_5:
        test r15, r15
        jne if_20_26_134_5_end
        jmp if_20_29_134_5_code
        if_20_29_134_5_code:
            mov rdi, 1
            exit_20_38_134_5:
                    mov rax, 60
                syscall
            exit_20_38_134_5_end:
        if_20_26_134_5_end:
    assert_134_5_end:
    mov qword [rsp - 80], 0
    bar_137_5:
        if_59_8_137_5:
        cmp_59_8_137_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_137_5_end
        jmp if_59_8_137_5_code
        if_59_8_137_5_code:
            jmp bar_137_5_end
        if_59_5_137_5_end:
        mov qword [rsp - 80], 0xff
    bar_137_5_end:
    cmp_138_12:
    cmp qword [rsp - 80], 0
    jne bool_false_138_12
    jmp bool_true_138_12
    bool_true_138_12:
    mov r15, true
    jmp bool_end_138_12
    bool_false_138_12:
    mov r15, false
    bool_end_138_12:
    assert_138_5:
        if_20_29_138_5:
        cmp_20_29_138_5:
        test r15, r15
        jne if_20_26_138_5_end
        jmp if_20_29_138_5_code
        if_20_29_138_5_code:
            mov rdi, 1
            exit_20_38_138_5:
                    mov rax, 60
                syscall
            exit_20_38_138_5_end:
        if_20_26_138_5_end:
    assert_138_5_end:
    mov qword [rsp - 80], 1
    bar_141_5:
        if_59_8_141_5:
        cmp_59_8_141_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_141_5_end
        jmp if_59_8_141_5_code
        if_59_8_141_5_code:
            jmp bar_141_5_end
        if_59_5_141_5_end:
        mov qword [rsp - 80], 0xff
    bar_141_5_end:
    cmp_142_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_142_12
    jmp bool_true_142_12
    bool_true_142_12:
    mov r15, true
    jmp bool_end_142_12
    bool_false_142_12:
    mov r15, false
    bool_end_142_12:
    assert_142_5:
        if_20_29_142_5:
        cmp_20_29_142_5:
        test r15, r15
        jne if_20_26_142_5_end
        jmp if_20_29_142_5_code
        if_20_29_142_5_code:
            mov rdi, 1
            exit_20_38_142_5:
                    mov rax, 60
                syscall
            exit_20_38_142_5_end:
        if_20_26_142_5_end:
    assert_142_5_end:
    mov qword [rsp - 88], 1
    baz_145_13:
        mov r15, qword [rsp - 88]
        imul r15, 2
    baz_145_13_end:
    mov qword [rsp - 96], r15
    cmp_146_12:
    cmp qword [rsp - 96], 2
    jne bool_false_146_12
    jmp bool_true_146_12
    bool_true_146_12:
    mov r15, true
    jmp bool_end_146_12
    bool_false_146_12:
    mov r15, false
    bool_end_146_12:
    assert_146_5:
        if_20_29_146_5:
        cmp_20_29_146_5:
        test r15, r15
        jne if_20_26_146_5_end
        jmp if_20_29_146_5_code
        if_20_29_146_5_code:
            mov rdi, 1
            exit_20_38_146_5:
                    mov rax, 60
                syscall
            exit_20_38_146_5_end:
        if_20_26_146_5_end:
    assert_146_5_end:
    baz_148_9:
        mov r15, 1
        imul r15, 2
    baz_148_9_end:
    mov qword [rsp - 96], r15
    cmp_149_12:
    cmp qword [rsp - 96], 2
    jne bool_false_149_12
    jmp bool_true_149_12
    bool_true_149_12:
    mov r15, true
    jmp bool_end_149_12
    bool_false_149_12:
    mov r15, false
    bool_end_149_12:
    assert_149_5:
        if_20_29_149_5:
        cmp_20_29_149_5:
        test r15, r15
        jne if_20_26_149_5_end
        jmp if_20_29_149_5_code
        if_20_29_149_5_code:
            mov rdi, 1
            exit_20_38_149_5:
                    mov rax, 60
                syscall
            exit_20_38_149_5_end:
        if_20_26_149_5_end:
    assert_149_5_end:
    baz_151_23:
        mov r15, 2
        imul r15, 2
    baz_151_23_end:
    mov qword [rsp - 112], r15
    mov qword [rsp - 104], 0
    cmp_152_12:
    cmp qword [rsp - 112], 4
    jne bool_false_152_12
    jmp bool_true_152_12
    bool_true_152_12:
    mov r15, true
    jmp bool_end_152_12
    bool_false_152_12:
    mov r15, false
    bool_end_152_12:
    assert_152_5:
        if_20_29_152_5:
        cmp_20_29_152_5:
        test r15, r15
        jne if_20_26_152_5_end
        jmp if_20_29_152_5_code
        if_20_29_152_5_code:
            mov rdi, 1
            exit_20_38_152_5:
                    mov rax, 60
                syscall
            exit_20_38_152_5_end:
        if_20_26_152_5_end:
    assert_152_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_158_12:
    cmp qword [rsp - 148], 10
    jne bool_false_158_12
    jmp bool_true_158_12
    bool_true_158_12:
    mov r15, true
    jmp bool_end_158_12
    bool_false_158_12:
    mov r15, false
    bool_end_158_12:
    assert_158_5:
        if_20_29_158_5:
        cmp_20_29_158_5:
        test r15, r15
        jne if_20_26_158_5_end
        jmp if_20_29_158_5_code
        if_20_29_158_5_code:
            mov rdi, 1
            exit_20_38_158_5:
                    mov rax, 60
                syscall
            exit_20_38_158_5_end:
        if_20_26_158_5_end:
    assert_158_5_end:
    cmp_159_12:
    cmp qword [rsp - 140], 2
    jne bool_false_159_12
    jmp bool_true_159_12
    bool_true_159_12:
    mov r15, true
    jmp bool_end_159_12
    bool_false_159_12:
    mov r15, false
    bool_end_159_12:
    assert_159_5:
        if_20_29_159_5:
        cmp_20_29_159_5:
        test r15, r15
        jne if_20_26_159_5_end
        jmp if_20_29_159_5_code
        if_20_29_159_5_code:
            mov rdi, 1
            exit_20_38_159_5:
                    mov rax, 60
                syscall
            exit_20_38_159_5_end:
        if_20_26_159_5_end:
    assert_159_5_end:
    cmp_160_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_160_12
    jmp bool_true_160_12
    bool_true_160_12:
    mov r15, true
    jmp bool_end_160_12
    bool_false_160_12:
    mov r15, false
    bool_end_160_12:
    assert_160_5:
        if_20_29_160_5:
        cmp_20_29_160_5:
        test r15, r15
        jne if_20_26_160_5_end
        jmp if_20_29_160_5_code
        if_20_29_160_5_code:
            mov rdi, 1
            exit_20_38_160_5:
                    mov rax, 60
                syscall
            exit_20_38_160_5_end:
        if_20_26_160_5_end:
    assert_160_5_end:
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
    neg qword [rsp - 164]
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
    neg qword [rsp - 156]
    lea rdi, [rsp - 148]
    lea rsi, [rsp - 164]
    mov rcx, 2
    rep movsq
    cmp_164_12:
    cmp qword [rsp - 148], -1
    jne bool_false_164_12
    jmp bool_true_164_12
    bool_true_164_12:
    mov r15, true
    jmp bool_end_164_12
    bool_false_164_12:
    mov r15, false
    bool_end_164_12:
    assert_164_5:
        if_20_29_164_5:
        cmp_20_29_164_5:
        test r15, r15
        jne if_20_26_164_5_end
        jmp if_20_29_164_5_code
        if_20_29_164_5_code:
            mov rdi, 1
            exit_20_38_164_5:
                    mov rax, 60
                syscall
            exit_20_38_164_5_end:
        if_20_26_164_5_end:
    assert_164_5_end:
    cmp_165_12:
    cmp qword [rsp - 140], -2
    jne bool_false_165_12
    jmp bool_true_165_12
    bool_true_165_12:
    mov r15, true
    jmp bool_end_165_12
    bool_false_165_12:
    mov r15, false
    bool_end_165_12:
    assert_165_5:
        if_20_29_165_5:
        cmp_20_29_165_5:
        test r15, r15
        jne if_20_26_165_5_end
        jmp if_20_29_165_5_code
        if_20_29_165_5_code:
            mov rdi, 1
            exit_20_38_165_5:
                    mov rax, 60
                syscall
            exit_20_38_165_5_end:
        if_20_26_165_5_end:
    assert_165_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_168_12:
    cmp qword [rsp - 184], -1
    jne bool_false_168_12
    jmp bool_true_168_12
    bool_true_168_12:
    mov r15, true
    jmp bool_end_168_12
    bool_false_168_12:
    mov r15, false
    bool_end_168_12:
    assert_168_5:
        if_20_29_168_5:
        cmp_20_29_168_5:
        test r15, r15
        jne if_20_26_168_5_end
        jmp if_20_29_168_5_code
        if_20_29_168_5_code:
            mov rdi, 1
            exit_20_38_168_5:
                    mov rax, 60
                syscall
            exit_20_38_168_5_end:
        if_20_26_168_5_end:
    assert_168_5_end:
    cmp_169_12:
    cmp qword [rsp - 176], -2
    jne bool_false_169_12
    jmp bool_true_169_12
    bool_true_169_12:
    mov r15, true
    jmp bool_end_169_12
    bool_false_169_12:
    mov r15, false
    bool_end_169_12:
    assert_169_5:
        if_20_29_169_5:
        cmp_20_29_169_5:
        test r15, r15
        jne if_20_26_169_5_end
        jmp if_20_29_169_5_code
        if_20_29_169_5_code:
            mov rdi, 1
            exit_20_38_169_5:
                    mov rax, 60
                syscall
            exit_20_38_169_5_end:
        if_20_26_169_5_end:
    assert_169_5_end:
    cmp_170_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_170_12
    jmp bool_true_170_12
    bool_true_170_12:
    mov r15, true
    jmp bool_end_170_12
    bool_false_170_12:
    mov r15, false
    bool_end_170_12:
    assert_170_5:
        if_20_29_170_5:
        cmp_20_29_170_5:
        test r15, r15
        jne if_20_26_170_5_end
        jmp if_20_29_170_5_code
        if_20_29_170_5_code:
            mov rdi, 1
            exit_20_38_170_5:
                    mov rax, 60
                syscall
            exit_20_38_170_5_end:
        if_20_26_170_5_end:
    assert_170_5_end:
    mov qword [rsp - 204], 0
    mov qword [rsp - 196], 0
    mov dword [rsp - 188], 0
    mov qword [rsp - 196], 73
    cmp_175_12:
        lea r13, [rsp - 204]
        mov r12, 0
        mov r11, 175
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 1
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        mov r14, qword [r13 + 8]
    cmp r14, 73
    jne bool_false_175_12
    jmp bool_true_175_12
    bool_true_175_12:
    mov r15, true
    jmp bool_end_175_12
    bool_false_175_12:
    mov r15, false
    bool_end_175_12:
    assert_175_5:
        if_20_29_175_5:
        cmp_20_29_175_5:
        test r15, r15
        jne if_20_26_175_5_end
        jmp if_20_29_175_5_code
        if_20_29_175_5_code:
            mov rdi, 1
            exit_20_38_175_5:
                    mov rax, 60
                syscall
            exit_20_38_175_5_end:
        if_20_26_175_5_end:
    assert_175_5_end:
    mov rcx, 64
    lea rdi, [rsp - 716]
    xor rax, rax
    rep stosq
    lea r15, [rsp - 716]
    mov r14, 1
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 0xffee
    cmp_179_12:
        lea r13, [rsp - 716]
        mov r12, 1
        mov r11, 179
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        mov r11, 179
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8]
    cmp r14, 0xffee
    jne bool_false_179_12
    jmp bool_true_179_12
    bool_true_179_12:
    mov r15, true
    jmp bool_end_179_12
    bool_false_179_12:
    mov r15, false
    bool_end_179_12:
    assert_179_5:
        if_20_29_179_5:
        cmp_20_29_179_5:
        test r15, r15
        jne if_20_26_179_5_end
        jmp if_20_29_179_5_code
        if_20_29_179_5_code:
            mov rdi, 1
            exit_20_38_179_5:
                    mov rax, 60
                syscall
            exit_20_38_179_5_end:
        if_20_26_179_5_end:
    assert_179_5_end:
    mov rcx, 8
    lea r15, [rsp - 716]
    mov r14, 1
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 182
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 716]
    mov r14, 0
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 183
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_188_12:
        lea r13, [rsp - 716]
        mov r12, 0
        mov r11, 188
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        mov r11, 188
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8]
    cmp r14, 0xffee
    jne bool_false_188_12
    jmp bool_true_188_12
    bool_true_188_12:
    mov r15, true
    jmp bool_end_188_12
    bool_false_188_12:
    mov r15, false
    bool_end_188_12:
    assert_188_5:
        if_20_29_188_5:
        cmp_20_29_188_5:
        test r15, r15
        jne if_20_26_188_5_end
        jmp if_20_29_188_5_code
        if_20_29_188_5_code:
            mov rdi, 1
            exit_20_38_188_5:
                    mov rax, 60
                syscall
            exit_20_38_188_5_end:
        if_20_26_188_5_end:
    assert_188_5_end:
    cmp_189_12:
        mov rcx, 8
        lea r13, [rsp - 716]
        mov r12, 0
        mov r11, 190
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 190
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 716]
        mov r12, 1
        mov r11, 191
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 191
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        je cmps_eq_189_12
        mov r14, false
        jmp cmps_end_189_12
        cmps_eq_189_12:
        mov r14, true
        cmps_end_189_12:
    test r14, r14
    je bool_false_189_12
    jmp bool_true_189_12
    bool_true_189_12:
    mov r15, true
    jmp bool_end_189_12
    bool_false_189_12:
    mov r15, false
    bool_end_189_12:
    assert_189_5:
        if_20_29_189_5:
        cmp_20_29_189_5:
        test r15, r15
        jne if_20_26_189_5_end
        jmp if_20_29_189_5_code
        if_20_29_189_5_code:
            mov rdi, 1
            exit_20_38_189_5:
                    mov rax, 60
                syscall
            exit_20_38_189_5_end:
        if_20_26_189_5_end:
    assert_189_5_end:
    mov rcx, 16
    lea rdi, [rsp - 844]
    xor rax, rax
    rep stosq
    mov rdx, hello.len
    mov rsi, hello
    print_196_5:
            mov rax, 1
            mov rdi, 0
        syscall
    print_196_5_end:
    loop_197_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_198_9:
                mov rax, 1
                mov rdi, 0
            syscall
        print_198_9_end:
        str_in_199_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 843]
                mov rdx, 127
            syscall
                mov byte [rsp - 844], al
                sub byte [rsp - 844], 1
        str_in_199_9_end:
        if_200_12:
        cmp_200_12:
        cmp byte [rsp - 844], 0
        jne if_202_19
        jmp if_200_12_code
        if_200_12_code:
            jmp loop_197_5_end
        jmp if_200_9_end
        if_202_19:
        cmp_202_19:
        cmp byte [rsp - 844], 4
        jg if_else_200_9
        jmp if_202_19_code
        if_202_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_203_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_203_13_end:
            jmp loop_197_5
        jmp if_200_9_end
        if_else_200_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_206_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_206_13_end:
            str_out_207_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 843]
                    movsx rdx, byte [rsp - 844]
                syscall
            str_out_207_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_208_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_208_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_209_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_209_13_end:
        if_200_9_end:
    jmp loop_197_5
    loop_197_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    lea rdi, [num_buffer + 19]
    mov byte [rdi], 10
    dec rdi
    mov rcx, 10
.convert_loop:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jnz .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    lea rdx, [num_buffer + 20]
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
