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
    mov qword [rsp - 205], 0
    mov qword [rsp - 197], 0
    mov qword [rsp - 213], 0
    cmp_130_12:
    cmp qword [rsp - 213], 0
    sete r15b
    bool_end_130_12:
    assert_130_5:
        if_13_29_130_5:
        cmp_13_29_130_5:
        cmp r15b, 0
        jne if_13_26_130_5_end
        if_13_29_130_5_code:
            mov rdi, 1
            exit_13_38_130_5:
                    mov rax, 60
                syscall
            exit_13_38_130_5_end:
        if_13_26_130_5_end:
    assert_130_5_end:
    mov qword [rsp - 213], -1
    cmp_134_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_134_12:
    assert_134_5:
        if_13_29_134_5:
        cmp_13_29_134_5:
        cmp r15b, 0
        jne if_13_26_134_5_end
        if_13_29_134_5_code:
            mov rdi, 1
            exit_13_38_134_5:
                    mov rax, 60
                syscall
            exit_13_38_134_5_end:
        if_13_26_134_5_end:
    assert_134_5_end:
    mov qword [rsp - 221], 1
    mov r15, qword [rsp - 221]
    mov r14, 139
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, 140
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    mov r13, 140
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_141_12:
    mov r14, 1
    mov r13, 141
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_141_12:
    assert_141_5:
        if_13_29_141_5:
        cmp_13_29_141_5:
        cmp r15b, 0
        jne if_13_26_141_5_end
        if_13_29_141_5_code:
            mov rdi, 1
            exit_13_38_141_5:
                    mov rax, 60
                syscall
            exit_13_38_141_5_end:
        if_13_26_141_5_end:
    assert_141_5_end:
    cmp_142_12:
    mov r14, 2
    mov r13, 142
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_142_12:
    assert_142_5:
        if_13_29_142_5:
        cmp_13_29_142_5:
        cmp r15b, 0
        jne if_13_26_142_5_end
        if_13_29_142_5_code:
            mov rdi, 1
            exit_13_38_142_5:
                    mov rax, 60
                syscall
            exit_13_38_142_5_end:
        if_13_26_142_5_end:
    assert_142_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 144
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 205]
    mov r15, 144
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_146_12:
    mov r14, 0
    mov r13, 146
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_146_12:
    assert_146_5:
        if_13_29_146_5:
        cmp_13_29_146_5:
        cmp r15b, 0
        jne if_13_26_146_5_end
        if_13_29_146_5_code:
            mov rdi, 1
            exit_13_38_146_5:
                    mov rax, 60
                syscall
            exit_13_38_146_5_end:
        if_13_26_146_5_end:
    assert_146_5_end:
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov rcx, 4
    mov r15, 149
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 205]
    mov r15, 149
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_150_12:
        mov rcx, 4
        mov r13, 150
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 150
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_150_12:
    assert_150_5:
        if_13_29_150_5:
        cmp_13_29_150_5:
        cmp r15b, 0
        jne if_13_26_150_5_end
        if_13_29_150_5_code:
            mov rdi, 1
            exit_13_38_150_5:
                    mov rax, 60
                syscall
            exit_13_38_150_5_end:
        if_13_26_150_5_end:
    assert_150_5_end:
    mov r15, 2
    mov r14, 153
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 253], -1
    cmp_154_12:
        mov rcx, 4
        mov r13, 154
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 154
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_154_12:
    assert_154_5:
        if_13_29_154_5:
        cmp_13_29_154_5:
        cmp r15b, 0
        jne if_13_26_154_5_end
        if_13_29_154_5_code:
            mov rdi, 1
            exit_13_38_154_5:
                    mov rax, 60
                syscall
            exit_13_38_154_5_end:
        if_13_26_154_5_end:
    assert_154_5_end:
    mov qword [rsp - 221], 3
    mov r15, qword [rsp - 221]
    mov r14, 157
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    sub r14, 1
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_157_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_157_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_158_12:
    mov r14, qword [rsp - 221]
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_158_12:
    assert_158_5:
        if_13_29_158_5:
        cmp_13_29_158_5:
        cmp r15b, 0
        jne if_13_26_158_5_end
        if_13_29_158_5_code:
            mov rdi, 1
            exit_13_38_158_5:
                    mov rax, 60
                syscall
            exit_13_38_158_5_end:
        if_13_26_158_5_end:
    assert_158_5_end:
    faz_160_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 205], 254
    faz_160_5_end:
    cmp_161_12:
    mov r14, 1
    mov r13, 161
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 254
    sete r15b
    bool_end_161_12:
    assert_161_5:
        if_13_29_161_5:
        cmp_13_29_161_5:
        cmp r15b, 0
        jne if_13_26_161_5_end
        if_13_29_161_5_code:
            mov rdi, 1
            exit_13_38_161_5:
                    mov rax, 60
                syscall
            exit_13_38_161_5_end:
        if_13_26_161_5_end:
    assert_161_5_end:
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    foo_164_5:
        mov qword [rsp - 269], 2
        mov qword [rsp - 261], 11
    foo_164_5_end:
    cmp_165_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_165_12:
    assert_165_5:
        if_13_29_165_5:
        cmp_13_29_165_5:
        cmp r15b, 0
        jne if_13_26_165_5_end
        if_13_29_165_5_code:
            mov rdi, 1
            exit_13_38_165_5:
                    mov rax, 60
                syscall
            exit_13_38_165_5_end:
        if_13_26_165_5_end:
    assert_165_5_end:
    cmp_166_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_13_29_166_5:
        cmp_13_29_166_5:
        cmp r15b, 0
        jne if_13_26_166_5_end
        if_13_29_166_5_code:
            mov rdi, 1
            exit_13_38_166_5:
                    mov rax, 60
                syscall
            exit_13_38_166_5_end:
        if_13_26_166_5_end:
    assert_166_5_end:
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
    cmp_169_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_169_12:
    assert_169_5:
        if_13_29_169_5:
        cmp_13_29_169_5:
        cmp r15b, 0
        jne if_13_26_169_5_end
        if_13_29_169_5_code:
            mov rdi, 1
            exit_13_38_169_5:
                    mov rax, 60
                syscall
            exit_13_38_169_5_end:
        if_13_26_169_5_end:
    assert_169_5_end:
    mov qword [rsp - 285], 3
    cmp_174_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_174_12:
    assert_174_5:
        if_13_29_174_5:
        cmp_13_29_174_5:
        cmp r15b, 0
        jne if_13_26_174_5_end
        if_13_29_174_5_code:
            mov rdi, 1
            exit_13_38_174_5:
                    mov rax, 60
                syscall
            exit_13_38_174_5_end:
        if_13_26_174_5_end:
    assert_174_5_end:
    mov qword [rsp - 293], 0
    bar_177_5:
        if_56_8_177_5:
        cmp_56_8_177_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_177_5_end
        if_56_8_177_5_code:
            jmp bar_177_5_end
        if_56_5_177_5_end:
        mov qword [rsp - 293], 255
    bar_177_5_end:
    cmp_178_12:
    cmp qword [rsp - 293], 0
    sete r15b
    bool_end_178_12:
    assert_178_5:
        if_13_29_178_5:
        cmp_13_29_178_5:
        cmp r15b, 0
        jne if_13_26_178_5_end
        if_13_29_178_5_code:
            mov rdi, 1
            exit_13_38_178_5:
                    mov rax, 60
                syscall
            exit_13_38_178_5_end:
        if_13_26_178_5_end:
    assert_178_5_end:
    mov qword [rsp - 293], 1
    bar_181_5:
        if_56_8_181_5:
        cmp_56_8_181_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_181_5_end
        if_56_8_181_5_code:
            jmp bar_181_5_end
        if_56_5_181_5_end:
        mov qword [rsp - 293], 255
    bar_181_5_end:
    cmp_182_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_182_12:
    assert_182_5:
        if_13_29_182_5:
        cmp_13_29_182_5:
        cmp r15b, 0
        jne if_13_26_182_5_end
        if_13_29_182_5_code:
            mov rdi, 1
            exit_13_38_182_5:
                    mov rax, 60
                syscall
            exit_13_38_182_5_end:
        if_13_26_182_5_end:
    assert_182_5_end:
    mov qword [rsp - 301], 1
    baz_185_13:
        mov r15, qword [rsp - 301]
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_185_13_end:
    cmp_186_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_186_12:
    assert_186_5:
        if_13_29_186_5:
        cmp_13_29_186_5:
        cmp r15b, 0
        jne if_13_26_186_5_end
        if_13_29_186_5_code:
            mov rdi, 1
            exit_13_38_186_5:
                    mov rax, 60
                syscall
            exit_13_38_186_5_end:
        if_13_26_186_5_end:
    assert_186_5_end:
    baz_188_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_188_9_end:
    cmp_189_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_189_12:
    assert_189_5:
        if_13_29_189_5:
        cmp_13_29_189_5:
        cmp r15b, 0
        jne if_13_26_189_5_end
        if_13_29_189_5_code:
            mov rdi, 1
            exit_13_38_189_5:
                    mov rax, 60
                syscall
            exit_13_38_189_5_end:
        if_13_26_189_5_end:
    assert_189_5_end:
    baz_191_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 325], r15
    baz_191_23_end:
    mov qword [rsp - 317], 0
    cmp_192_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_13_29_192_5:
        cmp_13_29_192_5:
        cmp r15b, 0
        jne if_13_26_192_5_end
        if_13_29_192_5_code:
            mov rdi, 1
            exit_13_38_192_5:
                    mov rax, 60
                syscall
            exit_13_38_192_5_end:
        if_13_26_192_5_end:
    assert_192_5_end:
    point_init_194_22:
        mov qword [rsp - 341], -1
        mov qword [rsp - 333], -2
    point_init_194_22_end:
    cmp_195_12:
    cmp qword [rsp - 341], -1
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_13_29_195_5:
        cmp_13_29_195_5:
        cmp r15b, 0
        jne if_13_26_195_5_end
        if_13_29_195_5_code:
            mov rdi, 1
            exit_13_38_195_5:
                    mov rax, 60
                syscall
            exit_13_38_195_5_end:
        if_13_26_195_5_end:
    assert_195_5_end:
    cmp_196_12:
    cmp qword [rsp - 333], -2
    sete r15b
    bool_end_196_12:
    assert_196_5:
        if_13_29_196_5:
        cmp_13_29_196_5:
        cmp r15b, 0
        jne if_13_26_196_5_end
        if_13_29_196_5_code:
            mov rdi, 1
            exit_13_38_196_5:
                    mov rax, 60
                syscall
            exit_13_38_196_5_end:
        if_13_26_196_5_end:
    assert_196_5_end:
    mov qword [rsp - 349], 1
    mov qword [rsp - 357], 2
    mov r15, qword [rsp - 349]
    imul r15, 10
    mov qword [rsp - 377], r15
    mov r15, qword [rsp - 357]
    mov qword [rsp - 369], r15
    mov dword [rsp - 361], 16711680
    cmp_202_12:
    cmp qword [rsp - 377], 10
    sete r15b
    bool_end_202_12:
    assert_202_5:
        if_13_29_202_5:
        cmp_13_29_202_5:
        cmp r15b, 0
        jne if_13_26_202_5_end
        if_13_29_202_5_code:
            mov rdi, 1
            exit_13_38_202_5:
                    mov rax, 60
                syscall
            exit_13_38_202_5_end:
        if_13_26_202_5_end:
    assert_202_5_end:
    cmp_203_12:
    cmp qword [rsp - 369], 2
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_13_29_203_5:
        cmp_13_29_203_5:
        cmp r15b, 0
        jne if_13_26_203_5_end
        if_13_29_203_5_code:
            mov rdi, 1
            exit_13_38_203_5:
                    mov rax, 60
                syscall
            exit_13_38_203_5_end:
        if_13_26_203_5_end:
    assert_203_5_end:
    cmp_204_12:
    cmp dword [rsp - 361], 16711680
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_13_29_204_5:
        cmp_13_29_204_5:
        cmp r15b, 0
        jne if_13_26_204_5_end
        if_13_29_204_5_code:
            mov rdi, 1
            exit_13_38_204_5:
                    mov rax, 60
                syscall
            exit_13_38_204_5_end:
        if_13_26_204_5_end:
    assert_204_5_end:
    mov r15, qword [rsp - 349]
    mov qword [rsp - 393], r15
    neg qword [rsp - 393]
    mov r15, qword [rsp - 357]
    mov qword [rsp - 385], r15
    neg qword [rsp - 385]
    mov rax, qword [rsp - 393]
    mov qword [rsp - 377], rax
    mov rax, qword [rsp - 385]
    mov qword [rsp - 369], rax
    cmp_208_12:
    cmp qword [rsp - 377], -1
    sete r15b
    bool_end_208_12:
    assert_208_5:
        if_13_29_208_5:
        cmp_13_29_208_5:
        cmp r15b, 0
        jne if_13_26_208_5_end
        if_13_29_208_5_code:
            mov rdi, 1
            exit_13_38_208_5:
                    mov rax, 60
                syscall
            exit_13_38_208_5_end:
        if_13_26_208_5_end:
    assert_208_5_end:
    cmp_209_12:
    cmp qword [rsp - 369], -2
    sete r15b
    bool_end_209_12:
    assert_209_5:
        if_13_29_209_5:
        cmp_13_29_209_5:
        cmp r15b, 0
        jne if_13_26_209_5_end
        if_13_29_209_5_code:
            mov rdi, 1
            exit_13_38_209_5:
                    mov rax, 60
                syscall
            exit_13_38_209_5_end:
        if_13_26_209_5_end:
    assert_209_5_end:
    lea rsi, [rsp - 377]
    lea rdi, [rsp - 413]
    mov rcx, 20
    rep movsb
    cmp_212_12:
    cmp qword [rsp - 413], -1
    sete r15b
    bool_end_212_12:
    assert_212_5:
        if_13_29_212_5:
        cmp_13_29_212_5:
        cmp r15b, 0
        jne if_13_26_212_5_end
        if_13_29_212_5_code:
            mov rdi, 1
            exit_13_38_212_5:
                    mov rax, 60
                syscall
            exit_13_38_212_5_end:
        if_13_26_212_5_end:
    assert_212_5_end:
    cmp_213_12:
    cmp qword [rsp - 405], -2
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_13_29_213_5:
        cmp_13_29_213_5:
        cmp r15b, 0
        jne if_13_26_213_5_end
        if_13_29_213_5_code:
            mov rdi, 1
            exit_13_38_213_5:
                    mov rax, 60
                syscall
            exit_13_38_213_5_end:
        if_13_26_213_5_end:
    assert_213_5_end:
    cmp_214_12:
    cmp dword [rsp - 397], 16711680
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_13_29_214_5:
        cmp_13_29_214_5:
        cmp r15b, 0
        jne if_13_26_214_5_end
        if_13_29_214_5_code:
            mov rdi, 1
            exit_13_38_214_5:
                    mov rax, 60
                syscall
            exit_13_38_214_5_end:
        if_13_26_214_5_end:
    assert_214_5_end:
    xor al, al
    lea rdi, [rsp - 453]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 445], 73
    cmp_220_12:
    lea r14, [rsp - 453]
    mov r13, 0
    mov r12, 220
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
    bool_end_220_12:
    assert_220_5:
        if_13_29_220_5:
        cmp_13_29_220_5:
        cmp r15b, 0
        jne if_13_26_220_5_end
        if_13_29_220_5_code:
            mov rdi, 1
            exit_13_38_220_5:
                    mov rax, 60
                syscall
            exit_13_38_220_5_end:
        if_13_26_220_5_end:
    assert_220_5_end:
    lea r15, [rsp - 453]
    mov r14, 1
    mov r13, 222
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_222_13:
        mov qword [r15 + 8], 74
    object_init_222_13_end:
    cmp_223_12:
    lea r14, [rsp - 453]
    mov r13, 1
    mov r12, 223
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
    bool_end_223_12:
    assert_223_5:
        if_13_29_223_5:
        cmp_13_29_223_5:
        cmp r15b, 0
        jne if_13_26_223_5_end
        if_13_29_223_5_code:
            mov rdi, 1
            exit_13_38_223_5:
                    mov rax, 60
                syscall
            exit_13_38_223_5_end:
        if_13_26_223_5_end:
    assert_223_5_end:
    xor al, al
    lea rdi, [rsp - 965]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 226
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 226
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_227_12:
    lea r14, [rsp - 965]
    mov r13, 1
    mov r12, 227
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 227
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_227_12:
    assert_227_5:
        if_13_29_227_5:
        cmp_13_29_227_5:
        cmp r15b, 0
        jne if_13_26_227_5_end
        if_13_29_227_5_code:
            mov rdi, 1
            exit_13_38_227_5:
                    mov rax, 60
                syscall
            exit_13_38_227_5_end:
        if_13_26_227_5_end:
    assert_227_5_end:
    mov rcx, 8
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 230
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 230
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 965]
    mov r14, 0
    mov r13, 231
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 231
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_237_12:
    lea r14, [rsp - 965]
    mov r13, 0
    mov r12, 237
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 237
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_13_29_237_5:
        cmp_13_29_237_5:
        cmp r15b, 0
        jne if_13_26_237_5_end
        if_13_29_237_5_code:
            mov rdi, 1
            exit_13_38_237_5:
                    mov rax, 60
                syscall
            exit_13_38_237_5_end:
        if_13_26_237_5_end:
    assert_237_5_end:
    cmp_238_12:
        mov rcx, 8
        lea r13, [rsp - 965]
        mov r12, 0
        mov r11, 239
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 239
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 965]
        mov r12, 1
        mov r11, 240
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 240
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
    bool_end_238_12:
    assert_238_5:
        if_13_29_238_5:
        cmp_13_29_238_5:
        cmp r15b, 0
        jne if_13_26_238_5_end
        if_13_29_238_5_code:
            mov rdi, 1
            exit_13_38_238_5:
                    mov rax, 60
                syscall
            exit_13_38_238_5_end:
        if_13_26_238_5_end:
    assert_238_5_end:
    xor al, al
    lea rdi, [rsp - 1093]
    mov rcx, 128
    rep stosb
    print_245_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_24_4_245_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_24_4_245_5_end:
    print_245_5_end:
    loop_246_5:
        print_247_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_24_4_247_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_24_4_247_9_end:
        print_247_9_end:
        str_in_248_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1092]
                mov rdx, 127
            syscall
                mov byte [rsp - 1093], al
                sub byte [rsp - 1093], 1
        str_in_248_9_end:
        if_249_12:
        cmp_249_12:
        cmp byte [rsp - 1093], 0
        jne if_251_19
        if_249_12_code:
            jmp loop_246_5_end
        jmp if_249_9_end
        if_251_19:
        cmp_251_19:
        cmp byte [rsp - 1093], 4
        jg if_else_249_9
        if_251_19_code:
            print_252_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_24_4_252_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_252_13_end:
            print_252_13_end:
            jmp loop_246_5
        jmp if_249_9_end
        if_else_249_9:
            print_255_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_24_4_255_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_255_13_end:
            print_255_13_end:
            str_out_256_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1092]
                    movsx rdx, byte [rsp - 1093]
                syscall
            str_out_256_13_end:
            print_257_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_24_4_257_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_257_13_end:
            print_257_13_end:
            print_258_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_24_4_258_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_258_13_end:
            print_258_13_end:
        if_249_9_end:
    jmp loop_246_5
    loop_246_5_end:
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
db 3
times 127 db 0
db `\n`
db `.`
db `hello `
db `that is not a name.\n`
db `enter name:\n`
db `hello world from baz\n`
dat.len equ $ - dat
