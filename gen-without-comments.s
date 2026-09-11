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
    cmp_121_12:
    cmp qword [rsp - 213], 0
    sete r15b
    bool_end_121_12:
    assert_121_5:
        if_13_29_121_5:
        cmp_13_29_121_5:
        cmp r15b, 0
        jne if_13_26_121_5_end
        if_13_29_121_5_code:
            mov rdi, 1
            exit_13_38_121_5:
                    mov rax, 60
                syscall
            exit_13_38_121_5_end:
        if_13_26_121_5_end:
    assert_121_5_end:
    mov qword [rsp - 213], -1
    cmp_125_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_125_12:
    assert_125_5:
        if_13_29_125_5:
        cmp_13_29_125_5:
        cmp r15b, 0
        jne if_13_26_125_5_end
        if_13_29_125_5_code:
            mov rdi, 1
            exit_13_38_125_5:
                    mov rax, 60
                syscall
            exit_13_38_125_5_end:
        if_13_26_125_5_end:
    assert_125_5_end:
    mov qword [rsp - 221], 1
    mov r15, qword [rsp - 221]
    mov r14, 130
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, 131
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    mov r13, 131
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_132_12:
    mov r14, 1
    mov r13, 132
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_132_12:
    assert_132_5:
        if_13_29_132_5:
        cmp_13_29_132_5:
        cmp r15b, 0
        jne if_13_26_132_5_end
        if_13_29_132_5_code:
            mov rdi, 1
            exit_13_38_132_5:
                    mov rax, 60
                syscall
            exit_13_38_132_5_end:
        if_13_26_132_5_end:
    assert_132_5_end:
    cmp_133_12:
    mov r14, 2
    mov r13, 133
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_13_29_133_5:
        cmp_13_29_133_5:
        cmp r15b, 0
        jne if_13_26_133_5_end
        if_13_29_133_5_code:
            mov rdi, 1
            exit_13_38_133_5:
                    mov rax, 60
                syscall
            exit_13_38_133_5_end:
        if_13_26_133_5_end:
    assert_133_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 135
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 205]
    mov r15, 135
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_137_12:
    mov r14, 0
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
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov rcx, 4
    mov r15, 140
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 205]
    mov r15, 140
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_141_12:
        mov rcx, 4
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 141
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
    mov r15, 2
    mov r14, 144
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 253], -1
    cmp_145_12:
        mov rcx, 4
        mov r13, 145
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 145
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
    bool_end_145_12:
    assert_145_5:
        if_13_29_145_5:
        cmp_13_29_145_5:
        cmp r15b, 0
        jne if_13_26_145_5_end
        if_13_29_145_5_code:
            mov rdi, 1
            exit_13_38_145_5:
                    mov rax, 60
                syscall
            exit_13_38_145_5_end:
        if_13_26_145_5_end:
    assert_145_5_end:
    mov qword [rsp - 221], 3
    mov r15, qword [rsp - 221]
    mov r14, 148
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    sub r14, 1
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_148_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_148_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_149_12:
    mov r14, qword [rsp - 221]
    mov r13, 149
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_149_12:
    assert_149_5:
        if_13_29_149_5:
        cmp_13_29_149_5:
        cmp r15b, 0
        jne if_13_26_149_5_end
        if_13_29_149_5_code:
            mov rdi, 1
            exit_13_38_149_5:
                    mov rax, 60
                syscall
            exit_13_38_149_5_end:
        if_13_26_149_5_end:
    assert_149_5_end:
    faz_151_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 205], 254
    faz_151_5_end:
    cmp_152_12:
    mov r14, 1
    mov r13, 152
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 254
    sete r15b
    bool_end_152_12:
    assert_152_5:
        if_13_29_152_5:
        cmp_13_29_152_5:
        cmp r15b, 0
        jne if_13_26_152_5_end
        if_13_29_152_5_code:
            mov rdi, 1
            exit_13_38_152_5:
                    mov rax, 60
                syscall
            exit_13_38_152_5_end:
        if_13_26_152_5_end:
    assert_152_5_end:
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    foo_155_5:
        mov qword [rsp - 269], 2
        mov qword [rsp - 261], 11
    foo_155_5_end:
    cmp_156_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_13_29_156_5:
        cmp_13_29_156_5:
        cmp r15b, 0
        jne if_13_26_156_5_end
        if_13_29_156_5_code:
            mov rdi, 1
            exit_13_38_156_5:
                    mov rax, 60
                syscall
            exit_13_38_156_5_end:
        if_13_26_156_5_end:
    assert_156_5_end:
    cmp_157_12:
    cmp qword [rsp - 261], 11
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
    lea rsi, [rsp - 269]
    lea rdi, [rsp - 285]
    mov rcx, 16
    rep movsb
    cmp_160_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_160_12:
    assert_160_5:
        if_13_29_160_5:
        cmp_13_29_160_5:
        cmp r15b, 0
        jne if_13_26_160_5_end
        if_13_29_160_5_code:
            mov rdi, 1
            exit_13_38_160_5:
                    mov rax, 60
                syscall
            exit_13_38_160_5_end:
        if_13_26_160_5_end:
    assert_160_5_end:
    mov qword [rsp - 285], 3
    cmp_165_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
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
    mov qword [rsp - 293], 0
    bar_168_5:
        if_56_8_168_5:
        cmp_56_8_168_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_168_5_end
        if_56_8_168_5_code:
            jmp bar_168_5_end
        if_56_5_168_5_end:
        mov qword [rsp - 293], 255
    bar_168_5_end:
    cmp_169_12:
    cmp qword [rsp - 293], 0
    sete r15b
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
    mov qword [rsp - 293], 1
    bar_172_5:
        if_56_8_172_5:
        cmp_56_8_172_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_172_5_end
        if_56_8_172_5_code:
            jmp bar_172_5_end
        if_56_5_172_5_end:
        mov qword [rsp - 293], 255
    bar_172_5_end:
    cmp_173_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_13_29_173_5:
        cmp_13_29_173_5:
        cmp r15b, 0
        jne if_13_26_173_5_end
        if_13_29_173_5_code:
            mov rdi, 1
            exit_13_38_173_5:
                    mov rax, 60
                syscall
            exit_13_38_173_5_end:
        if_13_26_173_5_end:
    assert_173_5_end:
    mov qword [rsp - 301], 1
    baz_176_13:
        mov r15, qword [rsp - 301]
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_176_13_end:
    cmp_177_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_13_29_177_5:
        cmp_13_29_177_5:
        cmp r15b, 0
        jne if_13_26_177_5_end
        if_13_29_177_5_code:
            mov rdi, 1
            exit_13_38_177_5:
                    mov rax, 60
                syscall
            exit_13_38_177_5_end:
        if_13_26_177_5_end:
    assert_177_5_end:
    baz_179_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_179_9_end:
    cmp_180_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_180_12:
    assert_180_5:
        if_13_29_180_5:
        cmp_13_29_180_5:
        cmp r15b, 0
        jne if_13_26_180_5_end
        if_13_29_180_5_code:
            mov rdi, 1
            exit_13_38_180_5:
                    mov rax, 60
                syscall
            exit_13_38_180_5_end:
        if_13_26_180_5_end:
    assert_180_5_end:
    baz_182_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 325], r15
    baz_182_23_end:
    mov qword [rsp - 317], 0
    cmp_183_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_13_29_183_5:
        cmp_13_29_183_5:
        cmp r15b, 0
        jne if_13_26_183_5_end
        if_13_29_183_5_code:
            mov rdi, 1
            exit_13_38_183_5:
                    mov rax, 60
                syscall
            exit_13_38_183_5_end:
        if_13_26_183_5_end:
    assert_183_5_end:
    mov qword [rsp - 333], 1
    mov qword [rsp - 341], 2
    mov r15, qword [rsp - 333]
    imul r15, 10
    mov qword [rsp - 361], r15
    mov r15, qword [rsp - 341]
    mov qword [rsp - 353], r15
    mov dword [rsp - 345], 16711680
    cmp_189_12:
    cmp qword [rsp - 361], 10
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
    cmp_190_12:
    cmp qword [rsp - 353], 2
    sete r15b
    bool_end_190_12:
    assert_190_5:
        if_13_29_190_5:
        cmp_13_29_190_5:
        cmp r15b, 0
        jne if_13_26_190_5_end
        if_13_29_190_5_code:
            mov rdi, 1
            exit_13_38_190_5:
                    mov rax, 60
                syscall
            exit_13_38_190_5_end:
        if_13_26_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    cmp dword [rsp - 345], 16711680
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
    mov r15, qword [rsp - 333]
    mov qword [rsp - 377], r15
    neg qword [rsp - 377]
    mov r15, qword [rsp - 341]
    mov qword [rsp - 369], r15
    neg qword [rsp - 369]
    lea rsi, [rsp - 377]
    lea rdi, [rsp - 361]
    mov rcx, 16
    rep movsb
    cmp_195_12:
    cmp qword [rsp - 361], -1
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
    cmp qword [rsp - 353], -2
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
    lea rsi, [rsp - 361]
    lea rdi, [rsp - 397]
    mov rcx, 20
    rep movsb
    cmp_199_12:
    cmp qword [rsp - 397], -1
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
    cmp qword [rsp - 389], -2
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
    cmp_201_12:
    cmp dword [rsp - 381], 16711680
    sete r15b
    bool_end_201_12:
    assert_201_5:
        if_13_29_201_5:
        cmp_13_29_201_5:
        cmp r15b, 0
        jne if_13_26_201_5_end
        if_13_29_201_5_code:
            mov rdi, 1
            exit_13_38_201_5:
                    mov rax, 60
                syscall
            exit_13_38_201_5_end:
        if_13_26_201_5_end:
    assert_201_5_end:
    mov qword [rsp - 417], 0
    mov qword [rsp - 409], 0
    mov dword [rsp - 401], 0
    mov qword [rsp - 409], 73
    cmp_207_12:
    lea r14, [rsp - 417]
    mov r13, 0
    mov r12, 207
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
    bool_end_207_12:
    assert_207_5:
        if_13_29_207_5:
        cmp_13_29_207_5:
        cmp r15b, 0
        jne if_13_26_207_5_end
        if_13_29_207_5_code:
            mov rdi, 1
            exit_13_38_207_5:
                    mov rax, 60
                syscall
            exit_13_38_207_5_end:
        if_13_26_207_5_end:
    assert_207_5_end:
    xor al, al
    lea rdi, [rsp - 929]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 929]
    mov r14, 1
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_211_12:
    lea r14, [rsp - 929]
    mov r13, 1
    mov r12, 211
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 211
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_13_29_211_5:
        cmp_13_29_211_5:
        cmp r15b, 0
        jne if_13_26_211_5_end
        if_13_29_211_5_code:
            mov rdi, 1
            exit_13_38_211_5:
                    mov rax, 60
                syscall
            exit_13_38_211_5_end:
        if_13_26_211_5_end:
    assert_211_5_end:
    mov rcx, 8
    lea r15, [rsp - 929]
    mov r14, 1
    mov r13, 214
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 214
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 929]
    mov r14, 0
    mov r13, 215
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 215
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_221_12:
    lea r14, [rsp - 929]
    mov r13, 0
    mov r12, 221
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 221
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_13_29_221_5:
        cmp_13_29_221_5:
        cmp r15b, 0
        jne if_13_26_221_5_end
        if_13_29_221_5_code:
            mov rdi, 1
            exit_13_38_221_5:
                    mov rax, 60
                syscall
            exit_13_38_221_5_end:
        if_13_26_221_5_end:
    assert_221_5_end:
    cmp_222_12:
        mov rcx, 8
        lea r13, [rsp - 929]
        mov r12, 0
        mov r11, 223
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 223
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 929]
        mov r12, 1
        mov r11, 224
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 224
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
    bool_end_222_12:
    assert_222_5:
        if_13_29_222_5:
        cmp_13_29_222_5:
        cmp r15b, 0
        jne if_13_26_222_5_end
        if_13_29_222_5_code:
            mov rdi, 1
            exit_13_38_222_5:
                    mov rax, 60
                syscall
            exit_13_38_222_5_end:
        if_13_26_222_5_end:
    assert_222_5_end:
    xor al, al
    lea rdi, [rsp - 1057]
    mov rcx, 128
    rep stosb
    print_229_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_24_4_229_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_24_4_229_5_end:
    print_229_5_end:
    loop_230_5:
        print_231_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_24_4_231_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_24_4_231_9_end:
        print_231_9_end:
        str_in_232_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1056]
                mov rdx, 127
            syscall
                mov byte [rsp - 1057], al
                sub byte [rsp - 1057], 1
        str_in_232_9_end:
        if_233_12:
        cmp_233_12:
        cmp byte [rsp - 1057], 0
        jne if_235_19
        if_233_12_code:
            jmp loop_230_5_end
        jmp if_233_9_end
        if_235_19:
        cmp_235_19:
        cmp byte [rsp - 1057], 4
        jg if_else_233_9
        if_235_19_code:
            print_236_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_24_4_236_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_236_13_end:
            print_236_13_end:
            jmp loop_230_5
        jmp if_233_9_end
        if_else_233_9:
            print_239_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_24_4_239_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_239_13_end:
            print_239_13_end:
            str_out_240_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1056]
                    movsx rdx, byte [rsp - 1057]
                syscall
            str_out_240_13_end:
            print_241_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_24_4_241_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_241_13_end:
            print_241_13_end:
            print_242_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_24_4_242_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_242_13_end:
            print_242_13_end:
        if_233_9_end:
    jmp loop_230_5
    loop_230_5_end:
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
