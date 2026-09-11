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
    cmp_126_12:
    cmp qword [rsp - 213], 0
    sete r15b
    bool_end_126_12:
    assert_126_5:
        if_13_29_126_5:
        cmp_13_29_126_5:
        cmp r15b, 0
        jne if_13_26_126_5_end
        if_13_29_126_5_code:
            mov rdi, 1
            exit_13_38_126_5:
                    mov rax, 60
                syscall
            exit_13_38_126_5_end:
        if_13_26_126_5_end:
    assert_126_5_end:
    mov qword [rsp - 213], -1
    cmp_130_12:
    cmp qword [rsp - 213], -1
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
    mov qword [rsp - 221], 1
    mov r15, qword [rsp - 221]
    mov r14, 135
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, 136
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    mov r13, 136
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_137_12:
    mov r14, 1
    mov r13, 137
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_13_29_137_5:
        cmp_13_29_137_5:
        cmp r15b, 0
        jne if_13_26_137_5_end
        if_13_29_137_5_code:
            mov rdi, 1
            exit_13_38_137_5:
                    mov rax, 60
                syscall
            exit_13_38_137_5_end:
        if_13_26_137_5_end:
    assert_137_5_end:
    cmp_138_12:
    mov r14, 2
    mov r13, 138
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_138_12:
    assert_138_5:
        if_13_29_138_5:
        cmp_13_29_138_5:
        cmp r15b, 0
        jne if_13_26_138_5_end
        if_13_29_138_5_code:
            mov rdi, 1
            exit_13_38_138_5:
                    mov rax, 60
                syscall
            exit_13_38_138_5_end:
        if_13_26_138_5_end:
    assert_138_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 140
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 205]
    mov r15, 140
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_142_12:
    mov r14, 0
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
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov rcx, 4
    mov r15, 145
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 205]
    mov r15, 145
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_146_12:
        mov rcx, 4
        mov r13, 146
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 146
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
    mov r15, 2
    mov r14, 149
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 253], -1
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
    sete r15b
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
    mov qword [rsp - 221], 3
    mov r15, qword [rsp - 221]
    mov r14, 153
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    sub r14, 1
    mov r13, 153
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_153_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_153_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_154_12:
    mov r14, qword [rsp - 221]
    mov r13, 154
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
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
    faz_156_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 205], 254
    faz_156_5_end:
    cmp_157_12:
    mov r14, 1
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 254
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_13_29_157_5:
        cmp_13_29_157_5:
        cmp r15b, 0
        jne if_13_26_157_5_end
        if_13_29_157_5_code:
            mov rdi, 1
            exit_13_38_157_5:
                    mov rax, 60
                syscall
            exit_13_38_157_5_end:
        if_13_26_157_5_end:
    assert_157_5_end:
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    foo_160_5:
        mov qword [rsp - 269], 2
        mov qword [rsp - 261], 11
    foo_160_5_end:
    cmp_161_12:
    cmp qword [rsp - 269], 2
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
    cmp_162_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_162_12:
    assert_162_5:
        if_13_29_162_5:
        cmp_13_29_162_5:
        cmp r15b, 0
        jne if_13_26_162_5_end
        if_13_29_162_5_code:
            mov rdi, 1
            exit_13_38_162_5:
                    mov rax, 60
                syscall
            exit_13_38_162_5_end:
        if_13_26_162_5_end:
    assert_162_5_end:
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
    cmp_165_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov qword [rsp - 285], 3
    cmp_170_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_13_29_170_5:
        cmp_13_29_170_5:
        cmp r15b, 0
        jne if_13_26_170_5_end
        if_13_29_170_5_code:
            mov rdi, 1
            exit_13_38_170_5:
                    mov rax, 60
                syscall
            exit_13_38_170_5_end:
        if_13_26_170_5_end:
    assert_170_5_end:
    mov qword [rsp - 293], 0
    bar_173_5:
        if_56_8_173_5:
        cmp_56_8_173_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_173_5_end
        if_56_8_173_5_code:
            jmp bar_173_5_end
        if_56_5_173_5_end:
        mov qword [rsp - 293], 255
    bar_173_5_end:
    cmp_174_12:
    cmp qword [rsp - 293], 0
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
    mov qword [rsp - 293], 1
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
    cmp qword [rsp - 293], 255
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
    mov qword [rsp - 301], 1
    baz_181_13:
        mov r15, qword [rsp - 301]
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_181_13_end:
    cmp_182_12:
    cmp qword [rsp - 309], 2
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
    baz_184_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_184_9_end:
    cmp_185_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_185_12:
    assert_185_5:
        if_13_29_185_5:
        cmp_13_29_185_5:
        cmp r15b, 0
        jne if_13_26_185_5_end
        if_13_29_185_5_code:
            mov rdi, 1
            exit_13_38_185_5:
                    mov rax, 60
                syscall
            exit_13_38_185_5_end:
        if_13_26_185_5_end:
    assert_185_5_end:
    baz_187_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 325], r15
    baz_187_23_end:
    mov qword [rsp - 317], 0
    cmp_188_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_188_12:
    assert_188_5:
        if_13_29_188_5:
        cmp_13_29_188_5:
        cmp r15b, 0
        jne if_13_26_188_5_end
        if_13_29_188_5_code:
            mov rdi, 1
            exit_13_38_188_5:
                    mov rax, 60
                syscall
            exit_13_38_188_5_end:
        if_13_26_188_5_end:
    assert_188_5_end:
    point_init_190_22:
        mov qword [rsp - 341], -1
        mov qword [rsp - 333], -2
    point_init_190_22_end:
    cmp_191_12:
    cmp qword [rsp - 341], -1
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_13_29_191_5:
        cmp_13_29_191_5:
        cmp r15b, 0
        jne if_13_26_191_5_end
        if_13_29_191_5_code:
            mov rdi, 1
            exit_13_38_191_5:
                    mov rax, 60
                syscall
            exit_13_38_191_5_end:
        if_13_26_191_5_end:
    assert_191_5_end:
    cmp_192_12:
    cmp qword [rsp - 333], -2
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
    mov qword [rsp - 349], 1
    mov qword [rsp - 357], 2
    mov r15, qword [rsp - 349]
    imul r15, 10
    mov qword [rsp - 377], r15
    mov r15, qword [rsp - 357]
    mov qword [rsp - 369], r15
    mov dword [rsp - 361], 16711680
    cmp_198_12:
    cmp qword [rsp - 377], 10
    sete r15b
    bool_end_198_12:
    assert_198_5:
        if_13_29_198_5:
        cmp_13_29_198_5:
        cmp r15b, 0
        jne if_13_26_198_5_end
        if_13_29_198_5_code:
            mov rdi, 1
            exit_13_38_198_5:
                    mov rax, 60
                syscall
            exit_13_38_198_5_end:
        if_13_26_198_5_end:
    assert_198_5_end:
    cmp_199_12:
    cmp qword [rsp - 369], 2
    sete r15b
    bool_end_199_12:
    assert_199_5:
        if_13_29_199_5:
        cmp_13_29_199_5:
        cmp r15b, 0
        jne if_13_26_199_5_end
        if_13_29_199_5_code:
            mov rdi, 1
            exit_13_38_199_5:
                    mov rax, 60
                syscall
            exit_13_38_199_5_end:
        if_13_26_199_5_end:
    assert_199_5_end:
    cmp_200_12:
    cmp dword [rsp - 361], 16711680
    sete r15b
    bool_end_200_12:
    assert_200_5:
        if_13_29_200_5:
        cmp_13_29_200_5:
        cmp r15b, 0
        jne if_13_26_200_5_end
        if_13_29_200_5_code:
            mov rdi, 1
            exit_13_38_200_5:
                    mov rax, 60
                syscall
            exit_13_38_200_5_end:
        if_13_26_200_5_end:
    assert_200_5_end:
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
    cmp_204_12:
    cmp qword [rsp - 377], -1
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
    cmp_205_12:
    cmp qword [rsp - 369], -2
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_13_29_205_5:
        cmp_13_29_205_5:
        cmp r15b, 0
        jne if_13_26_205_5_end
        if_13_29_205_5_code:
            mov rdi, 1
            exit_13_38_205_5:
                    mov rax, 60
                syscall
            exit_13_38_205_5_end:
        if_13_26_205_5_end:
    assert_205_5_end:
    lea rsi, [rsp - 377]
    lea rdi, [rsp - 413]
    mov rcx, 20
    rep movsb
    cmp_208_12:
    cmp qword [rsp - 413], -1
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
    cmp qword [rsp - 405], -2
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
    cmp_210_12:
    cmp dword [rsp - 397], 16711680
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_13_29_210_5:
        cmp_13_29_210_5:
        cmp r15b, 0
        jne if_13_26_210_5_end
        if_13_29_210_5_code:
            mov rdi, 1
            exit_13_38_210_5:
                    mov rax, 60
                syscall
            exit_13_38_210_5_end:
        if_13_26_210_5_end:
    assert_210_5_end:
    mov qword [rsp - 433], 0
    mov qword [rsp - 425], 0
    mov dword [rsp - 417], 0
    mov qword [rsp - 425], 73
    cmp_216_12:
    lea r14, [rsp - 433]
    mov r13, 0
    mov r12, 216
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_13_29_216_5:
        cmp_13_29_216_5:
        cmp r15b, 0
        jne if_13_26_216_5_end
        if_13_29_216_5_code:
            mov rdi, 1
            exit_13_38_216_5:
                    mov rax, 60
                syscall
            exit_13_38_216_5_end:
        if_13_26_216_5_end:
    assert_216_5_end:
    xor al, al
    lea rdi, [rsp - 945]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 945]
    mov r14, 1
    mov r13, 219
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 219
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_220_12:
    lea r14, [rsp - 945]
    mov r13, 1
    mov r12, 220
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 220
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    lea r15, [rsp - 945]
    mov r14, 1
    mov r13, 223
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 223
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 945]
    mov r14, 0
    mov r13, 224
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 224
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_230_12:
    lea r14, [rsp - 945]
    mov r13, 0
    mov r12, 230
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 230
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_13_29_230_5:
        cmp_13_29_230_5:
        cmp r15b, 0
        jne if_13_26_230_5_end
        if_13_29_230_5_code:
            mov rdi, 1
            exit_13_38_230_5:
                    mov rax, 60
                syscall
            exit_13_38_230_5_end:
        if_13_26_230_5_end:
    assert_230_5_end:
    cmp_231_12:
        mov rcx, 8
        lea r13, [rsp - 945]
        mov r12, 0
        mov r11, 232
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 232
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 945]
        mov r12, 1
        mov r11, 233
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 233
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
    bool_end_231_12:
    assert_231_5:
        if_13_29_231_5:
        cmp_13_29_231_5:
        cmp r15b, 0
        jne if_13_26_231_5_end
        if_13_29_231_5_code:
            mov rdi, 1
            exit_13_38_231_5:
                    mov rax, 60
                syscall
            exit_13_38_231_5_end:
        if_13_26_231_5_end:
    assert_231_5_end:
    xor al, al
    lea rdi, [rsp - 1073]
    mov rcx, 128
    rep stosb
    print_238_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_24_4_238_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_24_4_238_5_end:
    print_238_5_end:
    loop_239_5:
        print_240_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_24_4_240_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_24_4_240_9_end:
        print_240_9_end:
        str_in_241_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1072]
                mov rdx, 127
            syscall
                mov byte [rsp - 1073], al
                sub byte [rsp - 1073], 1
        str_in_241_9_end:
        if_242_12:
        cmp_242_12:
        cmp byte [rsp - 1073], 0
        jne if_244_19
        if_242_12_code:
            jmp loop_239_5_end
        jmp if_242_9_end
        if_244_19:
        cmp_244_19:
        cmp byte [rsp - 1073], 4
        jg if_else_242_9
        if_244_19_code:
            print_245_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_24_4_245_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_245_13_end:
            print_245_13_end:
            jmp loop_239_5
        jmp if_242_9_end
        if_else_242_9:
            print_248_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_24_4_248_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_248_13_end:
            print_248_13_end:
            str_out_249_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1072]
                    movsx rdx, byte [rsp - 1073]
                syscall
            str_out_249_13_end:
            print_250_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_24_4_250_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_250_13_end:
            print_250_13_end:
            print_251_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_24_4_251_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_251_13_end:
            print_251_13_end:
        if_242_9_end:
    jmp loop_239_5
    loop_239_5_end:
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
