section .bss
stk resd 0x10000
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz',10,''
hello.len equ $-hello
input: db '............................................................'
input.len equ $-input
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
    cmp_76_12:
        mov r13, 1
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_76_12
    jmp bool_true_76_12
    bool_true_76_12:
    mov r15, true
    jmp bool_end_76_12
    bool_false_76_12:
    mov r15, false
    bool_end_76_12:
    assert_76_5:
        if_36_8_76_5:
        cmp_36_8_76_5:
        cmp r15, false
        jne if_36_5_76_5_end
        jmp if_36_8_76_5_code
        if_36_8_76_5_code:
            mov rdi, 1
            exit_36_17_76_5:
                mov rax, 60
                syscall
            exit_36_17_76_5_end:
        if_36_5_76_5_end:
    assert_76_5_end:
    cmp_77_12:
        mov r13, 2
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_77_12
    jmp bool_true_77_12
    bool_true_77_12:
    mov r15, true
    jmp bool_end_77_12
    bool_false_77_12:
    mov r15, false
    bool_end_77_12:
    assert_77_5:
        if_36_8_77_5:
        cmp_36_8_77_5:
        cmp r15, false
        jne if_36_5_77_5_end
        jmp if_36_8_77_5_code
        if_36_8_77_5_code:
            mov rdi, 1
            exit_36_17_77_5:
                mov rax, 60
                syscall
            exit_36_17_77_5_end:
        if_36_5_77_5_end:
    assert_77_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r13, qword [rsp - 24]
    sub r13, 1
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_80_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_80_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_81_12:
        mov r13, qword [rsp - 24]
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_81_12
    jmp bool_true_81_12
    bool_true_81_12:
    mov r15, true
    jmp bool_end_81_12
    bool_false_81_12:
    mov r15, false
    bool_end_81_12:
    assert_81_5:
        if_36_8_81_5:
        cmp_36_8_81_5:
        cmp r15, false
        jne if_36_5_81_5_end
        jmp if_36_8_81_5_code
        if_36_8_81_5_code:
            mov rdi, 1
            exit_36_17_81_5:
                mov rax, 60
                syscall
            exit_36_17_81_5_end:
        if_36_5_81_5_end:
    assert_81_5_end:
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    foo_84_5:
        mov qword [rsp - 40], 0b10
        mov qword [rsp - 32], 0xb
    foo_84_5_end:
    cmp_85_12:
    cmp qword [rsp - 40], 2
    jne bool_false_85_12
    jmp bool_true_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
    assert_85_5:
        if_36_8_85_5:
        cmp_36_8_85_5:
        cmp r15, false
        jne if_36_5_85_5_end
        jmp if_36_8_85_5_code
        if_36_8_85_5_code:
            mov rdi, 1
            exit_36_17_85_5:
                mov rax, 60
                syscall
            exit_36_17_85_5_end:
        if_36_5_85_5_end:
    assert_85_5_end:
    cmp_86_12:
    cmp qword [rsp - 32], 0xb
    jne bool_false_86_12
    jmp bool_true_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
    assert_86_5:
        if_36_8_86_5:
        cmp_36_8_86_5:
        cmp r15, false
        jne if_36_5_86_5_end
        jmp if_36_8_86_5_code
        if_36_8_86_5_code:
            mov rdi, 1
            exit_36_17_86_5:
                mov rax, 60
                syscall
            exit_36_17_86_5_end:
        if_36_5_86_5_end:
    assert_86_5_end:
    mov qword [rsp - 48], 0
    bar_89_5:
        if_54_8_89_5:
        cmp_54_8_89_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_89_5_end
        jmp if_54_8_89_5_code
        if_54_8_89_5_code:
            jmp bar_89_5_end
        if_54_5_89_5_end:
        mov qword [rsp - 48], 0xff
    bar_89_5_end:
    cmp_90_12:
    cmp qword [rsp - 48], 0
    jne bool_false_90_12
    jmp bool_true_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
    assert_90_5:
        if_36_8_90_5:
        cmp_36_8_90_5:
        cmp r15, false
        jne if_36_5_90_5_end
        jmp if_36_8_90_5_code
        if_36_8_90_5_code:
            mov rdi, 1
            exit_36_17_90_5:
                mov rax, 60
                syscall
            exit_36_17_90_5_end:
        if_36_5_90_5_end:
    assert_90_5_end:
    mov qword [rsp - 48], 1
    bar_93_5:
        if_54_8_93_5:
        cmp_54_8_93_5:
        cmp qword [rsp - 48], 0
        jne if_54_5_93_5_end
        jmp if_54_8_93_5_code
        if_54_8_93_5_code:
            jmp bar_93_5_end
        if_54_5_93_5_end:
        mov qword [rsp - 48], 0xff
    bar_93_5_end:
    cmp_94_12:
    cmp qword [rsp - 48], 0xff
    jne bool_false_94_12
    jmp bool_true_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
    assert_94_5:
        if_36_8_94_5:
        cmp_36_8_94_5:
        cmp r15, false
        jne if_36_5_94_5_end
        jmp if_36_8_94_5_code
        if_36_8_94_5_code:
            mov rdi, 1
            exit_36_17_94_5:
                mov rax, 60
                syscall
            exit_36_17_94_5_end:
        if_36_5_94_5_end:
    assert_94_5_end:
    mov qword [rsp - 56], 1
    baz_97_13:
        mov r15, qword [rsp - 56]
        mov qword [rsp - 64], r15
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_97_13_end:
    cmp_98_12:
    cmp qword [rsp - 64], 2
    jne bool_false_98_12
    jmp bool_true_98_12
    bool_true_98_12:
    mov r15, true
    jmp bool_end_98_12
    bool_false_98_12:
    mov r15, false
    bool_end_98_12:
    assert_98_5:
        if_36_8_98_5:
        cmp_36_8_98_5:
        cmp r15, false
        jne if_36_5_98_5_end
        jmp if_36_8_98_5_code
        if_36_8_98_5_code:
            mov rdi, 1
            exit_36_17_98_5:
                mov rax, 60
                syscall
            exit_36_17_98_5_end:
        if_36_5_98_5_end:
    assert_98_5_end:
    baz_100_9:
        mov qword [rsp - 64], 1
        mov r15, qword [rsp - 64]
        imul r15, 2
        mov qword [rsp - 64], r15
    baz_100_9_end:
    cmp_101_12:
    cmp qword [rsp - 64], 2
    jne bool_false_101_12
    jmp bool_true_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
    assert_101_5:
        if_36_8_101_5:
        cmp_36_8_101_5:
        cmp r15, false
        jne if_36_5_101_5_end
        jmp if_36_8_101_5_code
        if_36_8_101_5_code:
            mov rdi, 1
            exit_36_17_101_5:
                mov rax, 60
                syscall
            exit_36_17_101_5_end:
        if_36_5_101_5_end:
    assert_101_5_end:
    baz_103_23:
        mov qword [rsp - 80], 2
        mov r15, qword [rsp - 80]
        imul r15, 2
        mov qword [rsp - 80], r15
    baz_103_23_end:
    mov qword [rsp - 72], 0
    cmp_104_12:
    cmp qword [rsp - 80], 4
    jne bool_false_104_12
    jmp bool_true_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_36_8_104_5:
        cmp_36_8_104_5:
        cmp r15, false
        jne if_36_5_104_5_end
        jmp if_36_8_104_5_code
        if_36_8_104_5_code:
            mov rdi, 1
            exit_36_17_104_5:
                mov rax, 60
                syscall
            exit_36_17_104_5_end:
        if_36_5_104_5_end:
    assert_104_5_end:
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
    cmp_110_12:
    cmp qword [rsp - 116], 10
    jne bool_false_110_12
    jmp bool_true_110_12
    bool_true_110_12:
    mov r15, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15, false
    bool_end_110_12:
    assert_110_5:
        if_36_8_110_5:
        cmp_36_8_110_5:
        cmp r15, false
        jne if_36_5_110_5_end
        jmp if_36_8_110_5_code
        if_36_8_110_5_code:
            mov rdi, 1
            exit_36_17_110_5:
                mov rax, 60
                syscall
            exit_36_17_110_5_end:
        if_36_5_110_5_end:
    assert_110_5_end:
    cmp_111_12:
    cmp qword [rsp - 108], 2
    jne bool_false_111_12
    jmp bool_true_111_12
    bool_true_111_12:
    mov r15, true
    jmp bool_end_111_12
    bool_false_111_12:
    mov r15, false
    bool_end_111_12:
    assert_111_5:
        if_36_8_111_5:
        cmp_36_8_111_5:
        cmp r15, false
        jne if_36_5_111_5_end
        jmp if_36_8_111_5_code
        if_36_8_111_5_code:
            mov rdi, 1
            exit_36_17_111_5:
                mov rax, 60
                syscall
            exit_36_17_111_5_end:
        if_36_5_111_5_end:
    assert_111_5_end:
    cmp_112_12:
    cmp dword [rsp - 100], 0xff0000
    jne bool_false_112_12
    jmp bool_true_112_12
    bool_true_112_12:
    mov r15, true
    jmp bool_end_112_12
    bool_false_112_12:
    mov r15, false
    bool_end_112_12:
    assert_112_5:
        if_36_8_112_5:
        cmp_36_8_112_5:
        cmp r15, false
        jne if_36_5_112_5_end
        jmp if_36_8_112_5_code
        if_36_8_112_5_code:
            mov rdi, 1
            exit_36_17_112_5:
                mov rax, 60
                syscall
            exit_36_17_112_5_end:
        if_36_5_112_5_end:
    assert_112_5_end:
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
    cmp_116_12:
    cmp qword [rsp - 116], -1
    jne bool_false_116_12
    jmp bool_true_116_12
    bool_true_116_12:
    mov r15, true
    jmp bool_end_116_12
    bool_false_116_12:
    mov r15, false
    bool_end_116_12:
    assert_116_5:
        if_36_8_116_5:
        cmp_36_8_116_5:
        cmp r15, false
        jne if_36_5_116_5_end
        jmp if_36_8_116_5_code
        if_36_8_116_5_code:
            mov rdi, 1
            exit_36_17_116_5:
                mov rax, 60
                syscall
            exit_36_17_116_5_end:
        if_36_5_116_5_end:
    assert_116_5_end:
    cmp_117_12:
    cmp qword [rsp - 108], -2
    jne bool_false_117_12
    jmp bool_true_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
    assert_117_5:
        if_36_8_117_5:
        cmp_36_8_117_5:
        cmp r15, false
        jne if_36_5_117_5_end
        jmp if_36_8_117_5_code
        if_36_8_117_5_code:
            mov rdi, 1
            exit_36_17_117_5:
                mov rax, 60
                syscall
            exit_36_17_117_5_end:
        if_36_5_117_5_end:
    assert_117_5_end:
    lea rdi, [rsp - 152]
    lea rsi, [rsp - 116]
    mov rcx, 20
    rep movsb
    cmp_120_12:
    cmp qword [rsp - 152], -1
    jne bool_false_120_12
    jmp bool_true_120_12
    bool_true_120_12:
    mov r15, true
    jmp bool_end_120_12
    bool_false_120_12:
    mov r15, false
    bool_end_120_12:
    assert_120_5:
        if_36_8_120_5:
        cmp_36_8_120_5:
        cmp r15, false
        jne if_36_5_120_5_end
        jmp if_36_8_120_5_code
        if_36_8_120_5_code:
            mov rdi, 1
            exit_36_17_120_5:
                mov rax, 60
                syscall
            exit_36_17_120_5_end:
        if_36_5_120_5_end:
    assert_120_5_end:
    cmp_121_12:
    cmp qword [rsp - 144], -2
    jne bool_false_121_12
    jmp bool_true_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_36_8_121_5:
        cmp_36_8_121_5:
        cmp r15, false
        jne if_36_5_121_5_end
        jmp if_36_8_121_5_code
        if_36_8_121_5_code:
            mov rdi, 1
            exit_36_17_121_5:
                mov rax, 60
                syscall
            exit_36_17_121_5_end:
        if_36_5_121_5_end:
    assert_121_5_end:
    cmp_122_12:
    cmp dword [rsp - 136], 0xff0000
    jne bool_false_122_12
    jmp bool_true_122_12
    bool_true_122_12:
    mov r15, true
    jmp bool_end_122_12
    bool_false_122_12:
    mov r15, false
    bool_end_122_12:
    assert_122_5:
        if_36_8_122_5:
        cmp_36_8_122_5:
        cmp r15, false
        jne if_36_5_122_5_end
        jmp if_36_8_122_5_code
        if_36_8_122_5_code:
            mov rdi, 1
            exit_36_17_122_5:
                mov rax, 60
                syscall
            exit_36_17_122_5_end:
        if_36_5_122_5_end:
    assert_122_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_124_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_124_5_end:
    loop_125_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_126_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_126_9_end:
        mov rdx, input.len
        mov rsi, input
        read_127_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 160], rax
        read_127_19_end:
        sub qword [rsp - 160], 1
        if_128_12:
        cmp_128_12:
        cmp qword [rsp - 160], 0
        jne if_130_19
        jmp if_128_12_code
        if_128_12_code:
            jmp loop_125_5_end
        jmp if_128_9_end
        if_130_19:
        cmp_130_19:
        cmp qword [rsp - 160], 4
        jg if_else_128_9
        jmp if_130_19_code
        if_130_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_131_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_131_13_end:
            jmp loop_125_5
        jmp if_128_9_end
        if_else_128_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_134_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_134_13_end:
            mov rdx, qword [rsp - 160]
            mov rsi, input
            print_135_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_135_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_136_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_136_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_137_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_137_13_end:
        if_128_9_end:
    jmp loop_125_5
    loop_125_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
