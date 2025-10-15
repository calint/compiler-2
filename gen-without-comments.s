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
    lea rdi, [rsp - 10432]
    mov rcx, 10432
    xor rax, rax
    rep stosb
    lea r15, [rsp - 10432]
    mov r14, 0
    imul r14, 5216
    add r15, r14
    lea r14, [rsp - 10432]
    mov r13, 1
    imul r13, 5216
    add r14, r13
    fuz_45_5:
        mov r13, r15
        mov r12, 2
        imul r12, 744
        add r13, r12
        mov r12, 1
        imul r12, 85
        add r13, r12
        add r13, 153
        mov dword [r13], 0xba
        mov r13, r15
        mov r12, 2
        imul r12, 744
        add r13, r12
        mov r12, 1
        imul r12, 85
        add r13, r12
        add r13, 80
        mov qword [r13], 0xab
        mov rcx, 8
        mov r13, r15
        mov r12, 2
        imul r12, 744
        add r13, r12
        add r13, 72
        lea rsi, [r13]
        mov r13, r14
        mov r12, 0
        imul r12, 744
        add r13, r12
        add r13, 72
        lea rdi, [r13]
        imul rcx, 85
        rep movsb
    fuz_45_5_end:
    cmp_47_21:
        mov rcx, 8
        lea r14, [rsp - 10432]
        mov r13, 0
        imul r13, 5216
        add r14, r13
        mov r13, 2
        imul r13, 744
        add r14, r13
        add r14, 72
        lea rsi, [r14]
        lea r14, [rsp - 10432]
        mov r13, 1
        imul r13, 5216
        add r14, r13
        mov r13, 0
        imul r13, 744
        add r14, r13
        add r14, 72
        lea rdi, [r14]
        imul rcx, 85
        repe cmpsb
        je cmps_eq_47_21
        mov r15, false
        jmp cmps_end_47_21
        cmps_eq_47_21:
        mov r15, true
        cmps_end_47_21:
    test r15, r15
    je bool_false_47_21
    jmp bool_true_47_21
    bool_true_47_21:
    mov byte [rsp - 10433], true
    jmp bool_end_47_21
    bool_false_47_21:
    mov byte [rsp - 10433], false
    bool_end_47_21:
    cmp_52_12:
    mov r14b, byte [rsp - 10433]
    test byte [rsp - 10433], r14b
    je bool_false_52_12
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
    mov rax, 60
    mov rdi, 0
    syscall
