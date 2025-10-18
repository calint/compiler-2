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
    cmp r15, 0
    mov r14, 83
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 4
    mov r14, 83
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    cmp r15, 0
    mov r14, 84
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 4
    mov r14, 84
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    cmp r14, 0
    mov r13, 84
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 4
    mov r13, 84
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_85_12:
        mov r13, 1
        cmp r13, 0
        mov r12, 85
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 85
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_85_12
    jmp bool_true_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
    assert_85_5:
        if_18_29_85_5:
        cmp_18_29_85_5:
        test r15, r15
        jne if_18_26_85_5_end
        jmp if_18_29_85_5_code
        if_18_29_85_5_code:
            mov rdi, 1
            exit_18_38_85_5:
                    mov rax, 60
                syscall
            exit_18_38_85_5_end:
        if_18_26_85_5_end:
    assert_85_5_end:
    cmp_86_12:
        mov r13, 2
        cmp r13, 0
        mov r12, 86
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 86
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_86_12
    jmp bool_true_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
    assert_86_5:
        if_18_29_86_5:
        cmp_18_29_86_5:
        test r15, r15
        jne if_18_26_86_5_end
        jmp if_18_29_86_5_code
        if_18_29_86_5_code:
            mov rdi, 1
            exit_18_38_86_5:
                    mov rax, 60
                syscall
            exit_18_38_86_5_end:
        if_18_26_86_5_end:
    assert_86_5_end:
    mov rcx, 2
    mov r15, 2
    cmp r15, 0
    mov r14, 88
    cmovl rbp, r14
    jl panic_bounds
    mov r14, rcx
    add r14, r15
    cmp r14, 4
    mov r13, 88
    cmovg rbp, r13
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    cmp rcx, 0
    mov r15, 88
    cmovl rbp, r15
    jl panic_bounds
    cmp rcx, 4
    mov r15, 88
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_90_12:
        mov r13, 0
        cmp r13, 0
        mov r12, 90
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 90
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_90_12
    jmp bool_true_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
    assert_90_5:
        if_18_29_90_5:
        cmp_18_29_90_5:
        test r15, r15
        jne if_18_26_90_5_end
        jmp if_18_29_90_5_code
        if_18_29_90_5_code:
            mov rdi, 1
            exit_18_38_90_5:
                    mov rax, 60
                syscall
            exit_18_38_90_5_end:
        if_18_26_90_5_end:
    assert_90_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    cmp rcx, 0
    mov r15, 93
    cmovl rbp, r15
    jl panic_bounds
    cmp rcx, 4
    mov r15, 93
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    cmp rcx, 0
    mov r15, 93
    cmovl rbp, r15
    jl panic_bounds
    cmp rcx, 8
    mov r15, 93
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_94_12:
        mov rcx, 4
        cmp rcx, 0
        mov r13, 94
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 4
        mov r13, 94
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 0
        mov r13, 94
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 8
        mov r13, 94
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_94_12
        mov r14, false
        jmp cmps_end_94_12
        cmps_eq_94_12:
        mov r14, true
        cmps_end_94_12:
    test r14, r14
    je bool_false_94_12
    jmp bool_true_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
    assert_94_5:
        if_18_29_94_5:
        cmp_18_29_94_5:
        test r15, r15
        jne if_18_26_94_5_end
        jmp if_18_29_94_5_code
        if_18_29_94_5_code:
            mov rdi, 1
            exit_18_38_94_5:
                    mov rax, 60
                syscall
            exit_18_38_94_5_end:
        if_18_26_94_5_end:
    assert_94_5_end:
    mov r15, 2
    cmp r15, 0
    mov r14, 96
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 8
    mov r14, 96
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_97_12:
        mov rcx, 4
        cmp rcx, 0
        mov r13, 97
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 4
        mov r13, 97
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 0
        mov r13, 97
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 8
        mov r13, 97
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_97_16
        mov r14, false
        jmp cmps_end_97_16
        cmps_eq_97_16:
        mov r14, true
        cmps_end_97_16:
    test r14, r14
    jne bool_false_97_12
    jmp bool_true_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
    assert_97_5:
        if_18_29_97_5:
        cmp_18_29_97_5:
        test r15, r15
        jne if_18_26_97_5_end
        jmp if_18_29_97_5_code
        if_18_29_97_5_code:
            mov rdi, 1
            exit_18_38_97_5:
                    mov rax, 60
                syscall
            exit_18_38_97_5_end:
        if_18_26_97_5_end:
    assert_97_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    cmp r15, 0
    mov r14, 100
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 4
    mov r14, 100
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    cmp r14, 0
    mov r13, 100
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 4
    mov r13, 100
    cmovge rbp, r13
    jge panic_bounds
    inv_100_16:
        lea r13, [rsp + r14 * 4 - 16]
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
        not dword [rsp + r15 * 4 - 16]
    inv_100_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_101_12:
        mov r13, qword [rsp - 24]
        cmp r13, 0
        mov r12, 101
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 101
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_101_12
    jmp bool_true_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
    assert_101_5:
        if_18_29_101_5:
        cmp_18_29_101_5:
        test r15, r15
        jne if_18_26_101_5_end
        jmp if_18_29_101_5_code
        if_18_29_101_5_code:
            mov rdi, 1
            exit_18_38_101_5:
                    mov rax, 60
                syscall
            exit_18_38_101_5_end:
        if_18_26_101_5_end:
    assert_101_5_end:
    faz_103_5:
        mov r15, 1
        cmp r15, 0
        mov r14, 74
        cmovl rbp, r14
        jl panic_bounds
        cmp r15, 4
        mov r14, 74
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_103_5_end:
    cmp_104_12:
        mov r13, 1
        cmp r13, 0
        mov r12, 104
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 104
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 0xfe
    jne bool_false_104_12
    jmp bool_true_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_18_29_104_5:
        cmp_18_29_104_5:
        test r15, r15
        jne if_18_26_104_5_end
        jmp if_18_29_104_5_code
        if_18_29_104_5_code:
            mov rdi, 1
            exit_18_38_104_5:
                    mov rax, 60
                syscall
            exit_18_38_104_5_end:
        if_18_26_104_5_end:
    assert_104_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_107_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_107_5_end:
    cmp_108_12:
    cmp qword [rsp - 72], 2
    jne bool_false_108_12
    jmp bool_true_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
    assert_108_5:
        if_18_29_108_5:
        cmp_18_29_108_5:
        test r15, r15
        jne if_18_26_108_5_end
        jmp if_18_29_108_5_code
        if_18_29_108_5_code:
            mov rdi, 1
            exit_18_38_108_5:
                    mov rax, 60
                syscall
            exit_18_38_108_5_end:
        if_18_26_108_5_end:
    assert_108_5_end:
    cmp_109_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_109_12
    jmp bool_true_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
    assert_109_5:
        if_18_29_109_5:
        cmp_18_29_109_5:
        test r15, r15
        jne if_18_26_109_5_end
        jmp if_18_29_109_5_code
        if_18_29_109_5_code:
            mov rdi, 1
            exit_18_38_109_5:
                    mov rax, 60
                syscall
            exit_18_38_109_5_end:
        if_18_26_109_5_end:
    assert_109_5_end:
    mov qword [rsp - 80], 0
    bar_112_5:
        if_57_8_112_5:
        cmp_57_8_112_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_112_5_end
        jmp if_57_8_112_5_code
        if_57_8_112_5_code:
            jmp bar_112_5_end
        if_57_5_112_5_end:
        mov qword [rsp - 80], 0xff
    bar_112_5_end:
    cmp_113_12:
    cmp qword [rsp - 80], 0
    jne bool_false_113_12
    jmp bool_true_113_12
    bool_true_113_12:
    mov r15, true
    jmp bool_end_113_12
    bool_false_113_12:
    mov r15, false
    bool_end_113_12:
    assert_113_5:
        if_18_29_113_5:
        cmp_18_29_113_5:
        test r15, r15
        jne if_18_26_113_5_end
        jmp if_18_29_113_5_code
        if_18_29_113_5_code:
            mov rdi, 1
            exit_18_38_113_5:
                    mov rax, 60
                syscall
            exit_18_38_113_5_end:
        if_18_26_113_5_end:
    assert_113_5_end:
    mov qword [rsp - 80], 1
    bar_116_5:
        if_57_8_116_5:
        cmp_57_8_116_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_116_5_end
        jmp if_57_8_116_5_code
        if_57_8_116_5_code:
            jmp bar_116_5_end
        if_57_5_116_5_end:
        mov qword [rsp - 80], 0xff
    bar_116_5_end:
    cmp_117_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_117_12
    jmp bool_true_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
    assert_117_5:
        if_18_29_117_5:
        cmp_18_29_117_5:
        test r15, r15
        jne if_18_26_117_5_end
        jmp if_18_29_117_5_code
        if_18_29_117_5_code:
            mov rdi, 1
            exit_18_38_117_5:
                    mov rax, 60
                syscall
            exit_18_38_117_5_end:
        if_18_26_117_5_end:
    assert_117_5_end:
    mov qword [rsp - 88], 1
    baz_120_13:
        mov r15, qword [rsp - 88]
        imul r15, 2
    baz_120_13_end:
    mov qword [rsp - 96], r15
    cmp_121_12:
    cmp qword [rsp - 96], 2
    jne bool_false_121_12
    jmp bool_true_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_18_29_121_5:
        cmp_18_29_121_5:
        test r15, r15
        jne if_18_26_121_5_end
        jmp if_18_29_121_5_code
        if_18_29_121_5_code:
            mov rdi, 1
            exit_18_38_121_5:
                    mov rax, 60
                syscall
            exit_18_38_121_5_end:
        if_18_26_121_5_end:
    assert_121_5_end:
    baz_123_9:
        mov r15, 1
        imul r15, 2
    baz_123_9_end:
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
        if_18_29_124_5:
        cmp_18_29_124_5:
        test r15, r15
        jne if_18_26_124_5_end
        jmp if_18_29_124_5_code
        if_18_29_124_5_code:
            mov rdi, 1
            exit_18_38_124_5:
                    mov rax, 60
                syscall
            exit_18_38_124_5_end:
        if_18_26_124_5_end:
    assert_124_5_end:
    baz_126_23:
        mov r15, 2
        imul r15, 2
    baz_126_23_end:
    mov qword [rsp - 112], r15
    mov qword [rsp - 104], 0
    cmp_127_12:
    cmp qword [rsp - 112], 4
    jne bool_false_127_12
    jmp bool_true_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
    assert_127_5:
        if_18_29_127_5:
        cmp_18_29_127_5:
        test r15, r15
        jne if_18_26_127_5_end
        jmp if_18_29_127_5_code
        if_18_29_127_5_code:
            mov rdi, 1
            exit_18_38_127_5:
                    mov rax, 60
                syscall
            exit_18_38_127_5_end:
        if_18_26_127_5_end:
    assert_127_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_133_12:
    cmp qword [rsp - 148], 10
    jne bool_false_133_12
    jmp bool_true_133_12
    bool_true_133_12:
    mov r15, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15, false
    bool_end_133_12:
    assert_133_5:
        if_18_29_133_5:
        cmp_18_29_133_5:
        test r15, r15
        jne if_18_26_133_5_end
        jmp if_18_29_133_5_code
        if_18_29_133_5_code:
            mov rdi, 1
            exit_18_38_133_5:
                    mov rax, 60
                syscall
            exit_18_38_133_5_end:
        if_18_26_133_5_end:
    assert_133_5_end:
    cmp_134_12:
    cmp qword [rsp - 140], 2
    jne bool_false_134_12
    jmp bool_true_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
    assert_134_5:
        if_18_29_134_5:
        cmp_18_29_134_5:
        test r15, r15
        jne if_18_26_134_5_end
        jmp if_18_29_134_5_code
        if_18_29_134_5_code:
            mov rdi, 1
            exit_18_38_134_5:
                    mov rax, 60
                syscall
            exit_18_38_134_5_end:
        if_18_26_134_5_end:
    assert_134_5_end:
    cmp_135_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_135_12
    jmp bool_true_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
    assert_135_5:
        if_18_29_135_5:
        cmp_18_29_135_5:
        test r15, r15
        jne if_18_26_135_5_end
        jmp if_18_29_135_5_code
        if_18_29_135_5_code:
            mov rdi, 1
            exit_18_38_135_5:
                    mov rax, 60
                syscall
            exit_18_38_135_5_end:
        if_18_26_135_5_end:
    assert_135_5_end:
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
    cmp_139_12:
    cmp qword [rsp - 148], -1
    jne bool_false_139_12
    jmp bool_true_139_12
    bool_true_139_12:
    mov r15, true
    jmp bool_end_139_12
    bool_false_139_12:
    mov r15, false
    bool_end_139_12:
    assert_139_5:
        if_18_29_139_5:
        cmp_18_29_139_5:
        test r15, r15
        jne if_18_26_139_5_end
        jmp if_18_29_139_5_code
        if_18_29_139_5_code:
            mov rdi, 1
            exit_18_38_139_5:
                    mov rax, 60
                syscall
            exit_18_38_139_5_end:
        if_18_26_139_5_end:
    assert_139_5_end:
    cmp_140_12:
    cmp qword [rsp - 140], -2
    jne bool_false_140_12
    jmp bool_true_140_12
    bool_true_140_12:
    mov r15, true
    jmp bool_end_140_12
    bool_false_140_12:
    mov r15, false
    bool_end_140_12:
    assert_140_5:
        if_18_29_140_5:
        cmp_18_29_140_5:
        test r15, r15
        jne if_18_26_140_5_end
        jmp if_18_29_140_5_code
        if_18_29_140_5_code:
            mov rdi, 1
            exit_18_38_140_5:
                    mov rax, 60
                syscall
            exit_18_38_140_5_end:
        if_18_26_140_5_end:
    assert_140_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_143_12:
    cmp qword [rsp - 184], -1
    jne bool_false_143_12
    jmp bool_true_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
    assert_143_5:
        if_18_29_143_5:
        cmp_18_29_143_5:
        test r15, r15
        jne if_18_26_143_5_end
        jmp if_18_29_143_5_code
        if_18_29_143_5_code:
            mov rdi, 1
            exit_18_38_143_5:
                    mov rax, 60
                syscall
            exit_18_38_143_5_end:
        if_18_26_143_5_end:
    assert_143_5_end:
    cmp_144_12:
    cmp qword [rsp - 176], -2
    jne bool_false_144_12
    jmp bool_true_144_12
    bool_true_144_12:
    mov r15, true
    jmp bool_end_144_12
    bool_false_144_12:
    mov r15, false
    bool_end_144_12:
    assert_144_5:
        if_18_29_144_5:
        cmp_18_29_144_5:
        test r15, r15
        jne if_18_26_144_5_end
        jmp if_18_29_144_5_code
        if_18_29_144_5_code:
            mov rdi, 1
            exit_18_38_144_5:
                    mov rax, 60
                syscall
            exit_18_38_144_5_end:
        if_18_26_144_5_end:
    assert_144_5_end:
    cmp_145_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_145_12
    jmp bool_true_145_12
    bool_true_145_12:
    mov r15, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15, false
    bool_end_145_12:
    assert_145_5:
        if_18_29_145_5:
        cmp_18_29_145_5:
        test r15, r15
        jne if_18_26_145_5_end
        jmp if_18_29_145_5_code
        if_18_29_145_5_code:
            mov rdi, 1
            exit_18_38_145_5:
                    mov rax, 60
                syscall
            exit_18_38_145_5_end:
        if_18_26_145_5_end:
    assert_145_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_150_12:
        lea r13, [rsp - 204]
        mov r12, 0
        cmp r12, 0
        mov r11, 150
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 1
        mov r11, 150
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        mov r14, qword [r13 + 8]
    cmp r14, 73
    jne bool_false_150_12
    jmp bool_true_150_12
    bool_true_150_12:
    mov r15, true
    jmp bool_end_150_12
    bool_false_150_12:
    mov r15, false
    bool_end_150_12:
    assert_150_5:
        if_18_29_150_5:
        cmp_18_29_150_5:
        test r15, r15
        jne if_18_26_150_5_end
        jmp if_18_29_150_5_code
        if_18_29_150_5_code:
            mov rdi, 1
            exit_18_38_150_5:
                    mov rax, 60
                syscall
            exit_18_38_150_5_end:
        if_18_26_150_5_end:
    assert_150_5_end:
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
    cmp r14, 0
    mov r13, 155
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 155
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    cmp r14, 0
    mov r13, 155
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 155
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8 + 0], 0xffee
    cmp_156_12:
        lea r13, [rsp - 796]
        mov r12, 1
        cmp r12, 0
        mov r11, 156
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 156
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 0
        mov r11, 156
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 156
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_156_12
    jmp bool_true_156_12
    bool_true_156_12:
    mov r15, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15, false
    bool_end_156_12:
    assert_156_5:
        if_18_29_156_5:
        cmp_18_29_156_5:
        test r15, r15
        jne if_18_26_156_5_end
        jmp if_18_29_156_5_code
        if_18_29_156_5_code:
            mov rdi, 1
            exit_18_38_156_5:
                    mov rax, 60
                syscall
            exit_18_38_156_5_end:
        if_18_26_156_5_end:
    assert_156_5_end:
    mov rcx, 8
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 0
    mov r13, 159
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 159
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 0
    mov r14, 159
    cmovl rbp, r14
    jl panic_bounds
    cmp rcx, 8
    mov r14, 159
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 796]
    mov r14, 0
    cmp r14, 0
    mov r13, 160
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 160
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 0
    mov r14, 160
    cmovl rbp, r14
    jl panic_bounds
    cmp rcx, 8
    mov r14, 160
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_163_12:
        lea r13, [rsp - 796]
        mov r12, 0
        cmp r12, 0
        mov r11, 163
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 163
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 0
        mov r11, 163
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 163
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_163_12
    jmp bool_true_163_12
    bool_true_163_12:
    mov r15, true
    jmp bool_end_163_12
    bool_false_163_12:
    mov r15, false
    bool_end_163_12:
    assert_163_5:
        if_18_29_163_5:
        cmp_18_29_163_5:
        test r15, r15
        jne if_18_26_163_5_end
        jmp if_18_29_163_5_code
        if_18_29_163_5_code:
            mov rdi, 1
            exit_18_38_163_5:
                    mov rax, 60
                syscall
            exit_18_38_163_5_end:
        if_18_26_163_5_end:
    assert_163_5_end:
    cmp_164_12:
        mov rcx, 8
        lea r13, [rsp - 796]
        mov r12, 0
        cmp r12, 0
        mov r11, 165
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        cmp rcx, 0
        mov r12, 165
        cmovl rbp, r12
        jl panic_bounds
        cmp rcx, 8
        mov r12, 165
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 796]
        mov r12, 1
        cmp r12, 0
        mov r11, 166
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 166
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        cmp rcx, 0
        mov r12, 166
        cmovl rbp, r12
        jl panic_bounds
        cmp rcx, 8
        mov r12, 166
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        je cmps_eq_164_12
        mov r14, false
        jmp cmps_end_164_12
        cmps_eq_164_12:
        mov r14, true
        cmps_end_164_12:
    test r14, r14
    je bool_false_164_12
    jmp bool_true_164_12
    bool_true_164_12:
    mov r15, true
    jmp bool_end_164_12
    bool_false_164_12:
    mov r15, false
    bool_end_164_12:
    assert_164_5:
        if_18_29_164_5:
        cmp_18_29_164_5:
        test r15, r15
        jne if_18_26_164_5_end
        jmp if_18_29_164_5_code
        if_18_29_164_5_code:
            mov rdi, 1
            exit_18_38_164_5:
                    mov rax, 60
                syscall
            exit_18_38_164_5_end:
        if_18_26_164_5_end:
    assert_164_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_170_5:
            mov rax, 1
            mov rdi, 1
        syscall
    print_170_5_end:
    loop_171_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_172_9:
                mov rax, 1
                mov rdi, 1
            syscall
        print_172_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_174_19:
                mov rax, 0
                mov rdi, 0
            syscall
                mov qword [rsp - 804], rax
        read_174_19_end:
        sub qword [rsp - 804], 1
        if_178_12:
        cmp_178_12:
        cmp qword [rsp - 804], 0
        jne if_180_19
        jmp if_178_12_code
        if_178_12_code:
            jmp loop_171_5_end
        jmp if_178_9_end
        if_180_19:
        cmp_180_19:
        cmp qword [rsp - 804], 4
        jg if_else_178_9
        jmp if_180_19_code
        if_180_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_181_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_181_13_end:
            jmp loop_171_5
        jmp if_178_9_end
        if_else_178_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_184_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_184_13_end:
            mov rdx, qword [rsp - 804]
            lea rsi, [rsp - 284]
            print_185_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_185_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_186_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_186_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_187_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_187_13_end:
        if_178_9_end:
    jmp loop_171_5
    loop_171_5_end:
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
