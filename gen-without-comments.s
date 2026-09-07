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
    mov qword [rsp - 69], -1
    cmp_113_12:
    cmp qword [rsp - 69], -1
    sete r15b
    bool_end_113_12:
    assert_113_5:
        if_13_29_113_5:
        cmp_13_29_113_5:
        cmp r15b, 0
        jne if_13_26_113_5_end
        if_13_29_113_5_code:
            mov rdi, 1
            exit_13_38_113_5:
                    mov rax, 60
                syscall
            exit_13_38_113_5_end:
        if_13_26_113_5_end:
    assert_113_5_end:
    mov qword [rsp - 85], 0
    mov qword [rsp - 77], 0
    mov qword [rsp - 93], 1
    mov r15, qword [rsp - 93]
    mov r14, 120
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 85], 2
    mov r15, qword [rsp - 93]
    add r15, 1
    mov r14, 121
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    mov r13, 121
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 85]
    mov dword [rsp + r15 * 4 - 85], r13d
    cmp_122_12:
    mov r14, 1
    mov r13, 122
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 85], 2
    sete r15b
    bool_end_122_12:
    assert_122_5:
        if_13_29_122_5:
        cmp_13_29_122_5:
        cmp r15b, 0
        jne if_13_26_122_5_end
        if_13_29_122_5_code:
            mov rdi, 1
            exit_13_38_122_5:
                    mov rax, 60
                syscall
            exit_13_38_122_5_end:
        if_13_26_122_5_end:
    assert_122_5_end:
    cmp_123_12:
    mov r14, 2
    mov r13, 123
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 85], 2
    sete r15b
    bool_end_123_12:
    assert_123_5:
        if_13_29_123_5:
        cmp_13_29_123_5:
        cmp r15b, 0
        jne if_13_26_123_5_end
        if_13_29_123_5_code:
            mov rdi, 1
            exit_13_38_123_5:
                    mov rax, 60
                syscall
            exit_13_38_123_5_end:
        if_13_26_123_5_end:
    assert_123_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 85]
    mov r15, 125
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 85]
    shl rcx, 2
    rep movsb
    cmp_127_12:
    mov r14, 0
    mov r13, 127
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 85], 2
    sete r15b
    bool_end_127_12:
    assert_127_5:
        if_13_29_127_5:
        cmp_13_29_127_5:
        cmp r15b, 0
        jne if_13_26_127_5_end
        if_13_29_127_5_code:
            mov rdi, 1
            exit_13_38_127_5:
                    mov rax, 60
                syscall
            exit_13_38_127_5_end:
        if_13_26_127_5_end:
    assert_127_5_end:
    mov qword [rsp - 125], 0
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
    mov rcx, 4
    mov r15, 130
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 85]
    mov r15, 130
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 125]
    shl rcx, 2
    rep movsb
    cmp_131_12:
        mov rcx, 4
        mov r13, 131
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 85]
        mov r13, 131
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_131_12:
    assert_131_5:
        if_13_29_131_5:
        cmp_13_29_131_5:
        cmp r15b, 0
        jne if_13_26_131_5_end
        if_13_29_131_5_code:
            mov rdi, 1
            exit_13_38_131_5:
                    mov rax, 60
                syscall
            exit_13_38_131_5_end:
        if_13_26_131_5_end:
    assert_131_5_end:
    mov r15, 2
    mov r14, 134
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 125], -1
    cmp_135_12:
        mov rcx, 4
        mov r13, 135
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 85]
        mov r13, 135
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_135_12:
    assert_135_5:
        if_13_29_135_5:
        cmp_13_29_135_5:
        cmp r15b, 0
        jne if_13_26_135_5_end
        if_13_29_135_5_code:
            mov rdi, 1
            exit_13_38_135_5:
                    mov rax, 60
                syscall
            exit_13_38_135_5_end:
        if_13_26_135_5_end:
    assert_135_5_end:
    mov qword [rsp - 93], 3
    mov r15, qword [rsp - 93]
    mov r14, 138
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    sub r14, 1
    mov r13, 138
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_138_16:
        mov r13d, dword [rsp + r14 * 4 - 85]
        mov dword [rsp + r15 * 4 - 85], r13d
        not dword [rsp + r15 * 4 - 85]
    inv_138_16_end:
    not dword [rsp + r15 * 4 - 85]
    cmp_139_12:
    mov r14, qword [rsp - 93]
    mov r13, 139
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 85], 2
    sete r15b
    bool_end_139_12:
    assert_139_5:
        if_13_29_139_5:
        cmp_13_29_139_5:
        cmp r15b, 0
        jne if_13_26_139_5_end
        if_13_29_139_5_code:
            mov rdi, 1
            exit_13_38_139_5:
                    mov rax, 60
                syscall
            exit_13_38_139_5_end:
        if_13_26_139_5_end:
    assert_139_5_end:
    faz_141_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 85], 254
    faz_141_5_end:
    cmp_142_12:
    mov r14, 1
    mov r13, 142
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 85], 254
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
    mov qword [rsp - 141], 0
    mov qword [rsp - 133], 0
    foo_145_5:
        mov qword [rsp - 141], 2
        mov qword [rsp - 133], 11
    foo_145_5_end:
    cmp_146_12:
    cmp qword [rsp - 141], 2
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
    cmp_147_12:
    cmp qword [rsp - 133], 11
    sete r15b
    bool_end_147_12:
    assert_147_5:
        if_13_29_147_5:
        cmp_13_29_147_5:
        cmp r15b, 0
        jne if_13_26_147_5_end
        if_13_29_147_5_code:
            mov rdi, 1
            exit_13_38_147_5:
                    mov rax, 60
                syscall
            exit_13_38_147_5_end:
        if_13_26_147_5_end:
    assert_147_5_end:
    lea rsi, [rsp - 141]
    lea rdi, [rsp - 157]
    mov rcx, 2
    rep movsq
    cmp_150_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
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
    mov qword [rsp - 157], 3
    cmp_155_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_155_12:
    assert_155_5:
        if_13_29_155_5:
        cmp_13_29_155_5:
        cmp r15b, 0
        jne if_13_26_155_5_end
        if_13_29_155_5_code:
            mov rdi, 1
            exit_13_38_155_5:
                    mov rax, 60
                syscall
            exit_13_38_155_5_end:
        if_13_26_155_5_end:
    assert_155_5_end:
    mov qword [rsp - 165], 0
    bar_158_5:
        if_56_8_158_5:
        cmp_56_8_158_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_158_5_end
        if_56_8_158_5_code:
            jmp bar_158_5_end
        if_56_5_158_5_end:
        mov qword [rsp - 165], 255
    bar_158_5_end:
    cmp_159_12:
    cmp qword [rsp - 165], 0
    sete r15b
    bool_end_159_12:
    assert_159_5:
        if_13_29_159_5:
        cmp_13_29_159_5:
        cmp r15b, 0
        jne if_13_26_159_5_end
        if_13_29_159_5_code:
            mov rdi, 1
            exit_13_38_159_5:
                    mov rax, 60
                syscall
            exit_13_38_159_5_end:
        if_13_26_159_5_end:
    assert_159_5_end:
    mov qword [rsp - 165], 1
    bar_162_5:
        if_56_8_162_5:
        cmp_56_8_162_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_162_5_end
        if_56_8_162_5_code:
            jmp bar_162_5_end
        if_56_5_162_5_end:
        mov qword [rsp - 165], 255
    bar_162_5_end:
    cmp_163_12:
    cmp qword [rsp - 165], 255
    sete r15b
    bool_end_163_12:
    assert_163_5:
        if_13_29_163_5:
        cmp_13_29_163_5:
        cmp r15b, 0
        jne if_13_26_163_5_end
        if_13_29_163_5_code:
            mov rdi, 1
            exit_13_38_163_5:
                    mov rax, 60
                syscall
            exit_13_38_163_5_end:
        if_13_26_163_5_end:
    assert_163_5_end:
    mov qword [rsp - 173], 1
    baz_166_13:
        mov r15, qword [rsp - 173]
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_166_13_end:
    cmp_167_12:
    cmp qword [rsp - 181], 2
    sete r15b
    bool_end_167_12:
    assert_167_5:
        if_13_29_167_5:
        cmp_13_29_167_5:
        cmp r15b, 0
        jne if_13_26_167_5_end
        if_13_29_167_5_code:
            mov rdi, 1
            exit_13_38_167_5:
                    mov rax, 60
                syscall
            exit_13_38_167_5_end:
        if_13_26_167_5_end:
    assert_167_5_end:
    baz_169_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_169_9_end:
    cmp_170_12:
    cmp qword [rsp - 181], 2
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
    baz_172_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 197], r15
    baz_172_23_end:
    mov qword [rsp - 189], 0
    cmp_173_12:
    cmp qword [rsp - 197], 4
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
    mov qword [rsp - 205], 1
    mov qword [rsp - 213], 2
    mov r15, qword [rsp - 205]
    imul r15, 10
    mov qword [rsp - 233], r15
    mov r15, qword [rsp - 213]
    mov qword [rsp - 225], r15
    mov dword [rsp - 217], 16711680
    cmp_179_12:
    cmp qword [rsp - 233], 10
    sete r15b
    bool_end_179_12:
    assert_179_5:
        if_13_29_179_5:
        cmp_13_29_179_5:
        cmp r15b, 0
        jne if_13_26_179_5_end
        if_13_29_179_5_code:
            mov rdi, 1
            exit_13_38_179_5:
                    mov rax, 60
                syscall
            exit_13_38_179_5_end:
        if_13_26_179_5_end:
    assert_179_5_end:
    cmp_180_12:
    cmp qword [rsp - 225], 2
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
    cmp_181_12:
    cmp dword [rsp - 217], 16711680
    sete r15b
    bool_end_181_12:
    assert_181_5:
        if_13_29_181_5:
        cmp_13_29_181_5:
        cmp r15b, 0
        jne if_13_26_181_5_end
        if_13_29_181_5_code:
            mov rdi, 1
            exit_13_38_181_5:
                    mov rax, 60
                syscall
            exit_13_38_181_5_end:
        if_13_26_181_5_end:
    assert_181_5_end:
    mov r15, qword [rsp - 205]
    mov qword [rsp - 249], r15
    neg qword [rsp - 249]
    mov r15, qword [rsp - 213]
    mov qword [rsp - 241], r15
    neg qword [rsp - 241]
    lea rsi, [rsp - 249]
    lea rdi, [rsp - 233]
    mov rcx, 2
    rep movsq
    cmp_185_12:
    cmp qword [rsp - 233], -1
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
    cmp_186_12:
    cmp qword [rsp - 225], -2
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
    lea rsi, [rsp - 233]
    lea rdi, [rsp - 269]
    mov rcx, 20
    rep movsb
    cmp_189_12:
    cmp qword [rsp - 269], -1
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
    cmp qword [rsp - 261], -2
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
    cmp dword [rsp - 253], 16711680
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
    mov qword [rsp - 289], 0
    mov qword [rsp - 281], 0
    mov dword [rsp - 273], 0
    mov qword [rsp - 281], 73
    cmp_196_12:
    lea r14, [rsp - 289]
    mov r13, 0
    mov r12, 196
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
    mov rcx, 512
    lea rdi, [rsp - 801]
    xor rax, rax
    rep stosb
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 199
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 199
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_200_12:
    lea r14, [rsp - 801]
    mov r13, 1
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 203
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 203
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 801]
    mov r14, 0
    mov r13, 204
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 204
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_209_12:
    lea r14, [rsp - 801]
    mov r13, 0
    mov r12, 209
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 209
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
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
        mov rcx, 8
        lea r13, [rsp - 801]
        mov r12, 0
        mov r11, 211
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 211
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 801]
        mov r12, 1
        mov r11, 212
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 212
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
    mov rcx, 128
    lea rdi, [rsp - 929]
    xor rax, rax
    rep stosb
    print_str_217_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        print_24_4_217_5:
                mov rax, 1
                mov rdi, 0
            syscall
        print_24_4_217_5_end:
    print_str_217_5_end:
    loop_218_5:
        print_str_219_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            print_24_4_219_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_24_4_219_9_end:
        print_str_219_9_end:
        str_in_220_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 928]
                mov rdx, 127
            syscall
                mov byte [rsp - 929], al
                sub byte [rsp - 929], 1
        str_in_220_9_end:
        if_221_12:
        cmp_221_12:
        cmp byte [rsp - 929], 0
        jne if_223_19
        if_221_12_code:
            jmp loop_218_5_end
        jmp if_221_9_end
        if_223_19:
        cmp_223_19:
        cmp byte [rsp - 929], 4
        jg if_else_221_9
        if_223_19_code:
            print_str_224_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                print_24_4_224_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_224_13_end:
            print_str_224_13_end:
            jmp loop_218_5
        jmp if_221_9_end
        if_else_221_9:
            print_str_227_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                print_24_4_227_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_227_13_end:
            print_str_227_13_end:
            str_out_228_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 928]
                    movsx rdx, byte [rsp - 929]
                syscall
            str_out_228_13_end:
            print_str_229_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                print_24_4_229_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_229_13_end:
            print_str_229_13_end:
            print_str_230_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                print_24_4_230_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_230_13_end:
            print_str_230_13_end:
        if_221_9_end:
    jmp loop_218_5
    loop_218_5_end:
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
db `\n`
db `.`
db `hello `
db `that is not a name.\n`
db `enter name:\n`
db `hello world from baz\n`
dat.len equ $ - dat
