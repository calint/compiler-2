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
    mov qword [rsp - 205], 0
    mov qword [rsp - 197], 0
    mov qword [rsp - 213], 0
    cmp_132_12:
    cmp qword [rsp - 213], 0
    sete r15b
    bool_end_132_12:
    assert_132_5:
        if_40_29_132_5:
        cmp_40_29_132_5:
        cmp r15b, 0
        jne if_40_26_132_5_end
        if_40_29_132_5_code:
            mov rdi, 1
            exit_40_38_132_5:
                    mov rax, 60
                syscall
            exit_40_38_132_5_end:
        if_40_26_132_5_end:
    assert_132_5_end:
    mov qword [rsp - 213], -1
    cmp_136_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_136_12:
    assert_136_5:
        if_40_29_136_5:
        cmp_40_29_136_5:
        cmp r15b, 0
        jne if_40_26_136_5_end
        if_40_29_136_5_code:
            mov rdi, 1
            exit_40_38_136_5:
                    mov rax, 60
                syscall
            exit_40_38_136_5_end:
        if_40_26_136_5_end:
    assert_136_5_end:
        cmp_140_16:
        bool_end_140_16:
        mov r15b, 1
        assert_140_9:
            if_40_29_140_9:
            cmp_40_29_140_9:
            cmp r15b, 0
            jne if_40_26_140_9_end
            if_40_29_140_9_code:
                mov rdi, 1
                exit_40_38_140_9:
                        mov rax, 60
                    syscall
                exit_40_38_140_9_end:
            if_40_26_140_9_end:
        assert_140_9_end:
    cmp_143_12:
    bool_end_143_12:
    mov r15b, 1
    assert_143_5:
        if_40_29_143_5:
        cmp_40_29_143_5:
        cmp r15b, 0
        jne if_40_26_143_5_end
        if_40_29_143_5_code:
            mov rdi, 1
            exit_40_38_143_5:
                    mov rax, 60
                syscall
            exit_40_38_143_5_end:
        if_40_26_143_5_end:
    assert_143_5_end:
    mov qword [rsp - 221], 1
    mov r15, qword [rsp - 221]
    mov r14, 148
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, 149
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    mov r13, 149
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_150_12:
    mov r14, 1
    mov r13, 150
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_150_12:
    assert_150_5:
        if_40_29_150_5:
        cmp_40_29_150_5:
        cmp r15b, 0
        jne if_40_26_150_5_end
        if_40_29_150_5_code:
            mov rdi, 1
            exit_40_38_150_5:
                    mov rax, 60
                syscall
            exit_40_38_150_5_end:
        if_40_26_150_5_end:
    assert_150_5_end:
    cmp_151_12:
    mov r14, 2
    mov r13, 151
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_151_12:
    assert_151_5:
        if_40_29_151_5:
        cmp_40_29_151_5:
        cmp r15b, 0
        jne if_40_26_151_5_end
        if_40_29_151_5_code:
            mov rdi, 1
            exit_40_38_151_5:
                    mov rax, 60
                syscall
            exit_40_38_151_5_end:
        if_40_26_151_5_end:
    assert_151_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 153
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 205]
    mov r15, 153
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_155_12:
    mov r14, 0
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_155_12:
    assert_155_5:
        if_40_29_155_5:
        cmp_40_29_155_5:
        cmp r15b, 0
        jne if_40_26_155_5_end
        if_40_29_155_5_code:
            mov rdi, 1
            exit_40_38_155_5:
                    mov rax, 60
                syscall
            exit_40_38_155_5_end:
        if_40_26_155_5_end:
    assert_155_5_end:
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov rcx, 4
    mov r15, 158
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 205]
    mov r15, 158
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_159_12:
        mov rcx, 4
        mov r13, 159
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 159
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
    bool_end_159_12:
    assert_159_5:
        if_40_29_159_5:
        cmp_40_29_159_5:
        cmp r15b, 0
        jne if_40_26_159_5_end
        if_40_29_159_5_code:
            mov rdi, 1
            exit_40_38_159_5:
                    mov rax, 60
                syscall
            exit_40_38_159_5_end:
        if_40_26_159_5_end:
    assert_159_5_end:
    mov r15, 2
    mov r14, 162
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 253], -1
    cmp_163_12:
        mov rcx, 4
        mov r13, 163
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 163
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
    bool_end_163_12:
    assert_163_5:
        if_40_29_163_5:
        cmp_40_29_163_5:
        cmp r15b, 0
        jne if_40_26_163_5_end
        if_40_29_163_5_code:
            mov rdi, 1
            exit_40_38_163_5:
                    mov rax, 60
                syscall
            exit_40_38_163_5_end:
        if_40_26_163_5_end:
    assert_163_5_end:
    mov qword [rsp - 221], 3
    mov r15, qword [rsp - 221]
    mov r14, 166
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    sub r14, 1
    mov r13, 166
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_166_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_166_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_167_12:
    mov r14, qword [rsp - 221]
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_167_12:
    assert_167_5:
        if_40_29_167_5:
        cmp_40_29_167_5:
        cmp r15b, 0
        jne if_40_26_167_5_end
        if_40_29_167_5_code:
            mov rdi, 1
            exit_40_38_167_5:
                    mov rax, 60
                syscall
            exit_40_38_167_5_end:
        if_40_26_167_5_end:
    assert_167_5_end:
    faz_169_5:
        mov r15, 1
        mov r14, 90
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 205], 254
    faz_169_5_end:
    cmp_170_12:
    mov r14, 1
    mov r13, 170
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 254
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_40_29_170_5:
        cmp_40_29_170_5:
        cmp r15b, 0
        jne if_40_26_170_5_end
        if_40_29_170_5_code:
            mov rdi, 1
            exit_40_38_170_5:
                    mov rax, 60
                syscall
            exit_40_38_170_5_end:
        if_40_26_170_5_end:
    assert_170_5_end:
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    foo_173_5:
        mov qword [rsp - 269], 2
        mov qword [rsp - 261], 11
    foo_173_5_end:
    cmp_174_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_174_12:
    assert_174_5:
        if_40_29_174_5:
        cmp_40_29_174_5:
        cmp r15b, 0
        jne if_40_26_174_5_end
        if_40_29_174_5_code:
            mov rdi, 1
            exit_40_38_174_5:
                    mov rax, 60
                syscall
            exit_40_38_174_5_end:
        if_40_26_174_5_end:
    assert_174_5_end:
    cmp_175_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_175_12:
    assert_175_5:
        if_40_29_175_5:
        cmp_40_29_175_5:
        cmp r15b, 0
        jne if_40_26_175_5_end
        if_40_29_175_5_code:
            mov rdi, 1
            exit_40_38_175_5:
                    mov rax, 60
                syscall
            exit_40_38_175_5_end:
        if_40_26_175_5_end:
    assert_175_5_end:
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
    cmp_178_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_178_12:
    assert_178_5:
        if_40_29_178_5:
        cmp_40_29_178_5:
        cmp r15b, 0
        jne if_40_26_178_5_end
        if_40_29_178_5_code:
            mov rdi, 1
            exit_40_38_178_5:
                    mov rax, 60
                syscall
            exit_40_38_178_5_end:
        if_40_26_178_5_end:
    assert_178_5_end:
    mov qword [rsp - 285], 3
    cmp_183_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_40_29_183_5:
        cmp_40_29_183_5:
        cmp r15b, 0
        jne if_40_26_183_5_end
        if_40_29_183_5_code:
            mov rdi, 1
            exit_40_38_183_5:
                    mov rax, 60
                syscall
            exit_40_38_183_5_end:
        if_40_26_183_5_end:
    assert_183_5_end:
    mov qword [rsp - 293], 0
    bar_186_5:
        if_73_8_186_5:
        cmp_73_8_186_5:
        cmp qword [rsp - 293], 0
        jne if_73_5_186_5_end
        if_73_8_186_5_code:
            jmp bar_186_5_end
        if_73_5_186_5_end:
        mov qword [rsp - 293], 255
    bar_186_5_end:
    cmp_187_12:
    cmp qword [rsp - 293], 0
    sete r15b
    bool_end_187_12:
    assert_187_5:
        if_40_29_187_5:
        cmp_40_29_187_5:
        cmp r15b, 0
        jne if_40_26_187_5_end
        if_40_29_187_5_code:
            mov rdi, 1
            exit_40_38_187_5:
                    mov rax, 60
                syscall
            exit_40_38_187_5_end:
        if_40_26_187_5_end:
    assert_187_5_end:
    mov qword [rsp - 293], 1
    bar_190_5:
        if_73_8_190_5:
        cmp_73_8_190_5:
        cmp qword [rsp - 293], 0
        jne if_73_5_190_5_end
        if_73_8_190_5_code:
            jmp bar_190_5_end
        if_73_5_190_5_end:
        mov qword [rsp - 293], 255
    bar_190_5_end:
    cmp_191_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_40_29_191_5:
        cmp_40_29_191_5:
        cmp r15b, 0
        jne if_40_26_191_5_end
        if_40_29_191_5_code:
            mov rdi, 1
            exit_40_38_191_5:
                    mov rax, 60
                syscall
            exit_40_38_191_5_end:
        if_40_26_191_5_end:
    assert_191_5_end:
    mov qword [rsp - 301], 1
    baz_194_13:
        mov r15, qword [rsp - 301]
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_194_13_end:
    cmp_195_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_40_29_195_5:
        cmp_40_29_195_5:
        cmp r15b, 0
        jne if_40_26_195_5_end
        if_40_29_195_5_code:
            mov rdi, 1
            exit_40_38_195_5:
                    mov rax, 60
                syscall
            exit_40_38_195_5_end:
        if_40_26_195_5_end:
    assert_195_5_end:
    baz_197_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_197_9_end:
    cmp_198_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_198_12:
    assert_198_5:
        if_40_29_198_5:
        cmp_40_29_198_5:
        cmp r15b, 0
        jne if_40_26_198_5_end
        if_40_29_198_5_code:
            mov rdi, 1
            exit_40_38_198_5:
                    mov rax, 60
                syscall
            exit_40_38_198_5_end:
        if_40_26_198_5_end:
    assert_198_5_end:
    baz_200_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 325], r15
    baz_200_23_end:
    mov qword [rsp - 317], 0
    cmp_201_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_40_29_201_5:
        cmp_40_29_201_5:
        cmp r15b, 0
        jne if_40_26_201_5_end
        if_40_29_201_5_code:
            mov rdi, 1
            exit_40_38_201_5:
                    mov rax, 60
                syscall
            exit_40_38_201_5_end:
        if_40_26_201_5_end:
    assert_201_5_end:
    point_init_203_22:
        mov qword [rsp - 341], -1
        mov qword [rsp - 333], -2
    point_init_203_22_end:
    cmp_204_12:
    cmp qword [rsp - 341], -1
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_40_29_204_5:
        cmp_40_29_204_5:
        cmp r15b, 0
        jne if_40_26_204_5_end
        if_40_29_204_5_code:
            mov rdi, 1
            exit_40_38_204_5:
                    mov rax, 60
                syscall
            exit_40_38_204_5_end:
        if_40_26_204_5_end:
    assert_204_5_end:
    cmp_205_12:
    cmp qword [rsp - 333], -2
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_40_29_205_5:
        cmp_40_29_205_5:
        cmp r15b, 0
        jne if_40_26_205_5_end
        if_40_29_205_5_code:
            mov rdi, 1
            exit_40_38_205_5:
                    mov rax, 60
                syscall
            exit_40_38_205_5_end:
        if_40_26_205_5_end:
    assert_205_5_end:
    mov qword [rsp - 349], 1
    mov qword [rsp - 357], 2
    mov r15, qword [rsp - 349]
    imul r15, 10
    mov qword [rsp - 377], r15
    mov r15, qword [rsp - 357]
    mov qword [rsp - 369], r15
    mov dword [rsp - 361], 16711680
    cmp_211_12:
    cmp qword [rsp - 377], 10
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_40_29_211_5:
        cmp_40_29_211_5:
        cmp r15b, 0
        jne if_40_26_211_5_end
        if_40_29_211_5_code:
            mov rdi, 1
            exit_40_38_211_5:
                    mov rax, 60
                syscall
            exit_40_38_211_5_end:
        if_40_26_211_5_end:
    assert_211_5_end:
    cmp_212_12:
    cmp qword [rsp - 369], 2
    sete r15b
    bool_end_212_12:
    assert_212_5:
        if_40_29_212_5:
        cmp_40_29_212_5:
        cmp r15b, 0
        jne if_40_26_212_5_end
        if_40_29_212_5_code:
            mov rdi, 1
            exit_40_38_212_5:
                    mov rax, 60
                syscall
            exit_40_38_212_5_end:
        if_40_26_212_5_end:
    assert_212_5_end:
    cmp_213_12:
    cmp dword [rsp - 361], 16711680
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_40_29_213_5:
        cmp_40_29_213_5:
        cmp r15b, 0
        jne if_40_26_213_5_end
        if_40_29_213_5_code:
            mov rdi, 1
            exit_40_38_213_5:
                    mov rax, 60
                syscall
            exit_40_38_213_5_end:
        if_40_26_213_5_end:
    assert_213_5_end:
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
    cmp_217_12:
    cmp qword [rsp - 377], -1
    sete r15b
    bool_end_217_12:
    assert_217_5:
        if_40_29_217_5:
        cmp_40_29_217_5:
        cmp r15b, 0
        jne if_40_26_217_5_end
        if_40_29_217_5_code:
            mov rdi, 1
            exit_40_38_217_5:
                    mov rax, 60
                syscall
            exit_40_38_217_5_end:
        if_40_26_217_5_end:
    assert_217_5_end:
    cmp_218_12:
    cmp qword [rsp - 369], -2
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_40_29_218_5:
        cmp_40_29_218_5:
        cmp r15b, 0
        jne if_40_26_218_5_end
        if_40_29_218_5_code:
            mov rdi, 1
            exit_40_38_218_5:
                    mov rax, 60
                syscall
            exit_40_38_218_5_end:
        if_40_26_218_5_end:
    assert_218_5_end:
    lea rsi, [rsp - 377]
    lea rdi, [rsp - 413]
    mov rcx, 20
    rep movsb
    cmp_221_12:
    cmp qword [rsp - 413], -1
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_40_29_221_5:
        cmp_40_29_221_5:
        cmp r15b, 0
        jne if_40_26_221_5_end
        if_40_29_221_5_code:
            mov rdi, 1
            exit_40_38_221_5:
                    mov rax, 60
                syscall
            exit_40_38_221_5_end:
        if_40_26_221_5_end:
    assert_221_5_end:
    cmp_222_12:
    cmp qword [rsp - 405], -2
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_40_29_222_5:
        cmp_40_29_222_5:
        cmp r15b, 0
        jne if_40_26_222_5_end
        if_40_29_222_5_code:
            mov rdi, 1
            exit_40_38_222_5:
                    mov rax, 60
                syscall
            exit_40_38_222_5_end:
        if_40_26_222_5_end:
    assert_222_5_end:
    cmp_223_12:
    cmp dword [rsp - 397], 16711680
    sete r15b
    bool_end_223_12:
    assert_223_5:
        if_40_29_223_5:
        cmp_40_29_223_5:
        cmp r15b, 0
        jne if_40_26_223_5_end
        if_40_29_223_5_code:
            mov rdi, 1
            exit_40_38_223_5:
                    mov rax, 60
                syscall
            exit_40_38_223_5_end:
        if_40_26_223_5_end:
    assert_223_5_end:
    xor al, al
    lea rdi, [rsp - 453]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 445], 73
    cmp_229_12:
    lea r14, [rsp - 453]
    mov r13, 0
    mov r12, 229
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
    bool_end_229_12:
    assert_229_5:
        if_40_29_229_5:
        cmp_40_29_229_5:
        cmp r15b, 0
        jne if_40_26_229_5_end
        if_40_29_229_5_code:
            mov rdi, 1
            exit_40_38_229_5:
                    mov rax, 60
                syscall
            exit_40_38_229_5_end:
        if_40_26_229_5_end:
    assert_229_5_end:
    lea r15, [rsp - 453]
    mov r14, 1
    mov r13, 231
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_231_13:
        mov qword [r15 + 8], 74
    object_init_231_13_end:
    cmp_232_12:
    lea r14, [rsp - 453]
    mov r13, 1
    mov r12, 232
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
    bool_end_232_12:
    assert_232_5:
        if_40_29_232_5:
        cmp_40_29_232_5:
        cmp r15b, 0
        jne if_40_26_232_5_end
        if_40_29_232_5_code:
            mov rdi, 1
            exit_40_38_232_5:
                    mov rax, 60
                syscall
            exit_40_38_232_5_end:
        if_40_26_232_5_end:
    assert_232_5_end:
    xor al, al
    lea rdi, [rsp - 965]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 235
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 235
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_236_12:
    lea r14, [rsp - 965]
    mov r13, 1
    mov r12, 236
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 236
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_40_29_236_5:
        cmp_40_29_236_5:
        cmp r15b, 0
        jne if_40_26_236_5_end
        if_40_29_236_5_code:
            mov rdi, 1
            exit_40_38_236_5:
                    mov rax, 60
                syscall
            exit_40_38_236_5_end:
        if_40_26_236_5_end:
    assert_236_5_end:
    mov rcx, 8
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 239
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 239
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 965]
    mov r14, 0
    mov r13, 240
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 240
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_246_12:
    lea r14, [rsp - 965]
    mov r13, 0
    mov r12, 246
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 246
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_246_12:
    assert_246_5:
        if_40_29_246_5:
        cmp_40_29_246_5:
        cmp r15b, 0
        jne if_40_26_246_5_end
        if_40_29_246_5_code:
            mov rdi, 1
            exit_40_38_246_5:
                    mov rax, 60
                syscall
            exit_40_38_246_5_end:
        if_40_26_246_5_end:
    assert_246_5_end:
    cmp_247_12:
        mov rcx, 8
        lea r13, [rsp - 965]
        mov r12, 0
        mov r11, 248
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 248
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 965]
        mov r12, 1
        mov r11, 249
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 249
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
    bool_end_247_12:
    assert_247_5:
        if_40_29_247_5:
        cmp_40_29_247_5:
        cmp r15b, 0
        jne if_40_26_247_5_end
        if_40_29_247_5_code:
            mov rdi, 1
            exit_40_38_247_5:
                    mov rax, 60
                syscall
            exit_40_38_247_5_end:
        if_40_26_247_5_end:
    assert_247_5_end:
    mov qword [rsp - 981], -1
    mov qword [rsp - 973], 2
    cmp_254_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_254_12:
    assert_254_5:
        if_40_29_254_5:
        cmp_40_29_254_5:
        cmp r15b, 0
        jne if_40_26_254_5_end
        if_40_29_254_5_code:
            mov rdi, 1
            exit_40_38_254_5:
                    mov rax, 60
                syscall
            exit_40_38_254_5_end:
        if_40_26_254_5_end:
    assert_254_5_end:
    cmp_255_12:
    mov r14, 0
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 981], -1
    sete r15b
    bool_end_255_12:
    assert_255_5:
        if_40_29_255_5:
        cmp_40_29_255_5:
        cmp r15b, 0
        jne if_40_26_255_5_end
        if_40_29_255_5_code:
            mov rdi, 1
            exit_40_38_255_5:
                    mov rax, 60
                syscall
            exit_40_38_255_5_end:
        if_40_26_255_5_end:
    assert_255_5_end:
    cmp_256_12:
    mov r14, 1
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 981], 2
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_40_29_256_5:
        cmp_40_29_256_5:
        cmp r15b, 0
        jne if_40_26_256_5_end
        if_40_29_256_5_code:
            mov rdi, 1
            exit_40_38_256_5:
                    mov rax, 60
                syscall
            exit_40_38_256_5_end:
        if_40_26_256_5_end:
    assert_256_5_end:
    xor al, al
    lea rdi, [rsp - 1109]
    mov rcx, 128
    rep stosb
    print_259_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_51_4_259_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_51_4_259_5_end:
    print_259_5_end:
    loop_260_5:
        print_261_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_51_4_261_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_51_4_261_9_end:
        print_261_9_end:
        str_in_262_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1108]
                mov rdx, 127
            syscall
                mov byte [rsp - 1109], al
                sub byte [rsp - 1109], 1
        str_in_262_9_end:
        if_263_12:
        cmp_263_12:
        cmp byte [rsp - 1109], 0
        jne if_265_19
        if_263_12_code:
            jmp loop_260_5_end
        jmp if_263_9_end
        if_265_19:
        cmp_265_19:
        cmp byte [rsp - 1109], 4
        jg if_else_263_9
        if_265_19_code:
            print_266_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_51_4_266_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_51_4_266_13_end:
            print_266_13_end:
            jmp loop_260_5
        jmp if_263_9_end
        if_else_263_9:
            print_269_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_51_4_269_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_51_4_269_13_end:
            print_269_13_end:
            str_out_270_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1108]
                    movsx rdx, byte [rsp - 1109]
                syscall
            str_out_270_13_end:
            print_271_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_51_4_271_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_51_4_271_13_end:
            print_271_13_end:
            print_272_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_51_4_272_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_51_4_272_13_end:
            print_272_13_end:
        if_263_9_end:
    jmp loop_260_5
    loop_260_5_end:
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
