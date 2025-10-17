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
    mov r14, 85
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 4
    mov r14, 85
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    cmp r15, 0
    mov r14, 86
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 4
    mov r14, 86
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    cmp r14, 0
    mov r13, 86
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 4
    mov r13, 86
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_87_12:
        mov r13, 1
        cmp r13, 0
        mov r12, 87
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 87
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
        if_19_8_87_5:
        cmp_19_8_87_5:
        test r15, r15
        jne if_19_5_87_5_end
        jmp if_19_8_87_5_code
        if_19_8_87_5_code:
            mov rdi, 1
            exit_19_17_87_5:
                    mov rax, 60
                syscall
            exit_19_17_87_5_end:
        if_19_5_87_5_end:
    assert_87_5_end:
    cmp_88_12:
        mov r13, 2
        cmp r13, 0
        mov r12, 88
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 88
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
        if_19_8_88_5:
        cmp_19_8_88_5:
        test r15, r15
        jne if_19_5_88_5_end
        jmp if_19_8_88_5_code
        if_19_8_88_5_code:
            mov rdi, 1
            exit_19_17_88_5:
                    mov rax, 60
                syscall
            exit_19_17_88_5_end:
        if_19_5_88_5_end:
    assert_88_5_end:
    mov rcx, 2
    mov r15, 2
    cmp r15, 0
    mov r14, 90
    cmovl rbp, r14
    jl panic_bounds
    mov r14, rcx
    add r14, r15
    cmp r14, 4
    mov r13, 90
    cmovg rbp, r13
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    cmp rcx, 0
    mov r15, 90
    cmovl rbp, r15
    jl panic_bounds
    cmp rcx, 4
    mov r15, 90
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_92_12:
        mov r13, 0
        cmp r13, 0
        mov r12, 92
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 92
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
        if_19_8_92_5:
        cmp_19_8_92_5:
        test r15, r15
        jne if_19_5_92_5_end
        jmp if_19_8_92_5_code
        if_19_8_92_5_code:
            mov rdi, 1
            exit_19_17_92_5:
                    mov rax, 60
                syscall
            exit_19_17_92_5_end:
        if_19_5_92_5_end:
    assert_92_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    cmp rcx, 0
    mov r15, 95
    cmovl rbp, r15
    jl panic_bounds
    cmp rcx, 4
    mov r15, 95
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    cmp rcx, 0
    mov r15, 95
    cmovl rbp, r15
    jl panic_bounds
    cmp rcx, 8
    mov r15, 95
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_96_12:
        mov rcx, 4
        cmp rcx, 0
        mov r13, 96
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 4
        mov r13, 96
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 0
        mov r13, 96
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 8
        mov r13, 96
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
        if_19_8_96_5:
        cmp_19_8_96_5:
        test r15, r15
        jne if_19_5_96_5_end
        jmp if_19_8_96_5_code
        if_19_8_96_5_code:
            mov rdi, 1
            exit_19_17_96_5:
                    mov rax, 60
                syscall
            exit_19_17_96_5_end:
        if_19_5_96_5_end:
    assert_96_5_end:
    mov r15, 2
    cmp r15, 0
    mov r14, 98
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 8
    mov r14, 98
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_99_12:
        mov rcx, 4
        cmp rcx, 0
        mov r13, 99
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 4
        mov r13, 99
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 0
        mov r13, 99
        cmovl rbp, r13
        jl panic_bounds
        cmp rcx, 8
        mov r13, 99
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_99_16
        mov r14, false
        jmp cmps_end_99_16
        cmps_eq_99_16:
        mov r14, true
        cmps_end_99_16:
    test r14, r14
    jne bool_false_99_12
    jmp bool_true_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
    assert_99_5:
        if_19_8_99_5:
        cmp_19_8_99_5:
        test r15, r15
        jne if_19_5_99_5_end
        jmp if_19_8_99_5_code
        if_19_8_99_5_code:
            mov rdi, 1
            exit_19_17_99_5:
                    mov rax, 60
                syscall
            exit_19_17_99_5_end:
        if_19_5_99_5_end:
    assert_99_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    cmp r15, 0
    mov r14, 102
    cmovl rbp, r14
    jl panic_bounds
    cmp r15, 4
    mov r14, 102
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    cmp r14, 0
    mov r13, 102
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 4
    mov r13, 102
    cmovge rbp, r13
    jge panic_bounds
    inv_102_16:
        lea r13, [rsp + r14 * 4 - 16]
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
        not dword [rsp + r15 * 4 - 16]
    inv_102_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_103_12:
        mov r13, qword [rsp - 24]
        cmp r13, 0
        mov r12, 103
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 103
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_103_12
    jmp bool_true_103_12
    bool_true_103_12:
    mov r15, true
    jmp bool_end_103_12
    bool_false_103_12:
    mov r15, false
    bool_end_103_12:
    assert_103_5:
        if_19_8_103_5:
        cmp_19_8_103_5:
        test r15, r15
        jne if_19_5_103_5_end
        jmp if_19_8_103_5_code
        if_19_8_103_5_code:
            mov rdi, 1
            exit_19_17_103_5:
                    mov rax, 60
                syscall
            exit_19_17_103_5_end:
        if_19_5_103_5_end:
    assert_103_5_end:
    faz_105_5:
        mov r15, 1
        cmp r15, 0
        mov r14, 76
        cmovl rbp, r14
        jl panic_bounds
        cmp r15, 4
        mov r14, 76
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_105_5_end:
    cmp_106_12:
        mov r13, 1
        cmp r13, 0
        mov r12, 106
        cmovl rbp, r12
        jl panic_bounds
        cmp r13, 4
        mov r12, 106
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 0xfe
    jne bool_false_106_12
    jmp bool_true_106_12
    bool_true_106_12:
    mov r15, true
    jmp bool_end_106_12
    bool_false_106_12:
    mov r15, false
    bool_end_106_12:
    assert_106_5:
        if_19_8_106_5:
        cmp_19_8_106_5:
        test r15, r15
        jne if_19_5_106_5_end
        jmp if_19_8_106_5_code
        if_19_8_106_5_code:
            mov rdi, 1
            exit_19_17_106_5:
                    mov rax, 60
                syscall
            exit_19_17_106_5_end:
        if_19_5_106_5_end:
    assert_106_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_109_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_109_5_end:
    cmp_110_12:
    cmp qword [rsp - 72], 2
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_19_8_110_5:
        cmp_19_8_110_5:
        test r15, r15
        jne if_19_5_110_5_end
        jmp if_19_8_110_5_code
        if_19_8_110_5_code:
            mov rdi, 1
            exit_19_17_110_5:
                    mov rax, 60
                syscall
            exit_19_17_110_5_end:
        if_19_5_110_5_end:
    assert_110_5_end:
    cmp_111_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_111_12
    jmp bool_true_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_19_8_111_5:
        cmp_19_8_111_5:
        test r15, r15
        jne if_19_5_111_5_end
        jmp if_19_8_111_5_code
        if_19_8_111_5_code:
            mov rdi, 1
            exit_19_17_111_5:
                    mov rax, 60
                syscall
            exit_19_17_111_5_end:
        if_19_5_111_5_end:
    assert_111_5_end:
    mov qword [rsp - 80], 0
    bar_114_5:
        if_59_8_114_5:
        cmp_59_8_114_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_114_5_end
        jmp if_59_8_114_5_code
        if_59_8_114_5_code:
            jmp bar_114_5_end
        if_59_5_114_5_end:
        mov qword [rsp - 80], 0xff
    bar_114_5_end:
    cmp_115_12:
    cmp qword [rsp - 80], 0
    jne bool_false_115_12
    jmp bool_true_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_19_8_115_5:
        cmp_19_8_115_5:
        test r15, r15
        jne if_19_5_115_5_end
        jmp if_19_8_115_5_code
        if_19_8_115_5_code:
            mov rdi, 1
            exit_19_17_115_5:
                    mov rax, 60
                syscall
            exit_19_17_115_5_end:
        if_19_5_115_5_end:
    assert_115_5_end:
    mov qword [rsp - 80], 1
    bar_118_5:
        if_59_8_118_5:
        cmp_59_8_118_5:
        cmp qword [rsp - 80], 0
        jne if_59_5_118_5_end
        jmp if_59_8_118_5_code
        if_59_8_118_5_code:
            jmp bar_118_5_end
        if_59_5_118_5_end:
        mov qword [rsp - 80], 0xff
    bar_118_5_end:
    cmp_119_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_119_12
    jmp bool_true_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
    assert_119_5:
        if_19_8_119_5:
        cmp_19_8_119_5:
        test r15, r15
        jne if_19_5_119_5_end
        jmp if_19_8_119_5_code
        if_19_8_119_5_code:
            mov rdi, 1
            exit_19_17_119_5:
                    mov rax, 60
                syscall
            exit_19_17_119_5_end:
        if_19_5_119_5_end:
    assert_119_5_end:
    mov qword [rsp - 88], 1
    baz_122_13:
        mov r15, qword [rsp - 88]
        imul r15, 2
    baz_122_13_end:
    mov qword [rsp - 96], r15
    cmp_123_12:
    cmp qword [rsp - 96], 2
    jne bool_false_123_12
    jmp bool_true_123_12
    bool_true_123_12:
    mov r15, true
    jmp bool_end_123_12
    bool_false_123_12:
    mov r15, false
    bool_end_123_12:
    assert_123_5:
        if_19_8_123_5:
        cmp_19_8_123_5:
        test r15, r15
        jne if_19_5_123_5_end
        jmp if_19_8_123_5_code
        if_19_8_123_5_code:
            mov rdi, 1
            exit_19_17_123_5:
                    mov rax, 60
                syscall
            exit_19_17_123_5_end:
        if_19_5_123_5_end:
    assert_123_5_end:
    baz_125_9:
        mov r15, 1
        imul r15, 2
    baz_125_9_end:
    mov qword [rsp - 96], r15
    cmp_126_12:
    cmp qword [rsp - 96], 2
    jne bool_false_126_12
    jmp bool_true_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_19_8_126_5:
        cmp_19_8_126_5:
        test r15, r15
        jne if_19_5_126_5_end
        jmp if_19_8_126_5_code
        if_19_8_126_5_code:
            mov rdi, 1
            exit_19_17_126_5:
                    mov rax, 60
                syscall
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
    assert_126_5_end:
    baz_128_23:
        mov r15, 2
        imul r15, 2
    baz_128_23_end:
    mov qword [rsp - 112], r15
    mov qword [rsp - 104], 0
    cmp_129_12:
    cmp qword [rsp - 112], 4
    jne bool_false_129_12
    jmp bool_true_129_12
    bool_true_129_12:
    mov r15, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15, false
    bool_end_129_12:
    assert_129_5:
        if_19_8_129_5:
        cmp_19_8_129_5:
        test r15, r15
        jne if_19_5_129_5_end
        jmp if_19_8_129_5_code
        if_19_8_129_5_code:
            mov rdi, 1
            exit_19_17_129_5:
                    mov rax, 60
                syscall
            exit_19_17_129_5_end:
        if_19_5_129_5_end:
    assert_129_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_135_12:
    cmp qword [rsp - 148], 10
    jne bool_false_135_12
    jmp bool_true_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
    assert_135_5:
        if_19_8_135_5:
        cmp_19_8_135_5:
        test r15, r15
        jne if_19_5_135_5_end
        jmp if_19_8_135_5_code
        if_19_8_135_5_code:
            mov rdi, 1
            exit_19_17_135_5:
                    mov rax, 60
                syscall
            exit_19_17_135_5_end:
        if_19_5_135_5_end:
    assert_135_5_end:
    cmp_136_12:
    cmp qword [rsp - 140], 2
    jne bool_false_136_12
    jmp bool_true_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_19_8_136_5:
        cmp_19_8_136_5:
        test r15, r15
        jne if_19_5_136_5_end
        jmp if_19_8_136_5_code
        if_19_8_136_5_code:
            mov rdi, 1
            exit_19_17_136_5:
                    mov rax, 60
                syscall
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
    assert_136_5_end:
    cmp_137_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_137_12
    jmp bool_true_137_12
    bool_true_137_12:
    mov r15, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15, false
    bool_end_137_12:
    assert_137_5:
        if_19_8_137_5:
        cmp_19_8_137_5:
        test r15, r15
        jne if_19_5_137_5_end
        jmp if_19_8_137_5_code
        if_19_8_137_5_code:
            mov rdi, 1
            exit_19_17_137_5:
                    mov rax, 60
                syscall
            exit_19_17_137_5_end:
        if_19_5_137_5_end:
    assert_137_5_end:
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
    cmp_141_12:
    cmp qword [rsp - 148], -1
    jne bool_false_141_12
    jmp bool_true_141_12
    bool_true_141_12:
    mov r15, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15, false
    bool_end_141_12:
    assert_141_5:
        if_19_8_141_5:
        cmp_19_8_141_5:
        test r15, r15
        jne if_19_5_141_5_end
        jmp if_19_8_141_5_code
        if_19_8_141_5_code:
            mov rdi, 1
            exit_19_17_141_5:
                    mov rax, 60
                syscall
            exit_19_17_141_5_end:
        if_19_5_141_5_end:
    assert_141_5_end:
    cmp_142_12:
    cmp qword [rsp - 140], -2
    jne bool_false_142_12
    jmp bool_true_142_12
    bool_true_142_12:
    mov r15, true
    jmp bool_end_142_12
    bool_false_142_12:
    mov r15, false
    bool_end_142_12:
    assert_142_5:
        if_19_8_142_5:
        cmp_19_8_142_5:
        test r15, r15
        jne if_19_5_142_5_end
        jmp if_19_8_142_5_code
        if_19_8_142_5_code:
            mov rdi, 1
            exit_19_17_142_5:
                    mov rax, 60
                syscall
            exit_19_17_142_5_end:
        if_19_5_142_5_end:
    assert_142_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_145_12:
    cmp qword [rsp - 184], -1
    jne bool_false_145_12
    jmp bool_true_145_12
    bool_true_145_12:
    mov r15, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15, false
    bool_end_145_12:
    assert_145_5:
        if_19_8_145_5:
        cmp_19_8_145_5:
        test r15, r15
        jne if_19_5_145_5_end
        jmp if_19_8_145_5_code
        if_19_8_145_5_code:
            mov rdi, 1
            exit_19_17_145_5:
                    mov rax, 60
                syscall
            exit_19_17_145_5_end:
        if_19_5_145_5_end:
    assert_145_5_end:
    cmp_146_12:
    cmp qword [rsp - 176], -2
    jne bool_false_146_12
    jmp bool_true_146_12
    bool_true_146_12:
    mov r15, true
    jmp bool_end_146_12
    bool_false_146_12:
    mov r15, false
    bool_end_146_12:
    assert_146_5:
        if_19_8_146_5:
        cmp_19_8_146_5:
        test r15, r15
        jne if_19_5_146_5_end
        jmp if_19_8_146_5_code
        if_19_8_146_5_code:
            mov rdi, 1
            exit_19_17_146_5:
                    mov rax, 60
                syscall
            exit_19_17_146_5_end:
        if_19_5_146_5_end:
    assert_146_5_end:
    cmp_147_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_147_12
    jmp bool_true_147_12
    bool_true_147_12:
    mov r15, true
    jmp bool_end_147_12
    bool_false_147_12:
    mov r15, false
    bool_end_147_12:
    assert_147_5:
        if_19_8_147_5:
        cmp_19_8_147_5:
        test r15, r15
        jne if_19_5_147_5_end
        jmp if_19_8_147_5_code
        if_19_8_147_5_code:
            mov rdi, 1
            exit_19_17_147_5:
                    mov rax, 60
                syscall
            exit_19_17_147_5_end:
        if_19_5_147_5_end:
    assert_147_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_152_12:
        lea r13, [rsp - 204]
        mov r12, 0
        cmp r12, 0
        mov r11, 152
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 1
        mov r11, 152
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        mov r14, qword [r13 + 8]
    cmp r14, 73
    jne bool_false_152_12
    jmp bool_true_152_12
    bool_true_152_12:
    mov r15, true
    jmp bool_end_152_12
    bool_false_152_12:
    mov r15, false
    bool_end_152_12:
    assert_152_5:
        if_19_8_152_5:
        cmp_19_8_152_5:
        test r15, r15
        jne if_19_5_152_5_end
        jmp if_19_8_152_5_code
        if_19_8_152_5_code:
            mov rdi, 1
            exit_19_17_152_5:
                    mov rax, 60
                syscall
            exit_19_17_152_5_end:
        if_19_5_152_5_end:
    assert_152_5_end:
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
    mov r13, 157
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 157
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    cmp r14, 0
    mov r13, 157
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 157
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8 + 0], 0xffee
    cmp_158_12:
        lea r13, [rsp - 796]
        mov r12, 1
        cmp r12, 0
        mov r11, 158
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 158
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        cmp r12, 0
        mov r11, 158
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 158
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_158_12
    jmp bool_true_158_12
    bool_true_158_12:
    mov r15, true
    jmp bool_end_158_12
    bool_false_158_12:
    mov r15, false
    bool_end_158_12:
    assert_158_5:
        if_19_8_158_5:
        cmp_19_8_158_5:
        test r15, r15
        jne if_19_5_158_5_end
        jmp if_19_8_158_5_code
        if_19_8_158_5_code:
            mov rdi, 1
            exit_19_17_158_5:
                    mov rax, 60
                syscall
            exit_19_17_158_5_end:
        if_19_5_158_5_end:
    assert_158_5_end:
    mov rcx, 8
    lea r15, [rsp - 796]
    mov r14, 1
    cmp r14, 0
    mov r13, 161
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 161
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 0
    mov r14, 161
    cmovl rbp, r14
    jl panic_bounds
    cmp rcx, 8
    mov r14, 161
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 796]
    mov r14, 0
    cmp r14, 0
    mov r13, 162
    cmovl rbp, r13
    jl panic_bounds
    cmp r14, 8
    mov r13, 162
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    cmp rcx, 0
    mov r14, 162
    cmovl rbp, r14
    jl panic_bounds
    cmp rcx, 8
    mov r14, 162
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_165_12:
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
        mov r12, 1
        cmp r12, 0
        mov r11, 165
        cmovl rbp, r11
        jl panic_bounds
        cmp r12, 8
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8 + 0]
    cmp r14, 0xffee
    jne bool_false_165_12
    jmp bool_true_165_12
    bool_true_165_12:
    mov r15, true
    jmp bool_end_165_12
    bool_false_165_12:
    mov r15, false
    bool_end_165_12:
    assert_165_5:
        if_19_8_165_5:
        cmp_19_8_165_5:
        test r15, r15
        jne if_19_5_165_5_end
        jmp if_19_8_165_5_code
        if_19_8_165_5_code:
            mov rdi, 1
            exit_19_17_165_5:
                    mov rax, 60
                syscall
            exit_19_17_165_5_end:
        if_19_5_165_5_end:
    assert_165_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_167_5:
            mov rax, 1
            mov rdi, 1
        syscall
    print_167_5_end:
    loop_168_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_169_9:
                mov rax, 1
                mov rdi, 1
            syscall
        print_169_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_171_19:
                mov rax, 0
                mov rdi, 0
            syscall
                mov qword [rsp - 804], rax
        read_171_19_end:
        sub qword [rsp - 804], 1
        if_175_12:
        cmp_175_12:
        cmp qword [rsp - 804], 0
        jne if_177_19
        jmp if_175_12_code
        if_175_12_code:
            jmp loop_168_5_end
        jmp if_175_9_end
        if_177_19:
        cmp_177_19:
        cmp qword [rsp - 804], 4
        jg if_else_175_9
        jmp if_177_19_code
        if_177_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_178_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_178_13_end:
            jmp loop_168_5
        jmp if_175_9_end
        if_else_175_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_181_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_181_13_end:
            mov rdx, qword [rsp - 804]
            lea rsi, [rsp - 284]
            print_182_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_182_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_183_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_183_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_184_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_184_13_end:
        if_175_9_end:
    jmp loop_168_5
    loop_168_5_end:
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
