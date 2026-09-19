default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    cmp_133_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_41_26_133_5:
        cmp_41_26_133_5:
        cmp r15b, 0
        jne if_41_23_133_5_end
        if_41_26_133_5_code:
            mov rdi, 1
            exit_41_32_133_5:
                    mov rax, 60
                syscall
            exit_41_32_133_5_end:
        if_41_23_133_5_end:
    assert_133_5_end:
    mov qword [rbp + 240], -1
    cmp_137_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_41_26_137_5:
        cmp_41_26_137_5:
        cmp r15b, 0
        jne if_41_23_137_5_end
        if_41_26_137_5_code:
            mov rdi, 1
            exit_41_32_137_5:
                    mov rax, 60
                syscall
            exit_41_32_137_5_end:
        if_41_23_137_5_end:
    assert_137_5_end:
        cmp_141_16:
        bool_end_141_16:
        mov r15b, 1
        assert_141_9:
            if_41_26_141_9:
            cmp_41_26_141_9:
            cmp r15b, 0
            jne if_41_23_141_9_end
            if_41_26_141_9_code:
                mov rdi, 1
                exit_41_32_141_9:
                        mov rax, 60
                    syscall
                exit_41_32_141_9_end:
            if_41_23_141_9_end:
        assert_141_9_end:
    cmp_144_12:
    bool_end_144_12:
    mov r15b, 1
    assert_144_5:
        if_41_26_144_5:
        cmp_41_26_144_5:
        cmp r15b, 0
        jne if_41_23_144_5_end
        if_41_26_144_5_code:
            mov rdi, 1
            exit_41_32_144_5:
                    mov rax, 60
                syscall
            exit_41_32_144_5_end:
        if_41_23_144_5_end:
    assert_144_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 149
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 150
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rbp + 248]
    mov r13, 150
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_151_12:
    mov r14, 1
    mov r13, 151
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_151_12:
    assert_151_5:
        if_41_26_151_5:
        cmp_41_26_151_5:
        cmp r15b, 0
        jne if_41_23_151_5_end
        if_41_26_151_5_code:
            mov rdi, 1
            exit_41_32_151_5:
                    mov rax, 60
                syscall
            exit_41_32_151_5_end:
        if_41_23_151_5_end:
    assert_151_5_end:
    cmp_152_12:
    mov r14, 2
    mov r13, 152
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_152_12:
    assert_152_5:
        if_41_26_152_5:
        cmp_41_26_152_5:
        cmp r15b, 0
        jne if_41_23_152_5_end
        if_41_26_152_5_code:
            mov rdi, 1
            exit_41_32_152_5:
                    mov rax, 60
                syscall
            exit_41_32_152_5_end:
        if_41_23_152_5_end:
    assert_152_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 154
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_156_12:
    mov r14, 0
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_41_26_156_5:
        cmp_41_26_156_5:
        cmp r15b, 0
        jne if_41_23_156_5_end
        if_41_26_156_5_code:
            mov rdi, 1
            exit_41_32_156_5:
                    mov rax, 60
                syscall
            exit_41_32_156_5_end:
        if_41_23_156_5_end:
    assert_156_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rbp + 224]
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_160_12:
        mov rcx, 4
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rbp + 224]
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_160_12:
    assert_160_5:
        if_41_26_160_5:
        cmp_41_26_160_5:
        cmp r15b, 0
        jne if_41_23_160_5_end
        if_41_26_160_5_code:
            mov rdi, 1
            exit_41_32_160_5:
                    mov rax, 60
                syscall
            exit_41_32_160_5_end:
        if_41_23_160_5_end:
    assert_160_5_end:
    mov r15, 2
    mov r14, 163
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_164_12:
        mov rcx, 4
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rbp + 224]
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_164_12:
    assert_164_5:
        if_41_26_164_5:
        cmp_41_26_164_5:
        cmp r15b, 0
        jne if_41_23_164_5_end
        if_41_26_164_5_code:
            mov rdi, 1
            exit_41_32_164_5:
                    mov rax, 60
                syscall
            exit_41_32_164_5_end:
        if_41_23_164_5_end:
    assert_164_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_167_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_167_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_168_12:
    mov r14, qword [rbp + 248]
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_168_12:
    assert_168_5:
        if_41_26_168_5:
        cmp_41_26_168_5:
        cmp r15b, 0
        jne if_41_23_168_5_end
        if_41_26_168_5_code:
            mov rdi, 1
            exit_41_32_168_5:
                    mov rax, 60
                syscall
            exit_41_32_168_5_end:
        if_41_23_168_5_end:
    assert_168_5_end:
    faz_170_5:
        mov r15, 1
        mov r14, 93
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rbp + r15 * 4 + 224], 254
    faz_170_5_end:
    cmp_171_12:
    mov r14, 1
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_171_12:
    assert_171_5:
        if_41_26_171_5:
        cmp_41_26_171_5:
        cmp r15b, 0
        jne if_41_23_171_5_end
        if_41_26_171_5_code:
            mov rdi, 1
            exit_41_32_171_5:
                    mov rax, 60
                syscall
            exit_41_32_171_5_end:
        if_41_23_171_5_end:
    assert_171_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_174_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_174_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_174_5
    foo_174_5_end:
    cmp_177_12:
    mov r14, 0
    mov r13, 177
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_41_26_177_5:
        cmp_41_26_177_5:
        cmp r15b, 0
        jne if_41_23_177_5_end
        if_41_26_177_5_code:
            mov rdi, 1
            exit_41_32_177_5:
                    mov rax, 60
                syscall
            exit_41_32_177_5_end:
        if_41_23_177_5_end:
    assert_177_5_end:
    cmp_178_12:
    mov r14, 1
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_178_12:
    assert_178_5:
        if_41_26_178_5:
        cmp_41_26_178_5:
        cmp r15b, 0
        jne if_41_23_178_5_end
        if_41_26_178_5_code:
            mov rdi, 1
            exit_41_32_178_5:
                    mov rax, 60
                syscall
            exit_41_32_178_5_end:
        if_41_23_178_5_end:
    assert_178_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_185_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_185_5_end:
    cmp_186_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_186_12:
    assert_186_5:
        if_41_26_186_5:
        cmp_41_26_186_5:
        cmp r15b, 0
        jne if_41_23_186_5_end
        if_41_26_186_5_code:
            mov rdi, 1
            exit_41_32_186_5:
                    mov rax, 60
                syscall
            exit_41_32_186_5_end:
        if_41_23_186_5_end:
    assert_186_5_end:
    cmp_187_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_187_12:
    assert_187_5:
        if_41_26_187_5:
        cmp_41_26_187_5:
        cmp r15b, 0
        jne if_41_23_187_5_end
        if_41_26_187_5_code:
            mov rdi, 1
            exit_41_32_187_5:
                    mov rax, 60
                syscall
            exit_41_32_187_5_end:
        if_41_23_187_5_end:
    assert_187_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_190_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_190_12:
    assert_190_5:
        if_41_26_190_5:
        cmp_41_26_190_5:
        cmp r15b, 0
        jne if_41_23_190_5_end
        if_41_26_190_5_code:
            mov rdi, 1
            exit_41_32_190_5:
                    mov rax, 60
                syscall
            exit_41_32_190_5_end:
        if_41_23_190_5_end:
    assert_190_5_end:
    mov qword [rbp + 320], 3
    cmp_195_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_41_26_195_5:
        cmp_41_26_195_5:
        cmp r15b, 0
        jne if_41_23_195_5_end
        if_41_26_195_5_code:
            mov rdi, 1
            exit_41_32_195_5:
                    mov rax, 60
                syscall
            exit_41_32_195_5_end:
        if_41_23_195_5_end:
    assert_195_5_end:
    mov qword [rbp + 336], 0
    bar_198_5:
        if_74_8_198_5:
        cmp_74_8_198_5:
        cmp qword [rbp + 336], 0
        jne if_74_5_198_5_end
        if_74_8_198_5_code:
            jmp bar_198_5_end
        if_74_5_198_5_end:
        mov qword [rbp + 336], 255
    bar_198_5_end:
    cmp_199_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_199_12:
    assert_199_5:
        if_41_26_199_5:
        cmp_41_26_199_5:
        cmp r15b, 0
        jne if_41_23_199_5_end
        if_41_26_199_5_code:
            mov rdi, 1
            exit_41_32_199_5:
                    mov rax, 60
                syscall
            exit_41_32_199_5_end:
        if_41_23_199_5_end:
    assert_199_5_end:
    mov qword [rbp + 336], 1
    bar_202_5:
        if_74_8_202_5:
        cmp_74_8_202_5:
        cmp qword [rbp + 336], 0
        jne if_74_5_202_5_end
        if_74_8_202_5_code:
            jmp bar_202_5_end
        if_74_5_202_5_end:
        mov qword [rbp + 336], 255
    bar_202_5_end:
    cmp_203_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_41_26_203_5:
        cmp_41_26_203_5:
        cmp r15b, 0
        jne if_41_23_203_5_end
        if_41_26_203_5_code:
            mov rdi, 1
            exit_41_32_203_5:
                    mov rax, 60
                syscall
            exit_41_32_203_5_end:
        if_41_23_203_5_end:
    assert_203_5_end:
    mov qword [rbp + 344], 1
    baz_206_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_206_13_end:
    cmp_207_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_41_26_207_5:
        cmp_41_26_207_5:
        cmp r15b, 0
        jne if_41_23_207_5_end
        if_41_26_207_5_code:
            mov rdi, 1
            exit_41_32_207_5:
                    mov rax, 60
                syscall
            exit_41_32_207_5_end:
        if_41_23_207_5_end:
    assert_207_5_end:
    baz_209_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_209_9_end:
    cmp_210_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_41_26_210_5:
        cmp_41_26_210_5:
        cmp r15b, 0
        jne if_41_23_210_5_end
        if_41_26_210_5_code:
            mov rdi, 1
            exit_41_32_210_5:
                    mov rax, 60
                syscall
            exit_41_32_210_5_end:
        if_41_23_210_5_end:
    assert_210_5_end:
    baz_212_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_212_23_end:
    mov qword [rbp + 368], 0
    cmp_213_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_41_26_213_5:
        cmp_41_26_213_5:
        cmp r15b, 0
        jne if_41_23_213_5_end
        if_41_26_213_5_code:
            mov rdi, 1
            exit_41_32_213_5:
                    mov rax, 60
                syscall
            exit_41_32_213_5_end:
        if_41_23_213_5_end:
    assert_213_5_end:
    point_init_215_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_215_22_end:
    cmp_216_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_41_26_216_5:
        cmp_41_26_216_5:
        cmp r15b, 0
        jne if_41_23_216_5_end
        if_41_26_216_5_code:
            mov rdi, 1
            exit_41_32_216_5:
                    mov rax, 60
                syscall
            exit_41_32_216_5_end:
        if_41_23_216_5_end:
    assert_216_5_end:
    cmp_217_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_217_12:
    assert_217_5:
        if_41_26_217_5:
        cmp_41_26_217_5:
        cmp r15b, 0
        jne if_41_23_217_5_end
        if_41_26_217_5_code:
            mov rdi, 1
            exit_41_32_217_5:
                    mov rax, 60
                syscall
            exit_41_32_217_5_end:
        if_41_23_217_5_end:
    assert_217_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_223_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_223_12:
    assert_223_5:
        if_41_26_223_5:
        cmp_41_26_223_5:
        cmp r15b, 0
        jne if_41_23_223_5_end
        if_41_26_223_5_code:
            mov rdi, 1
            exit_41_32_223_5:
                    mov rax, 60
                syscall
            exit_41_32_223_5_end:
        if_41_23_223_5_end:
    assert_223_5_end:
    cmp_224_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_224_12:
    assert_224_5:
        if_41_26_224_5:
        cmp_41_26_224_5:
        cmp r15b, 0
        jne if_41_23_224_5_end
        if_41_26_224_5_code:
            mov rdi, 1
            exit_41_32_224_5:
                    mov rax, 60
                syscall
            exit_41_32_224_5_end:
        if_41_23_224_5_end:
    assert_224_5_end:
    cmp_225_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_41_26_225_5:
        cmp_41_26_225_5:
        cmp r15b, 0
        jne if_41_23_225_5_end
        if_41_26_225_5_code:
            mov rdi, 1
            exit_41_32_225_5:
                    mov rax, 60
                syscall
            exit_41_32_225_5_end:
        if_41_23_225_5_end:
    assert_225_5_end:
    mov r15, qword [rbp + 392]
    mov qword [rbp + 428], r15
    neg qword [rbp + 428]
    mov r15, qword [rbp + 400]
    mov qword [rbp + 436], r15
    neg qword [rbp + 436]
    mov rax, qword [rbp + 428]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 436]
    mov qword [rbp + 416], rax
    cmp_229_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_41_26_229_5:
        cmp_41_26_229_5:
        cmp r15b, 0
        jne if_41_23_229_5_end
        if_41_26_229_5_code:
            mov rdi, 1
            exit_41_32_229_5:
                    mov rax, 60
                syscall
            exit_41_32_229_5_end:
        if_41_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_41_26_230_5:
        cmp_41_26_230_5:
        cmp r15b, 0
        jne if_41_23_230_5_end
        if_41_26_230_5_code:
            mov rdi, 1
            exit_41_32_230_5:
                    mov rax, 60
                syscall
            exit_41_32_230_5_end:
        if_41_23_230_5_end:
    assert_230_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_233_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_233_12:
    assert_233_5:
        if_41_26_233_5:
        cmp_41_26_233_5:
        cmp r15b, 0
        jne if_41_23_233_5_end
        if_41_26_233_5_code:
            mov rdi, 1
            exit_41_32_233_5:
                    mov rax, 60
                syscall
            exit_41_32_233_5_end:
        if_41_23_233_5_end:
    assert_233_5_end:
    cmp_234_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_41_26_234_5:
        cmp_41_26_234_5:
        cmp r15b, 0
        jne if_41_23_234_5_end
        if_41_26_234_5_code:
            mov rdi, 1
            exit_41_32_234_5:
                    mov rax, 60
                syscall
            exit_41_32_234_5_end:
        if_41_23_234_5_end:
    assert_234_5_end:
    cmp_235_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_235_12:
    assert_235_5:
        if_41_26_235_5:
        cmp_41_26_235_5:
        cmp r15b, 0
        jne if_41_23_235_5_end
        if_41_26_235_5_code:
            mov rdi, 1
            exit_41_32_235_5:
                    mov rax, 60
                syscall
            exit_41_32_235_5_end:
        if_41_23_235_5_end:
    assert_235_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_241_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 241
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_241_12:
    assert_241_5:
        if_41_26_241_5:
        cmp_41_26_241_5:
        cmp r15b, 0
        jne if_41_23_241_5_end
        if_41_26_241_5_code:
            mov rdi, 1
            exit_41_32_241_5:
                    mov rax, 60
                syscall
            exit_41_32_241_5_end:
        if_41_23_241_5_end:
    assert_241_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 243
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_243_13:
        mov qword [r15 + 8], 74
    object_init_243_13_end:
    cmp_244_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 244
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 74
    sete r15b
    bool_end_244_12:
    assert_244_5:
        if_41_26_244_5:
        cmp_41_26_244_5:
        cmp r15b, 0
        jne if_41_23_244_5_end
        if_41_26_244_5_code:
            mov rdi, 1
            exit_41_32_244_5:
                    mov rax, 60
                syscall
            exit_41_32_244_5_end:
        if_41_23_244_5_end:
    assert_244_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_248_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 248
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 248
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_248_12:
    assert_248_5:
        if_41_26_248_5:
        cmp_41_26_248_5:
        cmp r15b, 0
        jne if_41_23_248_5_end
        if_41_26_248_5_code:
            mov rdi, 1
            exit_41_32_248_5:
                    mov rax, 60
                syscall
            exit_41_32_248_5_end:
        if_41_23_248_5_end:
    assert_248_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 251
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 252
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_258_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 258
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 258
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_258_12:
    assert_258_5:
        if_41_26_258_5:
        cmp_41_26_258_5:
        cmp r15b, 0
        jne if_41_23_258_5_end
        if_41_26_258_5_code:
            mov rdi, 1
            exit_41_32_258_5:
                    mov rax, 60
                syscall
            exit_41_32_258_5_end:
        if_41_23_258_5_end:
    assert_258_5_end:
    cmp_259_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r11, 260
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 260
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r11, 261
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 261
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
    cmp r14b, 0
    setne r15b
    bool_end_259_12:
    assert_259_5:
        if_41_26_259_5:
        cmp_41_26_259_5:
        cmp r15b, 0
        jne if_41_23_259_5_end
        if_41_26_259_5_code:
            mov rdi, 1
            exit_41_32_259_5:
                    mov rax, 60
                syscall
            exit_41_32_259_5_end:
        if_41_23_259_5_end:
    assert_259_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_266_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_266_12:
    assert_266_5:
        if_41_26_266_5:
        cmp_41_26_266_5:
        cmp r15b, 0
        jne if_41_23_266_5_end
        if_41_26_266_5_code:
            mov rdi, 1
            exit_41_32_266_5:
                    mov rax, 60
                syscall
            exit_41_32_266_5_end:
        if_41_23_266_5_end:
    assert_266_5_end:
    cmp_267_12:
    mov r14, 0
    mov r13, 267
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_267_12:
    assert_267_5:
        if_41_26_267_5:
        cmp_41_26_267_5:
        cmp r15b, 0
        jne if_41_23_267_5_end
        if_41_26_267_5_code:
            mov rdi, 1
            exit_41_32_267_5:
                    mov rax, 60
                syscall
            exit_41_32_267_5_end:
        if_41_23_267_5_end:
    assert_267_5_end:
    cmp_268_12:
    mov r14, 1
    mov r13, 268
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_268_12:
    assert_268_5:
        if_41_26_268_5:
        cmp_41_26_268_5:
        cmp r15b, 0
        jne if_41_23_268_5_end
        if_41_26_268_5_code:
            mov rdi, 1
            exit_41_32_268_5:
                    mov rax, 60
                syscall
            exit_41_32_268_5_end:
        if_41_23_268_5_end:
    assert_268_5_end:
    xor al, al
    lea rdi, [rbp + 1032]
    mov rcx, 128
    rep stosb
    print_271_5:
        mov rdx, 21
        lea rsi, [rbp]
        sys_print_52_4_271_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_52_4_271_5_end:
    print_271_5_end:
    loop_272_5:
        print_273_9:
            mov rdx, 12
            lea rsi, [rbp + 21]
            sys_print_52_4_273_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_52_4_273_9_end:
        print_273_9_end:
        str_in_274_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rbp + 1033]
                mov rdx, 127
            syscall
                mov byte [rbp + 1032], al
                sub byte [rbp + 1032], 1
        str_in_274_9_end:
        if_275_12:
        cmp_275_12:
        cmp byte [rbp + 1032], 0
        jne if_277_19
        if_275_12_code:
            jmp loop_272_5_end
        jmp if_275_9_end
        if_277_19:
        cmp_277_19:
        cmp byte [rbp + 1032], 4
        jg if_else_275_9
        if_277_19_code:
            print_278_13:
                mov rdx, 20
                lea rsi, [rbp + 33]
                sys_print_52_4_278_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_278_13_end:
            print_278_13_end:
            jmp loop_272_5
        jmp if_275_9_end
        if_else_275_9:
            print_281_13:
                mov rdx, 6
                lea rsi, [rbp + 53]
                sys_print_52_4_281_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_281_13_end:
            print_281_13_end:
            str_out_282_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rbp + 1033]
                    movsx rdx, byte [rbp + 1032]
                syscall
            str_out_282_13_end:
            print_283_13:
                mov rdx, 1
                lea rsi, [rbp + 59]
                sys_print_52_4_283_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_283_13_end:
            print_283_13_end:
            print_284_13:
                mov rdx, 1
                lea rsi, [rbp + 60]
                sys_print_52_4_284_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_284_13_end:
            print_284_13_end:
        if_275_9_end:
    jmp loop_272_5
    loop_272_5_end:
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
section .data
align 16
dat:
db `hello world from baz\n`
db `enter name:\n`
db `that is not a name.\n`
db `hello `
db `.`
db `\n`
dq 1
times 24 db 0
db 3
times 127 db 0
dat.end:
section .bss.stack nobits alloc write
align 16
stk:
stk resb 131072
stk.end:
