default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    func.constants_and_variables.564.5:
        mov qword [rbp + 128], 1
        add qword [rbp + 128], 42
        cmp.24.12.564.5:
        cmp qword [rbp + 128], 43
        sete r15b
        bool.24.12.564.5.end:
        func.assert.24.5.564.5:
            if.6.27.24.5.564.5:
            cmp.6.27.24.5.564.5:
            cmp r15b, 0
            jne if.6.24.24.5.564.5.end
            if.6.27.24.5.564.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.24.5.564.5.end:
        func.assert.24.5.564.5.end:
    func.constants_and_variables.564.5.end:
    func.literals.565.5:
        mov qword [rbp + 128], 255
        mov qword [rbp + 136], 255
        mov qword [rbp + 144], 255
        cmp.36.12.565.5:
        mov r14, qword [rbp + 136]
        cmp qword [rbp + 128], r14
        sete r15b
        jne bool.36.12.565.5.end
        cmp.36.27.565.5:
        mov r14, qword [rbp + 144]
        cmp qword [rbp + 136], r14
        sete r15b
        bool.36.12.565.5.end:
        func.assert.36.5.565.5:
            if.6.27.36.5.565.5:
            cmp.6.27.36.5.565.5:
            cmp r15b, 0
            jne if.6.24.36.5.565.5.end
            if.6.27.36.5.565.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.36.5.565.5.end:
        func.assert.36.5.565.5.end:
        mov qword [rbp + 152], 97
        cmp.40.12.565.5:
        cmp qword [rbp + 152], 97
        sete r15b
        bool.40.12.565.5.end:
        func.assert.40.5.565.5:
            if.6.27.40.5.565.5:
            cmp.6.27.40.5.565.5:
            cmp r15b, 0
            jne if.6.24.40.5.565.5.end
            if.6.27.40.5.565.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.40.5.565.5.end:
        func.assert.40.5.565.5.end:
        mov r15, qword [rbp + 128]
        mov qword [rbp + 160], r15
        neg qword [rbp + 160]
        mov r15, qword [rbp + 128]
        mov qword [rbp + 168], r15
        not qword [rbp + 168]
        cmp.46.12.565.5:
        cmp qword [rbp + 160], -255
        sete r15b
        bool.46.12.565.5.end:
        func.assert.46.5.565.5:
            if.6.27.46.5.565.5:
            cmp.6.27.46.5.565.5:
            cmp r15b, 0
            jne if.6.24.46.5.565.5.end
            if.6.27.46.5.565.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.46.5.565.5.end:
        func.assert.46.5.565.5.end:
        cmp.47.12.565.5:
        cmp qword [rbp + 168], -256
        sete r15b
        bool.47.12.565.5.end:
        func.assert.47.5.565.5:
            if.6.27.47.5.565.5:
            cmp.6.27.47.5.565.5:
            cmp r15b, 0
            jne if.6.24.47.5.565.5.end
            if.6.27.47.5.565.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.47.5.565.5.end:
        func.assert.47.5.565.5.end:
    func.literals.565.5.end:
    func.arithmetic.566.5:
        mov qword [rbp + 128], 7
        mov qword [rbp + 136], 2
        cmp.58.12.566.5:
            mov r14, qword [rbp + 128]
            add r14, qword [rbp + 136]
        cmp r14, 9
        sete r15b
        bool.58.12.566.5.end:
        func.assert.58.5.566.5:
            if.6.27.58.5.566.5:
            cmp.6.27.58.5.566.5:
            cmp r15b, 0
            jne if.6.24.58.5.566.5.end
            if.6.27.58.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.58.5.566.5.end:
        func.assert.58.5.566.5.end:
        cmp.59.12.566.5:
            mov r14, qword [rbp + 128]
            sub r14, qword [rbp + 136]
        cmp r14, 5
        sete r15b
        bool.59.12.566.5.end:
        func.assert.59.5.566.5:
            if.6.27.59.5.566.5:
            cmp.6.27.59.5.566.5:
            cmp r15b, 0
            jne if.6.24.59.5.566.5.end
            if.6.27.59.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.59.5.566.5.end:
        func.assert.59.5.566.5.end:
        cmp.60.12.566.5:
            mov r14, qword [rbp + 128]
            imul r14, qword [rbp + 136]
        cmp r14, 14
        sete r15b
        bool.60.12.566.5.end:
        func.assert.60.5.566.5:
            if.6.27.60.5.566.5:
            cmp.6.27.60.5.566.5:
            cmp r15b, 0
            jne if.6.24.60.5.566.5.end
            if.6.27.60.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.60.5.566.5.end:
        func.assert.60.5.566.5.end:
        cmp.61.12.566.5:
            mov r14, qword [rbp + 128]
            mov rax, r14
            cqo
            idiv qword [rbp + 136]
            mov r14, rax
        cmp r14, 3
        sete r15b
        bool.61.12.566.5.end:
        func.assert.61.5.566.5:
            if.6.27.61.5.566.5:
            cmp.6.27.61.5.566.5:
            cmp r15b, 0
            jne if.6.24.61.5.566.5.end
            if.6.27.61.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.61.5.566.5.end:
        func.assert.61.5.566.5.end:
        cmp.62.12.566.5:
            mov r14, qword [rbp + 128]
            mov rax, r14
            cqo
            idiv qword [rbp + 136]
            mov r14, rdx
        cmp r14, 1
        sete r15b
        bool.62.12.566.5.end:
        func.assert.62.5.566.5:
            if.6.27.62.5.566.5:
            cmp.6.27.62.5.566.5:
            cmp r15b, 0
            jne if.6.24.62.5.566.5.end
            if.6.27.62.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.62.5.566.5.end:
        func.assert.62.5.566.5.end:
        cmp.63.12.566.5:
            mov r14, qword [rbp + 128]
            neg r14
            mov rax, r14
            cqo
            idiv qword [rbp + 136]
            mov r14, rax
        cmp r14, -3
        sete r15b
        bool.63.12.566.5.end:
        func.assert.63.5.566.5:
            if.6.27.63.5.566.5:
            cmp.6.27.63.5.566.5:
            cmp r15b, 0
            jne if.6.24.63.5.566.5.end
            if.6.27.63.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.63.5.566.5.end:
        func.assert.63.5.566.5.end:
        cmp.64.12.566.5:
            mov r14, qword [rbp + 128]
            neg r14
            mov rax, r14
            cqo
            idiv qword [rbp + 136]
            mov r14, rdx
        cmp r14, -1
        sete r15b
        bool.64.12.566.5.end:
        func.assert.64.5.566.5:
            if.6.27.64.5.566.5:
            cmp.6.27.64.5.566.5:
            cmp r15b, 0
            jne if.6.24.64.5.566.5.end
            if.6.27.64.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.64.5.566.5.end:
        func.assert.64.5.566.5.end:
        cmp.68.12.566.5:
            mov r14, qword [rbp + 128]
            and r14, 3
        cmp r14, 3
        sete r15b
        bool.68.12.566.5.end:
        func.assert.68.5.566.5:
            if.6.27.68.5.566.5:
            cmp.6.27.68.5.566.5:
            cmp r15b, 0
            jne if.6.24.68.5.566.5.end
            if.6.27.68.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.68.5.566.5.end:
        func.assert.68.5.566.5.end:
        cmp.69.12.566.5:
            mov r14, qword [rbp + 128]
            or r14, 8
        cmp r14, 15
        sete r15b
        bool.69.12.566.5.end:
        func.assert.69.5.566.5:
            if.6.27.69.5.566.5:
            cmp.6.27.69.5.566.5:
            cmp r15b, 0
            jne if.6.24.69.5.566.5.end
            if.6.27.69.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.69.5.566.5.end:
        func.assert.69.5.566.5.end:
        cmp.70.12.566.5:
            mov r14, qword [rbp + 128]
            xor r14, 1
        cmp r14, 6
        sete r15b
        bool.70.12.566.5.end:
        func.assert.70.5.566.5:
            if.6.27.70.5.566.5:
            cmp.6.27.70.5.566.5:
            cmp r15b, 0
            jne if.6.24.70.5.566.5.end
            if.6.27.70.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.70.5.566.5.end:
        func.assert.70.5.566.5.end:
        cmp.71.12.566.5:
            mov r14, qword [rbp + 128]
            sal r14, 2
        cmp r14, 28
        sete r15b
        bool.71.12.566.5.end:
        func.assert.71.5.566.5:
            if.6.27.71.5.566.5:
            cmp.6.27.71.5.566.5:
            cmp r15b, 0
            jne if.6.24.71.5.566.5.end
            if.6.27.71.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.71.5.566.5.end:
        func.assert.71.5.566.5.end:
        cmp.72.12.566.5:
            mov r14, qword [rbp + 128]
            neg r14
            sar r14, 1
        cmp r14, -4
        sete r15b
        bool.72.12.566.5.end:
        func.assert.72.5.566.5:
            if.6.27.72.5.566.5:
            cmp.6.27.72.5.566.5:
            cmp r15b, 0
            jne if.6.24.72.5.566.5.end
            if.6.27.72.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.72.5.566.5.end:
        func.assert.72.5.566.5.end:
        mov qword [rbp + 144], 1
        cmp.76.12.566.5:
            mov r14, qword [rbp + 144]
            add r14, 6
        cmp r14, 7
        sete r15b
        bool.76.12.566.5.end:
        func.assert.76.5.566.5:
            if.6.27.76.5.566.5:
            cmp.6.27.76.5.566.5:
            cmp r15b, 0
            jne if.6.24.76.5.566.5.end
            if.6.27.76.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.76.5.566.5.end:
        func.assert.76.5.566.5.end:
        cmp.77.12.566.5:
            mov r14, qword [rbp + 144]
            mov r13, qword [rbp + 144]
            sal r13, 3
            add r14, r13
        cmp r14, 9
        sete r15b
        bool.77.12.566.5.end:
        func.assert.77.5.566.5:
            if.6.27.77.5.566.5:
            cmp.6.27.77.5.566.5:
            cmp r15b, 0
            jne if.6.24.77.5.566.5.end
            if.6.27.77.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.77.5.566.5.end:
        func.assert.77.5.566.5.end:
        cmp.78.12.566.5:
            mov r14, qword [rbp + 136]
            mov r13, qword [rbp + 144]
            or r13, 2
            imul r14, r13
        cmp r14, 6
        sete r15b
        bool.78.12.566.5.end:
        func.assert.78.5.566.5:
            if.6.27.78.5.566.5:
            cmp.6.27.78.5.566.5:
            cmp r15b, 0
            jne if.6.24.78.5.566.5.end
            if.6.27.78.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.78.5.566.5.end:
        func.assert.78.5.566.5.end:
        cmp.79.12.566.5:
            mov r14, qword [rbp + 144]
            xor r14, 3
            and r14, 6
        cmp r14, 2
        sete r15b
        bool.79.12.566.5.end:
        func.assert.79.5.566.5:
            if.6.27.79.5.566.5:
            cmp.6.27.79.5.566.5:
            cmp r15b, 0
            jne if.6.24.79.5.566.5.end
            if.6.27.79.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.79.5.566.5.end:
        func.assert.79.5.566.5.end:
        cmp.85.12.566.5:
            mov r14, qword [rbp + 144]
            add r14, qword [rbp + 144]
            sal r14, 3
        cmp r14, 16
        sete r15b
        bool.85.12.566.5.end:
        func.assert.85.5.566.5:
            if.6.27.85.5.566.5:
            cmp.6.27.85.5.566.5:
            cmp r15b, 0
            jne if.6.24.85.5.566.5.end
            if.6.27.85.5.566.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.85.5.566.5.end:
        func.assert.85.5.566.5.end:
    func.arithmetic.566.5.end:
    func.sized_types.567.5:
        mov byte [rbp + 128], 100
        mov r15b, byte [rbp + 128]
        add r15b, byte [rbp + 128]
        mov byte [rbp + 128], r15b
        cmp.100.12.567.5:
        cmp byte [rbp + 128], -56
        sete r15b
        bool.100.12.567.5.end:
        func.assert.100.5.567.5:
            if.6.27.100.5.567.5:
            cmp.6.27.100.5.567.5:
            cmp r15b, 0
            jne if.6.24.100.5.567.5.end
            if.6.27.100.5.567.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.100.5.567.5.end:
        func.assert.100.5.567.5.end:
        movsx r15, byte [rbp + 128]
        mov qword [rbp + 136], r15
        cmp.104.12.567.5:
        cmp qword [rbp + 136], -56
        sete r15b
        bool.104.12.567.5.end:
        func.assert.104.5.567.5:
            if.6.27.104.5.567.5:
            cmp.6.27.104.5.567.5:
            cmp r15b, 0
            jne if.6.24.104.5.567.5.end
            if.6.27.104.5.567.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.104.5.567.5.end:
        func.assert.104.5.567.5.end:
        mov r15b, byte [rbp + 136]
        mov byte [rbp + 128], r15b
        add byte [rbp + 128], 1
        cmp.112.12.567.5:
        cmp byte [rbp + 128], -55
        sete r15b
        bool.112.12.567.5.end:
        func.assert.112.5.567.5:
            if.6.27.112.5.567.5:
            cmp.6.27.112.5.567.5:
            cmp r15b, 0
            jne if.6.24.112.5.567.5.end
            if.6.27.112.5.567.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.112.5.567.5.end:
        func.assert.112.5.567.5.end:
    func.sized_types.567.5.end:
    func.booleans.568.5:
        mov byte [rbp + 128], 1
        mov byte [rbp + 129], 0
        mov qword [rbp + 136], 1
        cmp.125.16.568.5:
        cmp qword [rbp + 136], 2
        setl byte [rbp + 144]
        bool.125.16.568.5.end:
        cmp.126.12.568.5:
        mov r15b, byte [rbp + 144]
        bool.126.12.568.5.end:
        func.assert.126.5.568.5:
            if.6.27.126.5.568.5:
            cmp.6.27.126.5.568.5:
            cmp r15b, 0
            jne if.6.24.126.5.568.5.end
            if.6.27.126.5.568.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.126.5.568.5.end:
        func.assert.126.5.568.5.end:
        cmp.129.12.568.5:
        cmp byte [rbp + 128], 0
        setne r15b
        je bool.129.12.568.5.end
        cmp.129.20.568.5:
        cmp byte [rbp + 129], 0
        sete r15b
        bool.129.12.568.5.end:
        func.assert.129.5.568.5:
            if.6.27.129.5.568.5:
            cmp.6.27.129.5.568.5:
            cmp r15b, 0
            jne if.6.24.129.5.568.5.end
            if.6.27.129.5.568.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.129.5.568.5.end:
        func.assert.129.5.568.5.end:
        cmp.130.12.568.5:
        cmp byte [rbp + 129], 0
        setne r15b
        jne bool.130.12.568.5.end
        cmp.130.18.568.5:
        mov r15b, byte [rbp + 128]
        bool.130.12.568.5.end:
        func.assert.130.5.568.5:
            if.6.27.130.5.568.5:
            cmp.6.27.130.5.568.5:
            cmp r15b, 0
            jne if.6.24.130.5.568.5.end
            if.6.27.130.5.568.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.130.5.568.5.end:
        func.assert.130.5.568.5.end:
        cmp.131.16.568.5:
        cmp.131.17.568.5:
        cmp byte [rbp + 129], 0
        sete r15b
        jne bool.131.12.568.5.end
        cmp.131.23.568.5:
        cmp qword [rbp + 136], 2
        setle r15b
        bool.131.12.568.5.end:
        func.assert.131.5.568.5:
            if.6.27.131.5.568.5:
            cmp.6.27.131.5.568.5:
            cmp r15b, 0
            jne if.6.24.131.5.568.5.end
            if.6.27.131.5.568.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.131.5.568.5.end:
        func.assert.131.5.568.5.end:
    func.booleans.568.5.end:
    func.control_flow.569.5:
        mov qword [rbp + 128], 3
        mov qword [rbp + 136], 0
        if.143.8.569.5:
        cmp.143.8.569.5:
        cmp qword [rbp + 128], 0
        jge if.145.15.569.5
        if.143.8.569.5.code:
            mov qword [rbp + 136], -1
        jmp if.143.5.569.5.end
        if.145.15.569.5:
        cmp.145.15.569.5:
        cmp qword [rbp + 128], 0
        jne if.143.5.569.5.else
        if.145.15.569.5.code:
            mov qword [rbp + 136], 0
        jmp if.143.5.569.5.end
        if.143.5.569.5.else:
            mov qword [rbp + 136], 1
        if.143.5.569.5.end:
        cmp.150.12.569.5:
        cmp qword [rbp + 136], 1
        sete r15b
        bool.150.12.569.5.end:
        func.assert.150.5.569.5:
            if.6.27.150.5.569.5:
            cmp.6.27.150.5.569.5:
            cmp r15b, 0
            jne if.6.24.150.5.569.5.end
            if.6.27.150.5.569.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.150.5.569.5.end:
        func.assert.150.5.569.5.end:
        if.153.8.569.5:
        cmp.153.8.569.5:
        cmp qword [rbp + 128], 3
        jne if.153.5.569.5.end
        if.153.8.569.5.code:
            mov qword [rbp + 136], 2
        if.153.5.569.5.end:
        cmp.154.12.569.5:
        cmp qword [rbp + 136], 2
        sete r15b
        bool.154.12.569.5.end:
        func.assert.154.5.569.5:
            if.6.27.154.5.569.5:
            cmp.6.27.154.5.569.5:
            cmp r15b, 0
            jne if.6.24.154.5.569.5.end
            if.6.27.154.5.569.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.154.5.569.5.end:
        func.assert.154.5.569.5.end:
        mov qword [rbp + 144], 0
        mov qword [rbp + 152], 0
        loop.159.5.569.5:
            add qword [rbp + 152], 1
            if.161.12.569.5:
            cmp.161.12.569.5:
            cmp qword [rbp + 152], 3
            je loop.159.5.569.5
            if.161.12.569.5.code:
            if.161.9.569.5.end:
            if.162.12.569.5:
            cmp.162.12.569.5:
            cmp qword [rbp + 152], 5
            jg loop.159.5.569.5.end
            if.162.12.569.5.code:
            if.162.9.569.5.end:
            mov r15, qword [rbp + 152]
            add qword [rbp + 144], r15
        jmp loop.159.5.569.5
        loop.159.5.569.5.end:
        cmp.165.12.569.5:
        cmp qword [rbp + 144], 12
        sete r15b
        bool.165.12.569.5.end:
        func.assert.165.5.569.5:
            if.6.27.165.5.569.5:
            cmp.6.27.165.5.569.5:
            cmp r15b, 0
            jne if.6.24.165.5.569.5.end
            if.6.27.165.5.569.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.165.5.569.5.end:
        func.assert.165.5.569.5.end:
    func.control_flow.569.5.end:
    func.scopes.570.5:
        mov qword [rbp + 128], 1
            mov qword [rbp + 136], 2
            cmp.180.16.570.5:
            cmp.180.31.570.5:
            cmp qword [rbp + 136], 2
            sete r15b
            bool.180.16.570.5.end:
            func.assert.180.9.570.5:
                if.6.27.180.9.570.5:
                cmp.6.27.180.9.570.5:
                cmp r15b, 0
                jne if.6.24.180.9.570.5.end
                if.6.27.180.9.570.5.code:
                    mov rdi, 1
                    mov rax, 60
                    syscall
                if.6.24.180.9.570.5.end:
            func.assert.180.9.570.5.end:
        cmp.182.12.570.5:
        cmp.182.27.570.5:
        cmp qword [rbp + 128], 1
        sete r15b
        bool.182.12.570.5.end:
        func.assert.182.5.570.5:
            if.6.27.182.5.570.5:
            cmp.6.27.182.5.570.5:
            cmp r15b, 0
            jne if.6.24.182.5.570.5.end
            if.6.27.182.5.570.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.182.5.570.5.end:
        func.assert.182.5.570.5.end:
    func.scopes.570.5.end:
    func.arrays.571.5:
        mov qword [rbp + 128], 0
        mov qword [rbp + 136], 0
        cmp.193.12.571.5:
        cmp dword [rbp + 140], 0
        sete r15b
        bool.193.12.571.5.end:
        func.assert.193.5.571.5:
            if.6.27.193.5.571.5:
            cmp.6.27.193.5.571.5:
            cmp r15b, 0
            jne if.6.24.193.5.571.5.end
            if.6.27.193.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.193.5.571.5.end:
        func.assert.193.5.571.5.end:
        lea rsi, [init.197.21]
        lea rdi, [rbp + 144]
        mov rcx, 32
        rep movsb
        cmp.198.12.571.5:
            mov r14, 4
        cmp r14, 4
        sete r15b
        bool.198.12.571.5.end:
        func.assert.198.5.571.5:
            if.6.27.198.5.571.5:
            cmp.6.27.198.5.571.5:
            cmp r15b, 0
            jne if.6.24.198.5.571.5.end
            if.6.27.198.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.198.5.571.5.end:
        func.assert.198.5.571.5.end:
        mov qword [rbp + 176], 2
        mov r15, qword [rbp + 176]
        mov r14, qword [rbp + 176]
        add r14, 1
        mov r13, qword [rbp + r14 * 8 + 144]
        mov qword [rbp + r15 * 8 + 144], r13
        sal qword [rbp + r15 * 8 + 144], 1
        cmp.204.12.571.5:
        cmp qword [rbp + 160], 14
        sete r15b
        bool.204.12.571.5.end:
        func.assert.204.5.571.5:
            if.6.27.204.5.571.5:
            cmp.6.27.204.5.571.5:
            cmp r15b, 0
            jne if.6.24.204.5.571.5.end
            if.6.27.204.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.204.5.571.5.end:
        func.assert.204.5.571.5.end:
        mov qword [rbp + 184], 0
        lea r15, [rbp + 144]
        mov qword [rbp + 200], 0
        foo.208.5.571.5:
            mov r14, qword [r15]
            add qword [rbp + 184], r14
            foo.208.5.571.5.continue:
                add r15, 8
                inc qword [rbp + 200]
                cmp qword [rbp + 200], 4
                jne foo.208.5.571.5
        foo.208.5.571.5.end:
        cmp.211.12.571.5:
        cmp qword [rbp + 184], 26
        sete r15b
        bool.211.12.571.5.end:
        func.assert.211.5.571.5:
            if.6.27.211.5.571.5:
            cmp.6.27.211.5.571.5:
            cmp r15b, 0
            jne if.6.24.211.5.571.5.end
            if.6.27.211.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.211.5.571.5.end:
        func.assert.211.5.571.5.end:
        lea r15, [rbp + 144]
        mov qword [rbp + 200], 0
        foo.217.5.571.5:
            mov r14, qword [rbp + 200]
            mov qword [r15], r14
            add qword [r15], 4
            foo.217.5.571.5.continue:
                add r15, 8
                inc qword [rbp + 200]
                cmp qword [rbp + 200], 4
                jne foo.217.5.571.5
        foo.217.5.571.5.end:
        cmp.220.12.571.5:
        cmp qword [rbp + 144], 4
        sete r15b
        jne bool.220.12.571.5.end
        cmp.220.31.571.5:
        cmp qword [rbp + 168], 7
        sete r15b
        bool.220.12.571.5.end:
        func.assert.220.5.571.5:
            if.6.27.220.5.571.5:
            cmp.6.27.220.5.571.5:
            cmp r15b, 0
            jne if.6.24.220.5.571.5.end
            if.6.27.220.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.220.5.571.5.end:
        func.assert.220.5.571.5.end:
        lea rsi, [rbp + 144]
        lea rdi, [rbp + 192]
        mov rcx, 32
        rep movsb
        mov qword [rbp + 192], 0
        cmp.225.12.571.5:
        cmp qword [rbp + 144], 4
        sete r15b
        bool.225.12.571.5.end:
        func.assert.225.5.571.5:
            if.6.27.225.5.571.5:
            cmp.6.27.225.5.571.5:
            cmp r15b, 0
            jne if.6.24.225.5.571.5.end
            if.6.27.225.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.225.5.571.5.end:
        func.assert.225.5.571.5.end:
        lea rsi, [rbp + 152]
        lea rdi, [rbp + 192]
        mov rcx, 24
        rep movsb
        cmp.229.12.571.5:
        cmp qword [rbp + 192], 5
        sete r15b
        jne bool.229.12.571.5.end
        cmp.229.29.571.5:
        cmp qword [rbp + 208], 7
        sete r15b
        jne bool.229.12.571.5.end
        cmp.229.46.571.5:
        cmp qword [rbp + 216], 7
        sete r15b
        bool.229.12.571.5.end:
        func.assert.229.5.571.5:
            if.6.27.229.5.571.5:
            cmp.6.27.229.5.571.5:
            cmp r15b, 0
            jne if.6.24.229.5.571.5.end
            if.6.27.229.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.229.5.571.5.end:
        func.assert.229.5.571.5.end:
        cmp.233.12.571.5:
            mov rcx, 3
            mov r14, 1
            lea rsi, [rbp + r14 * 8 + 144]
            lea rdi, [rbp + 192]
            shl rcx, 3
            test rcx, rcx
            repe cmpsb
            sete r15b
        bool.233.12.571.5.end:
        func.assert.233.5.571.5:
            if.6.27.233.5.571.5:
            cmp.6.27.233.5.571.5:
            cmp r15b, 0
            jne if.6.24.233.5.571.5.end
            if.6.27.233.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.233.5.571.5.end:
        func.assert.233.5.571.5.end:
        cmp.234.12.571.5:
            lea rsi, [rbp + 144]
            lea rdi, [rbp + 192]
            mov rcx, 4
            repe cmpsq
            setne r15b
        bool.234.12.571.5.end:
        func.assert.234.5.571.5:
            if.6.27.234.5.571.5:
            cmp.6.27.234.5.571.5:
            cmp r15b, 0
            jne if.6.24.234.5.571.5.end
            if.6.27.234.5.571.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.234.5.571.5.end:
        func.assert.234.5.571.5.end:
    func.arrays.571.5.end:
    func.strings.572.5:
        mov rdi, 1
        mov rdx, 24
        lea rsi, [rbp]
        mov rax, 1
        syscall
        mov dword [rbp + 128], 175792482
        cmp.255.12.572.5:
            mov r14, 4
        cmp r14, 4
        sete r15b
        jne bool.255.12.572.5.end
        cmp.255.40.572.5:
        cmp byte [rbp + 128], 98
        sete r15b
        bool.255.12.572.5.end:
        func.assert.255.5.572.5:
            if.6.27.255.5.572.5:
            cmp.6.27.255.5.572.5:
            cmp r15b, 0
            jne if.6.24.255.5.572.5.end
            if.6.27.255.5.572.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.255.5.572.5.end:
        func.assert.255.5.572.5.end:
        mov rdi, 1
        mov rdx, 2
        mov r15, 1
        lea rsi, [rbp + 128]
        add rsi, r15
        mov rax, 1
        syscall
        mov rdi, 1
        mov rdx, 1
        mov r15, 3
        lea rsi, [rbp + 128]
        add rsi, r15
        mov rax, 1
        syscall
    func.strings.572.5.end:
    func.functions.573.5:
        mov qword [rbp + 128], 3
        cmp.300.12.573.5:
            func.twice.300.12.573.5:
                mov r14, qword [rbp + 128]
                sal r14, 1
            func.twice.300.12.573.5.end:
        cmp r14, 6
        sete r15b
        bool.300.12.573.5.end:
        func.assert.300.5.573.5:
            if.6.27.300.5.573.5:
            cmp.6.27.300.5.573.5:
            cmp r15b, 0
            jne if.6.24.300.5.573.5.end
            if.6.27.300.5.573.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.300.5.573.5.end:
        func.assert.300.5.573.5.end:
        cmp.301.12.573.5:
            mov r13, qword [rbp + 128]
            add r13, 1
            func.twice.301.12.573.5:
                mov r14, r13
                sal r14, 1
            func.twice.301.12.573.5.end:
        cmp r14, 8
        sete r15b
        bool.301.12.573.5.end:
        func.assert.301.5.573.5:
            if.6.27.301.5.573.5:
            cmp.6.27.301.5.573.5:
            cmp r15b, 0
            jne if.6.24.301.5.573.5.end
            if.6.27.301.5.573.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.301.5.573.5.end:
        func.assert.301.5.573.5.end:
        func.increment.303.5.573.5:
            add qword [rbp + 128], 1
        func.increment.303.5.573.5.end:
        cmp.304.12.573.5:
        cmp qword [rbp + 128], 4
        sete r15b
        bool.304.12.573.5.end:
        func.assert.304.5.573.5:
            if.6.27.304.5.573.5:
            cmp.6.27.304.5.573.5:
            cmp r15b, 0
            jne if.6.24.304.5.573.5.end
            if.6.27.304.5.573.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.304.5.573.5.end:
        func.assert.304.5.573.5.end:
        cmp.306.12.573.5:
            mov r13, qword [rbp + 128]
            neg r13
            func.sign.306.12.573.5:
                mov r14, 0
                if.280.8.306.12.573.5:
                cmp.280.8.306.12.573.5:
                cmp r13, 0
                je func.sign.306.12.573.5.end
                if.280.8.306.12.573.5.code:
                if.280.5.306.12.573.5.end:
                mov r14, 1
                if.282.8.306.12.573.5:
                cmp.282.8.306.12.573.5:
                cmp r13, 0
                jge if.282.5.306.12.573.5.end
                if.282.8.306.12.573.5.code:
                    mov r14, -1
                if.282.5.306.12.573.5.end:
            func.sign.306.12.573.5.end:
        cmp r14, -1
        sete r15b
        bool.306.12.573.5.end:
        func.assert.306.5.573.5:
            if.6.27.306.5.573.5:
            cmp.6.27.306.5.573.5:
            cmp r15b, 0
            jne if.6.24.306.5.573.5.end
            if.6.27.306.5.573.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.306.5.573.5.end:
        func.assert.306.5.573.5.end:
        cmp.307.12.573.5:
            func.sign.307.12.573.5:
                mov r14, 0
                if.280.8.307.12.573.5:
                cmp.280.8.307.12.573.5:
                if.280.8.307.12.573.5.code:
                    jmp func.sign.307.12.573.5.end
                if.280.5.307.12.573.5.end:
                mov r14, 1
                if.282.8.307.12.573.5:
                cmp.282.8.307.12.573.5:
                if.282.5.307.12.573.5.end:
            func.sign.307.12.573.5.end:
        cmp r14, 0
        sete r15b
        bool.307.12.573.5.end:
        func.assert.307.5.573.5:
            if.6.27.307.5.573.5:
            cmp.6.27.307.5.573.5:
            cmp r15b, 0
            jne if.6.24.307.5.573.5.end
            if.6.27.307.5.573.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.307.5.573.5.end:
        func.assert.307.5.573.5.end:
        mov dword [rbp + 136], 4660
        cmp.310.12.573.5:
            func.low_byte.310.12.573.5:
                mov r14b, byte [rbp + 136]
            func.low_byte.310.12.573.5.end:
        cmp r14b, 52
        sete r15b
        bool.310.12.573.5.end:
        func.assert.310.5.573.5:
            if.6.27.310.5.573.5:
            cmp.6.27.310.5.573.5:
            cmp r15b, 0
            jne if.6.24.310.5.573.5.end
            if.6.27.310.5.573.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.310.5.573.5.end:
        func.assert.310.5.573.5.end:
        mov dword [rbp + 140], 9
        mov dword [rbp + 144], 8
        cmp.313.12.573.5:
            func.first.313.12.573.5:
                mov r14d, dword [rbp + 140]
            func.first.313.12.573.5.end:
        cmp r14d, 9
        sete r15b
        bool.313.12.573.5.end:
        func.assert.313.5.573.5:
            if.6.27.313.5.573.5:
            cmp.6.27.313.5.573.5:
            cmp r15b, 0
            jne if.6.24.313.5.573.5.end
            if.6.27.313.5.573.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.313.5.573.5.end:
        func.assert.313.5.573.5.end:
    func.functions.573.5.end:
    func.user_types.574.5:
        mov qword [rbp + 128], 1
        mov qword [rbp + 136], 2
        cmp.333.12.574.5:
        cmp qword [rbp + 128], 1
        sete r15b
        jne bool.333.12.574.5.end
        cmp.333.25.574.5:
        cmp qword [rbp + 136], 2
        sete r15b
        bool.333.12.574.5.end:
        func.assert.333.5.574.5:
            if.6.27.333.5.574.5:
            cmp.6.27.333.5.574.5:
            cmp r15b, 0
            jne if.6.24.333.5.574.5.end
            if.6.27.333.5.574.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.333.5.574.5.end:
        func.assert.333.5.574.5.end:
        mov qword [rbp + 144], 0
        mov qword [rbp + 152], 0
        cmp.337.12.574.5:
        cmp qword [rbp + 144], 0
        sete r15b
        jne bool.337.12.574.5.end
        cmp.337.30.574.5:
        cmp qword [rbp + 152], 0
        sete r15b
        bool.337.12.574.5.end:
        func.assert.337.5.574.5:
            if.6.27.337.5.574.5:
            cmp.6.27.337.5.574.5:
            cmp r15b, 0
            jne if.6.24.337.5.574.5.end
            if.6.27.337.5.574.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.337.5.574.5.end:
        func.assert.337.5.574.5.end:
        mov qword [rbp + 160], 3
        mov qword [rbp + 168], 4
        mov dword [rbp + 176], 16711680
        mov dword [rbp + 180], 0
        mov qword [rbp + 160], 5
        mov byte [rbp + 181], 122
        cmp.343.12.574.5:
        cmp qword [rbp + 160], 5
        sete r15b
        jne bool.343.12.574.5.end
        cmp.343.30.574.5:
        cmp dword [rbp + 176], 16711680
        sete r15b
        jne bool.343.12.574.5.end
        cmp.343.55.574.5:
        cmp byte [rbp + 181], 122
        sete r15b
        bool.343.12.574.5.end:
        func.assert.343.5.574.5:
            if.6.27.343.5.574.5:
            cmp.6.27.343.5.574.5:
            cmp r15b, 0
            jne if.6.24.343.5.574.5.end
            if.6.27.343.5.574.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.343.5.574.5.end:
        func.assert.343.5.574.5.end:
        mov rax, qword [rbp + 160]
        mov qword [rbp + 184], rax
        mov rax, qword [rbp + 168]
        mov qword [rbp + 192], rax
        mov qword [rbp + 192], 4
        cmp.348.12.574.5:
            lea rsi, [rbp + 184]
            lea rdi, [rbp + 160]
            cmpsq
            jne .Lbaz_equal.0
            cmpsq
            .Lbaz_equal.0:
            sete r15b
        bool.348.12.574.5.end:
        func.assert.348.5.574.5:
            if.6.27.348.5.574.5:
            cmp.6.27.348.5.574.5:
            cmp r15b, 0
            jne if.6.24.348.5.574.5.end
            if.6.27.348.5.574.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.348.5.574.5.end:
        func.assert.348.5.574.5.end:
        mov qword [rbp + 200], 1
        mov qword [rbp + 208], 2
        mov qword [rbp + 216], 3
        mov qword [rbp + 224], 4
        mov qword [rbp + 232], 0
        mov qword [rbp + 240], 0
        mov rax, qword [rbp + 200]
        mov qword [rbp + 232], rax
        mov rax, qword [rbp + 208]
        mov qword [rbp + 240], rax
        cmp.353.12.574.5:
        cmp qword [rbp + 240], 2
        sete r15b
        bool.353.12.574.5.end:
        func.assert.353.5.574.5:
            if.6.27.353.5.574.5:
            cmp.6.27.353.5.574.5:
            cmp r15b, 0
            jne if.6.24.353.5.574.5.end
            if.6.27.353.5.574.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.353.5.574.5.end:
        func.assert.353.5.574.5.end:
    func.user_types.574.5.end:
    func.methods.575.5:
        mov qword [rbp + 128], 1
        mov qword [rbp + 136], 2
        func.point.move.379.7.575.5:
            add qword [rbp + 128], 10
            add qword [rbp + 136], 20
        func.point.move.379.7.575.5.end:
        cmp.380.12.575.5:
        cmp qword [rbp + 128], 11
        sete r15b
        jne bool.380.12.575.5.end
        cmp.380.26.575.5:
        cmp qword [rbp + 136], 22
        sete r15b
        bool.380.12.575.5.end:
        func.assert.380.5.575.5:
            if.6.27.380.5.575.5:
            cmp.6.27.380.5.575.5:
            cmp r15b, 0
            jne if.6.24.380.5.575.5.end
            if.6.27.380.5.575.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.380.5.575.5.end:
        func.assert.380.5.575.5.end:
        cmp.381.12.575.5:
            func.point.sum.381.14.575.5:
                mov r14, qword [rbp + 128]
                add r14, qword [rbp + 136]
            func.point.sum.381.14.575.5.end:
        cmp r14, 33
        sete r15b
        bool.381.12.575.5.end:
        func.assert.381.5.575.5:
            if.6.27.381.5.575.5:
            cmp.6.27.381.5.575.5:
            cmp r15b, 0
            jne if.6.24.381.5.575.5.end
            if.6.27.381.5.575.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.381.5.575.5.end:
        func.assert.381.5.575.5.end:
        func.point.x.383.7.575.5:
            mov qword [rbp + 128], 0
        func.point.x.383.7.575.5.end:
        cmp.384.12.575.5:
        cmp qword [rbp + 128], 0
        sete r15b
        bool.384.12.575.5.end:
        func.assert.384.5.575.5:
            if.6.27.384.5.575.5:
            cmp.6.27.384.5.575.5:
            cmp r15b, 0
            jne if.6.24.384.5.575.5.end
            if.6.27.384.5.575.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.384.5.575.5.end:
        func.assert.384.5.575.5.end:
    func.methods.575.5.end:
    func.constructors.576.5:
        func.point.at.399.13.576.5:
            mov qword [rbp + 128], -1
            mov qword [rbp + 136], -2
        func.point.at.399.13.576.5.end:
        cmp.400.12.576.5:
        cmp qword [rbp + 128], -1
        sete r15b
        jne bool.400.12.576.5.end
        cmp.400.26.576.5:
        cmp qword [rbp + 136], -2
        sete r15b
        bool.400.12.576.5.end:
        func.assert.400.5.576.5:
            if.6.27.400.5.576.5:
            cmp.6.27.400.5.576.5:
            cmp r15b, 0
            jne if.6.24.400.5.576.5.end
            if.6.27.400.5.576.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.400.5.576.5.end:
        func.assert.400.5.576.5.end:
        mov qword [rbp + 144], 0
        mov qword [rbp + 152], 0
        mov qword [rbp + 160], 0
        mov qword [rbp + 168], 0
        func.point.at.404.14.576.5:
            mov qword [rbp + 160], 3
            mov qword [rbp + 168], 4
        func.point.at.404.14.576.5.end:
        cmp.405.12.576.5:
        cmp qword [rbp + 168], 4
        sete r15b
        bool.405.12.576.5.end:
        func.assert.405.5.576.5:
            if.6.27.405.5.576.5:
            cmp.6.27.405.5.576.5:
            cmp r15b, 0
            jne if.6.24.405.5.576.5.end
            if.6.27.405.5.576.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.405.5.576.5.end:
        func.assert.405.5.576.5.end:
    func.constructors.576.5.end:
    func.arrays_in_types.577.5:
        mov dword [rbp + 128], 7
        mov dword [rbp + 132], 0
        mov dword [rbp + 136], 8
        mov dword [rbp + 140], 0
        mov qword [rbp + 144], 0
        mov qword [rbp + 152], 0
        mov qword [rbp + 160], 2
        cmp.438.12.577.5:
        cmp qword [rbp + 136], 8
        sete r15b
        jne bool.438.12.577.5.end
        cmp.438.32.577.5:
        cmp qword [rbp + 144], 0
        sete r15b
        jne bool.438.12.577.5.end
        cmp.438.52.577.5:
        cmp qword [rbp + 160], 2
        sete r15b
        bool.438.12.577.5.end:
        func.assert.438.5.577.5:
            if.6.27.438.5.577.5:
            cmp.6.27.438.5.577.5:
            cmp r15b, 0
            jne if.6.24.438.5.577.5.end
            if.6.27.438.5.577.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.438.5.577.5.end:
        func.assert.438.5.577.5.end:
        func.stack.push.442.7.577.5:
            mov r15, qword [rbp + 160]
            mov qword [rbp + r15 * 8 + 128], 9
            add qword [rbp + 160], 1
        func.stack.push.442.7.577.5.end:
        cmp.443.12.577.5:
        cmp qword [rbp + 160], 3
        sete r15b
        jne bool.443.12.577.5.end
        cmp.443.29.577.5:
        cmp qword [rbp + 144], 9
        sete r15b
        bool.443.12.577.5.end:
        func.assert.443.5.577.5:
            if.6.27.443.5.577.5:
            cmp.6.27.443.5.577.5:
            cmp r15b, 0
            jne if.6.24.443.5.577.5.end
            if.6.27.443.5.577.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.443.5.577.5.end:
        func.assert.443.5.577.5.end:
        cmp.444.12.577.5:
            func.stack.pop.444.14.577.5:
                sub qword [rbp + 160], 1
                mov r13, qword [rbp + 160]
                mov r14, qword [rbp + r13 * 8 + 128]
            func.stack.pop.444.14.577.5.end:
        cmp r14, 9
        sete r15b
        bool.444.12.577.5.end:
        func.assert.444.5.577.5:
            if.6.27.444.5.577.5:
            cmp.6.27.444.5.577.5:
            cmp r15b, 0
            jne if.6.24.444.5.577.5.end
            if.6.27.444.5.577.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.444.5.577.5.end:
        func.assert.444.5.577.5.end:
        cmp.445.12.577.5:
        cmp qword [rbp + 160], 2
        sete r15b
        bool.445.12.577.5.end:
        func.assert.445.5.577.5:
            if.6.27.445.5.577.5:
            cmp.6.27.445.5.577.5:
            cmp r15b, 0
            jne if.6.24.445.5.577.5.end
            if.6.27.445.5.577.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.445.5.577.5.end:
        func.assert.445.5.577.5.end:
        cmp.448.12.577.5:
            mov r14, 4
        cmp r14, 4
        sete r15b
        bool.448.12.577.5.end:
        func.assert.448.5.577.5:
            if.6.27.448.5.577.5:
            cmp.6.27.448.5.577.5:
            cmp r15b, 0
            jne if.6.24.448.5.577.5.end
            if.6.27.448.5.577.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.448.5.577.5.end:
        func.assert.448.5.577.5.end:
        cmp.449.12.577.5:
            func.sum_all.449.12.577.5:
                mov r14, 0
                lea r13, [rbp + 128]
                mov qword [rbp + 176], 0
                foo.431.5.449.12.577.5:
                    add r14, qword [r13]
                    foo.431.5.449.12.577.5.continue:
                        add r13, 8
                        inc qword [rbp + 176]
                        cmp qword [rbp + 176], 4
                        jne foo.431.5.449.12.577.5
                foo.431.5.449.12.577.5.end:
            func.sum_all.449.12.577.5.end:
        cmp r14, 24
        sete r15b
        bool.449.12.577.5.end:
        func.assert.449.5.577.5:
            if.6.27.449.5.577.5:
            cmp.6.27.449.5.577.5:
            cmp r15b, 0
            jne if.6.24.449.5.577.5.end
            if.6.27.449.5.577.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.449.5.577.5.end:
        func.assert.449.5.577.5.end:
        lea rsi, [rbp + 128]
        lea rdi, [rbp + 168]
        mov rcx, 40
        rep movsb
        mov qword [rbp + 168], 0
        cmp.455.12.577.5:
        cmp qword [rbp + 128], 7
        sete r15b
        jne bool.455.12.577.5.end
        cmp.455.32.577.5:
            lea rsi, [rbp + 128]
            lea rdi, [rbp + 168]
            mov rcx, 5
            repe cmpsq
            setne r15b
        bool.455.12.577.5.end:
        func.assert.455.5.577.5:
            if.6.27.455.5.577.5:
            cmp.6.27.455.5.577.5:
            cmp r15b, 0
            jne if.6.24.455.5.577.5.end
            if.6.27.455.5.577.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.455.5.577.5.end:
        func.assert.455.5.577.5.end:
    func.arrays_in_types.577.5.end:
    func.data.578.5:
        cmp.478.12.578.5:
        cmp qword [rbp + 48], 8
        sete r15b
        bool.478.12.578.5.end:
        func.assert.478.5.578.5:
            if.6.27.478.5.578.5:
            cmp.6.27.478.5.578.5:
            cmp r15b, 0
            jne if.6.24.478.5.578.5.end
            if.6.27.478.5.578.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.478.5.578.5.end:
        func.assert.478.5.578.5.end:
        mov qword [rbp + 24], 3
        cmp.480.12.578.5:
        cmp qword [rbp + 24], 3
        sete r15b
        bool.480.12.578.5.end:
        func.assert.480.5.578.5:
            if.6.27.480.5.578.5:
            cmp.6.27.480.5.578.5:
            cmp r15b, 0
            jne if.6.24.480.5.578.5.end
            if.6.27.480.5.578.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.480.5.578.5.end:
        func.assert.480.5.578.5.end:
        func.count_call.483.5.578.5:
            add qword [rbp + 56], 1
        func.count_call.483.5.578.5.end:
        func.count_call.484.5.578.5:
            add qword [rbp + 56], 1
        func.count_call.484.5.578.5.end:
        cmp.485.12.578.5:
        cmp qword [rbp + 56], 2
        sete r15b
        bool.485.12.578.5.end:
        func.assert.485.5.578.5:
            if.6.27.485.5.578.5:
            cmp.6.27.485.5.578.5:
            cmp r15b, 0
            jne if.6.24.485.5.578.5.end
            if.6.27.485.5.578.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.485.5.578.5.end:
        func.assert.485.5.578.5.end:
        func.point.move.489.10.578.5:
            add qword [rbp + 64], 1
            add qword [rbp + 72], 1
        func.point.move.489.10.578.5.end:
        cmp.490.12.578.5:
        cmp qword [rbp + 64], 11
        sete r15b
        jne bool.490.12.578.5.end
        cmp.490.29.578.5:
        cmp qword [rbp + 72], 21
        sete r15b
        bool.490.12.578.5.end:
        func.assert.490.5.578.5:
            if.6.27.490.5.578.5:
            cmp.6.27.490.5.578.5:
            cmp r15b, 0
            jne if.6.24.490.5.578.5.end
            if.6.27.490.5.578.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.490.5.578.5.end:
        func.assert.490.5.578.5.end:
        func.stack.push.491.13.578.5:
            mov r15, qword [rbp + 112]
            mov qword [rbp + r15 * 8 + 80], 3
            add qword [rbp + 112], 1
        func.stack.push.491.13.578.5.end:
        cmp.492.12.578.5:
        cmp qword [rbp + 96], 3
        sete r15b
        jne bool.492.12.578.5.end
        cmp.492.38.578.5:
        cmp qword [rbp + 112], 3
        sete r15b
        bool.492.12.578.5.end:
        func.assert.492.5.578.5:
            if.6.27.492.5.578.5:
            cmp.6.27.492.5.578.5:
            cmp r15b, 0
            jne if.6.24.492.5.578.5.end
            if.6.27.492.5.578.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.492.5.578.5.end:
        func.assert.492.5.578.5.end:
        lea rsi, [rbp + 24]
        lea rdi, [rbp + 128]
        mov rcx, 32
        rep movsb
        mov qword [rbp + 136], 0
        cmp.497.12.578.5:
        cmp qword [rbp + 32], 2
        sete r15b
        bool.497.12.578.5.end:
        func.assert.497.5.578.5:
            if.6.27.497.5.578.5:
            cmp.6.27.497.5.578.5:
            cmp r15b, 0
            jne if.6.24.497.5.578.5.end
            if.6.27.497.5.578.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.497.5.578.5.end:
        func.assert.497.5.578.5.end:
    func.data.578.5.end:
    func.non_inlined.579.5:
        mov qword [rbp + 128], 5
        lea r15, [rbp + 136]
        mov qword [rbp + 144], r15
        lea r15, [rbp + 128]
        mov qword [rbp + 152], r15
        lea rbx, [rbp + 144]
        call func.factorial
        cmp.520.12.579.5:
        cmp qword [rbp + 136], 120
        sete r15b
        bool.520.12.579.5.end:
        func.assert.520.5.579.5:
            if.6.27.520.5.579.5:
            cmp.6.27.520.5.579.5:
            cmp r15b, 0
            jne if.6.24.520.5.579.5.end
            if.6.27.520.5.579.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.520.5.579.5.end:
        func.assert.520.5.579.5.end:
    func.non_inlined.579.5.end:
    func.checks.580.5:
        mov qword [rbp + 128], 0
        mov qword [rbp + 136], 0
        mov qword [rbp + 144], 0
        mov qword [rbp + 152], 0
        mov qword [rbp + 160], 3
        mov r15, qword [rbp + 160]
        mov qword [rbp + r15 * 8 + 128], 1
        cmp.538.12.580.5:
        cmp qword [rbp + 152], 1
        sete r15b
        bool.538.12.580.5.end:
        func.assert.538.5.580.5:
            if.6.27.538.5.580.5:
            cmp.6.27.538.5.580.5:
            cmp r15b, 0
            jne if.6.24.538.5.580.5.end
            if.6.27.538.5.580.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.538.5.580.5.end:
        func.assert.538.5.580.5.end:
        mov qword [rbp + 168], 5
        func.add_to_zero.545.13.580.5:
            mov qword [rbp + 176], 0
            mov r15, qword [rbp + 168]
            add qword [rbp + 176], r15
        func.add_to_zero.545.13.580.5.end:
        cmp.546.12.580.5:
        cmp qword [rbp + 176], 5
        sete r15b
        bool.546.12.580.5.end:
        func.assert.546.5.580.5:
            if.6.27.546.5.580.5:
            cmp.6.27.546.5.580.5:
            cmp r15b, 0
            jne if.6.24.546.5.580.5.end
            if.6.27.546.5.580.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.546.5.580.5.end:
        func.assert.546.5.580.5.end:
        mov qword [rbp + 184], 1
        mov qword [rbp + 192], 2
        mov qword [rbp + 184], 2
        mov qword [rbp + 192], 1
        cmp.556.12.580.5:
        cmp qword [rbp + 184], 2
        sete r15b
        jne bool.556.12.580.5.end
        cmp.556.25.580.5:
        cmp qword [rbp + 192], 1
        sete r15b
        bool.556.12.580.5.end:
        func.assert.556.5.580.5:
            if.6.27.556.5.580.5:
            cmp.6.27.556.5.580.5:
            cmp r15b, 0
            jne if.6.24.556.5.580.5.end
            if.6.27.556.5.580.5.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.6.24.556.5.580.5.end:
        func.assert.556.5.580.5.end:
    func.checks.580.5.end:
    mov rdi, 0
    mov rax, 60
    syscall
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.507.8:
    cmp.507.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.507.5.end
    if.507.8.code:
        ret
    if.507.5.end:
    mov r15, qword [rbx + 8]
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
    sub qword [rbx + 16], 1
    lea r15, [rbx + 24]
    mov qword [rbx + 32], r15
    lea r15, [rbx + 16]
    mov qword [rbx + 40], r15
    push rbx
    lea rbx, [rbx + 32]
    call func.factorial
    pop rbx
    mov r15, qword [rbx]
    mov r13, qword [rbx + 8]
    mov r14, qword [r13]
    imul r14, qword [rbx + 24]
    mov qword [r15], r14
    ret
size.func.factorial equ 32
section .rodata
init.197.21:
db `\x02\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00\x00\x00\x00\x00`
section .text
section .data
align 16
dat:
db `hello from the tutorial\n`
dq 1
dq 2
dq 4
dq 8
dq 0
dq 10
dq 20
dq 1, 2
times 2 dq 0
dq 2
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 65536
vars.end:
