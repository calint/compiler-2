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
    mov qword [rsp - 312], 0
    mov qword [rsp - 304], 0
    mov qword [rsp - 320], 0
    cmp_138_12:
    cmp qword [rsp - 320], 0
    sete r15b
    bool_end_138_12:
    assert_138_5:
        if_46_26_138_5:
        cmp_46_26_138_5:
        cmp r15b, 0
        jne if_46_23_138_5_end
        if_46_26_138_5_code:
            mov rdi, 1
            exit_46_32_138_5:
                    mov rax, 60
                syscall
            exit_46_32_138_5_end:
        if_46_23_138_5_end:
    assert_138_5_end:
    mov qword [rsp - 320], -1
    cmp_142_12:
    cmp qword [rsp - 320], -1
    sete r15b
    bool_end_142_12:
    assert_142_5:
        if_46_26_142_5:
        cmp_46_26_142_5:
        cmp r15b, 0
        jne if_46_23_142_5_end
        if_46_26_142_5_code:
            mov rdi, 1
            exit_46_32_142_5:
                    mov rax, 60
                syscall
            exit_46_32_142_5_end:
        if_46_23_142_5_end:
    assert_142_5_end:
        cmp_146_16:
        bool_end_146_16:
        mov r15b, 1
        assert_146_9:
            if_46_26_146_9:
            cmp_46_26_146_9:
            cmp r15b, 0
            jne if_46_23_146_9_end
            if_46_26_146_9_code:
                mov rdi, 1
                exit_46_32_146_9:
                        mov rax, 60
                    syscall
                exit_46_32_146_9_end:
            if_46_23_146_9_end:
        assert_146_9_end:
    cmp_149_12:
    bool_end_149_12:
    mov r15b, 1
    assert_149_5:
        if_46_26_149_5:
        cmp_46_26_149_5:
        cmp r15b, 0
        jne if_46_23_149_5_end
        if_46_26_149_5_code:
            mov rdi, 1
            exit_46_32_149_5:
                    mov rax, 60
                syscall
            exit_46_32_149_5_end:
        if_46_23_149_5_end:
    assert_149_5_end:
    mov qword [rsp - 328], 1
    mov r15, qword [rsp - 328]
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 312], 2
    mov r15, qword [rsp - 328]
    add r15, 1
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 328]
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 312]
    mov dword [rsp + r15 * 4 - 312], r13d
    cmp_156_12:
    mov r14, 1
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 312], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_46_26_156_5:
        cmp_46_26_156_5:
        cmp r15b, 0
        jne if_46_23_156_5_end
        if_46_26_156_5_code:
            mov rdi, 1
            exit_46_32_156_5:
                    mov rax, 60
                syscall
            exit_46_32_156_5_end:
        if_46_23_156_5_end:
    assert_156_5_end:
    cmp_157_12:
    mov r14, 2
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 312], 2
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_46_26_157_5:
        cmp_46_26_157_5:
        cmp r15b, 0
        jne if_46_23_157_5_end
        if_46_26_157_5_code:
            mov rdi, 1
            exit_46_32_157_5:
                    mov rax, 60
                syscall
            exit_46_32_157_5_end:
        if_46_23_157_5_end:
    assert_157_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 159
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 312]
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 312]
    shl rcx, 2
    rep movsb
    cmp_161_12:
    mov r14, 0
    mov r13, 161
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 312], 2
    sete r15b
    bool_end_161_12:
    assert_161_5:
        if_46_26_161_5:
        cmp_46_26_161_5:
        cmp r15b, 0
        jne if_46_23_161_5_end
        if_46_26_161_5_code:
            mov rdi, 1
            exit_46_32_161_5:
                    mov rax, 60
                syscall
            exit_46_32_161_5_end:
        if_46_23_161_5_end:
    assert_161_5_end:
    mov qword [rsp - 360], 0
    mov qword [rsp - 352], 0
    mov qword [rsp - 344], 0
    mov qword [rsp - 336], 0
    mov rcx, 4
    mov r15, 164
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 312]
    mov r15, 164
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 360]
    shl rcx, 2
    rep movsb
    cmp_165_12:
        mov rcx, 4
        mov r13, 165
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 312]
        mov r13, 165
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 360]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_165_12:
    assert_165_5:
        if_46_26_165_5:
        cmp_46_26_165_5:
        cmp r15b, 0
        jne if_46_23_165_5_end
        if_46_26_165_5_code:
            mov rdi, 1
            exit_46_32_165_5:
                    mov rax, 60
                syscall
            exit_46_32_165_5_end:
        if_46_23_165_5_end:
    assert_165_5_end:
    mov r15, 2
    mov r14, 168
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 360], -1
    cmp_169_12:
        mov rcx, 4
        mov r13, 169
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 312]
        mov r13, 169
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 360]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_169_12:
    assert_169_5:
        if_46_26_169_5:
        cmp_46_26_169_5:
        cmp r15b, 0
        jne if_46_23_169_5_end
        if_46_26_169_5_code:
            mov rdi, 1
            exit_46_32_169_5:
                    mov rax, 60
                syscall
            exit_46_32_169_5_end:
        if_46_23_169_5_end:
    assert_169_5_end:
    mov qword [rsp - 328], 3
    mov r15, qword [rsp - 328]
    mov r14, 172
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 328]
    sub r14, 1
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_172_16:
        mov r13d, dword [rsp + r14 * 4 - 312]
        mov dword [rsp + r15 * 4 - 312], r13d
        not dword [rsp + r15 * 4 - 312]
    inv_172_16_end:
    not dword [rsp + r15 * 4 - 312]
    cmp_173_12:
    mov r14, qword [rsp - 328]
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 312], 2
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_46_26_173_5:
        cmp_46_26_173_5:
        cmp r15b, 0
        jne if_46_23_173_5_end
        if_46_26_173_5_code:
            mov rdi, 1
            exit_46_32_173_5:
                    mov rax, 60
                syscall
            exit_46_32_173_5_end:
        if_46_23_173_5_end:
    assert_173_5_end:
    faz_175_5:
        mov r15, 1
        mov r14, 98
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 312], 254
    faz_175_5_end:
    cmp_176_12:
    mov r14, 1
    mov r13, 176
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 312], 254
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_46_26_176_5:
        cmp_46_26_176_5:
        cmp r15b, 0
        jne if_46_23_176_5_end
        if_46_26_176_5_code:
            mov rdi, 1
            exit_46_32_176_5:
                    mov rax, 60
                syscall
            exit_46_32_176_5_end:
        if_46_23_176_5_end:
    assert_176_5_end:
    mov qword [rsp - 376], 3
    mov qword [rsp - 368], 5
    lea r15, [rsp - 376]
    mov qword [rsp - 392], 0
    foo_179_5:
        mov r14, qword [rsp - 392]
        add qword [r15], r14
        add qword [r15], 2
        foo_179_5_continue:
            add r15, 8
            inc qword [rsp - 392]
            cmp qword [rsp - 392], 2
            jne foo_179_5
    foo_179_5_end:
    cmp_182_12:
    mov r14, 0
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rsp + r14 * 8 - 376], r13
    sete r15b
    bool_end_182_12:
    assert_182_5:
        if_46_26_182_5:
        cmp_46_26_182_5:
        cmp r15b, 0
        jne if_46_23_182_5_end
        if_46_26_182_5_code:
            mov rdi, 1
            exit_46_32_182_5:
                    mov rax, 60
                syscall
            exit_46_32_182_5_end:
        if_46_23_182_5_end:
    assert_182_5_end:
    cmp_183_12:
    mov r14, 1
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rsp + r14 * 8 - 376], r13
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_46_26_183_5:
        cmp_46_26_183_5:
        cmp r15b, 0
        jne if_46_23_183_5_end
        if_46_26_183_5_code:
            mov rdi, 1
            exit_46_32_183_5:
                    mov rax, 60
                syscall
            exit_46_32_183_5_end:
        if_46_23_183_5_end:
    assert_183_5_end:
    mov qword [rsp - 392], 0
    mov qword [rsp - 384], 0
    fooz_190_5:
        mov qword [rsp - 392], 2
        mov qword [rsp - 384], 11
    fooz_190_5_end:
    cmp_191_12:
    cmp qword [rsp - 392], 2
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_46_26_191_5:
        cmp_46_26_191_5:
        cmp r15b, 0
        jne if_46_23_191_5_end
        if_46_26_191_5_code:
            mov rdi, 1
            exit_46_32_191_5:
                    mov rax, 60
                syscall
            exit_46_32_191_5_end:
        if_46_23_191_5_end:
    assert_191_5_end:
    cmp_192_12:
    cmp qword [rsp - 384], 11
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_46_26_192_5:
        cmp_46_26_192_5:
        cmp r15b, 0
        jne if_46_23_192_5_end
        if_46_26_192_5_code:
            mov rdi, 1
            exit_46_32_192_5:
                    mov rax, 60
                syscall
            exit_46_32_192_5_end:
        if_46_23_192_5_end:
    assert_192_5_end:
    mov rax, qword [rsp - 392]
    mov qword [rsp - 408], rax
    mov rax, qword [rsp - 384]
    mov qword [rsp - 400], rax
    cmp_195_12:
        lea rsi, [rsp - 392]
        lea rdi, [rsp - 408]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_195_12:
    assert_195_5:
        if_46_26_195_5:
        cmp_46_26_195_5:
        cmp r15b, 0
        jne if_46_23_195_5_end
        if_46_26_195_5_code:
            mov rdi, 1
            exit_46_32_195_5:
                    mov rax, 60
                syscall
            exit_46_32_195_5_end:
        if_46_23_195_5_end:
    assert_195_5_end:
    mov qword [rsp - 408], 3
    cmp_200_12:
        lea rsi, [rsp - 392]
        lea rdi, [rsp - 408]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_200_12:
    assert_200_5:
        if_46_26_200_5:
        cmp_46_26_200_5:
        cmp r15b, 0
        jne if_46_23_200_5_end
        if_46_26_200_5_code:
            mov rdi, 1
            exit_46_32_200_5:
                    mov rax, 60
                syscall
            exit_46_32_200_5_end:
        if_46_23_200_5_end:
    assert_200_5_end:
    mov qword [rsp - 416], 0
    bar_203_5:
        if_79_8_203_5:
        cmp_79_8_203_5:
        cmp qword [rsp - 416], 0
        jne if_79_5_203_5_end
        if_79_8_203_5_code:
            jmp bar_203_5_end
        if_79_5_203_5_end:
        mov qword [rsp - 416], 255
    bar_203_5_end:
    cmp_204_12:
    cmp qword [rsp - 416], 0
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_46_26_204_5:
        cmp_46_26_204_5:
        cmp r15b, 0
        jne if_46_23_204_5_end
        if_46_26_204_5_code:
            mov rdi, 1
            exit_46_32_204_5:
                    mov rax, 60
                syscall
            exit_46_32_204_5_end:
        if_46_23_204_5_end:
    assert_204_5_end:
    mov qword [rsp - 416], 1
    bar_207_5:
        if_79_8_207_5:
        cmp_79_8_207_5:
        cmp qword [rsp - 416], 0
        jne if_79_5_207_5_end
        if_79_8_207_5_code:
            jmp bar_207_5_end
        if_79_5_207_5_end:
        mov qword [rsp - 416], 255
    bar_207_5_end:
    cmp_208_12:
    cmp qword [rsp - 416], 255
    sete r15b
    bool_end_208_12:
    assert_208_5:
        if_46_26_208_5:
        cmp_46_26_208_5:
        cmp r15b, 0
        jne if_46_23_208_5_end
        if_46_26_208_5_code:
            mov rdi, 1
            exit_46_32_208_5:
                    mov rax, 60
                syscall
            exit_46_32_208_5_end:
        if_46_23_208_5_end:
    assert_208_5_end:
    mov qword [rsp - 424], 1
    baz_211_13:
        mov r15, qword [rsp - 424]
        imul r15, 2
        mov qword [rsp - 432], r15
    baz_211_13_end:
    cmp_212_12:
    cmp qword [rsp - 432], 2
    sete r15b
    bool_end_212_12:
    assert_212_5:
        if_46_26_212_5:
        cmp_46_26_212_5:
        cmp r15b, 0
        jne if_46_23_212_5_end
        if_46_26_212_5_code:
            mov rdi, 1
            exit_46_32_212_5:
                    mov rax, 60
                syscall
            exit_46_32_212_5_end:
        if_46_23_212_5_end:
    assert_212_5_end:
    baz_214_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 432], r15
    baz_214_9_end:
    cmp_215_12:
    cmp qword [rsp - 432], 2
    sete r15b
    bool_end_215_12:
    assert_215_5:
        if_46_26_215_5:
        cmp_46_26_215_5:
        cmp r15b, 0
        jne if_46_23_215_5_end
        if_46_26_215_5_code:
            mov rdi, 1
            exit_46_32_215_5:
                    mov rax, 60
                syscall
            exit_46_32_215_5_end:
        if_46_23_215_5_end:
    assert_215_5_end:
    baz_217_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 448], r15
    baz_217_23_end:
    mov qword [rsp - 440], 0
    cmp_218_12:
    cmp qword [rsp - 448], 4
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_46_26_218_5:
        cmp_46_26_218_5:
        cmp r15b, 0
        jne if_46_23_218_5_end
        if_46_26_218_5_code:
            mov rdi, 1
            exit_46_32_218_5:
                    mov rax, 60
                syscall
            exit_46_32_218_5_end:
        if_46_23_218_5_end:
    assert_218_5_end:
    point_init_220_22:
        mov qword [rsp - 464], -1
        mov qword [rsp - 456], -2
    point_init_220_22_end:
    cmp_221_12:
    cmp qword [rsp - 464], -1
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_46_26_221_5:
        cmp_46_26_221_5:
        cmp r15b, 0
        jne if_46_23_221_5_end
        if_46_26_221_5_code:
            mov rdi, 1
            exit_46_32_221_5:
                    mov rax, 60
                syscall
            exit_46_32_221_5_end:
        if_46_23_221_5_end:
    assert_221_5_end:
    cmp_222_12:
    cmp qword [rsp - 456], -2
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_46_26_222_5:
        cmp_46_26_222_5:
        cmp r15b, 0
        jne if_46_23_222_5_end
        if_46_26_222_5_code:
            mov rdi, 1
            exit_46_32_222_5:
                    mov rax, 60
                syscall
            exit_46_32_222_5_end:
        if_46_23_222_5_end:
    assert_222_5_end:
    mov qword [rsp - 472], 1
    mov qword [rsp - 480], 2
    mov r15, qword [rsp - 472]
    imul r15, 10
    mov qword [rsp - 500], r15
    mov r15, qword [rsp - 480]
    mov qword [rsp - 492], r15
    mov dword [rsp - 484], 16711680
    cmp_228_12:
    cmp qword [rsp - 500], 10
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_46_26_228_5:
        cmp_46_26_228_5:
        cmp r15b, 0
        jne if_46_23_228_5_end
        if_46_26_228_5_code:
            mov rdi, 1
            exit_46_32_228_5:
                    mov rax, 60
                syscall
            exit_46_32_228_5_end:
        if_46_23_228_5_end:
    assert_228_5_end:
    cmp_229_12:
    cmp qword [rsp - 492], 2
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_46_26_229_5:
        cmp_46_26_229_5:
        cmp r15b, 0
        jne if_46_23_229_5_end
        if_46_26_229_5_code:
            mov rdi, 1
            exit_46_32_229_5:
                    mov rax, 60
                syscall
            exit_46_32_229_5_end:
        if_46_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    cmp dword [rsp - 484], 16711680
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_46_26_230_5:
        cmp_46_26_230_5:
        cmp r15b, 0
        jne if_46_23_230_5_end
        if_46_26_230_5_code:
            mov rdi, 1
            exit_46_32_230_5:
                    mov rax, 60
                syscall
            exit_46_32_230_5_end:
        if_46_23_230_5_end:
    assert_230_5_end:
    mov r15, qword [rsp - 472]
    mov qword [rsp - 516], r15
    neg qword [rsp - 516]
    mov r15, qword [rsp - 480]
    mov qword [rsp - 508], r15
    neg qword [rsp - 508]
    mov rax, qword [rsp - 516]
    mov qword [rsp - 500], rax
    mov rax, qword [rsp - 508]
    mov qword [rsp - 492], rax
    cmp_234_12:
    cmp qword [rsp - 500], -1
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_46_26_234_5:
        cmp_46_26_234_5:
        cmp r15b, 0
        jne if_46_23_234_5_end
        if_46_26_234_5_code:
            mov rdi, 1
            exit_46_32_234_5:
                    mov rax, 60
                syscall
            exit_46_32_234_5_end:
        if_46_23_234_5_end:
    assert_234_5_end:
    cmp_235_12:
    cmp qword [rsp - 492], -2
    sete r15b
    bool_end_235_12:
    assert_235_5:
        if_46_26_235_5:
        cmp_46_26_235_5:
        cmp r15b, 0
        jne if_46_23_235_5_end
        if_46_26_235_5_code:
            mov rdi, 1
            exit_46_32_235_5:
                    mov rax, 60
                syscall
            exit_46_32_235_5_end:
        if_46_23_235_5_end:
    assert_235_5_end:
    lea rsi, [rsp - 500]
    lea rdi, [rsp - 536]
    mov rcx, 20
    rep movsb
    cmp_238_12:
    cmp qword [rsp - 536], -1
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_46_26_238_5:
        cmp_46_26_238_5:
        cmp r15b, 0
        jne if_46_23_238_5_end
        if_46_26_238_5_code:
            mov rdi, 1
            exit_46_32_238_5:
                    mov rax, 60
                syscall
            exit_46_32_238_5_end:
        if_46_23_238_5_end:
    assert_238_5_end:
    cmp_239_12:
    cmp qword [rsp - 528], -2
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_46_26_239_5:
        cmp_46_26_239_5:
        cmp r15b, 0
        jne if_46_23_239_5_end
        if_46_26_239_5_code:
            mov rdi, 1
            exit_46_32_239_5:
                    mov rax, 60
                syscall
            exit_46_32_239_5_end:
        if_46_23_239_5_end:
    assert_239_5_end:
    cmp_240_12:
    cmp dword [rsp - 520], 16711680
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_46_26_240_5:
        cmp_46_26_240_5:
        cmp r15b, 0
        jne if_46_23_240_5_end
        if_46_26_240_5_code:
            mov rdi, 1
            exit_46_32_240_5:
                    mov rax, 60
                syscall
            exit_46_32_240_5_end:
        if_46_23_240_5_end:
    assert_240_5_end:
    xor al, al
    lea rdi, [rsp - 576]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 568], 73
    cmp_246_12:
    lea r14, [rsp - 576]
    mov r13, 0
    mov r12, 246
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
    bool_end_246_12:
    assert_246_5:
        if_46_26_246_5:
        cmp_46_26_246_5:
        cmp r15b, 0
        jne if_46_23_246_5_end
        if_46_26_246_5_code:
            mov rdi, 1
            exit_46_32_246_5:
                    mov rax, 60
                syscall
            exit_46_32_246_5_end:
        if_46_23_246_5_end:
    assert_246_5_end:
    lea r15, [rsp - 576]
    mov r14, 1
    mov r13, 248
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_248_13:
        mov qword [r15 + 8], 74
    object_init_248_13_end:
    cmp_249_12:
    lea r14, [rsp - 576]
    mov r13, 1
    mov r12, 249
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
    bool_end_249_12:
    assert_249_5:
        if_46_26_249_5:
        cmp_46_26_249_5:
        cmp r15b, 0
        jne if_46_23_249_5_end
        if_46_26_249_5_code:
            mov rdi, 1
            exit_46_32_249_5:
                    mov rax, 60
                syscall
            exit_46_32_249_5_end:
        if_46_23_249_5_end:
    assert_249_5_end:
    xor al, al
    lea rdi, [rsp - 1088]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 1088]
    mov r14, 1
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_253_12:
    lea r14, [rsp - 1088]
    mov r13, 1
    mov r12, 253
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 253
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_253_12:
    assert_253_5:
        if_46_26_253_5:
        cmp_46_26_253_5:
        cmp r15b, 0
        jne if_46_23_253_5_end
        if_46_26_253_5_code:
            mov rdi, 1
            exit_46_32_253_5:
                    mov rax, 60
                syscall
            exit_46_32_253_5_end:
        if_46_23_253_5_end:
    assert_253_5_end:
    mov rcx, 8
    lea r15, [rsp - 1088]
    mov r14, 1
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 256
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 1088]
    mov r14, 0
    mov r13, 257
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 257
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_263_12:
    lea r14, [rsp - 1088]
    mov r13, 0
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_263_12:
    assert_263_5:
        if_46_26_263_5:
        cmp_46_26_263_5:
        cmp r15b, 0
        jne if_46_23_263_5_end
        if_46_26_263_5_code:
            mov rdi, 1
            exit_46_32_263_5:
                    mov rax, 60
                syscall
            exit_46_32_263_5_end:
        if_46_23_263_5_end:
    assert_263_5_end:
    cmp_264_12:
        mov rcx, 8
        lea r13, [rsp - 1088]
        mov r12, 0
        mov r11, 265
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 265
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 1088]
        mov r12, 1
        mov r11, 266
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 266
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
    bool_end_264_12:
    assert_264_5:
        if_46_26_264_5:
        cmp_46_26_264_5:
        cmp r15b, 0
        jne if_46_23_264_5_end
        if_46_26_264_5_code:
            mov rdi, 1
            exit_46_32_264_5:
                    mov rax, 60
                syscall
            exit_46_32_264_5_end:
        if_46_23_264_5_end:
    assert_264_5_end:
    mov qword [rsp - 1104], -1
    mov qword [rsp - 1096], 2
    cmp_271_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_271_12:
    assert_271_5:
        if_46_26_271_5:
        cmp_46_26_271_5:
        cmp r15b, 0
        jne if_46_23_271_5_end
        if_46_26_271_5_code:
            mov rdi, 1
            exit_46_32_271_5:
                    mov rax, 60
                syscall
            exit_46_32_271_5_end:
        if_46_23_271_5_end:
    assert_271_5_end:
    cmp_272_12:
    mov r14, 0
    mov r13, 272
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1104], -1
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_46_26_272_5:
        cmp_46_26_272_5:
        cmp r15b, 0
        jne if_46_23_272_5_end
        if_46_26_272_5_code:
            mov rdi, 1
            exit_46_32_272_5:
                    mov rax, 60
                syscall
            exit_46_32_272_5_end:
        if_46_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
    mov r14, 1
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1104], 2
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_46_26_273_5:
        cmp_46_26_273_5:
        cmp r15b, 0
        jne if_46_23_273_5_end
        if_46_26_273_5_code:
            mov rdi, 1
            exit_46_32_273_5:
                    mov rax, 60
                syscall
            exit_46_32_273_5_end:
        if_46_23_273_5_end:
    assert_273_5_end:
    xor al, al
    lea rdi, [rsp - 1232]
    mov rcx, 128
    rep stosb
    print_276_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_57_4_276_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_57_4_276_5_end:
    print_276_5_end:
    loop_277_5:
        print_278_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_57_4_278_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_57_4_278_9_end:
        print_278_9_end:
        str_in_279_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1231]
                mov rdx, 127
            syscall
                mov byte [rsp - 1232], al
                sub byte [rsp - 1232], 1
        str_in_279_9_end:
        if_280_12:
        cmp_280_12:
        cmp byte [rsp - 1232], 0
        jne if_282_19
        if_280_12_code:
            jmp loop_277_5_end
        jmp if_280_9_end
        if_282_19:
        cmp_282_19:
        cmp byte [rsp - 1232], 4
        jg if_else_280_9
        if_282_19_code:
            print_283_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_57_4_283_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_283_13_end:
            print_283_13_end:
            jmp loop_277_5
        jmp if_280_9_end
        if_else_280_9:
            print_286_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_57_4_286_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_286_13_end:
            print_286_13_end:
            str_out_287_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1231]
                    movsx rdx, byte [rsp - 1232]
                syscall
            str_out_287_13_end:
            print_288_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_57_4_288_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_288_13_end:
            print_288_13_end:
            print_289_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_57_4_289_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_289_13_end:
            print_289_13_end:
        if_280_9_end:
    jmp loop_277_5
    loop_277_5_end:
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
dq 1
dq 2
dq 3
dq 4
times 16 db 0
dq 1
dq 2
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
