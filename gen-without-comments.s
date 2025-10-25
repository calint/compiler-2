DEFAULT REL
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $ - hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $ - prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $ - prompt2
prompt3: db 'hello '
prompt3.len equ $ - prompt3
dot: db '.'
dot.len equ $ - dot
nl: db '', 10,''
nl.len equ $ - nl
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    mov qword [rsp - 16], 0
    mov qword [rsp - 8], 0
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    mov r14, 107
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, 108
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    mov r13, 108
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_109_12:
    mov r14, 1
    mov r13, 109
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_109_12
    bool_true_109_12:
    mov r15b, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15b, false
    bool_end_109_12:
    assert_109_5:
        if_20_29_109_5:
        cmp_20_29_109_5:
        test r15b, r15b
        jne if_20_26_109_5_end
        if_20_29_109_5_code:
            mov rdi, 1
            exit_20_38_109_5:
                    mov rax, 60
                syscall
            exit_20_38_109_5_end:
        if_20_26_109_5_end:
    assert_109_5_end:
    cmp_110_12:
    mov r14, 2
    mov r13, 110
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_110_12
    bool_true_110_12:
    mov r15b, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15b, false
    bool_end_110_12:
    assert_110_5:
        if_20_29_110_5:
        cmp_20_29_110_5:
        test r15b, r15b
        jne if_20_26_110_5_end
        if_20_29_110_5_code:
            mov rdi, 1
            exit_20_38_110_5:
                    mov rax, 60
                syscall
            exit_20_38_110_5_end:
        if_20_26_110_5_end:
    assert_110_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 112
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    mov r15, 112
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_114_12:
    mov r14, 0
    mov r13, 114
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_114_12
    bool_true_114_12:
    mov r15b, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15b, false
    bool_end_114_12:
    assert_114_5:
        if_20_29_114_5:
        cmp_20_29_114_5:
        test r15b, r15b
        jne if_20_26_114_5_end
        if_20_29_114_5_code:
            mov rdi, 1
            exit_20_38_114_5:
                    mov rax, 60
                syscall
            exit_20_38_114_5_end:
        if_20_26_114_5_end:
    assert_114_5_end:
    mov qword [rsp - 56], 0
    mov qword [rsp - 48], 0
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    mov rcx, 4
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_118_12:
        mov rcx, 4
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_118_12
        mov r14, false
        jmp cmps_end_118_12
        cmps_eq_118_12:
        mov r14, true
        cmps_end_118_12:
    test r14, r14
    je bool_false_118_12
    bool_true_118_12:
    mov r15b, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15b, false
    bool_end_118_12:
    assert_118_5:
        if_20_29_118_5:
        cmp_20_29_118_5:
        test r15b, r15b
        jne if_20_26_118_5_end
        if_20_29_118_5_code:
            mov rdi, 1
            exit_20_38_118_5:
                    mov rax, 60
                syscall
            exit_20_38_118_5_end:
        if_20_26_118_5_end:
    assert_118_5_end:
    mov r15, 2
    mov r14, 121
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_122_12:
        mov rcx, 4
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_122_16
        mov r14, false
        jmp cmps_end_122_16
        cmps_eq_122_16:
        mov r14, true
        cmps_end_122_16:
    test r14, r14
    jne bool_false_122_12
    bool_true_122_12:
    mov r15b, true
    jmp bool_end_122_12
    bool_false_122_12:
    mov r15b, false
    bool_end_122_12:
    assert_122_5:
        if_20_29_122_5:
        cmp_20_29_122_5:
        test r15b, r15b
        jne if_20_26_122_5_end
        if_20_29_122_5_code:
            mov rdi, 1
            exit_20_38_122_5:
                    mov rax, 60
                syscall
            exit_20_38_122_5_end:
        if_20_26_122_5_end:
    assert_122_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    mov r13, 125
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_125_16:
        mov r13d, dword [rsp + r14 * 4 - 16]
        mov dword [rsp + r15 * 4 - 16], r13d
        not dword [rsp + r15 * 4 - 16]
    inv_125_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_126_12:
    mov r14, qword [rsp - 24]
    mov r13, 126
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_126_12
    bool_true_126_12:
    mov r15b, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15b, false
    bool_end_126_12:
    assert_126_5:
        if_20_29_126_5:
        cmp_20_29_126_5:
        test r15b, r15b
        jne if_20_26_126_5_end
        if_20_29_126_5_code:
            mov rdi, 1
            exit_20_38_126_5:
                    mov rax, 60
                syscall
            exit_20_38_126_5_end:
        if_20_26_126_5_end:
    assert_126_5_end:
    faz_128_5:
        mov r15, 1
        mov r14, 76
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_128_5_end:
    cmp_129_12:
    mov r14, 1
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 0xfe
    jne bool_false_129_12
    bool_true_129_12:
    mov r15b, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15b, false
    bool_end_129_12:
    assert_129_5:
        if_20_29_129_5:
        cmp_20_29_129_5:
        test r15b, r15b
        jne if_20_26_129_5_end
        if_20_29_129_5_code:
            mov rdi, 1
            exit_20_38_129_5:
                    mov rax, 60
                syscall
            exit_20_38_129_5_end:
        if_20_26_129_5_end:
    assert_129_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_132_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_132_5_end:
    cmp_133_12:
    cmp qword [rsp - 72], 2
    jne bool_false_133_12
    bool_true_133_12:
    mov r15b, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15b, false
    bool_end_133_12:
    assert_133_5:
        if_20_29_133_5:
        cmp_20_29_133_5:
        test r15b, r15b
        jne if_20_26_133_5_end
        if_20_29_133_5_code:
            mov rdi, 1
            exit_20_38_133_5:
                    mov rax, 60
                syscall
            exit_20_38_133_5_end:
        if_20_26_133_5_end:
    assert_133_5_end:
    cmp_134_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_134_12
    bool_true_134_12:
    mov r15b, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15b, false
    bool_end_134_12:
    assert_134_5:
        if_20_29_134_5:
        cmp_20_29_134_5:
        test r15b, r15b
        jne if_20_26_134_5_end
        if_20_29_134_5_code:
            mov rdi, 1
            exit_20_38_134_5:
                    mov rax, 60
                syscall
            exit_20_38_134_5_end:
        if_20_26_134_5_end:
    assert_134_5_end:
    lea rsi, [rsp - 72]
    lea rdi, [rsp - 88]
    mov rcx, 2
    rep movsq
    cmp_137_12:
        lea rsi, [rsp - 72]
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
        je cmps_eq_137_12
        mov r14, false
        jmp cmps_end_137_12
        cmps_eq_137_12:
        mov r14, true
        cmps_end_137_12:
    test r14, r14
    je bool_false_137_12
    bool_true_137_12:
    mov r15b, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15b, false
    bool_end_137_12:
    assert_137_5:
        if_20_29_137_5:
        cmp_20_29_137_5:
        test r15b, r15b
        jne if_20_26_137_5_end
        if_20_29_137_5_code:
            mov rdi, 1
            exit_20_38_137_5:
                    mov rax, 60
                syscall
            exit_20_38_137_5_end:
        if_20_26_137_5_end:
    assert_137_5_end:
    mov qword [rsp - 88], 3
    cmp_142_12:
        lea rsi, [rsp - 72]
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
        je cmps_eq_142_16
        mov r14, false
        jmp cmps_end_142_16
        cmps_eq_142_16:
        mov r14, true
        cmps_end_142_16:
    test r14, r14
    jne bool_false_142_12
    bool_true_142_12:
    mov r15b, true
    jmp bool_end_142_12
    bool_false_142_12:
    mov r15b, false
    bool_end_142_12:
    assert_142_5:
        if_20_29_142_5:
        cmp_20_29_142_5:
        test r15b, r15b
        jne if_20_26_142_5_end
        if_20_29_142_5_code:
            mov rdi, 1
            exit_20_38_142_5:
                    mov rax, 60
                syscall
            exit_20_38_142_5_end:
        if_20_26_142_5_end:
    assert_142_5_end:
    mov qword [rsp - 96], 0
    bar_145_5:
        if_59_8_145_5:
        cmp_59_8_145_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_145_5_end
        if_59_8_145_5_code:
            jmp bar_145_5_end
        if_59_5_145_5_end:
        mov qword [rsp - 96], 0xff
    bar_145_5_end:
    cmp_146_12:
    cmp qword [rsp - 96], 0
    jne bool_false_146_12
    bool_true_146_12:
    mov r15b, true
    jmp bool_end_146_12
    bool_false_146_12:
    mov r15b, false
    bool_end_146_12:
    assert_146_5:
        if_20_29_146_5:
        cmp_20_29_146_5:
        test r15b, r15b
        jne if_20_26_146_5_end
        if_20_29_146_5_code:
            mov rdi, 1
            exit_20_38_146_5:
                    mov rax, 60
                syscall
            exit_20_38_146_5_end:
        if_20_26_146_5_end:
    assert_146_5_end:
    mov qword [rsp - 96], 1
    bar_149_5:
        if_59_8_149_5:
        cmp_59_8_149_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_149_5_end
        if_59_8_149_5_code:
            jmp bar_149_5_end
        if_59_5_149_5_end:
        mov qword [rsp - 96], 0xff
    bar_149_5_end:
    cmp_150_12:
    cmp qword [rsp - 96], 0xff
    jne bool_false_150_12
    bool_true_150_12:
    mov r15b, true
    jmp bool_end_150_12
    bool_false_150_12:
    mov r15b, false
    bool_end_150_12:
    assert_150_5:
        if_20_29_150_5:
        cmp_20_29_150_5:
        test r15b, r15b
        jne if_20_26_150_5_end
        if_20_29_150_5_code:
            mov rdi, 1
            exit_20_38_150_5:
                    mov rax, 60
                syscall
            exit_20_38_150_5_end:
        if_20_26_150_5_end:
    assert_150_5_end:
    mov qword [rsp - 104], 1
    baz_153_13:
        mov r15, qword [rsp - 104]
        imul r15, 2
    baz_153_13_end:
    mov qword [rsp - 112], r15
    cmp_154_12:
    cmp qword [rsp - 112], 2
    jne bool_false_154_12
    bool_true_154_12:
    mov r15b, true
    jmp bool_end_154_12
    bool_false_154_12:
    mov r15b, false
    bool_end_154_12:
    assert_154_5:
        if_20_29_154_5:
        cmp_20_29_154_5:
        test r15b, r15b
        jne if_20_26_154_5_end
        if_20_29_154_5_code:
            mov rdi, 1
            exit_20_38_154_5:
                    mov rax, 60
                syscall
            exit_20_38_154_5_end:
        if_20_26_154_5_end:
    assert_154_5_end:
    baz_156_9:
        mov r15, 1
        imul r15, 2
    baz_156_9_end:
    mov qword [rsp - 112], r15
    cmp_157_12:
    cmp qword [rsp - 112], 2
    jne bool_false_157_12
    bool_true_157_12:
    mov r15b, true
    jmp bool_end_157_12
    bool_false_157_12:
    mov r15b, false
    bool_end_157_12:
    assert_157_5:
        if_20_29_157_5:
        cmp_20_29_157_5:
        test r15b, r15b
        jne if_20_26_157_5_end
        if_20_29_157_5_code:
            mov rdi, 1
            exit_20_38_157_5:
                    mov rax, 60
                syscall
            exit_20_38_157_5_end:
        if_20_26_157_5_end:
    assert_157_5_end:
    baz_159_23:
        mov qword [rsp - 128], 2
        mov r15, qword [rsp - 128]
        imul r15, 2
        mov qword [rsp - 128], r15
    baz_159_23_end:
    mov qword [rsp - 120], 0
    cmp_160_12:
    cmp qword [rsp - 128], 4
    jne bool_false_160_12
    bool_true_160_12:
    mov r15b, true
    jmp bool_end_160_12
    bool_false_160_12:
    mov r15b, false
    bool_end_160_12:
    assert_160_5:
        if_20_29_160_5:
        cmp_20_29_160_5:
        test r15b, r15b
        jne if_20_26_160_5_end
        if_20_29_160_5_code:
            mov rdi, 1
            exit_20_38_160_5:
                    mov rax, 60
                syscall
            exit_20_38_160_5_end:
        if_20_26_160_5_end:
    assert_160_5_end:
    mov qword [rsp - 136], 1
    mov qword [rsp - 144], 2
    mov r15, qword [rsp - 136]
    mov qword [rsp - 164], r15
    mov r15, qword [rsp - 164]
    imul r15, 10
    mov qword [rsp - 164], r15
    mov r15, qword [rsp - 144]
    mov qword [rsp - 156], r15
    mov dword [rsp - 148], 0xff0000
    cmp_166_12:
    cmp qword [rsp - 164], 10
    jne bool_false_166_12
    bool_true_166_12:
    mov r15b, true
    jmp bool_end_166_12
    bool_false_166_12:
    mov r15b, false
    bool_end_166_12:
    assert_166_5:
        if_20_29_166_5:
        cmp_20_29_166_5:
        test r15b, r15b
        jne if_20_26_166_5_end
        if_20_29_166_5_code:
            mov rdi, 1
            exit_20_38_166_5:
                    mov rax, 60
                syscall
            exit_20_38_166_5_end:
        if_20_26_166_5_end:
    assert_166_5_end:
    cmp_167_12:
    cmp qword [rsp - 156], 2
    jne bool_false_167_12
    bool_true_167_12:
    mov r15b, true
    jmp bool_end_167_12
    bool_false_167_12:
    mov r15b, false
    bool_end_167_12:
    assert_167_5:
        if_20_29_167_5:
        cmp_20_29_167_5:
        test r15b, r15b
        jne if_20_26_167_5_end
        if_20_29_167_5_code:
            mov rdi, 1
            exit_20_38_167_5:
                    mov rax, 60
                syscall
            exit_20_38_167_5_end:
        if_20_26_167_5_end:
    assert_167_5_end:
    cmp_168_12:
    cmp dword [rsp - 148], 0xff0000
    jne bool_false_168_12
    bool_true_168_12:
    mov r15b, true
    jmp bool_end_168_12
    bool_false_168_12:
    mov r15b, false
    bool_end_168_12:
    assert_168_5:
        if_20_29_168_5:
        cmp_20_29_168_5:
        test r15b, r15b
        jne if_20_26_168_5_end
        if_20_29_168_5_code:
            mov rdi, 1
            exit_20_38_168_5:
                    mov rax, 60
                syscall
            exit_20_38_168_5_end:
        if_20_26_168_5_end:
    assert_168_5_end:
    mov r15, qword [rsp - 136]
    mov qword [rsp - 180], r15
    neg qword [rsp - 180]
    mov r15, qword [rsp - 144]
    mov qword [rsp - 172], r15
    neg qword [rsp - 172]
    lea rsi, [rsp - 180]
    lea rdi, [rsp - 164]
    mov rcx, 2
    rep movsq
    cmp_172_12:
    cmp qword [rsp - 164], -1
    jne bool_false_172_12
    bool_true_172_12:
    mov r15b, true
    jmp bool_end_172_12
    bool_false_172_12:
    mov r15b, false
    bool_end_172_12:
    assert_172_5:
        if_20_29_172_5:
        cmp_20_29_172_5:
        test r15b, r15b
        jne if_20_26_172_5_end
        if_20_29_172_5_code:
            mov rdi, 1
            exit_20_38_172_5:
                    mov rax, 60
                syscall
            exit_20_38_172_5_end:
        if_20_26_172_5_end:
    assert_172_5_end:
    cmp_173_12:
    cmp qword [rsp - 156], -2
    jne bool_false_173_12
    bool_true_173_12:
    mov r15b, true
    jmp bool_end_173_12
    bool_false_173_12:
    mov r15b, false
    bool_end_173_12:
    assert_173_5:
        if_20_29_173_5:
        cmp_20_29_173_5:
        test r15b, r15b
        jne if_20_26_173_5_end
        if_20_29_173_5_code:
            mov rdi, 1
            exit_20_38_173_5:
                    mov rax, 60
                syscall
            exit_20_38_173_5_end:
        if_20_26_173_5_end:
    assert_173_5_end:
    lea rsi, [rsp - 164]
    lea rdi, [rsp - 200]
    mov rcx, 20
    rep movsb
    cmp_176_12:
    cmp qword [rsp - 200], -1
    jne bool_false_176_12
    bool_true_176_12:
    mov r15b, true
    jmp bool_end_176_12
    bool_false_176_12:
    mov r15b, false
    bool_end_176_12:
    assert_176_5:
        if_20_29_176_5:
        cmp_20_29_176_5:
        test r15b, r15b
        jne if_20_26_176_5_end
        if_20_29_176_5_code:
            mov rdi, 1
            exit_20_38_176_5:
                    mov rax, 60
                syscall
            exit_20_38_176_5_end:
        if_20_26_176_5_end:
    assert_176_5_end:
    cmp_177_12:
    cmp qword [rsp - 192], -2
    jne bool_false_177_12
    bool_true_177_12:
    mov r15b, true
    jmp bool_end_177_12
    bool_false_177_12:
    mov r15b, false
    bool_end_177_12:
    assert_177_5:
        if_20_29_177_5:
        cmp_20_29_177_5:
        test r15b, r15b
        jne if_20_26_177_5_end
        if_20_29_177_5_code:
            mov rdi, 1
            exit_20_38_177_5:
                    mov rax, 60
                syscall
            exit_20_38_177_5_end:
        if_20_26_177_5_end:
    assert_177_5_end:
    cmp_178_12:
    cmp dword [rsp - 184], 0xff0000
    jne bool_false_178_12
    bool_true_178_12:
    mov r15b, true
    jmp bool_end_178_12
    bool_false_178_12:
    mov r15b, false
    bool_end_178_12:
    assert_178_5:
        if_20_29_178_5:
        cmp_20_29_178_5:
        test r15b, r15b
        jne if_20_26_178_5_end
        if_20_29_178_5_code:
            mov rdi, 1
            exit_20_38_178_5:
                    mov rax, 60
                syscall
            exit_20_38_178_5_end:
        if_20_26_178_5_end:
    assert_178_5_end:
    mov qword [rsp - 220], 0
    mov qword [rsp - 212], 0
    mov dword [rsp - 204], 0
    mov qword [rsp - 212], 73
    cmp_183_12:
    lea r14, [rsp - 220]
    mov r13, 0
    mov r12, 183
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    jne bool_false_183_12
    bool_true_183_12:
    mov r15b, true
    jmp bool_end_183_12
    bool_false_183_12:
    mov r15b, false
    bool_end_183_12:
    assert_183_5:
        if_20_29_183_5:
        cmp_20_29_183_5:
        test r15b, r15b
        jne if_20_26_183_5_end
        if_20_29_183_5_code:
            mov rdi, 1
            exit_20_38_183_5:
                    mov rax, 60
                syscall
            exit_20_38_183_5_end:
        if_20_26_183_5_end:
    assert_183_5_end:
    mov rcx, 64
    lea rdi, [rsp - 732]
    xor rax, rax
    rep stosq
    lea r15, [rsp - 732]
    mov r14, 1
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 0xffee
    cmp_187_12:
    lea r14, [rsp - 732]
    mov r13, 1
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 0xffee
    jne bool_false_187_12
    bool_true_187_12:
    mov r15b, true
    jmp bool_end_187_12
    bool_false_187_12:
    mov r15b, false
    bool_end_187_12:
    assert_187_5:
        if_20_29_187_5:
        cmp_20_29_187_5:
        test r15b, r15b
        jne if_20_26_187_5_end
        if_20_29_187_5_code:
            mov rdi, 1
            exit_20_38_187_5:
                    mov rax, 60
                syscall
            exit_20_38_187_5_end:
        if_20_26_187_5_end:
    assert_187_5_end:
    mov rcx, 8
    lea r15, [rsp - 732]
    mov r14, 1
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 190
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 732]
    mov r14, 0
    mov r13, 191
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 191
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_196_12:
    lea r14, [rsp - 732]
    mov r13, 0
    mov r12, 196
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 196
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 0xffee
    jne bool_false_196_12
    bool_true_196_12:
    mov r15b, true
    jmp bool_end_196_12
    bool_false_196_12:
    mov r15b, false
    bool_end_196_12:
    assert_196_5:
        if_20_29_196_5:
        cmp_20_29_196_5:
        test r15b, r15b
        jne if_20_26_196_5_end
        if_20_29_196_5_code:
            mov rdi, 1
            exit_20_38_196_5:
                    mov rax, 60
                syscall
            exit_20_38_196_5_end:
        if_20_26_196_5_end:
    assert_196_5_end:
    cmp_197_12:
        mov rcx, 8
        lea r13, [rsp - 732]
        mov r12, 0
        mov r11, 198
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 198
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 732]
        mov r12, 1
        mov r11, 199
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 199
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        je cmps_eq_197_12
        mov r14, false
        jmp cmps_end_197_12
        cmps_eq_197_12:
        mov r14, true
        cmps_end_197_12:
    test r14, r14
    je bool_false_197_12
    bool_true_197_12:
    mov r15b, true
    jmp bool_end_197_12
    bool_false_197_12:
    mov r15b, false
    bool_end_197_12:
    assert_197_5:
        if_20_29_197_5:
        cmp_20_29_197_5:
        test r15b, r15b
        jne if_20_26_197_5_end
        if_20_29_197_5_code:
            mov rdi, 1
            exit_20_38_197_5:
                    mov rax, 60
                syscall
            exit_20_38_197_5_end:
        if_20_26_197_5_end:
    assert_197_5_end:
    mov rcx, 16
    lea rdi, [rsp - 860]
    xor rax, rax
    rep stosq
    mov rdx, hello.len
    mov rsi, hello
    print_204_5:
            mov rax, 1
            mov rdi, 0
        syscall
    print_204_5_end:
    loop_205_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_206_9:
                mov rax, 1
                mov rdi, 0
            syscall
        print_206_9_end:
        str_in_207_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 859]
                mov rdx, 127
            syscall
                mov byte [rsp - 860], al
                sub byte [rsp - 860], 1
        str_in_207_9_end:
        if_208_12:
        cmp_208_12:
        cmp byte [rsp - 860], 0
        jne if_210_19
        if_208_12_code:
            jmp loop_205_5_end
        jmp if_208_9_end
        if_210_19:
        cmp_210_19:
        cmp byte [rsp - 860], 4
        jg if_else_208_9
        if_210_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_211_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_211_13_end:
            jmp loop_205_5
        jmp if_208_9_end
        if_else_208_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_214_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_214_13_end:
            str_out_215_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 859]
                    movsx rdx, byte [rsp - 860]
                syscall
            str_out_215_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_216_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_216_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_217_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_217_13_end:
        if_208_9_end:
    jmp loop_205_5
    loop_205_5_end:
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
