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
    cmp_173_19:
        mov rcx, 3
        mov r15, 1
        mov r14, 173
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
        mov r14, 173
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
    bool_end_173_19:
    cmp_176_12:
    cmp byte [rbp + 288], 0
    setne r15b
    bool_end_176_12:
    assert_176_5:
        if_32_26_176_5:
        cmp_32_26_176_5:
        cmp r15b, 0
        jne if_32_23_176_5_end
        if_32_26_176_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_176_5_end:
    assert_176_5_end:
    mov r15, 2
    mov r14, 178
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_179_12:
        mov rcx, 4
        mov r14, 179
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 179
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
    bool_end_179_12:
    assert_179_5:
        if_32_26_179_5:
        cmp_32_26_179_5:
        cmp r15b, 0
        jne if_32_23_179_5_end
        if_32_26_179_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_179_5_end:
    assert_179_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_182_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_182_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_183_12:
    mov r14, qword [rbp + 248]
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_32_26_183_5:
        cmp_32_26_183_5:
        cmp r15b, 0
        jne if_32_23_183_5_end
        if_32_26_183_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_183_5_end:
    assert_183_5_end:
    faz_185_5:
        mov r15, 1
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_185_5_end:
    cmp_186_12:
    mov r14, 1
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_186_12:
    assert_186_5:
        if_32_26_186_5:
        cmp_32_26_186_5:
        cmp r15b, 0
        jne if_32_23_186_5_end
        if_32_26_186_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_186_5_end:
    assert_186_5_end:
    mov qword [rbp + 289], 3
    mov qword [rbp + 297], 5
    lea r15, [rbp + 289]
    mov qword [rbp + 313], 0
    foo_189_5:
        mov r14, qword [rbp + 313]
        add qword [r15], r14
        add qword [r15], 2
        foo_189_5_continue:
            add r15, 8
            inc qword [rbp + 313]
            cmp qword [rbp + 313], 2
            jne foo_189_5
    foo_189_5_end:
    cmp_192_12:
    mov r14, 0
    mov r13, 192
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
    bool_end_192_12:
    assert_192_5:
        if_32_26_192_5:
        cmp_32_26_192_5:
        cmp r15b, 0
        jne if_32_23_192_5_end
        if_32_26_192_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_192_5_end:
    assert_192_5_end:
    cmp_193_12:
    mov r14, 1
    mov r13, 193
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
    bool_end_193_12:
    assert_193_5:
        if_32_26_193_5:
        cmp_32_26_193_5:
        cmp r15b, 0
        jne if_32_23_193_5_end
        if_32_26_193_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_193_5_end:
    assert_193_5_end:
    mov qword [rbp + 305], 0
    mov qword [rbp + 313], 0
    fooz_200_5:
        mov qword [rbp + 305], 2
        mov qword [rbp + 313], 11
    fooz_200_5_end:
    cmp_201_12:
    cmp qword [rbp + 305], 2
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_32_26_201_5:
        cmp_32_26_201_5:
        cmp r15b, 0
        jne if_32_23_201_5_end
        if_32_26_201_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_201_5_end:
    assert_201_5_end:
    cmp_202_12:
    cmp qword [rbp + 313], 11
    sete r15b
    bool_end_202_12:
    assert_202_5:
        if_32_26_202_5:
        cmp_32_26_202_5:
        cmp r15b, 0
        jne if_32_23_202_5_end
        if_32_26_202_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_202_5_end:
    assert_202_5_end:
    mov rax, qword [rbp + 305]
    mov qword [rbp + 321], rax
    mov rax, qword [rbp + 313]
    mov qword [rbp + 329], rax
    cmp_205_12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool_end_205_12:
    assert_205_5:
        if_32_26_205_5:
        cmp_32_26_205_5:
        cmp r15b, 0
        jne if_32_23_205_5_end
        if_32_26_205_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_205_5_end:
    assert_205_5_end:
    mov qword [rbp + 321], 3
    cmp_210_12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool_end_210_12:
    assert_210_5:
        if_32_26_210_5:
        cmp_32_26_210_5:
        cmp r15b, 0
        jne if_32_23_210_5_end
        if_32_26_210_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_210_5_end:
    assert_210_5_end:
    mov qword [rbp + 337], 0
    bar_213_5:
        if_51_8_213_5:
        cmp_51_8_213_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_213_5_end
        if_51_8_213_5_code:
            jmp bar_213_5_end
        if_51_5_213_5_end:
        mov qword [rbp + 337], 255
    bar_213_5_end:
    cmp_214_12:
    cmp qword [rbp + 337], 0
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_32_26_214_5:
        cmp_32_26_214_5:
        cmp r15b, 0
        jne if_32_23_214_5_end
        if_32_26_214_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_214_5_end:
    assert_214_5_end:
    mov qword [rbp + 337], 1
    bar_217_5:
        if_51_8_217_5:
        cmp_51_8_217_5:
        cmp qword [rbp + 337], 0
        jne if_51_5_217_5_end
        if_51_8_217_5_code:
            jmp bar_217_5_end
        if_51_5_217_5_end:
        mov qword [rbp + 337], 255
    bar_217_5_end:
    cmp_218_12:
    cmp qword [rbp + 337], 255
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_32_26_218_5:
        cmp_32_26_218_5:
        cmp r15b, 0
        jne if_32_23_218_5_end
        if_32_26_218_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_218_5_end:
    assert_218_5_end:
    mov qword [rbp + 345], 1
    baz_221_13:
        mov r15, qword [rbp + 345]
        imul r15, 2
        mov qword [rbp + 353], r15
    baz_221_13_end:
    cmp_222_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_32_26_222_5:
        cmp_32_26_222_5:
        cmp r15b, 0
        jne if_32_23_222_5_end
        if_32_26_222_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_222_5_end:
    assert_222_5_end:
    baz_224_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 353], r15
    baz_224_9_end:
    cmp_225_12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_32_26_225_5:
        cmp_32_26_225_5:
        cmp r15b, 0
        jne if_32_23_225_5_end
        if_32_26_225_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_225_5_end:
    assert_225_5_end:
    baz_227_21:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 361], r15
    baz_227_21_end:
    mov qword [rbp + 369], 0
    cmp_228_12:
    cmp qword [rbp + 361], 6
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_32_26_228_5:
        cmp_32_26_228_5:
        cmp r15b, 0
        jne if_32_23_228_5_end
        if_32_26_228_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_228_5_end:
    assert_228_5_end:
    point_init_230_20:
        mov qword [rbp + 377], -1
        mov qword [rbp + 385], -2
    point_init_230_20_end:
    cmp_231_12:
    cmp qword [rbp + 377], -1
    sete r15b
    bool_end_231_12:
    assert_231_5:
        if_32_26_231_5:
        cmp_32_26_231_5:
        cmp r15b, 0
        jne if_32_23_231_5_end
        if_32_26_231_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_231_5_end:
    assert_231_5_end:
    cmp_232_12:
    cmp qword [rbp + 385], -2
    sete r15b
    bool_end_232_12:
    assert_232_5:
        if_32_26_232_5:
        cmp_32_26_232_5:
        cmp r15b, 0
        jne if_32_23_232_5_end
        if_32_26_232_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_232_5_end:
    assert_232_5_end:
    mov qword [rbp + 393], 1
    mov qword [rbp + 401], 2
    mov r15, qword [rbp + 393]
    imul r15, 10
    mov qword [rbp + 409], r15
    mov r15, qword [rbp + 401]
    mov qword [rbp + 417], r15
    mov dword [rbp + 425], 16711680
    cmp_238_12:
    cmp qword [rbp + 409], 10
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
    cmp_239_12:
    cmp qword [rbp + 417], 2
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_32_26_239_5:
        cmp_32_26_239_5:
        cmp r15b, 0
        jne if_32_23_239_5_end
        if_32_26_239_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_239_5_end:
    assert_239_5_end:
    cmp_240_12:
    cmp dword [rbp + 425], 16711680
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
    cmp_244_12:
    cmp qword [rbp + 409], -1
    sete r15b
    bool_end_244_12:
    assert_244_5:
        if_32_26_244_5:
        cmp_32_26_244_5:
        cmp r15b, 0
        jne if_32_23_244_5_end
        if_32_26_244_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_244_5_end:
    assert_244_5_end:
    cmp_245_12:
    cmp qword [rbp + 417], -2
    sete r15b
    bool_end_245_12:
    assert_245_5:
        if_32_26_245_5:
        cmp_32_26_245_5:
        cmp r15b, 0
        jne if_32_23_245_5_end
        if_32_26_245_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_245_5_end:
    assert_245_5_end:
    lea rsi, [rbp + 409]
    lea rdi, [rbp + 445]
    mov rcx, 20
    rep movsb
    cmp_248_12:
    cmp qword [rbp + 445], -1
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
    cmp_249_12:
    cmp qword [rbp + 453], -2
    sete r15b
    bool_end_249_12:
    assert_249_5:
        if_32_26_249_5:
        cmp_32_26_249_5:
        cmp r15b, 0
        jne if_32_23_249_5_end
        if_32_26_249_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_249_5_end:
    assert_249_5_end:
    cmp_250_12:
    cmp dword [rbp + 461], 16711680
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
    xor al, al
    lea rdi, [rbp + 465]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 473], 73
    cmp_256_12:
    lea r14, [rbp + 465]
    mov r13, 0
    mov r12, 256
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
    bool_end_256_12:
    assert_256_5:
        if_32_26_256_5:
        cmp_32_26_256_5:
        cmp r15b, 0
        jne if_32_23_256_5_end
        if_32_26_256_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_256_5_end:
    assert_256_5_end:
    lea r15, [rbp + 465]
    mov r14, 1
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_258_13:
        mov qword [r15 + 8], 74
    object_init_258_13_end:
    cmp_259_12:
    lea r14, [rbp + 465]
    mov r13, 1
    mov r12, 259
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
    bool_end_259_12:
    assert_259_5:
        if_32_26_259_5:
        cmp_32_26_259_5:
        cmp r15b, 0
        jne if_32_23_259_5_end
        if_32_26_259_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_259_5_end:
    assert_259_5_end:
    xor al, al
    lea rdi, [rbp + 505]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 505]
    mov r14, 1
    mov r13, 262
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 262
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_263_12:
    lea r14, [rbp + 505]
    mov r13, 1
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_263_12:
    assert_263_5:
        if_32_26_263_5:
        cmp_32_26_263_5:
        cmp r15b, 0
        jne if_32_23_263_5_end
        if_32_26_263_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_263_5_end:
    assert_263_5_end:
    mov rcx, 8
    lea r15, [rbp + 505]
    mov r14, 1
    mov r13, 266
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 266
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 505]
    mov r14, 0
    mov r13, 267
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 267
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_272_12:
    lea r14, [rbp + 505]
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
    mov r13, 1
    mov r12, 272
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_32_26_272_5:
        cmp_32_26_272_5:
        cmp r15b, 0
        jne if_32_23_272_5_end
        if_32_26_272_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
        mov rcx, 8
        lea r14, [rbp + 505]
        mov r13, 0
        mov r12, 274
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 274
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [r14]
        lea r14, [rbp + 505]
        mov r13, 1
        mov r12, 275
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 275
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
