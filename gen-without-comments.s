default rel
section .bss
stk resd 131072
stk.end:
section .text
bits 64
global _start
_start:
lea rsi, [dat]
lea rdi, [stk.end]
sub rdi, dat.len
mov rcx, dat.len
cld
rep movsb
mov rsp, stk.end
main:
    mov qword [rsp - 77], 0
    mov qword [rsp - 69], 0
    mov qword [rsp - 85], 1
    mov r15, qword [rsp - 85]
    mov r14, 111
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 77], 2
    mov r15, qword [rsp - 85]
    add r15, 1
    mov r14, 112
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 85]
    mov r13, 112
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 77]
    mov dword [rsp + r15 * 4 - 77], r13d
    cmp_113_12:
    mov r14, 1
    mov r13, 113
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_113_12:
    assert_113_5:
        if_13_29_113_5:
        cmp_13_29_113_5:
        cmp r15b, 0
        jne if_13_26_113_5_end
        if_13_29_113_5_code:
            mov rdi, 1
            exit_13_38_113_5:
                    mov rax, 60
                syscall
            exit_13_38_113_5_end:
        if_13_26_113_5_end:
    assert_113_5_end:
    cmp_114_12:
    mov r14, 2
    mov r13, 114
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_114_12:
    assert_114_5:
        if_13_29_114_5:
        cmp_13_29_114_5:
        cmp r15b, 0
        jne if_13_26_114_5_end
        if_13_29_114_5_code:
            mov rdi, 1
            exit_13_38_114_5:
                    mov rax, 60
                syscall
            exit_13_38_114_5_end:
        if_13_26_114_5_end:
    assert_114_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 116
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 77]
    mov r15, 116
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 77]
    shl rcx, 2
    rep movsb
    cmp_118_12:
    mov r14, 0
    mov r13, 118
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_118_12:
    assert_118_5:
        if_13_29_118_5:
        cmp_13_29_118_5:
        cmp r15b, 0
        jne if_13_26_118_5_end
        if_13_29_118_5_code:
            mov rdi, 1
            exit_13_38_118_5:
                    mov rax, 60
                syscall
            exit_13_38_118_5_end:
        if_13_26_118_5_end:
    assert_118_5_end:
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
    mov qword [rsp - 93], 0
    mov rcx, 4
    mov r15, 121
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 77]
    mov r15, 121
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 117]
    shl rcx, 2
    rep movsb
    cmp_122_12:
        mov rcx, 4
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 117]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_122_12:
    assert_122_5:
        if_13_29_122_5:
        cmp_13_29_122_5:
        cmp r15b, 0
        jne if_13_26_122_5_end
        if_13_29_122_5_code:
            mov rdi, 1
            exit_13_38_122_5:
                    mov rax, 60
                syscall
            exit_13_38_122_5_end:
        if_13_26_122_5_end:
    assert_122_5_end:
    mov r15, 2
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 117], -1
    cmp_126_12:
        mov rcx, 4
        mov r13, 126
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 126
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 117]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_126_12:
    assert_126_5:
        if_13_29_126_5:
        cmp_13_29_126_5:
        cmp r15b, 0
        jne if_13_26_126_5_end
        if_13_29_126_5_code:
            mov rdi, 1
            exit_13_38_126_5:
                    mov rax, 60
                syscall
            exit_13_38_126_5_end:
        if_13_26_126_5_end:
    assert_126_5_end:
    mov qword [rsp - 85], 3
    mov r15, qword [rsp - 85]
    mov r14, 129
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 85]
    sub r14, 1
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_129_16:
        mov r13d, dword [rsp + r14 * 4 - 77]
        mov dword [rsp + r15 * 4 - 77], r13d
        not dword [rsp + r15 * 4 - 77]
    inv_129_16_end:
    not dword [rsp + r15 * 4 - 77]
    cmp_130_12:
    mov r14, qword [rsp - 85]
    mov r13, 130
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_130_12:
    assert_130_5:
        if_13_29_130_5:
        cmp_13_29_130_5:
        cmp r15b, 0
        jne if_13_26_130_5_end
        if_13_29_130_5_code:
            mov rdi, 1
            exit_13_38_130_5:
                    mov rax, 60
                syscall
            exit_13_38_130_5_end:
        if_13_26_130_5_end:
    assert_130_5_end:
    faz_132_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 77], 254
    faz_132_5_end:
    cmp_133_12:
    mov r14, 1
    mov r13, 133
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 254
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_13_29_133_5:
        cmp_13_29_133_5:
        cmp r15b, 0
        jne if_13_26_133_5_end
        if_13_29_133_5_code:
            mov rdi, 1
            exit_13_38_133_5:
                    mov rax, 60
                syscall
            exit_13_38_133_5_end:
        if_13_26_133_5_end:
    assert_133_5_end:
    mov qword [rsp - 133], 0
    mov qword [rsp - 125], 0
    foo_136_5:
        mov qword [rsp - 133], 2
        mov qword [rsp - 125], 11
    foo_136_5_end:
    cmp_137_12:
    cmp qword [rsp - 133], 2
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_13_29_137_5:
        cmp_13_29_137_5:
        cmp r15b, 0
        jne if_13_26_137_5_end
        if_13_29_137_5_code:
            mov rdi, 1
            exit_13_38_137_5:
                    mov rax, 60
                syscall
            exit_13_38_137_5_end:
        if_13_26_137_5_end:
    assert_137_5_end:
    cmp_138_12:
    cmp qword [rsp - 125], 11
    sete r15b
    bool_end_138_12:
    assert_138_5:
        if_13_29_138_5:
        cmp_13_29_138_5:
        cmp r15b, 0
        jne if_13_26_138_5_end
        if_13_29_138_5_code:
            mov rdi, 1
            exit_13_38_138_5:
                    mov rax, 60
                syscall
            exit_13_38_138_5_end:
        if_13_26_138_5_end:
    assert_138_5_end:
    lea rsi, [rsp - 133]
    lea rdi, [rsp - 149]
    mov rcx, 2
    rep movsq
    cmp_141_12:
        lea rsi, [rsp - 133]
        lea rdi, [rsp - 149]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_141_12:
    assert_141_5:
        if_13_29_141_5:
        cmp_13_29_141_5:
        cmp r15b, 0
        jne if_13_26_141_5_end
        if_13_29_141_5_code:
            mov rdi, 1
            exit_13_38_141_5:
                    mov rax, 60
                syscall
            exit_13_38_141_5_end:
        if_13_26_141_5_end:
    assert_141_5_end:
    mov qword [rsp - 149], 3
    cmp_146_12:
        lea rsi, [rsp - 133]
        lea rdi, [rsp - 149]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_146_12:
    assert_146_5:
        if_13_29_146_5:
        cmp_13_29_146_5:
        cmp r15b, 0
        jne if_13_26_146_5_end
        if_13_29_146_5_code:
            mov rdi, 1
            exit_13_38_146_5:
                    mov rax, 60
                syscall
            exit_13_38_146_5_end:
        if_13_26_146_5_end:
    assert_146_5_end:
    mov qword [rsp - 157], 0
    bar_149_5:
        if_56_8_149_5:
        cmp_56_8_149_5:
        cmp qword [rsp - 157], 0
        jne if_56_5_149_5_end
        if_56_8_149_5_code:
            jmp bar_149_5_end
        if_56_5_149_5_end:
        mov qword [rsp - 157], 255
    bar_149_5_end:
    cmp_150_12:
    cmp qword [rsp - 157], 0
    sete r15b
    bool_end_150_12:
    assert_150_5:
        if_13_29_150_5:
        cmp_13_29_150_5:
        cmp r15b, 0
        jne if_13_26_150_5_end
        if_13_29_150_5_code:
            mov rdi, 1
            exit_13_38_150_5:
                    mov rax, 60
                syscall
            exit_13_38_150_5_end:
        if_13_26_150_5_end:
    assert_150_5_end:
    mov qword [rsp - 157], 1
    bar_153_5:
        if_56_8_153_5:
        cmp_56_8_153_5:
        cmp qword [rsp - 157], 0
        jne if_56_5_153_5_end
        if_56_8_153_5_code:
            jmp bar_153_5_end
        if_56_5_153_5_end:
        mov qword [rsp - 157], 255
    bar_153_5_end:
    cmp_154_12:
    cmp qword [rsp - 157], 255
    sete r15b
    bool_end_154_12:
    assert_154_5:
        if_13_29_154_5:
        cmp_13_29_154_5:
        cmp r15b, 0
        jne if_13_26_154_5_end
        if_13_29_154_5_code:
            mov rdi, 1
            exit_13_38_154_5:
                    mov rax, 60
                syscall
            exit_13_38_154_5_end:
        if_13_26_154_5_end:
    assert_154_5_end:
    mov qword [rsp - 165], 1
    baz_157_13:
        mov r15, qword [rsp - 165]
        imul r15, 2
        mov qword [rsp - 173], r15
    baz_157_13_end:
    cmp_158_12:
    cmp qword [rsp - 173], 2
    sete r15b
    bool_end_158_12:
    assert_158_5:
        if_13_29_158_5:
        cmp_13_29_158_5:
        cmp r15b, 0
        jne if_13_26_158_5_end
        if_13_29_158_5_code:
            mov rdi, 1
            exit_13_38_158_5:
                    mov rax, 60
                syscall
            exit_13_38_158_5_end:
        if_13_26_158_5_end:
    assert_158_5_end:
    baz_160_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 173], r15
    baz_160_9_end:
    cmp_161_12:
    cmp qword [rsp - 173], 2
    sete r15b
    bool_end_161_12:
    assert_161_5:
        if_13_29_161_5:
        cmp_13_29_161_5:
        cmp r15b, 0
        jne if_13_26_161_5_end
        if_13_29_161_5_code:
            mov rdi, 1
            exit_13_38_161_5:
                    mov rax, 60
                syscall
            exit_13_38_161_5_end:
        if_13_26_161_5_end:
    assert_161_5_end:
    baz_163_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 189], r15
    baz_163_23_end:
    mov qword [rsp - 181], 0
    cmp_164_12:
    cmp qword [rsp - 189], 4
    sete r15b
    bool_end_164_12:
    assert_164_5:
        if_13_29_164_5:
        cmp_13_29_164_5:
        cmp r15b, 0
        jne if_13_26_164_5_end
        if_13_29_164_5_code:
            mov rdi, 1
            exit_13_38_164_5:
                    mov rax, 60
                syscall
            exit_13_38_164_5_end:
        if_13_26_164_5_end:
    assert_164_5_end:
    mov qword [rsp - 197], 1
    mov qword [rsp - 205], 2
    mov r15, qword [rsp - 197]
    imul r15, 10
    mov qword [rsp - 225], r15
    mov r15, qword [rsp - 205]
    mov qword [rsp - 217], r15
    mov dword [rsp - 209], 16711680
    cmp_170_12:
    cmp qword [rsp - 225], 10
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_13_29_170_5:
        cmp_13_29_170_5:
        cmp r15b, 0
        jne if_13_26_170_5_end
        if_13_29_170_5_code:
            mov rdi, 1
            exit_13_38_170_5:
                    mov rax, 60
                syscall
            exit_13_38_170_5_end:
        if_13_26_170_5_end:
    assert_170_5_end:
    cmp_171_12:
    cmp qword [rsp - 217], 2
    sete r15b
    bool_end_171_12:
    assert_171_5:
        if_13_29_171_5:
        cmp_13_29_171_5:
        cmp r15b, 0
        jne if_13_26_171_5_end
        if_13_29_171_5_code:
            mov rdi, 1
            exit_13_38_171_5:
                    mov rax, 60
                syscall
            exit_13_38_171_5_end:
        if_13_26_171_5_end:
    assert_171_5_end:
    cmp_172_12:
    cmp dword [rsp - 209], 16711680
    sete r15b
    bool_end_172_12:
    assert_172_5:
        if_13_29_172_5:
        cmp_13_29_172_5:
        cmp r15b, 0
        jne if_13_26_172_5_end
        if_13_29_172_5_code:
            mov rdi, 1
            exit_13_38_172_5:
                    mov rax, 60
                syscall
            exit_13_38_172_5_end:
        if_13_26_172_5_end:
    assert_172_5_end:
    mov r15, qword [rsp - 197]
    mov qword [rsp - 241], r15
    neg qword [rsp - 241]
    mov r15, qword [rsp - 205]
    mov qword [rsp - 233], r15
    neg qword [rsp - 233]
    lea rsi, [rsp - 241]
    lea rdi, [rsp - 225]
    mov rcx, 2
    rep movsq
    cmp_176_12:
    cmp qword [rsp - 225], -1
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_13_29_176_5:
        cmp_13_29_176_5:
        cmp r15b, 0
        jne if_13_26_176_5_end
        if_13_29_176_5_code:
            mov rdi, 1
            exit_13_38_176_5:
                    mov rax, 60
                syscall
            exit_13_38_176_5_end:
        if_13_26_176_5_end:
    assert_176_5_end:
    cmp_177_12:
    cmp qword [rsp - 217], -2
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_13_29_177_5:
        cmp_13_29_177_5:
        cmp r15b, 0
        jne if_13_26_177_5_end
        if_13_29_177_5_code:
            mov rdi, 1
            exit_13_38_177_5:
                    mov rax, 60
                syscall
            exit_13_38_177_5_end:
        if_13_26_177_5_end:
    assert_177_5_end:
    lea rsi, [rsp - 225]
    lea rdi, [rsp - 261]
    mov rcx, 20
    rep movsb
    cmp_180_12:
    cmp qword [rsp - 261], -1
    sete r15b
    bool_end_180_12:
    assert_180_5:
        if_13_29_180_5:
        cmp_13_29_180_5:
        cmp r15b, 0
        jne if_13_26_180_5_end
        if_13_29_180_5_code:
            mov rdi, 1
            exit_13_38_180_5:
                    mov rax, 60
                syscall
            exit_13_38_180_5_end:
        if_13_26_180_5_end:
    assert_180_5_end:
    cmp_181_12:
    cmp qword [rsp - 253], -2
    sete r15b
    bool_end_181_12:
    assert_181_5:
        if_13_29_181_5:
        cmp_13_29_181_5:
        cmp r15b, 0
        jne if_13_26_181_5_end
        if_13_29_181_5_code:
            mov rdi, 1
            exit_13_38_181_5:
                    mov rax, 60
                syscall
            exit_13_38_181_5_end:
        if_13_26_181_5_end:
    assert_181_5_end:
    cmp_182_12:
    cmp dword [rsp - 245], 16711680
    sete r15b
    bool_end_182_12:
    assert_182_5:
        if_13_29_182_5:
        cmp_13_29_182_5:
        cmp r15b, 0
        jne if_13_26_182_5_end
        if_13_29_182_5_code:
            mov rdi, 1
            exit_13_38_182_5:
                    mov rax, 60
                syscall
            exit_13_38_182_5_end:
        if_13_26_182_5_end:
    assert_182_5_end:
    mov qword [rsp - 281], 0
    mov qword [rsp - 273], 0
    mov dword [rsp - 265], 0
    mov qword [rsp - 273], 73
    cmp_187_12:
    lea r14, [rsp - 281]
    mov r13, 0
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_187_12:
    assert_187_5:
        if_13_29_187_5:
        cmp_13_29_187_5:
        cmp r15b, 0
        jne if_13_26_187_5_end
        if_13_29_187_5_code:
            mov rdi, 1
            exit_13_38_187_5:
                    mov rax, 60
                syscall
            exit_13_38_187_5_end:
        if_13_26_187_5_end:
    assert_187_5_end:
    mov rcx, 512
    lea rdi, [rsp - 793]
    xor rax, rax
    rep stosb
    lea r15, [rsp - 793]
    mov r14, 1
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_191_12:
    lea r14, [rsp - 793]
    mov r13, 1
    mov r12, 191
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 191
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_13_29_191_5:
        cmp_13_29_191_5:
        cmp r15b, 0
        jne if_13_26_191_5_end
        if_13_29_191_5_code:
            mov rdi, 1
            exit_13_38_191_5:
                    mov rax, 60
                syscall
            exit_13_38_191_5_end:
        if_13_26_191_5_end:
    assert_191_5_end:
    mov rcx, 8
    lea r15, [rsp - 793]
    mov r14, 1
    mov r13, 194
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 194
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 793]
    mov r14, 0
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 195
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_200_12:
    lea r14, [rsp - 793]
    mov r13, 0
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_200_12:
    assert_200_5:
        if_13_29_200_5:
        cmp_13_29_200_5:
        cmp r15b, 0
        jne if_13_26_200_5_end
        if_13_29_200_5_code:
            mov rdi, 1
            exit_13_38_200_5:
                    mov rax, 60
                syscall
            exit_13_38_200_5_end:
        if_13_26_200_5_end:
    assert_200_5_end:
    cmp_201_12:
        mov rcx, 8
        lea r13, [rsp - 793]
        mov r12, 0
        mov r11, 202
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 202
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 793]
        mov r12, 1
        mov r11, 203
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 203
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_201_12:
    assert_201_5:
        if_13_29_201_5:
        cmp_13_29_201_5:
        cmp r15b, 0
        jne if_13_26_201_5_end
        if_13_29_201_5_code:
            mov rdi, 1
            exit_13_38_201_5:
                    mov rax, 60
                syscall
            exit_13_38_201_5_end:
        if_13_26_201_5_end:
    assert_201_5_end:
    mov rcx, 128
    lea rdi, [rsp - 921]
    xor rax, rax
    rep stosb
    print_str_208_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        print_24_4_208_5:
                mov rax, 1
                mov rdi, 0
            syscall
        print_24_4_208_5_end:
    print_str_208_5_end:
    loop_209_5:
        print_str_210_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            print_24_4_210_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_24_4_210_9_end:
        print_str_210_9_end:
        str_in_211_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 920]
                mov rdx, 127
            syscall
                mov byte [rsp - 921], al
                sub byte [rsp - 921], 1
        str_in_211_9_end:
        if_212_12:
        cmp_212_12:
        cmp byte [rsp - 921], 0
        jne if_214_19
        if_212_12_code:
            jmp loop_209_5_end
        jmp if_212_9_end
        if_214_19:
        cmp_214_19:
        cmp byte [rsp - 921], 4
        jg if_else_212_9
        if_214_19_code:
            print_str_215_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                print_24_4_215_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_215_13_end:
            print_str_215_13_end:
            jmp loop_209_5
        jmp if_212_9_end
        if_else_212_9:
            print_str_218_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                print_24_4_218_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_218_13_end:
            print_str_218_13_end:
            str_out_219_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 920]
                    movsx rdx, byte [rsp - 921]
                syscall
            str_out_219_13_end:
            print_str_220_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                print_24_4_220_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_220_13_end:
            print_str_220_13_end:
            print_str_221_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                print_24_4_221_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_221_13_end:
            print_str_221_13_end:
        if_212_9_end:
    jmp loop_209_5
    loop_209_5_end:
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
section .rodata
dat:
db '', 10, ''
db '.'
db 'hello '
db 'that is not a name.', 10, ''
db 'enter name:', 10, ''
db 'hello world from baz', 10, ''
dat.len equ $ - dat
