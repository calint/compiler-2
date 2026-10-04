default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp], 0
    mov qword [rbp + 8], 0
    mov qword [rbp + 16], 0
    mov qword [rbp + 24], 0
    cmp.56.15:
        func.str.size.56.17:
            mov r14, 8
        func.str.size.56.17.end:
    cmp r14, 8
    sete r15b
    bool.56.15.end:
    func.assert.56.5:
        if.4.32.56.5:
        cmp.4.32.56.5:
        cmp r15b, 0
        jne if.4.29.56.5.end
        if.4.32.56.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.4.29.56.5.end:
    func.assert.56.5.end:
    cmp.57.15:
        func.name.size.57.17:
            mov r14, 4
        func.name.size.57.17.end:
    cmp r14, 4
    sete r15b
    bool.57.15.end:
    func.assert.57.5:
        if.4.32.57.5:
        cmp.4.32.57.5:
        cmp r15b, 0
        jne if.4.29.57.5.end
        if.4.32.57.5.code:
            mov rdi, 2
            mov rax, 60
            syscall
        if.4.29.57.5.end:
    func.assert.57.5.end:
    func.str.set.59.7:
        mov qword [rbp + 8], 3
    func.str.set.59.7.end:
    func.name.set.60.7:
        mov qword [rbp + 24], 3
    func.name.set.60.7.end:
    cmp.61.15:
    cmp qword [rbp + 8], 3
    sete r15b
    jne bool.61.15.end
    cmp.61.30:
    cmp qword [rbp + 24], 3
    sete r15b
    bool.61.15.end:
    func.assert.61.5:
        if.4.32.61.5:
        cmp.4.32.61.5:
        cmp r15b, 0
        jne if.4.29.61.5.end
        if.4.32.61.5.code:
            mov rdi, 3
            mov rax, 60
            syscall
        if.4.29.61.5.end:
    func.assert.61.5.end:
    cmp.62.15:
        func.str.is_empty.62.21:
            cmp.30.11.62.21:
            cmp qword [rbp + 8], 0
            sete r15b
            bool.30.11.62.21.end:
        func.str.is_empty.62.21.end:
    cmp r15b, 0
    sete r15b
    jne bool.62.15.end
    cmp.62.36:
        func.name.is_empty.62.38:
            cmp.30.11.62.38:
            cmp qword [rbp + 24], 0
            sete r14b
            bool.30.11.62.38.end:
        func.name.is_empty.62.38.end:
    cmp r14b, 0
    sete r15b
    bool.62.15.end:
    func.assert.62.5:
        if.4.32.62.5:
        cmp.4.32.62.5:
        cmp r15b, 0
        jne if.4.29.62.5.end
        if.4.32.62.5.code:
            mov rdi, 4
            mov rax, 60
            syscall
        if.4.29.62.5.end:
    func.assert.62.5.end:
    mov qword [rbp + 32], 0
    mov qword [rbp + 40], 0
    func.str.set.65.8:
        mov qword [rbp + 40], 3
    func.str.set.65.8.end:
    cmp.66.15:
        func.str.equals.66.17:
            cmp.25.11.66.17:
            mov r14, qword [rbp + 40]
            cmp qword [rbp + 8], r14
            sete r15b
            bool.25.11.66.17.end:
        func.str.equals.66.17.end:
    cmp r15b, 0
    setne r15b
    bool.66.15.end:
    func.assert.66.5:
        if.4.32.66.5:
        cmp.4.32.66.5:
        cmp r15b, 0
        jne if.4.29.66.5.end
        if.4.32.66.5.code:
            mov rdi, 5
            mov rax, 60
            syscall
        if.4.29.66.5.end:
    func.assert.66.5.end:
    mov qword [rbp + 48], 0
    mov qword [rbp + 56], 0
    mov qword [rbp + 64], 0
    mov qword [rbp + 72], 0
    func.name.set.69.9:
        mov qword [rbp + 56], 2
    func.name.set.69.9.end:
    func.str.set.70.9:
        mov qword [rbp + 72], 5
    func.str.set.70.9.end:
    cmp.71.15:
    cmp qword [rbp + 56], 2
    sete r15b
    jne bool.71.15.end
    cmp.71.32:
    cmp qword [rbp + 72], 5
    sete r15b
    jne bool.71.15.end
    cmp.71.49:
        func.str.size.71.53:
            mov r14, 8
        func.str.size.71.53.end:
    cmp r14, 8
    sete r15b
    bool.71.15.end:
    func.assert.71.5:
        if.4.32.71.5:
        cmp.4.32.71.5:
        cmp r15b, 0
        jne if.4.29.71.5.end
        if.4.32.71.5.code:
            mov rdi, 6
            mov rax, 60
            syscall
        if.4.29.71.5.end:
    func.assert.71.5.end:
    func.name.set.73.7:
        mov qword [rbp + 24], 4
    func.name.set.73.7.end:
    cmp.74.15:
        func.name.is_full.74.17:
            cmp.35.11.74.17:
            cmp qword [rbp + 24], 4
            sete r15b
            bool.35.11.74.17.end:
        func.name.is_full.74.17.end:
    cmp r15b, 0
    setne r15b
    je bool.74.15.end
    cmp.74.31:
        func.str.is_full.74.37:
            cmp.35.11.74.37:
            cmp qword [rbp + 8], 8
            sete r15b
            bool.35.11.74.37.end:
        func.str.is_full.74.37.end:
    cmp r15b, 0
    sete r15b
    bool.74.15.end:
    func.assert.74.5:
        if.4.32.74.5:
        cmp.4.32.74.5:
        cmp r15b, 0
        jne if.4.29.74.5.end
        if.4.32.74.5.code:
            mov rdi, 7
            mov rax, 60
            syscall
        if.4.29.74.5.end:
    func.assert.74.5.end:
    lea r15, [rbp + 88]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.str.capacity_of
    cmp r15, r14
    ja baz_frame_overflow
    lea r15, [rbp + 80]
    mov qword [rbp + 88], r15
    lea r15, [rbp]
    mov qword [rbp + 96], r15
    lea rbx, [rbp + 88]
    call func.str.capacity_of
    lea r15, [rbp + 96]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.name.capacity_of
    cmp r15, r14
    ja baz_frame_overflow
    lea r15, [rbp + 88]
    mov qword [rbp + 96], r15
    lea r15, [rbp + 16]
    mov qword [rbp + 104], r15
    lea rbx, [rbp + 96]
    call func.name.capacity_of
    cmp.77.15:
    cmp qword [rbp + 80], 8
    sete r15b
    jne bool.77.15.end
    cmp.77.35:
    cmp qword [rbp + 88], 4
    sete r15b
    bool.77.15.end:
    func.assert.77.5:
        if.4.32.77.5:
        cmp.4.32.77.5:
        cmp r15b, 0
        jne if.4.29.77.5.end
        if.4.32.77.5.code:
            mov rdi, 8
            mov rax, 60
            syscall
        if.4.29.77.5.end:
    func.assert.77.5.end:
    mov dword [rbp + 100], 0
    func.name_str.copy_a.79.15:
        mov rax, qword [rbp + 48]
        mov qword [rbp + 96], rax
        mov rax, qword [rbp + 56]
        mov qword [rbp + 104], rax
    func.name_str.copy_a.79.15.end:
    cmp.80.15:
    cmp qword [rbp + 104], 2
    sete r15b
    bool.80.15.end:
    func.assert.80.5:
        if.4.32.80.5:
        cmp.4.32.80.5:
        cmp r15b, 0
        jne if.4.29.80.5.end
        if.4.32.80.5.code:
            mov rdi, 9
            mov rax, 60
            syscall
        if.4.29.80.5.end:
    func.assert.80.5.end:
    mov rdi, 0
    mov rax, 60
    syscall
func.str.capacity_of:
    mov r15, qword [rbx]
    mov qword [r15], 8
    ret
size.func.str.capacity_of equ 16
func.name.capacity_of:
    mov r15, qword [rbx]
    mov qword [r15], 4
    ret
size.func.name.capacity_of equ 16
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
