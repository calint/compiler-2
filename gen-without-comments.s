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
    cmp_119_12:
    cmp qword [rsp - 85], 0
    sete r15b
    bool_end_119_12:
    assert_119_5:
        if_13_29_119_5:
        cmp_13_29_119_5:
        cmp r15b, 0
        jne if_13_26_119_5_end
        if_13_29_119_5_code:
            mov rdi, 1
            exit_13_38_119_5:
                    mov rax, 60
                syscall
            exit_13_38_119_5_end:
        if_13_26_119_5_end:
    assert_119_5_end:
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
    mov r14, 126
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 77], 2
    mov r15, qword [rsp - 93]
    add r15, 1
    mov r14, 127
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    mov r13, 127
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 77]
    mov dword [rsp + r15 * 4 - 77], r13d
    cmp_128_12:
    mov r14, 1
    mov r13, 128
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_128_12:
    assert_128_5:
        if_13_29_128_5:
        cmp_13_29_128_5:
        cmp r15b, 0
        jne if_13_26_128_5_end
        if_13_29_128_5_code:
            mov rdi, 1
            exit_13_38_128_5:
                    mov rax, 60
                syscall
            exit_13_38_128_5_end:
        if_13_26_128_5_end:
    assert_128_5_end:
    cmp_129_12:
    mov r14, 2
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
    mov rcx, 2
    mov r15, 2
    mov r14, 131
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 77]
    mov r15, 131
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 77]
    shl rcx, 2
    rep movsb
    cmp_133_12:
    mov r14, 0
    mov r13, 133
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
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
    mov qword [rsp - 125], 0
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
    mov rcx, 4
    mov r15, 136
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 77]
    mov r15, 136
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 125]
    shl rcx, 2
    rep movsb
    cmp_137_12:
        mov rcx, 4
        mov r13, 137
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 137
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
    mov r15, 2
    mov r14, 140
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 125], -1
    cmp_141_12:
        mov rcx, 4
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 141
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
    mov qword [rsp - 93], 3
    mov r15, qword [rsp - 93]
    mov r14, 144
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    sub r14, 1
    mov r13, 144
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_144_16:
        mov r13d, dword [rsp + r14 * 4 - 77]
        mov dword [rsp + r15 * 4 - 77], r13d
        not dword [rsp + r15 * 4 - 77]
    inv_144_16_end:
    not dword [rsp + r15 * 4 - 77]
    cmp_145_12:
    mov r14, qword [rsp - 93]
    mov r13, 145
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_145_12:
    assert_145_5:
        if_13_29_145_5:
        cmp_13_29_145_5:
        cmp r15b, 0
        jne if_13_26_145_5_end
        if_13_29_145_5_code:
            mov rdi, 1
            exit_13_38_145_5:
                    mov rax, 60
                syscall
            exit_13_38_145_5_end:
        if_13_26_145_5_end:
    assert_145_5_end:
    faz_147_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 77], 254
    faz_147_5_end:
    cmp_148_12:
    mov r14, 1
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 254
    sete r15b
    bool_end_148_12:
    assert_148_5:
        if_13_29_148_5:
        cmp_13_29_148_5:
        cmp r15b, 0
        jne if_13_26_148_5_end
        if_13_29_148_5_code:
            mov rdi, 1
            exit_13_38_148_5:
                    mov rax, 60
                syscall
            exit_13_38_148_5_end:
        if_13_26_148_5_end:
    assert_148_5_end:
    mov qword [rsp - 141], 0
    mov qword [rsp - 133], 0
    foo_151_5:
        mov qword [rsp - 141], 2
        mov qword [rsp - 133], 11
    foo_151_5_end:
    cmp_152_12:
    cmp qword [rsp - 141], 2
    sete r15b
    bool_end_152_12:
    assert_152_5:
        if_13_29_152_5:
        cmp_13_29_152_5:
        cmp r15b, 0
        jne if_13_26_152_5_end
        if_13_29_152_5_code:
            mov rdi, 1
            exit_13_38_152_5:
                    mov rax, 60
                syscall
            exit_13_38_152_5_end:
        if_13_26_152_5_end:
    assert_152_5_end:
    cmp_153_12:
    cmp qword [rsp - 133], 11
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
    lea rsi, [rsp - 141]
    lea rdi, [rsp - 157]
    mov rcx, 2
    rep movsq
    cmp_156_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_156_12:
    assert_156_5:
        if_13_29_156_5:
        cmp_13_29_156_5:
        cmp r15b, 0
        jne if_13_26_156_5_end
        if_13_29_156_5_code:
            mov rdi, 1
            exit_13_38_156_5:
                    mov rax, 60
                syscall
            exit_13_38_156_5_end:
        if_13_26_156_5_end:
    assert_156_5_end:
    mov qword [rsp - 157], 3
    cmp_161_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
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
    mov qword [rsp - 165], 0
    bar_164_5:
        if_56_8_164_5:
        cmp_56_8_164_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_164_5_end
        if_56_8_164_5_code:
            jmp bar_164_5_end
        if_56_5_164_5_end:
        mov qword [rsp - 165], 255
    bar_164_5_end:
    cmp_165_12:
    cmp qword [rsp - 165], 0
    sete r15b
    bool_end_165_12:
    assert_165_5:
        if_13_29_165_5:
        cmp_13_29_165_5:
        cmp r15b, 0
        jne if_13_26_165_5_end
        if_13_29_165_5_code:
            mov rdi, 1
            exit_13_38_165_5:
                    mov rax, 60
                syscall
            exit_13_38_165_5_end:
        if_13_26_165_5_end:
    assert_165_5_end:
    mov qword [rsp - 165], 1
    bar_168_5:
        if_56_8_168_5:
        cmp_56_8_168_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_168_5_end
        if_56_8_168_5_code:
            jmp bar_168_5_end
        if_56_5_168_5_end:
        mov qword [rsp - 165], 255
    bar_168_5_end:
    cmp_169_12:
    cmp qword [rsp - 165], 255
    sete r15b
    bool_end_169_12:
    assert_169_5:
        if_13_29_169_5:
        cmp_13_29_169_5:
        cmp r15b, 0
        jne if_13_26_169_5_end
        if_13_29_169_5_code:
            mov rdi, 1
            exit_13_38_169_5:
                    mov rax, 60
                syscall
            exit_13_38_169_5_end:
        if_13_26_169_5_end:
    assert_169_5_end:
    mov qword [rsp - 173], 1
    baz_172_13:
        mov r15, qword [rsp - 173]
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_172_13_end:
    cmp_173_12:
    cmp qword [rsp - 181], 2
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_13_29_173_5:
        cmp_13_29_173_5:
        cmp r15b, 0
        jne if_13_26_173_5_end
        if_13_29_173_5_code:
            mov rdi, 1
            exit_13_38_173_5:
                    mov rax, 60
                syscall
            exit_13_38_173_5_end:
        if_13_26_173_5_end:
    assert_173_5_end:
    baz_175_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_175_9_end:
    cmp_176_12:
    cmp qword [rsp - 181], 2
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
    baz_178_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 197], r15
    baz_178_23_end:
    mov qword [rsp - 189], 0
    cmp_179_12:
    cmp qword [rsp - 197], 4
    sete r15b
    bool_end_179_12:
    assert_179_5:
        if_13_29_179_5:
        cmp_13_29_179_5:
        cmp r15b, 0
        jne if_13_26_179_5_end
        if_13_29_179_5_code:
            mov rdi, 1
            exit_13_38_179_5:
                    mov rax, 60
                syscall
            exit_13_38_179_5_end:
        if_13_26_179_5_end:
    assert_179_5_end:
    mov qword [rsp - 205], 1
    mov qword [rsp - 213], 2
    mov r15, qword [rsp - 205]
    imul r15, 10
    mov qword [rsp - 233], r15
    mov r15, qword [rsp - 213]
    mov qword [rsp - 225], r15
    mov dword [rsp - 217], 16711680
    cmp_185_12:
    cmp qword [rsp - 233], 10
    sete r15b
    bool_end_185_12:
    assert_185_5:
        if_13_29_185_5:
        cmp_13_29_185_5:
        cmp r15b, 0
        jne if_13_26_185_5_end
        if_13_29_185_5_code:
            mov rdi, 1
            exit_13_38_185_5:
                    mov rax, 60
                syscall
            exit_13_38_185_5_end:
        if_13_26_185_5_end:
    assert_185_5_end:
    cmp_186_12:
    cmp qword [rsp - 225], 2
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
    cmp dword [rsp - 217], 16711680
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
    cmp_191_12:
    cmp qword [rsp - 233], -1
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
    cmp_192_12:
    cmp qword [rsp - 225], -2
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
    lea rsi, [rsp - 233]
    lea rdi, [rsp - 269]
    mov rcx, 20
    rep movsb
    cmp_195_12:
    cmp qword [rsp - 269], -1
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_13_29_195_5:
        cmp_13_29_195_5:
        cmp r15b, 0
        jne if_13_26_195_5_end
        if_13_29_195_5_code:
            mov rdi, 1
            exit_13_38_195_5:
                    mov rax, 60
                syscall
            exit_13_38_195_5_end:
        if_13_26_195_5_end:
    assert_195_5_end:
    cmp_196_12:
    cmp qword [rsp - 261], -2
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
    cmp dword [rsp - 253], 16711680
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
    mov qword [rsp - 289], 0
    mov qword [rsp - 281], 0
    mov dword [rsp - 273], 0
    mov qword [rsp - 281], 73
    cmp_202_12:
    lea r14, [rsp - 289]
    mov r13, 0
    mov r12, 202
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
    bool_end_202_12:
    assert_202_5:
        if_13_29_202_5:
        cmp_13_29_202_5:
        cmp r15b, 0
        jne if_13_26_202_5_end
        if_13_29_202_5_code:
            mov rdi, 1
            exit_13_38_202_5:
                    mov rax, 60
                syscall
            exit_13_38_202_5_end:
        if_13_26_202_5_end:
    assert_202_5_end:
    mov rcx, 512
    lea rdi, [rsp - 801]
    xor rax, rax
    rep stosb
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 205
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 205
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_206_12:
    lea r14, [rsp - 801]
    mov r13, 1
    mov r12, 206
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 206
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_206_12:
    assert_206_5:
        if_13_29_206_5:
        cmp_13_29_206_5:
        cmp r15b, 0
        jne if_13_26_206_5_end
        if_13_29_206_5_code:
            mov rdi, 1
            exit_13_38_206_5:
                    mov rax, 60
                syscall
            exit_13_38_206_5_end:
        if_13_26_206_5_end:
    assert_206_5_end:
    mov rcx, 8
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 209
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 209
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 801]
    mov r14, 0
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
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_215_12:
    lea r14, [rsp - 801]
    mov r13, 0
    mov r12, 215
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 215
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_215_12:
    assert_215_5:
        if_13_29_215_5:
        cmp_13_29_215_5:
        cmp r15b, 0
        jne if_13_26_215_5_end
        if_13_29_215_5_code:
            mov rdi, 1
            exit_13_38_215_5:
                    mov rax, 60
                syscall
            exit_13_38_215_5_end:
        if_13_26_215_5_end:
    assert_215_5_end:
    cmp_216_12:
        mov rcx, 8
        lea r13, [rsp - 801]
        mov r12, 0
        mov r11, 217
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 217
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 801]
        mov r12, 1
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
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov rcx, 128
    lea rdi, [rsp - 929]
    xor rax, rax
    rep stosb
    print_str_223_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        print_24_4_223_5:
                mov rax, 1
                mov rdi, 0
            syscall
        print_24_4_223_5_end:
    print_str_223_5_end:
    loop_224_5:
        print_str_225_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            print_24_4_225_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_24_4_225_9_end:
        print_str_225_9_end:
        str_in_226_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 928]
                mov rdx, 127
            syscall
                mov byte [rsp - 929], al
                sub byte [rsp - 929], 1
        str_in_226_9_end:
        if_227_12:
        cmp_227_12:
        cmp byte [rsp - 929], 0
        jne if_229_19
        if_227_12_code:
            jmp loop_224_5_end
        jmp if_227_9_end
        if_229_19:
        cmp_229_19:
        cmp byte [rsp - 929], 4
        jg if_else_227_9
        if_229_19_code:
            print_str_230_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                print_24_4_230_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_230_13_end:
            print_str_230_13_end:
            jmp loop_224_5
        jmp if_227_9_end
        if_else_227_9:
            print_str_233_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                print_24_4_233_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_233_13_end:
            print_str_233_13_end:
            str_out_234_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 928]
                    movsx rdx, byte [rsp - 929]
                syscall
            str_out_234_13_end:
            print_str_235_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                print_24_4_235_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_235_13_end:
            print_str_235_13_end:
            print_str_236_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                print_24_4_236_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_236_13_end:
            print_str_236_13_end:
        if_227_9_end:
    jmp loop_224_5
    loop_224_5_end:
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
