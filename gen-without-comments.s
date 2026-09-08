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
    mov qword [rsp - 85], 0
    cmp_118_12:
    cmp qword [rsp - 85], 0
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
    mov qword [rsp - 85], -1
    cmp_122_12:
    cmp qword [rsp - 85], -1
    sete r15b
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
    mov qword [rsp - 93], 1
    mov r15, qword [rsp - 93]
    mov r14, 127
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 77], 2
    mov r15, qword [rsp - 93]
    add r15, 1
    mov r14, 128
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    mov r13, 128
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 77]
    mov dword [rsp + r15 * 4 - 77], r13d
    cmp_129_12:
    mov r14, 1
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_129_12:
    assert_129_5:
        if_13_29_129_5:
        cmp_13_29_129_5:
        cmp r15b, 0
        jne if_13_26_129_5_end
        if_13_29_129_5_code:
            mov rdi, 1
            exit_13_38_129_5:
                    mov rax, 60
                syscall
            exit_13_38_129_5_end:
        if_13_26_129_5_end:
    assert_129_5_end:
    cmp_130_12:
    mov r14, 2
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
    mov rcx, 2
    mov r15, 2
    mov r14, 132
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 77]
    mov r15, 132
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 77]
    shl rcx, 2
    rep movsb
    cmp_134_12:
    mov r14, 0
    mov r13, 134
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_134_12:
    assert_134_5:
        if_13_29_134_5:
        cmp_13_29_134_5:
        cmp r15b, 0
        jne if_13_26_134_5_end
        if_13_29_134_5_code:
            mov rdi, 1
            exit_13_38_134_5:
                    mov rax, 60
                syscall
            exit_13_38_134_5_end:
        if_13_26_134_5_end:
    assert_134_5_end:
    mov qword [rsp - 125], 0
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
    mov rcx, 4
    mov r15, 137
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 77]
    mov r15, 137
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 125]
    shl rcx, 2
    rep movsb
    cmp_138_12:
        mov rcx, 4
        mov r13, 138
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 138
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov r15, 2
    mov r14, 141
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 125], -1
    cmp_142_12:
        mov rcx, 4
        mov r13, 142
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 142
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_142_12:
    assert_142_5:
        if_13_29_142_5:
        cmp_13_29_142_5:
        cmp r15b, 0
        jne if_13_26_142_5_end
        if_13_29_142_5_code:
            mov rdi, 1
            exit_13_38_142_5:
                    mov rax, 60
                syscall
            exit_13_38_142_5_end:
        if_13_26_142_5_end:
    assert_142_5_end:
    mov qword [rsp - 93], 3
    mov r15, qword [rsp - 93]
    mov r14, 145
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    sub r14, 1
    mov r13, 145
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_145_16:
        mov r13d, dword [rsp + r14 * 4 - 77]
        mov dword [rsp + r15 * 4 - 77], r13d
        not dword [rsp + r15 * 4 - 77]
    inv_145_16_end:
    not dword [rsp + r15 * 4 - 77]
    cmp_146_12:
    mov r14, qword [rsp - 93]
    mov r13, 146
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
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
    faz_148_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 77], 254
    faz_148_5_end:
    cmp_149_12:
    mov r14, 1
    mov r13, 149
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 254
    sete r15b
    bool_end_149_12:
    assert_149_5:
        if_13_29_149_5:
        cmp_13_29_149_5:
        cmp r15b, 0
        jne if_13_26_149_5_end
        if_13_29_149_5_code:
            mov rdi, 1
            exit_13_38_149_5:
                    mov rax, 60
                syscall
            exit_13_38_149_5_end:
        if_13_26_149_5_end:
    assert_149_5_end:
    mov qword [rsp - 141], 0
    mov qword [rsp - 133], 0
    foo_152_5:
        mov qword [rsp - 141], 2
        mov qword [rsp - 133], 11
    foo_152_5_end:
    cmp_153_12:
    cmp qword [rsp - 141], 2
    sete r15b
    bool_end_153_12:
    assert_153_5:
        if_13_29_153_5:
        cmp_13_29_153_5:
        cmp r15b, 0
        jne if_13_26_153_5_end
        if_13_29_153_5_code:
            mov rdi, 1
            exit_13_38_153_5:
                    mov rax, 60
                syscall
            exit_13_38_153_5_end:
        if_13_26_153_5_end:
    assert_153_5_end:
    cmp_154_12:
    cmp qword [rsp - 133], 11
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
    lea rsi, [rsp - 141]
    lea rdi, [rsp - 157]
    mov rcx, 2
    rep movsq
    cmp_157_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_157_12:
    assert_157_5:
        if_13_29_157_5:
        cmp_13_29_157_5:
        cmp r15b, 0
        jne if_13_26_157_5_end
        if_13_29_157_5_code:
            mov rdi, 1
            exit_13_38_157_5:
                    mov rax, 60
                syscall
            exit_13_38_157_5_end:
        if_13_26_157_5_end:
    assert_157_5_end:
    mov qword [rsp - 157], 3
    cmp_162_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_162_12:
    assert_162_5:
        if_13_29_162_5:
        cmp_13_29_162_5:
        cmp r15b, 0
        jne if_13_26_162_5_end
        if_13_29_162_5_code:
            mov rdi, 1
            exit_13_38_162_5:
                    mov rax, 60
                syscall
            exit_13_38_162_5_end:
        if_13_26_162_5_end:
    assert_162_5_end:
    mov qword [rsp - 165], 0
    bar_165_5:
        if_56_8_165_5:
        cmp_56_8_165_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_165_5_end
        if_56_8_165_5_code:
            jmp bar_165_5_end
        if_56_5_165_5_end:
        mov qword [rsp - 165], 255
    bar_165_5_end:
    cmp_166_12:
    cmp qword [rsp - 165], 0
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_13_29_166_5:
        cmp_13_29_166_5:
        cmp r15b, 0
        jne if_13_26_166_5_end
        if_13_29_166_5_code:
            mov rdi, 1
            exit_13_38_166_5:
                    mov rax, 60
                syscall
            exit_13_38_166_5_end:
        if_13_26_166_5_end:
    assert_166_5_end:
    mov qword [rsp - 165], 1
    bar_169_5:
        if_56_8_169_5:
        cmp_56_8_169_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_169_5_end
        if_56_8_169_5_code:
            jmp bar_169_5_end
        if_56_5_169_5_end:
        mov qword [rsp - 165], 255
    bar_169_5_end:
    cmp_170_12:
    cmp qword [rsp - 165], 255
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
    mov qword [rsp - 173], 1
    baz_173_13:
        mov r15, qword [rsp - 173]
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_173_13_end:
    cmp_174_12:
    cmp qword [rsp - 181], 2
    sete r15b
    bool_end_174_12:
    assert_174_5:
        if_13_29_174_5:
        cmp_13_29_174_5:
        cmp r15b, 0
        jne if_13_26_174_5_end
        if_13_29_174_5_code:
            mov rdi, 1
            exit_13_38_174_5:
                    mov rax, 60
                syscall
            exit_13_38_174_5_end:
        if_13_26_174_5_end:
    assert_174_5_end:
    baz_176_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_176_9_end:
    cmp_177_12:
    cmp qword [rsp - 181], 2
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
    baz_179_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 197], r15
    baz_179_23_end:
    mov qword [rsp - 189], 0
    cmp_180_12:
    cmp qword [rsp - 197], 4
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
    mov qword [rsp - 205], 1
    mov qword [rsp - 213], 2
    mov r15, qword [rsp - 205]
    imul r15, 10
    mov qword [rsp - 233], r15
    mov r15, qword [rsp - 213]
    mov qword [rsp - 225], r15
    mov dword [rsp - 217], 16711680
    cmp_186_12:
    cmp qword [rsp - 233], 10
    sete r15b
    bool_end_186_12:
    assert_186_5:
        if_13_29_186_5:
        cmp_13_29_186_5:
        cmp r15b, 0
        jne if_13_26_186_5_end
        if_13_29_186_5_code:
            mov rdi, 1
            exit_13_38_186_5:
                    mov rax, 60
                syscall
            exit_13_38_186_5_end:
        if_13_26_186_5_end:
    assert_186_5_end:
    cmp_187_12:
    cmp qword [rsp - 225], 2
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
    cmp_188_12:
    cmp dword [rsp - 217], 16711680
    sete r15b
    bool_end_188_12:
    assert_188_5:
        if_13_29_188_5:
        cmp_13_29_188_5:
        cmp r15b, 0
        jne if_13_26_188_5_end
        if_13_29_188_5_code:
            mov rdi, 1
            exit_13_38_188_5:
                    mov rax, 60
                syscall
            exit_13_38_188_5_end:
        if_13_26_188_5_end:
    assert_188_5_end:
    mov r15, qword [rsp - 205]
    mov qword [rsp - 249], r15
    neg qword [rsp - 249]
    mov r15, qword [rsp - 213]
    mov qword [rsp - 241], r15
    neg qword [rsp - 241]
    lea rsi, [rsp - 249]
    lea rdi, [rsp - 233]
    mov rcx, 2
    rep movsq
    cmp_192_12:
    cmp qword [rsp - 233], -1
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_13_29_192_5:
        cmp_13_29_192_5:
        cmp r15b, 0
        jne if_13_26_192_5_end
        if_13_29_192_5_code:
            mov rdi, 1
            exit_13_38_192_5:
                    mov rax, 60
                syscall
            exit_13_38_192_5_end:
        if_13_26_192_5_end:
    assert_192_5_end:
    cmp_193_12:
    cmp qword [rsp - 225], -2
    sete r15b
    bool_end_193_12:
    assert_193_5:
        if_13_29_193_5:
        cmp_13_29_193_5:
        cmp r15b, 0
        jne if_13_26_193_5_end
        if_13_29_193_5_code:
            mov rdi, 1
            exit_13_38_193_5:
                    mov rax, 60
                syscall
            exit_13_38_193_5_end:
        if_13_26_193_5_end:
    assert_193_5_end:
    lea rsi, [rsp - 233]
    lea rdi, [rsp - 269]
    mov rcx, 20
    rep movsb
    cmp_196_12:
    cmp qword [rsp - 269], -1
    sete r15b
    bool_end_196_12:
    assert_196_5:
        if_13_29_196_5:
        cmp_13_29_196_5:
        cmp r15b, 0
        jne if_13_26_196_5_end
        if_13_29_196_5_code:
            mov rdi, 1
            exit_13_38_196_5:
                    mov rax, 60
                syscall
            exit_13_38_196_5_end:
        if_13_26_196_5_end:
    assert_196_5_end:
    cmp_197_12:
    cmp qword [rsp - 261], -2
    sete r15b
    bool_end_197_12:
    assert_197_5:
        if_13_29_197_5:
        cmp_13_29_197_5:
        cmp r15b, 0
        jne if_13_26_197_5_end
        if_13_29_197_5_code:
            mov rdi, 1
            exit_13_38_197_5:
                    mov rax, 60
                syscall
            exit_13_38_197_5_end:
        if_13_26_197_5_end:
    assert_197_5_end:
    cmp_198_12:
    cmp dword [rsp - 253], 16711680
    sete r15b
    bool_end_198_12:
    assert_198_5:
        if_13_29_198_5:
        cmp_13_29_198_5:
        cmp r15b, 0
        jne if_13_26_198_5_end
        if_13_29_198_5_code:
            mov rdi, 1
            exit_13_38_198_5:
                    mov rax, 60
                syscall
            exit_13_38_198_5_end:
        if_13_26_198_5_end:
    assert_198_5_end:
    mov qword [rsp - 289], 0
    mov qword [rsp - 281], 0
    mov dword [rsp - 273], 0
    mov qword [rsp - 281], 73
    cmp_203_12:
    lea r14, [rsp - 289]
    mov r13, 0
    mov r12, 203
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
    bool_end_203_12:
    assert_203_5:
        if_13_29_203_5:
        cmp_13_29_203_5:
        cmp r15b, 0
        jne if_13_26_203_5_end
        if_13_29_203_5_code:
            mov rdi, 1
            exit_13_38_203_5:
                    mov rax, 60
                syscall
            exit_13_38_203_5_end:
        if_13_26_203_5_end:
    assert_203_5_end:
    mov rcx, 512
    lea rdi, [rsp - 801]
    xor rax, rax
    rep stosb
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 206
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 206
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_207_12:
    lea r14, [rsp - 801]
    mov r13, 1
    mov r12, 207
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 207
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_13_29_207_5:
        cmp_13_29_207_5:
        cmp r15b, 0
        jne if_13_26_207_5_end
        if_13_29_207_5_code:
            mov rdi, 1
            exit_13_38_207_5:
                    mov rax, 60
                syscall
            exit_13_38_207_5_end:
        if_13_26_207_5_end:
    assert_207_5_end:
    mov rcx, 8
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 210
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 801]
    mov r14, 0
    mov r13, 211
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 211
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_216_12:
    lea r14, [rsp - 801]
    mov r13, 0
    mov r12, 216
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 216
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_13_29_216_5:
        cmp_13_29_216_5:
        cmp r15b, 0
        jne if_13_26_216_5_end
        if_13_29_216_5_code:
            mov rdi, 1
            exit_13_38_216_5:
                    mov rax, 60
                syscall
            exit_13_38_216_5_end:
        if_13_26_216_5_end:
    assert_216_5_end:
    cmp_217_12:
        mov rcx, 8
        lea r13, [rsp - 801]
        mov r12, 0
        mov r11, 218
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 218
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 801]
        mov r12, 1
        mov r11, 219
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 219
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
    bool_end_217_12:
    assert_217_5:
        if_13_29_217_5:
        cmp_13_29_217_5:
        cmp r15b, 0
        jne if_13_26_217_5_end
        if_13_29_217_5_code:
            mov rdi, 1
            exit_13_38_217_5:
                    mov rax, 60
                syscall
            exit_13_38_217_5_end:
        if_13_26_217_5_end:
    assert_217_5_end:
    mov rcx, 128
    lea rdi, [rsp - 929]
    xor rax, rax
    rep stosb
    print_str_224_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        print_24_4_224_5:
                mov rax, 1
                mov rdi, 0
            syscall
        print_24_4_224_5_end:
    print_str_224_5_end:
    loop_225_5:
        print_str_226_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            print_24_4_226_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_24_4_226_9_end:
        print_str_226_9_end:
        str_in_227_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 928]
                mov rdx, 127
            syscall
                mov byte [rsp - 929], al
                sub byte [rsp - 929], 1
        str_in_227_9_end:
        if_228_12:
        cmp_228_12:
        cmp byte [rsp - 929], 0
        jne if_230_19
        if_228_12_code:
            jmp loop_225_5_end
        jmp if_228_9_end
        if_230_19:
        cmp_230_19:
        cmp byte [rsp - 929], 4
        jg if_else_228_9
        if_230_19_code:
            print_str_231_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                print_24_4_231_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_231_13_end:
            print_str_231_13_end:
            jmp loop_225_5
        jmp if_228_9_end
        if_else_228_9:
            print_str_234_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                print_24_4_234_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_234_13_end:
            print_str_234_13_end:
            str_out_235_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 928]
                    movsx rdx, byte [rsp - 929]
                syscall
            str_out_235_13_end:
            print_str_236_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                print_24_4_236_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_236_13_end:
            print_str_236_13_end:
            print_str_237_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                print_24_4_237_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_237_13_end:
            print_str_237_13_end:
        if_228_9_end:
    jmp loop_225_5
    loop_225_5_end:
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
db `\n`
db `.`
db `hello `
db `that is not a name.\n`
db `enter name:\n`
db `hello world from baz\n`
dat.len equ $ - dat
