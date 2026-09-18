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
    mov qword [rsp - 248], 0
    mov qword [rsp - 240], 0
    mov qword [rsp - 256], 0
    cmp_135_12:
    cmp qword [rsp - 256], 0
    sete r15b
    bool_end_135_12:
    assert_135_5:
        if_43_26_135_5:
        cmp_43_26_135_5:
        cmp r15b, 0
        jne if_43_23_135_5_end
        if_43_26_135_5_code:
            mov rdi, 1
            exit_43_32_135_5:
                    mov rax, 60
                syscall
            exit_43_32_135_5_end:
        if_43_23_135_5_end:
    assert_135_5_end:
    mov qword [rsp - 256], -1
    cmp_139_12:
    cmp qword [rsp - 256], -1
    sete r15b
    bool_end_139_12:
    assert_139_5:
        if_43_26_139_5:
        cmp_43_26_139_5:
        cmp r15b, 0
        jne if_43_23_139_5_end
        if_43_26_139_5_code:
            mov rdi, 1
            exit_43_32_139_5:
                    mov rax, 60
                syscall
            exit_43_32_139_5_end:
        if_43_23_139_5_end:
    assert_139_5_end:
        cmp_143_16:
        bool_end_143_16:
        mov r15b, 1
        assert_143_9:
            if_43_26_143_9:
            cmp_43_26_143_9:
            cmp r15b, 0
            jne if_43_23_143_9_end
            if_43_26_143_9_code:
                mov rdi, 1
                exit_43_32_143_9:
                        mov rax, 60
                    syscall
                exit_43_32_143_9_end:
            if_43_23_143_9_end:
        assert_143_9_end:
    cmp_146_12:
    bool_end_146_12:
    mov r15b, 1
    assert_146_5:
        if_43_26_146_5:
        cmp_43_26_146_5:
        cmp r15b, 0
        jne if_43_23_146_5_end
        if_43_26_146_5_code:
            mov rdi, 1
            exit_43_32_146_5:
                    mov rax, 60
                syscall
            exit_43_32_146_5_end:
        if_43_23_146_5_end:
    assert_146_5_end:
    mov qword [rsp - 264], 1
    mov r15, qword [rsp - 264]
    mov r14, 151
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 248], 2
    mov r15, qword [rsp - 264]
    add r15, 1
    mov r14, 152
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 264]
    mov r13, 152
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 248]
    mov dword [rsp + r15 * 4 - 248], r13d
    cmp_153_12:
    mov r14, 1
    mov r13, 153
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 248], 2
    sete r15b
    bool_end_153_12:
    assert_153_5:
        if_43_26_153_5:
        cmp_43_26_153_5:
        cmp r15b, 0
        jne if_43_23_153_5_end
        if_43_26_153_5_code:
            mov rdi, 1
            exit_43_32_153_5:
                    mov rax, 60
                syscall
            exit_43_32_153_5_end:
        if_43_23_153_5_end:
    assert_153_5_end:
    cmp_154_12:
    mov r14, 2
    mov r13, 154
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 248], 2
    sete r15b
    bool_end_154_12:
    assert_154_5:
        if_43_26_154_5:
        cmp_43_26_154_5:
        cmp r15b, 0
        jne if_43_23_154_5_end
        if_43_26_154_5_code:
            mov rdi, 1
            exit_43_32_154_5:
                    mov rax, 60
                syscall
            exit_43_32_154_5_end:
        if_43_23_154_5_end:
    assert_154_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 156
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 248]
    mov r15, 156
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 248]
    shl rcx, 2
    rep movsb
    cmp_158_12:
    mov r14, 0
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 248], 2
    sete r15b
    bool_end_158_12:
    assert_158_5:
        if_43_26_158_5:
        cmp_43_26_158_5:
        cmp r15b, 0
        jne if_43_23_158_5_end
        if_43_26_158_5_code:
            mov rdi, 1
            exit_43_32_158_5:
                    mov rax, 60
                syscall
            exit_43_32_158_5_end:
        if_43_23_158_5_end:
    assert_158_5_end:
    mov qword [rsp - 296], 0
    mov qword [rsp - 288], 0
    mov qword [rsp - 280], 0
    mov qword [rsp - 272], 0
    mov rcx, 4
    mov r15, 161
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 248]
    mov r15, 161
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 296]
    shl rcx, 2
    rep movsb
    cmp_162_12:
        mov rcx, 4
        mov r13, 162
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 248]
        mov r13, 162
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 296]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_162_12:
    assert_162_5:
        if_43_26_162_5:
        cmp_43_26_162_5:
        cmp r15b, 0
        jne if_43_23_162_5_end
        if_43_26_162_5_code:
            mov rdi, 1
            exit_43_32_162_5:
                    mov rax, 60
                syscall
            exit_43_32_162_5_end:
        if_43_23_162_5_end:
    assert_162_5_end:
    mov r15, 2
    mov r14, 165
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 296], -1
    cmp_166_12:
        mov rcx, 4
        mov r13, 166
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 248]
        mov r13, 166
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 296]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_43_26_166_5:
        cmp_43_26_166_5:
        cmp r15b, 0
        jne if_43_23_166_5_end
        if_43_26_166_5_code:
            mov rdi, 1
            exit_43_32_166_5:
                    mov rax, 60
                syscall
            exit_43_32_166_5_end:
        if_43_23_166_5_end:
    assert_166_5_end:
    mov qword [rsp - 264], 3
    mov r15, qword [rsp - 264]
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 264]
    sub r14, 1
    mov r13, 169
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_169_16:
        mov r13d, dword [rsp + r14 * 4 - 248]
        mov dword [rsp + r15 * 4 - 248], r13d
        not dword [rsp + r15 * 4 - 248]
    inv_169_16_end:
    not dword [rsp + r15 * 4 - 248]
    cmp_170_12:
    mov r14, qword [rsp - 264]
    mov r13, 170
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 248], 2
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_43_26_170_5:
        cmp_43_26_170_5:
        cmp r15b, 0
        jne if_43_23_170_5_end
        if_43_26_170_5_code:
            mov rdi, 1
            exit_43_32_170_5:
                    mov rax, 60
                syscall
            exit_43_32_170_5_end:
        if_43_23_170_5_end:
    assert_170_5_end:
    faz_172_5:
        mov r15, 1
        mov r14, 95
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 248], 254
    faz_172_5_end:
    cmp_173_12:
    mov r14, 1
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 248], 254
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_43_26_173_5:
        cmp_43_26_173_5:
        cmp r15b, 0
        jne if_43_23_173_5_end
        if_43_26_173_5_code:
            mov rdi, 1
            exit_43_32_173_5:
                    mov rax, 60
                syscall
            exit_43_32_173_5_end:
        if_43_23_173_5_end:
    assert_173_5_end:
    mov qword [rsp - 312], 3
    mov qword [rsp - 304], 5
    lea r15, [rsp - 312]
    mov qword [rsp - 328], 0
    foo_176_5:
        mov r14, qword [rsp - 328]
        add qword [r15], r14
        add qword [r15], 2
        foo_176_5_continue:
            add r15, 8
            inc qword [rsp - 328]
            cmp qword [rsp - 328], 2
            jne foo_176_5
    foo_176_5_end:
    cmp_179_12:
    mov r14, 0
    mov r13, 179
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rsp + r14 * 8 - 312], r13
    sete r15b
    bool_end_179_12:
    assert_179_5:
        if_43_26_179_5:
        cmp_43_26_179_5:
        cmp r15b, 0
        jne if_43_23_179_5_end
        if_43_26_179_5_code:
            mov rdi, 1
            exit_43_32_179_5:
                    mov rax, 60
                syscall
            exit_43_32_179_5_end:
        if_43_23_179_5_end:
    assert_179_5_end:
    cmp_180_12:
    mov r14, 1
    mov r13, 180
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rsp + r14 * 8 - 312], r13
    sete r15b
    bool_end_180_12:
    assert_180_5:
        if_43_26_180_5:
        cmp_43_26_180_5:
        cmp r15b, 0
        jne if_43_23_180_5_end
        if_43_26_180_5_code:
            mov rdi, 1
            exit_43_32_180_5:
                    mov rax, 60
                syscall
            exit_43_32_180_5_end:
        if_43_23_180_5_end:
    assert_180_5_end:
    mov qword [rsp - 328], 0
    mov qword [rsp - 320], 0
    fooz_187_5:
        mov qword [rsp - 328], 2
        mov qword [rsp - 320], 11
    fooz_187_5_end:
    cmp_188_12:
    cmp qword [rsp - 328], 2
    sete r15b
    bool_end_188_12:
    assert_188_5:
        if_43_26_188_5:
        cmp_43_26_188_5:
        cmp r15b, 0
        jne if_43_23_188_5_end
        if_43_26_188_5_code:
            mov rdi, 1
            exit_43_32_188_5:
                    mov rax, 60
                syscall
            exit_43_32_188_5_end:
        if_43_23_188_5_end:
    assert_188_5_end:
    cmp_189_12:
    cmp qword [rsp - 320], 11
    sete r15b
    bool_end_189_12:
    assert_189_5:
        if_43_26_189_5:
        cmp_43_26_189_5:
        cmp r15b, 0
        jne if_43_23_189_5_end
        if_43_26_189_5_code:
            mov rdi, 1
            exit_43_32_189_5:
                    mov rax, 60
                syscall
            exit_43_32_189_5_end:
        if_43_23_189_5_end:
    assert_189_5_end:
    mov rax, qword [rsp - 328]
    mov qword [rsp - 344], rax
    mov rax, qword [rsp - 320]
    mov qword [rsp - 336], rax
    cmp_192_12:
        lea rsi, [rsp - 328]
        lea rdi, [rsp - 344]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_192_12:
    assert_192_5:
        if_43_26_192_5:
        cmp_43_26_192_5:
        cmp r15b, 0
        jne if_43_23_192_5_end
        if_43_26_192_5_code:
            mov rdi, 1
            exit_43_32_192_5:
                    mov rax, 60
                syscall
            exit_43_32_192_5_end:
        if_43_23_192_5_end:
    assert_192_5_end:
    mov qword [rsp - 344], 3
    cmp_197_12:
        lea rsi, [rsp - 328]
        lea rdi, [rsp - 344]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_197_12:
    assert_197_5:
        if_43_26_197_5:
        cmp_43_26_197_5:
        cmp r15b, 0
        jne if_43_23_197_5_end
        if_43_26_197_5_code:
            mov rdi, 1
            exit_43_32_197_5:
                    mov rax, 60
                syscall
            exit_43_32_197_5_end:
        if_43_23_197_5_end:
    assert_197_5_end:
    mov qword [rsp - 352], 0
    bar_200_5:
        if_76_8_200_5:
        cmp_76_8_200_5:
        cmp qword [rsp - 352], 0
        jne if_76_5_200_5_end
        if_76_8_200_5_code:
            jmp bar_200_5_end
        if_76_5_200_5_end:
        mov qword [rsp - 352], 255
    bar_200_5_end:
    cmp_201_12:
    cmp qword [rsp - 352], 0
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_43_26_201_5:
        cmp_43_26_201_5:
        cmp r15b, 0
        jne if_43_23_201_5_end
        if_43_26_201_5_code:
            mov rdi, 1
            exit_43_32_201_5:
                    mov rax, 60
                syscall
            exit_43_32_201_5_end:
        if_43_23_201_5_end:
    assert_201_5_end:
    mov qword [rsp - 352], 1
    bar_204_5:
        if_76_8_204_5:
        cmp_76_8_204_5:
        cmp qword [rsp - 352], 0
        jne if_76_5_204_5_end
        if_76_8_204_5_code:
            jmp bar_204_5_end
        if_76_5_204_5_end:
        mov qword [rsp - 352], 255
    bar_204_5_end:
    cmp_205_12:
    cmp qword [rsp - 352], 255
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_43_26_205_5:
        cmp_43_26_205_5:
        cmp r15b, 0
        jne if_43_23_205_5_end
        if_43_26_205_5_code:
            mov rdi, 1
            exit_43_32_205_5:
                    mov rax, 60
                syscall
            exit_43_32_205_5_end:
        if_43_23_205_5_end:
    assert_205_5_end:
    mov qword [rsp - 360], 1
    baz_208_13:
        mov r15, qword [rsp - 360]
        imul r15, 2
        mov qword [rsp - 368], r15
    baz_208_13_end:
    cmp_209_12:
    cmp qword [rsp - 368], 2
    sete r15b
    bool_end_209_12:
    assert_209_5:
        if_43_26_209_5:
        cmp_43_26_209_5:
        cmp r15b, 0
        jne if_43_23_209_5_end
        if_43_26_209_5_code:
            mov rdi, 1
            exit_43_32_209_5:
                    mov rax, 60
                syscall
            exit_43_32_209_5_end:
        if_43_23_209_5_end:
    assert_209_5_end:
    baz_211_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 368], r15
    baz_211_9_end:
    cmp_212_12:
    cmp qword [rsp - 368], 2
    sete r15b
    bool_end_212_12:
    assert_212_5:
        if_43_26_212_5:
        cmp_43_26_212_5:
        cmp r15b, 0
        jne if_43_23_212_5_end
        if_43_26_212_5_code:
            mov rdi, 1
            exit_43_32_212_5:
                    mov rax, 60
                syscall
            exit_43_32_212_5_end:
        if_43_23_212_5_end:
    assert_212_5_end:
    baz_214_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 384], r15
    baz_214_23_end:
    mov qword [rsp - 376], 0
    cmp_215_12:
    cmp qword [rsp - 384], 4
    sete r15b
    bool_end_215_12:
    assert_215_5:
        if_43_26_215_5:
        cmp_43_26_215_5:
        cmp r15b, 0
        jne if_43_23_215_5_end
        if_43_26_215_5_code:
            mov rdi, 1
            exit_43_32_215_5:
                    mov rax, 60
                syscall
            exit_43_32_215_5_end:
        if_43_23_215_5_end:
    assert_215_5_end:
    point_init_217_22:
        mov qword [rsp - 400], -1
        mov qword [rsp - 392], -2
    point_init_217_22_end:
    cmp_218_12:
    cmp qword [rsp - 400], -1
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_43_26_218_5:
        cmp_43_26_218_5:
        cmp r15b, 0
        jne if_43_23_218_5_end
        if_43_26_218_5_code:
            mov rdi, 1
            exit_43_32_218_5:
                    mov rax, 60
                syscall
            exit_43_32_218_5_end:
        if_43_23_218_5_end:
    assert_218_5_end:
    cmp_219_12:
    cmp qword [rsp - 392], -2
    sete r15b
    bool_end_219_12:
    assert_219_5:
        if_43_26_219_5:
        cmp_43_26_219_5:
        cmp r15b, 0
        jne if_43_23_219_5_end
        if_43_26_219_5_code:
            mov rdi, 1
            exit_43_32_219_5:
                    mov rax, 60
                syscall
            exit_43_32_219_5_end:
        if_43_23_219_5_end:
    assert_219_5_end:
    mov qword [rsp - 408], 1
    mov qword [rsp - 416], 2
    mov r15, qword [rsp - 408]
    imul r15, 10
    mov qword [rsp - 436], r15
    mov r15, qword [rsp - 416]
    mov qword [rsp - 428], r15
    mov dword [rsp - 420], 16711680
    cmp_225_12:
    cmp qword [rsp - 436], 10
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_43_26_225_5:
        cmp_43_26_225_5:
        cmp r15b, 0
        jne if_43_23_225_5_end
        if_43_26_225_5_code:
            mov rdi, 1
            exit_43_32_225_5:
                    mov rax, 60
                syscall
            exit_43_32_225_5_end:
        if_43_23_225_5_end:
    assert_225_5_end:
    cmp_226_12:
    cmp qword [rsp - 428], 2
    sete r15b
    bool_end_226_12:
    assert_226_5:
        if_43_26_226_5:
        cmp_43_26_226_5:
        cmp r15b, 0
        jne if_43_23_226_5_end
        if_43_26_226_5_code:
            mov rdi, 1
            exit_43_32_226_5:
                    mov rax, 60
                syscall
            exit_43_32_226_5_end:
        if_43_23_226_5_end:
    assert_226_5_end:
    cmp_227_12:
    cmp dword [rsp - 420], 16711680
    sete r15b
    bool_end_227_12:
    assert_227_5:
        if_43_26_227_5:
        cmp_43_26_227_5:
        cmp r15b, 0
        jne if_43_23_227_5_end
        if_43_26_227_5_code:
            mov rdi, 1
            exit_43_32_227_5:
                    mov rax, 60
                syscall
            exit_43_32_227_5_end:
        if_43_23_227_5_end:
    assert_227_5_end:
    mov r15, qword [rsp - 408]
    mov qword [rsp - 452], r15
    neg qword [rsp - 452]
    mov r15, qword [rsp - 416]
    mov qword [rsp - 444], r15
    neg qword [rsp - 444]
    mov rax, qword [rsp - 452]
    mov qword [rsp - 436], rax
    mov rax, qword [rsp - 444]
    mov qword [rsp - 428], rax
    cmp_231_12:
    cmp qword [rsp - 436], -1
    sete r15b
    bool_end_231_12:
    assert_231_5:
        if_43_26_231_5:
        cmp_43_26_231_5:
        cmp r15b, 0
        jne if_43_23_231_5_end
        if_43_26_231_5_code:
            mov rdi, 1
            exit_43_32_231_5:
                    mov rax, 60
                syscall
            exit_43_32_231_5_end:
        if_43_23_231_5_end:
    assert_231_5_end:
    cmp_232_12:
    cmp qword [rsp - 428], -2
    sete r15b
    bool_end_232_12:
    assert_232_5:
        if_43_26_232_5:
        cmp_43_26_232_5:
        cmp r15b, 0
        jne if_43_23_232_5_end
        if_43_26_232_5_code:
            mov rdi, 1
            exit_43_32_232_5:
                    mov rax, 60
                syscall
            exit_43_32_232_5_end:
        if_43_23_232_5_end:
    assert_232_5_end:
    lea rsi, [rsp - 436]
    lea rdi, [rsp - 472]
    mov rcx, 20
    rep movsb
    cmp_235_12:
    cmp qword [rsp - 472], -1
    sete r15b
    bool_end_235_12:
    assert_235_5:
        if_43_26_235_5:
        cmp_43_26_235_5:
        cmp r15b, 0
        jne if_43_23_235_5_end
        if_43_26_235_5_code:
            mov rdi, 1
            exit_43_32_235_5:
                    mov rax, 60
                syscall
            exit_43_32_235_5_end:
        if_43_23_235_5_end:
    assert_235_5_end:
    cmp_236_12:
    cmp qword [rsp - 464], -2
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_43_26_236_5:
        cmp_43_26_236_5:
        cmp r15b, 0
        jne if_43_23_236_5_end
        if_43_26_236_5_code:
            mov rdi, 1
            exit_43_32_236_5:
                    mov rax, 60
                syscall
            exit_43_32_236_5_end:
        if_43_23_236_5_end:
    assert_236_5_end:
    cmp_237_12:
    cmp dword [rsp - 456], 16711680
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_43_26_237_5:
        cmp_43_26_237_5:
        cmp r15b, 0
        jne if_43_23_237_5_end
        if_43_26_237_5_code:
            mov rdi, 1
            exit_43_32_237_5:
                    mov rax, 60
                syscall
            exit_43_32_237_5_end:
        if_43_23_237_5_end:
    assert_237_5_end:
    xor al, al
    lea rdi, [rsp - 512]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 504], 73
    cmp_243_12:
    lea r14, [rsp - 512]
    mov r13, 0
    mov r12, 243
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
    bool_end_243_12:
    assert_243_5:
        if_43_26_243_5:
        cmp_43_26_243_5:
        cmp r15b, 0
        jne if_43_23_243_5_end
        if_43_26_243_5_code:
            mov rdi, 1
            exit_43_32_243_5:
                    mov rax, 60
                syscall
            exit_43_32_243_5_end:
        if_43_23_243_5_end:
    assert_243_5_end:
    lea r15, [rsp - 512]
    mov r14, 1
    mov r13, 245
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_245_13:
        mov qword [r15 + 8], 74
    object_init_245_13_end:
    cmp_246_12:
    lea r14, [rsp - 512]
    mov r13, 1
    mov r12, 246
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
    bool_end_246_12:
    assert_246_5:
        if_43_26_246_5:
        cmp_43_26_246_5:
        cmp r15b, 0
        jne if_43_23_246_5_end
        if_43_26_246_5_code:
            mov rdi, 1
            exit_43_32_246_5:
                    mov rax, 60
                syscall
            exit_43_32_246_5_end:
        if_43_23_246_5_end:
    assert_246_5_end:
    xor al, al
    lea rdi, [rsp - 1024]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 1024]
    mov r14, 1
    mov r13, 249
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 249
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_250_12:
    lea r14, [rsp - 1024]
    mov r13, 1
    mov r12, 250
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 250
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_250_12:
    assert_250_5:
        if_43_26_250_5:
        cmp_43_26_250_5:
        cmp r15b, 0
        jne if_43_23_250_5_end
        if_43_26_250_5_code:
            mov rdi, 1
            exit_43_32_250_5:
                    mov rax, 60
                syscall
            exit_43_32_250_5_end:
        if_43_23_250_5_end:
    assert_250_5_end:
    mov rcx, 8
    lea r15, [rsp - 1024]
    mov r14, 1
    mov r13, 253
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 253
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 1024]
    mov r14, 0
    mov r13, 254
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 254
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_260_12:
    lea r14, [rsp - 1024]
    mov r13, 0
    mov r12, 260
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 260
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_260_12:
    assert_260_5:
        if_43_26_260_5:
        cmp_43_26_260_5:
        cmp r15b, 0
        jne if_43_23_260_5_end
        if_43_26_260_5_code:
            mov rdi, 1
            exit_43_32_260_5:
                    mov rax, 60
                syscall
            exit_43_32_260_5_end:
        if_43_23_260_5_end:
    assert_260_5_end:
    cmp_261_12:
        mov rcx, 8
        lea r13, [rsp - 1024]
        mov r12, 0
        mov r11, 262
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 262
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 1024]
        mov r12, 1
        mov r11, 263
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 263
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
    bool_end_261_12:
    assert_261_5:
        if_43_26_261_5:
        cmp_43_26_261_5:
        cmp r15b, 0
        jne if_43_23_261_5_end
        if_43_26_261_5_code:
            mov rdi, 1
            exit_43_32_261_5:
                    mov rax, 60
                syscall
            exit_43_32_261_5_end:
        if_43_23_261_5_end:
    assert_261_5_end:
    mov qword [rsp - 1040], -1
    mov qword [rsp - 1032], 2
    cmp_268_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_268_12:
    assert_268_5:
        if_43_26_268_5:
        cmp_43_26_268_5:
        cmp r15b, 0
        jne if_43_23_268_5_end
        if_43_26_268_5_code:
            mov rdi, 1
            exit_43_32_268_5:
                    mov rax, 60
                syscall
            exit_43_32_268_5_end:
        if_43_23_268_5_end:
    assert_268_5_end:
    cmp_269_12:
    mov r14, 0
    mov r13, 269
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1040], -1
    sete r15b
    bool_end_269_12:
    assert_269_5:
        if_43_26_269_5:
        cmp_43_26_269_5:
        cmp r15b, 0
        jne if_43_23_269_5_end
        if_43_26_269_5_code:
            mov rdi, 1
            exit_43_32_269_5:
                    mov rax, 60
                syscall
            exit_43_32_269_5_end:
        if_43_23_269_5_end:
    assert_269_5_end:
    cmp_270_12:
    mov r14, 1
    mov r13, 270
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1040], 2
    sete r15b
    bool_end_270_12:
    assert_270_5:
        if_43_26_270_5:
        cmp_43_26_270_5:
        cmp r15b, 0
        jne if_43_23_270_5_end
        if_43_26_270_5_code:
            mov rdi, 1
            exit_43_32_270_5:
                    mov rax, 60
                syscall
            exit_43_32_270_5_end:
        if_43_23_270_5_end:
    assert_270_5_end:
    xor al, al
    lea rdi, [rsp - 1168]
    mov rcx, 128
    rep stosb
    print_273_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_54_4_273_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_54_4_273_5_end:
    print_273_5_end:
    loop_274_5:
        print_275_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_54_4_275_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_54_4_275_9_end:
        print_275_9_end:
        str_in_276_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1167]
                mov rdx, 127
            syscall
                mov byte [rsp - 1168], al
                sub byte [rsp - 1168], 1
        str_in_276_9_end:
        if_277_12:
        cmp_277_12:
        cmp byte [rsp - 1168], 0
        jne if_279_19
        if_277_12_code:
            jmp loop_274_5_end
        jmp if_277_9_end
        if_279_19:
        cmp_279_19:
        cmp byte [rsp - 1168], 4
        jg if_else_277_9
        if_279_19_code:
            print_280_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_54_4_280_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_54_4_280_13_end:
            print_280_13_end:
            jmp loop_274_5
        jmp if_277_9_end
        if_else_277_9:
            print_283_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_54_4_283_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_54_4_283_13_end:
            print_283_13_end:
            str_out_284_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1167]
                    movsx rdx, byte [rsp - 1168]
                syscall
            str_out_284_13_end:
            print_285_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_54_4_285_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_54_4_285_13_end:
            print_285_13_end:
            print_286_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_54_4_286_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_54_4_286_13_end:
            print_286_13_end:
        if_277_9_end:
    jmp loop_274_5
    loop_274_5_end:
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
db 1
db 2
times 1 db 0
dq 1
db 3
times 127 db 0
dq 1
times 24 db 0
db `\n`
db `.`
db `hello `
db `that is not a name.\n`
db `enter name:\n`
db `hello world from baz\n`
dat.len equ $ - dat
