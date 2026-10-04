default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp], 0
    mov qword [rbp + 8], 0
    mov word [rbp], 25185
    mov byte [rbp + 2], 99
    mov qword [rbp + 8], 3
    mov qword [rbp + 16], 0
    mov qword [rbp + 24], 0
    func.str.append.name.25.7:
        mov rcx, qword [rbp + 8]
        cmp rcx, 3
        ja baz_bounds_line_15
        lea rsi, [rbp]
        mov r15, qword [rbp + 24]
        test r15, r15
        js baz_bounds_line_15
        lea r14, [rcx + r15]
        cmp r14, 8
        jg baz_bounds_line_15
        lea rdi, [rbp + r15 + 16]
        rep movsb
        mov r15, qword [rbp + 8]
        add qword [rbp + 24], r15
    func.str.append.name.25.7.end:
    func.str.append.name.26.7:
        mov rcx, qword [rbp + 8]
        cmp rcx, 3
        ja baz_bounds_line_15
        lea rsi, [rbp]
        mov r15, qword [rbp + 24]
        test r15, r15
        js baz_bounds_line_15
        lea r14, [rcx + r15]
        cmp r14, 8
        jg baz_bounds_line_15
        lea rdi, [rbp + r15 + 16]
        rep movsb
        mov r15, qword [rbp + 8]
        add qword [rbp + 24], r15
    func.str.append.name.26.7.end:
    cmp.27.15:
    cmp qword [rbp + 24], 6
    sete r15b
    jne bool.27.15.end
    cmp.27.30:
    cmp byte [rbp + 19], 97
    sete r15b
    jne bool.27.15.end
    cmp.27.52:
    cmp byte [rbp + 21], 99
    sete r15b
    bool.27.15.end:
    func.assert.27.5:
        if.4.32.27.5:
        cmp.4.32.27.5:
        cmp r15b, 0
        jne if.4.29.27.5.end
        if.4.32.27.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.4.29.27.5.end:
    func.assert.27.5.end:
    mov qword [rbp + 32], 0
    mov qword [rbp + 40], 0
    func.str.append.str.31.7:
        mov rcx, qword [rbp + 24]
        cmp rcx, 8
        ja baz_bounds_line_15
        lea rsi, [rbp + 16]
        mov r15, qword [rbp + 40]
        test r15, r15
        js baz_bounds_line_15
        lea r14, [rcx + r15]
        cmp r14, 8
        jg baz_bounds_line_15
        lea rdi, [rbp + r15 + 32]
        rep movsb
        mov r15, qword [rbp + 24]
        add qword [rbp + 40], r15
    func.str.append.str.31.7.end:
    cmp.32.15:
    cmp qword [rbp + 40], 6
    sete r15b
    bool.32.15.end:
    func.assert.32.5:
        if.4.32.32.5:
        cmp.4.32.32.5:
        cmp r15b, 0
        jne if.4.29.32.5.end
        if.4.32.32.5.code:
            mov rdi, 2
            mov rax, 60
            syscall
        if.4.29.32.5.end:
    func.assert.32.5.end:
    mov qword [rbp + 48], 0
    mov qword [rbp + 56], 0
    func.name.append.name.36.7:
        mov rcx, qword [rbp + 8]
        cmp rcx, 3
        ja baz_bounds_line_15
        lea rsi, [rbp]
        mov r15, qword [rbp + 56]
        test r15, r15
        js baz_bounds_line_15
        lea r14, [rcx + r15]
        cmp r14, 3
        jg baz_bounds_line_15
        lea rdi, [rbp + r15 + 48]
        rep movsb
        mov r15, qword [rbp + 8]
        add qword [rbp + 56], r15
    func.name.append.name.36.7.end:
    cmp.37.15:
    cmp qword [rbp + 56], 3
    sete r15b
    jne bool.37.15.end
    cmp.37.30:
    cmp byte [rbp + 49], 98
    sete r15b
    bool.37.15.end:
    func.assert.37.5:
        if.4.32.37.5:
        cmp.4.32.37.5:
        cmp r15b, 0
        jne if.4.29.37.5.end
        if.4.32.37.5.code:
            mov rdi, 3
            mov rax, 60
            syscall
        if.4.29.37.5.end:
    func.assert.37.5.end:
    mov rdi, 0
    mov rax, 60
    syscall
baz_frame_overflow:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_frame_overflow]
    mov rdx, msg_frame_overflow_len
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
msg_frame_overflow:
db `panic: frame overflow\n`
msg_frame_overflow_len equ $ - msg_frame_overflow
section .text
baz_bounds_line_15:
    mov rbp, 15
baz_bounds_panic:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    mov rdi, strict qword num_buffer + 19
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
    mov rdx, strict qword num_buffer + 20
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
msg_panic:
db `panic: bounds at line `
msg_panic_len equ $ - msg_panic
section .bss
num_buffer:
resb 21
section .data
align 16
dat:
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 65536
vars.end:
