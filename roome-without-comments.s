default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 128], 0
    xor al, al
    lea rdi, [rbp + 136]
    mov rcx, 128
    rep stosb
    func.print.87.5:
        mov rdi, 1
        mov rdx, 92
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.87.5.end:
    loop.88.5:
        add qword [rbp + 128], 1
        lea r15, [rbp + 128]
        mov qword [rbp + 264], r15
        lea rbx, [rbp + 264]
        call func.print_num
        func.print.91.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 121]
            mov rax, 1
            syscall
        func.print.91.9.end:
        func.print.92.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 92]
            mov rax, 1
            syscall
        func.print.92.9.end:
        func.str.in.93.12:
            mov qword [rbp + 264], 0
            loop.22.5.93.12:
                if.23.12.93.12:
                cmp.23.12.93.12:
                    mov r15, 127
                cmp qword [rbp + 264], r15
                je loop.22.5.93.12.end
                if.23.12.93.12.code:
                if.23.9.93.12.end:
                if.24.12.93.12:
                cmp.24.12.93.12:
                    mov rdi, 0
                    mov rdx, 1
                    mov r14, qword [rbp + 264]
                    lea rsi, [rbp + 137]
                    add rsi, r14
                    mov rax, 0
                    syscall
                    mov r15, rax
                cmp r15, 0
                je loop.22.5.93.12.end
                if.24.12.93.12.code:
                if.24.9.93.12.end:
                if.25.12.93.12:
                cmp.25.12.93.12:
                mov r15, qword [rbp + 264]
                cmp byte [rbp + r15 + 137], 127
                jne if.25.9.93.12.end
                if.25.12.93.12.code:
                    if.26.16.93.12:
                    cmp.26.16.93.12:
                    cmp qword [rbp + 264], 0
                    jle if.26.13.93.12.end
                    if.26.16.93.12.code:
                        sub qword [rbp + 264], 1
                        mov rdi, 1
                        mov rdx, 3
                        lea rsi, [rbp + 118]
                        mov rax, 1
                        syscall
                    if.26.13.93.12.end:
                    jmp loop.22.5.93.12
                if.25.9.93.12.end:
                mov rdi, 1
                mov rdx, 1
                mov r15, qword [rbp + 264]
                lea rsi, [rbp + 137]
                add rsi, r15
                mov rax, 1
                syscall
                if.33.12.93.12:
                cmp.33.12.93.12:
                mov r15, qword [rbp + 264]
                cmp byte [rbp + r15 + 137], 10
                je loop.22.5.93.12.end
                if.33.12.93.12.code:
                if.33.9.93.12.end:
                add qword [rbp + 264], 1
            jmp loop.22.5.93.12
            loop.22.5.93.12.end:
            mov r15b, byte [rbp + 264]
            mov byte [rbp + 136], r15b
        func.str.in.93.12.end:
        if.94.12:
        cmp.94.12:
        cmp byte [rbp + 136], 4
        jg if.94.9.else
        if.94.12.code:
            func.print.95.13:
                mov rdi, 1
                mov rdx, 16
                lea rsi, [rbp + 94]
                mov rax, 1
                syscall
            func.print.95.13.end:
            jmp loop.88.5
        if.94.9.else:
            func.print.98.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 110]
                mov rax, 1
                syscall
            func.print.98.13.end:
            func.str.out.99.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 136]
                lea rsi, [rbp + 137]
                mov rax, 1
                syscall
            func.str.out.99.16.end:
            func.print.100.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 116]
                mov rax, 1
                syscall
            func.print.100.13.end:
            func.print.101.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 117]
                mov rax, 1
                syscall
            func.print.101.13.end:
        if.94.9.end:
    jmp loop.88.5
    loop.88.5.end:
func.print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
    mov byte [rbx + 40], 0
    if.49.8:
    cmp.49.8:
    cmp qword [rbx + 32], 0
    jge if.49.5.end
    if.49.8.code:
        mov byte [rbx + 40], 1
    if.49.5.end:
    if.52.8:
    cmp.52.8:
    cmp qword [rbx + 32], 0
    jle if.52.5.end
    if.52.8.code:
        neg qword [rbx + 32]
    if.52.5.end:
    mov qword [rbx + 48], 20
    loop.57.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
            mov r14, 48
            mov r13, qword [rbx + 32]
            mov rax, r13
            cqo
            mov r12, 10
            idiv r12
            mov r13, rdx
            sub r14, r13
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 32]
        cqo
        mov r15, 10
        idiv r15
        mov qword [rbx + 32], rax
        if.61.12:
        cmp.61.12:
        cmp qword [rbx + 32], 0
        jne loop.57.5
        if.61.12.code:
        if.61.9.end:
    loop.57.5.end:
    if.64.8:
    cmp.64.8:
    cmp byte [rbx + 40], 0
    je if.64.5.end
    if.64.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov byte [rbx + r15 + 8], 45
    if.64.5.end:
    mov qword [rbx + 56], 0
    loop.70.5:
        mov r15, qword [rbx + 56]
        mov r14, qword [rbx + 48]
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 56], 1
        add qword [rbx + 48], 1
        if.74.12:
        cmp.74.12:
        cmp qword [rbx + 48], 20
        jne loop.70.5
        if.74.12.code:
        if.74.9.end:
    loop.70.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
section .data
align 16
dat:
db `welcome to adventure #6\n    type 'help'\n\nu r in roome\nu c me\nexits: none\ntodo: find an exit\n`
db `> `
db `unknown command\n`
db `hello `
db `.`
db `\n`
db `\b \b`
db `: `
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 65536
vars.end:
