section .bss
stk resd 0x10000
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $-hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
nl: db '', 10,''
nl.len equ $-nl
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
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, qword [rsp - 24]
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_80_12:
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_80_12
    bool_true_80_12:
    mov r15, true
    jmp bool_end_80_12
    bool_false_80_12:
    mov r15, false
    bool_end_80_12:
    assert_80_5:
        if_19_8_80_5:
        cmp_19_8_80_5:
        cmp r15, false
        jne if_19_5_80_5_end
        if_19_8_80_5_code:
            mov rdi, 1
            exit_19_17_80_5:
                mov rax, 60
                syscall
            exit_19_17_80_5_end:
        if_19_5_80_5_end:
    assert_80_5_end:
    cmp_81_12:
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
    assert_81_5:
        if_19_8_81_5:
        cmp_19_8_81_5:
        cmp r15, false
        jne if_19_5_81_5_end
        if_19_8_81_5_code:
            mov rdi, 1
            exit_19_17_81_5:
                mov rax, 60
                syscall
            exit_19_17_81_5_end:
        if_19_5_81_5_end:
    assert_81_5_end:
    mov r15, 2
    lea rsi, [rsp + r15 * 4 - 16]
    lea rdi, [rsp - 16]
    mov rcx, 2
    shl rcx, 2
    rep movsb
    cmp_86_12:
        mov r13, 0
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
    assert_86_5:
        if_19_8_86_5:
        cmp_19_8_86_5:
        cmp r15, false
        jne if_19_5_86_5_end
        if_19_8_86_5_code:
            mov rdi, 1
            exit_19_17_86_5:
                mov rax, 60
                syscall
            exit_19_17_86_5_end:
        if_19_5_86_5_end:
    assert_86_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_89_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_89_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_90_12:
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
    assert_90_5:
        if_19_8_90_5:
        cmp_19_8_90_5:
        cmp r15, false
        jne if_19_5_90_5_end
        if_19_8_90_5_code:
            mov rdi, 1
            exit_19_17_90_5:
                mov rax, 60
                syscall
            exit_19_17_90_5_end:
        if_19_5_90_5_end:
    assert_90_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_93_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_93_5_end:
    cmp_94_12:
    cmp qword [rsp - 40], 2
    jne bool_false_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
    assert_94_5:
        if_19_8_94_5:
        cmp_19_8_94_5:
        cmp r15, false
        jne if_19_5_94_5_end
        if_19_8_94_5_code:
            mov rdi, 1
            exit_19_17_94_5:
                mov rax, 60
                syscall
            exit_19_17_94_5_end:
        if_19_5_94_5_end:
    assert_94_5_end:
    cmp_95_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_95_12
    bool_true_95_12:
    mov r15, true
    jmp bool_end_95_12
    bool_false_95_12:
    mov r15, false
    bool_end_95_12:
    assert_95_5:
        if_19_8_95_5:
        cmp_19_8_95_5:
        cmp r15, false
        jne if_19_5_95_5_end
        if_19_8_95_5_code:
            mov rdi, 1
            exit_19_17_95_5:
                mov rax, 60
                syscall
            exit_19_17_95_5_end:
        if_19_5_95_5_end:
    assert_95_5_end:
    mov qword [rsp - 48], 0
    bar_98_5:
        if_57_8_98_5:
        cmp_57_8_98_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_98_5_end
        if_57_8_98_5_code:
            jmp bar_98_5_end
        if_57_5_98_5_end:
        mov qword [rsp - 48], 0xff
    bar_98_5_end:
    cmp_99_12:
    cmp qword [rsp - 48], 0
    jne bool_false_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
    assert_99_5:
        if_19_8_99_5:
        cmp_19_8_99_5:
        cmp r15, false
        jne if_19_5_99_5_end
        if_19_8_99_5_code:
            mov rdi, 1
            exit_19_17_99_5:
                mov rax, 60
                syscall
            exit_19_17_99_5_end:
        if_19_5_99_5_end:
    assert_99_5_end:
    mov qword [rsp - 48], 1
    bar_102_5:
        if_57_8_102_5:
        cmp_57_8_102_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_102_5_end
        if_57_8_102_5_code:
            jmp bar_102_5_end
        if_57_5_102_5_end:
        mov qword [rsp - 48], 0xff
    bar_102_5_end:
    cmp_103_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_103_12
    bool_true_103_12:
    mov r15, true
    jmp bool_end_103_12
    bool_false_103_12:
    mov r15, false
    bool_end_103_12:
    assert_103_5:
        if_19_8_103_5:
        cmp_19_8_103_5:
        cmp r15, false
        jne if_19_5_103_5_end
        if_19_8_103_5_code:
            mov rdi, 1
            exit_19_17_103_5:
                mov rax, 60
                syscall
            exit_19_17_103_5_end:
        if_19_5_103_5_end:
    assert_103_5_end:
    mov qword [rsp - 56], 1
    baz_106_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_106_13_end:
    cmp_107_12:
    cmp qword [rsp - 64], 2
    jne bool_false_107_12
    bool_true_107_12:
    mov r15, true
    jmp bool_end_107_12
    bool_false_107_12:
    mov r15, false
    bool_end_107_12:
    assert_107_5:
        if_19_8_107_5:
        cmp_19_8_107_5:
        cmp r15, false
        jne if_19_5_107_5_end
        if_19_8_107_5_code:
            mov rdi, 1
            exit_19_17_107_5:
                mov rax, 60
                syscall
            exit_19_17_107_5_end:
        if_19_5_107_5_end:
    assert_107_5_end:
    baz_109_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_109_9_end:
    cmp_110_12:
    cmp qword [rsp - 64], 2
    jne bool_false_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_19_8_110_5:
        cmp_19_8_110_5:
        cmp r15, false
        jne if_19_5_110_5_end
        if_19_8_110_5_code:
            mov rdi, 1
            exit_19_17_110_5:
                mov rax, 60
                syscall
            exit_19_17_110_5_end:
        if_19_5_110_5_end:
    assert_110_5_end:
    baz_112_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_112_23_end:
    mov qword [rsp - 72], 0
    cmp_113_12:
    cmp qword [rsp - 80], 4
    jne bool_false_113_12
    bool_true_113_12:
    mov r15, true
    jmp bool_end_113_12
    bool_false_113_12:
    mov r15, false
    bool_end_113_12:
    assert_113_5:
        if_19_8_113_5:
        cmp_19_8_113_5:
        cmp r15, false
        jne if_19_5_113_5_end
        if_19_8_113_5_code:
            mov rdi, 1
            exit_19_17_113_5:
                mov rax, 60
                syscall
            exit_19_17_113_5_end:
        if_19_5_113_5_end:
    assert_113_5_end:
    mov qword [rsp - 88], 1
    mov qword [rsp - 96], 2
        mov r15, qword [rsp - 88]
        mov qword [rsp - 116], r15
        mov r15, qword [rsp - 116]
        imul r15, 10
        mov qword [rsp - 116], r15
        mov r15, qword [rsp - 96]
        mov qword [rsp - 108], r15
    mov dword [rsp - 100], 0xff0000
    cmp_119_12:
    cmp qword [rsp - 116], 10
    jne bool_false_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
    assert_119_5:
        if_19_8_119_5:
        cmp_19_8_119_5:
        cmp r15, false
        jne if_19_5_119_5_end
        if_19_8_119_5_code:
            mov rdi, 1
            exit_19_17_119_5:
                mov rax, 60
                syscall
            exit_19_17_119_5_end:
        if_19_5_119_5_end:
    assert_119_5_end:
    cmp_120_12:
    cmp qword [rsp - 108], 2
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_19_8_120_5:
        cmp_19_8_120_5:
        cmp r15, false
        jne if_19_5_120_5_end
        if_19_8_120_5_code:
            mov rdi, 1
            exit_19_17_120_5:
                mov rax, 60
                syscall
            exit_19_17_120_5_end:
        if_19_5_120_5_end:
    assert_120_5_end:
    cmp_121_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_19_8_121_5:
        cmp_19_8_121_5:
        cmp r15, false
        jne if_19_5_121_5_end
        if_19_8_121_5_code:
            mov rdi, 1
            exit_19_17_121_5:
                mov rax, 60
                syscall
            exit_19_17_121_5_end:
        if_19_5_121_5_end:
    assert_121_5_end:
    mov r15, qword [rsp - 88]
    mov qword [rsp - 132], r15
    neg qword [rsp - 132]
    mov r15, qword [rsp - 96]
    mov qword [rsp - 124], r15
    neg qword [rsp - 124]
    lea rdi, [rsp - 116]
    lea rsi, [rsp - 132]
    mov rcx, 16
    rep movsb
    cmp_125_12:
    cmp qword [rsp - 116], -1
    jne bool_false_125_12
    bool_true_125_12:
    mov r15, true
    jmp bool_end_125_12
    bool_false_125_12:
    mov r15, false
    bool_end_125_12:
    assert_125_5:
        if_19_8_125_5:
        cmp_19_8_125_5:
        cmp r15, false
        jne if_19_5_125_5_end
        if_19_8_125_5_code:
            mov rdi, 1
            exit_19_17_125_5:
                mov rax, 60
                syscall
            exit_19_17_125_5_end:
        if_19_5_125_5_end:
    assert_125_5_end:
    cmp_126_12:
    cmp qword [rsp - 108], -2
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_19_8_126_5:
        cmp_19_8_126_5:
        cmp r15, false
        jne if_19_5_126_5_end
        if_19_8_126_5_code:
            mov rdi, 1
            exit_19_17_126_5:
                mov rax, 60
                syscall
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
    assert_126_5_end:
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_129_12:
    cmp qword [rsp - 152], -1
    jne bool_false_129_12
    bool_true_129_12:
    mov r15, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15, false
    bool_end_129_12:
    assert_129_5:
        if_19_8_129_5:
        cmp_19_8_129_5:
        cmp r15, false
        jne if_19_5_129_5_end
        if_19_8_129_5_code:
            mov rdi, 1
            exit_19_17_129_5:
                mov rax, 60
                syscall
            exit_19_17_129_5_end:
        if_19_5_129_5_end:
    assert_129_5_end:
    cmp_130_12:
    cmp qword [rsp - 144], -2
    jne bool_false_130_12
    bool_true_130_12:
    mov r15, true
    jmp bool_end_130_12
    bool_false_130_12:
    mov r15, false
    bool_end_130_12:
    assert_130_5:
        if_19_8_130_5:
        cmp_19_8_130_5:
        cmp r15, false
        jne if_19_5_130_5_end
        if_19_8_130_5_code:
            mov rdi, 1
            exit_19_17_130_5:
                mov rax, 60
                syscall
            exit_19_17_130_5_end:
        if_19_5_130_5_end:
    assert_130_5_end:
    cmp_131_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_131_12
    bool_true_131_12:
    mov r15, true
    jmp bool_end_131_12
    bool_false_131_12:
    mov r15, false
    bool_end_131_12:
    assert_131_5:
        if_19_8_131_5:
        cmp_19_8_131_5:
        cmp r15, false
        jne if_19_5_131_5_end
        if_19_8_131_5_code:
            mov rdi, 1
            exit_19_17_131_5:
                mov rax, 60
                syscall
            exit_19_17_131_5_end:
        if_19_5_131_5_end:
    assert_131_5_end:
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 164], 73
    cmp_136_12:
        lea r13, [rsp - 172]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_19_8_136_5:
        cmp_19_8_136_5:
        cmp r15, false
        jne if_19_5_136_5_end
        if_19_8_136_5_code:
            mov rdi, 1
            exit_19_17_136_5:
                mov rax, 60
                syscall
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
    assert_136_5_end:
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_140_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_140_5_end:
    loop_141_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_142_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_142_9_end:
        mov rdx, 80
        lea rsi, [rsp - 252]
        read_144_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 260], rax
        read_144_19_end:
        sub qword [rsp - 260], 1
        if_148_12:
        cmp_148_12:
        cmp qword [rsp - 260], 0
        jne if_150_19
        if_148_12_code:
            jmp loop_141_5_end
        jmp if_148_9_end
        if_150_19:
        cmp_150_19:
        cmp qword [rsp - 260], 4
        jg if_else_148_9
        if_150_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_151_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_151_13_end:
            jmp loop_141_5
        jmp if_148_9_end
        if_else_148_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_154_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_154_13_end:
            mov rdx, qword [rsp - 260]
            lea rsi, [rsp - 252]
            print_155_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_155_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_156_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_156_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_157_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_157_13_end:
        if_148_9_end:
    jmp loop_141_5
    loop_141_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
