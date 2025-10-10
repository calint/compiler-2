section .bss
stk resd 0x10000
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz',10,''
hello.len equ $-hello
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
nl: db '',10,''
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
    cmp_79_12:
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_79_12
    bool_true_79_12:
    mov r15, true
    jmp bool_end_79_12
    bool_false_79_12:
    mov r15, false
    bool_end_79_12:
    assert_79_5:
        if_19_8_79_5:
        cmp_19_8_79_5:
        cmp r15, false
        jne if_19_5_79_5_end
        if_19_8_79_5_code:
            mov rdi, 1
            exit_19_17_79_5:
                mov rax, 60
                syscall
            exit_19_17_79_5_end:
        if_19_5_79_5_end:
    assert_79_5_end:
    cmp_80_12:
        mov r13, 2
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
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_83_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_83_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_84_12:
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
    assert_84_5:
        if_19_8_84_5:
        cmp_19_8_84_5:
        cmp r15, false
        jne if_19_5_84_5_end
        if_19_8_84_5_code:
            mov rdi, 1
            exit_19_17_84_5:
                mov rax, 60
                syscall
            exit_19_17_84_5_end:
        if_19_5_84_5_end:
    assert_84_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_87_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_87_5_end:
    cmp_88_12:
    cmp qword [rsp - 40], 2
    jne bool_false_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
    assert_88_5:
        if_19_8_88_5:
        cmp_19_8_88_5:
        cmp r15, false
        jne if_19_5_88_5_end
        if_19_8_88_5_code:
            mov rdi, 1
            exit_19_17_88_5:
                mov rax, 60
                syscall
            exit_19_17_88_5_end:
        if_19_5_88_5_end:
    assert_88_5_end:
    cmp_89_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_89_12
    bool_true_89_12:
    mov r15, true
    jmp bool_end_89_12
    bool_false_89_12:
    mov r15, false
    bool_end_89_12:
    assert_89_5:
        if_19_8_89_5:
        cmp_19_8_89_5:
        cmp r15, false
        jne if_19_5_89_5_end
        if_19_8_89_5_code:
            mov rdi, 1
            exit_19_17_89_5:
                mov rax, 60
                syscall
            exit_19_17_89_5_end:
        if_19_5_89_5_end:
    assert_89_5_end:
    mov qword [rsp - 48], 0
    bar_92_5:
        if_56_8_92_5:
        cmp_56_8_92_5:
        cmp qword [rsp - 48], 0
        jne if_56_5_92_5_end
        if_56_8_92_5_code:
            jmp bar_92_5_end
        if_56_5_92_5_end:
        mov qword [rsp - 48], 0xff
    bar_92_5_end:
    cmp_93_12:
    cmp qword [rsp - 48], 0
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
    assert_93_5:
        if_19_8_93_5:
        cmp_19_8_93_5:
        cmp r15, false
        jne if_19_5_93_5_end
        if_19_8_93_5_code:
            mov rdi, 1
            exit_19_17_93_5:
                mov rax, 60
                syscall
            exit_19_17_93_5_end:
        if_19_5_93_5_end:
    assert_93_5_end:
    mov qword [rsp - 48], 1
    bar_96_5:
        if_56_8_96_5:
        cmp_56_8_96_5:
        cmp qword [rsp - 48], 0
        jne if_56_5_96_5_end
        if_56_8_96_5_code:
            jmp bar_96_5_end
        if_56_5_96_5_end:
        mov qword [rsp - 48], 0xff
    bar_96_5_end:
    cmp_97_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
    assert_97_5:
        if_19_8_97_5:
        cmp_19_8_97_5:
        cmp r15, false
        jne if_19_5_97_5_end
        if_19_8_97_5_code:
            mov rdi, 1
            exit_19_17_97_5:
                mov rax, 60
                syscall
            exit_19_17_97_5_end:
        if_19_5_97_5_end:
    assert_97_5_end:
    mov qword [rsp - 56], 1
    baz_100_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_100_13_end:
    cmp_101_12:
    cmp qword [rsp - 64], 2
    jne bool_false_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
    assert_101_5:
        if_19_8_101_5:
        cmp_19_8_101_5:
        cmp r15, false
        jne if_19_5_101_5_end
        if_19_8_101_5_code:
            mov rdi, 1
            exit_19_17_101_5:
                mov rax, 60
                syscall
            exit_19_17_101_5_end:
        if_19_5_101_5_end:
    assert_101_5_end:
    baz_103_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_103_9_end:
    cmp_104_12:
    cmp qword [rsp - 64], 2
    jne bool_false_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_19_8_104_5:
        cmp_19_8_104_5:
        cmp r15, false
        jne if_19_5_104_5_end
        if_19_8_104_5_code:
            mov rdi, 1
            exit_19_17_104_5:
                mov rax, 60
                syscall
            exit_19_17_104_5_end:
        if_19_5_104_5_end:
    assert_104_5_end:
    baz_106_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_106_23_end:
    mov qword [rsp - 72], 0
    cmp_107_12:
    cmp qword [rsp - 80], 4
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
    cmp_113_12:
    cmp qword [rsp - 116], 10
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
    cmp_114_12:
    cmp qword [rsp - 108], 2
    jne bool_false_114_12
    bool_true_114_12:
    mov r15, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15, false
    bool_end_114_12:
    assert_114_5:
        if_19_8_114_5:
        cmp_19_8_114_5:
        cmp r15, false
        jne if_19_5_114_5_end
        if_19_8_114_5_code:
            mov rdi, 1
            exit_19_17_114_5:
                mov rax, 60
                syscall
            exit_19_17_114_5_end:
        if_19_5_114_5_end:
    assert_114_5_end:
    cmp_115_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_19_8_115_5:
        cmp_19_8_115_5:
        cmp r15, false
        jne if_19_5_115_5_end
        if_19_8_115_5_code:
            mov rdi, 1
            exit_19_17_115_5:
                mov rax, 60
                syscall
            exit_19_17_115_5_end:
        if_19_5_115_5_end:
    assert_115_5_end:
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
    cmp_119_12:
    cmp qword [rsp - 116], -1
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
    cmp qword [rsp - 108], -2
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
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_123_12:
    cmp qword [rsp - 152], -1
    jne bool_false_123_12
    bool_true_123_12:
    mov r15, true
    jmp bool_end_123_12
    bool_false_123_12:
    mov r15, false
    bool_end_123_12:
    assert_123_5:
        if_19_8_123_5:
        cmp_19_8_123_5:
        cmp r15, false
        jne if_19_5_123_5_end
        if_19_8_123_5_code:
            mov rdi, 1
            exit_19_17_123_5:
                mov rax, 60
                syscall
            exit_19_17_123_5_end:
        if_19_5_123_5_end:
    assert_123_5_end:
    cmp_124_12:
    cmp qword [rsp - 144], -2
    jne bool_false_124_12
    bool_true_124_12:
    mov r15, true
    jmp bool_end_124_12
    bool_false_124_12:
    mov r15, false
    bool_end_124_12:
    assert_124_5:
        if_19_8_124_5:
        cmp_19_8_124_5:
        cmp r15, false
        jne if_19_5_124_5_end
        if_19_8_124_5_code:
            mov rdi, 1
            exit_19_17_124_5:
                mov rax, 60
                syscall
            exit_19_17_124_5_end:
        if_19_5_124_5_end:
    assert_124_5_end:
    cmp_125_12:
    cmp dword [rsp - 136], 0xff0000
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
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 164], 73
    cmp_130_12:
        lea r13, [rsp - 172]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
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
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_134_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_134_5_end:
    loop_135_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_136_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_136_9_end:
        mov rdx, 80
        lea rsi, [rsp - 252]
        read_138_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 260], rax
        read_138_19_end:
        sub qword [rsp - 260], 1
        if_142_12:
        cmp_142_12:
        cmp qword [rsp - 260], 0
        jne if_144_19
        if_142_12_code:
            jmp loop_135_5_end
        jmp if_142_9_end
        if_144_19:
        cmp_144_19:
        cmp qword [rsp - 260], 4
        jg if_else_142_9
        if_144_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_145_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_145_13_end:
            jmp loop_135_5
        jmp if_142_9_end
        if_else_142_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_148_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_148_13_end:
            mov rdx, qword [rsp - 260]
            lea rsi, [rsp - 252]
            print_149_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_149_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_150_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_150_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_151_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_151_13_end:
        if_142_9_end:
    jmp loop_135_5
    loop_135_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
