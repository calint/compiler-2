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
    cmp_166_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_36_26_166_5:
        cmp_36_26_166_5:
        cmp r15b, 0
        jne if_36_23_166_5_end
        if_36_26_166_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_166_5_end:
    assert_166_5_end:
    mov qword [rbp + 240], -1
    cmp_170_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_36_26_170_5:
        cmp_36_26_170_5:
        cmp r15b, 0
        jne if_36_23_170_5_end
        if_36_26_170_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_170_5_end:
    assert_170_5_end:
        cmp_174_16:
        bool_end_174_16:
        mov r15b, 1
        assert_174_9:
            if_36_26_174_9:
            cmp_36_26_174_9:
            cmp r15b, 0
            jne if_36_23_174_9_end
            if_36_26_174_9_code:
                mov r14, 1
                mov rdi, r14
                mov rax, 60
                syscall
            if_36_23_174_9_end:
        assert_174_9_end:
    cmp_177_12:
    bool_end_177_12:
    mov r15b, 1
    assert_177_5:
        if_36_26_177_5:
        cmp_36_26_177_5:
        cmp r15b, 0
        jne if_36_23_177_5_end
        if_36_26_177_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_177_5_end:
    assert_177_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 183
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_184_12:
    mov r14, 1
    mov r13, 184
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_184_12:
    assert_184_5:
        if_36_26_184_5:
        cmp_36_26_184_5:
        cmp r15b, 0
        jne if_36_23_184_5_end
        if_36_26_184_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_184_5_end:
    assert_184_5_end:
    cmp_185_12:
    mov r14, 2
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
        if_36_26_185_5:
        cmp_36_26_185_5:
        cmp r15b, 0
        jne if_36_23_185_5_end
        if_36_26_185_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_185_5_end:
    assert_185_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 187
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 187
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_189_12:
    mov r14, 0
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_189_12:
    assert_189_5:
        if_36_26_189_5:
        cmp_36_26_189_5:
        cmp r15b, 0
        jne if_36_23_189_5_end
        if_36_26_189_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_189_5_end:
    assert_189_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 192
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 192
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_193_12:
        mov rcx, 4
        mov r13, 193
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 193
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_193_12:
    assert_193_5:
        if_36_26_193_5:
        cmp_36_26_193_5:
        cmp r15b, 0
        jne if_36_23_193_5_end
        if_36_26_193_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_193_5_end:
    assert_193_5_end:
    mov r15, 2
    mov r14, 196
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_197_12:
        mov rcx, 4
        mov r13, 197
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 197
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_197_12:
    assert_197_5:
        if_36_26_197_5:
        cmp_36_26_197_5:
        cmp r15b, 0
        jne if_36_23_197_5_end
        if_36_26_197_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_197_5_end:
    assert_197_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 200
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 200
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_200_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_200_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_201_12:
    mov r14, qword [rbp + 248]
    mov r13, 201
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_36_26_201_5:
        cmp_36_26_201_5:
        cmp r15b, 0
        jne if_36_23_201_5_end
        if_36_26_201_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_201_5_end:
    assert_201_5_end:
    faz_203_5:
        mov r15, 1
        mov r14, 88
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_203_5_end:
    cmp_204_12:
    mov r14, 1
    mov r13, 204
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_36_26_204_5:
        cmp_36_26_204_5:
        cmp r15b, 0
        jne if_36_23_204_5_end
        if_36_26_204_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_204_5_end:
    assert_204_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_207_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_207_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_207_5
    foo_207_5_end:
    cmp_210_12:
    mov r14, 0
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_36_26_210_5:
        cmp_36_26_210_5:
        cmp r15b, 0
        jne if_36_23_210_5_end
        if_36_26_210_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_210_5_end:
    assert_210_5_end:
    cmp_211_12:
    mov r14, 1
    mov r13, 211
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_36_26_211_5:
        cmp_36_26_211_5:
        cmp r15b, 0
        jne if_36_23_211_5_end
        if_36_26_211_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_211_5_end:
    assert_211_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_218_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_218_5_end:
    cmp_219_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_219_12:
    assert_219_5:
        if_36_26_219_5:
        cmp_36_26_219_5:
        cmp r15b, 0
        jne if_36_23_219_5_end
        if_36_26_219_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_219_5_end:
    assert_219_5_end:
    cmp_220_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_220_12:
    assert_220_5:
        if_36_26_220_5:
        cmp_36_26_220_5:
        cmp r15b, 0
        jne if_36_23_220_5_end
        if_36_26_220_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_220_5_end:
    assert_220_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_223_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_223_12:
    assert_223_5:
        if_36_26_223_5:
        cmp_36_26_223_5:
        cmp r15b, 0
        jne if_36_23_223_5_end
        if_36_26_223_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_223_5_end:
    assert_223_5_end:
    mov qword [rbp + 320], 3
    cmp_228_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_36_26_228_5:
        cmp_36_26_228_5:
        cmp r15b, 0
        jne if_36_23_228_5_end
        if_36_26_228_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_228_5_end:
    assert_228_5_end:
    mov qword [rbp + 336], 0
    bar_231_5:
        if_69_8_231_5:
        cmp_69_8_231_5:
        cmp qword [rbp + 336], 0
        jne if_69_5_231_5_end
        if_69_8_231_5_code:
            jmp bar_231_5_end
        if_69_5_231_5_end:
        mov qword [rbp + 336], 255
    bar_231_5_end:
    cmp_232_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_232_12:
    assert_232_5:
        if_36_26_232_5:
        cmp_36_26_232_5:
        cmp r15b, 0
        jne if_36_23_232_5_end
        if_36_26_232_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_232_5_end:
    assert_232_5_end:
    mov qword [rbp + 336], 1
    bar_235_5:
        if_69_8_235_5:
        cmp_69_8_235_5:
        cmp qword [rbp + 336], 0
        jne if_69_5_235_5_end
        if_69_8_235_5_code:
            jmp bar_235_5_end
        if_69_5_235_5_end:
        mov qword [rbp + 336], 255
    bar_235_5_end:
    cmp_236_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_36_26_236_5:
        cmp_36_26_236_5:
        cmp r15b, 0
        jne if_36_23_236_5_end
        if_36_26_236_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_236_5_end:
    assert_236_5_end:
    mov qword [rbp + 344], 1
    baz_239_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_239_13_end:
    cmp_240_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_36_26_240_5:
        cmp_36_26_240_5:
        cmp r15b, 0
        jne if_36_23_240_5_end
        if_36_26_240_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_240_5_end:
    assert_240_5_end:
    baz_242_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_242_9_end:
    cmp_243_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_243_12:
    assert_243_5:
        if_36_26_243_5:
        cmp_36_26_243_5:
        cmp r15b, 0
        jne if_36_23_243_5_end
        if_36_26_243_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_243_5_end:
    assert_243_5_end:
    baz_245_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_245_23_end:
    mov qword [rbp + 368], 0
    cmp_246_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_246_12:
    assert_246_5:
        if_36_26_246_5:
        cmp_36_26_246_5:
        cmp r15b, 0
        jne if_36_23_246_5_end
        if_36_26_246_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_246_5_end:
    assert_246_5_end:
    point_init_248_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_248_22_end:
    cmp_249_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_249_12:
    assert_249_5:
        if_36_26_249_5:
        cmp_36_26_249_5:
        cmp r15b, 0
        jne if_36_23_249_5_end
        if_36_26_249_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_249_5_end:
    assert_249_5_end:
    cmp_250_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_250_12:
    assert_250_5:
        if_36_26_250_5:
        cmp_36_26_250_5:
        cmp r15b, 0
        jne if_36_23_250_5_end
        if_36_26_250_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_250_5_end:
    assert_250_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_256_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_36_26_256_5:
        cmp_36_26_256_5:
        cmp r15b, 0
        jne if_36_23_256_5_end
        if_36_26_256_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_256_5_end:
    assert_256_5_end:
    cmp_257_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_257_12:
    assert_257_5:
        if_36_26_257_5:
        cmp_36_26_257_5:
        cmp r15b, 0
        jne if_36_23_257_5_end
        if_36_26_257_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_257_5_end:
    assert_257_5_end:
    cmp_258_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_258_12:
    assert_258_5:
        if_36_26_258_5:
        cmp_36_26_258_5:
        cmp r15b, 0
        jne if_36_23_258_5_end
        if_36_26_258_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_258_5_end:
    assert_258_5_end:
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
    cmp_262_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_262_12:
    assert_262_5:
        if_36_26_262_5:
        cmp_36_26_262_5:
        cmp r15b, 0
        jne if_36_23_262_5_end
        if_36_26_262_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_262_5_end:
    assert_262_5_end:
    cmp_263_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_263_12:
    assert_263_5:
        if_36_26_263_5:
        cmp_36_26_263_5:
        cmp r15b, 0
        jne if_36_23_263_5_end
        if_36_26_263_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_263_5_end:
    assert_263_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_266_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_266_12:
    assert_266_5:
        if_36_26_266_5:
        cmp_36_26_266_5:
        cmp r15b, 0
        jne if_36_23_266_5_end
        if_36_26_266_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_266_5_end:
    assert_266_5_end:
    cmp_267_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_267_12:
    assert_267_5:
        if_36_26_267_5:
        cmp_36_26_267_5:
        cmp r15b, 0
        jne if_36_23_267_5_end
        if_36_26_267_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_267_5_end:
    assert_267_5_end:
    cmp_268_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_268_12:
    assert_268_5:
        if_36_26_268_5:
        cmp_36_26_268_5:
        cmp r15b, 0
        jne if_36_23_268_5_end
        if_36_26_268_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_268_5_end:
    assert_268_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_274_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 274
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_36_26_274_5:
        cmp_36_26_274_5:
        cmp r15b, 0
        jne if_36_23_274_5_end
        if_36_26_274_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_274_5_end:
    assert_274_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_276_13:
        mov qword [r15 + 8], 74
    object_init_276_13_end:
    cmp_277_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 277
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 74
    sete r15b
    bool_end_277_12:
    assert_277_5:
        if_36_26_277_5:
        cmp_36_26_277_5:
        cmp r15b, 0
        jne if_36_23_277_5_end
        if_36_26_277_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_277_5_end:
    assert_277_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_281_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 281
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 281
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_281_12:
    assert_281_5:
        if_36_26_281_5:
        cmp_36_26_281_5:
        cmp r15b, 0
        jne if_36_23_281_5_end
        if_36_26_281_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_281_5_end:
    assert_281_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 284
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 284
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 285
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 285
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_291_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 291
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 291
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_291_12:
    assert_291_5:
        if_36_26_291_5:
        cmp_36_26_291_5:
        cmp r15b, 0
        jne if_36_23_291_5_end
        if_36_26_291_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_291_5_end:
    assert_291_5_end:
    cmp_292_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r11, 293
        test r12, r12
        cmovs rbp, r11
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r11
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 293
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r11, 294
        test r12, r12
        cmovs rbp, r11
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r11
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 294
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_292_12:
    assert_292_5:
        if_36_26_292_5:
        cmp_36_26_292_5:
        cmp r15b, 0
        jne if_36_23_292_5_end
        if_36_26_292_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_292_5_end:
    assert_292_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_299_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_299_12:
    assert_299_5:
        if_36_26_299_5:
        cmp_36_26_299_5:
        cmp r15b, 0
        jne if_36_23_299_5_end
        if_36_26_299_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_299_5_end:
    assert_299_5_end:
    cmp_300_12:
    mov r14, 0
    mov r13, 300
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_300_12:
    assert_300_5:
        if_36_26_300_5:
        cmp_36_26_300_5:
        cmp r15b, 0
        jne if_36_23_300_5_end
        if_36_26_300_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_300_5_end:
    assert_300_5_end:
    cmp_301_12:
    mov r14, 1
    mov r13, 301
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_301_12:
    assert_301_5:
        if_36_26_301_5:
        cmp_36_26_301_5:
        cmp r15b, 0
        jne if_36_23_301_5_end
        if_36_26_301_5_code:
            mov r14, 1
            mov rdi, r14
            mov rax, 60
            syscall
        if_36_23_301_5_end:
    assert_301_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_305_5:
        mov rdx, 21
        lea rsi, [rbp]
        sys_print_47_4_305_5:
                mov rax, 1
                mov rdi, 0
            push r11
            syscall
            pop r11
        sys_print_47_4_305_5_end:
    print_305_5_end:
    loop_306_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_309_9:
            mov rdx, 2
            lea rsi, [rbp + 61]
            sys_print_47_4_309_9:
                    mov rax, 1
                    mov rdi, 0
                push r11
                syscall
                pop r11
            sys_print_47_4_309_9_end:
        print_309_9_end:
        print_310_9:
            mov rdx, 12
            lea rsi, [rbp + 21]
            sys_print_47_4_310_9:
                    mov rax, 1
                    mov rdi, 0
                push r11
                syscall
                pop r11
            sys_print_47_4_310_9_end:
        print_310_9_end:
        str_in_311_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rbp + 1041]
                mov rdx, 127
            push r11
            syscall
            pop r11
                mov byte [rbp + 1040], al
                sub byte [rbp + 1040], 1
        str_in_311_9_end:
        if_312_12:
        cmp_312_12:
        cmp byte [rbp + 1040], 0
        jne if_314_19
        if_312_12_code:
            jmp loop_306_5_end
        jmp if_312_9_end
        if_314_19:
        cmp_314_19:
        cmp byte [rbp + 1040], 4
        jg if_else_312_9
        if_314_19_code:
            print_315_13:
                mov rdx, 20
                lea rsi, [rbp + 33]
                sys_print_47_4_315_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_315_13_end:
            print_315_13_end:
            jmp loop_306_5
        jmp if_312_9_end
        if_else_312_9:
            print_318_13:
                mov rdx, 6
                lea rsi, [rbp + 53]
                sys_print_47_4_318_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_318_13_end:
            print_318_13_end:
            str_out_319_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rbp + 1041]
                    movsx rdx, byte [rbp + 1040]
                push r11
                syscall
                pop r11
            str_out_319_13_end:
            print_320_13:
                mov rdx, 1
                lea rsi, [rbp + 59]
                sys_print_47_4_320_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_320_13_end:
            print_320_13_end:
            print_321_13:
                mov rdx, 1
                lea rsi, [rbp + 60]
                sys_print_47_4_321_13:
                        mov rax, 1
                        mov rdi, 0
                    push r11
                    syscall
                    pop r11
                sys_print_47_4_321_13_end:
            print_321_13_end:
        if_312_9_end:
    jmp loop_306_5
    loop_306_5_end:
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
    if_130_8:
    cmp_130_8:
    cmp qword [rbx + 28], 0
    jge if_130_5_end
    if_130_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_130_5_end:
    mov qword [rbx + 37], 20
    loop_136_5:
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
        mov r14, 140
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
        if_142_12:
        cmp_142_12:
        cmp qword [rbx + 28], 0
        jne if_142_9_end
        if_142_12_code:
            jmp loop_136_5_end
        if_142_9_end:
    jmp loop_136_5
    loop_136_5_end:
    if_145_8:
    cmp_145_8:
    cmp byte [rbx + 36], 0
    je if_145_5_end
    if_145_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 147
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_145_5_end:
    mov qword [rbx + 45], 0
    loop_151_5:
        mov r15, qword [rbx + 45]
        mov r14, 152
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 152
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
        if_155_12:
        cmp_155_12:
        cmp qword [rbx + 37], 20
        jne if_155_9_end
        if_155_12_code:
            jmp loop_151_5_end
        if_155_9_end:
    jmp loop_151_5
    loop_151_5_end:
    mov rdx, qword [rbx + 45]
    lea rsi, [rbx + 8]
    sys_print_158_5:
            mov rax, 1
            mov rdi, 0
        push r11
        syscall
        pop r11
    sys_print_158_5_end:
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
