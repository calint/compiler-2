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
    cmp_137_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_32_28_137_5:
        cmp_32_28_137_5:
        cmp r15b, 0
        jne if_32_25_137_5_end
        if_32_28_137_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_137_5_end:
    assert_137_5_end:
    mov qword [rbp + 240], -1
    cmp_141_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_141_12:
    assert_141_5:
        if_32_28_141_5:
        cmp_32_28_141_5:
        cmp r15b, 0
        jne if_32_25_141_5_end
        if_32_28_141_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_141_5_end:
    assert_141_5_end:
        cmp_145_16:
        bool_end_145_16:
        mov r15b, 1
        assert_145_9:
            if_32_28_145_9:
            cmp_32_28_145_9:
            cmp r15b, 0
            jne if_32_25_145_9_end
            if_32_28_145_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_32_25_145_9_end:
        assert_145_9_end:
    cmp_148_12:
    bool_end_148_12:
    mov r15b, 1
    assert_148_5:
        if_32_28_148_5:
        cmp_32_28_148_5:
        cmp r15b, 0
        jne if_32_25_148_5_end
        if_32_28_148_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_148_5_end:
    assert_148_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 153
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 154
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_155_12:
    mov r14, 1
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_155_12:
    assert_155_5:
        if_32_28_155_5:
        cmp_32_28_155_5:
        cmp r15b, 0
        jne if_32_25_155_5_end
        if_32_28_155_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_155_5_end:
    assert_155_5_end:
    cmp_156_12:
    mov r14, 2
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_32_28_156_5:
        cmp_32_28_156_5:
        cmp r15b, 0
        jne if_32_25_156_5_end
        if_32_28_156_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_156_5_end:
    assert_156_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 158
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 158
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_160_12:
    mov r14, 0
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
        if_32_28_160_5:
        cmp_32_28_160_5:
        cmp r15b, 0
        jne if_32_25_160_5_end
        if_32_28_160_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_160_5_end:
    assert_160_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 163
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 163
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_164_12:
        mov rcx, 3
        mov r14, 1
        mov r13, 164
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
        mov r13, 164
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
    bool_end_164_12:
    assert_164_5:
        if_32_28_164_5:
        cmp_32_28_164_5:
        cmp r15b, 0
        jne if_32_25_164_5_end
        if_32_28_164_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_164_5_end:
    assert_164_5_end:
    mov r15, 2
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_168_12:
        mov rcx, 4
        mov r14, 168
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 168
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
    bool_end_168_12:
    assert_168_5:
        if_32_28_168_5:
        cmp_32_28_168_5:
        cmp r15b, 0
        jne if_32_25_168_5_end
        if_32_28_168_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_168_5_end:
    assert_168_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 171
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_171_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_171_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_172_12:
    mov r14, qword [rbp + 248]
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_172_12:
    assert_172_5:
        if_32_28_172_5:
        cmp_32_28_172_5:
        cmp r15b, 0
        jne if_32_25_172_5_end
        if_32_28_172_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_172_5_end:
    assert_172_5_end:
    faz_174_5:
        mov r15, 1
        mov r14, 67
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_174_5_end:
    cmp_175_12:
    mov r14, 1
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_175_12:
    assert_175_5:
        if_32_28_175_5:
        cmp_32_28_175_5:
        cmp r15b, 0
        jne if_32_25_175_5_end
        if_32_28_175_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_175_5_end:
    assert_175_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_178_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_178_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_178_5
    foo_178_5_end:
    cmp_181_12:
    mov r14, 0
    mov r13, 181
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
    bool_end_181_12:
    assert_181_5:
        if_32_28_181_5:
        cmp_32_28_181_5:
        cmp r15b, 0
        jne if_32_25_181_5_end
        if_32_28_181_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_181_5_end:
    assert_181_5_end:
    cmp_182_12:
    mov r14, 1
    mov r13, 182
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
    bool_end_182_12:
    assert_182_5:
        if_32_28_182_5:
        cmp_32_28_182_5:
        cmp r15b, 0
        jne if_32_25_182_5_end
        if_32_28_182_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_182_5_end:
    assert_182_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_189_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_189_5_end:
    cmp_190_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_190_12:
    assert_190_5:
        if_32_28_190_5:
        cmp_32_28_190_5:
        cmp r15b, 0
        jne if_32_25_190_5_end
        if_32_28_190_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_32_28_191_5:
        cmp_32_28_191_5:
        cmp r15b, 0
        jne if_32_25_191_5_end
        if_32_28_191_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_191_5_end:
    assert_191_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_194_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool_end_194_12:
    assert_194_5:
        if_32_28_194_5:
        cmp_32_28_194_5:
        cmp r15b, 0
        jne if_32_25_194_5_end
        if_32_28_194_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_194_5_end:
    assert_194_5_end:
    mov qword [rbp + 320], 3
    cmp_199_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool_end_199_12:
    assert_199_5:
        if_32_28_199_5:
        cmp_32_28_199_5:
        cmp r15b, 0
        jne if_32_25_199_5_end
        if_32_28_199_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_199_5_end:
    assert_199_5_end:
    mov qword [rbp + 336], 0
    bar_202_5:
        if_48_8_202_5:
        cmp_48_8_202_5:
        cmp qword [rbp + 336], 0
        jne if_48_5_202_5_end
        if_48_8_202_5_code:
            jmp bar_202_5_end
        if_48_5_202_5_end:
        mov qword [rbp + 336], 255
    bar_202_5_end:
    cmp_203_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_32_28_203_5:
        cmp_32_28_203_5:
        cmp r15b, 0
        jne if_32_25_203_5_end
        if_32_28_203_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_203_5_end:
    assert_203_5_end:
    mov qword [rbp + 336], 1
    bar_206_5:
        if_48_8_206_5:
        cmp_48_8_206_5:
        cmp qword [rbp + 336], 0
        jne if_48_5_206_5_end
        if_48_8_206_5_code:
            jmp bar_206_5_end
        if_48_5_206_5_end:
        mov qword [rbp + 336], 255
    bar_206_5_end:
    cmp_207_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_32_28_207_5:
        cmp_32_28_207_5:
        cmp r15b, 0
        jne if_32_25_207_5_end
        if_32_28_207_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_207_5_end:
    assert_207_5_end:
    mov qword [rbp + 344], 1
    baz_210_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_210_13_end:
    cmp_211_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_32_28_211_5:
        cmp_32_28_211_5:
        cmp r15b, 0
        jne if_32_25_211_5_end
        if_32_28_211_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_211_5_end:
    assert_211_5_end:
    baz_213_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_213_9_end:
    cmp_214_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_32_28_214_5:
        cmp_32_28_214_5:
        cmp r15b, 0
        jne if_32_25_214_5_end
        if_32_28_214_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_214_5_end:
    assert_214_5_end:
    baz_216_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_216_23_end:
    mov qword [rbp + 368], 0
    cmp_217_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_217_12:
    assert_217_5:
        if_32_28_217_5:
        cmp_32_28_217_5:
        cmp r15b, 0
        jne if_32_25_217_5_end
        if_32_28_217_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_217_5_end:
    assert_217_5_end:
    point_init_219_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_219_22_end:
    cmp_220_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_220_12:
    assert_220_5:
        if_32_28_220_5:
        cmp_32_28_220_5:
        cmp r15b, 0
        jne if_32_25_220_5_end
        if_32_28_220_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_220_5_end:
    assert_220_5_end:
    cmp_221_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_32_28_221_5:
        cmp_32_28_221_5:
        cmp r15b, 0
        jne if_32_25_221_5_end
        if_32_28_221_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_221_5_end:
    assert_221_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_227_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_227_12:
    assert_227_5:
        if_32_28_227_5:
        cmp_32_28_227_5:
        cmp r15b, 0
        jne if_32_25_227_5_end
        if_32_28_227_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_227_5_end:
    assert_227_5_end:
    cmp_228_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_32_28_228_5:
        cmp_32_28_228_5:
        cmp r15b, 0
        jne if_32_25_228_5_end
        if_32_28_228_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_228_5_end:
    assert_228_5_end:
    cmp_229_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_32_28_229_5:
        cmp_32_28_229_5:
        cmp r15b, 0
        jne if_32_25_229_5_end
        if_32_28_229_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_229_5_end:
    assert_229_5_end:
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
    cmp_233_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_233_12:
    assert_233_5:
        if_32_28_233_5:
        cmp_32_28_233_5:
        cmp r15b, 0
        jne if_32_25_233_5_end
        if_32_28_233_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_233_5_end:
    assert_233_5_end:
    cmp_234_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_32_28_234_5:
        cmp_32_28_234_5:
        cmp r15b, 0
        jne if_32_25_234_5_end
        if_32_28_234_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_234_5_end:
    assert_234_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_237_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_32_28_237_5:
        cmp_32_28_237_5:
        cmp r15b, 0
        jne if_32_25_237_5_end
        if_32_28_237_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_237_5_end:
    assert_237_5_end:
    cmp_238_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_32_28_238_5:
        cmp_32_28_238_5:
        cmp r15b, 0
        jne if_32_25_238_5_end
        if_32_28_238_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_238_5_end:
    assert_238_5_end:
    cmp_239_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_32_28_239_5:
        cmp_32_28_239_5:
        cmp r15b, 0
        jne if_32_25_239_5_end
        if_32_28_239_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_239_5_end:
    assert_239_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_245_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 245
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
    bool_end_245_12:
    assert_245_5:
        if_32_28_245_5:
        cmp_32_28_245_5:
        cmp r15b, 0
        jne if_32_25_245_5_end
        if_32_28_245_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_245_5_end:
    assert_245_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_247_13:
        mov qword [r15 + 8], 74
    object_init_247_13_end:
    cmp_248_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 248
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
    bool_end_248_12:
    assert_248_5:
        if_32_28_248_5:
        cmp_32_28_248_5:
        cmp r15b, 0
        jne if_32_25_248_5_end
        if_32_28_248_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_248_5_end:
    assert_248_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_252_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 252
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 252
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_252_12:
    assert_252_5:
        if_32_28_252_5:
        cmp_32_28_252_5:
        cmp r15b, 0
        jne if_32_25_252_5_end
        if_32_28_252_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_252_5_end:
    assert_252_5_end:
    mov rcx, 8
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
    mov r14, 255
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 256
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_262_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 262
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 262
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_262_12:
    assert_262_5:
        if_32_28_262_5:
        cmp_32_28_262_5:
        cmp r15b, 0
        jne if_32_25_262_5_end
        if_32_28_262_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_262_5_end:
    assert_262_5_end:
    cmp_263_12:
        mov rcx, 8
        lea r14, [rbp + 504]
        mov r13, 0
        mov r12, 264
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 264
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [r14]
        lea r14, [rbp + 504]
        mov r13, 1
        mov r12, 265
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 265
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
    bool_end_263_12:
    assert_263_5:
        if_32_28_263_5:
        cmp_32_28_263_5:
        cmp r15b, 0
        jne if_32_25_263_5_end
        if_32_28_263_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_263_5_end:
    assert_263_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_270_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_270_12:
    assert_270_5:
        if_32_28_270_5:
        cmp_32_28_270_5:
        cmp r15b, 0
        jne if_32_25_270_5_end
        if_32_28_270_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_270_5_end:
    assert_270_5_end:
    cmp_271_12:
    mov r14, 0
    mov r13, 271
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_271_12:
    assert_271_5:
        if_32_28_271_5:
        cmp_32_28_271_5:
        cmp r15b, 0
        jne if_32_25_271_5_end
        if_32_28_271_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_271_5_end:
    assert_271_5_end:
    cmp_272_12:
    mov r14, 1
    mov r13, 272
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_32_28_272_5:
        cmp_32_28_272_5:
        cmp r15b, 0
        jne if_32_25_272_5_end
        if_32_28_272_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_32_25_272_5_end:
    assert_272_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_276_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_276_5_end:
    loop_277_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_280_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_280_9_end:
        print_281_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_281_9_end:
        str_in_282_9:
            mov rdi, 0
            lea rsi, [rbp + 1041]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
            sub byte [rbp + 1040], 1
        str_in_282_9_end:
        if_283_12:
        cmp_283_12:
        cmp byte [rbp + 1040], 0
        jne if_285_19
        if_283_12_code:
            jmp loop_277_5_end
        jmp if_283_9_end
        if_285_19:
        cmp_285_19:
        cmp byte [rbp + 1040], 4
        jg if_else_283_9
        if_285_19_code:
            print_286_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_286_13_end:
            jmp loop_277_5
        jmp if_283_9_end
        if_else_283_9:
            print_289_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_289_13_end:
            str_out_290_13:
                mov rdi, 1
                lea rsi, [rbp + 1041]
                movsx rdx, byte [rbp + 1040]
                mov rax, 1
                syscall
            str_out_290_13_end:
            print_291_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_291_13_end:
            print_292_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_292_13_end:
        if_283_9_end:
    jmp loop_277_5
    loop_277_5_end:
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
    if_101_8:
    cmp_101_8:
    cmp qword [rbx + 28], 0
    jge if_101_5_end
    if_101_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_101_5_end:
    mov qword [rbx + 37], 20
    loop_107_5:
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
        mov r14, 111
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
        if_113_12:
        cmp_113_12:
        cmp qword [rbx + 28], 0
        jne if_113_9_end
        if_113_12_code:
            jmp loop_107_5_end
        if_113_9_end:
    jmp loop_107_5
    loop_107_5_end:
    if_116_8:
    cmp_116_8:
    cmp byte [rbx + 36], 0
    je if_116_5_end
    if_116_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 118
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_116_5_end:
    mov qword [rbx + 45], 0
    loop_122_5:
        mov r15, qword [rbx + 45]
        mov r14, 123
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 123
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
        if_126_12:
        cmp_126_12:
        cmp qword [rbx + 37], 20
        jne if_126_9_end
        if_126_12_code:
            jmp loop_122_5_end
        if_126_9_end:
    jmp loop_122_5
    loop_122_5_end:
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
