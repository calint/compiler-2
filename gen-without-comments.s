DEFAULT REL
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
test_passed: db 'test passed', 10,''
test_passed.len equ $ - test_passed
test_failed: db 'test failed', 10,''
test_failed.len equ $ - test_failed
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 3600]
    mov rcx, 3600
    xor rax, rax
    rep stosb
    mov dword [rsp - 3612], 5
    mov dword [rsp - 3608], 10
    mov dword [rsp - 3604], 15
    mov qword [rsp - 3620], 0
    loop_94_5:
        mov qword [rsp - 3628], 0
        loop_96_9:
            lea r15, [rsp - 3600]
            mov r14, 0
            cmp r14, 2
            mov r13, 97
            cmovge rbp, r13
            jge panic_bounds
            imul r14, 1800
            add r15, r14
            mov r14, 0
            cmp r14, 3
            mov r13, 97
            cmovge rbp, r13
            jge panic_bounds
            imul r14, 600
            add r15, r14
            mov r14, qword [rsp - 3628]
            cmp r14, 4
            mov r13, 97
            cmovge rbp, r13
            jge panic_bounds
            imul r14, 148
            add r15, r14
            init_mesh_97_13:
                mov r14d, dword [rsp - 3628]
                mov dword [r15 + 144], r14d
                mov qword [rsp - 3636], 0
                loop_43_5_97_13:
                    mov r14, r15
                    mov r13, qword [rsp - 3636]
                    cmp r13, 8
                    mov r12, 44
                    cmovge rbp, r12
                    jge panic_bounds
                    imul r13, 12
                    add r14, r13
                    mov r13d, dword [rsp - 3636]
                    mov dword [r14], r13d
                    movsx r13, dword [r14]
                    imul r13, 10
                    mov dword [r14], r13d
                    mov r14, r15
                    mov r13, qword [rsp - 3636]
                    cmp r13, 8
                    mov r12, 45
                    cmovge rbp, r12
                    jge panic_bounds
                    imul r13, 12
                    add r14, r13
                    add r14, 4
                    mov r13d, dword [rsp - 3636]
                    mov dword [r14], r13d
                    movsx r13, dword [r14]
                    imul r13, 20
                    mov dword [r14], r13d
                    mov r14, r15
                    mov r13, qword [rsp - 3636]
                    cmp r13, 8
                    mov r12, 46
                    cmovge rbp, r12
                    jge panic_bounds
                    imul r13, 12
                    add r14, r13
                    add r14, 8
                    mov r13d, dword [rsp - 3636]
                    mov dword [r14], r13d
                    movsx r13, dword [r14]
                    imul r13, 30
                    mov dword [r14], r13d
                    add qword [rsp - 3636], 1
                    if_48_12_97_13:
                    cmp_48_12_97_13:
                        mov r14, qword [rsp - 3636]
                        mov r13, 8
                    cmp r14, r13
                    jne if_48_9_97_13_end
                    jmp if_48_12_97_13_code
                    if_48_12_97_13_code:
                        jmp loop_43_5_97_13_end
                    if_48_9_97_13_end:
                jmp loop_43_5_97_13
                loop_43_5_97_13_end:
                mov qword [rsp - 3636], 0
                loop_52_5_97_13:
                    mov r14, r15
                    mov r13, qword [rsp - 3636]
                    cmp r13, 12
                    mov r12, 53
                    cmovge rbp, r12
                    jge panic_bounds
                    mov r12d, dword [rsp - 3636]
                    mov dword [r14 + r13 * 4 + 96], r12d
                    add qword [rsp - 3636], 1
                    if_55_12_97_13:
                    cmp_55_12_97_13:
                        mov r14, qword [rsp - 3636]
                        mov r13, 12
                    cmp r14, r13
                    jne if_55_9_97_13_end
                    jmp if_55_12_97_13_code
                    if_55_12_97_13_code:
                        jmp loop_52_5_97_13_end
                    if_55_9_97_13_end:
                jmp loop_52_5_97_13
                loop_52_5_97_13_end:
            init_mesh_97_13_end:
            add qword [rsp - 3628], 1
            if_99_16:
            cmp_99_16:
                mov r15, qword [rsp - 3628]
                mov r14, 4
            cmp r15, r14
            jne if_99_13_end
            jmp if_99_16_code
            if_99_16_code:
                jmp loop_96_9_end
            if_99_13_end:
        jmp loop_96_9
        loop_96_9_end:
        add qword [rsp - 3620], 1
        if_102_12:
        cmp_102_12:
            mov r15, qword [rsp - 3620]
            mov r14, 1
        cmp r15, r14
        jne if_102_9_end
        jmp if_102_12_code
        if_102_12_code:
            jmp loop_94_5_end
        if_102_9_end:
    jmp loop_94_5
    loop_94_5_end:
    lea r15, [rsp - 3600]
    mov r14, 0
    cmp r14, 2
    mov r13, 107
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 0
    cmp r14, 3
    mov r13, 107
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    mov r14, 3
    cmp r14, 4
    mov r13, 107
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 148
    add r15, r14
    verify_mesh_107_5:
        cmp_60_12_107_5:
            mov r12, r15
            add r12, 144
            movsx r13, dword [r12]
            mov r12, 3
        cmp r13, r12
        jne bool_false_60_12_107_5
        jmp bool_true_60_12_107_5
        bool_true_60_12_107_5:
        mov r14, true
        jmp bool_end_60_12_107_5
        bool_false_60_12_107_5:
        mov r14, false
        bool_end_60_12_107_5:
        assert_60_5_107_5:
            if_11_8_60_5_107_5:
            cmp_11_8_60_5_107_5:
                mov r13, r14
            test r13, r13
            jne if_11_5_60_5_107_5_end
            jmp if_11_8_60_5_107_5_code
            if_11_8_60_5_107_5_code:
                mov rdi, 1
                exit_11_17_60_5_107_5:
                    mov rax, 60
                    syscall
                exit_11_17_60_5_107_5_end:
            if_11_5_60_5_107_5_end:
        assert_60_5_107_5_end:
        mov qword [rsp - 3628], 0
        loop_62_5_107_5:
            cmp_63_16_107_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                cmp r11, 8
                mov r10, 63
                cmovge rbp, r10
                jge panic_bounds
                imul r11, 12
                add r12, r11
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 10
            cmp r13, r12
            jne bool_false_63_16_107_5
            jmp bool_true_63_16_107_5
            bool_true_63_16_107_5:
            mov r14, true
            jmp bool_end_63_16_107_5
            bool_false_63_16_107_5:
            mov r14, false
            bool_end_63_16_107_5:
            assert_63_9_107_5:
                if_11_8_63_9_107_5:
                cmp_11_8_63_9_107_5:
                    mov r13, r14
                test r13, r13
                jne if_11_5_63_9_107_5_end
                jmp if_11_8_63_9_107_5_code
                if_11_8_63_9_107_5_code:
                    mov rdi, 1
                    exit_11_17_63_9_107_5:
                        mov rax, 60
                        syscall
                    exit_11_17_63_9_107_5_end:
                if_11_5_63_9_107_5_end:
            assert_63_9_107_5_end:
            cmp_64_16_107_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                cmp r11, 8
                mov r10, 64
                cmovge rbp, r10
                jge panic_bounds
                imul r11, 12
                add r12, r11
                add r12, 4
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 20
            cmp r13, r12
            jne bool_false_64_16_107_5
            jmp bool_true_64_16_107_5
            bool_true_64_16_107_5:
            mov r14, true
            jmp bool_end_64_16_107_5
            bool_false_64_16_107_5:
            mov r14, false
            bool_end_64_16_107_5:
            assert_64_9_107_5:
                if_11_8_64_9_107_5:
                cmp_11_8_64_9_107_5:
                    mov r13, r14
                test r13, r13
                jne if_11_5_64_9_107_5_end
                jmp if_11_8_64_9_107_5_code
                if_11_8_64_9_107_5_code:
                    mov rdi, 1
                    exit_11_17_64_9_107_5:
                        mov rax, 60
                        syscall
                    exit_11_17_64_9_107_5_end:
                if_11_5_64_9_107_5_end:
            assert_64_9_107_5_end:
            cmp_65_16_107_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                cmp r11, 8
                mov r10, 65
                cmovge rbp, r10
                jge panic_bounds
                imul r11, 12
                add r12, r11
                add r12, 8
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 30
            cmp r13, r12
            jne bool_false_65_16_107_5
            jmp bool_true_65_16_107_5
            bool_true_65_16_107_5:
            mov r14, true
            jmp bool_end_65_16_107_5
            bool_false_65_16_107_5:
            mov r14, false
            bool_end_65_16_107_5:
            assert_65_9_107_5:
                if_11_8_65_9_107_5:
                cmp_11_8_65_9_107_5:
                    mov r13, r14
                test r13, r13
                jne if_11_5_65_9_107_5_end
                jmp if_11_8_65_9_107_5_code
                if_11_8_65_9_107_5_code:
                    mov rdi, 1
                    exit_11_17_65_9_107_5:
                        mov rax, 60
                        syscall
                    exit_11_17_65_9_107_5_end:
                if_11_5_65_9_107_5_end:
            assert_65_9_107_5_end:
            add qword [rsp - 3628], 1
            if_67_12_107_5:
            cmp_67_12_107_5:
                mov r14, qword [rsp - 3628]
                mov r13, 8
            cmp r14, r13
            jne if_67_9_107_5_end
            jmp if_67_12_107_5_code
            if_67_12_107_5_code:
                jmp loop_62_5_107_5_end
            if_67_9_107_5_end:
        jmp loop_62_5_107_5
        loop_62_5_107_5_end:
    verify_mesh_107_5_end:
    lea r15, [rsp - 3600]
    mov r14, 0
    cmp r14, 2
    mov r13, 110
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 1
    cmp r14, 3
    mov r13, 110
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    mov r14, 2
    cmp r14, 4
    mov r13, 110
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 148
    add r15, r14
    mov r14, 5
    cmp r14, 8
    mov r13, 110
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 12
    add r15, r14
    mov dword [r15], 0xff
    lea r15, [rsp - 3600]
    mov r14, 0
    cmp r14, 2
    mov r13, 111
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 1
    cmp r14, 3
    mov r13, 111
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    mov r14, 2
    cmp r14, 4
    mov r13, 111
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 148
    add r15, r14
    mov r14, 5
    cmp r14, 8
    mov r13, 111
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 12
    add r15, r14
    add r15, 4
    mov dword [r15], 0xaa
    lea r15, [rsp - 3600]
    mov r14, 0
    cmp r14, 2
    mov r13, 112
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 1
    cmp r14, 3
    mov r13, 112
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    mov r14, 2
    cmp r14, 4
    mov r13, 112
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 148
    add r15, r14
    mov r14, 5
    cmp r14, 8
    mov r13, 112
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 12
    add r15, r14
    add r15, 8
    mov dword [r15], 0xbb
    cmp_113_12:
        lea r13, [rsp - 3600]
        mov r12, 0
        cmp r12, 2
        mov r11, 113
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 1800
        add r13, r12
        mov r12, 1
        cmp r12, 3
        mov r11, 113
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 600
        add r13, r12
        mov r12, 2
        cmp r12, 4
        mov r11, 113
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 148
        add r13, r12
        mov r12, 5
        cmp r12, 8
        mov r11, 113
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 12
        add r13, r12
        movsx r14, dword [r13]
        mov r13, 0xff
    cmp r14, r13
    jne bool_false_113_12
    jmp bool_true_113_12
    bool_true_113_12:
    mov r15, true
    jmp bool_end_113_12
    bool_false_113_12:
    mov r15, false
    bool_end_113_12:
    assert_113_5:
        if_11_8_113_5:
        cmp_11_8_113_5:
            mov r14, r15
        test r14, r14
        jne if_11_5_113_5_end
        jmp if_11_8_113_5_code
        if_11_8_113_5_code:
            mov rdi, 1
            exit_11_17_113_5:
                mov rax, 60
                syscall
            exit_11_17_113_5_end:
        if_11_5_113_5_end:
    assert_113_5_end:
    cmp_114_12:
        lea r13, [rsp - 3600]
        mov r12, 0
        cmp r12, 2
        mov r11, 114
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 1800
        add r13, r12
        mov r12, 1
        cmp r12, 3
        mov r11, 114
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 600
        add r13, r12
        mov r12, 2
        cmp r12, 4
        mov r11, 114
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 148
        add r13, r12
        mov r12, 5
        cmp r12, 8
        mov r11, 114
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 12
        add r13, r12
        add r13, 4
        movsx r14, dword [r13]
        mov r13, 0xaa
    cmp r14, r13
    jne bool_false_114_12
    jmp bool_true_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
    assert_114_5:
        if_11_8_114_5:
        cmp_11_8_114_5:
            mov r14, r15
        test r14, r14
        jne if_11_5_114_5_end
        jmp if_11_8_114_5_code
        if_11_8_114_5_code:
            mov rdi, 1
            exit_11_17_114_5:
                mov rax, 60
                syscall
            exit_11_17_114_5_end:
        if_11_5_114_5_end:
    assert_114_5_end:
    cmp_115_12:
        lea r13, [rsp - 3600]
        mov r12, 0
        cmp r12, 2
        mov r11, 115
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 1800
        add r13, r12
        mov r12, 1
        cmp r12, 3
        mov r11, 115
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 600
        add r13, r12
        mov r12, 2
        cmp r12, 4
        mov r11, 115
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 148
        add r13, r12
        mov r12, 5
        cmp r12, 8
        mov r11, 115
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 12
        add r13, r12
        add r13, 8
        movsx r14, dword [r13]
        mov r13, 0xbb
    cmp r14, r13
    jne bool_false_115_12
    jmp bool_true_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_11_8_115_5:
        cmp_11_8_115_5:
            mov r14, r15
        test r14, r14
        jne if_11_5_115_5_end
        jmp if_11_8_115_5_code
        if_11_8_115_5_code:
            mov rdi, 1
            exit_11_17_115_5:
                mov rax, 60
                syscall
            exit_11_17_115_5_end:
        if_11_5_115_5_end:
    assert_115_5_end:
    mov rcx, 4
    lea r15, [rsp - 3600]
    mov r14, 0
    cmp r14, 2
    mov r13, 119
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 0
    cmp r14, 3
    mov r13, 119
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    cmp rcx, 4
    mov r14, 119
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 3600]
    mov r14, 1
    cmp r14, 2
    mov r13, 120
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 2
    cmp r14, 3
    mov r13, 120
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    cmp rcx, 4
    mov r14, 120
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    imul rcx, 148
    rep movsb
    mov qword [rsp - 3628], 0
    loop_126_5:
        cmp_127_16:
            lea r13, [rsp - 3600]
            mov r12, 1
            cmp r12, 2
            mov r11, 127
            cmovge rbp, r11
            jge panic_bounds
            imul r12, 1800
            add r13, r12
            mov r12, 2
            cmp r12, 3
            mov r11, 127
            cmovge rbp, r11
            jge panic_bounds
            imul r12, 600
            add r13, r12
            mov r12, qword [rsp - 3628]
            cmp r12, 4
            mov r11, 127
            cmovge rbp, r11
            jge panic_bounds
            imul r12, 148
            add r13, r12
            add r13, 144
            movsx r14, dword [r13]
            mov r13, qword [rsp - 3628]
        cmp r14, r13
        jne bool_false_127_16
        jmp bool_true_127_16
        bool_true_127_16:
        mov r15, true
        jmp bool_end_127_16
        bool_false_127_16:
        mov r15, false
        bool_end_127_16:
        assert_127_9:
            if_11_8_127_9:
            cmp_11_8_127_9:
                mov r14, r15
            test r14, r14
            jne if_11_5_127_9_end
            jmp if_11_8_127_9_code
            if_11_8_127_9_code:
                mov rdi, 1
                exit_11_17_127_9:
                    mov rax, 60
                    syscall
                exit_11_17_127_9_end:
            if_11_5_127_9_end:
        assert_127_9_end:
        add qword [rsp - 3628], 1
        if_129_12:
        cmp_129_12:
            mov r15, qword [rsp - 3628]
            mov r14, 4
        cmp r15, r14
        jne if_129_9_end
        jmp if_129_12_code
        if_129_12_code:
            jmp loop_126_5_end
        if_129_9_end:
    jmp loop_126_5
    loop_126_5_end:
    cmp_133_21:
        mov rcx, 8
        lea r14, [rsp - 3600]
        mov r13, 0
        cmp r13, 2
        mov r12, 134
        cmovge rbp, r12
        jge panic_bounds
        imul r13, 1800
        add r14, r13
        mov r13, 0
        cmp r13, 3
        mov r12, 134
        cmovge rbp, r12
        jge panic_bounds
        imul r13, 600
        add r14, r13
        mov r13, 1
        cmp r13, 4
        mov r12, 134
        cmovge rbp, r12
        jge panic_bounds
        imul r13, 148
        add r14, r13
        cmp rcx, 8
        mov r13, 134
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [r14]
        lea r14, [rsp - 3600]
        mov r13, 1
        cmp r13, 2
        mov r12, 135
        cmovge rbp, r12
        jge panic_bounds
        imul r13, 1800
        add r14, r13
        mov r13, 2
        cmp r13, 3
        mov r12, 135
        cmovge rbp, r12
        jge panic_bounds
        imul r13, 600
        add r14, r13
        mov r13, 1
        cmp r13, 4
        mov r12, 135
        cmovge rbp, r12
        jge panic_bounds
        imul r13, 148
        add r14, r13
        cmp rcx, 8
        mov r13, 135
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [r14]
        imul rcx, 12
        repe cmpsb
        je cmps_eq_133_21
        mov r15, false
        jmp cmps_end_133_21
        cmps_eq_133_21:
        mov r15, true
        cmps_end_133_21:
    test r15, r15
    je bool_false_133_21
    jmp bool_true_133_21
    bool_true_133_21:
    mov byte [rsp - 3629], true
    jmp bool_end_133_21
    bool_false_133_21:
    mov byte [rsp - 3629], false
    bool_end_133_21:
    cmp_138_12:
        movsx r14, byte [rsp - 3629]
    test r14, r14
    je bool_false_138_12
    jmp bool_true_138_12
    bool_true_138_12:
    mov r15, true
    jmp bool_end_138_12
    bool_false_138_12:
    mov r15, false
    bool_end_138_12:
    assert_138_5:
        if_11_8_138_5:
        cmp_11_8_138_5:
            mov r14, r15
        test r14, r14
        jne if_11_5_138_5_end
        jmp if_11_8_138_5_code
        if_11_8_138_5_code:
            mov rdi, 1
            exit_11_17_138_5:
                mov rax, 60
                syscall
            exit_11_17_138_5_end:
        if_11_5_138_5_end:
    assert_138_5_end:
    mov rcx, 6
    lea r15, [rsp - 3600]
    mov r14, 0
    cmp r14, 2
    mov r13, 142
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 0
    cmp r14, 3
    mov r13, 142
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    mov r14, 2
    cmp r14, 4
    mov r13, 142
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 148
    add r15, r14
    cmp rcx, 12
    mov r14, 142
    cmovg rbp, r14
    jg panic_bounds
    add r15, 96
    lea rsi, [r15]
    lea r15, [rsp - 3600]
    mov r14, 0
    cmp r14, 2
    mov r13, 143
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 1
    cmp r14, 3
    mov r13, 143
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    mov r14, 1
    cmp r14, 4
    mov r13, 143
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 148
    add r15, r14
    cmp rcx, 12
    mov r14, 143
    cmovg rbp, r14
    jg panic_bounds
    add r15, 96
    lea rdi, [r15]
    shl rcx, 2
    rep movsb
    mov qword [rsp - 3637], 0
    loop_148_5:
        cmp_149_16:
            lea r13, [rsp - 3600]
            mov r12, 0
            cmp r12, 2
            mov r11, 149
            cmovge rbp, r11
            jge panic_bounds
            imul r12, 1800
            add r13, r12
            mov r12, 1
            cmp r12, 3
            mov r11, 149
            cmovge rbp, r11
            jge panic_bounds
            imul r12, 600
            add r13, r12
            mov r12, 1
            cmp r12, 4
            mov r11, 149
            cmovge rbp, r11
            jge panic_bounds
            imul r12, 148
            add r13, r12
            mov r12, qword [rsp - 3637]
            cmp r12, 12
            mov r11, 149
            cmovge rbp, r11
            jge panic_bounds
            movsx r14, dword [r13 + r12 * 4 + 96]
            mov r13, qword [rsp - 3637]
        cmp r14, r13
        jne bool_false_149_16
        jmp bool_true_149_16
        bool_true_149_16:
        mov r15, true
        jmp bool_end_149_16
        bool_false_149_16:
        mov r15, false
        bool_end_149_16:
        assert_149_9:
            if_11_8_149_9:
            cmp_11_8_149_9:
                mov r14, r15
            test r14, r14
            jne if_11_5_149_9_end
            jmp if_11_8_149_9_code
            if_11_8_149_9_code:
                mov rdi, 1
                exit_11_17_149_9:
                    mov rax, 60
                    syscall
                exit_11_17_149_9_end:
            if_11_5_149_9_end:
        assert_149_9_end:
        add qword [rsp - 3637], 1
        if_151_12:
        cmp_151_12:
            mov r15, qword [rsp - 3637]
            mov r14, 6
        cmp r15, r14
        jne if_151_9_end
        jmp if_151_12_code
        if_151_12_code:
            jmp loop_148_5_end
        if_151_9_end:
    jmp loop_148_5
    loop_148_5_end:
    mov qword [rsp - 3645], 0
    loop_157_5:
        lea r15, [rsp - 3600]
        mov r14, 1
        cmp r14, 2
        mov r13, 158
        cmovge rbp, r13
        jge panic_bounds
        imul r14, 1800
        add r15, r14
        mov r14, 0
        cmp r14, 3
        mov r13, 158
        cmovge rbp, r13
        jge panic_bounds
        imul r14, 600
        add r15, r14
        mov r14, qword [rsp - 3645]
        cmp r14, 4
        mov r13, 158
        cmovge rbp, r13
        jge panic_bounds
        imul r14, 148
        add r15, r14
        mov r14, qword [rsp - 3645]
        add r14, 100
        init_mesh_158_9:
            mov dword [r15 + 144], r14d
            mov qword [rsp - 3653], 0
            loop_43_5_158_9:
                mov r13, r15
                mov r12, qword [rsp - 3653]
                cmp r12, 8
                mov r11, 44
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                mov r12d, dword [rsp - 3653]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 10
                mov dword [r13], r12d
                mov r13, r15
                mov r12, qword [rsp - 3653]
                cmp r12, 8
                mov r11, 45
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                add r13, 4
                mov r12d, dword [rsp - 3653]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 20
                mov dword [r13], r12d
                mov r13, r15
                mov r12, qword [rsp - 3653]
                cmp r12, 8
                mov r11, 46
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                add r13, 8
                mov r12d, dword [rsp - 3653]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 30
                mov dword [r13], r12d
                add qword [rsp - 3653], 1
                if_48_12_158_9:
                cmp_48_12_158_9:
                    mov r13, qword [rsp - 3653]
                    mov r12, 8
                cmp r13, r12
                jne if_48_9_158_9_end
                jmp if_48_12_158_9_code
                if_48_12_158_9_code:
                    jmp loop_43_5_158_9_end
                if_48_9_158_9_end:
            jmp loop_43_5_158_9
            loop_43_5_158_9_end:
            mov qword [rsp - 3653], 0
            loop_52_5_158_9:
                mov r13, r15
                mov r12, qword [rsp - 3653]
                cmp r12, 12
                mov r11, 53
                cmovge rbp, r11
                jge panic_bounds
                mov r11d, dword [rsp - 3653]
                mov dword [r13 + r12 * 4 + 96], r11d
                add qword [rsp - 3653], 1
                if_55_12_158_9:
                cmp_55_12_158_9:
                    mov r13, qword [rsp - 3653]
                    mov r12, 12
                cmp r13, r12
                jne if_55_9_158_9_end
                jmp if_55_12_158_9_code
                if_55_12_158_9_code:
                    jmp loop_52_5_158_9_end
                if_55_9_158_9_end:
            jmp loop_52_5_158_9
            loop_52_5_158_9_end:
        init_mesh_158_9_end:
        add qword [rsp - 3645], 1
        if_160_12:
        cmp_160_12:
            mov r15, qword [rsp - 3645]
            mov r14, 4
        cmp r15, r14
        jne if_160_9_end
        jmp if_160_12_code
        if_160_12_code:
            jmp loop_157_5_end
        if_160_9_end:
    jmp loop_157_5
    loop_157_5_end:
    lea r15, [rsp - 3600]
    mov r14, 1
    cmp r14, 2
    mov r13, 163
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 1800
    add r15, r14
    mov r14, 0
    cmp r14, 3
    mov r13, 163
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 600
    add r15, r14
    transform_vertices_163_5:
        mov qword [rsp - 3653], 0
        loop_73_5_163_5:
            mov qword [rsp - 3661], 0
            loop_75_9_163_5:
                mov r14, r15
                mov r13, qword [rsp - 3653]
                cmp r13, 4
                mov r12, 76
                cmovge rbp, r12
                jge panic_bounds
                imul r13, 148
                add r14, r13
                mov r13, qword [rsp - 3661]
                cmp r13, 8
                mov r12, 76
                cmovge rbp, r12
                jge panic_bounds
                imul r13, 12
                add r14, r13
                mov r13, r15
                mov r12, qword [rsp - 3653]
                cmp r12, 4
                mov r11, 76
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 148
                add r13, r12
                mov r12, qword [rsp - 3661]
                cmp r12, 8
                mov r11, 76
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                mov r12d, dword [r13]
                mov dword [r14], r12d
                mov r13d, dword [rsp - 3612]
                add dword [r14], r13d
                mov r14, r15
                mov r13, qword [rsp - 3653]
                cmp r13, 4
                mov r12, 77
                cmovge rbp, r12
                jge panic_bounds
                imul r13, 148
                add r14, r13
                mov r13, qword [rsp - 3661]
                cmp r13, 8
                mov r12, 77
                cmovge rbp, r12
                jge panic_bounds
                imul r13, 12
                add r14, r13
                add r14, 4
                mov r13, r15
                mov r12, qword [rsp - 3653]
                cmp r12, 4
                mov r11, 77
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 148
                add r13, r12
                mov r12, qword [rsp - 3661]
                cmp r12, 8
                mov r11, 77
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                add r13, 4
                mov r12d, dword [r13]
                mov dword [r14], r12d
                mov r13d, dword [rsp - 3608]
                add dword [r14], r13d
                mov r14, r15
                mov r13, qword [rsp - 3653]
                cmp r13, 4
                mov r12, 78
                cmovge rbp, r12
                jge panic_bounds
                imul r13, 148
                add r14, r13
                mov r13, qword [rsp - 3661]
                cmp r13, 8
                mov r12, 78
                cmovge rbp, r12
                jge panic_bounds
                imul r13, 12
                add r14, r13
                add r14, 8
                mov r13, r15
                mov r12, qword [rsp - 3653]
                cmp r12, 4
                mov r11, 78
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 148
                add r13, r12
                mov r12, qword [rsp - 3661]
                cmp r12, 8
                mov r11, 78
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                add r13, 8
                mov r12d, dword [r13]
                mov dword [r14], r12d
                mov r13d, dword [rsp - 3604]
                add dword [r14], r13d
                add qword [rsp - 3661], 1
                if_80_16_163_5:
                cmp_80_16_163_5:
                    mov r14, qword [rsp - 3661]
                    mov r13, 8
                cmp r14, r13
                jne if_80_13_163_5_end
                jmp if_80_16_163_5_code
                if_80_16_163_5_code:
                    jmp loop_75_9_163_5_end
                if_80_13_163_5_end:
            jmp loop_75_9_163_5
            loop_75_9_163_5_end:
            add qword [rsp - 3653], 1
            if_83_12_163_5:
            cmp_83_12_163_5:
                mov r14, qword [rsp - 3653]
                mov r13, 4
            cmp r14, r13
            jne if_83_9_163_5_end
            jmp if_83_12_163_5_code
            if_83_12_163_5_code:
                jmp loop_73_5_163_5_end
            if_83_9_163_5_end:
        jmp loop_73_5_163_5
        loop_73_5_163_5_end:
    transform_vertices_163_5_end:
    cmp_164_12:
        lea r13, [rsp - 3600]
        mov r12, 1
        cmp r12, 2
        mov r11, 164
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 1800
        add r13, r12
        mov r12, 0
        cmp r12, 3
        mov r11, 164
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 600
        add r13, r12
        mov r12, 0
        cmp r12, 4
        mov r11, 164
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 148
        add r13, r12
        mov r12, 0
        cmp r12, 8
        mov r11, 164
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 12
        add r13, r12
        movsx r14, dword [r13]
        mov r13, 0
        add r13, 5
    cmp r14, r13
    jne bool_false_164_12
    jmp bool_true_164_12
    bool_true_164_12:
    mov r15, true
    jmp bool_end_164_12
    bool_false_164_12:
    mov r15, false
    bool_end_164_12:
    assert_164_5:
        if_11_8_164_5:
        cmp_11_8_164_5:
            mov r14, r15
        test r14, r14
        jne if_11_5_164_5_end
        jmp if_11_8_164_5_code
        if_11_8_164_5_code:
            mov rdi, 1
            exit_11_17_164_5:
                mov rax, 60
                syscall
            exit_11_17_164_5_end:
        if_11_5_164_5_end:
    assert_164_5_end:
    cmp_165_12:
        lea r13, [rsp - 3600]
        mov r12, 1
        cmp r12, 2
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 1800
        add r13, r12
        mov r12, 0
        cmp r12, 3
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 600
        add r13, r12
        mov r12, 1
        cmp r12, 4
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 148
        add r13, r12
        mov r12, 3
        cmp r12, 8
        mov r11, 165
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 12
        add r13, r12
        add r13, 4
        movsx r14, dword [r13]
        mov r13, 60
        add r13, 10
    cmp r14, r13
    jne bool_false_165_12
    jmp bool_true_165_12
    bool_true_165_12:
    mov r15, true
    jmp bool_end_165_12
    bool_false_165_12:
    mov r15, false
    bool_end_165_12:
    assert_165_5:
        if_11_8_165_5:
        cmp_11_8_165_5:
            mov r14, r15
        test r14, r14
        jne if_11_5_165_5_end
        jmp if_11_8_165_5_code
        if_11_8_165_5_code:
            mov rdi, 1
            exit_11_17_165_5:
                mov rax, 60
                syscall
            exit_11_17_165_5_end:
        if_11_5_165_5_end:
    assert_165_5_end:
    cmp_166_12:
        lea r13, [rsp - 3600]
        mov r12, 1
        cmp r12, 2
        mov r11, 166
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 1800
        add r13, r12
        mov r12, 0
        cmp r12, 3
        mov r11, 166
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 600
        add r13, r12
        mov r12, 2
        cmp r12, 4
        mov r11, 166
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 148
        add r13, r12
        mov r12, 7
        cmp r12, 8
        mov r11, 166
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 12
        add r13, r12
        add r13, 8
        movsx r14, dword [r13]
        mov r13, 210
        add r13, 15
    cmp r14, r13
    jne bool_false_166_12
    jmp bool_true_166_12
    bool_true_166_12:
    mov r15, true
    jmp bool_end_166_12
    bool_false_166_12:
    mov r15, false
    bool_end_166_12:
    assert_166_5:
        if_11_8_166_5:
        cmp_11_8_166_5:
            mov r14, r15
        test r14, r14
        jne if_11_5_166_5_end
        jmp if_11_8_166_5_code
        if_11_8_166_5_code:
            mov rdi, 1
            exit_11_17_166_5:
                mov rax, 60
                syscall
            exit_11_17_166_5_end:
        if_11_5_166_5_end:
    assert_166_5_end:
    lea rdi, [rsp - 4237]
    mov rcx, 592
    xor rax, rax
    rep stosb
    mov qword [rsp - 4245], 0
    loop_171_5:
        lea r15, [rsp - 4237]
        mov r14, qword [rsp - 4245]
        cmp r14, 4
        mov r13, 172
        cmovge rbp, r13
        jge panic_bounds
        imul r14, 148
        add r15, r14
        mov r14, qword [rsp - 4245]
        imul r14, 100
        init_mesh_172_9:
            mov dword [r15 + 144], r14d
            mov qword [rsp - 4253], 0
            loop_43_5_172_9:
                mov r13, r15
                mov r12, qword [rsp - 4253]
                cmp r12, 8
                mov r11, 44
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                mov r12d, dword [rsp - 4253]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 10
                mov dword [r13], r12d
                mov r13, r15
                mov r12, qword [rsp - 4253]
                cmp r12, 8
                mov r11, 45
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                add r13, 4
                mov r12d, dword [rsp - 4253]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 20
                mov dword [r13], r12d
                mov r13, r15
                mov r12, qword [rsp - 4253]
                cmp r12, 8
                mov r11, 46
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 12
                add r13, r12
                add r13, 8
                mov r12d, dword [rsp - 4253]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 30
                mov dword [r13], r12d
                add qword [rsp - 4253], 1
                if_48_12_172_9:
                cmp_48_12_172_9:
                    mov r13, qword [rsp - 4253]
                    mov r12, 8
                cmp r13, r12
                jne if_48_9_172_9_end
                jmp if_48_12_172_9_code
                if_48_12_172_9_code:
                    jmp loop_43_5_172_9_end
                if_48_9_172_9_end:
            jmp loop_43_5_172_9
            loop_43_5_172_9_end:
            mov qword [rsp - 4253], 0
            loop_52_5_172_9:
                mov r13, r15
                mov r12, qword [rsp - 4253]
                cmp r12, 12
                mov r11, 53
                cmovge rbp, r11
                jge panic_bounds
                mov r11d, dword [rsp - 4253]
                mov dword [r13 + r12 * 4 + 96], r11d
                add qword [rsp - 4253], 1
                if_55_12_172_9:
                cmp_55_12_172_9:
                    mov r13, qword [rsp - 4253]
                    mov r12, 12
                cmp r13, r12
                jne if_55_9_172_9_end
                jmp if_55_12_172_9_code
                if_55_12_172_9_code:
                    jmp loop_52_5_172_9_end
                if_55_9_172_9_end:
            jmp loop_52_5_172_9
            loop_52_5_172_9_end:
        init_mesh_172_9_end:
        add qword [rsp - 4245], 1
        if_174_12:
        cmp_174_12:
            mov r15, qword [rsp - 4245]
            mov r14, 4
        cmp r15, r14
        jne if_174_9_end
        jmp if_174_12_code
        if_174_12_code:
            jmp loop_171_5_end
        if_174_9_end:
    jmp loop_171_5
    loop_171_5_end:
    mov qword [rsp - 4253], 0
    loop_179_5:
        cmp_180_16:
            lea r13, [rsp - 4237]
            mov r12, qword [rsp - 4253]
            cmp r12, 4
            mov r11, 180
            cmovge rbp, r11
            jge panic_bounds
            imul r12, 148
            add r13, r12
            add r13, 144
            movsx r14, dword [r13]
            mov r13, qword [rsp - 4253]
            imul r13, 100
        cmp r14, r13
        jne bool_false_180_16
        jmp bool_true_180_16
        bool_true_180_16:
        mov r15, true
        jmp bool_end_180_16
        bool_false_180_16:
        mov r15, false
        bool_end_180_16:
        assert_180_9:
            if_11_8_180_9:
            cmp_11_8_180_9:
                mov r14, r15
            test r14, r14
            jne if_11_5_180_9_end
            jmp if_11_8_180_9_code
            if_11_8_180_9_code:
                mov rdi, 1
                exit_11_17_180_9:
                    mov rax, 60
                    syscall
                exit_11_17_180_9_end:
            if_11_5_180_9_end:
        assert_180_9_end:
        add qword [rsp - 4253], 1
        if_182_12:
        cmp_182_12:
            mov r15, qword [rsp - 4253]
            mov r14, 4
        cmp r15, r14
        jne if_182_9_end
        jmp if_182_12_code
        if_182_12_code:
            jmp loop_179_5_end
        if_182_9_end:
    jmp loop_179_5
    loop_179_5_end:
    mov rdx, test_passed.len
    mov rsi, test_passed
    print_185_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_185_5_end:
    mov rdi, 0
    exit_186_5:
        mov rax, 60
        syscall
    exit_186_5_end:
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
