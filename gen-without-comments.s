default rel
section .bss
stk resd 65536
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
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, qword [rsp - 221]
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_132_12:
    mov r14, 1
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
    lea rsi, [rsp + r15 * 4 - 205]
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_137_12:
    mov r14, 0
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
    lea rsi, [rsp - 205]
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_141_12:
        mov rcx, 4
        lea rsi, [rsp - 205]
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
    mov dword [rsp + r15 * 4 - 253], -1
    cmp_145_12:
        mov rcx, 4
        lea rsi, [rsp - 205]
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
    mov r14, qword [rsp - 221]
    sub r14, 1
    inv_148_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_148_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_149_12:
    mov r14, qword [rsp - 221]
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
        mov dword [rsp + r15 * 4 - 205], 254
    faz_151_5_end:
    cmp_152_12:
    mov r14, 1
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
    mov rcx, 2
    rep movsq
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
    mov rcx, 2
    rep movsq
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
    cmp_206_12:
    lea r14, [rsp - 417]
    mov r13, 0
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_206_12:
    assert_206_5:
        if_13_29_206_5:
        cmp_13_29_206_5:
        cmp r15b, 0
        jne if_13_26_206_5_end
        if_13_29_206_5_code:
            mov rdi, 1
            exit_13_38_206_5:
                    mov rax, 60
                syscall
            exit_13_38_206_5_end:
        if_13_26_206_5_end:
    assert_206_5_end:
    mov rcx, 512
    lea rdi, [rsp - 929]
    xor rax, rax
    rep stosb
    lea r15, [rsp - 929]
    mov r14, 1
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov qword [r15 + r14 * 8], 65518
    cmp_210_12:
    lea r14, [rsp - 929]
    mov r13, 1
    shl r13, 6
    add r14, r13
    mov r13, 1
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    lea r15, [rsp - 929]
    mov r14, 1
    shl r14, 6
    add r15, r14
    lea rsi, [r15]
    lea r15, [rsp - 929]
    mov r14, 0
    shl r14, 6
    add r15, r14
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_219_12:
    lea r14, [rsp - 929]
    mov r13, 0
    shl r13, 6
    add r14, r13
    mov r13, 1
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_219_12:
    assert_219_5:
        if_13_29_219_5:
        cmp_13_29_219_5:
        cmp r15b, 0
        jne if_13_26_219_5_end
        if_13_29_219_5_code:
            mov rdi, 1
            exit_13_38_219_5:
                    mov rax, 60
                syscall
            exit_13_38_219_5_end:
        if_13_26_219_5_end:
    assert_219_5_end:
    cmp_220_12:
        mov rcx, 8
        lea r13, [rsp - 929]
        mov r12, 0
        shl r12, 6
        add r13, r12
        lea rsi, [r13]
        lea r13, [rsp - 929]
        mov r12, 1
        shl r12, 6
        add r13, r12
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov rcx, 128
    lea rdi, [rsp - 1057]
    xor rax, rax
    rep stosb
    print_227_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_24_4_227_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_24_4_227_5_end:
    print_227_5_end:
    loop_228_5:
        print_229_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_24_4_229_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_24_4_229_9_end:
        print_229_9_end:
        str_in_230_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1056]
                mov rdx, 127
            syscall
                mov byte [rsp - 1057], al
                sub byte [rsp - 1057], 1
        str_in_230_9_end:
        if_231_12:
        cmp_231_12:
        cmp byte [rsp - 1057], 0
        jne if_233_19
        if_231_12_code:
            jmp loop_228_5_end
        jmp if_231_9_end
        if_233_19:
        cmp_233_19:
        cmp byte [rsp - 1057], 4
        jg if_else_231_9
        if_233_19_code:
            print_234_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_24_4_234_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_234_13_end:
            print_234_13_end:
            jmp loop_228_5
        jmp if_231_9_end
        if_else_231_9:
            print_237_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_24_4_237_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_237_13_end:
            print_237_13_end:
            str_out_238_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1056]
                    movsx rdx, byte [rsp - 1057]
                syscall
            str_out_238_13_end:
            print_239_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_24_4_239_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_239_13_end:
            print_239_13_end:
            print_240_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_24_4_240_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_240_13_end:
            print_240_13_end:
        if_231_9_end:
    jmp loop_228_5
    loop_228_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
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
