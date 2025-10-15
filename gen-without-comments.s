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
    lea rdi, [rsp - 80]
    mov rcx, 80
    xor rax, rax
    rep stosb
    lea r14, [rsp - 80]
    mov r13, 1
    cmp r13, 2
    mov r12, 30
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 40
    add r14, r13
    mov r13, 2
    cmp r13, 4
    mov r12, 30
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 3
    add r14, r13
    add r14, 8
    movsx r15, byte [r14]
    foo_30_5:
        mov r14, 2
        cmp r14, 0
        mov r13, 21
        cmovge rbp, r13
        jge panic_bounds
        mov qword [rsp + r14 * 8 - 0], 0x10
    foo_30_5_end:
    lea r15, [rsp - 80]
    mov r14, 1
    cmp r14, 2
    mov r13, 31
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 40
    add r15, r14
    mov r14, 1
    cmp r14, 4
    mov r13, 31
    cmovge rbp, r13
    jge panic_bounds
    bar_31_5:
        lea r13, [r15 + r14 * 8 + 8]
        mov r12, 2
        cmp r12, 8
        mov r11, 25
        cmovge rbp, r11
        jge panic_bounds
        mov byte [r13 + r12 + 0], 0x10
    bar_31_5_end:
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
