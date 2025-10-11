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
    mov r15, 2
    lea rsi, [rsp + r15 * 4 - 16]
    lea rdi, [rsp - 16]
    mov rcx, 2
    shl rcx, 2
    rep movsb
    cmp_84_12:
        mov r13, 0
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
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_87_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_87_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_88_12:
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
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
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_91_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_91_5_end:
    cmp_92_12:
    cmp qword [rsp - 40], 2
    jne bool_false_92_12
    bool_true_92_12:
    mov r15, true
    jmp bool_end_92_12
    bool_false_92_12:
    mov r15, false
    bool_end_92_12:
    assert_92_5:
        if_19_8_92_5:
        cmp_19_8_92_5:
        cmp r15, false
        jne if_19_5_92_5_end
        if_19_8_92_5_code:
            mov rdi, 1
            exit_19_17_92_5:
                mov rax, 60
                syscall
            exit_19_17_92_5_end:
        if_19_5_92_5_end:
    assert_92_5_end:
    cmp_93_12:
    cmp qword [rsp - 32], 0xb
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
    mov qword [rsp - 48], 0
    bar_96_5:
        if_57_8_96_5:
        cmp_57_8_96_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_96_5_end
        if_57_8_96_5_code:
            jmp bar_96_5_end
        if_57_5_96_5_end:
        mov qword [rsp - 48], 0xff
    bar_96_5_end:
    cmp_97_12:
    cmp qword [rsp - 48], 0
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
    mov qword [rsp - 48], 1
    bar_100_5:
        if_57_8_100_5:
        cmp_57_8_100_5:
        cmp qword [rsp - 48], 0
        jne if_57_5_100_5_end
        if_57_8_100_5_code:
            jmp bar_100_5_end
        if_57_5_100_5_end:
        mov qword [rsp - 48], 0xff
    bar_100_5_end:
    cmp_101_12:
    cmp qword [rsp - 48], 0xff
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
    mov qword [rsp - 56], 1
    baz_104_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_104_13_end:
    cmp_105_12:
    cmp qword [rsp - 64], 2
    jne bool_false_105_12
    bool_true_105_12:
    mov r15, true
    jmp bool_end_105_12
    bool_false_105_12:
    mov r15, false
    bool_end_105_12:
    assert_105_5:
        if_19_8_105_5:
        cmp_19_8_105_5:
        cmp r15, false
        jne if_19_5_105_5_end
        if_19_8_105_5_code:
            mov rdi, 1
            exit_19_17_105_5:
                mov rax, 60
                syscall
            exit_19_17_105_5_end:
        if_19_5_105_5_end:
    assert_105_5_end:
    baz_107_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_107_9_end:
    cmp_108_12:
    cmp qword [rsp - 64], 2
    jne bool_false_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
    assert_108_5:
        if_19_8_108_5:
        cmp_19_8_108_5:
        cmp r15, false
        jne if_19_5_108_5_end
        if_19_8_108_5_code:
            mov rdi, 1
            exit_19_17_108_5:
                mov rax, 60
                syscall
            exit_19_17_108_5_end:
        if_19_5_108_5_end:
    assert_108_5_end:
    baz_110_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_110_23_end:
    mov qword [rsp - 72], 0
    cmp_111_12:
    cmp qword [rsp - 80], 4
    jne bool_false_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_19_8_111_5:
        cmp_19_8_111_5:
        cmp r15, false
        jne if_19_5_111_5_end
        if_19_8_111_5_code:
            mov rdi, 1
            exit_19_17_111_5:
                mov rax, 60
                syscall
            exit_19_17_111_5_end:
        if_19_5_111_5_end:
    assert_111_5_end:
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
    cmp_117_12:
    cmp qword [rsp - 116], 10
    jne bool_false_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
    assert_117_5:
        if_19_8_117_5:
        cmp_19_8_117_5:
        cmp r15, false
        jne if_19_5_117_5_end
        if_19_8_117_5_code:
            mov rdi, 1
            exit_19_17_117_5:
                mov rax, 60
                syscall
            exit_19_17_117_5_end:
        if_19_5_117_5_end:
    assert_117_5_end:
    cmp_118_12:
    cmp qword [rsp - 108], 2
    jne bool_false_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
    assert_118_5:
        if_19_8_118_5:
        cmp_19_8_118_5:
        cmp r15, false
        jne if_19_5_118_5_end
        if_19_8_118_5_code:
            mov rdi, 1
            exit_19_17_118_5:
                mov rax, 60
                syscall
            exit_19_17_118_5_end:
        if_19_5_118_5_end:
    assert_118_5_end:
    cmp_119_12:
    cmp dword [rsp - 100], 0xff0000
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
    cmp_123_12:
    cmp qword [rsp - 116], -1
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
    cmp qword [rsp - 108], -2
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
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_127_12:
    cmp qword [rsp - 152], -1
    jne bool_false_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
    assert_127_5:
        if_19_8_127_5:
        cmp_19_8_127_5:
        cmp r15, false
        jne if_19_5_127_5_end
        if_19_8_127_5_code:
            mov rdi, 1
            exit_19_17_127_5:
                mov rax, 60
                syscall
            exit_19_17_127_5_end:
        if_19_5_127_5_end:
    assert_127_5_end:
    cmp_128_12:
    cmp qword [rsp - 144], -2
    jne bool_false_128_12
    bool_true_128_12:
    mov r15, true
    jmp bool_end_128_12
    bool_false_128_12:
    mov r15, false
    bool_end_128_12:
    assert_128_5:
        if_19_8_128_5:
        cmp_19_8_128_5:
        cmp r15, false
        jne if_19_5_128_5_end
        if_19_8_128_5_code:
            mov rdi, 1
            exit_19_17_128_5:
                mov rax, 60
                syscall
            exit_19_17_128_5_end:
        if_19_5_128_5_end:
    assert_128_5_end:
    cmp_129_12:
    cmp dword [rsp - 136], 0xff0000
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
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 164], 73
    cmp_134_12:
        lea r13, [rsp - 172]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
    assert_134_5:
        if_19_8_134_5:
        cmp_19_8_134_5:
        cmp r15, false
        jne if_19_5_134_5_end
        if_19_8_134_5_code:
            mov rdi, 1
            exit_19_17_134_5:
                mov rax, 60
                syscall
            exit_19_17_134_5_end:
        if_19_5_134_5_end:
    assert_134_5_end:
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_138_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_138_5_end:
    loop_139_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_140_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_140_9_end:
        mov rdx, 80
        lea rsi, [rsp - 252]
        read_142_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 260], rax
        read_142_19_end:
        sub qword [rsp - 260], 1
        if_146_12:
        cmp_146_12:
        cmp qword [rsp - 260], 0
        jne if_148_19
        if_146_12_code:
            jmp loop_139_5_end
        jmp if_146_9_end
        if_148_19:
        cmp_148_19:
        cmp qword [rsp - 260], 4
        jg if_else_146_9
        if_148_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_149_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_149_13_end:
            jmp loop_139_5
        jmp if_146_9_end
        if_else_146_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_152_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_152_13_end:
            mov rdx, qword [rsp - 260]
            lea rsi, [rsp - 252]
            print_153_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_153_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_154_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_154_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_155_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_155_13_end:
        if_146_9_end:
    jmp loop_139_5
    loop_139_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
