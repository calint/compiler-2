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
    cmp_144_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_144_12:
    assert_144_5:
        if_32_26_144_5:
        cmp_32_26_144_5:
        cmp r15b, 0
        jne if_32_23_144_5_end
        if_32_26_144_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_144_5_end:
    assert_144_5_end:
    mov qword [rbp + 240], -1
    cmp_148_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_148_12:
    assert_148_5:
        if_32_26_148_5:
        cmp_32_26_148_5:
        cmp r15b, 0
        jne if_32_23_148_5_end
        if_32_26_148_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_148_5_end:
    assert_148_5_end:
        cmp_154_16:
        bool_end_154_16:
        mov r15b, 1
        assert_154_9:
            if_32_26_154_9:
            cmp_32_26_154_9:
            cmp r15b, 0
            jne if_32_23_154_9_end
            if_32_26_154_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_32_23_154_9_end:
        assert_154_9_end:
    cmp_157_12:
    bool_end_157_12:
    mov r15b, 1
    assert_157_5:
        if_32_26_157_5:
        cmp_32_26_157_5:
        cmp r15b, 0
        jne if_32_23_157_5_end
        if_32_26_157_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_157_5_end:
    assert_157_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 162
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 163
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 163
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_164_12:
    mov r14, 1
    mov r13, 164
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_164_12:
    assert_164_5:
        if_32_26_164_5:
        cmp_32_26_164_5:
        cmp r15b, 0
        jne if_32_23_164_5_end
        if_32_26_164_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_164_5_end:
    assert_164_5_end:
    cmp_165_12:
    mov r14, 2
    mov r13, 165
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_165_12:
    assert_165_5:
        if_32_26_165_5:
        cmp_32_26_165_5:
        cmp r15b, 0
        jne if_32_23_165_5_end
        if_32_26_165_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_165_5_end:
    assert_165_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_168_12:
    mov r14, 0
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_168_12:
    assert_168_5:
        if_32_26_168_5:
        cmp_32_26_168_5:
        cmp r15b, 0
        jne if_32_23_168_5_end
        if_32_26_168_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_168_5_end:
    assert_168_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 172
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 172
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_173_12:
        mov rcx, 3
        mov r14, 1
        mov r13, 173
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        mov r12, rcx
        add r12, r14
        cmp r12, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 * 4 + 224]
        mov r14, 1
        mov r13, 173
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        mov r12, rcx
        add r12, r14
        cmp r12, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + r14 * 4 + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool_end_173_12:
    assert_173_5:
        if_32_26_173_5:
        cmp_32_26_173_5:
        cmp r15b, 0
        jne if_32_23_173_5_end
        if_32_26_173_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_173_5_end:
    assert_173_5_end:
    mov r15, 2
    mov r14, 176
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_177_12:
        mov rcx, 4
        mov r14, 177
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 177
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
    bool_end_177_12:
    assert_177_5:
        if_32_26_177_5:
        cmp_32_26_177_5:
        cmp r15b, 0
        jne if_32_23_177_5_end
        if_32_26_177_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_177_5_end:
    assert_177_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 180
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 180
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_180_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_180_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_181_12:
    mov r14, qword [rbp + 248]
    mov r13, 181
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
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
    faz_183_5:
        mov r15, 1
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_183_5_end:
    cmp_184_12:
    mov r14, 1
    mov r13, 184
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_184_12:
    assert_184_5:
        if_32_26_184_5:
        cmp_32_26_184_5:
        cmp r15b, 0
        jne if_32_23_184_5_end
        if_32_26_184_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_184_5_end:
    assert_184_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_187_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_187_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_187_5
    foo_187_5_end:
    cmp_190_12:
    mov r14, 0
    mov r13, 190
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
    bool_end_190_12:
    assert_190_5:
        if_32_26_190_5:
        cmp_32_26_190_5:
        cmp r15b, 0
        jne if_32_23_190_5_end
        if_32_26_190_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    mov r14, 1
    mov r13, 191
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
    bool_end_191_12:
    assert_191_5:
        if_32_26_191_5:
        cmp_32_26_191_5:
        cmp r15b, 0
        jne if_32_23_191_5_end
        if_32_26_191_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_191_5_end:
    assert_191_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_198_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_198_5_end:
    cmp_199_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_199_12:
    assert_199_5:
        if_32_26_199_5:
        cmp_32_26_199_5:
        cmp r15b, 0
        jne if_32_23_199_5_end
        if_32_26_199_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_199_5_end:
    assert_199_5_end:
    cmp_200_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_200_12:
    assert_200_5:
        if_32_26_200_5:
        cmp_32_26_200_5:
        cmp r15b, 0
        jne if_32_23_200_5_end
        if_32_26_200_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_200_5_end:
    assert_200_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_203_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
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
    mov qword [rbp + 320], 3
    cmp_208_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool_end_208_12:
    assert_208_5:
        if_32_26_208_5:
        cmp_32_26_208_5:
        cmp r15b, 0
        jne if_32_23_208_5_end
        if_32_26_208_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_208_5_end:
    assert_208_5_end:
    mov qword [rbp + 336], 0
    bar_211_5:
        if_51_8_211_5:
        cmp_51_8_211_5:
        cmp qword [rbp + 336], 0
        jne if_51_5_211_5_end
        if_51_8_211_5_code:
            jmp bar_211_5_end
        if_51_5_211_5_end:
        mov qword [rbp + 336], 255
    bar_211_5_end:
    cmp_212_12:
    cmp qword [rbp + 336], 0
    sete r15b
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
    mov qword [rbp + 336], 1
    bar_215_5:
        if_51_8_215_5:
        cmp_51_8_215_5:
        cmp qword [rbp + 336], 0
        jne if_51_5_215_5_end
        if_51_8_215_5_code:
            jmp bar_215_5_end
        if_51_5_215_5_end:
        mov qword [rbp + 336], 255
    bar_215_5_end:
    cmp_216_12:
    cmp qword [rbp + 336], 255
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
    mov qword [rbp + 344], 1
    baz_219_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_219_13_end:
    cmp_220_12:
    cmp qword [rbp + 352], 2
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
    baz_222_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_222_9_end:
    cmp_223_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_223_12:
    assert_223_5:
        if_32_26_223_5:
        cmp_32_26_223_5:
        cmp r15b, 0
        jne if_32_23_223_5_end
        if_32_26_223_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_223_5_end:
    assert_223_5_end:
    baz_225_21:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_225_21_end:
    mov qword [rbp + 368], 0
    cmp_226_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_226_12:
    assert_226_5:
        if_32_26_226_5:
        cmp_32_26_226_5:
        cmp r15b, 0
        jne if_32_23_226_5_end
        if_32_26_226_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_226_5_end:
    assert_226_5_end:
    point_init_228_20:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_228_20_end:
    cmp_229_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_32_26_229_5:
        cmp_32_26_229_5:
        cmp r15b, 0
        jne if_32_23_229_5_end
        if_32_26_229_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    cmp qword [rbp + 384], -2
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
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_236_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_32_26_236_5:
        cmp_32_26_236_5:
        cmp r15b, 0
        jne if_32_23_236_5_end
        if_32_26_236_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_236_5_end:
    assert_236_5_end:
    cmp_237_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_32_26_237_5:
        cmp_32_26_237_5:
        cmp r15b, 0
        jne if_32_23_237_5_end
        if_32_26_237_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_237_5_end:
    assert_237_5_end:
    cmp_238_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_32_26_238_5:
        cmp_32_26_238_5:
        cmp r15b, 0
        jne if_32_23_238_5_end
        if_32_26_238_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_238_5_end:
    assert_238_5_end:
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
    cmp_242_12:
    cmp qword [rbp + 408], -1
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
    cmp_243_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_243_12:
    assert_243_5:
        if_32_26_243_5:
        cmp_32_26_243_5:
        cmp r15b, 0
        jne if_32_23_243_5_end
        if_32_26_243_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_243_5_end:
    assert_243_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_246_12:
    cmp qword [rbp + 444], -1
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
    cmp qword [rbp + 452], -2
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
    cmp_248_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_248_12:
    assert_248_5:
        if_32_26_248_5:
        cmp_32_26_248_5:
        cmp r15b, 0
        jne if_32_23_248_5_end
        if_32_26_248_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_248_5_end:
    assert_248_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_254_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 254
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
    bool_end_254_12:
    assert_254_5:
        if_32_26_254_5:
        cmp_32_26_254_5:
        cmp r15b, 0
        jne if_32_23_254_5_end
        if_32_26_254_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_254_5_end:
    assert_254_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_256_13:
        mov qword [r15 + 8], 74
    object_init_256_13_end:
    cmp_257_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 257
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
    bool_end_257_12:
    assert_257_5:
        if_32_26_257_5:
        cmp_32_26_257_5:
        cmp r15b, 0
        jne if_32_23_257_5_end
        if_32_26_257_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_257_5_end:
    assert_257_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_261_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 261
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 261
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_261_12:
    assert_261_5:
        if_32_26_261_5:
        cmp_32_26_261_5:
        cmp r15b, 0
        jne if_32_23_261_5_end
        if_32_26_261_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_261_5_end:
    assert_261_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 264
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 264
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 265
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 265
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_270_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 270
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 270
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_270_12:
    assert_270_5:
        if_32_26_270_5:
        cmp_32_26_270_5:
        cmp r15b, 0
        jne if_32_23_270_5_end
        if_32_26_270_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_270_5_end:
    assert_270_5_end:
    cmp_271_12:
        mov rcx, 8
        lea r14, [rbp + 504]
        mov r13, 0
        mov r12, 272
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 272
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [r14]
        lea r14, [rbp + 504]
        mov r13, 1
        mov r12, 273
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 273
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [r14]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool_end_271_12:
    assert_271_5:
        if_32_26_271_5:
        cmp_32_26_271_5:
        cmp r15b, 0
        jne if_32_23_271_5_end
        if_32_26_271_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_271_5_end:
    assert_271_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_278_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_278_12:
    assert_278_5:
        if_32_26_278_5:
        cmp_32_26_278_5:
        cmp r15b, 0
        jne if_32_23_278_5_end
        if_32_26_278_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_278_5_end:
    assert_278_5_end:
    cmp_279_12:
    mov r14, 0
    mov r13, 279
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_279_12:
    assert_279_5:
        if_32_26_279_5:
        cmp_32_26_279_5:
        cmp r15b, 0
        jne if_32_23_279_5_end
        if_32_26_279_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_279_5_end:
    assert_279_5_end:
    cmp_280_12:
    mov r14, 1
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
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
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_284_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_284_5_end:
    loop_285_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_288_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_288_9_end:
        print_289_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_289_9_end:
        str_in_290_9:
            mov rdi, 0
            lea rsi, [rbp + 1041]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
            sub byte [rbp + 1040], 1
        str_in_290_9_end:
        if_291_12:
        cmp_291_12:
        cmp byte [rbp + 1040], 0
        jne if_293_19
        if_291_12_code:
            jmp loop_285_5_end
        jmp if_291_9_end
        if_293_19:
        cmp_293_19:
        cmp byte [rbp + 1040], 4
        jg if_else_291_9
        if_293_19_code:
            print_294_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_294_13_end:
            jmp loop_285_5
        jmp if_291_9_end
        if_else_291_9:
            print_297_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_297_13_end:
            str_out_298_13:
                mov rdi, 1
                lea rsi, [rbp + 1041]
                movsx rdx, byte [rbp + 1040]
                mov rax, 1
                syscall
            str_out_298_13_end:
            print_299_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_299_13_end:
            print_300_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_300_13_end:
        if_291_9_end:
    jmp loop_285_5
    loop_285_5_end:
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
    if_107_8:
    cmp_107_8:
    cmp qword [rbx + 28], 0
    jge if_107_5_end
    if_107_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_107_5_end:
    mov qword [rbx + 37], 20
    loop_113_5:
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
        mov r14, 118
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
        if_120_12:
        cmp_120_12:
        cmp qword [rbx + 28], 0
        jne if_120_9_end
        if_120_12_code:
            jmp loop_113_5_end
        if_120_9_end:
    jmp loop_113_5
    loop_113_5_end:
    if_123_8:
    cmp_123_8:
    cmp byte [rbx + 36], 0
    je if_123_5_end
    if_123_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_123_5_end:
    mov qword [rbx + 45], 0
    loop_129_5:
        mov r15, qword [rbx + 45]
        mov r14, 130
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 130
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
        if_133_12:
        cmp_133_12:
        cmp qword [rbx + 37], 20
        jne if_133_9_end
        if_133_12_code:
            jmp loop_129_5_end
        if_133_9_end:
    jmp loop_129_5
    loop_129_5_end:
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
