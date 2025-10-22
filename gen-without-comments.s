DEFAULT REL
section .bss
stk resd 131072
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
    mov qword [rsp - 20], 0xe
    mov qword [rsp - 12], 0xd
    mov dword [rsp - 4], 0xff0000
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    mov dword [rsp - 24], 0
    lea rsi, [rsp - 20]
    lea rdi, [rsp - 40]
    mov rcx, 20
    rep movsb
    cmp_17_12:
    cmp qword [rsp - 40], 0xe
    jne bool_false_17_12
    jmp bool_true_17_12
    bool_true_17_12:
    mov r15, true
    jmp bool_end_17_12
    bool_false_17_12:
    mov r15, false
    bool_end_17_12:
    assert_17_5:
        if_8_8_17_5:
        cmp_8_8_17_5:
        test r15, r15
        jne if_8_5_17_5_end
        jmp if_8_8_17_5_code
        if_8_8_17_5_code:
            mov rdi, 1
            exit_8_17_17_5:
                    mov rax, 60
                syscall
            exit_8_17_17_5_end:
        if_8_5_17_5_end:
    assert_17_5_end:
    cmp_18_12:
    cmp qword [rsp - 32], 0xd
    jne bool_false_18_12
    jmp bool_true_18_12
    bool_true_18_12:
    mov r15, true
    jmp bool_end_18_12
    bool_false_18_12:
    mov r15, false
    bool_end_18_12:
    assert_18_5:
        if_8_8_18_5:
        cmp_8_8_18_5:
        test r15, r15
        jne if_8_5_18_5_end
        jmp if_8_8_18_5_code
        if_8_8_18_5_code:
            mov rdi, 1
            exit_8_17_18_5:
                    mov rax, 60
                syscall
            exit_8_17_18_5_end:
        if_8_5_18_5_end:
    assert_18_5_end:
    cmp_19_12:
    cmp dword [rsp - 24], 0xff0000
    jne bool_false_19_12
    jmp bool_true_19_12
    bool_true_19_12:
    mov r15, true
    jmp bool_end_19_12
    bool_false_19_12:
    mov r15, false
    bool_end_19_12:
    assert_19_5:
        if_8_8_19_5:
        cmp_8_8_19_5:
        test r15, r15
        jne if_8_5_19_5_end
        jmp if_8_8_19_5_code
        if_8_8_19_5_code:
            mov rdi, 1
            exit_8_17_19_5:
                    mov rax, 60
                syscall
            exit_8_17_19_5_end:
        if_8_5_19_5_end:
    assert_19_5_end:
    mov qword [rsp - 56], 0xab
    mov qword [rsp - 48], 0xba
    lea rsi, [rsp - 56]
    lea rdi, [rsp - 40]
    mov rcx, 2
    rep movsq
    cmp_23_12:
    cmp qword [rsp - 40], 0xab
    jne bool_false_23_12
    jmp bool_true_23_12
    bool_true_23_12:
    mov r15, true
    jmp bool_end_23_12
    bool_false_23_12:
    mov r15, false
    bool_end_23_12:
    assert_23_5:
        if_8_8_23_5:
        cmp_8_8_23_5:
        test r15, r15
        jne if_8_5_23_5_end
        jmp if_8_8_23_5_code
        if_8_8_23_5_code:
            mov rdi, 1
            exit_8_17_23_5:
                    mov rax, 60
                syscall
            exit_8_17_23_5_end:
        if_8_5_23_5_end:
    assert_23_5_end:
    cmp_24_12:
    cmp qword [rsp - 32], 0xba
    jne bool_false_24_12
    jmp bool_true_24_12
    bool_true_24_12:
    mov r15, true
    jmp bool_end_24_12
    bool_false_24_12:
    mov r15, false
    bool_end_24_12:
    assert_24_5:
        if_8_8_24_5:
        cmp_8_8_24_5:
        test r15, r15
        jne if_8_5_24_5_end
        jmp if_8_8_24_5_code
        if_8_8_24_5_code:
            mov rdi, 1
            exit_8_17_24_5:
                    mov rax, 60
                syscall
            exit_8_17_24_5_end:
        if_8_5_24_5_end:
    assert_24_5_end:
    mov rcx, 25
    lea rdi, [rsp - 256]
    xor rax, rax
    rep stosq
    lea r15, [rsp - 256]
    mov r14, 1
    mov r13, 27
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 10
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    mov qword [r15 + 8], 0xab
    lea r15, [rsp - 256]
    mov r14, 2
    mov r13, 29
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 10
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    lea r14, [rsp - 256]
    mov r13, 1
    mov r12, 29
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 10
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    lea rsi, [r14]
    lea rdi, [r15]
    mov rcx, 25
    rep movsq
    cmp_30_12:
    lea r14, [rsp - 256]
    mov r13, 2
    mov r12, 30
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 10
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    lea r13, [rsp - 256]
    mov r12, 1
    mov r11, 30
    test r12, r12
    cmovs rbp, r11
    js panic_bounds
    cmp r12, 10
    cmovge rbp, r11
    jge panic_bounds
    imul r12, 20
    add r13, r12
    mov r12, qword [r13 + 8]
    cmp qword [r14 + 8], r12
    jne bool_false_30_12
    jmp bool_true_30_12
    bool_true_30_12:
    mov r15, true
    jmp bool_end_30_12
    bool_false_30_12:
    mov r15, false
    bool_end_30_12:
    assert_30_5:
        if_8_8_30_5:
        cmp_8_8_30_5:
        test r15, r15
        jne if_8_5_30_5_end
        jmp if_8_8_30_5_code
        if_8_8_30_5_code:
            mov rdi, 1
            exit_8_17_30_5:
                    mov rax, 60
                syscall
            exit_8_17_30_5_end:
        if_8_5_30_5_end:
    assert_30_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    lea rdi, [num_buffer + 19]
    mov byte [rdi], 10
    dec rdi
    mov rcx, 10
.convert_loop:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jnz .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    lea rdx, [num_buffer + 20]
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
    msg_panic: db 'panic: bounds at line '
    msg_panic_len equ $ - msg_panic
section .bss
    num_buffer: resb 21
