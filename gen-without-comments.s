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
    cmp_139_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_139_12:
    assert_139_5:
        if_34_26_139_5:
        cmp_34_26_139_5:
        cmp r15b, 0
        jne if_34_23_139_5_end
        if_34_26_139_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_139_5_end:
    assert_139_5_end:
    mov qword [rbp + 240], -1
    cmp_143_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_143_12:
    assert_143_5:
        if_34_26_143_5:
        cmp_34_26_143_5:
        cmp r15b, 0
        jne if_34_23_143_5_end
        if_34_26_143_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_143_5_end:
    assert_143_5_end:
        cmp_147_16:
        bool_end_147_16:
        mov r15b, 1
        assert_147_9:
            if_34_26_147_9:
            cmp_34_26_147_9:
            cmp r15b, 0
            jne if_34_23_147_9_end
            if_34_26_147_9_code:
                mov rdi, 1
                mov rax, 60
                syscall
            if_34_23_147_9_end:
        assert_147_9_end:
    cmp_150_12:
    bool_end_150_12:
    mov r15b, 1
    assert_150_5:
        if_34_26_150_5:
        cmp_34_26_150_5:
        cmp r15b, 0
        jne if_34_23_150_5_end
        if_34_26_150_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_150_5_end:
    assert_150_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 156
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_157_12:
    mov r14, 1
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_34_26_157_5:
        cmp_34_26_157_5:
        cmp r15b, 0
        jne if_34_23_157_5_end
        if_34_26_157_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_157_5_end:
    assert_157_5_end:
    cmp_158_12:
    mov r14, 2
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_158_12:
    assert_158_5:
        if_34_26_158_5:
        cmp_34_26_158_5:
        cmp r15b, 0
        jne if_34_23_158_5_end
        if_34_26_158_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_158_5_end:
    assert_158_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 160
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 160
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_162_12:
    mov r14, 0
    mov r13, 162
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_162_12:
    assert_162_5:
        if_34_26_162_5:
        cmp_34_26_162_5:
        cmp r15b, 0
        jne if_34_23_162_5_end
        if_34_26_162_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_162_5_end:
    assert_162_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 165
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 165
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_166_12:
        mov rcx, 3
        mov r14, 1
        mov r13, 166
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
        mov r13, 166
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
    bool_end_166_12:
    assert_166_5:
        if_34_26_166_5:
        cmp_34_26_166_5:
        cmp r15b, 0
        jne if_34_23_166_5_end
        if_34_26_166_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_166_5_end:
    assert_166_5_end:
    mov r15, 2
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_170_12:
        mov rcx, 4
        mov r14, 170
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 170
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
    bool_end_170_12:
    assert_170_5:
        if_34_26_170_5:
        cmp_34_26_170_5:
        cmp r15b, 0
        jne if_34_23_170_5_end
        if_34_26_170_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_170_5_end:
    assert_170_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 173
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_173_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_173_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_174_12:
    mov r14, qword [rbp + 248]
    mov r13, 174
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_174_12:
    assert_174_5:
        if_34_26_174_5:
        cmp_34_26_174_5:
        cmp r15b, 0
        jne if_34_23_174_5_end
        if_34_26_174_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_174_5_end:
    assert_174_5_end:
    faz_176_5:
        mov r15, 1
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_176_5_end:
    cmp_177_12:
    mov r14, 1
    mov r13, 177
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_34_26_177_5:
        cmp_34_26_177_5:
        cmp r15b, 0
        jne if_34_23_177_5_end
        if_34_26_177_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_177_5_end:
    assert_177_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_180_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_180_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_180_5
    foo_180_5_end:
    cmp_183_12:
    mov r14, 0
    mov r13, 183
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
    bool_end_183_12:
    assert_183_5:
        if_34_26_183_5:
        cmp_34_26_183_5:
        cmp r15b, 0
        jne if_34_23_183_5_end
        if_34_26_183_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_183_5_end:
    assert_183_5_end:
    cmp_184_12:
    mov r14, 1
    mov r13, 184
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
    bool_end_184_12:
    assert_184_5:
        if_34_26_184_5:
        cmp_34_26_184_5:
        cmp r15b, 0
        jne if_34_23_184_5_end
        if_34_26_184_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_184_5_end:
    assert_184_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_191_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_191_5_end:
    cmp_192_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_34_26_192_5:
        cmp_34_26_192_5:
        cmp r15b, 0
        jne if_34_23_192_5_end
        if_34_26_192_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_192_5_end:
    assert_192_5_end:
    cmp_193_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_193_12:
    assert_193_5:
        if_34_26_193_5:
        cmp_34_26_193_5:
        cmp r15b, 0
        jne if_34_23_193_5_end
        if_34_26_193_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_193_5_end:
    assert_193_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_196_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r15b
    bool_end_196_12:
    assert_196_5:
        if_34_26_196_5:
        cmp_34_26_196_5:
        cmp r15b, 0
        jne if_34_23_196_5_end
        if_34_26_196_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_196_5_end:
    assert_196_5_end:
    mov qword [rbp + 320], 3
    cmp_201_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool_end_201_12:
    assert_201_5:
        if_34_26_201_5:
        cmp_34_26_201_5:
        cmp r15b, 0
        jne if_34_23_201_5_end
        if_34_26_201_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_201_5_end:
    assert_201_5_end:
    mov qword [rbp + 336], 0
    bar_204_5:
        if_50_8_204_5:
        cmp_50_8_204_5:
        cmp qword [rbp + 336], 0
        jne if_50_5_204_5_end
        if_50_8_204_5_code:
            jmp bar_204_5_end
        if_50_5_204_5_end:
        mov qword [rbp + 336], 255
    bar_204_5_end:
    cmp_205_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_34_26_205_5:
        cmp_34_26_205_5:
        cmp r15b, 0
        jne if_34_23_205_5_end
        if_34_26_205_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_205_5_end:
    assert_205_5_end:
    mov qword [rbp + 336], 1
    bar_208_5:
        if_50_8_208_5:
        cmp_50_8_208_5:
        cmp qword [rbp + 336], 0
        jne if_50_5_208_5_end
        if_50_8_208_5_code:
            jmp bar_208_5_end
        if_50_5_208_5_end:
        mov qword [rbp + 336], 255
    bar_208_5_end:
    cmp_209_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_209_12:
    assert_209_5:
        if_34_26_209_5:
        cmp_34_26_209_5:
        cmp r15b, 0
        jne if_34_23_209_5_end
        if_34_26_209_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_209_5_end:
    assert_209_5_end:
    mov qword [rbp + 344], 1
    baz_212_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_212_13_end:
    cmp_213_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_34_26_213_5:
        cmp_34_26_213_5:
        cmp r15b, 0
        jne if_34_23_213_5_end
        if_34_26_213_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_213_5_end:
    assert_213_5_end:
    baz_215_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_215_9_end:
    cmp_216_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_34_26_216_5:
        cmp_34_26_216_5:
        cmp r15b, 0
        jne if_34_23_216_5_end
        if_34_26_216_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_216_5_end:
    assert_216_5_end:
    baz_218_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_218_23_end:
    mov qword [rbp + 368], 0
    cmp_219_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_219_12:
    assert_219_5:
        if_34_26_219_5:
        cmp_34_26_219_5:
        cmp r15b, 0
        jne if_34_23_219_5_end
        if_34_26_219_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_219_5_end:
    assert_219_5_end:
    point_init_221_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_221_22_end:
    cmp_222_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_34_26_222_5:
        cmp_34_26_222_5:
        cmp r15b, 0
        jne if_34_23_222_5_end
        if_34_26_222_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_222_5_end:
    assert_222_5_end:
    cmp_223_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_223_12:
    assert_223_5:
        if_34_26_223_5:
        cmp_34_26_223_5:
        cmp r15b, 0
        jne if_34_23_223_5_end
        if_34_26_223_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_223_5_end:
    assert_223_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_229_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_34_26_229_5:
        cmp_34_26_229_5:
        cmp r15b, 0
        jne if_34_23_229_5_end
        if_34_26_229_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_34_26_230_5:
        cmp_34_26_230_5:
        cmp r15b, 0
        jne if_34_23_230_5_end
        if_34_26_230_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_230_5_end:
    assert_230_5_end:
    cmp_231_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_231_12:
    assert_231_5:
        if_34_26_231_5:
        cmp_34_26_231_5:
        cmp r15b, 0
        jne if_34_23_231_5_end
        if_34_26_231_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_231_5_end:
    assert_231_5_end:
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
    cmp_235_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_235_12:
    assert_235_5:
        if_34_26_235_5:
        cmp_34_26_235_5:
        cmp r15b, 0
        jne if_34_23_235_5_end
        if_34_26_235_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_235_5_end:
    assert_235_5_end:
    cmp_236_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_34_26_236_5:
        cmp_34_26_236_5:
        cmp r15b, 0
        jne if_34_23_236_5_end
        if_34_26_236_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_236_5_end:
    assert_236_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_239_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_34_26_239_5:
        cmp_34_26_239_5:
        cmp r15b, 0
        jne if_34_23_239_5_end
        if_34_26_239_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_239_5_end:
    assert_239_5_end:
    cmp_240_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_34_26_240_5:
        cmp_34_26_240_5:
        cmp r15b, 0
        jne if_34_23_240_5_end
        if_34_26_240_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_240_5_end:
    assert_240_5_end:
    cmp_241_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_241_12:
    assert_241_5:
        if_34_26_241_5:
        cmp_34_26_241_5:
        cmp r15b, 0
        jne if_34_23_241_5_end
        if_34_26_241_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_241_5_end:
    assert_241_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_247_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 247
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
    bool_end_247_12:
    assert_247_5:
        if_34_26_247_5:
        cmp_34_26_247_5:
        cmp r15b, 0
        jne if_34_23_247_5_end
        if_34_26_247_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_247_5_end:
    assert_247_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 249
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_249_13:
        mov qword [r15 + 8], 74
    object_init_249_13_end:
    cmp_250_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 250
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
    bool_end_250_12:
    assert_250_5:
        if_34_26_250_5:
        cmp_34_26_250_5:
        cmp r15b, 0
        jne if_34_23_250_5_end
        if_34_26_250_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_250_5_end:
    assert_250_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 253
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 253
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_254_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 254
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 254
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_254_12:
    assert_254_5:
        if_34_26_254_5:
        cmp_34_26_254_5:
        cmp r15b, 0
        jne if_34_23_254_5_end
        if_34_26_254_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_254_5_end:
    assert_254_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 257
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 257
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 258
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_264_12:
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
        if_34_26_264_5:
        cmp_34_26_264_5:
        cmp r15b, 0
        jne if_34_23_264_5_end
        if_34_26_264_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_264_5_end:
    assert_264_5_end:
    cmp_265_12:
        mov rcx, 8
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
        mov r13, 266
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [r14]
        lea r14, [rbp + 504]
        mov r13, 1
        mov r12, 267
        test r13, r13
        cmovs rbp, r12
        js baz_bounds_panic
        cmp r13, 8
        cmovge rbp, r12
        jge baz_bounds_panic
        shl r13, 6
        add r14, r13
        mov r13, 267
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
    bool_end_265_12:
    assert_265_5:
        if_34_26_265_5:
        cmp_34_26_265_5:
        cmp r15b, 0
        jne if_34_23_265_5_end
        if_34_26_265_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_265_5_end:
    assert_265_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_272_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_34_26_272_5:
        cmp_34_26_272_5:
        cmp r15b, 0
        jne if_34_23_272_5_end
        if_34_26_272_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
    mov r14, 0
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_34_26_273_5:
        cmp_34_26_273_5:
        cmp r15b, 0
        jne if_34_23_273_5_end
        if_34_26_273_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_273_5_end:
    assert_273_5_end:
    cmp_274_12:
    mov r14, 1
    mov r13, 274
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_34_26_274_5:
        cmp_34_26_274_5:
        cmp r15b, 0
        jne if_34_23_274_5_end
        if_34_26_274_5_code:
            mov rdi, 1
            mov rax, 60
            syscall
        if_34_23_274_5_end:
    assert_274_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_278_5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    print_278_5_end:
    loop_279_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_282_9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        print_282_9_end:
        print_283_9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        print_283_9_end:
        str_in_284_9:
            mov rdi, 0
            lea rsi, [rbp + 1041]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1168], rax
            mov r15b, byte [rbp + 1168]
            mov byte [rbp + 1040], r15b
            sub byte [rbp + 1040], 1
        str_in_284_9_end:
        if_285_12:
        cmp_285_12:
        cmp byte [rbp + 1040], 0
        jne if_287_19
        if_285_12_code:
            jmp loop_279_5_end
        jmp if_285_9_end
        if_287_19:
        cmp_287_19:
        cmp byte [rbp + 1040], 4
        jg if_else_285_9
        if_287_19_code:
            print_288_13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            print_288_13_end:
            jmp loop_279_5
        jmp if_285_9_end
        if_else_285_9:
            print_291_13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            print_291_13_end:
            str_out_292_13:
                mov rdi, 1
                lea rsi, [rbp + 1041]
                movsx rdx, byte [rbp + 1040]
                mov rax, 1
                syscall
            str_out_292_13_end:
            print_293_13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            print_293_13_end:
            print_294_13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            print_294_13_end:
        if_285_9_end:
    jmp loop_279_5
    loop_279_5_end:
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
    if_103_8:
    cmp_103_8:
    cmp qword [rbx + 28], 0
    jge if_103_5_end
    if_103_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_103_5_end:
    mov qword [rbx + 37], 20
    loop_109_5:
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
        mov r14, 113
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
        if_115_12:
        cmp_115_12:
        cmp qword [rbx + 28], 0
        jne if_115_9_end
        if_115_12_code:
            jmp loop_109_5_end
        if_115_9_end:
    jmp loop_109_5
    loop_109_5_end:
    if_118_8:
    cmp_118_8:
    cmp byte [rbx + 36], 0
    je if_118_5_end
    if_118_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 120
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_118_5_end:
    mov qword [rbx + 45], 0
    loop_124_5:
        mov r15, qword [rbx + 45]
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 125
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
        if_128_12:
        cmp_128_12:
        cmp qword [rbx + 37], 20
        jne if_128_9_end
        if_128_12_code:
            jmp loop_124_5_end
        if_128_9_end:
    jmp loop_124_5
    loop_124_5_end:
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
