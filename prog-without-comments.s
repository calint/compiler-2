default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 384], 0
    cmp.187.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.187.12.end:
    func.assert.187.5:
        if.38.27.187.5:
        cmp.38.27.187.5:
        cmp r15b, 0
        jne if.38.24.187.5.end
        if.38.27.187.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.187.5.end:
    func.assert.187.5.end:
    mov qword [rbp + 384], -1
    cmp.190.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.190.12.end:
    func.assert.190.5:
        if.38.27.190.5:
        cmp.38.27.190.5:
        cmp r15b, 0
        jne if.38.24.190.5.end
        if.38.27.190.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.190.5.end:
    func.assert.190.5.end:
        func.assert.196.9:
            if.38.27.196.9:
            cmp.38.27.196.9:
            if.38.24.196.9.end:
        func.assert.196.9.end:
    func.assert.199.5:
        if.38.27.199.5:
        cmp.38.27.199.5:
        if.38.24.199.5.end:
    func.assert.199.5.end:
    cmp.201.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.201.12.end
    cmp.201.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.201.12.end:
    func.assert.201.5:
        if.38.27.201.5:
        cmp.38.27.201.5:
        cmp r15b, 0
        jne if.38.24.201.5.end
        if.38.27.201.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.201.5.end:
    func.assert.201.5.end:
    cmp.202.12:
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.202.12.end
    cmp.202.30:
    cmp byte [rbp + 227], 122
    sete r15b
    bool.202.12.end:
    func.assert.202.5:
        if.38.27.202.5:
        cmp.38.27.202.5:
        cmp r15b, 0
        jne if.38.24.202.5.end
        if.38.27.202.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.202.5.end:
    func.assert.202.5.end:
    mov qword [rbp + 392], 7
    cmp.205.12:
        mov r14, qword [rbp + 392]
        and r14, 3
    cmp r14, 3
    sete r15b
    bool.205.12.end:
    func.assert.205.5:
        if.38.27.205.5:
        cmp.38.27.205.5:
        cmp r15b, 0
        jne if.38.24.205.5.end
        if.38.27.205.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.205.5.end:
    func.assert.205.5.end:
    cmp.206.12:
        mov r14, qword [rbp + 392]
        or r14, 8
    cmp r14, 15
    sete r15b
    bool.206.12.end:
    func.assert.206.5:
        if.38.27.206.5:
        cmp.38.27.206.5:
        cmp r15b, 0
        jne if.38.24.206.5.end
        if.38.27.206.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.206.5.end:
    func.assert.206.5.end:
    cmp.207.12:
        mov r14, qword [rbp + 392]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.207.12.end:
    func.assert.207.5:
        if.38.27.207.5:
        cmp.38.27.207.5:
        cmp r15b, 0
        jne if.38.24.207.5.end
        if.38.27.207.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.207.5.end:
    func.assert.207.5.end:
    cmp.208.12:
        mov r14, qword [rbp + 392]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.208.12.end:
    func.assert.208.5:
        if.38.27.208.5:
        cmp.38.27.208.5:
        cmp r15b, 0
        jne if.38.24.208.5.end
        if.38.27.208.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.208.5.end:
    func.assert.208.5.end:
    cmp.209.12:
        mov r14, qword [rbp + 392]
        neg r14
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.209.12.end:
    func.assert.209.5:
        if.38.27.209.5:
        cmp.38.27.209.5:
        cmp r15b, 0
        jne if.38.24.209.5.end
        if.38.27.209.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.209.5.end:
    func.assert.209.5.end:
    cmp.212.12:
        mov r14, qword [rbp + 392]
        mov r13, qword [rbp + 392]
        sal r13, 1
        add r14, r13
    cmp r14, 21
    sete r15b
    bool.212.12.end:
    func.assert.212.5:
        if.38.27.212.5:
        cmp.38.27.212.5:
        cmp r15b, 0
        jne if.38.24.212.5.end
        if.38.27.212.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.212.5.end:
    func.assert.212.5.end:
    cmp.213.12:
        mov r14, qword [rbp + 392]
        add r14, qword [rbp + 392]
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.213.12.end:
    func.assert.213.5:
        if.38.27.213.5:
        cmp.38.27.213.5:
        cmp r15b, 0
        jne if.38.24.213.5.end
        if.38.27.213.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.213.5.end:
    func.assert.213.5.end:
    cmp.216.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.216.12.end
    cmp.216.23:
    cmp.216.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.216.12.end
    cmp.216.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.216.12.end:
    func.assert.216.5:
        if.38.27.216.5:
        cmp.38.27.216.5:
        cmp r15b, 0
        jne if.38.24.216.5.end
        if.38.27.216.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.216.5.end:
    func.assert.216.5.end:
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.221.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.221.12.end:
    func.assert.221.5:
        if.38.27.221.5:
        cmp.38.27.221.5:
        cmp r15b, 0
        jne if.38.24.221.5.end
        if.38.27.221.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.221.5.end:
    func.assert.221.5.end:
    movsx r15, byte [rbp + 400]
    mov qword [rbp + 408], r15
    cmp.225.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.225.12.end:
    func.assert.225.5:
        if.38.27.225.5:
        cmp.38.27.225.5:
        cmp r15b, 0
        jne if.38.24.225.5.end
        if.38.27.225.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.225.5.end:
    func.assert.225.5.end:
    mov qword [rbp + 416], 65
    cmp.233.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.233.12.end:
    func.assert.233.5:
        if.38.27.233.5:
        cmp.38.27.233.5:
        cmp r15b, 0
        jne if.38.24.233.5.end
        if.38.27.233.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.233.5.end:
    func.assert.233.5.end:
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
    mov qword [rbp + 440], 1
    mov r15, qword [rbp + 440]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 424], 2
    mov r15, qword [rbp + 440]
    add r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14, qword [rbp + 440]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
    cmp.241.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.241.12.end:
    func.assert.241.5:
        if.38.27.241.5:
        cmp.38.27.241.5:
        cmp r15b, 0
        jne if.38.24.241.5.end
        if.38.27.241.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.241.5.end:
    func.assert.241.5.end:
    cmp.242.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.242.12.end:
    func.assert.242.5:
        if.38.27.242.5:
        cmp.38.27.242.5:
        cmp r15b, 0
        jne if.38.24.242.5.end
        if.38.27.242.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.242.5.end:
    func.assert.242.5.end:
    mov r15, 2
    mov r14, 2
    test r14, r14
    js baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    mov r13, r15
    add r13, r14
    cmp r13, 4
    jg baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jg baz_bounds_panic
    mov rax, qword [rbp + r14 * 4 + 424]
    mov qword [rbp + 424], rax
    cmp.245.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.245.12.end:
    func.assert.245.5:
        if.38.27.245.5:
        cmp.38.27.245.5:
        cmp r15b, 0
        jne if.38.24.245.5.end
        if.38.27.245.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.245.5.end:
    func.assert.245.5.end:
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
    mov r15, 4
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jg baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    cmp r15, 8
    jg baz_bounds_panic
    mov rax, qword [rbp + 424]
    mov qword [rbp + 448], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 456], rax
    cmp.250.14:
        mov rcx, 3
        mov r15, 1
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
        mov r14, rcx
        add r14, r15
        cmp r14, 4
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 424]
        mov r15, 1
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
        mov r14, rcx
        add r14, r15
        cmp r14, 8
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 480]
    bool.250.14.end:
    cmp.253.12:
    mov r15b, byte [rbp + 480]
    bool.253.12.end:
    func.assert.253.5:
        if.38.27.253.5:
        cmp.38.27.253.5:
        cmp r15b, 0
        jne if.38.24.253.5.end
        if.38.27.253.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.253.5.end:
    func.assert.253.5.end:
    mov dword [rbp + 456], -1
    cmp.256.12:
        mov rcx, 4
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 424]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.256.12.end:
    func.assert.256.5:
        if.38.27.256.5:
        cmp.38.27.256.5:
        cmp r15b, 0
        jne if.38.24.256.5.end
        if.38.27.256.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.256.5.end:
    func.assert.256.5.end:
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
    cmp.259.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.259.12.end:
    func.assert.259.5:
        if.38.27.259.5:
        cmp.38.27.259.5:
        cmp r15b, 0
        jne if.38.24.259.5.end
        if.38.27.259.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.259.5.end:
    func.assert.259.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.267.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv.267.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.269.12:
    mov r14, qword [rbp + 440]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
    bool.269.12.end:
    func.assert.269.5:
        if.38.27.269.5:
        cmp.38.27.269.5:
        cmp r15b, 0
        jne if.38.24.269.5.end
        if.38.27.269.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.269.5.end:
    func.assert.269.5.end:
    func.faz.271.5:
        mov dword [rbp + 428], 254
    func.faz.271.5.end:
    cmp.272.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.272.12.end:
    func.assert.272.5:
        if.38.27.272.5:
        cmp.38.27.272.5:
        cmp r15b, 0
        jne if.38.24.272.5.end
        if.38.27.272.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.272.5.end:
    func.assert.272.5.end:
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov qword [rbp + 528], 0
    foo.275.5:
        mov r14, qword [rbp + 528]
        add qword [r15], r14
        add qword [r15], 2
        foo.275.5.continue:
            add r15, 8
            inc qword [rbp + 528]
            cmp qword [rbp + 528], 2
            jne foo.275.5
    foo.275.5.end:
    cmp.278.12:
    cmp qword [rbp + 504], 5
    sete r15b
    bool.278.12.end:
    func.assert.278.5:
        if.38.27.278.5:
        cmp.38.27.278.5:
        cmp r15b, 0
        jne if.38.24.278.5.end
        if.38.27.278.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.278.5.end:
    func.assert.278.5.end:
    cmp.279.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.279.12.end:
    func.assert.279.5:
        if.38.27.279.5:
        cmp.38.27.279.5:
        cmp r15b, 0
        jne if.38.24.279.5.end
        if.38.27.279.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.279.5.end:
    func.assert.279.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz.287.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz.287.7.end:
    cmp.290.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.290.12.end:
    func.assert.290.5:
        if.38.27.290.5:
        cmp.38.27.290.5:
        cmp r15b, 0
        jne if.38.24.290.5.end
        if.38.27.290.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.290.5.end:
    func.assert.290.5.end:
    cmp.291.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.291.12.end:
    func.assert.291.5:
        if.38.27.291.5:
        cmp.38.27.291.5:
        cmp r15b, 0
        jne if.38.24.291.5.end
        if.38.27.291.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.291.5.end:
    func.assert.291.5.end:
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
    cmp.296.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    bool.296.12.end:
    func.assert.296.5:
        if.38.27.296.5:
        cmp.38.27.296.5:
        cmp r15b, 0
        jne if.38.24.296.5.end
        if.38.27.296.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.296.5.end:
    func.assert.296.5.end:
    mov qword [rbp + 536], 3
    cmp.301.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    bool.301.12.end:
    func.assert.301.5:
        if.38.27.301.5:
        cmp.38.27.301.5:
        cmp r15b, 0
        jne if.38.24.301.5.end
        if.38.27.301.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.301.5.end:
    func.assert.301.5.end:
    mov qword [rbp + 552], 0
    func.bar.304.5:
        if.59.8.304.5:
        cmp.59.8.304.5:
        cmp qword [rbp + 552], 0
        je func.bar.304.5.end
        if.59.8.304.5.code:
        if.59.5.304.5.end:
        mov qword [rbp + 552], 255
    func.bar.304.5.end:
    cmp.305.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.305.12.end:
    func.assert.305.5:
        if.38.27.305.5:
        cmp.38.27.305.5:
        cmp r15b, 0
        jne if.38.24.305.5.end
        if.38.27.305.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.305.5.end:
    func.assert.305.5.end:
    mov qword [rbp + 552], 1
    func.bar.308.5:
        if.59.8.308.5:
        cmp.59.8.308.5:
        cmp qword [rbp + 552], 0
        je func.bar.308.5.end
        if.59.8.308.5.code:
        if.59.5.308.5.end:
        mov qword [rbp + 552], 255
    func.bar.308.5.end:
    cmp.309.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.309.12.end:
    func.assert.309.5:
        if.38.27.309.5:
        cmp.38.27.309.5:
        cmp r15b, 0
        jne if.38.24.309.5.end
        if.38.27.309.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.309.5.end:
    func.assert.309.5.end:
    mov qword [rbp + 560], 1
    func.baz.312.13:
        mov r15, qword [rbp + 560]
        mov qword [rbp + 568], r15
        sal qword [rbp + 568], 1
    func.baz.312.13.end:
    cmp.313.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.313.12.end:
    func.assert.313.5:
        if.38.27.313.5:
        cmp.38.27.313.5:
        cmp r15b, 0
        jne if.38.24.313.5.end
        if.38.27.313.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.313.5.end:
    func.assert.313.5.end:
    func.baz.315.9:
        mov qword [rbp + 568], 2
    func.baz.315.9.end:
    cmp.316.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.316.12.end:
    func.assert.316.5:
        if.38.27.316.5:
        cmp.38.27.316.5:
        cmp r15b, 0
        jne if.38.24.316.5.end
        if.38.27.316.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.316.5.end:
    func.assert.316.5.end:
    mov qword [rbp + 576], 5
    lea r15, [rbp + 592]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.factorial
    cmp r15, r14
    ja baz_frame_overflow
    lea r15, [rbp + 584]
    mov qword [rbp + 592], r15
    lea r15, [rbp + 576]
    mov qword [rbp + 600], r15
    lea rbx, [rbp + 592]
    call func.factorial
    cmp.320.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.320.12.end:
    func.assert.320.5:
        if.38.27.320.5:
        cmp.38.27.320.5:
        cmp r15b, 0
        jne if.38.24.320.5.end
        if.38.27.320.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.320.5.end:
    func.assert.320.5.end:
    func.baz.322.20:
        mov qword [rbp + 592], 6
    func.baz.322.20.end:
    mov qword [rbp + 600], 0
    cmp.323.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.323.12.end:
    func.assert.323.5:
        if.38.27.323.5:
        cmp.38.27.323.5:
        cmp r15b, 0
        jne if.38.24.323.5.end
        if.38.27.323.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.323.5.end:
    func.assert.323.5.end:
    func.point.at.325.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at.325.14.end:
    cmp.329.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.329.12.end:
    func.assert.329.5:
        if.38.27.329.5:
        cmp.38.27.329.5:
        cmp r15b, 0
        jne if.38.24.329.5.end
        if.38.27.329.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.329.5.end:
    func.assert.329.5.end:
    cmp.330.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.330.12.end:
    func.assert.330.5:
        if.38.27.330.5:
        cmp.38.27.330.5:
        cmp r15b, 0
        jne if.38.24.330.5.end
        if.38.27.330.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.330.5.end:
    func.assert.330.5.end:
    func.point.x.332.8:
        mov qword [rbp + 608], 2
    func.point.x.332.8.end:
    cmp.333.12:
    cmp qword [rbp + 608], 2
    sete r15b
    bool.333.12.end:
    func.assert.333.5:
        if.38.27.333.5:
        cmp.38.27.333.5:
        cmp r15b, 0
        jne if.38.24.333.5.end
        if.38.27.333.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.333.5.end:
    func.assert.333.5.end:
    cmp.334.12:
        func.point.sum.334.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
        func.point.sum.334.15.end:
    cmp r14, 0
    sete r15b
    bool.334.12.end:
    func.assert.334.5:
        if.38.27.334.5:
        cmp.38.27.334.5:
        cmp r15b, 0
        jne if.38.24.334.5.end
        if.38.27.334.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.334.5.end:
    func.assert.334.5.end:
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.340.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.340.12.end:
    func.assert.340.5:
        if.38.27.340.5:
        cmp.38.27.340.5:
        cmp r15b, 0
        jne if.38.24.340.5.end
        if.38.27.340.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.340.5.end:
    func.assert.340.5.end:
    cmp.341.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.341.12.end:
    func.assert.341.5:
        if.38.27.341.5:
        cmp.38.27.341.5:
        cmp r15b, 0
        jne if.38.24.341.5.end
        if.38.27.341.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.341.5.end:
    func.assert.341.5.end:
    cmp.342.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.342.12.end:
    func.assert.342.5:
        if.38.27.342.5:
        cmp.38.27.342.5:
        cmp r15b, 0
        jne if.38.24.342.5.end
        if.38.27.342.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.342.5.end:
    func.assert.342.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 664], r15
    neg qword [rbp + 664]
    mov r15, qword [rbp + 632]
    mov qword [rbp + 672], r15
    neg qword [rbp + 672]
    mov rax, qword [rbp + 664]
    mov qword [rbp + 640], rax
    mov rax, qword [rbp + 672]
    mov qword [rbp + 648], rax
    cmp.346.12:
    cmp qword [rbp + 640], -1
    sete r15b
    bool.346.12.end:
    func.assert.346.5:
        if.38.27.346.5:
        cmp.38.27.346.5:
        cmp r15b, 0
        jne if.38.24.346.5.end
        if.38.27.346.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.346.5.end:
    func.assert.346.5.end:
    cmp.347.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.347.12.end:
    func.assert.347.5:
        if.38.27.347.5:
        cmp.38.27.347.5:
        cmp r15b, 0
        jne if.38.24.347.5.end
        if.38.27.347.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.347.5.end:
    func.assert.347.5.end:
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.350.12:
    cmp qword [rbp + 680], -1
    sete r15b
    bool.350.12.end:
    func.assert.350.5:
        if.38.27.350.5:
        cmp.38.27.350.5:
        cmp r15b, 0
        jne if.38.24.350.5.end
        if.38.27.350.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.350.5.end:
    func.assert.350.5.end:
    cmp.351.12:
    cmp qword [rbp + 688], -2
    sete r15b
    bool.351.12.end:
    func.assert.351.5:
        if.38.27.351.5:
        cmp.38.27.351.5:
        cmp r15b, 0
        jne if.38.24.351.5.end
        if.38.27.351.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.351.5.end:
    func.assert.351.5.end:
    cmp.352.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.352.12.end:
    func.assert.352.5:
        if.38.27.352.5:
        cmp.38.27.352.5:
        cmp r15b, 0
        jne if.38.24.352.5.end
        if.38.27.352.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.352.5.end:
    func.assert.352.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.355.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.355.12.end:
    func.assert.355.5:
        if.38.27.355.5:
        cmp.38.27.355.5:
        cmp r15b, 0
        jne if.38.24.355.5.end
        if.38.27.355.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.355.5.end:
    func.assert.355.5.end:
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.357.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.357.12.end:
    func.assert.357.5:
        if.38.27.357.5:
        cmp.38.27.357.5:
        cmp r15b, 0
        jne if.38.24.357.5.end
        if.38.27.357.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.357.5.end:
    func.assert.357.5.end:
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.367.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.367.12.end:
    func.assert.367.5:
        if.38.27.367.5:
        cmp.38.27.367.5:
        cmp r15b, 0
        jne if.38.24.367.5.end
        if.38.27.367.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.367.5.end:
    func.assert.367.5.end:
    func.object.at.368.13:
        func.point.at.119.16.368.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at.119.16.368.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at.368.13.end:
    cmp.369.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.369.12.end:
    func.assert.369.5:
        if.38.27.369.5:
        cmp.38.27.369.5:
        cmp r15b, 0
        jne if.38.24.369.5.end
        if.38.27.369.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.369.5.end:
    func.assert.369.5.end:
    func.point.fooz.371.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz.371.15.end:
    cmp.372.12:
        func.point.sum.372.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
        func.point.sum.372.22.end:
    cmp r14, 13
    sete r15b
    bool.372.12.end:
    func.assert.372.5:
        if.38.27.372.5:
        cmp.38.27.372.5:
        cmp r15b, 0
        jne if.38.24.372.5.end
        if.38.27.372.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.372.5.end:
    func.assert.372.5.end:
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.377.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.377.12.end:
    func.assert.377.5:
        if.38.27.377.5:
        cmp.38.27.377.5:
        cmp r15b, 0
        jne if.38.24.377.5.end
        if.38.27.377.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.377.5.end:
    func.assert.377.5.end:
    mov rcx, 8
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 816]
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 752]
    shl rcx, 3
    rep movsb
    cmp.386.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.386.12.end:
    func.assert.386.5:
        if.38.27.386.5:
        cmp.38.27.386.5:
        cmp r15b, 0
        jne if.38.24.386.5.end
        if.38.27.386.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.386.5.end:
    func.assert.386.5.end:
    cmp.387.12:
        mov rcx, 8
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 752]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 816]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.387.12.end:
    func.assert.387.5:
        if.38.27.387.5:
        cmp.38.27.387.5:
        cmp r15b, 0
        jne if.38.24.387.5.end
        if.38.27.387.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.387.5.end:
    func.assert.387.5.end:
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    cmp.394.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.394.12.end:
    func.assert.394.5:
        if.38.27.394.5:
        cmp.38.27.394.5:
        cmp r15b, 0
        jne if.38.24.394.5.end
        if.38.27.394.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.394.5.end:
    func.assert.394.5.end:
    cmp.395.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.395.12.end:
    func.assert.395.5:
        if.38.27.395.5:
        cmp.38.27.395.5:
        cmp r15b, 0
        jne if.38.24.395.5.end
        if.38.27.395.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.395.5.end:
    func.assert.395.5.end:
    cmp.396.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.396.12.end:
    func.assert.396.5:
        if.38.27.396.5:
        cmp.38.27.396.5:
        cmp r15b, 0
        jne if.38.24.396.5.end
        if.38.27.396.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.396.5.end:
    func.assert.396.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print.400.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.400.5.end:
    loop.401.5:
        add qword [rbp + 1280], 1
        lea r15, [rbp + 1416]
        lea r14, [vars]
        cmp r15, r14
        jb baz_frame_overflow
        mov r14, strict qword vars.end
        cmp r15, r14
        ja baz_frame_overflow
        sub r14, r15
        mov r15, size.func.print_num
        cmp r15, r14
        ja baz_frame_overflow
        lea r15, [rbp + 1280]
        mov qword [rbp + 1416], r15
        lea rbx, [rbp + 1416]
        call func.print_num
        func.print.404.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.404.9.end:
        func.print.405.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.405.9.end:
        func.str.input.406.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
            sub byte [rbp + 1288], 1
        func.str.input.406.12.end:
        if.408.12:
        cmp.408.12:
        cmp byte [rbp + 1288], 0
        jle loop.401.5.end
        if.408.12.code:
        if.410.19:
        cmp.410.19:
        cmp byte [rbp + 1288], 4
        jg if.408.9.else
        if.410.19.code:
            func.print.411.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.411.13.end:
            jmp loop.401.5
        if.408.9.else:
            func.greet.414.13:
                func.print.99.5.414.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.99.5.414.13.end:
                func.str.print.100.10.414.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1288]
                    test rdx, rdx
                    js baz_bounds_panic
                    cmp rdx, 127
                    jg baz_bounds_panic
                    lea rsi, [rbp + 1289]
                    mov rax, 1
                    syscall
                func.str.print.100.10.414.13.end:
                func.print.101.5.414.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.101.5.414.13.end:
                func.print.102.5.414.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.102.5.414.13.end:
                add qword [rbp + 368], 1
            func.greet.414.13.end:
        if.408.9.end:
    jmp loop.401.5
    loop.401.5.end:
    func.print.418.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
        mov rax, 1
        syscall
    func.print.418.5.end:
    lea r15, [rbp + 1416]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.print_num
    cmp r15, r14
    ja baz_frame_overflow
    lea r15, [rbp + 368]
    mov qword [rbp + 1416], r15
    lea rbx, [rbp + 1416]
    call func.print_num
    func.print.420.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.420.5.end:
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
    mov rdi, 1
    mov rdx, 3
    test rdx, rdx
    js baz_bounds_panic
    cmp rdx, 13
    jg baz_bounds_panic
    lea rsi, [rbp + 1416]
    mov rax, 1
    syscall
    mov rdi, 1
    mov rdx, 1
    mov r15, 13
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    test rdx, rdx
    js baz_bounds_panic
    mov r14, rdx
    add r14, r15
    cmp r14, 13
    jg baz_bounds_panic
    lea rsi, [rbp + 1416]
    add rsi, r15
    mov rax, 1
    syscall
    mov rdi, 0
    mov rax, 60
    syscall
func.print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
    mov byte [rbx + 40], 0
    if.142.8:
    cmp.142.8:
    cmp qword [rbx + 32], 0
    jge if.142.5.end
    if.142.8.code:
        mov byte [rbx + 40], 1
    if.142.5.end:
    if.145.8:
    cmp.145.8:
    cmp qword [rbx + 32], 0
    jle if.145.5.end
    if.145.8.code:
        neg qword [rbx + 32]
    if.145.5.end:
    mov qword [rbx + 48], 20
    loop.150.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
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
        if.154.12:
        cmp.154.12:
        cmp qword [rbx + 32], 0
        jne loop.150.5
        if.154.12.code:
        if.154.9.end:
    loop.150.5.end:
    if.157.8:
    cmp.157.8:
    cmp byte [rbx + 40], 0
    je if.157.5.end
    if.157.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.157.5.end:
    mov qword [rbx + 56], 0
    loop.163.5:
        mov r15, qword [rbx + 56]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        test r14, r14
        js baz_bounds_panic
        cmp r14, 20
        jge baz_bounds_panic
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 56], 1
        add qword [rbx + 48], 1
        if.167.12:
        cmp.167.12:
        cmp qword [rbx + 48], 20
        jne loop.163.5
        if.167.12.code:
        if.167.9.end:
    loop.163.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    test rdx, rdx
    js baz_bounds_panic
    cmp rdx, 20
    jg baz_bounds_panic
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.175.8:
    cmp.175.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.175.5.end
    if.175.8.code:
        ret
    if.175.5.end:
    mov r15, qword [rbx + 8]
    mov r14, qword [r15]
    mov qword [rbx + 16], r14
    sub qword [rbx + 16], 1
    lea r15, [rbx + 32]
    lea r14, [vars]
    cmp r15, r14
    jb baz_frame_overflow
    mov r14, strict qword vars.end
    cmp r15, r14
    ja baz_frame_overflow
    sub r14, r15
    mov r15, size.func.factorial
    cmp r15, r14
    ja baz_frame_overflow
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
    mov rax, 60
    mov rdi, 255
    syscall
section .data
align 16
dat:
db `hello world from baz\n`
db `enter name:\n`
db `that is not a name.\n`
db `hello `
db `.`
db `\n`
db `: `
times 1 db 0
dq 1
times 24 db 0
db 3
times 127 db 0
db 3
db `baz`
times 124 db 0
db `names greeted: `
times 1 db 0
dq 0
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 131072
vars.end:
