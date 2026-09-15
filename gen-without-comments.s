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
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov qword [rsp - 245], 0
    cmp_133_12:
    cmp qword [rsp - 245], 0
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_41_26_133_5:
        cmp_41_26_133_5:
        cmp r15b, 0
        jne if_41_23_133_5_end
        if_41_26_133_5_code:
            mov rdi, 1
            exit_41_32_133_5:
                    mov rax, 60
                syscall
            exit_41_32_133_5_end:
        if_41_23_133_5_end:
    assert_133_5_end:
    mov qword [rsp - 245], -1
    cmp_137_12:
    cmp qword [rsp - 245], -1
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_41_26_137_5:
        cmp_41_26_137_5:
        cmp r15b, 0
        jne if_41_23_137_5_end
        if_41_26_137_5_code:
            mov rdi, 1
            exit_41_32_137_5:
                    mov rax, 60
                syscall
            exit_41_32_137_5_end:
        if_41_23_137_5_end:
    assert_137_5_end:
        cmp_141_16:
        bool_end_141_16:
        mov r15b, 1
        assert_141_9:
            if_41_26_141_9:
            cmp_41_26_141_9:
            cmp r15b, 0
            jne if_41_23_141_9_end
            if_41_26_141_9_code:
                mov rdi, 1
                exit_41_32_141_9:
                        mov rax, 60
                    syscall
                exit_41_32_141_9_end:
            if_41_23_141_9_end:
        assert_141_9_end:
    cmp_144_12:
    bool_end_144_12:
    mov r15b, 1
    assert_144_5:
        if_41_26_144_5:
        cmp_41_26_144_5:
        cmp r15b, 0
        jne if_41_23_144_5_end
        if_41_26_144_5_code:
            mov rdi, 1
            exit_41_32_144_5:
                    mov rax, 60
                syscall
            exit_41_32_144_5_end:
        if_41_23_144_5_end:
    assert_144_5_end:
    mov qword [rsp - 253], 1
    mov r15, qword [rsp - 253]
    mov r14, 149
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 237], 2
    mov r15, qword [rsp - 253]
    add r15, 1
    mov r14, 150
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 253]
    mov r13, 150
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 237]
    mov dword [rsp + r15 * 4 - 237], r13d
    cmp_151_12:
    mov r14, 1
    mov r13, 151
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_151_12:
    assert_151_5:
        if_41_26_151_5:
        cmp_41_26_151_5:
        cmp r15b, 0
        jne if_41_23_151_5_end
        if_41_26_151_5_code:
            mov rdi, 1
            exit_41_32_151_5:
                    mov rax, 60
                syscall
            exit_41_32_151_5_end:
        if_41_23_151_5_end:
    assert_151_5_end:
    cmp_152_12:
    mov r14, 2
    mov r13, 152
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_152_12:
    assert_152_5:
        if_41_26_152_5:
        cmp_41_26_152_5:
        cmp r15b, 0
        jne if_41_23_152_5_end
        if_41_26_152_5_code:
            mov rdi, 1
            exit_41_32_152_5:
                    mov rax, 60
                syscall
            exit_41_32_152_5_end:
        if_41_23_152_5_end:
    assert_152_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 237]
    mov r15, 154
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 237]
    shl rcx, 2
    rep movsb
    cmp_156_12:
    mov r14, 0
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_41_26_156_5:
        cmp_41_26_156_5:
        cmp r15b, 0
        jne if_41_23_156_5_end
        if_41_26_156_5_code:
            mov rdi, 1
            exit_41_32_156_5:
                    mov rax, 60
                syscall
            exit_41_32_156_5_end:
        if_41_23_156_5_end:
    assert_156_5_end:
    mov qword [rsp - 285], 0
    mov qword [rsp - 277], 0
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    mov rcx, 4
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 237]
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 285]
    shl rcx, 2
    rep movsb
    cmp_160_12:
        mov rcx, 4
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 237]
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 285]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_160_12:
    assert_160_5:
        if_41_26_160_5:
        cmp_41_26_160_5:
        cmp r15b, 0
        jne if_41_23_160_5_end
        if_41_26_160_5_code:
            mov rdi, 1
            exit_41_32_160_5:
                    mov rax, 60
                syscall
            exit_41_32_160_5_end:
        if_41_23_160_5_end:
    assert_160_5_end:
    mov r15, 2
    mov r14, 163
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 285], -1
    cmp_164_12:
        mov rcx, 4
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 237]
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 285]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_164_12:
    assert_164_5:
        if_41_26_164_5:
        cmp_41_26_164_5:
        cmp r15b, 0
        jne if_41_23_164_5_end
        if_41_26_164_5_code:
            mov rdi, 1
            exit_41_32_164_5:
                    mov rax, 60
                syscall
            exit_41_32_164_5_end:
        if_41_23_164_5_end:
    assert_164_5_end:
    mov qword [rsp - 253], 3
    mov r15, qword [rsp - 253]
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 253]
    sub r14, 1
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_167_16:
        mov r13d, dword [rsp + r14 * 4 - 237]
        mov dword [rsp + r15 * 4 - 237], r13d
        not dword [rsp + r15 * 4 - 237]
    inv_167_16_end:
    not dword [rsp + r15 * 4 - 237]
    cmp_168_12:
    mov r14, qword [rsp - 253]
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_168_12:
    assert_168_5:
        if_41_26_168_5:
        cmp_41_26_168_5:
        cmp r15b, 0
        jne if_41_23_168_5_end
        if_41_26_168_5_code:
            mov rdi, 1
            exit_41_32_168_5:
                    mov rax, 60
                syscall
            exit_41_32_168_5_end:
        if_41_23_168_5_end:
    assert_168_5_end:
    faz_170_5:
        mov r15, 1
        mov r14, 93
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 237], 254
    faz_170_5_end:
    cmp_171_12:
    mov r14, 1
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 254
    sete r15b
    bool_end_171_12:
    assert_171_5:
        if_41_26_171_5:
        cmp_41_26_171_5:
        cmp r15b, 0
        jne if_41_23_171_5_end
        if_41_26_171_5_code:
            mov rdi, 1
            exit_41_32_171_5:
                    mov rax, 60
                syscall
            exit_41_32_171_5_end:
        if_41_23_171_5_end:
    assert_171_5_end:
    mov qword [rsp - 301], 0
    mov qword [rsp - 293], 0
    foo_174_5:
        mov qword [rsp - 301], 2
        mov qword [rsp - 293], 11
    foo_174_5_end:
    cmp_175_12:
    cmp qword [rsp - 301], 2
    sete r15b
    bool_end_175_12:
    assert_175_5:
        if_41_26_175_5:
        cmp_41_26_175_5:
        cmp r15b, 0
        jne if_41_23_175_5_end
        if_41_26_175_5_code:
            mov rdi, 1
            exit_41_32_175_5:
                    mov rax, 60
                syscall
            exit_41_32_175_5_end:
        if_41_23_175_5_end:
    assert_175_5_end:
    cmp_176_12:
    cmp qword [rsp - 293], 11
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_41_26_176_5:
        cmp_41_26_176_5:
        cmp r15b, 0
        jne if_41_23_176_5_end
        if_41_26_176_5_code:
            mov rdi, 1
            exit_41_32_176_5:
                    mov rax, 60
                syscall
            exit_41_32_176_5_end:
        if_41_23_176_5_end:
    assert_176_5_end:
    mov rax, qword [rsp - 301]
    mov qword [rsp - 317], rax
    mov rax, qword [rsp - 293]
    mov qword [rsp - 309], rax
    cmp_179_12:
        lea rsi, [rsp - 301]
        lea rdi, [rsp - 317]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_179_12:
    assert_179_5:
        if_41_26_179_5:
        cmp_41_26_179_5:
        cmp r15b, 0
        jne if_41_23_179_5_end
        if_41_26_179_5_code:
            mov rdi, 1
            exit_41_32_179_5:
                    mov rax, 60
                syscall
            exit_41_32_179_5_end:
        if_41_23_179_5_end:
    assert_179_5_end:
    mov qword [rsp - 317], 3
    cmp_184_12:
        lea rsi, [rsp - 301]
        lea rdi, [rsp - 317]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_184_12:
    assert_184_5:
        if_41_26_184_5:
        cmp_41_26_184_5:
        cmp r15b, 0
        jne if_41_23_184_5_end
        if_41_26_184_5_code:
            mov rdi, 1
            exit_41_32_184_5:
                    mov rax, 60
                syscall
            exit_41_32_184_5_end:
        if_41_23_184_5_end:
    assert_184_5_end:
    mov qword [rsp - 325], 0
    bar_187_5:
        if_74_8_187_5:
        cmp_74_8_187_5:
        cmp qword [rsp - 325], 0
        jne if_74_5_187_5_end
        if_74_8_187_5_code:
            jmp bar_187_5_end
        if_74_5_187_5_end:
        mov qword [rsp - 325], 255
    bar_187_5_end:
    cmp_188_12:
    cmp qword [rsp - 325], 0
    sete r15b
    bool_end_188_12:
    assert_188_5:
        if_41_26_188_5:
        cmp_41_26_188_5:
        cmp r15b, 0
        jne if_41_23_188_5_end
        if_41_26_188_5_code:
            mov rdi, 1
            exit_41_32_188_5:
                    mov rax, 60
                syscall
            exit_41_32_188_5_end:
        if_41_23_188_5_end:
    assert_188_5_end:
    mov qword [rsp - 325], 1
    bar_191_5:
        if_74_8_191_5:
        cmp_74_8_191_5:
        cmp qword [rsp - 325], 0
        jne if_74_5_191_5_end
        if_74_8_191_5_code:
            jmp bar_191_5_end
        if_74_5_191_5_end:
        mov qword [rsp - 325], 255
    bar_191_5_end:
    cmp_192_12:
    cmp qword [rsp - 325], 255
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_41_26_192_5:
        cmp_41_26_192_5:
        cmp r15b, 0
        jne if_41_23_192_5_end
        if_41_26_192_5_code:
            mov rdi, 1
            exit_41_32_192_5:
                    mov rax, 60
                syscall
            exit_41_32_192_5_end:
        if_41_23_192_5_end:
    assert_192_5_end:
    mov qword [rsp - 333], 1
    baz_195_13:
        mov r15, qword [rsp - 333]
        imul r15, 2
        mov qword [rsp - 341], r15
    baz_195_13_end:
    cmp_196_12:
    cmp qword [rsp - 341], 2
    sete r15b
    bool_end_196_12:
    assert_196_5:
        if_41_26_196_5:
        cmp_41_26_196_5:
        cmp r15b, 0
        jne if_41_23_196_5_end
        if_41_26_196_5_code:
            mov rdi, 1
            exit_41_32_196_5:
                    mov rax, 60
                syscall
            exit_41_32_196_5_end:
        if_41_23_196_5_end:
    assert_196_5_end:
    baz_198_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 341], r15
    baz_198_9_end:
    cmp_199_12:
    cmp qword [rsp - 341], 2
    sete r15b
    bool_end_199_12:
    assert_199_5:
        if_41_26_199_5:
        cmp_41_26_199_5:
        cmp r15b, 0
        jne if_41_23_199_5_end
        if_41_26_199_5_code:
            mov rdi, 1
            exit_41_32_199_5:
                    mov rax, 60
                syscall
            exit_41_32_199_5_end:
        if_41_23_199_5_end:
    assert_199_5_end:
    baz_201_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 357], r15
    baz_201_23_end:
    mov qword [rsp - 349], 0
    cmp_202_12:
    cmp qword [rsp - 357], 4
    sete r15b
    bool_end_202_12:
    assert_202_5:
        if_41_26_202_5:
        cmp_41_26_202_5:
        cmp r15b, 0
        jne if_41_23_202_5_end
        if_41_26_202_5_code:
            mov rdi, 1
            exit_41_32_202_5:
                    mov rax, 60
                syscall
            exit_41_32_202_5_end:
        if_41_23_202_5_end:
    assert_202_5_end:
    point_init_204_22:
        mov qword [rsp - 373], -1
        mov qword [rsp - 365], -2
    point_init_204_22_end:
    cmp_205_12:
    cmp qword [rsp - 373], -1
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_41_26_205_5:
        cmp_41_26_205_5:
        cmp r15b, 0
        jne if_41_23_205_5_end
        if_41_26_205_5_code:
            mov rdi, 1
            exit_41_32_205_5:
                    mov rax, 60
                syscall
            exit_41_32_205_5_end:
        if_41_23_205_5_end:
    assert_205_5_end:
    cmp_206_12:
    cmp qword [rsp - 365], -2
    sete r15b
    bool_end_206_12:
    assert_206_5:
        if_41_26_206_5:
        cmp_41_26_206_5:
        cmp r15b, 0
        jne if_41_23_206_5_end
        if_41_26_206_5_code:
            mov rdi, 1
            exit_41_32_206_5:
                    mov rax, 60
                syscall
            exit_41_32_206_5_end:
        if_41_23_206_5_end:
    assert_206_5_end:
    mov qword [rsp - 381], 1
    mov qword [rsp - 389], 2
    mov r15, qword [rsp - 381]
    imul r15, 10
    mov qword [rsp - 409], r15
    mov r15, qword [rsp - 389]
    mov qword [rsp - 401], r15
    mov dword [rsp - 393], 16711680
    cmp_212_12:
    cmp qword [rsp - 409], 10
    sete r15b
    bool_end_212_12:
    assert_212_5:
        if_41_26_212_5:
        cmp_41_26_212_5:
        cmp r15b, 0
        jne if_41_23_212_5_end
        if_41_26_212_5_code:
            mov rdi, 1
            exit_41_32_212_5:
                    mov rax, 60
                syscall
            exit_41_32_212_5_end:
        if_41_23_212_5_end:
    assert_212_5_end:
    cmp_213_12:
    cmp qword [rsp - 401], 2
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_41_26_213_5:
        cmp_41_26_213_5:
        cmp r15b, 0
        jne if_41_23_213_5_end
        if_41_26_213_5_code:
            mov rdi, 1
            exit_41_32_213_5:
                    mov rax, 60
                syscall
            exit_41_32_213_5_end:
        if_41_23_213_5_end:
    assert_213_5_end:
    cmp_214_12:
    cmp dword [rsp - 393], 16711680
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_41_26_214_5:
        cmp_41_26_214_5:
        cmp r15b, 0
        jne if_41_23_214_5_end
        if_41_26_214_5_code:
            mov rdi, 1
            exit_41_32_214_5:
                    mov rax, 60
                syscall
            exit_41_32_214_5_end:
        if_41_23_214_5_end:
    assert_214_5_end:
    mov r15, qword [rsp - 381]
    mov qword [rsp - 425], r15
    neg qword [rsp - 425]
    mov r15, qword [rsp - 389]
    mov qword [rsp - 417], r15
    neg qword [rsp - 417]
    mov rax, qword [rsp - 425]
    mov qword [rsp - 409], rax
    mov rax, qword [rsp - 417]
    mov qword [rsp - 401], rax
    cmp_218_12:
    cmp qword [rsp - 409], -1
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_41_26_218_5:
        cmp_41_26_218_5:
        cmp r15b, 0
        jne if_41_23_218_5_end
        if_41_26_218_5_code:
            mov rdi, 1
            exit_41_32_218_5:
                    mov rax, 60
                syscall
            exit_41_32_218_5_end:
        if_41_23_218_5_end:
    assert_218_5_end:
    cmp_219_12:
    cmp qword [rsp - 401], -2
    sete r15b
    bool_end_219_12:
    assert_219_5:
        if_41_26_219_5:
        cmp_41_26_219_5:
        cmp r15b, 0
        jne if_41_23_219_5_end
        if_41_26_219_5_code:
            mov rdi, 1
            exit_41_32_219_5:
                    mov rax, 60
                syscall
            exit_41_32_219_5_end:
        if_41_23_219_5_end:
    assert_219_5_end:
    lea rsi, [rsp - 409]
    lea rdi, [rsp - 445]
    mov rcx, 20
    rep movsb
    cmp_222_12:
    cmp qword [rsp - 445], -1
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_41_26_222_5:
        cmp_41_26_222_5:
        cmp r15b, 0
        jne if_41_23_222_5_end
        if_41_26_222_5_code:
            mov rdi, 1
            exit_41_32_222_5:
                    mov rax, 60
                syscall
            exit_41_32_222_5_end:
        if_41_23_222_5_end:
    assert_222_5_end:
    cmp_223_12:
    cmp qword [rsp - 437], -2
    sete r15b
    bool_end_223_12:
    assert_223_5:
        if_41_26_223_5:
        cmp_41_26_223_5:
        cmp r15b, 0
        jne if_41_23_223_5_end
        if_41_26_223_5_code:
            mov rdi, 1
            exit_41_32_223_5:
                    mov rax, 60
                syscall
            exit_41_32_223_5_end:
        if_41_23_223_5_end:
    assert_223_5_end:
    cmp_224_12:
    cmp dword [rsp - 429], 16711680
    sete r15b
    bool_end_224_12:
    assert_224_5:
        if_41_26_224_5:
        cmp_41_26_224_5:
        cmp r15b, 0
        jne if_41_23_224_5_end
        if_41_26_224_5_code:
            mov rdi, 1
            exit_41_32_224_5:
                    mov rax, 60
                syscall
            exit_41_32_224_5_end:
        if_41_23_224_5_end:
    assert_224_5_end:
    xor al, al
    lea rdi, [rsp - 485]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 477], 73
    cmp_230_12:
    lea r14, [rsp - 485]
    mov r13, 0
    mov r12, 230
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
    bool_end_230_12:
    assert_230_5:
        if_41_26_230_5:
        cmp_41_26_230_5:
        cmp r15b, 0
        jne if_41_23_230_5_end
        if_41_26_230_5_code:
            mov rdi, 1
            exit_41_32_230_5:
                    mov rax, 60
                syscall
            exit_41_32_230_5_end:
        if_41_23_230_5_end:
    assert_230_5_end:
    lea r15, [rsp - 485]
    mov r14, 1
    mov r13, 232
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_232_13:
        mov qword [r15 + 8], 74
    object_init_232_13_end:
    cmp_233_12:
    lea r14, [rsp - 485]
    mov r13, 1
    mov r12, 233
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
    bool_end_233_12:
    assert_233_5:
        if_41_26_233_5:
        cmp_41_26_233_5:
        cmp r15b, 0
        jne if_41_23_233_5_end
        if_41_26_233_5_code:
            mov rdi, 1
            exit_41_32_233_5:
                    mov rax, 60
                syscall
            exit_41_32_233_5_end:
        if_41_23_233_5_end:
    assert_233_5_end:
    xor al, al
    lea rdi, [rsp - 997]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 997]
    mov r14, 1
    mov r13, 236
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 236
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_237_12:
    lea r14, [rsp - 997]
    mov r13, 1
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
        if_41_26_237_5:
        cmp_41_26_237_5:
        cmp r15b, 0
        jne if_41_23_237_5_end
        if_41_26_237_5_code:
            mov rdi, 1
            exit_41_32_237_5:
                    mov rax, 60
                syscall
            exit_41_32_237_5_end:
        if_41_23_237_5_end:
    assert_237_5_end:
    mov rcx, 8
    lea r15, [rsp - 997]
    mov r14, 1
    mov r13, 240
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 240
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 997]
    mov r14, 0
    mov r13, 241
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 241
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_247_12:
    lea r14, [rsp - 997]
    mov r13, 0
    mov r12, 247
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 247
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_247_12:
    assert_247_5:
        if_41_26_247_5:
        cmp_41_26_247_5:
        cmp r15b, 0
        jne if_41_23_247_5_end
        if_41_26_247_5_code:
            mov rdi, 1
            exit_41_32_247_5:
                    mov rax, 60
                syscall
            exit_41_32_247_5_end:
        if_41_23_247_5_end:
    assert_247_5_end:
    cmp_248_12:
        mov rcx, 8
        lea r13, [rsp - 997]
        mov r12, 0
        mov r11, 249
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 249
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 997]
        mov r12, 1
        mov r11, 250
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 250
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
    bool_end_248_12:
    assert_248_5:
        if_41_26_248_5:
        cmp_41_26_248_5:
        cmp r15b, 0
        jne if_41_23_248_5_end
        if_41_26_248_5_code:
            mov rdi, 1
            exit_41_32_248_5:
                    mov rax, 60
                syscall
            exit_41_32_248_5_end:
        if_41_23_248_5_end:
    assert_248_5_end:
    mov qword [rsp - 1013], -1
    mov qword [rsp - 1005], 2
    cmp_255_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_255_12:
    assert_255_5:
        if_41_26_255_5:
        cmp_41_26_255_5:
        cmp r15b, 0
        jne if_41_23_255_5_end
        if_41_26_255_5_code:
            mov rdi, 1
            exit_41_32_255_5:
                    mov rax, 60
                syscall
            exit_41_32_255_5_end:
        if_41_23_255_5_end:
    assert_255_5_end:
    cmp_256_12:
    mov r14, 0
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1013], -1
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_41_26_256_5:
        cmp_41_26_256_5:
        cmp r15b, 0
        jne if_41_23_256_5_end
        if_41_26_256_5_code:
            mov rdi, 1
            exit_41_32_256_5:
                    mov rax, 60
                syscall
            exit_41_32_256_5_end:
        if_41_23_256_5_end:
    assert_256_5_end:
    cmp_257_12:
    mov r14, 1
    mov r13, 257
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1013], 2
    sete r15b
    bool_end_257_12:
    assert_257_5:
        if_41_26_257_5:
        cmp_41_26_257_5:
        cmp r15b, 0
        jne if_41_23_257_5_end
        if_41_26_257_5_code:
            mov rdi, 1
            exit_41_32_257_5:
                    mov rax, 60
                syscall
            exit_41_32_257_5_end:
        if_41_23_257_5_end:
    assert_257_5_end:
    xor al, al
    lea rdi, [rsp - 1141]
    mov rcx, 128
    rep stosb
    print_260_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_52_4_260_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_52_4_260_5_end:
    print_260_5_end:
    loop_261_5:
        print_262_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_52_4_262_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_52_4_262_9_end:
        print_262_9_end:
        str_in_263_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1140]
                mov rdx, 127
            syscall
                mov byte [rsp - 1141], al
                sub byte [rsp - 1141], 1
        str_in_263_9_end:
        if_264_12:
        cmp_264_12:
        cmp byte [rsp - 1141], 0
        jne if_266_19
        if_264_12_code:
            jmp loop_261_5_end
        jmp if_264_9_end
        if_266_19:
        cmp_266_19:
        cmp byte [rsp - 1141], 4
        jg if_else_264_9
        if_266_19_code:
            print_267_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_52_4_267_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_267_13_end:
            print_267_13_end:
            jmp loop_261_5
        jmp if_264_9_end
        if_else_264_9:
            print_270_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_52_4_270_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_270_13_end:
            print_270_13_end:
            str_out_271_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1140]
                    movsx rdx, byte [rsp - 1141]
                syscall
            str_out_271_13_end:
            print_272_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_52_4_272_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_272_13_end:
            print_272_13_end:
            print_273_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_52_4_273_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_273_13_end:
            print_273_13_end:
        if_264_9_end:
    jmp loop_261_5
    loop_261_5_end:
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
dq 1
times 24 db 0
db `\n`
db `.`
db `hello `
db `that is not a name.\n`
db `enter name:\n`
db `hello world from baz\n`
dat.len equ $ - dat
