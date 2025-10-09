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
    cmp_75_12:
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_75_12
    bool_true_75_12:
    mov r15, true
    jmp bool_end_75_12
    bool_false_75_12:
    mov r15, false
    bool_end_75_12:
    assert_75_5:
        if_18_8_75_5:
        cmp_18_8_75_5:
        cmp r15, false
        jne if_18_5_75_5_end
        if_18_8_75_5_code:
            mov rdi, 1
            exit_18_17_75_5:
                mov rax, 60
                syscall
            exit_18_17_75_5_end:
        if_18_5_75_5_end:
    assert_75_5_end:
    cmp_76_12:
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_76_12
    bool_true_76_12:
    mov r15, true
    jmp bool_end_76_12
    bool_false_76_12:
    mov r15, false
    bool_end_76_12:
    assert_76_5:
        if_18_8_76_5:
        cmp_18_8_76_5:
        cmp r15, false
        jne if_18_5_76_5_end
        if_18_8_76_5_code:
            mov rdi, 1
            exit_18_17_76_5:
                mov rax, 60
                syscall
            exit_18_17_76_5_end:
        if_18_5_76_5_end:
    assert_76_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_79_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_79_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_80_12:
        mov r13, qword [rsp - 24]
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
        if_18_8_80_5:
        cmp_18_8_80_5:
        cmp r15, false
        jne if_18_5_80_5_end
        if_18_8_80_5_code:
            mov rdi, 1
            exit_18_17_80_5:
                mov rax, 60
                syscall
            exit_18_17_80_5_end:
        if_18_5_80_5_end:
    assert_80_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_83_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_83_5_end:
    cmp_84_12:
    cmp qword [rsp - 40], 2
    jne bool_false_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
    assert_84_5:
        if_18_8_84_5:
        cmp_18_8_84_5:
        cmp r15, false
        jne if_18_5_84_5_end
        if_18_8_84_5_code:
            mov rdi, 1
            exit_18_17_84_5:
                mov rax, 60
                syscall
            exit_18_17_84_5_end:
        if_18_5_84_5_end:
    assert_84_5_end:
    cmp_85_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
    assert_85_5:
        if_18_8_85_5:
        cmp_18_8_85_5:
        cmp r15, false
        jne if_18_5_85_5_end
        if_18_8_85_5_code:
            mov rdi, 1
            exit_18_17_85_5:
                mov rax, 60
                syscall
            exit_18_17_85_5_end:
        if_18_5_85_5_end:
    assert_85_5_end:
    mov qword [rsp - 48], 0
    bar_88_5:
        if_53_8_88_5:
        cmp_53_8_88_5:
        cmp qword [rsp - 48], 0
        jne if_53_5_88_5_end
        if_53_8_88_5_code:
            jmp bar_88_5_end
        if_53_5_88_5_end:
        mov qword [rsp - 48], 0xff
    bar_88_5_end:
    cmp_89_12:
    cmp qword [rsp - 48], 0
    jne bool_false_89_12
    bool_true_89_12:
    mov r15, true
    jmp bool_end_89_12
    bool_false_89_12:
    mov r15, false
    bool_end_89_12:
    assert_89_5:
        if_18_8_89_5:
        cmp_18_8_89_5:
        cmp r15, false
        jne if_18_5_89_5_end
        if_18_8_89_5_code:
            mov rdi, 1
            exit_18_17_89_5:
                mov rax, 60
                syscall
            exit_18_17_89_5_end:
        if_18_5_89_5_end:
    assert_89_5_end:
    mov qword [rsp - 48], 1
    bar_92_5:
        if_53_8_92_5:
        cmp_53_8_92_5:
        cmp qword [rsp - 48], 0
        jne if_53_5_92_5_end
        if_53_8_92_5_code:
            jmp bar_92_5_end
        if_53_5_92_5_end:
        mov qword [rsp - 48], 0xff
    bar_92_5_end:
    cmp_93_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_93_12
    bool_true_93_12:
    mov r15, true
    jmp bool_end_93_12
    bool_false_93_12:
    mov r15, false
    bool_end_93_12:
    assert_93_5:
        if_18_8_93_5:
        cmp_18_8_93_5:
        cmp r15, false
        jne if_18_5_93_5_end
        if_18_8_93_5_code:
            mov rdi, 1
            exit_18_17_93_5:
                mov rax, 60
                syscall
            exit_18_17_93_5_end:
        if_18_5_93_5_end:
    assert_93_5_end:
    mov qword [rsp - 56], 1
    baz_96_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_96_13_end:
    cmp_97_12:
    cmp qword [rsp - 64], 2
    jne bool_false_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
    assert_97_5:
        if_18_8_97_5:
        cmp_18_8_97_5:
        cmp r15, false
        jne if_18_5_97_5_end
        if_18_8_97_5_code:
            mov rdi, 1
            exit_18_17_97_5:
                mov rax, 60
                syscall
            exit_18_17_97_5_end:
        if_18_5_97_5_end:
    assert_97_5_end:
    baz_99_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_99_9_end:
    cmp_100_12:
    cmp qword [rsp - 64], 2
    jne bool_false_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
    assert_100_5:
        if_18_8_100_5:
        cmp_18_8_100_5:
        cmp r15, false
        jne if_18_5_100_5_end
        if_18_8_100_5_code:
            mov rdi, 1
            exit_18_17_100_5:
                mov rax, 60
                syscall
            exit_18_17_100_5_end:
        if_18_5_100_5_end:
    assert_100_5_end:
    baz_102_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_102_23_end:
    mov qword [rsp - 72], 0
    cmp_103_12:
    cmp qword [rsp - 80], 4
    jne bool_false_103_12
    bool_true_103_12:
    mov r15, true
    jmp bool_end_103_12
    bool_false_103_12:
    mov r15, false
    bool_end_103_12:
    assert_103_5:
        if_18_8_103_5:
        cmp_18_8_103_5:
        cmp r15, false
        jne if_18_5_103_5_end
        if_18_8_103_5_code:
            mov rdi, 1
            exit_18_17_103_5:
                mov rax, 60
                syscall
            exit_18_17_103_5_end:
        if_18_5_103_5_end:
    assert_103_5_end:
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
    cmp_109_12:
    cmp qword [rsp - 116], 10
    jne bool_false_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
    assert_109_5:
        if_18_8_109_5:
        cmp_18_8_109_5:
        cmp r15, false
        jne if_18_5_109_5_end
        if_18_8_109_5_code:
            mov rdi, 1
            exit_18_17_109_5:
                mov rax, 60
                syscall
            exit_18_17_109_5_end:
        if_18_5_109_5_end:
    assert_109_5_end:
    cmp_110_12:
    cmp qword [rsp - 108], 2
    jne bool_false_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_18_8_110_5:
        cmp_18_8_110_5:
        cmp r15, false
        jne if_18_5_110_5_end
        if_18_8_110_5_code:
            mov rdi, 1
            exit_18_17_110_5:
                mov rax, 60
                syscall
            exit_18_17_110_5_end:
        if_18_5_110_5_end:
    assert_110_5_end:
    cmp_111_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_18_8_111_5:
        cmp_18_8_111_5:
        cmp r15, false
        jne if_18_5_111_5_end
        if_18_8_111_5_code:
            mov rdi, 1
            exit_18_17_111_5:
                mov rax, 60
                syscall
            exit_18_17_111_5_end:
        if_18_5_111_5_end:
    assert_111_5_end:
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
    cmp_115_12:
    cmp qword [rsp - 116], -1
    jne bool_false_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_18_8_115_5:
        cmp_18_8_115_5:
        cmp r15, false
        jne if_18_5_115_5_end
        if_18_8_115_5_code:
            mov rdi, 1
            exit_18_17_115_5:
                mov rax, 60
                syscall
            exit_18_17_115_5_end:
        if_18_5_115_5_end:
    assert_115_5_end:
    cmp_116_12:
    cmp qword [rsp - 108], -2
    jne bool_false_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
    assert_116_5:
        if_18_8_116_5:
        cmp_18_8_116_5:
        cmp r15, false
        jne if_18_5_116_5_end
        if_18_8_116_5_code:
            mov rdi, 1
            exit_18_17_116_5:
                mov rax, 60
                syscall
            exit_18_17_116_5_end:
        if_18_5_116_5_end:
    assert_116_5_end:
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_119_12:
    cmp qword [rsp - 152], -1
    jne bool_false_119_12
    bool_true_119_12:
    mov r15, true
    jmp bool_end_119_12
    bool_false_119_12:
    mov r15, false
    bool_end_119_12:
    assert_119_5:
        if_18_8_119_5:
        cmp_18_8_119_5:
        cmp r15, false
        jne if_18_5_119_5_end
        if_18_8_119_5_code:
            mov rdi, 1
            exit_18_17_119_5:
                mov rax, 60
                syscall
            exit_18_17_119_5_end:
        if_18_5_119_5_end:
    assert_119_5_end:
    cmp_120_12:
    cmp qword [rsp - 144], -2
    jne bool_false_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_18_8_120_5:
        cmp_18_8_120_5:
        cmp r15, false
        jne if_18_5_120_5_end
        if_18_8_120_5_code:
            mov rdi, 1
            exit_18_17_120_5:
                mov rax, 60
                syscall
            exit_18_17_120_5_end:
        if_18_5_120_5_end:
    assert_120_5_end:
    cmp_121_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_18_8_121_5:
        cmp_18_8_121_5:
        cmp r15, false
        jne if_18_5_121_5_end
        if_18_8_121_5_code:
            mov rdi, 1
            exit_18_17_121_5:
                mov rax, 60
                syscall
            exit_18_17_121_5_end:
        if_18_5_121_5_end:
    assert_121_5_end:
    lea rdi, [rsp - 172]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 164], 73
    cmp_126_12:
        lea r13, [rsp - 172]
        mov r12, 0
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_18_8_126_5:
        cmp_18_8_126_5:
        cmp r15, false
        jne if_18_5_126_5_end
        if_18_8_126_5_code:
            mov rdi, 1
            exit_18_17_126_5:
                mov rax, 60
                syscall
            exit_18_17_126_5_end:
        if_18_5_126_5_end:
    assert_126_5_end:
    lea rdi, [rsp - 252]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_130_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_130_5_end:
    loop_131_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_132_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_132_9_end:
        mov rdx, 80
        lea rsi, [rsp - 252]
        read_133_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 260], rax
        read_133_19_end:
        sub qword [rsp - 260], 1
        if_136_12:
        cmp_136_12:
        cmp qword [rsp - 260], 0
        jne if_138_19
        if_136_12_code:
            jmp loop_131_5_end
        jmp if_136_9_end
        if_138_19:
        cmp_138_19:
        cmp qword [rsp - 260], 4
        jg if_else_136_9
        if_138_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_139_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_139_13_end:
            jmp loop_131_5
        jmp if_136_9_end
        if_else_136_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_142_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_142_13_end:
            mov rdx, qword [rsp - 260]
            lea rsi, [rsp - 252]
            print_143_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_143_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_144_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_144_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_145_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_145_13_end:
        if_136_9_end:
    jmp loop_131_5
    loop_131_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
