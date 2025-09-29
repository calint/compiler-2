section .bss
stk resd 1024
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
counter: dq 0
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    mov qword[rsp-16], 0
    mov qword[rsp-8], 0
    foo_67_5:
        mov qword[rsp-16], 0b10
        mov qword[rsp-8], 0xb
        assert_51_5_67_5:
            cmp_51_12_67_5:
            cmp qword[rsp-16], 0b10
            jne bool_false_51_12_67_5
            bool_true_51_12_67_5:  ; opt1
            mov r15, true
            jmp bool_end_51_12_67_5
            bool_false_51_12_67_5:
            mov r15, false
            bool_end_51_12_67_5:
            if_20_8_51_5_67_5:
            cmp_20_8_51_5_67_5:
            cmp r15, false
            jne if_20_5_51_5_67_5_end
            if_20_8_51_5_67_5_code:  ; opt1
                exit_20_17_51_5_67_5:
                    mov rdi, 1
                    mov rax, 60
                    syscall
                exit_20_17_51_5_67_5_end:
            if_20_5_51_5_67_5_end:
        assert_51_5_67_5_end:
    foo_67_5_end:
    assert_68_5:
        cmp_68_12:
        cmp qword[rsp-16], 2
        jne bool_false_68_12
        bool_true_68_12:  ; opt1
        mov r15, true
        jmp bool_end_68_12
        bool_false_68_12:
        mov r15, false
        bool_end_68_12:
        if_20_8_68_5:
        cmp_20_8_68_5:
        cmp r15, false
        jne if_20_5_68_5_end
        if_20_8_68_5_code:  ; opt1
            exit_20_17_68_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_68_5_end:
        if_20_5_68_5_end:
    assert_68_5_end:
    assert_69_5:
        cmp_69_12:
        cmp qword[rsp-8], 0xb
        jne bool_false_69_12
        bool_true_69_12:  ; opt1
        mov r15, true
        jmp bool_end_69_12
        bool_false_69_12:
        mov r15, false
        bool_end_69_12:
        if_20_8_69_5:
        cmp_20_8_69_5:
        cmp r15, false
        jne if_20_5_69_5_end
        if_20_8_69_5_code:  ; opt1
            exit_20_17_69_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_69_5_end:
        if_20_5_69_5_end:
    assert_69_5_end:
    mov qword[rsp-24], 0
    bar_72_5:
        if_56_8_72_5:
        cmp_56_8_72_5:
        cmp qword[rsp-24], 0
        jne if_56_5_72_5_end
        if_56_8_72_5_code:  ; opt1
            jmp bar_72_5_end
        if_56_5_72_5_end:
        mov qword[rsp-24], 0xff
    bar_72_5_end:
    assert_73_5:
        cmp_73_12:
        cmp qword[rsp-24], 0
        jne bool_false_73_12
        bool_true_73_12:  ; opt1
        mov r15, true
        jmp bool_end_73_12
        bool_false_73_12:
        mov r15, false
        bool_end_73_12:
        if_20_8_73_5:
        cmp_20_8_73_5:
        cmp r15, false
        jne if_20_5_73_5_end
        if_20_8_73_5_code:  ; opt1
            exit_20_17_73_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_73_5_end:
        if_20_5_73_5_end:
    assert_73_5_end:
    mov qword[rsp-24], 1
    bar_76_5:
        if_56_8_76_5:
        cmp_56_8_76_5:
        cmp qword[rsp-24], 0
        jne if_56_5_76_5_end
        if_56_8_76_5_code:  ; opt1
            jmp bar_76_5_end
        if_56_5_76_5_end:
        mov qword[rsp-24], 0xff
    bar_76_5_end:
    assert_77_5:
        cmp_77_12:
        cmp qword[rsp-24], 0xff
        jne bool_false_77_12
        bool_true_77_12:  ; opt1
        mov r15, true
        jmp bool_end_77_12
        bool_false_77_12:
        mov r15, false
        bool_end_77_12:
        if_20_8_77_5:
        cmp_20_8_77_5:
        cmp r15, false
        jne if_20_5_77_5_end
        if_20_8_77_5_code:  ; opt1
            exit_20_17_77_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_77_5_end:
        if_20_5_77_5_end:
    assert_77_5_end:
    mov qword[rsp-32], 1
    baz_80_13:
        mov r15, qword[rsp-32]
        mov qword[rsp-40], r15
        mov r15, qword[rsp-40]
        imul r15, 2
        mov qword[rsp-40], r15
    baz_80_13_end:
    assert_81_5:
        cmp_81_12:
        cmp qword[rsp-40], 2
        jne bool_false_81_12
        bool_true_81_12:  ; opt1
        mov r15, true
        jmp bool_end_81_12
        bool_false_81_12:
        mov r15, false
        bool_end_81_12:
        if_20_8_81_5:
        cmp_20_8_81_5:
        cmp r15, false
        jne if_20_5_81_5_end
        if_20_8_81_5_code:  ; opt1
            exit_20_17_81_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_81_5_end:
        if_20_5_81_5_end:
    assert_81_5_end:
    baz_83_9:
        mov qword[rsp-40], 1
        mov r15, qword[rsp-40]
        imul r15, 2
        mov qword[rsp-40], r15
    baz_83_9_end:
    assert_84_5:
        cmp_84_12:
        cmp qword[rsp-40], 2
        jne bool_false_84_12
        bool_true_84_12:  ; opt1
        mov r15, true
        jmp bool_end_84_12
        bool_false_84_12:
        mov r15, false
        bool_end_84_12:
        if_20_8_84_5:
        cmp_20_8_84_5:
        cmp r15, false
        jne if_20_5_84_5_end
        if_20_8_84_5_code:  ; opt1
            exit_20_17_84_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_84_5_end:
        if_20_5_84_5_end:
    assert_84_5_end:
    baz_86_23:
        mov qword[rsp-56], 2
        mov r15, qword[rsp-56]
        imul r15, 2
        mov qword[rsp-56], r15
    baz_86_23_end:
    mov qword[rsp-48], 0
    assert_87_5:
        cmp_87_12:
        cmp qword[rsp-56], 4
        jne bool_false_87_12
        bool_true_87_12:  ; opt1
        mov r15, true
        jmp bool_end_87_12
        bool_false_87_12:
        mov r15, false
        bool_end_87_12:
        if_20_8_87_5:
        cmp_20_8_87_5:
        cmp r15, false
        jne if_20_5_87_5_end
        if_20_8_87_5_code:  ; opt1
            exit_20_17_87_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_87_5_end:
        if_20_5_87_5_end:
    assert_87_5_end:
    mov qword[rsp-64], 1
    mov qword[rsp-72], 2
        mov r15, qword[rsp-64]
        mov qword[rsp-92], r15
        mov r15, qword[rsp-92]
        imul r15, 10
        mov qword[rsp-92], r15
        mov r15, qword[rsp-72]
        mov qword[rsp-84], r15
    mov dword[rsp-76], 0xff0000
    assert_93_5:
        cmp_93_12:
        cmp qword[rsp-92], 10
        jne bool_false_93_12
        bool_true_93_12:  ; opt1
        mov r15, true
        jmp bool_end_93_12
        bool_false_93_12:
        mov r15, false
        bool_end_93_12:
        if_20_8_93_5:
        cmp_20_8_93_5:
        cmp r15, false
        jne if_20_5_93_5_end
        if_20_8_93_5_code:  ; opt1
            exit_20_17_93_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_93_5_end:
        if_20_5_93_5_end:
    assert_93_5_end:
    assert_94_5:
        cmp_94_12:
        cmp qword[rsp-84], 2
        jne bool_false_94_12
        bool_true_94_12:  ; opt1
        mov r15, true
        jmp bool_end_94_12
        bool_false_94_12:
        mov r15, false
        bool_end_94_12:
        if_20_8_94_5:
        cmp_20_8_94_5:
        cmp r15, false
        jne if_20_5_94_5_end
        if_20_8_94_5_code:  ; opt1
            exit_20_17_94_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_94_5_end:
        if_20_5_94_5_end:
    assert_94_5_end:
    assert_95_5:
        cmp_95_12:
        cmp dword[rsp-76], 0xff0000
        jne bool_false_95_12
        bool_true_95_12:  ; opt1
        mov r15, true
        jmp bool_end_95_12
        bool_false_95_12:
        mov r15, false
        bool_end_95_12:
        if_20_8_95_5:
        cmp_20_8_95_5:
        cmp r15, false
        jne if_20_5_95_5_end
        if_20_8_95_5_code:  ; opt1
            exit_20_17_95_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_95_5_end:
        if_20_5_95_5_end:
    assert_95_5_end:
    mov r15, qword[rsp-64]
    mov qword[rsp-108], r15
    neg qword[rsp-108]
    mov r15, qword[rsp-72]
    mov qword[rsp-100], r15
    neg qword[rsp-100]
    mov r15, qword[rsp-108]
    mov qword[rsp-92], r15
    mov r15, qword[rsp-100]
    mov qword[rsp-84], r15
    assert_99_5:
        cmp_99_12:
        cmp qword[rsp-92], -1
        jne bool_false_99_12
        bool_true_99_12:  ; opt1
        mov r15, true
        jmp bool_end_99_12
        bool_false_99_12:
        mov r15, false
        bool_end_99_12:
        if_20_8_99_5:
        cmp_20_8_99_5:
        cmp r15, false
        jne if_20_5_99_5_end
        if_20_8_99_5_code:  ; opt1
            exit_20_17_99_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_99_5_end:
        if_20_5_99_5_end:
    assert_99_5_end:
    assert_100_5:
        cmp_100_12:
        cmp qword[rsp-84], -2
        jne bool_false_100_12
        bool_true_100_12:  ; opt1
        mov r15, true
        jmp bool_end_100_12
        bool_false_100_12:
        mov r15, false
        bool_end_100_12:
        if_20_8_100_5:
        cmp_20_8_100_5:
        cmp r15, false
        jne if_20_5_100_5_end
        if_20_8_100_5_code:  ; opt1
            exit_20_17_100_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_100_5_end:
        if_20_5_100_5_end:
    assert_100_5_end:
        mov r15, qword[rsp-92]
        mov qword[rsp-128], r15
        mov r15, qword[rsp-84]
        mov qword[rsp-120], r15
    mov r15d, dword[rsp-76]
    mov dword[rsp-112], r15d
    assert_103_5:
        cmp_103_12:
        cmp qword[rsp-128], -1
        jne bool_false_103_12
        bool_true_103_12:  ; opt1
        mov r15, true
        jmp bool_end_103_12
        bool_false_103_12:
        mov r15, false
        bool_end_103_12:
        if_20_8_103_5:
        cmp_20_8_103_5:
        cmp r15, false
        jne if_20_5_103_5_end
        if_20_8_103_5_code:  ; opt1
            exit_20_17_103_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_103_5_end:
        if_20_5_103_5_end:
    assert_103_5_end:
    assert_104_5:
        cmp_104_12:
        cmp qword[rsp-120], -2
        jne bool_false_104_12
        bool_true_104_12:  ; opt1
        mov r15, true
        jmp bool_end_104_12
        bool_false_104_12:
        mov r15, false
        bool_end_104_12:
        if_20_8_104_5:
        cmp_20_8_104_5:
        cmp r15, false
        jne if_20_5_104_5_end
        if_20_8_104_5_code:  ; opt1
            exit_20_17_104_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_104_5_end:
        if_20_5_104_5_end:
    assert_104_5_end:
    assert_105_5:
        cmp_105_12:
        cmp dword[rsp-112], 0xff0000
        jne bool_false_105_12
        bool_true_105_12:  ; opt1
        mov r15, true
        jmp bool_end_105_12
        bool_false_105_12:
        mov r15, false
        bool_end_105_12:
        if_20_8_105_5:
        cmp_20_8_105_5:
        cmp r15, false
        jne if_20_5_105_5_end
        if_20_8_105_5_code:  ; opt1
            exit_20_17_105_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_105_5_end:
        if_20_5_105_5_end:
    assert_105_5_end:
    add qword[counter], 1
    assert_108_5:
        cmp_108_12:
        cmp qword[counter], 1
        jne bool_false_108_12
        bool_true_108_12:  ; opt1
        mov r15, true
        jmp bool_end_108_12
        bool_false_108_12:
        mov r15, false
        bool_end_108_12:
        if_20_8_108_5:
        cmp_20_8_108_5:
        cmp r15, false
        jne if_20_5_108_5_end
        if_20_8_108_5_code:  ; opt1
            exit_20_17_108_5:
                mov rdi, 1
                mov rax, 60
                syscall
            exit_20_17_108_5_end:
        if_20_5_108_5_end:
    assert_108_5_end:
    print_110_5:
        mov rdx, hello.len
        mov rsi, hello
        mov rax, 1
        mov rdi, 1
        syscall
    print_110_5_end:
    loop_111_5:
        print_112_9:
            mov rdx, prompt1.len
            mov rsi, prompt1
            mov rax, 1
            mov rdi, 1
            syscall
        print_112_9_end:
        read_113_19:
            mov rdx, input.len
            mov rsi, input
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword[rsp-136], rax
        read_113_19_end:
        sub qword[rsp-136], 1
        if_114_12:
        cmp_114_12:
        cmp qword[rsp-136], 0
        jne if_116_19
        if_114_12_code:  ; opt1
            jmp loop_111_5_end
        jmp if_114_9_end
        if_116_19:
        cmp_116_19:
        cmp qword[rsp-136], 4
        jg if_else_114_9
        if_116_19_code:  ; opt1
            print_117_13:
                mov rdx, prompt2.len
                mov rsi, prompt2
                mov rax, 1
                mov rdi, 1
                syscall
            print_117_13_end:
            jmp loop_111_5
        jmp if_114_9_end
        if_else_114_9:
            print_120_13:
                mov rdx, prompt3.len
                mov rsi, prompt3
                mov rax, 1
                mov rdi, 1
                syscall
            print_120_13_end:
            print_121_13:
                mov rdx, qword[rsp-136]
                mov rsi, input
                mov rax, 1
                mov rdi, 1
                syscall
            print_121_13_end:
            print_122_13:
                mov rdx, dot.len
                mov rsi, dot
                mov rax, 1
                mov rdi, 1
                syscall
            print_122_13_end:
            print_123_13:
                mov rdx, nl.len
                mov rsi, nl
                mov rax, 1
                mov rdi, 1
                syscall
            print_123_13_end:
        if_114_9_end:
    jmp loop_111_5
    loop_111_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
