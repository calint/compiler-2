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
    mov r14, 1
    imul r14, 5216
    add r15, r14
    mov r14, 2
    imul r14, 744
    add r15, r14
    add r15, 72
    fuz_41_5:
        mov r14, r15
        mov r13, 1
        imul r13, 85
        add r14, r13
        add r14, 81
        mov dword [r14], 0xba
        mov r14, r15
        mov r13, 1
        imul r13, 85
        add r14, r13
        add r14, 8
        mov qword [r14], 0xab
        mov rcx, 1
        mov r14, r15
        mov r13, 1
        imul r13, 85
        add r14, r13
        lea rsi, [r14]
        mov r14, r15
        mov r13, 4
        imul r13, 85
        add r14, r13
        lea rdi, [r14]
        imul rcx, 85
        rep movsb
    fuz_41_5_end:
    cmp_42_12:
        lea r13, [rsp - 10432]
        mov r12, 1
        imul r12, 5216
        add r13, r12
        mov r12, 2
        imul r12, 744
        add r13, r12
        mov r12, 4
        imul r12, 85
        add r13, r12
        add r13, 153
        movsx r14, dword [r13]
    cmp r14, 0xba
    jne bool_false_42_12
    jmp bool_true_42_12
    bool_true_42_12:
    mov r15, true
    jmp bool_end_42_12
    bool_false_42_12:
    mov r15, false
    bool_end_42_12:
    assert_42_5:
        if_8_8_42_5:
        cmp_8_8_42_5:
        test r15, r15
        jne if_8_5_42_5_end
        jmp if_8_8_42_5_code
        if_8_8_42_5_code:
            mov rdi, 1
            exit_8_17_42_5:
                mov rax, 60
                syscall
            exit_8_17_42_5_end:
        if_8_5_42_5_end:
    assert_42_5_end:
    cmp_43_12:
        lea r13, [rsp - 10432]
        mov r12, 1
        imul r12, 5216
        add r13, r12
        mov r12, 2
        imul r12, 744
        add r13, r12
        mov r12, 4
        imul r12, 85
        add r13, r12
        add r13, 80
        mov r14, qword [r13]
    cmp r14, 0xab
    jne bool_false_43_12
    jmp bool_true_43_12
    bool_true_43_12:
    mov r15, true
    jmp bool_end_43_12
    bool_false_43_12:
    mov r15, false
    bool_end_43_12:
    assert_43_5:
        if_8_8_43_5:
        cmp_8_8_43_5:
        test r15, r15
        jne if_8_5_43_5_end
        jmp if_8_8_43_5_code
        if_8_8_43_5_code:
            mov rdi, 1
            exit_8_17_43_5:
                mov rax, 60
                syscall
            exit_8_17_43_5_end:
        if_8_5_43_5_end:
    assert_43_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
