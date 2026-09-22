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
    cmp_141_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_141_12:
    assert_141_5:
        if_36_26_141_5:
        cmp_36_26_141_5:
        cmp r15b, 0
        jne if_36_23_141_5_end
        if_36_26_141_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_141_5_end:
    assert_141_5_end:
    mov qword [rbp + 240], -1
    cmp_145_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_145_12:
    assert_145_5:
        if_36_26_145_5:
        cmp_36_26_145_5:
        cmp r15b, 0
        jne if_36_23_145_5_end
        if_36_26_145_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_145_5_end:
    assert_145_5_end:
        cmp_149_16:
        bool_end_149_16:
        mov r15b, 1
        assert_149_9:
            if_36_26_149_9:
            cmp_36_26_149_9:
            cmp r15b, 0
            jne if_36_23_149_9_end
            if_36_26_149_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_36_23_149_9_end:
        assert_149_9_end:
    cmp_152_12:
    bool_end_152_12:
    mov r15b, 1
    assert_152_5:
        if_36_26_152_5:
        cmp_36_26_152_5:
        cmp r15b, 0
        jne if_36_23_152_5_end
        if_36_26_152_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_152_5_end:
    assert_152_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 157
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 158
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_159_12:
    mov r14, 1
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_159_12:
    assert_159_5:
        if_36_26_159_5:
        cmp_36_26_159_5:
        cmp r15b, 0
        jne if_36_23_159_5_end
        if_36_26_159_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_159_5_end:
    assert_159_5_end:
    cmp_160_12:
    mov r14, 2
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_160_12:
    assert_160_5:
        if_36_26_160_5:
        cmp_36_26_160_5:
        cmp r15b, 0
        jne if_36_23_160_5_end
        if_36_26_160_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_160_5_end:
    assert_160_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 162
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 162
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_164_12:
    mov r14, 0
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
        if_36_26_164_5:
        cmp_36_26_164_5:
        cmp r15b, 0
        jne if_36_23_164_5_end
        if_36_26_164_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_164_5_end:
    assert_164_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 167
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_168_12:
        mov rcx, 4
        mov r13, 168
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 168
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
    bool_end_168_12:
    assert_168_5:
        if_36_26_168_5:
        cmp_36_26_168_5:
        cmp r15b, 0
        jne if_36_23_168_5_end
        if_36_26_168_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_168_5_end:
    assert_168_5_end:
    mov r15, 2
    mov r14, 171
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_172_12:
        mov rcx, 4
        mov r13, 172
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 172
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
    bool_end_172_12:
    assert_172_5:
        if_36_26_172_5:
        cmp_36_26_172_5:
        cmp r15b, 0
        jne if_36_23_172_5_end
        if_36_26_172_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_172_5_end:
    assert_172_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 175
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_175_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_175_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_176_12:
    mov r14, qword [rbp + 248]
    mov r13, 176
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_36_26_176_5:
        cmp_36_26_176_5:
        cmp r15b, 0
        jne if_36_23_176_5_end
        if_36_26_176_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_176_5_end:
    assert_176_5_end:
    faz_178_5:
        mov r15, 1
        mov r14, 71
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_178_5_end:
    cmp_179_12:
    mov r14, 1
    mov r13, 179
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_179_12:
    assert_179_5:
        if_36_26_179_5:
        cmp_36_26_179_5:
        cmp r15b, 0
        jne if_36_23_179_5_end
        if_36_26_179_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_179_5_end:
    assert_179_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_182_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_182_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_182_5
    foo_182_5_end:
    cmp_185_12:
    mov r14, 0
    mov r13, 185
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
    bool_end_185_12:
    assert_185_5:
        if_36_26_185_5:
        cmp_36_26_185_5:
        cmp r15b, 0
        jne if_36_23_185_5_end
        if_36_26_185_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_185_5_end:
    assert_185_5_end:
    cmp_186_12:
    mov r14, 1
    mov r13, 186
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
    bool_end_186_12:
    assert_186_5:
        if_36_26_186_5:
        cmp_36_26_186_5:
        cmp r15b, 0
        jne if_36_23_186_5_end
        if_36_26_186_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_186_5_end:
    assert_186_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_193_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_193_5_end:
    cmp_194_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_194_12:
    assert_194_5:
        if_36_26_194_5:
        cmp_36_26_194_5:
        cmp r15b, 0
        jne if_36_23_194_5_end
        if_36_26_194_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_194_5_end:
    assert_194_5_end:
    cmp_195_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_36_26_195_5:
        cmp_36_26_195_5:
        cmp r15b, 0
        jne if_36_23_195_5_end
        if_36_26_195_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_195_5_end:
    assert_195_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_198_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_198_12:
    assert_198_5:
        if_36_26_198_5:
        cmp_36_26_198_5:
        cmp r15b, 0
        jne if_36_23_198_5_end
        if_36_26_198_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_198_5_end:
    assert_198_5_end:
    mov qword [rbp + 320], 3
    cmp_203_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_36_26_203_5:
        cmp_36_26_203_5:
        cmp r15b, 0
        jne if_36_23_203_5_end
        if_36_26_203_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_203_5_end:
    assert_203_5_end:
    mov qword [rbp + 336], 0
    bar_206_5:
        if_52_8_206_5:
        cmp_52_8_206_5:
        cmp qword [rbp + 336], 0
        jne if_52_5_206_5_end
        if_52_8_206_5_code:
            jmp bar_206_5_end
        if_52_5_206_5_end:
        mov qword [rbp + 336], 255
    bar_206_5_end:
    cmp_207_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_36_26_207_5:
        cmp_36_26_207_5:
        cmp r15b, 0
        jne if_36_23_207_5_end
        if_36_26_207_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_207_5_end:
    assert_207_5_end:
    mov qword [rbp + 336], 1
    bar_210_5:
        if_52_8_210_5:
        cmp_52_8_210_5:
        cmp qword [rbp + 336], 0
        jne if_52_5_210_5_end
        if_52_8_210_5_code:
            jmp bar_210_5_end
        if_52_5_210_5_end:
        mov qword [rbp + 336], 255
    bar_210_5_end:
    cmp_211_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_36_26_211_5:
        cmp_36_26_211_5:
        cmp r15b, 0
        jne if_36_23_211_5_end
        if_36_26_211_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_211_5_end:
    assert_211_5_end:
    mov qword [rbp + 344], 1
    baz_214_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_214_13_end:
    cmp_215_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_215_12:
    assert_215_5:
        if_36_26_215_5:
        cmp_36_26_215_5:
        cmp r15b, 0
        jne if_36_23_215_5_end
        if_36_26_215_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_215_5_end:
    assert_215_5_end:
    baz_217_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_217_9_end:
    cmp_218_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_36_26_218_5:
        cmp_36_26_218_5:
        cmp r15b, 0
        jne if_36_23_218_5_end
        if_36_26_218_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_218_5_end:
    assert_218_5_end:
    baz_220_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_220_23_end:
    mov qword [rbp + 368], 0
    cmp_221_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_36_26_221_5:
        cmp_36_26_221_5:
        cmp r15b, 0
        jne if_36_23_221_5_end
        if_36_26_221_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_221_5_end:
    assert_221_5_end:
    point_init_223_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_223_22_end:
    cmp_224_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_224_12:
    assert_224_5:
        if_36_26_224_5:
        cmp_36_26_224_5:
        cmp r15b, 0
        jne if_36_23_224_5_end
        if_36_26_224_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_224_5_end:
    assert_224_5_end:
    cmp_225_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_36_26_225_5:
        cmp_36_26_225_5:
        cmp r15b, 0
        jne if_36_23_225_5_end
        if_36_26_225_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_225_5_end:
    assert_225_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_231_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_231_12:
    assert_231_5:
        if_36_26_231_5:
        cmp_36_26_231_5:
        cmp r15b, 0
        jne if_36_23_231_5_end
        if_36_26_231_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_231_5_end:
    assert_231_5_end:
    cmp_232_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_232_12:
    assert_232_5:
        if_36_26_232_5:
        cmp_36_26_232_5:
        cmp r15b, 0
        jne if_36_23_232_5_end
        if_36_26_232_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_232_5_end:
    assert_232_5_end:
    cmp_233_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_233_12:
    assert_233_5:
        if_36_26_233_5:
        cmp_36_26_233_5:
        cmp r15b, 0
        jne if_36_23_233_5_end
        if_36_26_233_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_233_5_end:
    assert_233_5_end:
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
    cmp_237_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_36_26_237_5:
        cmp_36_26_237_5:
        cmp r15b, 0
        jne if_36_23_237_5_end
        if_36_26_237_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_237_5_end:
    assert_237_5_end:
    cmp_238_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_36_26_238_5:
        cmp_36_26_238_5:
        cmp r15b, 0
        jne if_36_23_238_5_end
        if_36_26_238_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_238_5_end:
    assert_238_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_241_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_241_12:
    assert_241_5:
        if_36_26_241_5:
        cmp_36_26_241_5:
        cmp r15b, 0
        jne if_36_23_241_5_end
        if_36_26_241_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_241_5_end:
    assert_241_5_end:
    cmp_242_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_242_12:
    assert_242_5:
        if_36_26_242_5:
        cmp_36_26_242_5:
        cmp r15b, 0
        jne if_36_23_242_5_end
        if_36_26_242_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_242_5_end:
    assert_242_5_end:
    cmp_243_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_243_12:
    assert_243_5:
        if_36_26_243_5:
        cmp_36_26_243_5:
        cmp r15b, 0
        jne if_36_23_243_5_end
        if_36_26_243_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_243_5_end:
    assert_243_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_249_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 249
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
    bool_end_249_12:
    assert_249_5:
        if_36_26_249_5:
        cmp_36_26_249_5:
        cmp r15b, 0
        jne if_36_23_249_5_end
        if_36_26_249_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_249_5_end:
    assert_249_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_251_13:
        mov qword [r15 + 8], 74
    object_init_251_13_end:
    cmp_252_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 252
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
    bool_end_252_12:
    assert_252_5:
        if_36_26_252_5:
        cmp_36_26_252_5:
        cmp r15b, 0
        jne if_36_23_252_5_end
        if_36_26_252_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_252_5_end:
    assert_252_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 255
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_256_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 256
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 256
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_36_26_256_5:
        cmp_36_26_256_5:
        cmp r15b, 0
        jne if_36_23_256_5_end
        if_36_26_256_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_256_5_end:
    assert_256_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 259
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 259
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 260
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_266_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 266
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 266
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_266_12:
    assert_266_5:
        if_36_26_266_5:
        cmp_36_26_266_5:
        cmp r15b, 0
        jne if_36_23_266_5_end
        if_36_26_266_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_266_5_end:
    assert_266_5_end:
    cmp_267_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r10, 268
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 268
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r10, 269
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 269
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
    bool_end_267_12:
    assert_267_5:
        if_36_26_267_5:
        cmp_36_26_267_5:
        cmp r15b, 0
        jne if_36_23_267_5_end
        if_36_26_267_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_267_5_end:
    assert_267_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_274_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_36_26_274_5:
        cmp_36_26_274_5:
        cmp r15b, 0
        jne if_36_23_274_5_end
        if_36_26_274_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_274_5_end:
    assert_274_5_end:
    cmp_275_12:
    mov r14, 0
    mov r13, 275
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_275_12:
    assert_275_5:
        if_36_26_275_5:
        cmp_36_26_275_5:
        cmp r15b, 0
        jne if_36_23_275_5_end
        if_36_26_275_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_275_5_end:
    assert_275_5_end:
    cmp_276_12:
    mov r14, 1
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_276_12:
    assert_276_5:
        if_36_26_276_5:
        cmp_36_26_276_5:
        cmp r15b, 0
        jne if_36_23_276_5_end
        if_36_26_276_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_36_23_276_5_end:
    assert_276_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_280_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_280_5_end:
    loop_281_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_284_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_284_9_end:
        print_285_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_285_9_end:
        str_in_286_9:
            mov rdi, 0
            lea rsi, [rbp + 1041]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
            sub byte [rbp + 1040], 1
        str_in_286_9_end:
        if_287_12:
        cmp_287_12:
        cmp byte [rbp + 1040], 0
        jne if_289_19
        if_287_12_code:
            jmp loop_281_5_end
        jmp if_287_9_end
        if_289_19:
        cmp_289_19:
        cmp byte [rbp + 1040], 4
        jg if_else_287_9
        if_289_19_code:
            print_290_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_290_13_end:
            jmp loop_281_5
        jmp if_287_9_end
        if_else_287_9:
            print_293_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_293_13_end:
            str_out_294_13:
                mov rdi, 1
                lea rsi, [rbp + 1041]
                movsx rdx, byte [rbp + 1040]
                mov rax, 1
                syscall
            str_out_294_13_end:
            print_295_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_295_13_end:
            print_296_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_296_13_end:
        if_287_9_end:
    jmp loop_281_5
    loop_281_5_end:
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
    if_105_8:
    cmp_105_8:
    cmp qword [rbx + 28], 0
    jge if_105_5_end
    if_105_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_105_5_end:
    mov qword [rbx + 37], 20
    loop_111_5:
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
        mov r14, 115
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
        if_117_12:
        cmp_117_12:
        cmp qword [rbx + 28], 0
        jne if_117_9_end
        if_117_12_code:
            jmp loop_111_5_end
        if_117_9_end:
    jmp loop_111_5
    loop_111_5_end:
    if_120_8:
    cmp_120_8:
    cmp byte [rbx + 36], 0
    je if_120_5_end
    if_120_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 122
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_120_5_end:
    mov qword [rbx + 45], 0
    loop_126_5:
        mov r15, qword [rbx + 45]
        mov r14, 127
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 127
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
        if_130_12:
        cmp_130_12:
        cmp qword [rbx + 37], 20
        jne if_130_9_end
        if_130_12_code:
            jmp loop_126_5_end
        if_130_9_end:
    jmp loop_126_5
    loop_126_5_end:
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
