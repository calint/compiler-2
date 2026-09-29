default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 384], 0
    cmp.183.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.183.12.end:
    func.assert.183.5:
        if.38.27.183.5:
        cmp.38.27.183.5:
        cmp r15b, 0
        jne if.38.24.183.5.end
        if.38.27.183.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.183.5.end:
    func.assert.183.5.end:
    mov qword [rbp + 384], -1
    cmp.186.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.186.12.end:
    func.assert.186.5:
        if.38.27.186.5:
        cmp.38.27.186.5:
        cmp r15b, 0
        jne if.38.24.186.5.end
        if.38.27.186.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.186.5.end:
    func.assert.186.5.end:
        func.assert.192.9:
            if.38.27.192.9:
            cmp.38.27.192.9:
            if.38.24.192.9.end:
        func.assert.192.9.end:
    func.assert.195.5:
        if.38.27.195.5:
        cmp.38.27.195.5:
        if.38.24.195.5.end:
    func.assert.195.5.end:
    cmp.197.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.197.12.end
    cmp.197.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.197.12.end:
    func.assert.197.5:
        if.38.27.197.5:
        cmp.38.27.197.5:
        cmp r15b, 0
        jne if.38.24.197.5.end
        if.38.27.197.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.197.5.end:
    func.assert.197.5.end:
    cmp.198.12:
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.198.12.end
    cmp.198.30:
    cmp byte [rbp + 227], 122
    sete r15b
    bool.198.12.end:
    func.assert.198.5:
        if.38.27.198.5:
        cmp.38.27.198.5:
        cmp r15b, 0
        jne if.38.24.198.5.end
        if.38.27.198.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.198.5.end:
    func.assert.198.5.end:
    mov qword [rbp + 392], 7
    cmp.201.12:
        mov r14, qword [rbp + 392]
        and r14, 3
    cmp r14, 3
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
        mov r14, qword [rbp + 392]
        or r14, 8
    cmp r14, 15
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
    cmp.203.12:
        mov r14, qword [rbp + 392]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.203.12.end:
    func.assert.203.5:
        if.38.27.203.5:
        cmp.38.27.203.5:
        cmp r15b, 0
        jne if.38.24.203.5.end
        if.38.27.203.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.203.5.end:
    func.assert.203.5.end:
    cmp.204.12:
        mov r14, qword [rbp + 392]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.204.12.end:
    func.assert.204.5:
        if.38.27.204.5:
        cmp.38.27.204.5:
        cmp r15b, 0
        jne if.38.24.204.5.end
        if.38.27.204.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.204.5.end:
    func.assert.204.5.end:
    cmp.205.12:
        mov r14, qword [rbp + 392]
        neg r14
        sar r14, 1
    cmp r14, -4
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
    cmp.208.12:
        mov r14, qword [rbp + 392]
        mov r13, qword [rbp + 392]
        sal r13, 1
        add r14, r13
    cmp r14, 21
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
        add r14, qword [rbp + 392]
        sal r14, 1
    cmp r14, 28
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
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.212.12.end
    cmp.212.23:
    cmp.212.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.212.12.end
    cmp.212.34:
    cmp qword [rbp + 392], 0
    setl r15b
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
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.217.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.217.12.end:
    func.assert.217.5:
        if.38.27.217.5:
        cmp.38.27.217.5:
        cmp r15b, 0
        jne if.38.24.217.5.end
        if.38.27.217.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.217.5.end:
    func.assert.217.5.end:
    movsx r15, byte [rbp + 400]
    mov qword [rbp + 408], r15
    cmp.221.12:
    cmp qword [rbp + 408], -56
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
    mov qword [rbp + 416], 65
    cmp.229.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.229.12.end:
    func.assert.229.5:
        if.38.27.229.5:
        cmp.38.27.229.5:
        cmp r15b, 0
        jne if.38.24.229.5.end
        if.38.27.229.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.229.5.end:
    func.assert.229.5.end:
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
    cmp.237.12:
    cmp dword [rbp + 428], 2
    sete r15b
    bool.237.12.end:
    func.assert.237.5:
        if.38.27.237.5:
        cmp.38.27.237.5:
        cmp r15b, 0
        jne if.38.24.237.5.end
        if.38.27.237.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.237.5.end:
    func.assert.237.5.end:
    cmp.238.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.238.12.end:
    func.assert.238.5:
        if.38.27.238.5:
        cmp.38.27.238.5:
        cmp r15b, 0
        jne if.38.24.238.5.end
        if.38.27.238.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.238.5.end:
    func.assert.238.5.end:
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
    cmp.241.12:
    cmp dword [rbp + 424], 2
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
    cmp.246.14:
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
    bool.246.14.end:
    cmp.249.12:
    mov r15b, byte [rbp + 480]
    bool.249.12.end:
    func.assert.249.5:
        if.38.27.249.5:
        cmp.38.27.249.5:
        cmp r15b, 0
        jne if.38.24.249.5.end
        if.38.27.249.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.249.5.end:
    func.assert.249.5.end:
    mov dword [rbp + 456], -1
    cmp.252.12:
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
    bool.252.12.end:
    func.assert.252.5:
        if.38.27.252.5:
        cmp.38.27.252.5:
        cmp r15b, 0
        jne if.38.24.252.5.end
        if.38.27.252.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.252.5.end:
    func.assert.252.5.end:
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
    cmp.255.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.255.12.end:
    func.assert.255.5:
        if.38.27.255.5:
        cmp.38.27.255.5:
        cmp r15b, 0
        jne if.38.24.255.5.end
        if.38.27.255.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.255.5.end:
    func.assert.255.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.263.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv.263.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.265.12:
    mov r14, qword [rbp + 440]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
    bool.265.12.end:
    func.assert.265.5:
        if.38.27.265.5:
        cmp.38.27.265.5:
        cmp r15b, 0
        jne if.38.24.265.5.end
        if.38.27.265.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.265.5.end:
    func.assert.265.5.end:
    func.faz.267.5:
        mov dword [rbp + 428], 254
    func.faz.267.5.end:
    cmp.268.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.268.12.end:
    func.assert.268.5:
        if.38.27.268.5:
        cmp.38.27.268.5:
        cmp r15b, 0
        jne if.38.24.268.5.end
        if.38.27.268.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.268.5.end:
    func.assert.268.5.end:
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov qword [rbp + 528], 0
    foo.271.5:
        mov r14, qword [rbp + 528]
        add qword [r15], r14
        add qword [r15], 2
        foo.271.5.continue:
            add r15, 8
            inc qword [rbp + 528]
            cmp qword [rbp + 528], 2
            jne foo.271.5
    foo.271.5.end:
    cmp.274.12:
    cmp qword [rbp + 504], 5
    sete r15b
    bool.274.12.end:
    func.assert.274.5:
        if.38.27.274.5:
        cmp.38.27.274.5:
        cmp r15b, 0
        jne if.38.24.274.5.end
        if.38.27.274.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.274.5.end:
    func.assert.274.5.end:
    cmp.275.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.275.12.end:
    func.assert.275.5:
        if.38.27.275.5:
        cmp.38.27.275.5:
        cmp r15b, 0
        jne if.38.24.275.5.end
        if.38.27.275.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.275.5.end:
    func.assert.275.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz.283.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz.283.7.end:
    cmp.286.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.286.12.end:
    func.assert.286.5:
        if.38.27.286.5:
        cmp.38.27.286.5:
        cmp r15b, 0
        jne if.38.24.286.5.end
        if.38.27.286.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.286.5.end:
    func.assert.286.5.end:
    cmp.287.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.287.12.end:
    func.assert.287.5:
        if.38.27.287.5:
        cmp.38.27.287.5:
        cmp r15b, 0
        jne if.38.24.287.5.end
        if.38.27.287.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.287.5.end:
    func.assert.287.5.end:
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
    cmp.292.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    bool.292.12.end:
    func.assert.292.5:
        if.38.27.292.5:
        cmp.38.27.292.5:
        cmp r15b, 0
        jne if.38.24.292.5.end
        if.38.27.292.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.292.5.end:
    func.assert.292.5.end:
    mov qword [rbp + 536], 3
    cmp.297.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    bool.297.12.end:
    func.assert.297.5:
        if.38.27.297.5:
        cmp.38.27.297.5:
        cmp r15b, 0
        jne if.38.24.297.5.end
        if.38.27.297.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.297.5.end:
    func.assert.297.5.end:
    mov qword [rbp + 552], 0
    func.bar.300.5:
        if.59.8.300.5:
        cmp.59.8.300.5:
        cmp qword [rbp + 552], 0
        je func.bar.300.5.end
        if.59.8.300.5.code:
        if.59.5.300.5.end:
        mov qword [rbp + 552], 255
    func.bar.300.5.end:
    cmp.301.12:
    cmp qword [rbp + 552], 0
    sete r15b
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
    mov qword [rbp + 552], 1
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
    cmp qword [rbp + 552], 255
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
    mov qword [rbp + 560], 1
    func.baz.308.13:
        mov r15, qword [rbp + 560]
        mov qword [rbp + 568], r15
        sal qword [rbp + 568], 1
    func.baz.308.13.end:
    cmp.309.12:
    cmp qword [rbp + 568], 2
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
    func.baz.311.9:
        mov qword [rbp + 568], 2
    func.baz.311.9.end:
    cmp.312.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.312.12.end:
    func.assert.312.5:
        if.38.27.312.5:
        cmp.38.27.312.5:
        cmp r15b, 0
        jne if.38.24.312.5.end
        if.38.27.312.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.312.5.end:
    func.assert.312.5.end:
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
    cmp.316.12:
    cmp qword [rbp + 584], 120
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
    func.baz.318.20:
        mov qword [rbp + 592], 6
    func.baz.318.20.end:
    mov qword [rbp + 600], 0
    cmp.319.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.319.12.end:
    func.assert.319.5:
        if.38.27.319.5:
        cmp.38.27.319.5:
        cmp r15b, 0
        jne if.38.24.319.5.end
        if.38.27.319.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.319.5.end:
    func.assert.319.5.end:
    func.point.at.321.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at.321.14.end:
    cmp.325.12:
    cmp qword [rbp + 608], -1
    sete r15b
    bool.325.12.end:
    func.assert.325.5:
        if.38.27.325.5:
        cmp.38.27.325.5:
        cmp r15b, 0
        jne if.38.24.325.5.end
        if.38.27.325.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.325.5.end:
    func.assert.325.5.end:
    cmp.326.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.326.12.end:
    func.assert.326.5:
        if.38.27.326.5:
        cmp.38.27.326.5:
        cmp r15b, 0
        jne if.38.24.326.5.end
        if.38.27.326.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.326.5.end:
    func.assert.326.5.end:
    func.point.x.328.8:
        mov qword [rbp + 608], 2
    func.point.x.328.8.end:
    cmp.329.12:
    cmp qword [rbp + 608], 2
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
        func.point.sum.330.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
        func.point.sum.330.15.end:
    cmp r14, 0
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
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.336.12:
    cmp qword [rbp + 640], 10
    sete r15b
    bool.336.12.end:
    func.assert.336.5:
        if.38.27.336.5:
        cmp.38.27.336.5:
        cmp r15b, 0
        jne if.38.24.336.5.end
        if.38.27.336.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.336.5.end:
    func.assert.336.5.end:
    cmp.337.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.337.12.end:
    func.assert.337.5:
        if.38.27.337.5:
        cmp.38.27.337.5:
        cmp r15b, 0
        jne if.38.24.337.5.end
        if.38.27.337.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.337.5.end:
    func.assert.337.5.end:
    cmp.338.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.338.12.end:
    func.assert.338.5:
        if.38.27.338.5:
        cmp.38.27.338.5:
        cmp r15b, 0
        jne if.38.24.338.5.end
        if.38.27.338.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.338.5.end:
    func.assert.338.5.end:
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
    cmp.342.12:
    cmp qword [rbp + 640], -1
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
    cmp.343.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.343.12.end:
    func.assert.343.5:
        if.38.27.343.5:
        cmp.38.27.343.5:
        cmp r15b, 0
        jne if.38.24.343.5.end
        if.38.27.343.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.343.5.end:
    func.assert.343.5.end:
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.346.12:
    cmp qword [rbp + 680], -1
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
    cmp qword [rbp + 688], -2
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
    cmp.348.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.348.12.end:
    func.assert.348.5:
        if.38.27.348.5:
        cmp.38.27.348.5:
        cmp r15b, 0
        jne if.38.24.348.5.end
        if.38.27.348.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.348.5.end:
    func.assert.348.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.351.12:
    cmp qword [rbp + 680], 1
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
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.353.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.353.12.end:
    func.assert.353.5:
        if.38.27.353.5:
        cmp.38.27.353.5:
        cmp r15b, 0
        jne if.38.24.353.5.end
        if.38.27.353.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.353.5.end:
    func.assert.353.5.end:
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.363.12:
    cmp qword [rbp + 712], 73
    sete r15b
    bool.363.12.end:
    func.assert.363.5:
        if.38.27.363.5:
        cmp.38.27.363.5:
        cmp r15b, 0
        jne if.38.24.363.5.end
        if.38.27.363.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.363.5.end:
    func.assert.363.5.end:
    func.object.at.364.13:
        func.point.at.115.16.364.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at.115.16.364.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at.364.13.end:
    cmp.365.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.365.12.end:
    func.assert.365.5:
        if.38.27.365.5:
        cmp.38.27.365.5:
        cmp r15b, 0
        jne if.38.24.365.5.end
        if.38.27.365.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.365.5.end:
    func.assert.365.5.end:
    func.point.fooz.367.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz.367.15.end:
    cmp.368.12:
        func.point.sum.368.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
        func.point.sum.368.22.end:
    cmp r14, 13
    sete r15b
    bool.368.12.end:
    func.assert.368.5:
        if.38.27.368.5:
        cmp.38.27.368.5:
        cmp r15b, 0
        jne if.38.24.368.5.end
        if.38.27.368.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.368.5.end:
    func.assert.368.5.end:
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.373.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.373.12.end:
    func.assert.373.5:
        if.38.27.373.5:
        cmp.38.27.373.5:
        cmp r15b, 0
        jne if.38.24.373.5.end
        if.38.27.373.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.373.5.end:
    func.assert.373.5.end:
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
    cmp.382.12:
    cmp qword [rbp + 760], 65518
    sete r15b
    bool.382.12.end:
    func.assert.382.5:
        if.38.27.382.5:
        cmp.38.27.382.5:
        cmp r15b, 0
        jne if.38.24.382.5.end
        if.38.27.382.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.382.5.end:
    func.assert.382.5.end:
    cmp.383.12:
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
    bool.383.12.end:
    func.assert.383.5:
        if.38.27.383.5:
        cmp.38.27.383.5:
        cmp r15b, 0
        jne if.38.24.383.5.end
        if.38.27.383.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.383.5.end:
    func.assert.383.5.end:
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    cmp.390.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.390.12.end:
    func.assert.390.5:
        if.38.27.390.5:
        cmp.38.27.390.5:
        cmp r15b, 0
        jne if.38.24.390.5.end
        if.38.27.390.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.390.5.end:
    func.assert.390.5.end:
    cmp.391.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.391.12.end:
    func.assert.391.5:
        if.38.27.391.5:
        cmp.38.27.391.5:
        cmp r15b, 0
        jne if.38.24.391.5.end
        if.38.27.391.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.391.5.end:
    func.assert.391.5.end:
    cmp.392.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.392.12.end:
    func.assert.392.5:
        if.38.27.392.5:
        cmp.38.27.392.5:
        cmp r15b, 0
        jne if.38.24.392.5.end
        if.38.27.392.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.392.5.end:
    func.assert.392.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print.396.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.396.5.end:
    loop.397.5:
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
        func.print.400.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.400.9.end:
        func.print.401.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.401.9.end:
        func.str.input.402.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
            sub byte [rbp + 1288], 1
        func.str.input.402.12.end:
        if.404.12:
        cmp.404.12:
        cmp byte [rbp + 1288], 0
        jle loop.397.5.end
        if.404.12.code:
        if.406.19:
        cmp.406.19:
        cmp byte [rbp + 1288], 4
        jg if.404.9.else
        if.406.19.code:
            func.print.407.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.407.13.end:
            jmp loop.397.5
        if.404.9.else:
            func.greet.410.13:
                func.print.95.5.410.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.95.5.410.13.end:
                func.str.print.96.10.410.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1288]
                    test rdx, rdx
                    js baz_bounds_panic
                    cmp rdx, 127
                    jg baz_bounds_panic
                    lea rsi, [rbp + 1289]
                    mov rax, 1
                    syscall
                func.str.print.96.10.410.13.end:
                func.print.97.5.410.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.97.5.410.13.end:
                func.print.98.5.410.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.98.5.410.13.end:
                add qword [rbp + 368], 1
            func.greet.410.13.end:
        if.404.9.end:
    jmp loop.397.5
    loop.397.5.end:
    func.print.414.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
        mov rax, 1
        syscall
    func.print.414.5.end:
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
    func.print.416.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.416.5.end:
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
    if.138.8:
    cmp.138.8:
    cmp qword [rbx + 32], 0
    jge if.138.5.end
    if.138.8.code:
        mov byte [rbx + 40], 1
    if.138.5.end:
    if.141.8:
    cmp.141.8:
    cmp qword [rbx + 32], 0
    jle if.141.5.end
    if.141.8.code:
        neg qword [rbx + 32]
    if.141.5.end:
    mov qword [rbx + 48], 20
    loop.146.5:
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
        if.150.12:
        cmp.150.12:
        cmp qword [rbx + 32], 0
        jne loop.146.5
        if.150.12.code:
        if.150.9.end:
    loop.146.5.end:
    if.153.8:
    cmp.153.8:
    cmp byte [rbx + 40], 0
    je if.153.5.end
    if.153.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.153.5.end:
    mov qword [rbx + 56], 0
    loop.159.5:
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
        if.163.12:
        cmp.163.12:
        cmp qword [rbx + 48], 20
        jne loop.159.5
        if.163.12.code:
        if.163.9.end:
    loop.159.5.end:
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
    if.171.8:
    cmp.171.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.171.5.end
    if.171.8.code:
        ret
    if.171.5.end:
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
resb 65536
vars.end:
