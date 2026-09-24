default rel
%macro PUSH_REGS 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro POP_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    cmp_146_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_146_12:
    assert_146_5:
        if_32_26_146_5:
        cmp_32_26_146_5:
        cmp r15b, 0
        jne if_32_23_146_5_end
        if_32_26_146_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_146_5_end:
    assert_146_5_end:
    mov qword [rbp + 240], -1
    cmp_150_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_150_12:
    assert_150_5:
        if_32_26_150_5:
        cmp_32_26_150_5:
        cmp r15b, 0
        jne if_32_23_150_5_end
        if_32_26_150_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_150_5_end:
    assert_150_5_end:
        cmp_156_16:
        bool_end_156_16:
        mov r15b, 1
        assert_156_9:
            if_32_26_156_9:
            cmp_32_26_156_9:
            cmp r15b, 0
            jne if_32_23_156_9_end
            if_32_26_156_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_32_23_156_9_end:
        assert_156_9_end:
    cmp_159_12:
    bool_end_159_12:
    mov r15b, 1
    assert_159_5:
        if_32_26_159_5:
        cmp_32_26_159_5:
        cmp r15b, 0
        jne if_32_23_159_5_end
        if_32_26_159_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_159_5_end:
    assert_159_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 164
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 165
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 165
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_166_12:
    mov r14, 1
    mov r13, 166
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_32_26_166_5:
        cmp_32_26_166_5:
        cmp r15b, 0
        jne if_32_23_166_5_end
        if_32_26_166_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_166_5_end:
    assert_166_5_end:
    cmp_167_12:
    mov r14, 2
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_167_12:
    assert_167_5:
        if_32_26_167_5:
        cmp_32_26_167_5:
        cmp r15b, 0
        jne if_32_23_167_5_end
        if_32_26_167_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_167_5_end:
    assert_167_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 169
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_170_12:
    mov r14, 0
    mov r13, 170
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_32_26_170_5:
        cmp_32_26_170_5:
        cmp r15b, 0
        jne if_32_23_170_5_end
        if_32_26_170_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_170_5_end:
    assert_170_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 174
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 174
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_175_19:
        mov rcx, 3
        mov r15, 1
        mov r14, 175
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 224]
        mov r15, 1
        mov r14, 175
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 288]
    bool_end_175_19:
    cmp_178_12:
    cmp byte [rbp + 288], 0
    setne r15b
    bool_end_178_12:
    assert_178_5:
        if_32_26_178_5:
        cmp_32_26_178_5:
        cmp r15b, 0
        jne if_32_23_178_5_end
        if_32_26_178_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_178_5_end:
    assert_178_5_end:
    mov r15, 2
    mov r14, 180
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_181_12:
        mov rcx, 4
        mov r14, 181
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 181
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool_end_181_12:
    assert_181_5:
        if_32_26_181_5:
        cmp_32_26_181_5:
        cmp r15b, 0
        jne if_32_23_181_5_end
        if_32_26_181_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_181_5_end:
    assert_181_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 184
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 184
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_184_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_184_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_185_12:
    mov r14, qword [rbp + 248]
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_185_12:
    assert_185_5:
        if_32_26_185_5:
        cmp_32_26_185_5:
        cmp r15b, 0
        jne if_32_23_185_5_end
        if_32_26_185_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_185_5_end:
    assert_185_5_end:
    faz_187_5:
        mov r15, 1
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_187_5_end:
    cmp_188_12:
    mov r14, 1
    mov r13, 188
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_188_12:
    assert_188_5:
        if_32_26_188_5:
        cmp_32_26_188_5:
        cmp r15b, 0
        jne if_32_23_188_5_end
        if_32_26_188_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_188_5_end:
    assert_188_5_end:
    mov qword [rbp + 289], 3
    mov qword [rbp + 297], 5
    lea r15, [rbp + 289]
    mov qword [rbp + 313], 0
    foo_191_5:
        mov r14, qword [rbp + 313]
        add qword [r15], r14
        add qword [r15], 2
        foo_191_5_continue:
            add r15, 8
            inc qword [rbp + 313]
            cmp qword [rbp + 313], 2
            jne foo_191_5
    foo_191_5_end:
    cmp_194_12:
    mov r14, 0
    mov r13, 194
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
    sete r15b
    bool_end_194_12:
    assert_194_5:
        if_32_26_194_5:
        cmp_32_26_194_5:
        cmp r15b, 0
        jne if_32_23_194_5_end
        if_32_26_194_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_194_5_end:
    assert_194_5_end:
    cmp_195_12:
    mov r14, 1
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_32_26_195_5:
        cmp_32_26_195_5:
        cmp r15b, 0
        jne if_32_23_195_5_end
        if_32_26_195_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_195_5_end:
    assert_195_5_end:
    mov qword [rbp + 305], 0
    mov qword [rbp + 313], 0
    fooz_202_5:
        mov qword [rbp + 305], 2
        mov qword [rbp + 313], 11
    fooz_202_5_end:
    cmp_203_12:
    cmp qword [rbp + 305], 2
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_32_26_203_5:
        cmp_32_26_203_5:
        cmp r15b, 0
        jne if_32_23_203_5_end
        if_32_26_203_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_203_5_end:
    assert_203_5_end:
    cmp_204_12:
    cmp qword [rbp + 313], 11
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_32_26_204_5:
        cmp_32_26_204_5:
        cmp r15b, 0
        jne if_32_23_204_5_end
        if_32_26_204_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_204_5_end:
    assert_204_5_end:
    mov rax, qword [rbp + 305]
    mov qword [rbp + 321], rax
    mov rax, qword [rbp + 313]
    mov qword [rbp + 329], rax
    cmp_207_12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool_end_207_12:
    assert_207_5:
        if_32_26_207_5:
        cmp_32_26_207_5:
        cmp r15b, 0
        jne if_32_23_207_5_end
        if_32_26_207_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_207_5_end:
    assert_207_5_end:
    mov qword [rbp + 321], 3
    cmp_212_12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool_end_212_12:
    assert_212_5:
        if_32_26_212_5:
        cmp_32_26_212_5:
        cmp r15b, 0
        jne if_32_23_212_5_end
        if_32_26_212_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_212_5_end:
    assert_212_5_end:
    mov qword [rbp + 337], 0
    bar_215_5:
        if_51_8_215_5:
        cmp_51_8_215_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_215_5_end
        if_51_8_215_5_code:
            jmp bar_215_5_end
        if_51_5_215_5_end:
        mov qword [rbp + 337], 255
    bar_215_5_end:
    cmp_216_12:
    cmp qword [rbp + 337], 0
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_32_26_216_5:
        cmp_32_26_216_5:
        cmp r15b, 0
        jne if_32_23_216_5_end
        if_32_26_216_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_216_5_end:
    assert_216_5_end:
    mov qword [rbp + 337], 1
    bar_219_5:
        if_51_8_219_5:
        cmp_51_8_219_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_219_5_end
        if_51_8_219_5_code:
            jmp bar_219_5_end
        if_51_5_219_5_end:
        mov qword [rbp + 337], 255
    bar_219_5_end:
    cmp_220_12:
    cmp qword [rbp + 337], 255
    sete r15b
    bool_end_220_12:
    assert_220_5:
        if_32_26_220_5:
        cmp_32_26_220_5:
        cmp r15b, 0
        jne if_32_23_220_5_end
        if_32_26_220_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_220_5_end:
    assert_220_5_end:
    mov qword [rbp + 345], 1
    baz_223_13:
        mov r15, qword [rbp + 345]
        imul r15, 2
        mov qword [rbp + 353], r15
    baz_223_13_end:
    cmp_224_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_224_12:
    assert_224_5:
        if_32_26_224_5:
        cmp_32_26_224_5:
        cmp r15b, 0
        jne if_32_23_224_5_end
        if_32_26_224_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_224_5_end:
    assert_224_5_end:
    baz_226_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 353], r15
    baz_226_9_end:
    cmp_227_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_227_12:
    assert_227_5:
        if_32_26_227_5:
        cmp_32_26_227_5:
        cmp r15b, 0
        jne if_32_23_227_5_end
        if_32_26_227_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_227_5_end:
    assert_227_5_end:
    baz_229_21:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 361], r15
    baz_229_21_end:
    mov qword [rbp + 369], 0
    cmp_230_12:
    cmp qword [rbp + 361], 6
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_32_26_230_5:
        cmp_32_26_230_5:
        cmp r15b, 0
        jne if_32_23_230_5_end
        if_32_26_230_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_230_5_end:
    assert_230_5_end:
    point_init_232_20:
        mov qword [rbp + 377], -1
        mov qword [rbp + 385], -2
    point_init_232_20_end:
    cmp_233_12:
    cmp qword [rbp + 377], -1
    sete r15b
    bool_end_233_12:
    assert_233_5:
        if_32_26_233_5:
        cmp_32_26_233_5:
        cmp r15b, 0
        jne if_32_23_233_5_end
        if_32_26_233_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_233_5_end:
    assert_233_5_end:
    cmp_234_12:
    cmp qword [rbp + 385], -2
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_32_26_234_5:
        cmp_32_26_234_5:
        cmp r15b, 0
        jne if_32_23_234_5_end
        if_32_26_234_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_234_5_end:
    assert_234_5_end:
    mov qword [rbp + 393], 1
    mov qword [rbp + 401], 2
    mov r15, qword [rbp + 393]
    imul r15, 10
    mov qword [rbp + 409], r15
    mov r15, qword [rbp + 401]
    mov qword [rbp + 417], r15
    mov dword [rbp + 425], 16711680
    cmp_240_12:
    cmp qword [rbp + 409], 10
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_32_26_240_5:
        cmp_32_26_240_5:
        cmp r15b, 0
        jne if_32_23_240_5_end
        if_32_26_240_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_240_5_end:
    assert_240_5_end:
    cmp_241_12:
    cmp qword [rbp + 417], 2
    sete r15b
    bool_end_241_12:
    assert_241_5:
        if_32_26_241_5:
        cmp_32_26_241_5:
        cmp r15b, 0
        jne if_32_23_241_5_end
        if_32_26_241_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_241_5_end:
    assert_241_5_end:
    cmp_242_12:
    cmp dword [rbp + 425], 16711680
    sete r15b
    bool_end_242_12:
    assert_242_5:
        if_32_26_242_5:
        cmp_32_26_242_5:
        cmp r15b, 0
        jne if_32_23_242_5_end
        if_32_26_242_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_242_5_end:
    assert_242_5_end:
    mov r15, qword [rbp + 393]
    mov qword [rbp + 429], r15
    neg qword [rbp + 429]
    mov r15, qword [rbp + 401]
    mov qword [rbp + 437], r15
    neg qword [rbp + 437]
    mov rax, qword [rbp + 429]
    mov qword [rbp + 409], rax
    mov rax, qword [rbp + 437]
    mov qword [rbp + 417], rax
    cmp_246_12:
    cmp qword [rbp + 409], -1
    sete r15b
    bool_end_246_12:
    assert_246_5:
        if_32_26_246_5:
        cmp_32_26_246_5:
        cmp r15b, 0
        jne if_32_23_246_5_end
        if_32_26_246_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_246_5_end:
    assert_246_5_end:
    cmp_247_12:
    cmp qword [rbp + 417], -2
    sete r15b
    bool_end_247_12:
    assert_247_5:
        if_32_26_247_5:
        cmp_32_26_247_5:
        cmp r15b, 0
        jne if_32_23_247_5_end
        if_32_26_247_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_247_5_end:
    assert_247_5_end:
    lea rsi, [rbp + 409]
    lea rdi, [rbp + 445]
    mov rcx, 20
    rep movsb
    cmp_250_12:
    cmp qword [rbp + 445], -1
    sete r15b
    bool_end_250_12:
    assert_250_5:
        if_32_26_250_5:
        cmp_32_26_250_5:
        cmp r15b, 0
        jne if_32_23_250_5_end
        if_32_26_250_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_250_5_end:
    assert_250_5_end:
    cmp_251_12:
    cmp qword [rbp + 453], -2
    sete r15b
    bool_end_251_12:
    assert_251_5:
        if_32_26_251_5:
        cmp_32_26_251_5:
        cmp r15b, 0
        jne if_32_23_251_5_end
        if_32_26_251_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_251_5_end:
    assert_251_5_end:
    cmp_252_12:
    cmp dword [rbp + 461], 16711680
    sete r15b
    bool_end_252_12:
    assert_252_5:
        if_32_26_252_5:
        cmp_32_26_252_5:
        cmp r15b, 0
        jne if_32_23_252_5_end
        if_32_26_252_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_252_5_end:
    assert_252_5_end:
    xor al, al
    lea rdi, [rbp + 465]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 473], 73
    cmp_258_12:
    mov r14, 0
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    cmp qword [rbp + r14 + 473], 73
    sete r15b
    bool_end_258_12:
    assert_258_5:
        if_32_26_258_5:
        cmp_32_26_258_5:
        cmp r15b, 0
        jne if_32_23_258_5_end
        if_32_26_258_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_258_5_end:
    assert_258_5_end:
    mov r15, 1
    mov r14, 259
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
    imul r15, 20
    object_init_259_13:
        mov qword [rbp + r15 + 465], 2
        mov qword [rbp + r15 + 473], 74
        mov dword [rbp + r15 + 481], 16777215
    object_init_259_13_end:
    cmp_260_12:
    mov r14, 1
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    cmp qword [rbp + r14 + 473], 74
    sete r15b
    bool_end_260_12:
    assert_260_5:
        if_32_26_260_5:
        cmp_32_26_260_5:
        cmp r15b, 0
        jne if_32_23_260_5_end
        if_32_26_260_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_260_5_end:
    assert_260_5_end:
    xor al, al
    lea rdi, [rbp + 505]
    mov rcx, 512
    rep stosb
    mov r15, 1
    mov r14, 263
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    lea r15, [rbp + r15 + 505]
    mov r14, 1
    mov r13, 263
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_264_12:
    mov r14, 1
    mov r13, 264
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 505]
    mov r13, 1
    mov r12, 264
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_264_12:
    assert_264_5:
        if_32_26_264_5:
        cmp_32_26_264_5:
        cmp r15b, 0
        jne if_32_23_264_5_end
        if_32_26_264_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_264_5_end:
    assert_264_5_end:
    mov rcx, 8
    mov r15, 1
    mov r14, 267
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 267
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 + 505]
    mov r15, 0
    mov r14, 268
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 268
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [rbp + r15 + 505]
    shl rcx, 3
    rep movsb
    cmp_273_12:
    mov r14, 0
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 505]
    mov r13, 1
    mov r12, 273
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_32_26_273_5:
        cmp_32_26_273_5:
        cmp r15b, 0
        jne if_32_23_273_5_end
        if_32_26_273_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_273_5_end:
    assert_273_5_end:
    cmp_274_12:
        mov rcx, 8
        mov r14, 0
        mov r13, 275
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 275
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 + 505]
        mov r14, 1
        mov r13, 276
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 276
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + r14 + 505]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool_end_274_12:
    assert_274_5:
        if_32_26_274_5:
        cmp_32_26_274_5:
        cmp r15b, 0
        jne if_32_23_274_5_end
        if_32_26_274_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_274_5_end:
    assert_274_5_end:
    mov qword [rbp + 1017], -1
    mov qword [rbp + 1025], 2
    cmp_280_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_280_12:
    assert_280_5:
        if_32_26_280_5:
        cmp_32_26_280_5:
        cmp r15b, 0
        jne if_32_23_280_5_end
        if_32_26_280_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_280_5_end:
    assert_280_5_end:
    cmp_281_12:
    mov r14, 0
    mov r13, 281
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], -1
    sete r15b
    bool_end_281_12:
    assert_281_5:
        if_32_26_281_5:
        cmp_32_26_281_5:
        cmp r15b, 0
        jne if_32_23_281_5_end
        if_32_26_281_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_281_5_end:
    assert_281_5_end:
    cmp_282_12:
    mov r14, 1
    mov r13, 282
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], 2
    sete r15b
    bool_end_282_12:
    assert_282_5:
        if_32_26_282_5:
        cmp_32_26_282_5:
        cmp r15b, 0
        jne if_32_23_282_5_end
        if_32_26_282_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_282_5_end:
    assert_282_5_end:
    mov qword [rbp + 1033], 0
    xor al, al
    lea rdi, [rbp + 1041]
    mov rcx, 128
    rep stosb
    print_286_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_286_5_end:
    loop_287_5:
        add qword [rbp + 1033], 1
        lea r15, [rbp + 1033]
        mov qword [rbp + 1169], r15
        PUSH_REGS
        lea rbx, [rbp + 1169]
        call print_num
        POP_REGS
        print_290_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_290_9_end:
        print_291_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_291_9_end:
        str_in_292_9:
            mov rdi, 0
            lea rsi, [rbp + 1042]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1169], rax
            mov r15b, byte [rbp + 1169]
            mov byte [rbp + 1041], r15b
            sub byte [rbp + 1041], 1
        str_in_292_9_end:
        if_293_12:
        cmp_293_12:
        cmp byte [rbp + 1041], 0
        jne if_295_19
        if_293_12_code:
            jmp loop_287_5_end
        jmp if_293_9_end
        if_295_19:
        cmp_295_19:
        cmp byte [rbp + 1041], 4
        jg if_else_293_9
        if_295_19_code:
            print_296_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_296_13_end:
            jmp loop_287_5
        jmp if_293_9_end
        if_else_293_9:
            print_299_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_299_13_end:
            str_out_300_13:
                mov rdi, 1
                lea rsi, [rbp + 1042]
                movsx rdx, byte [rbp + 1041]
                mov rax, 1
                syscall
            str_out_300_13_end:
            print_301_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_301_13_end:
            print_302_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_302_13_end:
        if_293_9_end:
    jmp loop_287_5
    loop_287_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
    mov byte [rbx + 36], 0
    if_109_8:
    cmp_109_8:
    cmp qword [rbx + 28], 0
    jge if_109_5_end
    if_109_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_109_5_end:
    mov qword [rbx + 37], 20
    loop_115_5:
        sub qword [rbx + 37], 1
        mov qword [rbx + 45], 48
        mov r15, qword [rbx + 28]
        mov rax, r15
        cqo
        mov r14, 10
        idiv r14
        mov r15, rdx
        add qword [rbx + 45], r15
        mov r15, qword [rbx + 37]
        mov r14, 120
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 28]
        cqo
        mov r15, 10
        idiv r15
        mov qword [rbx + 28], rax
        if_122_12:
        cmp_122_12:
        cmp qword [rbx + 28], 0
        jne if_122_9_end
        if_122_12_code:
            jmp loop_115_5_end
        if_122_9_end:
    jmp loop_115_5
    loop_115_5_end:
    if_125_8:
    cmp_125_8:
    cmp byte [rbx + 36], 0
    je if_125_5_end
    if_125_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 127
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_125_5_end:
    mov qword [rbx + 45], 0
    loop_131_5:
        mov r15, qword [rbx + 45]
        mov r14, 132
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 132
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 45], 1
        add qword [rbx + 37], 1
        if_135_12:
        cmp_135_12:
        cmp qword [rbx + 37], 20
        jne if_135_9_end
        if_135_12_code:
            jmp loop_131_5_end
        if_135_9_end:
    jmp loop_131_5
    loop_131_5_end:
    mov rdi, 1
    lea rsi, [rbx + 8]
    mov rdx, qword [rbx + 45]
    mov rax, 1
    syscall
    ret
print_num.size equ 53
baz_bounds_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    mov rdi, strict qword num_buffer + 19
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
    mov rdx, strict qword num_buffer + 20
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
db `: `
dq 1
times 24 db 0
db 3
times 127 db 0
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
vars resb 131072
vars.end:
