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
    double_94_27:
        mov r15, 5
        imul r15, 2
    double_94_27_end:
    triple_94_38:
        mov r14, 2
        imul r14, 3
    triple_94_38_end:
    add_94_23:
        mov qword [rsp - 16], r15
        add qword [rsp - 16], r14
    add_94_23_end:
    increment_94_59:
        mov r15, 3
        add r15, 1
    increment_94_59_end:
    decrement_94_73:
        mov r14, 5
        sub r14, 1
    decrement_94_73_end:
    multiply_94_50:
        mov r13, r15
        imul r13, r14
        mov qword [rsp - 8], r13
    multiply_94_50_end:
    cmp_95_12:
    cmp qword [rsp - 16], 16
    sete r15b
    bool_end_95_12:
    assert_95_5:
        if_15_8_95_5:
        cmp_15_8_95_5:
        cmp r15b, 0
        jne if_15_5_95_5_end
        if_15_8_95_5_code:
            mov rdi, 1
            exit_15_17_95_5:
                    mov rax, 60
                syscall
            exit_15_17_95_5_end:
        if_15_5_95_5_end:
    assert_95_5_end:
    cmp_96_12:
    cmp qword [rsp - 8], 16
    sete r15b
    bool_end_96_12:
    assert_96_5:
        if_15_8_96_5:
        cmp_15_8_96_5:
        cmp r15b, 0
        jne if_15_5_96_5_end
        if_15_8_96_5_code:
            mov rdi, 1
            exit_15_17_96_5:
                    mov rax, 60
                syscall
            exit_15_17_96_5_end:
        if_15_5_96_5_end:
    assert_96_5_end:
    double_101_38:
        mov r14, 5
        imul r14, 2
    double_101_38_end:
    add_101_30:
        mov r15, 10
        add r15, r14
    add_101_30_end:
    negate_101_23:
        mov qword [rsp - 32], r15
        neg qword [rsp - 32]
    negate_101_23_end:
    square_101_60:
        mov r15, 5
        imul r15, 5
    square_101_60_end:
    triple_101_71:
        mov r14, 3
        imul r14, 3
    triple_101_71_end:
    subtract_101_51:
        mov qword [rsp - 24], r15
        sub qword [rsp - 24], r14
    subtract_101_51_end:
    cmp_102_12:
    cmp qword [rsp - 32], -20
    sete r15b
    bool_end_102_12:
    assert_102_5:
        if_15_8_102_5:
        cmp_15_8_102_5:
        cmp r15b, 0
        jne if_15_5_102_5_end
        if_15_8_102_5_code:
            mov rdi, 1
            exit_15_17_102_5:
                    mov rax, 60
                syscall
            exit_15_17_102_5_end:
        if_15_5_102_5_end:
    assert_102_5_end:
    cmp_103_12:
    cmp qword [rsp - 24], 16
    sete r15b
    bool_end_103_12:
    assert_103_5:
        if_15_8_103_5:
        cmp_15_8_103_5:
        cmp r15b, 0
        jne if_15_5_103_5_end
        if_15_8_103_5_code:
            mov rdi, 1
            exit_15_17_103_5:
                    mov rax, 60
                syscall
            exit_15_17_103_5_end:
        if_15_5_103_5_end:
    assert_103_5_end:
    double_108_33:
        mov r14, 10
        imul r14, 2
    double_108_33_end:
    square_108_45:
        mov r13, 4
        imul r13, 4
    square_108_45_end:
    add_108_29:
        mov r15, r14
        add r15, r13
    add_108_29_end:
    halve_108_23:
        mov qword [rsp - 48], r15
        mov rax, qword [rsp - 48]
        cqo
        mov r14, 2
        idiv r14
        mov qword [rsp - 48], rax
    halve_108_23_end:
    halve_108_67:
        mov r15, 8
        mov rax, r15
        cqo
        mov r14, 2
        idiv r14
        mov r15, rax
    halve_108_67_end:
    double_108_87:
        mov r13, 2
        imul r13, 2
    double_108_87_end:
    increment_108_77:
        mov r14, r13
        add r14, 1
    increment_108_77_end:
    multiply_108_58:
        mov r13, r15
        imul r13, r14
        mov qword [rsp - 40], r13
    multiply_108_58_end:
    cmp_109_12:
    cmp qword [rsp - 48], 18
    sete r15b
    bool_end_109_12:
    assert_109_5:
        if_15_8_109_5:
        cmp_15_8_109_5:
        cmp r15b, 0
        jne if_15_5_109_5_end
        if_15_8_109_5_code:
            mov rdi, 1
            exit_15_17_109_5:
                    mov rax, 60
                syscall
            exit_15_17_109_5_end:
        if_15_5_109_5_end:
    assert_109_5_end:
    cmp_110_12:
    cmp qword [rsp - 40], 20
    sete r15b
    bool_end_110_12:
    assert_110_5:
        if_15_8_110_5:
        cmp_15_8_110_5:
        cmp r15b, 0
        jne if_15_5_110_5_end
        if_15_8_110_5_code:
            mov rdi, 1
            exit_15_17_110_5:
                    mov rax, 60
                syscall
            exit_15_17_110_5_end:
        if_15_5_110_5_end:
    assert_110_5_end:
    mov qword [rsp - 56], 3
    mov qword [rsp - 64], 4
    square_117_23:
        mov r15, qword [rsp - 56]
        imul r15, qword [rsp - 56]
        mov qword [rsp - 80], r15
    square_117_23_end:
    cube_117_35:
        mov r15, 2
        imul r15, 2
        imul r15, 2
    cube_117_35_end:
    add qword [rsp - 80], r15
    double_117_44:
        mov r15, qword [rsp - 64]
        imul r15, 2
    double_117_44_end:
    triple_117_57:
        mov r14, 2
        imul r14, 3
    triple_117_57_end:
    imul r15, r14
    sub r15, 10
    mov qword [rsp - 72], r15
    cmp_118_12:
    cmp qword [rsp - 80], 17
    sete r15b
    bool_end_118_12:
    assert_118_5:
        if_15_8_118_5:
        cmp_15_8_118_5:
        cmp r15b, 0
        jne if_15_5_118_5_end
        if_15_8_118_5_code:
            mov rdi, 1
            exit_15_17_118_5:
                    mov rax, 60
                syscall
            exit_15_17_118_5_end:
        if_15_5_118_5_end:
    assert_118_5_end:
    cmp_119_12:
    cmp qword [rsp - 72], 38
    sete r15b
    bool_end_119_12:
    assert_119_5:
        if_15_8_119_5:
        cmp_15_8_119_5:
        cmp r15b, 0
        jne if_15_5_119_5_end
        if_15_8_119_5_code:
            mov rdi, 1
            exit_15_17_119_5:
                    mov rax, 60
                syscall
            exit_15_17_119_5_end:
        if_15_5_119_5_end:
    assert_119_5_end:
    multiply_124_29:
        mov r15, 2
        imul r15, 3
    multiply_124_29_end:
    square_124_52:
        mov r13, 2
        imul r13, 2
    square_124_52_end:
    double_124_45:
        mov r14, r13
        imul r14, 2
    double_124_45_end:
    add_124_25:
        mov qword [rsp - 100], r15
        add qword [rsp - 100], r14
    add_124_25_end:
    triple_124_74:
        mov r15, 10
        imul r15, 3
    triple_124_74_end:
    halve_124_86:
        mov r14, 20
        mov rax, r14
        cqo
        mov r13, 2
        idiv r13
        mov r14, rax
    halve_124_86_end:
    subtract_124_65:
        mov qword [rsp - 92], r15
        sub qword [rsp - 92], r14
    subtract_124_65_end:
    compute_color_124_99:
        mov r15, 255
        imul r15, 65536
        mov qword [rsp - 84], r15
        mov r15, 0
        imul r15, 256
        add qword [rsp - 84], r15
        add qword [rsp - 84], 0
    compute_color_124_99_end:
    cmp_125_12:
    cmp qword [rsp - 100], 14
    sete r15b
    bool_end_125_12:
    assert_125_5:
        if_15_8_125_5:
        cmp_15_8_125_5:
        cmp r15b, 0
        jne if_15_5_125_5_end
        if_15_8_125_5_code:
            mov rdi, 1
            exit_15_17_125_5:
                    mov rax, 60
                syscall
            exit_15_17_125_5_end:
        if_15_5_125_5_end:
    assert_125_5_end:
    cmp_126_12:
    cmp qword [rsp - 92], 20
    sete r15b
    bool_end_126_12:
    assert_126_5:
        if_15_8_126_5:
        cmp_15_8_126_5:
        cmp r15b, 0
        jne if_15_5_126_5_end
        if_15_8_126_5_code:
            mov rdi, 1
            exit_15_17_126_5:
                    mov rax, 60
                syscall
            exit_15_17_126_5_end:
        if_15_5_126_5_end:
    assert_126_5_end:
    cmp_127_12:
    cmp dword [rsp - 84], 16711680
    sete r15b
    bool_end_127_12:
    assert_127_5:
        if_15_8_127_5:
        cmp_15_8_127_5:
        cmp r15b, 0
        jne if_15_5_127_5_end
        if_15_8_127_5_code:
            mov rdi, 1
            exit_15_17_127_5:
                    mov rax, 60
                syscall
            exit_15_17_127_5_end:
        if_15_5_127_5_end:
    assert_127_5_end:
    double_132_27:
        mov r15, 5
        imul r15, 2
    double_132_27_end:
    triple_132_38:
        mov r14, 3
        imul r14, 3
    triple_132_38_end:
    max_132_23:
        if_67_8_132_23:
        cmp_67_8_132_23:
        cmp r15, r14
        jle if_else_67_5_132_23
        if_67_8_132_23_code:
            mov qword [rsp - 116], r15
        jmp if_67_5_132_23_end
        if_else_67_5_132_23:
            mov qword [rsp - 116], r14
        if_67_5_132_23_end:
    max_132_23_end:
    square_132_54:
        mov r15, 4
        imul r15, 4
    square_132_54_end:
    cube_132_65:
        mov r14, 2
        imul r14, 2
        imul r14, 2
    cube_132_65_end:
    min_132_50:
        if_75_8_132_50:
        cmp_75_8_132_50:
        cmp r15, r14
        jge if_else_75_5_132_50
        if_75_8_132_50_code:
            mov qword [rsp - 108], r15
        jmp if_75_5_132_50_end
        if_else_75_5_132_50:
            mov qword [rsp - 108], r14
        if_75_5_132_50_end:
    min_132_50_end:
    cmp_133_12:
    cmp qword [rsp - 116], 10
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_15_8_133_5:
        cmp_15_8_133_5:
        cmp r15b, 0
        jne if_15_5_133_5_end
        if_15_8_133_5_code:
            mov rdi, 1
            exit_15_17_133_5:
                    mov rax, 60
                syscall
            exit_15_17_133_5_end:
        if_15_5_133_5_end:
    assert_133_5_end:
    cmp_134_12:
    cmp qword [rsp - 108], 8
    sete r15b
    bool_end_134_12:
    assert_134_5:
        if_15_8_134_5:
        cmp_15_8_134_5:
        cmp r15b, 0
        jne if_15_5_134_5_end
        if_15_8_134_5_code:
            mov rdi, 1
            exit_15_17_134_5:
                    mov rax, 60
                syscall
            exit_15_17_134_5_end:
        if_15_5_134_5_end:
    assert_134_5_end:
    add_139_34:
        mov r14, 5
        add r14, 10
    add_139_34_end:
    negate_139_27:
        mov r15, r14
        neg r15
    negate_139_27_end:
    abs_139_23:
        if_83_8_139_23:
        cmp_83_8_139_23:
        cmp r15, 0
        jge if_else_83_5_139_23
        if_83_8_139_23_code:
            mov qword [rsp - 132], r15
            neg qword [rsp - 132]
        jmp if_83_5_139_23_end
        if_else_83_5_139_23:
            mov qword [rsp - 132], r15
        if_83_5_139_23_end:
    abs_139_23_end:
    subtract_139_52:
        mov r15, 5
        sub r15, 20
    subtract_139_52_end:
    abs_139_48:
        if_83_8_139_48:
        cmp_83_8_139_48:
        cmp r15, 0
        jge if_else_83_5_139_48
        if_83_8_139_48_code:
            mov qword [rsp - 124], r15
            neg qword [rsp - 124]
        jmp if_83_5_139_48_end
        if_else_83_5_139_48:
            mov qword [rsp - 124], r15
        if_83_5_139_48_end:
    abs_139_48_end:
    cmp_140_12:
    cmp qword [rsp - 132], 15
    sete r15b
    bool_end_140_12:
    assert_140_5:
        if_15_8_140_5:
        cmp_15_8_140_5:
        cmp r15b, 0
        jne if_15_5_140_5_end
        if_15_8_140_5_code:
            mov rdi, 1
            exit_15_17_140_5:
                    mov rax, 60
                syscall
            exit_15_17_140_5_end:
        if_15_5_140_5_end:
    assert_140_5_end:
    cmp_141_12:
    cmp qword [rsp - 124], 15
    sete r15b
    bool_end_141_12:
    assert_141_5:
        if_15_8_141_5:
        cmp_15_8_141_5:
        cmp r15b, 0
        jne if_15_5_141_5_end
        if_15_8_141_5_code:
            mov rdi, 1
            exit_15_17_141_5:
                    mov rax, 60
                syscall
            exit_15_17_141_5_end:
        if_15_5_141_5_end:
    assert_141_5_end:
    mov qword [rsp - 140], 5
    mov qword [rsp - 148], 3
    double_148_25:
        mov r15, qword [rsp - 140]
        imul r15, 2
        mov qword [rsp - 168], r15
    double_148_25_end:
    triple_148_37:
        mov r15, qword [rsp - 148]
        imul r15, 3
    triple_148_37_end:
    add qword [rsp - 168], r15
    increment_148_49:
        mov r15, 2
        add r15, 1
    increment_148_49_end:
    sub qword [rsp - 168], r15
    square_148_72:
        mov r15, qword [rsp - 140]
        imul r15, qword [rsp - 140]
    square_148_72_end:
    multiply_148_63:
        mov r14, r15
        imul r14, 2
        mov qword [rsp - 160], r14
    multiply_148_63_end:
    halve_148_88:
        mov r15, 10
        mov rax, r15
        cqo
        mov r14, 2
        idiv r14
        mov r15, rax
    halve_148_88_end:
    mov rax, qword [rsp - 160]
    cqo
    idiv r15
    mov qword [rsp - 160], rax
    increment_148_114:
        mov r15d, 254
        add r15d, 1
    increment_148_114_end:
    compute_color_148_100:
        movsx r14, r15d
        imul r14, 65536
        mov qword [rsp - 152], r14
        mov r14, 0
        imul r14, 256
        add qword [rsp - 152], r14
        add qword [rsp - 152], 0
    compute_color_148_100_end:
    cmp_149_12:
    cmp qword [rsp - 168], 16
    sete r15b
    bool_end_149_12:
    assert_149_5:
        if_15_8_149_5:
        cmp_15_8_149_5:
        cmp r15b, 0
        jne if_15_5_149_5_end
        if_15_8_149_5_code:
            mov rdi, 1
            exit_15_17_149_5:
                    mov rax, 60
                syscall
            exit_15_17_149_5_end:
        if_15_5_149_5_end:
    assert_149_5_end:
    cmp_150_12:
    cmp qword [rsp - 160], 10
    sete r15b
    bool_end_150_12:
    assert_150_5:
        if_15_8_150_5:
        cmp_15_8_150_5:
        cmp r15b, 0
        jne if_15_5_150_5_end
        if_15_8_150_5_code:
            mov rdi, 1
            exit_15_17_150_5:
                    mov rax, 60
                syscall
            exit_15_17_150_5_end:
        if_15_5_150_5_end:
    assert_150_5_end:
    cmp_151_12:
    cmp dword [rsp - 152], 16711680
    sete r15b
    bool_end_151_12:
    assert_151_5:
        if_15_8_151_5:
        cmp_15_8_151_5:
        cmp r15b, 0
        jne if_15_5_151_5_end
        if_15_8_151_5_code:
            mov rdi, 1
            exit_15_17_151_5:
                    mov rax, 60
                syscall
            exit_15_17_151_5_end:
        if_15_5_151_5_end:
    assert_151_5_end:
    add_156_34:
        mov r14, 3
        add r14, 2
    add_156_34_end:
    double_156_27:
        mov r15, r14
        imul r15, 2
    double_156_27_end:
    increment_156_58:
        mov r13, 4
        add r13, 1
    increment_156_58_end:
    multiply_156_46:
        mov r14, 2
        imul r14, r13
    multiply_156_46_end:
    add_156_23:
        mov qword [rsp - 184], r15
        add qword [rsp - 184], r14
    add_156_23_end:
    add_156_90:
        mov r14, 2
        add r14, 3
    add_156_90_end:
    square_156_83:
        mov r15, r14
        imul r15, r14
    square_156_83_end:
    triple_156_109:
        mov r13, 2
        imul r13, 3
    triple_156_109_end:
    double_156_102:
        mov r14, r13
        imul r14, 2
    double_156_102_end:
    subtract_156_74:
        mov qword [rsp - 176], r15
        sub qword [rsp - 176], r14
    subtract_156_74_end:
    cmp_157_12:
    cmp qword [rsp - 184], 20
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_15_8_157_5:
        cmp_15_8_157_5:
        cmp r15b, 0
        jne if_15_5_157_5_end
        if_15_8_157_5_code:
            mov rdi, 1
            exit_15_17_157_5:
                    mov rax, 60
                syscall
            exit_15_17_157_5_end:
        if_15_5_157_5_end:
    assert_157_5_end:
    cmp_158_12:
    cmp qword [rsp - 176], 13
    sete r15b
    bool_end_158_12:
    assert_158_5:
        if_15_8_158_5:
        cmp_15_8_158_5:
        cmp r15b, 0
        jne if_15_5_158_5_end
        if_15_8_158_5_code:
            mov rdi, 1
            exit_15_17_158_5:
                    mov rax, 60
                syscall
            exit_15_17_158_5_end:
        if_15_5_158_5_end:
    assert_158_5_end:
    mov qword [rsp - 204], 0
    mov qword [rsp - 196], 0
    mov dword [rsp - 188], 0
    double_164_29:
        mov r14, 3
        imul r14, 2
    double_164_29_end:
    triple_164_40:
        mov r13, 2
        imul r13, 3
    triple_164_40_end:
    multiply_164_20:
        mov r15, r14
        imul r15, r13
    multiply_164_20_end:
    halve_164_65:
        mov r13, 10
        mov rax, r13
        cqo
        mov r12, 2
        idiv r12
        mov r13, rax
    halve_164_65_end:
    subtract_164_52:
        mov r14, 20
        sub r14, r13
    subtract_164_52_end:
    add_164_16:
        mov qword [rsp - 204], r15
        add qword [rsp - 204], r14
    add_164_16_end:
    square_165_35:
        mov r14, 3
        imul r14, 3
    square_165_35_end:
    increment_165_25:
        mov r15, r14
        add r15, 1
    increment_165_25_end:
    cube_165_57:
        mov r13, 2
        imul r13, 2
        imul r13, 2
    cube_165_57_end:
    decrement_165_47:
        mov r14, r13
        sub r14, 1
    decrement_165_47_end:
    multiply_165_16:
        mov r13, r15
        imul r13, r14
        mov qword [rsp - 196], r13
    multiply_165_16_end:
    cmp_166_12:
    cmp qword [rsp - 204], 51
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_15_8_166_5:
        cmp_15_8_166_5:
        cmp r15b, 0
        jne if_15_5_166_5_end
        if_15_8_166_5_code:
            mov rdi, 1
            exit_15_17_166_5:
                    mov rax, 60
                syscall
            exit_15_17_166_5_end:
        if_15_5_166_5_end:
    assert_166_5_end:
    cmp_167_12:
    cmp qword [rsp - 196], 70
    sete r15b
    bool_end_167_12:
    assert_167_5:
        if_15_8_167_5:
        cmp_15_8_167_5:
        cmp r15b, 0
        jne if_15_5_167_5_end
        if_15_8_167_5_code:
            mov rdi, 1
            exit_15_17_167_5:
                    mov rax, 60
                syscall
            exit_15_17_167_5_end:
        if_15_5_167_5_end:
    assert_167_5_end:
    square_172_43:
        mov r13, 2
        imul r13, 2
    square_172_43_end:
    triple_172_36:
        mov r14, r13
        imul r14, 3
    triple_172_36_end:
    double_172_29:
        mov r15, r14
        imul r15, 2
    double_172_29_end:
    halve_172_23:
        mov qword [rsp - 220], r15
        mov rax, qword [rsp - 220]
        cqo
        mov r14, 2
        idiv r14
        mov qword [rsp - 220], rax
    halve_172_23_end:
    double_172_71:
        mov r14, 5
        imul r14, 2
    double_172_71_end:
    increment_172_61:
        mov r15, r14
        add r15, 1
    increment_172_61_end:
    triple_172_93:
        mov r13, 4
        imul r13, 3
    triple_172_93_end:
    decrement_172_83:
        mov r14, r13
        sub r14, 1
    decrement_172_83_end:
    add_172_57:
        mov qword [rsp - 212], r15
        add qword [rsp - 212], r14
    add_172_57_end:
    cmp_173_12:
    cmp qword [rsp - 220], 12
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_15_8_173_5:
        cmp_15_8_173_5:
        cmp r15b, 0
        jne if_15_5_173_5_end
        if_15_8_173_5_code:
            mov rdi, 1
            exit_15_17_173_5:
                    mov rax, 60
                syscall
            exit_15_17_173_5_end:
        if_15_5_173_5_end:
    assert_173_5_end:
    cmp_174_12:
    cmp qword [rsp - 212], 22
    sete r15b
    bool_end_174_12:
    assert_174_5:
        if_15_8_174_5:
        cmp_15_8_174_5:
        cmp r15b, 0
        jne if_15_5_174_5_end
        if_15_8_174_5_code:
            mov rdi, 1
            exit_15_17_174_5:
                    mov rax, 60
                syscall
            exit_15_17_174_5_end:
        if_15_5_174_5_end:
    assert_174_5_end:
    add_179_27:
        mov r15, 5
        add r15, 10
    add_179_27_end:
    multiply_179_39:
        mov r14, 3
        imul r14, 4
    multiply_179_39_end:
    max_179_23:
        if_67_8_179_23:
        cmp_67_8_179_23:
        cmp r15, r14
        jle if_else_67_5_179_23
        if_67_8_179_23_code:
            mov qword [rsp - 236], r15
        jmp if_67_5_179_23_end
        if_else_67_5_179_23:
            mov qword [rsp - 236], r14
        if_67_5_179_23_end:
    max_179_23_end:
    add qword [rsp - 236], 5
    square_179_64:
        mov r14, 5
        imul r14, 5
    square_179_64_end:
    double_179_75:
        mov r13, 10
        imul r13, 2
    double_179_75_end:
    min_179_60:
        if_75_8_179_60:
        cmp_75_8_179_60:
        cmp r14, r13
        jge if_else_75_5_179_60
        if_75_8_179_60_code:
            mov r15, r14
        jmp if_75_5_179_60_end
        if_else_75_5_179_60:
            mov r15, r13
        if_75_5_179_60_end:
    min_179_60_end:
    imul r15, 2
    mov qword [rsp - 228], r15
    cmp_180_12:
    cmp qword [rsp - 236], 20
    sete r15b
    bool_end_180_12:
    assert_180_5:
        if_15_8_180_5:
        cmp_15_8_180_5:
        cmp r15b, 0
        jne if_15_5_180_5_end
        if_15_8_180_5_code:
            mov rdi, 1
            exit_15_17_180_5:
                    mov rax, 60
                syscall
            exit_15_17_180_5_end:
        if_15_5_180_5_end:
    assert_180_5_end:
    cmp_181_12:
    cmp qword [rsp - 228], 40
    sete r15b
    bool_end_181_12:
    assert_181_5:
        if_15_8_181_5:
        cmp_15_8_181_5:
        cmp r15b, 0
        jne if_15_5_181_5_end
        if_15_8_181_5_code:
            mov rdi, 1
            exit_15_17_181_5:
                    mov rax, 60
                syscall
            exit_15_17_181_5_end:
        if_15_5_181_5_end:
    assert_181_5_end:
    mov qword [rsp - 244], 128
    mov qword [rsp - 252], 64
    mov qword [rsp - 260], 32
    add_188_25:
        mov r15, qword [rsp - 244]
        add r15, qword [rsp - 252]
        mov qword [rsp - 280], r15
    add_188_25_end:
    subtract_188_36:
        mov r15, qword [rsp - 244]
        sub r15, qword [rsp - 260]
        mov qword [rsp - 272], r15
    subtract_188_36_end:
    halve_188_67:
        mov r14d, dword [rsp - 244]
        mov eax, r14d
        cdq
        mov r13, 2
        idiv r13
        mov r14d, eax
    halve_188_67_end:
    double_188_77:
        mov r13d, dword [rsp - 252]
        imul r13d, 2
    double_188_77_end:
    triple_188_88:
        mov r12d, dword [rsp - 260]
        imul r12d, 3
    triple_188_88_end:
    compute_color_188_53:
        mov r15d, r14d
        imul r15d, 65536
        mov r11d, r13d
        imul r11d, 256
        add r15d, r11d
        add r15d, r12d
    compute_color_188_53_end:
    mov dword [rsp - 264], r15d
    cmp_189_12:
    cmp qword [rsp - 280], 192
    sete r15b
    bool_end_189_12:
    assert_189_5:
        if_15_8_189_5:
        cmp_15_8_189_5:
        cmp r15b, 0
        jne if_15_5_189_5_end
        if_15_8_189_5_code:
            mov rdi, 1
            exit_15_17_189_5:
                    mov rax, 60
                syscall
            exit_15_17_189_5_end:
        if_15_5_189_5_end:
    assert_189_5_end:
    cmp_190_12:
    cmp qword [rsp - 272], 96
    sete r15b
    bool_end_190_12:
    assert_190_5:
        if_15_8_190_5:
        cmp_15_8_190_5:
        cmp r15b, 0
        jne if_15_5_190_5_end
        if_15_8_190_5_code:
            mov rdi, 1
            exit_15_17_190_5:
                    mov rax, 60
                syscall
            exit_15_17_190_5_end:
        if_15_5_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    cmp dword [rsp - 264], 4227168
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_15_8_191_5:
        cmp_15_8_191_5:
        cmp r15b, 0
        jne if_15_5_191_5_end
        if_15_8_191_5_code:
            mov rdi, 1
            exit_15_17_191_5:
                    mov rax, 60
                syscall
            exit_15_17_191_5_end:
        if_15_5_191_5_end:
    assert_191_5_end:
    triple_196_35:
        mov r14, 2
        imul r14, 3
    triple_196_35_end:
    square_196_46:
        mov r13, 3
        imul r13, 3
    square_196_46_end:
    add_196_31:
        mov r15, r14
        add r15, r13
    add_196_31_end:
    double_196_24:
        mov r14, r15
        imul r14, 2
        mov qword [rsp - 296], r14
    double_196_24_end:
    increment_196_66:
        mov r14, 19
        add r14, 1
    increment_196_66_end:
    halve_196_60:
        mov r15, r14
        mov rax, r15
        cqo
        mov r13, 2
        idiv r13
        mov r15, rax
    halve_196_60_end:
    sub qword [rsp - 296], r15
    double_196_97:
        mov r14, 8
        imul r14, 2
    double_196_97_end:
    halve_196_91:
        mov r15, r14
        mov rax, r15
        cqo
        mov r13, 2
        idiv r13
        mov r15, rax
    halve_196_91_end:
    square_196_116:
        mov r13, 2
        imul r13, 2
    square_196_116_end:
    add_196_109:
        mov r14, 3
        add r14, r13
    add_196_109_end:
    multiply_196_82:
        mov r13, r15
        imul r13, r14
        mov qword [rsp - 288], r13
    multiply_196_82_end:
    cmp_197_12:
    cmp qword [rsp - 296], 20
    sete r15b
    bool_end_197_12:
    assert_197_5:
        if_15_8_197_5:
        cmp_15_8_197_5:
        cmp r15b, 0
        jne if_15_5_197_5_end
        if_15_8_197_5_code:
            mov rdi, 1
            exit_15_17_197_5:
                    mov rax, 60
                syscall
            exit_15_17_197_5_end:
        if_15_5_197_5_end:
    assert_197_5_end:
    cmp_198_12:
    cmp qword [rsp - 288], 56
    sete r15b
    bool_end_198_12:
    assert_198_5:
        if_15_8_198_5:
        cmp_15_8_198_5:
        cmp r15b, 0
        jne if_15_5_198_5_end
        if_15_8_198_5_code:
            mov rdi, 1
            exit_15_17_198_5:
                    mov rax, 60
                syscall
            exit_15_17_198_5_end:
        if_15_5_198_5_end:
    assert_198_5_end:
    mov qword [rsp - 304], 2
    mov qword [rsp - 312], 3
    square_205_37:
        mov r14, qword [rsp - 304]
        imul r14, qword [rsp - 304]
    square_205_37_end:
    cube_205_48:
        mov r13, qword [rsp - 312]
        imul r13, qword [rsp - 312]
        imul r13, qword [rsp - 312]
    cube_205_48_end:
    multiply_205_28:
        mov r15, r14
        imul r15, r13
    multiply_205_28_end:
    double_205_67:
        mov r13, 10
        imul r13, 2
    double_205_67_end:
    triple_205_79:
        mov r12, 2
        imul r12, 3
    triple_205_79_end:
    subtract_205_58:
        mov r14, r13
        sub r14, r12
    subtract_205_58_end:
    add_205_24:
        mov qword [rsp - 328], r15
        add qword [rsp - 328], r14
    add_205_24_end:
    halve_205_96:
        mov r15, 100
        mov rax, r15
        cqo
        mov r14, 2
        idiv r14
        mov r15, rax
    halve_205_96_end:
    square_205_112:
        mov r13, 8
        imul r13, 8
    square_205_112_end:
    triple_205_123:
        mov r12, 20
        imul r12, 3
    triple_205_123_end:
    min_205_108:
        if_75_8_205_108:
        cmp_75_8_205_108:
        cmp r13, r12
        jge if_else_75_5_205_108
        if_75_8_205_108_code:
            mov r14, r13
        jmp if_75_5_205_108_end
        if_else_75_5_205_108:
            mov r14, r12
        if_75_5_205_108_end:
    min_205_108_end:
    max_205_92:
        if_67_8_205_92:
        cmp_67_8_205_92:
        cmp r15, r14
        jle if_else_67_5_205_92
        if_67_8_205_92_code:
            mov qword [rsp - 320], r15
        jmp if_67_5_205_92_end
        if_else_67_5_205_92:
            mov qword [rsp - 320], r14
        if_67_5_205_92_end:
    max_205_92_end:
    cmp_206_12:
    cmp qword [rsp - 328], 122
    sete r15b
    bool_end_206_12:
    assert_206_5:
        if_15_8_206_5:
        cmp_15_8_206_5:
        cmp r15b, 0
        jne if_15_5_206_5_end
        if_15_8_206_5_code:
            mov rdi, 1
            exit_15_17_206_5:
                    mov rax, 60
                syscall
            exit_15_17_206_5_end:
        if_15_5_206_5_end:
    assert_206_5_end:
    cmp_207_12:
    cmp qword [rsp - 320], 60
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_15_8_207_5:
        cmp_15_8_207_5:
        cmp r15b, 0
        jne if_15_5_207_5_end
        if_15_8_207_5_code:
            mov rdi, 1
            exit_15_17_207_5:
                    mov rax, 60
                syscall
            exit_15_17_207_5_end:
        if_15_5_207_5_end:
    assert_207_5_end:
    square_212_42:
        mov r13, 5
        imul r13, 5
    square_212_42_end:
    double_212_35:
        mov r14, r13
        imul r14, 2
    double_212_35_end:
    cube_212_61:
        mov r12, 2
        imul r12, 2
        imul r12, 2
    cube_212_61_end:
    triple_212_54:
        mov r13, r12
        imul r13, 3
    triple_212_54_end:
    add_212_31:
        mov r15, r14
        add r15, r13
    add_212_31_end:
    halve_212_25:
        mov qword [rsp - 348], r15
        mov rax, qword [rsp - 348]
        cqo
        mov r14, 2
        idiv r14
        mov qword [rsp - 348], rax
    halve_212_25_end:
    decrement_212_92:
        mov r14, 10
        sub r14, 1
    decrement_212_92_end:
    halve_212_107:
        mov r13, 8
        mov rax, r13
        cqo
        mov r12, 2
        idiv r12
        mov r13, rax
    halve_212_107_end:
    multiply_212_83:
        mov r15, r14
        imul r15, r13
    multiply_212_83_end:
    increment_212_73:
        mov qword [rsp - 340], r15
        add qword [rsp - 340], 1
    increment_212_73_end:
    mov dword [rsp - 332], 11259375
    cmp_213_12:
    cmp qword [rsp - 348], 37
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_15_8_213_5:
        cmp_15_8_213_5:
        cmp r15b, 0
        jne if_15_5_213_5_end
        if_15_8_213_5_code:
            mov rdi, 1
            exit_15_17_213_5:
                    mov rax, 60
                syscall
            exit_15_17_213_5_end:
        if_15_5_213_5_end:
    assert_213_5_end:
    cmp_214_12:
    cmp qword [rsp - 340], 37
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_15_8_214_5:
        cmp_15_8_214_5:
        cmp r15b, 0
        jne if_15_5_214_5_end
        if_15_8_214_5_code:
            mov rdi, 1
            exit_15_17_214_5:
                    mov rax, 60
                syscall
            exit_15_17_214_5_end:
        if_15_5_214_5_end:
    assert_214_5_end:
    multiply_219_37:
        mov r14, 2
        imul r14, 3
    multiply_219_37_end:
    square_219_53:
        mov r13, 4
        imul r13, 4
    square_219_53_end:
    subtract_219_28:
        mov r15, r14
        sub r15, r13
    subtract_219_28_end:
    abs_219_24:
        if_83_8_219_24:
        cmp_83_8_219_24:
        cmp r15, 0
        jge if_else_83_5_219_24
        if_83_8_219_24_code:
            mov qword [rsp - 364], r15
            neg qword [rsp - 364]
        jmp if_83_5_219_24_end
        if_else_83_5_219_24:
            mov qword [rsp - 364], r15
        if_83_5_219_24_end:
    abs_219_24_end:
    double_219_81:
        mov r13, 5
        imul r13, 2
    double_219_81_end:
    triple_219_92:
        mov r12, 3
        imul r12, 3
    triple_219_92_end:
    add_219_77:
        mov r14, r13
        add r14, r12
    add_219_77_end:
    negate_219_70:
        mov r15, r14
        neg r15
    negate_219_70_end:
    abs_219_66:
        if_83_8_219_66:
        cmp_83_8_219_66:
        cmp r15, 0
        jge if_else_83_5_219_66
        if_83_8_219_66_code:
            mov qword [rsp - 356], r15
            neg qword [rsp - 356]
        jmp if_83_5_219_66_end
        if_else_83_5_219_66:
            mov qword [rsp - 356], r15
        if_83_5_219_66_end:
    abs_219_66_end:
    cmp_220_12:
    cmp qword [rsp - 364], 10
    sete r15b
    bool_end_220_12:
    assert_220_5:
        if_15_8_220_5:
        cmp_15_8_220_5:
        cmp r15b, 0
        jne if_15_5_220_5_end
        if_15_8_220_5_code:
            mov rdi, 1
            exit_15_17_220_5:
                    mov rax, 60
                syscall
            exit_15_17_220_5_end:
        if_15_5_220_5_end:
    assert_220_5_end:
    cmp_221_12:
    cmp qword [rsp - 356], 19
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_15_8_221_5:
        cmp_15_8_221_5:
        cmp r15b, 0
        jne if_15_5_221_5_end
        if_15_8_221_5_code:
            mov rdi, 1
            exit_15_17_221_5:
                    mov rax, 60
                syscall
            exit_15_17_221_5_end:
        if_15_5_221_5_end:
    assert_221_5_end:
    double_226_31:
        mov r15, 3
        imul r15, 2
    double_226_31_end:
    add_226_24:
        mov qword [rsp - 380], 5
        add qword [rsp - 380], r15
    add_226_24_end:
    increment_226_44:
        mov r15, 2
        add r15, 1
    increment_226_44_end:
    imul r15, qword [rsp - 380]
    mov qword [rsp - 380], r15
    halve_226_59:
        mov r15, 20
        mov rax, r15
        cqo
        mov r14, 2
        idiv r14
        mov r15, rax
    halve_226_59_end:
    sub qword [rsp - 380], r15
    square_226_77:
        mov r15, 2
        imul r15, 2
    square_226_77_end:
    triple_226_70:
        mov r14, r15
        imul r14, 3
        mov qword [rsp - 372], r14
    triple_226_70_end:
    decrement_226_99:
        mov r14, 5
        sub r14, 1
    decrement_226_99_end:
    multiply_226_90:
        mov r15, r14
        imul r15, 3
    multiply_226_90_end:
    add qword [rsp - 372], r15
    cmp_227_12:
    cmp qword [rsp - 380], 23
    sete r15b
    bool_end_227_12:
    assert_227_5:
        if_15_8_227_5:
        cmp_15_8_227_5:
        cmp r15b, 0
        jne if_15_5_227_5_end
        if_15_8_227_5_code:
            mov rdi, 1
            exit_15_17_227_5:
                    mov rax, 60
                syscall
            exit_15_17_227_5_end:
        if_15_5_227_5_end:
    assert_227_5_end:
    cmp_228_12:
    cmp qword [rsp - 372], 24
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_15_8_228_5:
        cmp_15_8_228_5:
        cmp r15b, 0
        jne if_15_5_228_5_end
        if_15_8_228_5_code:
            mov rdi, 1
            exit_15_17_228_5:
                    mov rax, 60
                syscall
            exit_15_17_228_5_end:
        if_15_5_228_5_end:
    assert_228_5_end:
    mov qword [rsp - 400], 10
    mov qword [rsp - 392], 20
    compute_color_233_34:
        mov r15, 255
        imul r15, 65536
        mov qword [rsp - 384], r15
        mov r15, 255
        imul r15, 256
        add qword [rsp - 384], r15
        add qword [rsp - 384], 255
    compute_color_233_34_end:
    double_234_29:
        mov r14, 3
        imul r14, 2
    double_234_29_end:
    triple_234_40:
        mov r13, 2
        imul r13, 3
    triple_234_40_end:
    add_234_25:
        mov r15, r14
        add r15, r13
    add_234_25_end:
    increment_234_58:
        mov r13, 3
        add r13, 1
    increment_234_58_end:
    halve_234_52:
        mov r14, r13
        mov rax, r14
        cqo
        mov r12, 2
        idiv r12
        mov r14, rax
    halve_234_52_end:
    multiply_234_16:
        mov r13, r15
        imul r13, r14
        mov qword [rsp - 400], r13
    multiply_234_16_end:
    increment_235_32:
        mov r14, 5
        add r14, 1
    increment_235_32_end:
    square_235_25:
        mov r15, r14
        imul r15, r14
    square_235_25_end:
    halve_235_54:
        mov r13, 10
        mov rax, r13
        cqo
        mov r12, 2
        idiv r12
        mov r13, rax
    halve_235_54_end:
    double_235_47:
        mov r14, r13
        imul r14, 2
    double_235_47_end:
    subtract_235_16:
        mov qword [rsp - 392], r15
        sub qword [rsp - 392], r14
    subtract_235_16_end:
    cmp_236_12:
    cmp qword [rsp - 400], 24
    sete r15b
    bool_end_236_12:
    assert_236_5:
        if_15_8_236_5:
        cmp_15_8_236_5:
        cmp r15b, 0
        jne if_15_5_236_5_end
        if_15_8_236_5_code:
            mov rdi, 1
            exit_15_17_236_5:
                    mov rax, 60
                syscall
            exit_15_17_236_5_end:
        if_15_5_236_5_end:
    assert_236_5_end:
    cmp_237_12:
    cmp qword [rsp - 392], 26
    sete r15b
    bool_end_237_12:
    assert_237_5:
        if_15_8_237_5:
        cmp_15_8_237_5:
        cmp r15b, 0
        jne if_15_5_237_5_end
        if_15_8_237_5_code:
            mov rdi, 1
            exit_15_17_237_5:
                    mov rax, 60
                syscall
            exit_15_17_237_5_end:
        if_15_5_237_5_end:
    assert_237_5_end:
    square_242_41:
        mov r13, 4
        imul r13, 4
    square_242_41_end:
    double_242_59:
        mov r11, 3
        imul r11, 2
    double_242_59_end:
    triple_242_52:
        mov r12, r11
        imul r12, 3
    triple_242_52_end:
    add_242_37:
        mov r14, r13
        add r14, r12
    add_242_37_end:
    halve_242_31:
        mov r15, r14
        mov rax, r15
        cqo
        mov r13, 2
        idiv r13
        mov r15, rax
    halve_242_31_end:
    double_242_24:
        mov r14, r15
        imul r14, 2
        mov qword [rsp - 416], r14
    double_242_24_end:
    cube_242_96:
        mov r13, 3
        imul r13, 3
        imul r13, 3
    cube_242_96_end:
    square_242_112:
        mov r11, 2
        imul r11, 2
    square_242_112_end:
    double_242_105:
        mov r12, r11
        imul r12, 2
    double_242_105_end:
    subtract_242_87:
        mov r14, r13
        sub r14, r12
    subtract_242_87_end:
    halve_242_81:
        mov r15, r14
        mov rax, r15
        cqo
        mov r13, 2
        idiv r13
        mov r15, rax
    halve_242_81_end:
    triple_242_74:
        mov r14, r15
        imul r14, 3
        mov qword [rsp - 408], r14
    triple_242_74_end:
    cmp_243_12:
    cmp qword [rsp - 416], 34
    sete r15b
    bool_end_243_12:
    assert_243_5:
        if_15_8_243_5:
        cmp_15_8_243_5:
        cmp r15b, 0
        jne if_15_5_243_5_end
        if_15_8_243_5_code:
            mov rdi, 1
            exit_15_17_243_5:
                    mov rax, 60
                syscall
            exit_15_17_243_5_end:
        if_15_5_243_5_end:
    assert_243_5_end:
    cmp_244_12:
    cmp qword [rsp - 408], 27
    sete r15b
    bool_end_244_12:
    assert_244_5:
        if_15_8_244_5:
        cmp_15_8_244_5:
        cmp r15b, 0
        jne if_15_5_244_5_end
        if_15_8_244_5_code:
            mov rdi, 1
            exit_15_17_244_5:
                    mov rax, 60
                syscall
            exit_15_17_244_5_end:
        if_15_5_244_5_end:
    assert_244_5_end:
    double_249_32:
        mov r14, 5
        imul r14, 2
    double_249_32_end:
    triple_249_43:
        mov r13, 3
        imul r13, 3
    triple_249_43_end:
    max_249_28:
        if_67_8_249_28:
        cmp_67_8_249_28:
        cmp r14, r13
        jle if_else_67_5_249_28
        if_67_8_249_28_code:
            mov r15, r14
        jmp if_67_5_249_28_end
        if_else_67_5_249_28:
            mov r15, r13
        if_67_5_249_28_end:
    max_249_28_end:
    square_249_59:
        mov r13, 3
        imul r13, 3
    square_249_59_end:
    add_249_55:
        mov r14, r13
        add r14, 10
    add_249_55_end:
    min_249_24:
        if_75_8_249_24:
        cmp_75_8_249_24:
        cmp r15, r14
        jge if_else_75_5_249_24
        if_75_8_249_24_code:
            mov qword [rsp - 432], r15
        jmp if_75_5_249_24_end
        if_else_75_5_249_24:
            mov qword [rsp - 432], r14
        if_75_5_249_24_end:
    min_249_24_end:
    cube_249_84:
        mov r14, 2
        imul r14, 2
        imul r14, 2
    cube_249_84_end:
    square_249_93:
        mov r13, 4
        imul r13, 4
    square_249_93_end:
    min_249_80:
        if_75_8_249_80:
        cmp_75_8_249_80:
        cmp r14, r13
        jge if_else_75_5_249_80
        if_75_8_249_80_code:
            mov r15, r14
        jmp if_75_5_249_80_end
        if_else_75_5_249_80:
            mov r15, r13
        if_75_5_249_80_end:
    min_249_80_end:
    halve_249_105:
        mov r14, 30
        mov rax, r14
        cqo
        mov r13, 2
        idiv r13
        mov r14, rax
    halve_249_105_end:
    max_249_76:
        if_67_8_249_76:
        cmp_67_8_249_76:
        cmp r15, r14
        jle if_else_67_5_249_76
        if_67_8_249_76_code:
            mov qword [rsp - 424], r15
        jmp if_67_5_249_76_end
        if_else_67_5_249_76:
            mov qword [rsp - 424], r14
        if_67_5_249_76_end:
    max_249_76_end:
    cmp_250_12:
    cmp qword [rsp - 432], 10
    sete r15b
    bool_end_250_12:
    assert_250_5:
        if_15_8_250_5:
        cmp_15_8_250_5:
        cmp r15b, 0
        jne if_15_5_250_5_end
        if_15_8_250_5_code:
            mov rdi, 1
            exit_15_17_250_5:
                    mov rax, 60
                syscall
            exit_15_17_250_5_end:
        if_15_5_250_5_end:
    assert_250_5_end:
    cmp_251_12:
    cmp qword [rsp - 424], 15
    sete r15b
    bool_end_251_12:
    assert_251_5:
        if_15_8_251_5:
        cmp_15_8_251_5:
        cmp r15b, 0
        jne if_15_5_251_5_end
        if_15_8_251_5_code:
            mov rdi, 1
            exit_15_17_251_5:
                    mov rax, 60
                syscall
            exit_15_17_251_5_end:
        if_15_5_251_5_end:
    assert_251_5_end:
    negate_256_33:
        mov r14, -5
    negate_256_33_end:
    abs_256_29:
        if_83_8_256_29:
        cmp_83_8_256_29:
        cmp r14, 0
        jge if_else_83_5_256_29
        if_83_8_256_29_code:
            mov r15, r14
            neg r15
        jmp if_83_5_256_29_end
        if_else_83_5_256_29:
            mov r15, r14
        if_83_5_256_29_end:
    abs_256_29_end:
    double_256_49:
        mov r13, 3
        imul r13, 2
    double_256_49_end:
    max_256_45:
        if_67_8_256_45:
        cmp_67_8_256_45:
        cmp r13, 4
        jle if_else_67_5_256_45
        if_67_8_256_45_code:
            mov r14, r13
        jmp if_67_5_256_45_end
        if_else_67_5_256_45:
            mov r14, 4
        if_67_5_256_45_end:
    max_256_45_end:
    add_256_25:
        mov qword [rsp - 452], r15
        add qword [rsp - 452], r14
    add_256_25_end:
    triple_256_78:
        mov r14, 2
        imul r14, 3
    triple_256_78_end:
    min_256_74:
        if_75_8_256_74:
        cmp_75_8_256_74:
        cmp r14, 10
        jge if_else_75_5_256_74
        if_75_8_256_74_code:
            mov r15, r14
        jmp if_75_5_256_74_end
        if_else_75_5_256_74:
            mov r15, 10
        if_75_5_256_74_end:
    min_256_74_end:
    halve_256_104:
        mov r13, 6
        mov rax, r13
        cqo
        mov r12, 2
        idiv r12
        mov r13, rax
    halve_256_104_end:
    increment_256_94:
        mov r14, r13
        add r14, 1
    increment_256_94_end:
    multiply_256_65:
        mov r13, r15
        imul r13, r14
        mov qword [rsp - 444], r13
    multiply_256_65_end:
    compute_color_256_117:
        mov r15, 18
        imul r15, 65536
        mov qword [rsp - 436], r15
        mov r15, 52
        imul r15, 256
        add qword [rsp - 436], r15
        add qword [rsp - 436], 86
    compute_color_256_117_end:
    cmp_257_12:
    cmp qword [rsp - 452], 11
    sete r15b
    bool_end_257_12:
    assert_257_5:
        if_15_8_257_5:
        cmp_15_8_257_5:
        cmp r15b, 0
        jne if_15_5_257_5_end
        if_15_8_257_5_code:
            mov rdi, 1
            exit_15_17_257_5:
                    mov rax, 60
                syscall
            exit_15_17_257_5_end:
        if_15_5_257_5_end:
    assert_257_5_end:
    cmp_258_12:
    cmp qword [rsp - 444], 24
    sete r15b
    bool_end_258_12:
    assert_258_5:
        if_15_8_258_5:
        cmp_15_8_258_5:
        cmp r15b, 0
        jne if_15_5_258_5_end
        if_15_8_258_5_code:
            mov rdi, 1
            exit_15_17_258_5:
                    mov rax, 60
                syscall
            exit_15_17_258_5_end:
        if_15_5_258_5_end:
    assert_258_5_end:
    triple_263_44:
        mov r13, 2
        imul r13, 3
    triple_263_44_end:
    double_263_37:
        mov r14, r13
        imul r14, 2
    double_263_37_end:
    square_263_62:
        mov r12, 4
        imul r12, 4
    square_263_62_end:
    halve_263_56:
        mov r13, r12
        mov rax, r13
        cqo
        mov r11, 2
        idiv r11
        mov r13, rax
    halve_263_56_end:
    multiply_263_28:
        mov r15, r14
        imul r15, r13
    multiply_263_28_end:
    cube_263_84:
        mov r13, 2
        imul r13, 2
        imul r13, 2
    cube_263_84_end:
    increment_263_93:
        mov r12, 5
        add r12, 1
    increment_263_93_end:
    subtract_263_75:
        mov r14, r13
        sub r14, r12
    subtract_263_75_end:
    add_263_24:
        mov qword [rsp - 468], r15
        add qword [rsp - 468], r14
    add_263_24_end:
    square_263_120:
        mov r14, 2
        imul r14, 2
    square_263_120_end:
    triple_263_140:
        mov r12, 1
        imul r12, 3
    triple_263_140_end:
    halve_263_151:
        mov r11, 6
        mov rax, r11
        cqo
        mov r10, 2
        idiv r10
        mov r11, rax
    halve_263_151_end:
    multiply_263_131:
        mov r13, r12
        imul r13, r11
    multiply_263_131_end:
    add_263_116:
        mov r15, r14
        add r15, r13
    add_263_116_end:
    double_263_109:
        mov r14, r15
        imul r14, 2
        mov qword [rsp - 460], r14
    double_263_109_end:
    cmp_264_12:
    cmp qword [rsp - 468], 98
    sete r15b
    bool_end_264_12:
    assert_264_5:
        if_15_8_264_5:
        cmp_15_8_264_5:
        cmp r15b, 0
        jne if_15_5_264_5_end
        if_15_8_264_5_code:
            mov rdi, 1
            exit_15_17_264_5:
                    mov rax, 60
                syscall
            exit_15_17_264_5_end:
        if_15_5_264_5_end:
    assert_264_5_end:
    cmp_265_12:
    cmp qword [rsp - 460], 26
    sete r15b
    bool_end_265_12:
    assert_265_5:
        if_15_8_265_5:
        cmp_15_8_265_5:
        cmp r15b, 0
        jne if_15_5_265_5_end
        if_15_8_265_5_code:
            mov rdi, 1
            exit_15_17_265_5:
                    mov rax, 60
                syscall
            exit_15_17_265_5_end:
        if_15_5_265_5_end:
    assert_265_5_end:
    square_270_50:
        mov r14, 3
        imul r14, 3
    square_270_50_end:
    multiply_270_41:
        mov r15, r14
        imul r15, 2
    multiply_270_41_end:
    triple_270_72:
        mov r13, 2
        imul r13, 3
    triple_270_72_end:
    double_270_65:
        mov r14, r13
        imul r14, 2
    double_270_65_end:
    subtract_270_32:
        mov qword [rsp - 484], r15
        sub qword [rsp - 484], r14
    subtract_270_32_end:
    triple_270_95:
        mov r14, 20
        imul r14, 3
    triple_270_95_end:
    halve_270_89:
        mov r15, r14
        mov rax, r15
        cqo
        mov r13, 2
        idiv r13
        mov r15, rax
    halve_270_89_end:
    decrement_270_118:
        mov r13, 10
        sub r13, 1
    decrement_270_118_end:
    increment_270_108:
        mov r14, r13
        add r14, 1
    increment_270_108_end:
    add_270_85:
        mov qword [rsp - 476], r15
        add qword [rsp - 476], r14
    add_270_85_end:
    mov qword [rsp - 504], 0
    mov qword [rsp - 496], 0
    mov dword [rsp - 488], 16711935
    mov rax, qword [rsp - 484]
    mov qword [rsp - 504], rax
    mov rax, qword [rsp - 476]
    mov qword [rsp - 496], rax
    cmp_273_12:
    cmp qword [rsp - 504], 6
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_15_8_273_5:
        cmp_15_8_273_5:
        cmp r15b, 0
        jne if_15_5_273_5_end
        if_15_8_273_5_code:
            mov rdi, 1
            exit_15_17_273_5:
                    mov rax, 60
                syscall
            exit_15_17_273_5_end:
        if_15_5_273_5_end:
    assert_273_5_end:
    cmp_274_12:
    cmp qword [rsp - 496], 40
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_15_8_274_5:
        cmp_15_8_274_5:
        cmp r15b, 0
        jne if_15_5_274_5_end
        if_15_8_274_5_code:
            mov rdi, 1
            exit_15_17_274_5:
                    mov rax, 60
                syscall
            exit_15_17_274_5_end:
        if_15_5_274_5_end:
    assert_274_5_end:
    double_279_36:
        mov r13, 4
        imul r13, 2
    double_279_36_end:
    triple_279_47:
        mov r12, 2
        imul r12, 3
    triple_279_47_end:
    max_279_32:
        if_67_8_279_32:
        cmp_67_8_279_32:
        cmp r13, r12
        jle if_else_67_5_279_32
        if_67_8_279_32_code:
            mov r14, r13
        jmp if_67_5_279_32_end
        if_else_67_5_279_32:
            mov r14, r12
        if_67_5_279_32_end:
    max_279_32_end:
    halve_279_66:
        mov r12, 10
        mov rax, r12
        cqo
        mov r11, 2
        idiv r11
        mov r12, rax
    halve_279_66_end:
    min_279_59:
        if_75_8_279_59:
        cmp_75_8_279_59:
            mov r11, 3
        cmp r11, r12
        jge if_else_75_5_279_59
        if_75_8_279_59_code:
            mov r13, 3
        jmp if_75_5_279_59_end
        if_else_75_5_279_59:
            mov r13, r12
        if_75_5_279_59_end:
    min_279_59_end:
    multiply_279_23:
        mov r15, r14
        imul r15, r13
    multiply_279_23_end:
    square_279_90:
        mov r12, 2
        imul r12, 2
    square_279_90_end:
    negate_279_83:
        mov r13, r12
        neg r13
    negate_279_83_end:
    abs_279_79:
        if_83_8_279_79:
        cmp_83_8_279_79:
        cmp r13, 0
        jge if_else_83_5_279_79
        if_83_8_279_79_code:
            mov r14, r13
            neg r14
        jmp if_83_5_279_79_end
        if_else_83_5_279_79:
            mov r14, r13
        if_83_5_279_79_end:
    abs_279_79_end:
    add_279_19:
        mov qword [rsp - 512], r15
        add qword [rsp - 512], r14
    add_279_19_end:
    increment_280_33:
        mov r14, 2
        add r14, 1
    increment_280_33_end:
    cube_280_28:
        mov r15, r14
        imul r15, r14
        imul r15, r14
    cube_280_28_end:
    double_280_63:
        mov r12, 6
        imul r12, 2
    double_280_63_end:
    halve_280_57:
        mov r13, r12
        mov rax, r13
        cqo
        mov r11, 2
        idiv r11
        mov r13, rax
    halve_280_57_end:
    triple_280_85:
        mov r11, 2
        imul r11, 3
    triple_280_85_end:
    decrement_280_75:
        mov r12, r11
        sub r12, 1
    decrement_280_75_end:
    multiply_280_48:
        mov r14, r13
        imul r14, r12
    multiply_280_48_end:
    subtract_280_19:
        mov qword [rsp - 520], r15
        sub qword [rsp - 520], r14
    subtract_280_19_end:
    mov r15, qword [rsp - 512]
    mov qword [rsp - 536], r15
    mov r15, qword [rsp - 520]
    mov qword [rsp - 528], r15
    cmp_282_12:
    cmp qword [rsp - 536], 28
    sete r15b
    bool_end_282_12:
    assert_282_5:
        if_15_8_282_5:
        cmp_15_8_282_5:
        cmp r15b, 0
        jne if_15_5_282_5_end
        if_15_8_282_5_code:
            mov rdi, 1
            exit_15_17_282_5:
                    mov rax, 60
                syscall
            exit_15_17_282_5_end:
        if_15_5_282_5_end:
    assert_282_5_end:
    cmp_283_12:
    cmp qword [rsp - 528], -3
    sete r15b
    bool_end_283_12:
    assert_283_5:
        if_15_8_283_5:
        cmp_15_8_283_5:
        cmp r15b, 0
        jne if_15_5_283_5_end
        if_15_8_283_5_code:
            mov rdi, 1
            exit_15_17_283_5:
                    mov rax, 60
                syscall
            exit_15_17_283_5_end:
        if_15_5_283_5_end:
    assert_283_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
section .rodata
dat:
dat.len equ $ - dat
