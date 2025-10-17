DEFAULT REL
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
nl_str: db '', 10,''
nl_str.len equ $ - nl_str
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 512]
    mov rcx, 512
    xor rax, rax
    rep stosb
    lea r15, [rsp - 512]
    mov r14, 0
    cmp r14, 2
    mov r13, 72
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 8
    add r15, r14
    mov byte [r15 + 128], 1
    lea r15, [rsp - 512]
    mov r14, 1
    cmp r14, 2
    mov r13, 73
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 8
    add r15, r14
    mov byte [r15 + 128], 2
    lea r15, [rsp - 512]
    mov r14, 0
    cmp r14, 2
    mov r13, 75
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 8
    add r15, r14
    lea r14, [rsp - 512]
    mov r13, 1
    cmp r13, 2
    mov r12, 75
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 8
    add r14, r13
    foo_75_5:
        cmp_67_12_75_5:
            mov r11, r15
            movsx r12, byte [r11 + 128]
            mov r10, r14
            movsx r11, byte [r10 + 128]
            add r12, r11
        cmp r12, 3
        jne bool_false_67_12_75_5
        jmp bool_true_67_12_75_5
        bool_true_67_12_75_5:
        mov r13, true
        jmp bool_end_67_12_75_5
        bool_false_67_12_75_5:
        mov r13, false
        bool_end_67_12_75_5:
        assert_67_5_75_5:
            if_9_29_67_5_75_5:
            cmp_9_29_67_5_75_5:
            test r13, r13
            jne if_9_26_67_5_75_5_end
            jmp if_9_29_67_5_75_5_code
            if_9_29_67_5_75_5_code:
                mov rdi, 1
                exit_9_38_67_5_75_5:
                    mov rax, 60
                    syscall
                exit_9_38_67_5_75_5_end:
            if_9_26_67_5_75_5_end:
        assert_67_5_75_5_end:
    foo_75_5_end:
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
