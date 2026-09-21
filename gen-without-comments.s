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
    cmp_172_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_172_12:
    assert_172_5:
        if_42_26_172_5:
        cmp_42_26_172_5:
        cmp r15b, 0
        jne if_42_23_172_5_end
        if_42_26_172_5_code:
            mov rdi, 1
            exit_42_32_172_5:
                    mov rax, 60
                syscall
            exit_42_32_172_5_end:
        if_42_23_172_5_end:
    assert_172_5_end:
    mov qword [rbp + 240], -1
    cmp_176_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_42_26_176_5:
        cmp_42_26_176_5:
        cmp r15b, 0
        jne if_42_23_176_5_end
        if_42_26_176_5_code:
            mov rdi, 1
            exit_42_32_176_5:
                    mov rax, 60
                syscall
            exit_42_32_176_5_end:
        if_42_23_176_5_end:
    assert_176_5_end:
        cmp_180_16:
        bool_end_180_16:
        mov r15b, 1
        assert_180_9:
            if_42_26_180_9:
            cmp_42_26_180_9:
            cmp r15b, 0
            jne if_42_23_180_9_end
            if_42_26_180_9_code:
                mov rdi, 1
                exit_42_32_180_9:
                        mov rax, 60
                    syscall
                exit_42_32_180_9_end:
            if_42_23_180_9_end:
        assert_180_9_end:
    cmp_183_12:
    bool_end_183_12:
    mov r15b, 1
    assert_183_5:
        if_42_26_183_5:
        cmp_42_26_183_5:
        cmp r15b, 0
        jne if_42_23_183_5_end
        if_42_26_183_5_code:
            mov rdi, 1
            exit_42_32_183_5:
                    mov rax, 60
                syscall
            exit_42_32_183_5_end:
        if_42_23_183_5_end:
    assert_183_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 188
    test r15, r15
    cmovs rbp, r14
    js baz_panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge baz_panic_bounds
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 189
    test r15, r15
    cmovs rbp, r14
    js baz_panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge baz_panic_bounds
    mov r14, qword [rbp + 248]
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge baz_panic_bounds
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_190_12:
    mov r14, 1
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge baz_panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_190_12:
    assert_190_5:
        if_42_26_190_5:
        cmp_42_26_190_5:
        cmp r15b, 0
        jne if_42_23_190_5_end
        if_42_26_190_5_code:
            mov rdi, 1
            exit_42_32_190_5:
                    mov rax, 60
                syscall
            exit_42_32_190_5_end:
        if_42_23_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    mov r14, 2
    mov r13, 191
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge baz_panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_42_26_191_5:
        cmp_42_26_191_5:
        cmp r15b, 0
        jne if_42_23_191_5_end
        if_42_26_191_5_code:
            mov rdi, 1
            exit_42_32_191_5:
                    mov rax, 60
                syscall
            exit_42_32_191_5_end:
        if_42_23_191_5_end:
    assert_191_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 193
    test r15, r15
    cmovs rbp, r14
    js baz_panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_panic_bounds
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 193
    test rcx, rcx
    cmovs rbp, r15
    js baz_panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_panic_bounds
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_195_12:
    mov r14, 0
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge baz_panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_42_26_195_5:
        cmp_42_26_195_5:
        cmp r15b, 0
        jne if_42_23_195_5_end
        if_42_26_195_5_code:
            mov rdi, 1
            exit_42_32_195_5:
                    mov rax, 60
                syscall
            exit_42_32_195_5_end:
        if_42_23_195_5_end:
    assert_195_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 198
    test rcx, rcx
    cmovs rbp, r15
    js baz_panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_panic_bounds
    lea rsi, [rbp + 224]
    mov r15, 198
    test rcx, rcx
    cmovs rbp, r15
    js baz_panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_panic_bounds
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_199_12:
        mov rcx, 4
        mov r13, 199
        test rcx, rcx
        cmovs rbp, r13
        js baz_panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_panic_bounds
        lea rsi, [rbp + 224]
        mov r13, 199
        test rcx, rcx
        cmovs rbp, r13
        js baz_panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_panic_bounds
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_199_12:
    assert_199_5:
        if_42_26_199_5:
        cmp_42_26_199_5:
        cmp r15b, 0
        jne if_42_23_199_5_end
        if_42_26_199_5_code:
            mov rdi, 1
            exit_42_32_199_5:
                    mov rax, 60
                syscall
            exit_42_32_199_5_end:
        if_42_23_199_5_end:
    assert_199_5_end:
    mov r15, 2
    mov r14, 202
    test r15, r15
    cmovs rbp, r14
    js baz_panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge baz_panic_bounds
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_203_12:
        mov rcx, 4
        mov r13, 203
        test rcx, rcx
        cmovs rbp, r13
        js baz_panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_panic_bounds
        lea rsi, [rbp + 224]
        mov r13, 203
        test rcx, rcx
        cmovs rbp, r13
        js baz_panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_panic_bounds
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_42_26_203_5:
        cmp_42_26_203_5:
        cmp r15b, 0
        jne if_42_23_203_5_end
        if_42_26_203_5_code:
            mov rdi, 1
            exit_42_32_203_5:
                    mov rax, 60
                syscall
            exit_42_32_203_5_end:
        if_42_23_203_5_end:
    assert_203_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 206
    test r15, r15
    cmovs rbp, r14
    js baz_panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge baz_panic_bounds
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 206
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge baz_panic_bounds
    inv_206_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_206_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_207_12:
    mov r14, qword [rbp + 248]
    mov r13, 207
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge baz_panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_42_26_207_5:
        cmp_42_26_207_5:
        cmp r15b, 0
        jne if_42_23_207_5_end
        if_42_26_207_5_code:
            mov rdi, 1
            exit_42_32_207_5:
                    mov rax, 60
                syscall
            exit_42_32_207_5_end:
        if_42_23_207_5_end:
    assert_207_5_end:
    faz_209_5:
        mov r15, 1
        mov r14, 94
        test r15, r15
        cmovs rbp, r14
        js baz_panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge baz_panic_bounds
        mov dword [rbp + r15 * 4 + 224], 254
    faz_209_5_end:
    cmp_210_12:
    mov r14, 1
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge baz_panic_bounds
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_42_26_210_5:
        cmp_42_26_210_5:
        cmp r15b, 0
        jne if_42_23_210_5_end
        if_42_26_210_5_code:
            mov rdi, 1
            exit_42_32_210_5:
                    mov rax, 60
                syscall
            exit_42_32_210_5_end:
        if_42_23_210_5_end:
    assert_210_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_213_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_213_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_213_5
    foo_213_5_end:
    cmp_216_12:
    mov r14, 0
    mov r13, 216
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge baz_panic_bounds
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_42_26_216_5:
        cmp_42_26_216_5:
        cmp r15b, 0
        jne if_42_23_216_5_end
        if_42_26_216_5_code:
            mov rdi, 1
            exit_42_32_216_5:
                    mov rax, 60
                syscall
            exit_42_32_216_5_end:
        if_42_23_216_5_end:
    assert_216_5_end:
    cmp_217_12:
    mov r14, 1
    mov r13, 217
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge baz_panic_bounds
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_217_12:
    assert_217_5:
        if_42_26_217_5:
        cmp_42_26_217_5:
        cmp r15b, 0
        jne if_42_23_217_5_end
        if_42_26_217_5_code:
            mov rdi, 1
            exit_42_32_217_5:
                    mov rax, 60
                syscall
            exit_42_32_217_5_end:
        if_42_23_217_5_end:
    assert_217_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_224_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_224_5_end:
    cmp_225_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_42_26_225_5:
        cmp_42_26_225_5:
        cmp r15b, 0
        jne if_42_23_225_5_end
        if_42_26_225_5_code:
            mov rdi, 1
            exit_42_32_225_5:
                    mov rax, 60
                syscall
            exit_42_32_225_5_end:
        if_42_23_225_5_end:
    assert_225_5_end:
    cmp_226_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_226_12:
    assert_226_5:
        if_42_26_226_5:
        cmp_42_26_226_5:
        cmp r15b, 0
        jne if_42_23_226_5_end
        if_42_26_226_5_code:
            mov rdi, 1
            exit_42_32_226_5:
                    mov rax, 60
                syscall
            exit_42_32_226_5_end:
        if_42_23_226_5_end:
    assert_226_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_229_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_229_12:
    assert_229_5:
        if_42_26_229_5:
        cmp_42_26_229_5:
        cmp r15b, 0
        jne if_42_23_229_5_end
        if_42_26_229_5_code:
            mov rdi, 1
            exit_42_32_229_5:
                    mov rax, 60
                syscall
            exit_42_32_229_5_end:
        if_42_23_229_5_end:
    assert_229_5_end:
    mov qword [rbp + 320], 3
    cmp_234_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_42_26_234_5:
        cmp_42_26_234_5:
        cmp r15b, 0
        jne if_42_23_234_5_end
        if_42_26_234_5_code:
            mov rdi, 1
            exit_42_32_234_5:
                    mov rax, 60
                syscall
            exit_42_32_234_5_end:
        if_42_23_234_5_end:
    assert_234_5_end:
    mov qword [rbp + 336], 0
    bar_237_5:
        if_75_8_237_5:
        cmp_75_8_237_5:
        cmp qword [rbp + 336], 0
        jne if_75_5_237_5_end
        if_75_8_237_5_code:
            jmp bar_237_5_end
        if_75_5_237_5_end:
        mov qword [rbp + 336], 255
    bar_237_5_end:
    cmp_238_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_42_26_238_5:
        cmp_42_26_238_5:
        cmp r15b, 0
        jne if_42_23_238_5_end
        if_42_26_238_5_code:
            mov rdi, 1
            exit_42_32_238_5:
                    mov rax, 60
                syscall
            exit_42_32_238_5_end:
        if_42_23_238_5_end:
    assert_238_5_end:
    mov qword [rbp + 336], 1
    bar_241_5:
        if_75_8_241_5:
        cmp_75_8_241_5:
        cmp qword [rbp + 336], 0
        jne if_75_5_241_5_end
        if_75_8_241_5_code:
            jmp bar_241_5_end
        if_75_5_241_5_end:
        mov qword [rbp + 336], 255
    bar_241_5_end:
    cmp_242_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_242_12:
    assert_242_5:
        if_42_26_242_5:
        cmp_42_26_242_5:
        cmp r15b, 0
        jne if_42_23_242_5_end
        if_42_26_242_5_code:
            mov rdi, 1
            exit_42_32_242_5:
                    mov rax, 60
                syscall
            exit_42_32_242_5_end:
        if_42_23_242_5_end:
    assert_242_5_end:
    mov qword [rbp + 344], 1
    baz_245_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_245_13_end:
    cmp_246_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_246_12:
    assert_246_5:
        if_42_26_246_5:
        cmp_42_26_246_5:
        cmp r15b, 0
        jne if_42_23_246_5_end
        if_42_26_246_5_code:
            mov rdi, 1
            exit_42_32_246_5:
                    mov rax, 60
                syscall
            exit_42_32_246_5_end:
        if_42_23_246_5_end:
    assert_246_5_end:
    baz_248_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_248_9_end:
    cmp_249_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_249_12:
    assert_249_5:
        if_42_26_249_5:
        cmp_42_26_249_5:
        cmp r15b, 0
        jne if_42_23_249_5_end
        if_42_26_249_5_code:
            mov rdi, 1
            exit_42_32_249_5:
                    mov rax, 60
                syscall
            exit_42_32_249_5_end:
        if_42_23_249_5_end:
    assert_249_5_end:
    baz_251_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_251_23_end:
    mov qword [rbp + 368], 0
    cmp_252_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_252_12:
    assert_252_5:
        if_42_26_252_5:
        cmp_42_26_252_5:
        cmp r15b, 0
        jne if_42_23_252_5_end
        if_42_26_252_5_code:
            mov rdi, 1
            exit_42_32_252_5:
                    mov rax, 60
                syscall
            exit_42_32_252_5_end:
        if_42_23_252_5_end:
    assert_252_5_end:
    point_init_254_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_254_22_end:
    cmp_255_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_255_12:
    assert_255_5:
        if_42_26_255_5:
        cmp_42_26_255_5:
        cmp r15b, 0
        jne if_42_23_255_5_end
        if_42_26_255_5_code:
            mov rdi, 1
            exit_42_32_255_5:
                    mov rax, 60
                syscall
            exit_42_32_255_5_end:
        if_42_23_255_5_end:
    assert_255_5_end:
    cmp_256_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_42_26_256_5:
        cmp_42_26_256_5:
        cmp r15b, 0
        jne if_42_23_256_5_end
        if_42_26_256_5_code:
            mov rdi, 1
            exit_42_32_256_5:
                    mov rax, 60
                syscall
            exit_42_32_256_5_end:
        if_42_23_256_5_end:
    assert_256_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_262_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_262_12:
    assert_262_5:
        if_42_26_262_5:
        cmp_42_26_262_5:
        cmp r15b, 0
        jne if_42_23_262_5_end
        if_42_26_262_5_code:
            mov rdi, 1
            exit_42_32_262_5:
                    mov rax, 60
                syscall
            exit_42_32_262_5_end:
        if_42_23_262_5_end:
    assert_262_5_end:
    cmp_263_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_263_12:
    assert_263_5:
        if_42_26_263_5:
        cmp_42_26_263_5:
        cmp r15b, 0
        jne if_42_23_263_5_end
        if_42_26_263_5_code:
            mov rdi, 1
            exit_42_32_263_5:
                    mov rax, 60
                syscall
            exit_42_32_263_5_end:
        if_42_23_263_5_end:
    assert_263_5_end:
    cmp_264_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_264_12:
    assert_264_5:
        if_42_26_264_5:
        cmp_42_26_264_5:
        cmp r15b, 0
        jne if_42_23_264_5_end
        if_42_26_264_5_code:
            mov rdi, 1
            exit_42_32_264_5:
                    mov rax, 60
                syscall
            exit_42_32_264_5_end:
        if_42_23_264_5_end:
    assert_264_5_end:
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
    cmp_268_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_268_12:
    assert_268_5:
        if_42_26_268_5:
        cmp_42_26_268_5:
        cmp r15b, 0
        jne if_42_23_268_5_end
        if_42_26_268_5_code:
            mov rdi, 1
            exit_42_32_268_5:
                    mov rax, 60
                syscall
            exit_42_32_268_5_end:
        if_42_23_268_5_end:
    assert_268_5_end:
    cmp_269_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_269_12:
    assert_269_5:
        if_42_26_269_5:
        cmp_42_26_269_5:
        cmp r15b, 0
        jne if_42_23_269_5_end
        if_42_26_269_5_code:
            mov rdi, 1
            exit_42_32_269_5:
                    mov rax, 60
                syscall
            exit_42_32_269_5_end:
        if_42_23_269_5_end:
    assert_269_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_272_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_42_26_272_5:
        cmp_42_26_272_5:
        cmp r15b, 0
        jne if_42_23_272_5_end
        if_42_26_272_5_code:
            mov rdi, 1
            exit_42_32_272_5:
                    mov rax, 60
                syscall
            exit_42_32_272_5_end:
        if_42_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_42_26_273_5:
        cmp_42_26_273_5:
        cmp r15b, 0
        jne if_42_23_273_5_end
        if_42_26_273_5_code:
            mov rdi, 1
            exit_42_32_273_5:
                    mov rax, 60
                syscall
            exit_42_32_273_5_end:
        if_42_23_273_5_end:
    assert_273_5_end:
    cmp_274_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_42_26_274_5:
        cmp_42_26_274_5:
        cmp r15b, 0
        jne if_42_23_274_5_end
        if_42_26_274_5_code:
            mov rdi, 1
            exit_42_32_274_5:
                    mov rax, 60
                syscall
            exit_42_32_274_5_end:
        if_42_23_274_5_end:
    assert_274_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_280_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 280
    test r13, r13
    cmovs rbp, r12
    js baz_panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge baz_panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_280_12:
    assert_280_5:
        if_42_26_280_5:
        cmp_42_26_280_5:
        cmp r15b, 0
        jne if_42_23_280_5_end
        if_42_26_280_5_code:
            mov rdi, 1
            exit_42_32_280_5:
                    mov rax, 60
                syscall
            exit_42_32_280_5_end:
        if_42_23_280_5_end:
    assert_280_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 282
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge baz_panic_bounds
    imul r14, 20
    add r15, r14
    object_init_282_13:
        mov qword [r15 + 8], 74
    object_init_282_13_end:
    cmp_283_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 283
    test r13, r13
    cmovs rbp, r12
    js baz_panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge baz_panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 74
    sete r15b
    bool_end_283_12:
    assert_283_5:
        if_42_26_283_5:
        cmp_42_26_283_5:
        cmp r15b, 0
        jne if_42_23_283_5_end
        if_42_26_283_5_code:
            mov rdi, 1
            exit_42_32_283_5:
                    mov rax, 60
                syscall
            exit_42_32_283_5_end:
        if_42_23_283_5_end:
    assert_283_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge baz_panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge baz_panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_287_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 287
    test r13, r13
    cmovs rbp, r12
    js baz_panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge baz_panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 287
    test r13, r13
    cmovs rbp, r12
    js baz_panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge baz_panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_287_12:
    assert_287_5:
        if_42_26_287_5:
        cmp_42_26_287_5:
        cmp r15b, 0
        jne if_42_23_287_5_end
        if_42_26_287_5_code:
            mov rdi, 1
            exit_42_32_287_5:
                    mov rax, 60
                syscall
            exit_42_32_287_5_end:
        if_42_23_287_5_end:
    assert_287_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 290
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge baz_panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 290
    test rcx, rcx
    cmovs rbp, r14
    js baz_panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_panic_bounds
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 291
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge baz_panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 291
    test rcx, rcx
    cmovs rbp, r14
    js baz_panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_297_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 297
    test r13, r13
    cmovs rbp, r12
    js baz_panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge baz_panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 297
    test r13, r13
    cmovs rbp, r12
    js baz_panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge baz_panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_297_12:
    assert_297_5:
        if_42_26_297_5:
        cmp_42_26_297_5:
        cmp r15b, 0
        jne if_42_23_297_5_end
        if_42_26_297_5_code:
            mov rdi, 1
            exit_42_32_297_5:
                    mov rax, 60
                syscall
            exit_42_32_297_5_end:
        if_42_23_297_5_end:
    assert_297_5_end:
    cmp_298_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r10, 299
        test r12, r12
        cmovs rbp, r10
        js baz_panic_bounds
        cmp r12, 8
        cmovge rbp, r10
        jge baz_panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 299
        test rcx, rcx
        cmovs rbp, r12
        js baz_panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_panic_bounds
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r10, 300
        test r12, r12
        cmovs rbp, r10
        js baz_panic_bounds
        cmp r12, 8
        cmovge rbp, r10
        jge baz_panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 300
        test rcx, rcx
        cmovs rbp, r12
        js baz_panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_298_12:
    assert_298_5:
        if_42_26_298_5:
        cmp_42_26_298_5:
        cmp r15b, 0
        jne if_42_23_298_5_end
        if_42_26_298_5_code:
            mov rdi, 1
            exit_42_32_298_5:
                    mov rax, 60
                syscall
            exit_42_32_298_5_end:
        if_42_23_298_5_end:
    assert_298_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_305_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_305_12:
    assert_305_5:
        if_42_26_305_5:
        cmp_42_26_305_5:
        cmp r15b, 0
        jne if_42_23_305_5_end
        if_42_26_305_5_code:
            mov rdi, 1
            exit_42_32_305_5:
                    mov rax, 60
                syscall
            exit_42_32_305_5_end:
        if_42_23_305_5_end:
    assert_305_5_end:
    cmp_306_12:
    mov r14, 0
    mov r13, 306
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge baz_panic_bounds
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_306_12:
    assert_306_5:
        if_42_26_306_5:
        cmp_42_26_306_5:
        cmp r15b, 0
        jne if_42_23_306_5_end
        if_42_26_306_5_code:
            mov rdi, 1
            exit_42_32_306_5:
                    mov rax, 60
                syscall
            exit_42_32_306_5_end:
        if_42_23_306_5_end:
    assert_306_5_end:
    cmp_307_12:
    mov r14, 1
    mov r13, 307
    test r14, r14
    cmovs rbp, r13
    js baz_panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge baz_panic_bounds
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_307_12:
    assert_307_5:
        if_42_26_307_5:
        cmp_42_26_307_5:
        cmp r15b, 0
        jne if_42_23_307_5_end
        if_42_26_307_5_code:
            mov rdi, 1
            exit_42_32_307_5:
                    mov rax, 60
                syscall
            exit_42_32_307_5_end:
        if_42_23_307_5_end:
    assert_307_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_311_5:
        mov rdx, 21
        lea rsi, [rbp]
        sys_print_53_4_311_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_53_4_311_5_end:
    print_311_5_end:
    loop_312_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea r12, [rbp + 1168]
        call print_num
        POP_REGS
        print_315_9:
            mov rdx, 2
            lea rsi, [rbp + 61]
            sys_print_53_4_315_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_53_4_315_9_end:
        print_315_9_end:
        print_316_9:
            mov rdx, 12
            lea rsi, [rbp + 21]
            sys_print_53_4_316_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_53_4_316_9_end:
        print_316_9_end:
        str_in_317_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rbp + 1041]
                mov rdx, 127
            syscall
                mov byte [rbp + 1040], al
                sub byte [rbp + 1040], 1
        str_in_317_9_end:
        if_318_12:
        cmp_318_12:
        cmp byte [rbp + 1040], 0
        jne if_320_19
        if_318_12_code:
            jmp loop_312_5_end
        jmp if_318_9_end
        if_320_19:
        cmp_320_19:
        cmp byte [rbp + 1040], 4
        jg if_else_318_9
        if_320_19_code:
            print_321_13:
                mov rdx, 20
                lea rsi, [rbp + 33]
                sys_print_53_4_321_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_321_13_end:
            print_321_13_end:
            jmp loop_312_5
        jmp if_318_9_end
        if_else_318_9:
            print_324_13:
                mov rdx, 6
                lea rsi, [rbp + 53]
                sys_print_53_4_324_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_324_13_end:
            print_324_13_end:
            str_out_325_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rbp + 1041]
                    movsx rdx, byte [rbp + 1040]
                syscall
            str_out_325_13_end:
            print_326_13:
                mov rdx, 1
                lea rsi, [rbp + 59]
                sys_print_53_4_326_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_326_13_end:
            print_326_13_end:
            print_327_13:
                mov rdx, 1
                lea rsi, [rbp + 60]
                sys_print_53_4_327_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_327_13_end:
            print_327_13_end:
        if_318_9_end:
    jmp loop_312_5
    loop_312_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
print_num:
    mov qword [r12 + 8], 0
    mov qword [r12 + 16], 0
    mov dword [r12 + 24], 0
    mov r15, qword [r12]
    mov r14, qword [r15]
    mov qword [r12 + 28], r14
    mov byte [r12 + 36], 0
    if_136_8:
    cmp_136_8:
    cmp qword [r12 + 28], 0
    jge if_136_5_end
    if_136_8_code:
        mov byte [r12 + 36], 1
        neg qword [r12 + 28]
    if_136_5_end:
    mov qword [r12 + 37], 20
    loop_142_5:
        sub qword [r12 + 37], 1
        mov qword [r12 + 45], 48
        mov r15, qword [r12 + 28]
        mov rax, r15
        cqo
        mov r14, 10
        idiv r14
        mov r15, rdx
        add qword [r12 + 45], r15
        mov r15, qword [r12 + 37]
        mov r14, 146
        test r15, r15
        cmovs rbp, r14
        js baz_panic_bounds
        cmp r15, 20
        cmovge rbp, r14
        jge baz_panic_bounds
        mov r14b, byte [r12 + 45]
        mov byte [r12 + r15 + 8], r14b
        mov rax, qword [r12 + 28]
        cqo
        mov r15, 10
        idiv r15
        mov qword [r12 + 28], rax
        if_148_12:
        cmp_148_12:
        cmp qword [r12 + 28], 0
        jne if_148_9_end
        if_148_12_code:
            jmp loop_142_5_end
        if_148_9_end:
    jmp loop_142_5
    loop_142_5_end:
    if_151_8:
    cmp_151_8:
    cmp byte [r12 + 36], 0
    je if_151_5_end
    if_151_8_code:
        sub qword [r12 + 37], 1
        mov r15, qword [r12 + 37]
        mov r14, 153
        test r15, r15
        cmovs rbp, r14
        js baz_panic_bounds
        cmp r15, 20
        cmovge rbp, r14
        jge baz_panic_bounds
        mov byte [r12 + r15 + 8], 45
    if_151_5_end:
    mov qword [r12 + 45], 0
    loop_157_5:
        mov r15, qword [r12 + 45]
        mov r14, 158
        test r15, r15
        cmovs rbp, r14
        js baz_panic_bounds
        cmp r15, 20
        cmovge rbp, r14
        jge baz_panic_bounds
        mov r14, qword [r12 + 37]
        mov r13, 158
        test r14, r14
        cmovs rbp, r13
        js baz_panic_bounds
        cmp r14, 20
        cmovge rbp, r13
        jge baz_panic_bounds
        mov r13b, byte [r12 + r14 + 8]
        mov byte [r12 + r15 + 8], r13b
        add qword [r12 + 45], 1
        add qword [r12 + 37], 1
        if_161_12:
        cmp_161_12:
        cmp qword [r12 + 37], 20
        jne if_161_9_end
        if_161_12_code:
            jmp loop_157_5_end
        if_161_9_end:
    jmp loop_157_5
    loop_157_5_end:
    mov rdx, qword [r12 + 45]
    lea rsi, [r12 + 8]
    sys_print_164_5:
            mov rax, 1
            mov rdi, 0
        syscall
    sys_print_164_5_end:
    ret
print_num.size equ 53
baz_panic_bounds:
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
