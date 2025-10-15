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
    lea rdi, [rsp - 8640]
    mov rcx, 8640
    xor rax, rax
    rep stosb
    baz_45_5:
        lea r15, [rsp - 8640]
        mov r14, 1
        cmp r14, 2
        mov r13, 40
        cmovge rbp, r13
        jge panic_bounds
        imul r14, 4320
        add r15, r14
        add r15, 8
        bar_40_5_45_5:
            mov r14, r15
            mov r13, 1
            cmp r13, 7
            mov r12, 35
            cmovge rbp, r12
            jge panic_bounds
            imul r13, 616
            add r14, r13
            foo_35_5_40_5_45_5:
                mov dword [r14 + 65], 2
                mov byte [r14 + 64], 1
                mov r13, r14
                mov r12, 0
                cmp r12, 8
                mov r11, 31
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 69
                add r13, r12
                mov byte [r13], 0x61
            foo_35_5_40_5_45_5_end:
            mov r14, r15
            mov r13, 5
            cmp r13, 7
            mov r12, 36
            cmovge rbp, r12
            jge panic_bounds
            imul r13, 616
            add r14, r13
            foo_36_5_40_5_45_5:
                mov dword [r14 + 65], 2
                mov byte [r14 + 64], 1
                mov r13, r14
                mov r12, 0
                cmp r12, 8
                mov r11, 31
                cmovge rbp, r11
                jge panic_bounds
                imul r12, 69
                add r13, r12
                mov byte [r13], 0x61
            foo_36_5_40_5_45_5_end:
        bar_40_5_45_5_end:
    baz_45_5_end:
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
