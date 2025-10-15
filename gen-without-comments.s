DEFAULT REL
section .bss
stk resd 65536
stk.end:
true equ 1
false equ 0
section .data
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 8640]
    mov rcx, 8640
    xor rax, rax
    rep stosb
    baz_47_5:
        lea r15, [rsp - 8640]
        mov r14, 1
        imul r14, 4320
        add r15, r14
        add r15, 8
        bar_42_5_47_5:
            mov r14, r15
            mov r13, 2
            imul r13, 616
            add r14, r13
            mov r13, 1
            imul r13, 69
            add r14, r13
            add r14, 64
            foo_37_5_42_5_47_5:
                mov dword [r14 + 65], 2
                mov byte [r14 + 64], 1
                mov r13, r14
                mov r12, 0
                mov byte [r13 + r12 + 0], 0x61
            foo_37_5_42_5_47_5_end:
            mov r14, r15
            mov r13, 3
            imul r13, 616
            add r14, r13
            mov r13, 5
            imul r13, 69
            add r14, r13
            add r14, 64
            foo_38_5_42_5_47_5:
                mov dword [r14 + 65], 2
                mov byte [r14 + 64], 1
                mov r13, r14
                mov r12, 0
                mov byte [r13 + r12 + 0], 0x61
            foo_38_5_42_5_47_5_end:
        bar_42_5_47_5_end:
    baz_47_5_end:
    cmp_48_12:
        lea r13, [rsp - 8640]
        mov r12, 1
        imul r12, 4320
        add r13, r12
        mov r12, 2
        imul r12, 616
        add r13, r12
        mov r12, 1
        imul r12, 69
        add r13, r12
        add r13, 137
        movsx r14, dword [r13]
    cmp r14, 2
    jne bool_false_48_12
    jmp bool_true_48_12
    bool_true_48_12:
    mov r15, true
    jmp bool_end_48_12
    bool_false_48_12:
    mov r15, false
    bool_end_48_12:
    assert_48_5:
        if_8_8_48_5:
        cmp_8_8_48_5:
        test r15, r15
        jne if_8_5_48_5_end
        jmp if_8_8_48_5_code
        if_8_8_48_5_code:
            mov rdi, 1
            exit_8_17_48_5:
                mov rax, 60
                syscall
            exit_8_17_48_5_end:
        if_8_5_48_5_end:
    assert_48_5_end:
    cmp_49_12:
        lea r13, [rsp - 8640]
        mov r12, 1
        imul r12, 4320
        add r13, r12
        mov r12, 2
        imul r12, 616
        add r13, r12
        mov r12, 1
        imul r12, 69
        add r13, r12
        add r13, 136
        movsx r14, byte [r13]
    cmp r14, 1
    jne bool_false_49_12
    jmp bool_true_49_12
    bool_true_49_12:
    mov r15, true
    jmp bool_end_49_12
    bool_false_49_12:
    mov r15, false
    bool_end_49_12:
    assert_49_5:
        if_8_8_49_5:
        cmp_8_8_49_5:
        test r15, r15
        jne if_8_5_49_5_end
        jmp if_8_8_49_5_code
        if_8_8_49_5_code:
            mov rdi, 1
            exit_8_17_49_5:
                mov rax, 60
                syscall
            exit_8_17_49_5_end:
        if_8_5_49_5_end:
    assert_49_5_end:
    cmp_50_12:
        lea r13, [rsp - 8640]
        mov r12, 1
        imul r12, 4320
        add r13, r12
        mov r12, 2
        imul r12, 616
        add r13, r12
        mov r12, 1
        imul r12, 69
        add r13, r12
        mov r12, 0
        movsx r14, byte [r13 + r12 + 72]
    cmp r14, 0x61
    jne bool_false_50_12
    jmp bool_true_50_12
    bool_true_50_12:
    mov r15, true
    jmp bool_end_50_12
    bool_false_50_12:
    mov r15, false
    bool_end_50_12:
    assert_50_5:
        if_8_8_50_5:
        cmp_8_8_50_5:
        test r15, r15
        jne if_8_5_50_5_end
        jmp if_8_8_50_5_code
        if_8_8_50_5_code:
            mov rdi, 1
            exit_8_17_50_5:
                mov rax, 60
                syscall
            exit_8_17_50_5_end:
        if_8_5_50_5_end:
    assert_50_5_end:
    cmp_51_12:
        lea r13, [rsp - 8640]
        mov r12, 1
        imul r12, 4320
        add r13, r12
        mov r12, 3
        imul r12, 616
        add r13, r12
        mov r12, 5
        imul r12, 69
        add r13, r12
        add r13, 137
        movsx r14, dword [r13]
    cmp r14, 2
    jne bool_false_51_12
    jmp bool_true_51_12
    bool_true_51_12:
    mov r15, true
    jmp bool_end_51_12
    bool_false_51_12:
    mov r15, false
    bool_end_51_12:
    assert_51_5:
        if_8_8_51_5:
        cmp_8_8_51_5:
        test r15, r15
        jne if_8_5_51_5_end
        jmp if_8_8_51_5_code
        if_8_8_51_5_code:
            mov rdi, 1
            exit_8_17_51_5:
                mov rax, 60
                syscall
            exit_8_17_51_5_end:
        if_8_5_51_5_end:
    assert_51_5_end:
    cmp_52_12:
        lea r13, [rsp - 8640]
        mov r12, 1
        imul r12, 4320
        add r13, r12
        mov r12, 3
        imul r12, 616
        add r13, r12
        mov r12, 5
        imul r12, 69
        add r13, r12
        add r13, 136
        movsx r14, byte [r13]
    cmp r14, 1
    jne bool_false_52_12
    jmp bool_true_52_12
    bool_true_52_12:
    mov r15, true
    jmp bool_end_52_12
    bool_false_52_12:
    mov r15, false
    bool_end_52_12:
    assert_52_5:
        if_8_8_52_5:
        cmp_8_8_52_5:
        test r15, r15
        jne if_8_5_52_5_end
        jmp if_8_8_52_5_code
        if_8_8_52_5_code:
            mov rdi, 1
            exit_8_17_52_5:
                mov rax, 60
                syscall
            exit_8_17_52_5_end:
        if_8_5_52_5_end:
    assert_52_5_end:
    cmp_53_12:
        lea r13, [rsp - 8640]
        mov r12, 1
        imul r12, 4320
        add r13, r12
        mov r12, 3
        imul r12, 616
        add r13, r12
        mov r12, 5
        imul r12, 69
        add r13, r12
        mov r12, 0
        movsx r14, byte [r13 + r12 + 72]
    cmp r14, 0x61
    jne bool_false_53_12
    jmp bool_true_53_12
    bool_true_53_12:
    mov r15, true
    jmp bool_end_53_12
    bool_false_53_12:
    mov r15, false
    bool_end_53_12:
    assert_53_5:
        if_8_8_53_5:
        cmp_8_8_53_5:
        test r15, r15
        jne if_8_5_53_5_end
        jmp if_8_8_53_5_code
        if_8_8_53_5_code:
            mov rdi, 1
            exit_8_17_53_5:
                mov rax, 60
                syscall
            exit_8_17_53_5_end:
        if_8_5_53_5_end:
    assert_53_5_end:
mov rdi, 0
mov rax, 60
syscall
    mov rax, 60
    mov rdi, 0
    syscall
