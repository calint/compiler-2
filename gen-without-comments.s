DEFAULT REL
section .bss
stk resd 65536
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
    loop_97_5:
        mov qword [rsp - 3628], 0
        loop_99_9:
            lea r15, [rsp - 3600]
            mov r14, 0
            imul r14, 1800
            add r15, r14
            mov r14, 0
            imul r14, 600
            add r15, r14
            mov r14, qword [rsp - 3628]
            imul r14, 148
            add r15, r14
            init_mesh_100_13:
                mov r14d, dword [rsp - 3628]
                mov dword [r15 + 144], r14d
                jmp init_mesh_100_13_end
                mov qword [rsp - 3636], 0
                loop_45_5_100_13:
                    mov r14, r15
                    mov r13, qword [rsp - 3636]
                    imul r13, 12
                    add r14, r13
                    mov r13d, dword [rsp - 3636]
                    mov dword [r14], r13d
                    movsx r13, dword [r14]
                    imul r13, 10
                    mov dword [r14], r13d
                    mov r14, r15
                    mov r13, qword [rsp - 3636]
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
                    imul r13, 12
                    add r14, r13
                    add r14, 8
                    mov r13d, dword [rsp - 3636]
                    mov dword [r14], r13d
                    movsx r13, dword [r14]
                    imul r13, 30
                    mov dword [r14], r13d
                    add qword [rsp - 3636], 1
                    if_50_12_100_13:
                    cmp_50_12_100_13:
                    cmp qword [rsp - 3636], 8
                    jne if_50_9_100_13_end
                    jmp if_50_12_100_13_code
                    if_50_12_100_13_code:
                        jmp loop_45_5_100_13_end
                    if_50_9_100_13_end:
                jmp loop_45_5_100_13
                loop_45_5_100_13_end:
                mov qword [rsp - 3636], 0
                loop_54_5_100_13:
                    mov r14, r15
                    mov r13, qword [rsp - 3636]
                    mov r12d, dword [rsp - 3636]
                    mov dword [r14 + r13 * 4 + 96], r12d
                    add qword [rsp - 3636], 1
                    if_57_12_100_13:
                    cmp_57_12_100_13:
                    cmp qword [rsp - 3636], 12
                    jne if_57_9_100_13_end
                    jmp if_57_12_100_13_code
                    if_57_12_100_13_code:
                        jmp loop_54_5_100_13_end
                    if_57_9_100_13_end:
                jmp loop_54_5_100_13
                loop_54_5_100_13_end:
            init_mesh_100_13_end:
            add qword [rsp - 3628], 1
            if_102_16:
            cmp_102_16:
            cmp qword [rsp - 3628], 4
            jne if_102_13_end
            jmp if_102_16_code
            if_102_16_code:
                jmp loop_99_9_end
            if_102_13_end:
        jmp loop_99_9
        loop_99_9_end:
        add qword [rsp - 3620], 1
        if_105_12:
        cmp_105_12:
        cmp qword [rsp - 3620], 1
        jne if_105_9_end
        jmp if_105_12_code
        if_105_12_code:
            jmp loop_97_5_end
        if_105_9_end:
    jmp loop_97_5
    loop_97_5_end:
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 0
    imul r14, 600
    add r15, r14
    mov r14, 0
    imul r14, 148
    add r15, r14
    verify_mesh_109_5:
        cmp_62_12_109_5:
        cmp dword [rsp - 3456], 0
        jne bool_false_62_12_109_5
        jmp bool_true_62_12_109_5
        bool_true_62_12_109_5:
        mov r14, true
        jmp bool_end_62_12_109_5
        bool_false_62_12_109_5:
        mov r14, false
        bool_end_62_12_109_5:
        assert_62_5_109_5:
            if_11_8_62_5_109_5:
            cmp_11_8_62_5_109_5:
            test r14, r14
            jne if_11_5_62_5_109_5_end
            jmp if_11_8_62_5_109_5_code
            if_11_8_62_5_109_5_code:
                mov rdi, 1
                exit_11_17_62_5_109_5:
                    mov rax, 60
                    syscall
                exit_11_17_62_5_109_5_end:
            if_11_5_62_5_109_5_end:
        assert_62_5_109_5_end:
        jmp verify_mesh_109_5_end
        mov qword [rsp - 3628], 0
        loop_65_5_109_5:
            cmp_66_16_109_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                imul r11, 12
                add r12, r11
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 10
            cmp r13, r12
            jne bool_false_66_16_109_5
            jmp bool_true_66_16_109_5
            bool_true_66_16_109_5:
            mov r14, true
            jmp bool_end_66_16_109_5
            bool_false_66_16_109_5:
            mov r14, false
            bool_end_66_16_109_5:
            assert_66_9_109_5:
                if_11_8_66_9_109_5:
                cmp_11_8_66_9_109_5:
                test r14, r14
                jne if_11_5_66_9_109_5_end
                jmp if_11_8_66_9_109_5_code
                if_11_8_66_9_109_5_code:
                    mov rdi, 1
                    exit_11_17_66_9_109_5:
                        mov rax, 60
                        syscall
                    exit_11_17_66_9_109_5_end:
                if_11_5_66_9_109_5_end:
            assert_66_9_109_5_end:
            cmp_67_16_109_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                imul r11, 12
                add r12, r11
                add r12, 4
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 20
            cmp r13, r12
            jne bool_false_67_16_109_5
            jmp bool_true_67_16_109_5
            bool_true_67_16_109_5:
            mov r14, true
            jmp bool_end_67_16_109_5
            bool_false_67_16_109_5:
            mov r14, false
            bool_end_67_16_109_5:
            assert_67_9_109_5:
                if_11_8_67_9_109_5:
                cmp_11_8_67_9_109_5:
                test r14, r14
                jne if_11_5_67_9_109_5_end
                jmp if_11_8_67_9_109_5_code
                if_11_8_67_9_109_5_code:
                    mov rdi, 1
                    exit_11_17_67_9_109_5:
                        mov rax, 60
                        syscall
                    exit_11_17_67_9_109_5_end:
                if_11_5_67_9_109_5_end:
            assert_67_9_109_5_end:
            cmp_68_16_109_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                imul r11, 12
                add r12, r11
                add r12, 8
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 30
            cmp r13, r12
            jne bool_false_68_16_109_5
            jmp bool_true_68_16_109_5
            bool_true_68_16_109_5:
            mov r14, true
            jmp bool_end_68_16_109_5
            bool_false_68_16_109_5:
            mov r14, false
            bool_end_68_16_109_5:
            assert_68_9_109_5:
                if_11_8_68_9_109_5:
                cmp_11_8_68_9_109_5:
                test r14, r14
                jne if_11_5_68_9_109_5_end
                jmp if_11_8_68_9_109_5_code
                if_11_8_68_9_109_5_code:
                    mov rdi, 1
                    exit_11_17_68_9_109_5:
                        mov rax, 60
                        syscall
                    exit_11_17_68_9_109_5_end:
                if_11_5_68_9_109_5_end:
            assert_68_9_109_5_end:
            add qword [rsp - 3628], 1
            if_70_12_109_5:
            cmp_70_12_109_5:
            cmp qword [rsp - 3628], 8
            jne if_70_9_109_5_end
            jmp if_70_12_109_5_code
            if_70_12_109_5_code:
                jmp loop_65_5_109_5_end
            if_70_9_109_5_end:
        jmp loop_65_5_109_5
        loop_65_5_109_5_end:
    verify_mesh_109_5_end:
    cmp_110_12:
        lea r13, [rsp - 3600]
        mov r12, 0
        imul r12, 1800
        add r13, r12
        mov r12, 0
        imul r12, 600
        add r13, r12
        mov r12, 3
        imul r12, 148
        add r13, r12
        add r13, 144
        movsx r14, dword [r13]
    cmp r14, 3
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_11_8_110_5:
        cmp_11_8_110_5:
        test r15, r15
        jne if_11_5_110_5_end
        jmp if_11_8_110_5_code
        if_11_8_110_5_code:
            mov rdi, 1
            exit_11_17_110_5:
                mov rax, 60
                syscall
            exit_11_17_110_5_end:
        if_11_5_110_5_end:
    assert_110_5_end:
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 0
    imul r14, 600
    add r15, r14
    mov r14, 3
    imul r14, 148
    add r15, r14
    verify_mesh_111_5:
        cmp_62_12_111_5:
        cmp dword [rsp - 3456], 3
        jne bool_false_62_12_111_5
        jmp bool_true_62_12_111_5
        bool_true_62_12_111_5:
        mov r14, true
        jmp bool_end_62_12_111_5
        bool_false_62_12_111_5:
        mov r14, false
        bool_end_62_12_111_5:
        assert_62_5_111_5:
            if_11_8_62_5_111_5:
            cmp_11_8_62_5_111_5:
            test r14, r14
            jne if_11_5_62_5_111_5_end
            jmp if_11_8_62_5_111_5_code
            if_11_8_62_5_111_5_code:
                mov rdi, 1
                exit_11_17_62_5_111_5:
                    mov rax, 60
                    syscall
                exit_11_17_62_5_111_5_end:
            if_11_5_62_5_111_5_end:
        assert_62_5_111_5_end:
        jmp verify_mesh_111_5_end
        mov qword [rsp - 3628], 0
        loop_65_5_111_5:
            cmp_66_16_111_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                imul r11, 12
                add r12, r11
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 10
            cmp r13, r12
            jne bool_false_66_16_111_5
            jmp bool_true_66_16_111_5
            bool_true_66_16_111_5:
            mov r14, true
            jmp bool_end_66_16_111_5
            bool_false_66_16_111_5:
            mov r14, false
            bool_end_66_16_111_5:
            assert_66_9_111_5:
                if_11_8_66_9_111_5:
                cmp_11_8_66_9_111_5:
                test r14, r14
                jne if_11_5_66_9_111_5_end
                jmp if_11_8_66_9_111_5_code
                if_11_8_66_9_111_5_code:
                    mov rdi, 1
                    exit_11_17_66_9_111_5:
                        mov rax, 60
                        syscall
                    exit_11_17_66_9_111_5_end:
                if_11_5_66_9_111_5_end:
            assert_66_9_111_5_end:
            cmp_67_16_111_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                imul r11, 12
                add r12, r11
                add r12, 4
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 20
            cmp r13, r12
            jne bool_false_67_16_111_5
            jmp bool_true_67_16_111_5
            bool_true_67_16_111_5:
            mov r14, true
            jmp bool_end_67_16_111_5
            bool_false_67_16_111_5:
            mov r14, false
            bool_end_67_16_111_5:
            assert_67_9_111_5:
                if_11_8_67_9_111_5:
                cmp_11_8_67_9_111_5:
                test r14, r14
                jne if_11_5_67_9_111_5_end
                jmp if_11_8_67_9_111_5_code
                if_11_8_67_9_111_5_code:
                    mov rdi, 1
                    exit_11_17_67_9_111_5:
                        mov rax, 60
                        syscall
                    exit_11_17_67_9_111_5_end:
                if_11_5_67_9_111_5_end:
            assert_67_9_111_5_end:
            cmp_68_16_111_5:
                mov r12, r15
                mov r11, qword [rsp - 3628]
                imul r11, 12
                add r12, r11
                add r12, 8
                movsx r13, dword [r12]
                mov r12, qword [rsp - 3628]
                imul r12, 30
            cmp r13, r12
            jne bool_false_68_16_111_5
            jmp bool_true_68_16_111_5
            bool_true_68_16_111_5:
            mov r14, true
            jmp bool_end_68_16_111_5
            bool_false_68_16_111_5:
            mov r14, false
            bool_end_68_16_111_5:
            assert_68_9_111_5:
                if_11_8_68_9_111_5:
                cmp_11_8_68_9_111_5:
                test r14, r14
                jne if_11_5_68_9_111_5_end
                jmp if_11_8_68_9_111_5_code
                if_11_8_68_9_111_5_code:
                    mov rdi, 1
                    exit_11_17_68_9_111_5:
                        mov rax, 60
                        syscall
                    exit_11_17_68_9_111_5_end:
                if_11_5_68_9_111_5_end:
            assert_68_9_111_5_end:
            add qword [rsp - 3628], 1
            if_70_12_111_5:
            cmp_70_12_111_5:
            cmp qword [rsp - 3628], 8
            jne if_70_9_111_5_end
            jmp if_70_12_111_5_code
            if_70_12_111_5_code:
                jmp loop_65_5_111_5_end
            if_70_9_111_5_end:
        jmp loop_65_5_111_5
        loop_65_5_111_5_end:
    verify_mesh_111_5_end:
mov rdi, 0
mov rax, 60
syscall
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 1
    imul r14, 600
    add r15, r14
    mov r14, 2
    imul r14, 148
    add r15, r14
    mov r14, 5
    imul r14, 12
    add r15, r14
    mov dword [r15], 0xff
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 1
    imul r14, 600
    add r15, r14
    mov r14, 2
    imul r14, 148
    add r15, r14
    mov r14, 5
    imul r14, 12
    add r15, r14
    add r15, 4
    mov dword [r15], 0xaa
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 1
    imul r14, 600
    add r15, r14
    mov r14, 2
    imul r14, 148
    add r15, r14
    mov r14, 5
    imul r14, 12
    add r15, r14
    add r15, 8
    mov dword [r15], 0xbb
    cmp_118_12:
        lea r13, [rsp - 3600]
        mov r12, 0
        imul r12, 1800
        add r13, r12
        mov r12, 1
        imul r12, 600
        add r13, r12
        mov r12, 2
        imul r12, 148
        add r13, r12
        mov r12, 5
        imul r12, 12
        add r13, r12
        movsx r14, dword [r13]
    cmp r14, 0xff
    jne bool_false_118_12
    jmp bool_true_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
    assert_118_5:
        if_11_8_118_5:
        cmp_11_8_118_5:
        test r15, r15
        jne if_11_5_118_5_end
        jmp if_11_8_118_5_code
        if_11_8_118_5_code:
            mov rdi, 1
            exit_11_17_118_5:
                mov rax, 60
                syscall
            exit_11_17_118_5_end:
        if_11_5_118_5_end:
    assert_118_5_end:
    cmp_119_12:
        lea r13, [rsp - 3600]
        mov r12, 0
        imul r12, 1800
        add r13, r12
        mov r12, 1
        imul r12, 600
        add r13, r12
        mov r12, 2
        imul r12, 148
        add r13, r12
        mov r12, 5
        imul r12, 12
        add r13, r12
        add r13, 4
        movsx r14, dword [r13]
    cmp r14, 0xaa
    jne bool_false_119_12
    jmp bool_true_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
    assert_119_5:
        if_11_8_119_5:
        cmp_11_8_119_5:
        test r15, r15
        jne if_11_5_119_5_end
        jmp if_11_8_119_5_code
        if_11_8_119_5_code:
            mov rdi, 1
            exit_11_17_119_5:
                mov rax, 60
                syscall
            exit_11_17_119_5_end:
        if_11_5_119_5_end:
    assert_119_5_end:
    cmp_120_12:
        lea r13, [rsp - 3600]
        mov r12, 0
        imul r12, 1800
        add r13, r12
        mov r12, 1
        imul r12, 600
        add r13, r12
        mov r12, 2
        imul r12, 148
        add r13, r12
        mov r12, 5
        imul r12, 12
        add r13, r12
        add r13, 8
        movsx r14, dword [r13]
    cmp r14, 0xbb
    jne bool_false_120_12
    jmp bool_true_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_11_8_120_5:
        cmp_11_8_120_5:
        test r15, r15
        jne if_11_5_120_5_end
        jmp if_11_8_120_5_code
        if_11_8_120_5_code:
            mov rdi, 1
            exit_11_17_120_5:
                mov rax, 60
                syscall
            exit_11_17_120_5_end:
        if_11_5_120_5_end:
    assert_120_5_end:
    mov rcx, 4
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 0
    imul r14, 600
    add r15, r14
    lea rsi, [r15]
    lea r15, [rsp - 3600]
    mov r14, 1
    imul r14, 1800
    add r15, r14
    mov r14, 2
    imul r14, 600
    add r15, r14
    lea rdi, [r15]
    imul rcx, 148
    rep movsb
    mov qword [rsp - 3628], 0
    loop_131_5:
        cmp_132_16:
            lea r13, [rsp - 3600]
            mov r12, 1
            imul r12, 1800
            add r13, r12
            mov r12, 2
            imul r12, 600
            add r13, r12
            mov r12, qword [rsp - 3628]
            imul r12, 148
            add r13, r12
            add r13, 144
            movsx r14, dword [r13]
        cmp r14, qword [rsp - 3628]
        jne bool_false_132_16
        jmp bool_true_132_16
        bool_true_132_16:
        mov r15, true
        jmp bool_end_132_16
        bool_false_132_16:
        mov r15, false
        bool_end_132_16:
        assert_132_9:
            if_11_8_132_9:
            cmp_11_8_132_9:
            test r15, r15
            jne if_11_5_132_9_end
            jmp if_11_8_132_9_code
            if_11_8_132_9_code:
                mov rdi, 1
                exit_11_17_132_9:
                    mov rax, 60
                    syscall
                exit_11_17_132_9_end:
            if_11_5_132_9_end:
        assert_132_9_end:
        add qword [rsp - 3628], 1
        if_134_12:
        cmp_134_12:
        cmp qword [rsp - 3628], 4
        jne if_134_9_end
        jmp if_134_12_code
        if_134_12_code:
            jmp loop_131_5_end
        if_134_9_end:
    jmp loop_131_5
    loop_131_5_end:
    cmp_138_21:
        mov rcx, 8
        lea r14, [rsp - 3600]
        mov r13, 0
        imul r13, 1800
        add r14, r13
        mov r13, 0
        imul r13, 600
        add r14, r13
        mov r13, 1
        imul r13, 148
        add r14, r13
        lea rsi, [r14]
        lea r14, [rsp - 3600]
        mov r13, 1
        imul r13, 1800
        add r14, r13
        mov r13, 2
        imul r13, 600
        add r14, r13
        mov r13, 1
        imul r13, 148
        add r14, r13
        lea rdi, [r14]
        imul rcx, 12
        repe cmpsb
        je cmps_eq_138_21
        mov r15, false
        jmp cmps_end_138_21
        cmps_eq_138_21:
        mov r15, true
        cmps_end_138_21:
    test r15, r15
    je bool_false_138_21
    jmp bool_true_138_21
    bool_true_138_21:
    mov byte [rsp - 3629], true
    jmp bool_end_138_21
    bool_false_138_21:
    mov byte [rsp - 3629], false
    bool_end_138_21:
    cmp_143_12:
    mov r14b, byte [rsp - 3629]
    test byte [rsp - 3629], r14b
    je bool_false_143_12
    jmp bool_true_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
    assert_143_5:
        if_11_8_143_5:
        cmp_11_8_143_5:
        test r15, r15
        jne if_11_5_143_5_end
        jmp if_11_8_143_5_code
        if_11_8_143_5_code:
            mov rdi, 1
            exit_11_17_143_5:
                mov rax, 60
                syscall
            exit_11_17_143_5_end:
        if_11_5_143_5_end:
    assert_143_5_end:
    mov rcx, 6
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 0
    imul r14, 600
    add r15, r14
    mov r14, 2
    imul r14, 148
    add r15, r14
    add r15, 96
    lea rsi, [r15]
    lea r15, [rsp - 3600]
    mov r14, 0
    imul r14, 1800
    add r15, r14
    mov r14, 1
    imul r14, 600
    add r15, r14
    mov r14, 1
    imul r14, 148
    add r15, r14
    add r15, 96
    lea rdi, [r15]
    shl rcx, 2
    rep movsb
    mov qword [rsp - 3637], 0
    loop_153_5:
        cmp_154_16:
            lea r13, [rsp - 3600]
            mov r12, 0
            imul r12, 1800
            add r13, r12
            mov r12, 1
            imul r12, 600
            add r13, r12
            mov r12, 1
            imul r12, 148
            add r13, r12
            mov r12, qword [rsp - 3637]
            movsx r14, dword [r13 + r12 * 4 + 96]
        cmp r14, qword [rsp - 3637]
        jne bool_false_154_16
        jmp bool_true_154_16
        bool_true_154_16:
        mov r15, true
        jmp bool_end_154_16
        bool_false_154_16:
        mov r15, false
        bool_end_154_16:
        assert_154_9:
            if_11_8_154_9:
            cmp_11_8_154_9:
            test r15, r15
            jne if_11_5_154_9_end
            jmp if_11_8_154_9_code
            if_11_8_154_9_code:
                mov rdi, 1
                exit_11_17_154_9:
                    mov rax, 60
                    syscall
                exit_11_17_154_9_end:
            if_11_5_154_9_end:
        assert_154_9_end:
        add qword [rsp - 3637], 1
        if_156_12:
        cmp_156_12:
        cmp qword [rsp - 3637], 6
        jne if_156_9_end
        jmp if_156_12_code
        if_156_12_code:
            jmp loop_153_5_end
        if_156_9_end:
    jmp loop_153_5
    loop_153_5_end:
    mov qword [rsp - 3645], 0
    loop_162_5:
        lea r15, [rsp - 3600]
        mov r14, 1
        imul r14, 1800
        add r15, r14
        mov r14, 0
        imul r14, 600
        add r15, r14
        mov r14, qword [rsp - 3645]
        imul r14, 148
        add r15, r14
        mov r14, qword [rsp - 3645]
        add r14, 100
        init_mesh_163_9:
            mov dword [r15 + 144], r14d
            jmp init_mesh_163_9_end
            mov qword [rsp - 3653], 0
            loop_45_5_163_9:
                mov r13, r15
                mov r12, qword [rsp - 3653]
                imul r12, 12
                add r13, r12
                mov r12d, dword [rsp - 3653]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 10
                mov dword [r13], r12d
                mov r13, r15
                mov r12, qword [rsp - 3653]
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
                imul r12, 12
                add r13, r12
                add r13, 8
                mov r12d, dword [rsp - 3653]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 30
                mov dword [r13], r12d
                add qword [rsp - 3653], 1
                if_50_12_163_9:
                cmp_50_12_163_9:
                cmp qword [rsp - 3653], 8
                jne if_50_9_163_9_end
                jmp if_50_12_163_9_code
                if_50_12_163_9_code:
                    jmp loop_45_5_163_9_end
                if_50_9_163_9_end:
            jmp loop_45_5_163_9
            loop_45_5_163_9_end:
            mov qword [rsp - 3653], 0
            loop_54_5_163_9:
                mov r13, r15
                mov r12, qword [rsp - 3653]
                mov r11d, dword [rsp - 3653]
                mov dword [r13 + r12 * 4 + 96], r11d
                add qword [rsp - 3653], 1
                if_57_12_163_9:
                cmp_57_12_163_9:
                cmp qword [rsp - 3653], 12
                jne if_57_9_163_9_end
                jmp if_57_12_163_9_code
                if_57_12_163_9_code:
                    jmp loop_54_5_163_9_end
                if_57_9_163_9_end:
            jmp loop_54_5_163_9
            loop_54_5_163_9_end:
        init_mesh_163_9_end:
        add qword [rsp - 3645], 1
        if_165_12:
        cmp_165_12:
        cmp qword [rsp - 3645], 4
        jne if_165_9_end
        jmp if_165_12_code
        if_165_12_code:
            jmp loop_162_5_end
        if_165_9_end:
    jmp loop_162_5
    loop_162_5_end:
    lea r15, [rsp - 3600]
    mov r14, 1
    imul r14, 1800
    add r15, r14
    mov r14, 0
    imul r14, 600
    add r15, r14
    transform_vertices_168_5:
        mov qword [rsp - 3653], 0
        loop_76_5_168_5:
            mov qword [rsp - 3661], 0
            loop_78_9_168_5:
                mov r14, r15
                mov r13, qword [rsp - 3653]
                imul r13, 148
                add r14, r13
                mov r13, qword [rsp - 3661]
                imul r13, 12
                add r14, r13
                mov r13, r15
                mov r12, qword [rsp - 3653]
                imul r12, 148
                add r13, r12
                mov r12, qword [rsp - 3661]
                imul r12, 12
                add r13, r12
                mov r12d, dword [r13]
                mov dword [r14], r12d
                mov r13d, dword [rsp - 3612]
                add dword [r14], r13d
                mov r14, r15
                mov r13, qword [rsp - 3653]
                imul r13, 148
                add r14, r13
                mov r13, qword [rsp - 3661]
                imul r13, 12
                add r14, r13
                add r14, 4
                mov r13, r15
                mov r12, qword [rsp - 3653]
                imul r12, 148
                add r13, r12
                mov r12, qword [rsp - 3661]
                imul r12, 12
                add r13, r12
                add r13, 4
                mov r12d, dword [r13]
                mov dword [r14], r12d
                mov r13d, dword [rsp - 3608]
                add dword [r14], r13d
                mov r14, r15
                mov r13, qword [rsp - 3653]
                imul r13, 148
                add r14, r13
                mov r13, qword [rsp - 3661]
                imul r13, 12
                add r14, r13
                add r14, 8
                mov r13, r15
                mov r12, qword [rsp - 3653]
                imul r12, 148
                add r13, r12
                mov r12, qword [rsp - 3661]
                imul r12, 12
                add r13, r12
                add r13, 8
                mov r12d, dword [r13]
                mov dword [r14], r12d
                mov r13d, dword [rsp - 3604]
                add dword [r14], r13d
                add qword [rsp - 3661], 1
                if_83_16_168_5:
                cmp_83_16_168_5:
                cmp qword [rsp - 3661], 8
                jne if_83_13_168_5_end
                jmp if_83_16_168_5_code
                if_83_16_168_5_code:
                    jmp loop_78_9_168_5_end
                if_83_13_168_5_end:
            jmp loop_78_9_168_5
            loop_78_9_168_5_end:
            add qword [rsp - 3653], 1
            if_86_12_168_5:
            cmp_86_12_168_5:
            cmp qword [rsp - 3653], 4
            jne if_86_9_168_5_end
            jmp if_86_12_168_5_code
            if_86_12_168_5_code:
                jmp loop_76_5_168_5_end
            if_86_9_168_5_end:
        jmp loop_76_5_168_5
        loop_76_5_168_5_end:
    transform_vertices_168_5_end:
    cmp_169_12:
        lea r13, [rsp - 3600]
        mov r12, 1
        imul r12, 1800
        add r13, r12
        mov r12, 0
        imul r12, 600
        add r13, r12
        mov r12, 0
        imul r12, 148
        add r13, r12
        mov r12, 0
        imul r12, 12
        add r13, r12
        movsx r14, dword [r13]
        mov r13, 0
        add r13, 5
    cmp r14, r13
    jne bool_false_169_12
    jmp bool_true_169_12
    bool_true_169_12:
    mov r15, true
    jmp bool_end_169_12
    bool_false_169_12:
    mov r15, false
    bool_end_169_12:
    assert_169_5:
        if_11_8_169_5:
        cmp_11_8_169_5:
        test r15, r15
        jne if_11_5_169_5_end
        jmp if_11_8_169_5_code
        if_11_8_169_5_code:
            mov rdi, 1
            exit_11_17_169_5:
                mov rax, 60
                syscall
            exit_11_17_169_5_end:
        if_11_5_169_5_end:
    assert_169_5_end:
    cmp_170_12:
        lea r13, [rsp - 3600]
        mov r12, 1
        imul r12, 1800
        add r13, r12
        mov r12, 0
        imul r12, 600
        add r13, r12
        mov r12, 1
        imul r12, 148
        add r13, r12
        mov r12, 3
        imul r12, 12
        add r13, r12
        add r13, 4
        movsx r14, dword [r13]
        mov r13, 60
        add r13, 10
    cmp r14, r13
    jne bool_false_170_12
    jmp bool_true_170_12
    bool_true_170_12:
    mov r15, true
    jmp bool_end_170_12
    bool_false_170_12:
    mov r15, false
    bool_end_170_12:
    assert_170_5:
        if_11_8_170_5:
        cmp_11_8_170_5:
        test r15, r15
        jne if_11_5_170_5_end
        jmp if_11_8_170_5_code
        if_11_8_170_5_code:
            mov rdi, 1
            exit_11_17_170_5:
                mov rax, 60
                syscall
            exit_11_17_170_5_end:
        if_11_5_170_5_end:
    assert_170_5_end:
    cmp_171_12:
        lea r13, [rsp - 3600]
        mov r12, 1
        imul r12, 1800
        add r13, r12
        mov r12, 0
        imul r12, 600
        add r13, r12
        mov r12, 2
        imul r12, 148
        add r13, r12
        mov r12, 7
        imul r12, 12
        add r13, r12
        add r13, 8
        movsx r14, dword [r13]
        mov r13, 210
        add r13, 15
    cmp r14, r13
    jne bool_false_171_12
    jmp bool_true_171_12
    bool_true_171_12:
    mov r15, true
    jmp bool_end_171_12
    bool_false_171_12:
    mov r15, false
    bool_end_171_12:
    assert_171_5:
        if_11_8_171_5:
        cmp_11_8_171_5:
        test r15, r15
        jne if_11_5_171_5_end
        jmp if_11_8_171_5_code
        if_11_8_171_5_code:
            mov rdi, 1
            exit_11_17_171_5:
                mov rax, 60
                syscall
            exit_11_17_171_5_end:
        if_11_5_171_5_end:
    assert_171_5_end:
    lea rdi, [rsp - 4237]
    mov rcx, 592
    xor rax, rax
    rep stosb
    mov qword [rsp - 4245], 0
    loop_176_5:
        lea r15, [rsp - 4237]
        mov r14, qword [rsp - 4245]
        imul r14, 148
        add r15, r14
        mov r14, qword [rsp - 4245]
        imul r14, 100
        init_mesh_177_9:
            mov dword [r15 + 144], r14d
            jmp init_mesh_177_9_end
            mov qword [rsp - 4253], 0
            loop_45_5_177_9:
                mov r13, r15
                mov r12, qword [rsp - 4253]
                imul r12, 12
                add r13, r12
                mov r12d, dword [rsp - 4253]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 10
                mov dword [r13], r12d
                mov r13, r15
                mov r12, qword [rsp - 4253]
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
                imul r12, 12
                add r13, r12
                add r13, 8
                mov r12d, dword [rsp - 4253]
                mov dword [r13], r12d
                movsx r12, dword [r13]
                imul r12, 30
                mov dword [r13], r12d
                add qword [rsp - 4253], 1
                if_50_12_177_9:
                cmp_50_12_177_9:
                cmp qword [rsp - 4253], 8
                jne if_50_9_177_9_end
                jmp if_50_12_177_9_code
                if_50_12_177_9_code:
                    jmp loop_45_5_177_9_end
                if_50_9_177_9_end:
            jmp loop_45_5_177_9
            loop_45_5_177_9_end:
            mov qword [rsp - 4253], 0
            loop_54_5_177_9:
                mov r13, r15
                mov r12, qword [rsp - 4253]
                mov r11d, dword [rsp - 4253]
                mov dword [r13 + r12 * 4 + 96], r11d
                add qword [rsp - 4253], 1
                if_57_12_177_9:
                cmp_57_12_177_9:
                cmp qword [rsp - 4253], 12
                jne if_57_9_177_9_end
                jmp if_57_12_177_9_code
                if_57_12_177_9_code:
                    jmp loop_54_5_177_9_end
                if_57_9_177_9_end:
            jmp loop_54_5_177_9
            loop_54_5_177_9_end:
        init_mesh_177_9_end:
        add qword [rsp - 4245], 1
        if_179_12:
        cmp_179_12:
        cmp qword [rsp - 4245], 4
        jne if_179_9_end
        jmp if_179_12_code
        if_179_12_code:
            jmp loop_176_5_end
        if_179_9_end:
    jmp loop_176_5
    loop_176_5_end:
    mov qword [rsp - 4253], 0
    loop_184_5:
        cmp_185_16:
            lea r13, [rsp - 4237]
            mov r12, qword [rsp - 4253]
            imul r12, 148
            add r13, r12
            add r13, 144
            movsx r14, dword [r13]
            mov r13, qword [rsp - 4253]
            imul r13, 100
        cmp r14, r13
        jne bool_false_185_16
        jmp bool_true_185_16
        bool_true_185_16:
        mov r15, true
        jmp bool_end_185_16
        bool_false_185_16:
        mov r15, false
        bool_end_185_16:
        assert_185_9:
            if_11_8_185_9:
            cmp_11_8_185_9:
            test r15, r15
            jne if_11_5_185_9_end
            jmp if_11_8_185_9_code
            if_11_8_185_9_code:
                mov rdi, 1
                exit_11_17_185_9:
                    mov rax, 60
                    syscall
                exit_11_17_185_9_end:
            if_11_5_185_9_end:
        assert_185_9_end:
        add qword [rsp - 4253], 1
        if_187_12:
        cmp_187_12:
        cmp qword [rsp - 4253], 4
        jne if_187_9_end
        jmp if_187_12_code
        if_187_12_code:
            jmp loop_184_5_end
        if_187_9_end:
    jmp loop_184_5
    loop_184_5_end:
    mov rdx, test_passed.len
    mov rsi, test_passed
    print_190_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_190_5_end:
    mov rdi, 0
    exit_191_5:
        mov rax, 60
        syscall
    exit_191_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
