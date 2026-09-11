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
    mov qword [rsp - 205], 0
    mov qword [rsp - 197], 0
    mov qword [rsp - 213], 0
    cmp_132_12:
    cmp qword [rsp - 213], 0
    sete r15b
    bool_end_132_12:
    assert_132_5:
        if_13_29_132_5:
        cmp_13_29_132_5:
        cmp r15b, 0
        jne if_13_26_132_5_end
        if_13_29_132_5_code:
            mov rdi, 1
            exit_13_38_132_5:
                    mov rax, 60
                syscall
            exit_13_38_132_5_end:
        if_13_26_132_5_end:
    assert_132_5_end:
    mov qword [rsp - 213], -1
    cmp_136_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_136_12:
    assert_136_5:
        if_13_29_136_5:
        cmp_13_29_136_5:
        cmp r15b, 0
        jne if_13_26_136_5_end
        if_13_29_136_5_code:
            mov rdi, 1
            exit_13_38_136_5:
                    mov rax, 60
                syscall
            exit_13_38_136_5_end:
        if_13_26_136_5_end:
    assert_136_5_end:
    mov qword [rsp - 221], 1
    mov r15, qword [rsp - 221]
    mov r14, 141
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, 142
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    mov r13, 142
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_143_12:
    mov r14, 1
    mov r13, 143
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_143_12:
    assert_143_5:
        if_13_29_143_5:
        cmp_13_29_143_5:
        cmp r15b, 0
        jne if_13_26_143_5_end
        if_13_29_143_5_code:
            mov rdi, 1
            exit_13_38_143_5:
                    mov rax, 60
                syscall
            exit_13_38_143_5_end:
        if_13_26_143_5_end:
    assert_143_5_end:
    cmp_144_12:
    mov r14, 2
    mov r13, 144
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_144_12:
    assert_144_5:
        if_13_29_144_5:
        cmp_13_29_144_5:
        cmp r15b, 0
        jne if_13_26_144_5_end
        if_13_29_144_5_code:
            mov rdi, 1
            exit_13_38_144_5:
                    mov rax, 60
                syscall
            exit_13_38_144_5_end:
        if_13_26_144_5_end:
    assert_144_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 146
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 205]
    mov r15, 146
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_148_12:
    mov r14, 0
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
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
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov rcx, 4
    mov r15, 151
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 205]
    mov r15, 151
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_152_12:
        mov rcx, 4
        mov r13, 152
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 152
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov r15, 2
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 253], -1
    cmp_156_12:
        mov rcx, 4
        mov r13, 156
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 156
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
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
    mov qword [rsp - 221], 3
    mov r15, qword [rsp - 221]
    mov r14, 159
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    sub r14, 1
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_159_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_159_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_160_12:
    mov r14, qword [rsp - 221]
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_160_12:
    assert_160_5:
        if_13_29_160_5:
        cmp_13_29_160_5:
        cmp r15b, 0
        jne if_13_26_160_5_end
        if_13_29_160_5_code:
            mov rdi, 1
            exit_13_38_160_5:
                    mov rax, 60
                syscall
            exit_13_38_160_5_end:
        if_13_26_160_5_end:
    assert_160_5_end:
    faz_162_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 205], 254
    faz_162_5_end:
    cmp_163_12:
    mov r14, 1
    mov r13, 163
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 254
    sete r15b
    bool_end_163_12:
    assert_163_5:
        if_13_29_163_5:
        cmp_13_29_163_5:
        cmp r15b, 0
        jne if_13_26_163_5_end
        if_13_29_163_5_code:
            mov rdi, 1
            exit_13_38_163_5:
                    mov rax, 60
                syscall
            exit_13_38_163_5_end:
        if_13_26_163_5_end:
    assert_163_5_end:
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    foo_166_5:
        mov qword [rsp - 269], 2
        mov qword [rsp - 261], 11
    foo_166_5_end:
    cmp_167_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_167_12:
    assert_167_5:
        if_13_29_167_5:
        cmp_13_29_167_5:
        cmp r15b, 0
        jne if_13_26_167_5_end
        if_13_29_167_5_code:
            mov rdi, 1
            exit_13_38_167_5:
                    mov rax, 60
                syscall
            exit_13_38_167_5_end:
        if_13_26_167_5_end:
    assert_167_5_end:
    cmp_168_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_168_12:
    assert_168_5:
        if_13_29_168_5:
        cmp_13_29_168_5:
        cmp r15b, 0
        jne if_13_26_168_5_end
        if_13_29_168_5_code:
            mov rdi, 1
            exit_13_38_168_5:
                    mov rax, 60
                syscall
            exit_13_38_168_5_end:
        if_13_26_168_5_end:
    assert_168_5_end:
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
    cmp_171_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov qword [rsp - 285], 3
    cmp_176_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
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
    mov qword [rsp - 293], 0
    bar_179_5:
        if_56_8_179_5:
        cmp_56_8_179_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_179_5_end
        if_56_8_179_5_code:
            jmp bar_179_5_end
        if_56_5_179_5_end:
        mov qword [rsp - 293], 255
    bar_179_5_end:
    cmp_180_12:
    cmp qword [rsp - 293], 0
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
    mov qword [rsp - 293], 1
    bar_183_5:
        if_56_8_183_5:
        cmp_56_8_183_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_183_5_end
        if_56_8_183_5_code:
            jmp bar_183_5_end
        if_56_5_183_5_end:
        mov qword [rsp - 293], 255
    bar_183_5_end:
    cmp_184_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_184_12:
    assert_184_5:
        if_13_29_184_5:
        cmp_13_29_184_5:
        cmp r15b, 0
        jne if_13_26_184_5_end
        if_13_29_184_5_code:
            mov rdi, 1
            exit_13_38_184_5:
                    mov rax, 60
                syscall
            exit_13_38_184_5_end:
        if_13_26_184_5_end:
    assert_184_5_end:
    mov qword [rsp - 301], 1
    baz_187_13:
        mov r15, qword [rsp - 301]
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_187_13_end:
    cmp_188_12:
    cmp qword [rsp - 309], 2
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
    baz_190_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_190_9_end:
    cmp_191_12:
    cmp qword [rsp - 309], 2
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
    baz_193_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 325], r15
    baz_193_23_end:
    mov qword [rsp - 317], 0
    cmp_194_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_194_12:
    assert_194_5:
        if_13_29_194_5:
        cmp_13_29_194_5:
        cmp r15b, 0
        jne if_13_26_194_5_end
        if_13_29_194_5_code:
            mov rdi, 1
            exit_13_38_194_5:
                    mov rax, 60
                syscall
            exit_13_38_194_5_end:
        if_13_26_194_5_end:
    assert_194_5_end:
    point_init_196_22:
        mov qword [rsp - 341], -1
        mov qword [rsp - 333], -2
    point_init_196_22_end:
    cmp_197_12:
    cmp qword [rsp - 341], -1
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
    cmp qword [rsp - 333], -2
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
    mov qword [rsp - 349], 1
    mov qword [rsp - 357], 2
    mov r15, qword [rsp - 349]
    imul r15, 10
    mov qword [rsp - 377], r15
    mov r15, qword [rsp - 357]
    mov qword [rsp - 369], r15
    mov dword [rsp - 361], 16711680
    cmp_204_12:
    cmp qword [rsp - 377], 10
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_13_29_204_5:
        cmp_13_29_204_5:
        cmp r15b, 0
        jne if_13_26_204_5_end
        if_13_29_204_5_code:
            mov rdi, 1
            exit_13_38_204_5:
                    mov rax, 60
                syscall
            exit_13_38_204_5_end:
        if_13_26_204_5_end:
    assert_204_5_end:
    cmp_205_12:
    cmp qword [rsp - 369], 2
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_13_29_205_5:
        cmp_13_29_205_5:
        cmp r15b, 0
        jne if_13_26_205_5_end
        if_13_29_205_5_code:
            mov rdi, 1
            exit_13_38_205_5:
                    mov rax, 60
                syscall
            exit_13_38_205_5_end:
        if_13_26_205_5_end:
    assert_205_5_end:
    cmp_206_12:
    cmp dword [rsp - 361], 16711680
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
    mov r15, qword [rsp - 349]
    mov qword [rsp - 393], r15
    neg qword [rsp - 393]
    mov r15, qword [rsp - 357]
    mov qword [rsp - 385], r15
    neg qword [rsp - 385]
    mov rax, qword [rsp - 393]
    mov qword [rsp - 377], rax
    mov rax, qword [rsp - 385]
    mov qword [rsp - 369], rax
    cmp_210_12:
    cmp qword [rsp - 377], -1
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_13_29_210_5:
        cmp_13_29_210_5:
        cmp r15b, 0
        jne if_13_26_210_5_end
        if_13_29_210_5_code:
            mov rdi, 1
            exit_13_38_210_5:
                    mov rax, 60
                syscall
            exit_13_38_210_5_end:
        if_13_26_210_5_end:
    assert_210_5_end:
    cmp_211_12:
    cmp qword [rsp - 369], -2
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_13_29_211_5:
        cmp_13_29_211_5:
        cmp r15b, 0
        jne if_13_26_211_5_end
        if_13_29_211_5_code:
            mov rdi, 1
            exit_13_38_211_5:
                    mov rax, 60
                syscall
            exit_13_38_211_5_end:
        if_13_26_211_5_end:
    assert_211_5_end:
    lea rsi, [rsp - 377]
    lea rdi, [rsp - 413]
    mov rcx, 20
    rep movsb
    cmp_214_12:
    cmp qword [rsp - 413], -1
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_13_29_214_5:
        cmp_13_29_214_5:
        cmp r15b, 0
        jne if_13_26_214_5_end
        if_13_29_214_5_code:
            mov rdi, 1
            exit_13_38_214_5:
                    mov rax, 60
                syscall
            exit_13_38_214_5_end:
        if_13_26_214_5_end:
    assert_214_5_end:
    cmp_215_12:
    cmp qword [rsp - 405], -2
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
    cmp dword [rsp - 397], 16711680
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
    xor al, al
    lea rdi, [rsp - 453]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 445], 73
    cmp_222_12:
    lea r14, [rsp - 453]
    mov r13, 0
    mov r12, 222
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
    bool_end_222_12:
    assert_222_5:
        if_13_29_222_5:
        cmp_13_29_222_5:
        cmp r15b, 0
        jne if_13_26_222_5_end
        if_13_29_222_5_code:
            mov rdi, 1
            exit_13_38_222_5:
                    mov rax, 60
                syscall
            exit_13_38_222_5_end:
        if_13_26_222_5_end:
    assert_222_5_end:
    lea r15, [rsp - 453]
    mov r14, 1
    mov r13, 224
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_224_13:
        mov qword [r15 + 8], 74
    object_init_224_13_end:
    cmp_225_12:
    lea r14, [rsp - 453]
    mov r13, 1
    mov r12, 225
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
    bool_end_225_12:
    assert_225_5:
        if_13_29_225_5:
        cmp_13_29_225_5:
        cmp r15b, 0
        jne if_13_26_225_5_end
        if_13_29_225_5_code:
            mov rdi, 1
            exit_13_38_225_5:
                    mov rax, 60
                syscall
            exit_13_38_225_5_end:
        if_13_26_225_5_end:
    assert_225_5_end:
    xor al, al
    lea rdi, [rsp - 965]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 228
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 228
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_229_12:
    lea r14, [rsp - 965]
    mov r13, 1
    mov r12, 229
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 229
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_13_29_229_5:
        cmp_13_29_229_5:
        cmp r15b, 0
        jne if_13_26_229_5_end
        if_13_29_229_5_code:
            mov rdi, 1
            exit_13_38_229_5:
                    mov rax, 60
                syscall
            exit_13_38_229_5_end:
        if_13_26_229_5_end:
    assert_229_5_end:
    mov rcx, 8
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 232
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 232
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 965]
    mov r14, 0
    mov r13, 233
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 233
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_239_12:
    lea r14, [rsp - 965]
    mov r13, 0
    mov r12, 239
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 239
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_13_29_239_5:
        cmp_13_29_239_5:
        cmp r15b, 0
        jne if_13_26_239_5_end
        if_13_29_239_5_code:
            mov rdi, 1
            exit_13_38_239_5:
                    mov rax, 60
                syscall
            exit_13_38_239_5_end:
        if_13_26_239_5_end:
    assert_239_5_end:
    cmp_240_12:
        mov rcx, 8
        lea r13, [rsp - 965]
        mov r12, 0
        mov r11, 241
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 241
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 965]
        mov r12, 1
        mov r11, 242
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 242
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
    bool_end_240_12:
    assert_240_5:
        if_13_29_240_5:
        cmp_13_29_240_5:
        cmp r15b, 0
        jne if_13_26_240_5_end
        if_13_29_240_5_code:
            mov rdi, 1
            exit_13_38_240_5:
                    mov rax, 60
                syscall
            exit_13_38_240_5_end:
        if_13_26_240_5_end:
    assert_240_5_end:
    xor al, al
    lea rdi, [rsp - 1093]
    mov rcx, 128
    rep stosb
    print_247_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_24_4_247_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_24_4_247_5_end:
    print_247_5_end:
    loop_248_5:
        print_249_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_24_4_249_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_24_4_249_9_end:
        print_249_9_end:
        str_in_250_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1092]
                mov rdx, 127
            syscall
                mov byte [rsp - 1093], al
                sub byte [rsp - 1093], 1
        str_in_250_9_end:
        if_251_12:
        cmp_251_12:
        cmp byte [rsp - 1093], 0
        jne if_253_19
        if_251_12_code:
            jmp loop_248_5_end
        jmp if_251_9_end
        if_253_19:
        cmp_253_19:
        cmp byte [rsp - 1093], 4
        jg if_else_251_9
        if_253_19_code:
            print_254_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_24_4_254_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_254_13_end:
            print_254_13_end:
            jmp loop_248_5
        jmp if_251_9_end
        if_else_251_9:
            print_257_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_24_4_257_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_257_13_end:
            print_257_13_end:
            str_out_258_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1092]
                    movsx rdx, byte [rsp - 1093]
                syscall
            str_out_258_13_end:
            print_259_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_24_4_259_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_259_13_end:
            print_259_13_end:
            print_260_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_24_4_260_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_260_13_end:
            print_260_13_end:
        if_251_9_end:
    jmp loop_248_5
    loop_248_5_end:
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
db 3
times 127 db 0
db `\n`
db `.`
db `hello `
db `that is not a name.\n`
db `enter name:\n`
db `hello world from baz\n`
dat.len equ $ - dat
