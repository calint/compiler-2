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
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    mov r14, 85
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, 86
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    mov r13, 86
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_87_12:
        mov r13, 1
        mov r12, 87
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_87_12
    jmp bool_true_87_12
    bool_true_87_12:
    mov r15, true
    jmp bool_end_87_12
    bool_false_87_12:
    mov r15, false
    bool_end_87_12:
    assert_87_5:
        if_20_29_87_5:
        cmp_20_29_87_5:
        test r15, r15
        jne if_20_26_87_5_end
        jmp if_20_29_87_5_code
        if_20_29_87_5_code:
            mov rdi, 1
            exit_20_38_87_5:
                    mov rax, 60
                syscall
            exit_20_38_87_5_end:
        if_20_26_87_5_end:
    assert_87_5_end:
    cmp_88_12:
        mov r13, 2
        mov r12, 88
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_88_12
    jmp bool_true_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
    assert_88_5:
        if_20_29_88_5:
        cmp_20_29_88_5:
        test r15, r15
        jne if_20_26_88_5_end
        jmp if_20_29_88_5_code
        if_20_29_88_5_code:
            mov rdi, 1
            exit_20_38_88_5:
                    mov rax, 60
                syscall
            exit_20_38_88_5_end:
        if_20_26_88_5_end:
    assert_88_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 90
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    mov r15, 90
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_92_12:
        mov r13, 0
        mov r12, 92
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_92_12
    jmp bool_true_92_12
    bool_true_92_12:
    mov r15, true
    jmp bool_end_92_12
    bool_false_92_12:
    mov r15, false
    bool_end_92_12:
    assert_92_5:
        if_20_29_92_5:
        cmp_20_29_92_5:
        test r15, r15
        jne if_20_26_92_5_end
        jmp if_20_29_92_5_code
        if_20_29_92_5_code:
            mov rdi, 1
            exit_20_38_92_5:
                    mov rax, 60
                syscall
            exit_20_38_92_5_end:
        if_20_26_92_5_end:
    assert_92_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    mov r15, 95
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    mov r15, 95
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_96_12:
        mov rcx, 4
        mov r13, 96
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 96
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_96_12
        mov r14, false
        jmp cmps_end_96_12
        cmps_eq_96_12:
        mov r14, true
        cmps_end_96_12:
    test r14, r14
    je bool_false_96_12
    jmp bool_true_96_12
    bool_true_96_12:
    mov r15, true
    jmp bool_end_96_12
    bool_false_96_12:
    mov r15, false
    bool_end_96_12:
    assert_96_5:
        if_20_29_96_5:
        cmp_20_29_96_5:
        test r15, r15
        jne if_20_26_96_5_end
        jmp if_20_29_96_5_code
        if_20_29_96_5_code:
            mov rdi, 1
            exit_20_38_96_5:
                    mov rax, 60
                syscall
            exit_20_38_96_5_end:
        if_20_26_96_5_end:
    assert_96_5_end:
    mov r15, 2
    mov r14, 99
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_100_12:
        mov rcx, 4
        mov r13, 100
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 100
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_100_16
        mov r14, false
        jmp cmps_end_100_16
        cmps_eq_100_16:
        mov r14, true
        cmps_end_100_16:
    test r14, r14
    jne bool_false_100_12
    jmp bool_true_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
    assert_100_5:
        if_20_29_100_5:
        cmp_20_29_100_5:
        test r15, r15
        jne if_20_26_100_5_end
        jmp if_20_29_100_5_code
        if_20_29_100_5_code:
            mov rdi, 1
            exit_20_38_100_5:
                    mov rax, 60
                syscall
            exit_20_38_100_5_end:
        if_20_26_100_5_end:
    assert_100_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r14, 103
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    mov r13, 103
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_103_16:
        lea r13, [rsp + r14 * 4 - 16]
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
        not dword [rsp + r15 * 4 - 16]
    inv_103_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_104_12:
        mov r13, qword [rsp - 24]
        mov r12, 104
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_104_12
    jmp bool_true_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_20_29_104_5:
        cmp_20_29_104_5:
        test r15, r15
        jne if_20_26_104_5_end
        jmp if_20_29_104_5_code
        if_20_29_104_5_code:
            mov rdi, 1
            exit_20_38_104_5:
                    mov rax, 60
                syscall
            exit_20_38_104_5_end:
        if_20_26_104_5_end:
    assert_104_5_end:
    faz_106_5:
        mov r15, 1
        mov r14, 76
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_106_5_end:
    cmp_107_12:
        mov r13, 1
        mov r12, 107
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 0xfe
    jne bool_false_107_12
    jmp bool_true_107_12
    bool_true_107_12:
    mov r15, true
    jmp bool_end_107_12
    bool_false_107_12:
    mov r15, false
    bool_end_107_12:
    assert_107_5:
        if_20_29_107_5:
        cmp_20_29_107_5:
        test r15, r15
        jne if_20_26_107_5_end
        jmp if_20_29_107_5_code
        if_20_29_107_5_code:
            mov rdi, 1
            exit_20_38_107_5:
                    mov rax, 60
                syscall
            exit_20_38_107_5_end:
        if_20_26_107_5_end:
    assert_107_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_110_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_110_5_end:
    cmp_111_12:
    cmp qword [rsp - 72], 2
    jne bool_false_111_12
    jmp bool_true_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_20_29_111_5:
        cmp_20_29_111_5:
        test r15, r15
        jne if_20_26_111_5_end
        jmp if_20_29_111_5_code
        if_20_29_111_5_code:
            mov rdi, 1
            exit_20_38_111_5:
                    mov rax, 60
                syscall
            exit_20_38_111_5_end:
        if_20_26_111_5_end:
    assert_111_5_end:
    cmp_112_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_112_12
    jmp bool_true_112_12
    bool_true_112_12:
    mov r15, true
    jmp bool_end_112_12
    bool_false_112_12:
    mov r15, false
    bool_end_112_12:
    assert_112_5:
        if_20_29_112_5:
        cmp_20_29_112_5:
        test r15, r15
        jne if_20_26_112_5_end
        jmp if_20_29_112_5_code
        if_20_29_112_5_code:
            mov rdi, 1
            exit_20_38_112_5:
                    mov rax, 60
                syscall
            exit_20_38_112_5_end:
        if_20_26_112_5_end:
    assert_112_5_end:
    mov qword [rsp - 80], 0
    bar_115_5:
        if_59_8_115_5:
        cmp_59_8_115_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_115_5_end
        jmp if_59_8_115_5_code
        if_59_8_115_5_code:
            jmp bar_115_5_end
        if_59_5_115_5_end:
        mov qword [rsp - 80], 0xff
    bar_115_5_end:
    cmp_116_12:
    cmp qword [rsp - 80], 0
    jne bool_false_116_12
    jmp bool_true_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
    assert_116_5:
        if_20_29_116_5:
        cmp_20_29_116_5:
        test r15, r15
        jne if_20_26_116_5_end
        jmp if_20_29_116_5_code
        if_20_29_116_5_code:
            mov rdi, 1
            exit_20_38_116_5:
                    mov rax, 60
                syscall
            exit_20_38_116_5_end:
        if_20_26_116_5_end:
    assert_116_5_end:
    mov qword [rsp - 80], 1
    bar_119_5:
        if_59_8_119_5:
        cmp_59_8_119_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_119_5_end
        jmp if_59_8_119_5_code
        if_59_8_119_5_code:
            jmp bar_119_5_end
        if_59_5_119_5_end:
        mov qword [rsp - 80], 0xff
    bar_119_5_end:
    cmp_120_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_120_12
    jmp bool_true_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_20_29_120_5:
        cmp_20_29_120_5:
        test r15, r15
        jne if_20_26_120_5_end
        jmp if_20_29_120_5_code
        if_20_29_120_5_code:
            mov rdi, 1
            exit_20_38_120_5:
                    mov rax, 60
                syscall
            exit_20_38_120_5_end:
        if_20_26_120_5_end:
    assert_120_5_end:
    mov qword [rsp - 88], 1
    baz_123_13:
        mov r15, qword [rsp - 88]
        imul r15, 2
    baz_123_13_end:
    mov qword [rsp - 96], r15
    cmp_124_12:
    cmp qword [rsp - 96], 2
    jne bool_false_124_12
    jmp bool_true_124_12
    bool_true_124_12:
    mov r15, true
    jmp bool_end_124_12
    bool_false_124_12:
    mov r15, false
    bool_end_124_12:
    assert_124_5:
        if_20_29_124_5:
        cmp_20_29_124_5:
        test r15, r15
        jne if_20_26_124_5_end
        jmp if_20_29_124_5_code
        if_20_29_124_5_code:
            mov rdi, 1
            exit_20_38_124_5:
                    mov rax, 60
                syscall
            exit_20_38_124_5_end:
        if_20_26_124_5_end:
    assert_124_5_end:
    baz_126_9:
        mov r15, 1
        imul r15, 2
    baz_126_9_end:
    mov qword [rsp - 96], r15
    cmp_127_12:
    cmp qword [rsp - 96], 2
    jne bool_false_127_12
    jmp bool_true_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
    assert_127_5:
        if_20_29_127_5:
        cmp_20_29_127_5:
        test r15, r15
        jne if_20_26_127_5_end
        jmp if_20_29_127_5_code
        if_20_29_127_5_code:
            mov rdi, 1
            exit_20_38_127_5:
                    mov rax, 60
                syscall
            exit_20_38_127_5_end:
        if_20_26_127_5_end:
    assert_127_5_end:
    baz_129_23:
        mov r15, 2
        imul r15, 2
    baz_129_23_end:
    mov qword [rsp - 112], r15
    mov qword [rsp - 104], 0
    cmp_130_12:
    cmp qword [rsp - 112], 4
    jne bool_false_130_12
    jmp bool_true_130_12
    bool_true_130_12:
    mov r15, true
    jmp bool_end_130_12
    bool_false_130_12:
    mov r15, false
    bool_end_130_12:
    assert_130_5:
        if_20_29_130_5:
        cmp_20_29_130_5:
        test r15, r15
        jne if_20_26_130_5_end
        jmp if_20_29_130_5_code
        if_20_29_130_5_code:
            mov rdi, 1
            exit_20_38_130_5:
                    mov rax, 60
                syscall
            exit_20_38_130_5_end:
        if_20_26_130_5_end:
    assert_130_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_136_12:
    cmp qword [rsp - 148], 10
    jne bool_false_136_12
    jmp bool_true_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_20_29_136_5:
        cmp_20_29_136_5:
        test r15, r15
        jne if_20_26_136_5_end
        jmp if_20_29_136_5_code
        if_20_29_136_5_code:
            mov rdi, 1
            exit_20_38_136_5:
                    mov rax, 60
                syscall
            exit_20_38_136_5_end:
        if_20_26_136_5_end:
    assert_136_5_end:
    cmp_137_12:
    cmp qword [rsp - 140], 2
    jne bool_false_137_12
    jmp bool_true_137_12
    bool_true_137_12:
    mov r15, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15, false
    bool_end_137_12:
    assert_137_5:
        if_20_29_137_5:
        cmp_20_29_137_5:
        test r15, r15
        jne if_20_26_137_5_end
        jmp if_20_29_137_5_code
        if_20_29_137_5_code:
            mov rdi, 1
            exit_20_38_137_5:
                    mov rax, 60
                syscall
            exit_20_38_137_5_end:
        if_20_26_137_5_end:
    assert_137_5_end:
    cmp_138_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_138_12
    jmp bool_true_138_12
    bool_true_138_12:
    mov r15, true
    jmp bool_end_138_12
    bool_false_138_12:
    mov r15, false
    bool_end_138_12:
    assert_138_5:
        if_20_29_138_5:
        cmp_20_29_138_5:
        test r15, r15
        jne if_20_26_138_5_end
        jmp if_20_29_138_5_code
        if_20_29_138_5_code:
            mov rdi, 1
            exit_20_38_138_5:
                    mov rax, 60
                syscall
            exit_20_38_138_5_end:
        if_20_26_138_5_end:
    assert_138_5_end:
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
    neg qword [rsp - 164]
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
    neg qword [rsp - 156]
    lea rdi, [rsp - 148]
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
    cmp_142_12:
    cmp qword [rsp - 148], -1
    jne bool_false_142_12
    jmp bool_true_142_12
    bool_true_142_12:
    mov r15, true
    jmp bool_end_142_12
    bool_false_142_12:
    mov r15, false
    bool_end_142_12:
    assert_142_5:
        if_20_29_142_5:
        cmp_20_29_142_5:
        test r15, r15
        jne if_20_26_142_5_end
        jmp if_20_29_142_5_code
        if_20_29_142_5_code:
            mov rdi, 1
            exit_20_38_142_5:
                    mov rax, 60
                syscall
            exit_20_38_142_5_end:
        if_20_26_142_5_end:
    assert_142_5_end:
    cmp_143_12:
    cmp qword [rsp - 140], -2
    jne bool_false_143_12
    jmp bool_true_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
    assert_143_5:
        if_20_29_143_5:
        cmp_20_29_143_5:
        test r15, r15
        jne if_20_26_143_5_end
        jmp if_20_29_143_5_code
        if_20_29_143_5_code:
            mov rdi, 1
            exit_20_38_143_5:
                    mov rax, 60
                syscall
            exit_20_38_143_5_end:
        if_20_26_143_5_end:
    assert_143_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_146_12:
    cmp qword [rsp - 184], -1
    jne bool_false_146_12
    jmp bool_true_146_12
    bool_true_146_12:
    mov r15, true
    jmp bool_end_146_12
    bool_false_146_12:
    mov r15, false
    bool_end_146_12:
    assert_146_5:
        if_20_29_146_5:
        cmp_20_29_146_5:
        test r15, r15
        jne if_20_26_146_5_end
        jmp if_20_29_146_5_code
        if_20_29_146_5_code:
            mov rdi, 1
            exit_20_38_146_5:
                    mov rax, 60
                syscall
            exit_20_38_146_5_end:
        if_20_26_146_5_end:
    assert_146_5_end:
    cmp_147_12:
    cmp qword [rsp - 176], -2
    jne bool_false_147_12
    jmp bool_true_147_12
    bool_true_147_12:
    mov r15, true
    jmp bool_end_147_12
    bool_false_147_12:
    mov r15, false
    bool_end_147_12:
    assert_147_5:
        if_20_29_147_5:
        cmp_20_29_147_5:
        test r15, r15
        jne if_20_26_147_5_end
        jmp if_20_29_147_5_code
        if_20_29_147_5_code:
            mov rdi, 1
            exit_20_38_147_5:
                    mov rax, 60
                syscall
            exit_20_38_147_5_end:
        if_20_26_147_5_end:
    assert_147_5_end:
    cmp_148_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_148_12
    jmp bool_true_148_12
    bool_true_148_12:
    mov r15, true
    jmp bool_end_148_12
    bool_false_148_12:
    mov r15, false
    bool_end_148_12:
    assert_148_5:
        if_20_29_148_5:
        cmp_20_29_148_5:
        test r15, r15
        jne if_20_26_148_5_end
        jmp if_20_29_148_5_code
        if_20_29_148_5_code:
            mov rdi, 1
            exit_20_38_148_5:
                    mov rax, 60
                syscall
            exit_20_38_148_5_end:
        if_20_26_148_5_end:
    assert_148_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_153_12:
        lea r13, [rsp - 204]
        mov r12, 0
        mov r11, 153
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 1
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        mov r14, qword [r13 + 8]
    cmp r14, 73
    jne bool_false_153_12
    jmp bool_true_153_12
    bool_true_153_12:
    mov r15, true
    jmp bool_end_153_12
    bool_false_153_12:
    mov r15, false
    bool_end_153_12:
    assert_153_5:
        if_20_29_153_5:
        cmp_20_29_153_5:
        test r15, r15
        jne if_20_26_153_5_end
        jmp if_20_29_153_5_code
        if_20_29_153_5_code:
            mov rdi, 1
            exit_20_38_153_5:
                    mov rax, 60
                syscall
            exit_20_38_153_5_end:
        if_20_26_153_5_end:
    assert_153_5_end:
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
    lea r15, [rsp - 796]
    mov r14, 1
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 158
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 0xffee
    cmp_159_12:
        lea r13, [rsp - 796]
        mov r12, 1
        mov r11, 159
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        mov r11, 159
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8]
    cmp r14, 0xffee
    jne bool_false_159_12
    jmp bool_true_159_12
    bool_true_159_12:
    mov r15, true
    jmp bool_end_159_12
    bool_false_159_12:
    mov r15, false
    bool_end_159_12:
    assert_159_5:
        if_20_29_159_5:
        cmp_20_29_159_5:
        test r15, r15
        jne if_20_26_159_5_end
        jmp if_20_29_159_5_code
        if_20_29_159_5_code:
            mov rdi, 1
            exit_20_38_159_5:
                    mov rax, 60
                syscall
            exit_20_38_159_5_end:
        if_20_26_159_5_end:
    assert_159_5_end:
    mov rcx, 8
    lea r15, [rsp - 796]
    mov r14, 1
    mov r13, 162
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 162
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 796]
    mov r14, 0
    mov r13, 163
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 163
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_167_12:
        lea r13, [rsp - 796]
        mov r12, 0
        mov r11, 167
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        mov r11, 167
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8]
    cmp r14, 0xffee
    jne bool_false_167_12
    jmp bool_true_167_12
    bool_true_167_12:
    mov r15, true
    jmp bool_end_167_12
    bool_false_167_12:
    mov r15, false
    bool_end_167_12:
    assert_167_5:
        if_20_29_167_5:
        cmp_20_29_167_5:
        test r15, r15
        jne if_20_26_167_5_end
        jmp if_20_29_167_5_code
        if_20_29_167_5_code:
            mov rdi, 1
            exit_20_38_167_5:
                    mov rax, 60
                syscall
            exit_20_38_167_5_end:
        if_20_26_167_5_end:
    assert_167_5_end:
    cmp_168_12:
        mov rcx, 8
        lea r13, [rsp - 796]
        mov r12, 0
        mov r11, 169
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 169
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 796]
        mov r12, 1
        mov r11, 170
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 170
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        je cmps_eq_168_12
        mov r14, false
        jmp cmps_end_168_12
        cmps_eq_168_12:
        mov r14, true
        cmps_end_168_12:
    test r14, r14
    je bool_false_168_12
    jmp bool_true_168_12
    bool_true_168_12:
    mov r15, true
    jmp bool_end_168_12
    bool_false_168_12:
    mov r15, false
    bool_end_168_12:
    assert_168_5:
        if_20_29_168_5:
        cmp_20_29_168_5:
        test r15, r15
        jne if_20_26_168_5_end
        jmp if_20_29_168_5_code
        if_20_29_168_5_code:
            mov rdi, 1
            exit_20_38_168_5:
                    mov rax, 60
                syscall
            exit_20_38_168_5_end:
        if_20_26_168_5_end:
    assert_168_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_174_5:
            mov rax, 1
            mov rdi, 1
        syscall
    print_174_5_end:
    loop_175_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_176_9:
                mov rax, 1
                mov rdi, 1
            syscall
        print_176_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_178_19:
                mov rax, 0
                mov rdi, 0
            syscall
                mov qword [rsp - 804], rax
        read_178_19_end:
        sub qword [rsp - 804], 1
        if_182_12:
        cmp_182_12:
        cmp qword [rsp - 804], 0
        jne if_184_19
        jmp if_182_12_code
        if_182_12_code:
            jmp loop_175_5_end
        jmp if_182_9_end
        if_184_19:
        cmp_184_19:
        cmp qword [rsp - 804], 4
        jg if_else_182_9
        jmp if_184_19_code
        if_184_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_185_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_185_13_end:
            jmp loop_175_5
        jmp if_182_9_end
        if_else_182_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_188_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_188_13_end:
            mov rdx, qword [rsp - 804]
            lea rsi, [rsp - 284]
            print_189_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_189_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_190_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_190_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_191_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_191_13_end:
        if_182_9_end:
    jmp loop_175_5
    loop_175_5_end:
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
