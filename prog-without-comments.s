default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 384], 0
    cmp.188.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.188.12.end:
    func.assert.188.5:
        if.38.27.188.5:
        cmp.38.27.188.5:
        cmp r15b, 0
        jne if.38.24.188.5.end
        if.38.27.188.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.188.5.end:
    func.assert.188.5.end:
    mov qword [rbp + 384], -1
    cmp.191.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.191.12.end:
    func.assert.191.5:
        if.38.27.191.5:
        cmp.38.27.191.5:
        cmp r15b, 0
        jne if.38.24.191.5.end
        if.38.27.191.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.191.5.end:
    func.assert.191.5.end:
        func.assert.197.9:
            if.38.27.197.9:
            cmp.38.27.197.9:
            if.38.24.197.9.end:
        func.assert.197.9.end:
    func.assert.200.5:
        if.38.27.200.5:
        cmp.38.27.200.5:
        if.38.24.200.5.end:
    func.assert.200.5.end:
    cmp.202.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.202.12.end
    cmp.202.29:
    cmp qword [rbp + 88], 0
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
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.203.12.end
    cmp.203.30:
    cmp byte [rbp + 227], 122
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
    mov qword [rbp + 392], 7
    cmp.206.12:
        mov r14, qword [rbp + 392]
        and r14, 3
    cmp r14, 3
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
        or r14, 8
    cmp r14, 15
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
        xor r14, 1
    cmp r14, 6
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
        sal r14, 2
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
    cmp.210.12:
        mov r14, qword [rbp + 392]
        neg r14
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.210.12.end:
    func.assert.210.5:
        if.38.27.210.5:
        cmp.38.27.210.5:
        cmp r15b, 0
        jne if.38.24.210.5.end
        if.38.27.210.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.210.5.end:
    func.assert.210.5.end:
    cmp.213.12:
        mov r14, qword [rbp + 392]
        mov r13, qword [rbp + 392]
        sal r13, 1
        add r14, r13
    cmp r14, 21
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
    cmp.214.12:
        mov r14, qword [rbp + 392]
        add r14, qword [rbp + 392]
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.214.12.end:
    func.assert.214.5:
        if.38.27.214.5:
        cmp.38.27.214.5:
        cmp r15b, 0
        jne if.38.24.214.5.end
        if.38.27.214.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.214.5.end:
    func.assert.214.5.end:
    cmp.217.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.217.12.end
    cmp.217.23:
    cmp.217.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.217.12.end
    cmp.217.34:
    cmp qword [rbp + 392], 0
    setl r15b
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
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.222.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.222.12.end:
    func.assert.222.5:
        if.38.27.222.5:
        cmp.38.27.222.5:
        cmp r15b, 0
        jne if.38.24.222.5.end
        if.38.27.222.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.222.5.end:
    func.assert.222.5.end:
    movsx r15, byte [rbp + 400]
    mov qword [rbp + 408], r15
    cmp.226.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.226.12.end:
    func.assert.226.5:
        if.38.27.226.5:
        cmp.38.27.226.5:
        cmp r15b, 0
        jne if.38.24.226.5.end
        if.38.27.226.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.226.5.end:
    func.assert.226.5.end:
    mov qword [rbp + 416], 65
    cmp.234.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.234.12.end:
    func.assert.234.5:
        if.38.27.234.5:
        cmp.38.27.234.5:
        cmp r15b, 0
        jne if.38.24.234.5.end
        if.38.27.234.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.234.5.end:
    func.assert.234.5.end:
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
    mov qword [rbp + 440], 1
    mov r15, qword [rbp + 440]
    mov r14, 240
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 424], 2
    mov r15, qword [rbp + 440]
    add r15, 1
    mov r14, 241
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 440]
    mov r13, 241
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
    cmp.242.12:
    cmp dword [rbp + 428], 2
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
    cmp.243.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.243.12.end:
    func.assert.243.5:
        if.38.27.243.5:
        cmp.38.27.243.5:
        cmp r15b, 0
        jne if.38.24.243.5.end
        if.38.27.243.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.243.5.end:
    func.assert.243.5.end:
    mov r15, 2
    mov r14, 2
    mov r13, 245
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    mov r12, r15
    add r12, r14
    cmp r12, 4
    cmovg rbp, r13
    jg baz_bounds_panic
    mov r13, 245
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
    mov rax, qword [rbp + r14 * 4 + 424]
    mov qword [rbp + 424], rax
    cmp.246.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.246.12.end:
    func.assert.246.5:
        if.38.27.246.5:
        cmp.38.27.246.5:
        cmp r15b, 0
        jne if.38.24.246.5.end
        if.38.27.246.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.246.5.end:
    func.assert.246.5.end:
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
    mov r15, 4
    mov r14, 250
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    mov r14, 250
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    mov rax, qword [rbp + 424]
    mov qword [rbp + 448], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 456], rax
    cmp.251.14:
        mov rcx, 3
        mov r15, 1
        mov r14, 251
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 424]
        mov r15, 1
        mov r14, 251
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 480]
    bool.251.14.end:
    cmp.254.12:
    mov r15b, byte [rbp + 480]
    bool.254.12.end:
    func.assert.254.5:
        if.38.27.254.5:
        cmp.38.27.254.5:
        cmp r15b, 0
        jne if.38.24.254.5.end
        if.38.27.254.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.254.5.end:
    func.assert.254.5.end:
    mov dword [rbp + 456], -1
    cmp.257.12:
        mov rcx, 4
        mov r14, 257
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 424]
        mov r14, 257
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.257.12.end:
    func.assert.257.5:
        if.38.27.257.5:
        cmp.38.27.257.5:
        cmp r15b, 0
        jne if.38.24.257.5.end
        if.38.27.257.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.257.5.end:
    func.assert.257.5.end:
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
    cmp.260.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.260.12.end:
    func.assert.260.5:
        if.38.27.260.5:
        cmp.38.27.260.5:
        cmp r15b, 0
        jne if.38.24.260.5.end
        if.38.27.260.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.260.5.end:
    func.assert.260.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    sub r15, 1
    mov r14, 268
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    func.inv.268.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv.268.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    mov r14, 269
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.270.12:
    mov r14, qword [rbp + 440]
    mov r13, 270
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
    bool.270.12.end:
    func.assert.270.5:
        if.38.27.270.5:
        cmp.38.27.270.5:
        cmp r15b, 0
        jne if.38.24.270.5.end
        if.38.27.270.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.270.5.end:
    func.assert.270.5.end:
    func.faz.272.5:
        mov dword [rbp + 428], 254
    func.faz.272.5.end:
    cmp.273.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.273.12.end:
    func.assert.273.5:
        if.38.27.273.5:
        cmp.38.27.273.5:
        cmp r15b, 0
        jne if.38.24.273.5.end
        if.38.27.273.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.273.5.end:
    func.assert.273.5.end:
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov r14, 0
    foo.276.5:
        add qword [r15], r14
        add qword [r15], 2
        foo.276.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.276.5
    foo.276.5.end:
    cmp.279.12:
    cmp qword [rbp + 504], 5
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
    cmp.280.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.280.12.end:
    func.assert.280.5:
        if.38.27.280.5:
        cmp.38.27.280.5:
        cmp r15b, 0
        jne if.38.24.280.5.end
        if.38.27.280.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.280.5.end:
    func.assert.280.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz.288.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz.288.7.end:
    cmp.291.12:
    cmp qword [rbp + 520], 2
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
    cmp.292.12:
    cmp qword [rbp + 528], 11
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
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
    cmp.297.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
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
    mov qword [rbp + 536], 3
    cmp.302.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    bool.302.12.end:
    func.assert.302.5:
        if.38.27.302.5:
        cmp.38.27.302.5:
        cmp r15b, 0
        jne if.38.24.302.5.end
        if.38.27.302.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.302.5.end:
    func.assert.302.5.end:
    mov qword [rbp + 552], 0
    func.bar.305.5:
        if.60.8.305.5:
        cmp.60.8.305.5:
        cmp qword [rbp + 552], 0
        je func.bar.305.5.end
        if.60.8.305.5.code:
        if.60.5.305.5.end:
        mov qword [rbp + 552], 255
    func.bar.305.5.end:
    cmp.306.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.306.12.end:
    func.assert.306.5:
        if.38.27.306.5:
        cmp.38.27.306.5:
        cmp r15b, 0
        jne if.38.24.306.5.end
        if.38.27.306.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.306.5.end:
    func.assert.306.5.end:
    mov qword [rbp + 552], 1
    func.bar.309.5:
        if.60.8.309.5:
        cmp.60.8.309.5:
        cmp qword [rbp + 552], 0
        je func.bar.309.5.end
        if.60.8.309.5.code:
        if.60.5.309.5.end:
        mov qword [rbp + 552], 255
    func.bar.309.5.end:
    cmp.310.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.310.12.end:
    func.assert.310.5:
        if.38.27.310.5:
        cmp.38.27.310.5:
        cmp r15b, 0
        jne if.38.24.310.5.end
        if.38.27.310.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.310.5.end:
    func.assert.310.5.end:
    mov qword [rbp + 560], 1
    func.baz.313.13:
        mov r15, qword [rbp + 560]
        mov qword [rbp + 568], r15
        sal qword [rbp + 568], 1
    func.baz.313.13.end:
    cmp.314.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.314.12.end:
    func.assert.314.5:
        if.38.27.314.5:
        cmp.38.27.314.5:
        cmp r15b, 0
        jne if.38.24.314.5.end
        if.38.27.314.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.314.5.end:
    func.assert.314.5.end:
    func.baz.316.9:
        mov qword [rbp + 568], 2
    func.baz.316.9.end:
    cmp.317.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.317.12.end:
    func.assert.317.5:
        if.38.27.317.5:
        cmp.38.27.317.5:
        cmp r15b, 0
        jne if.38.24.317.5.end
        if.38.27.317.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.317.5.end:
    func.assert.317.5.end:
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
    cmp.321.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.321.12.end:
    func.assert.321.5:
        if.38.27.321.5:
        cmp.38.27.321.5:
        cmp r15b, 0
        jne if.38.24.321.5.end
        if.38.27.321.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.321.5.end:
    func.assert.321.5.end:
    func.baz.323.20:
        mov qword [rbp + 592], 6
    func.baz.323.20.end:
    mov qword [rbp + 600], 0
    cmp.324.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.324.12.end:
    func.assert.324.5:
        if.38.27.324.5:
        cmp.38.27.324.5:
        cmp r15b, 0
        jne if.38.24.324.5.end
        if.38.27.324.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.324.5.end:
    func.assert.324.5.end:
    func.point.at.326.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at.326.14.end:
    cmp.330.12:
    cmp qword [rbp + 608], -1
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
    cmp.331.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.331.12.end:
    func.assert.331.5:
        if.38.27.331.5:
        cmp.38.27.331.5:
        cmp r15b, 0
        jne if.38.24.331.5.end
        if.38.27.331.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.331.5.end:
    func.assert.331.5.end:
    func.point.x.333.8:
        mov qword [rbp + 608], 2
    func.point.x.333.8.end:
    cmp.334.12:
    cmp qword [rbp + 608], 2
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
    cmp.335.12:
        func.point.sum.335.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
        func.point.sum.335.15.end:
    cmp r14, 0
    sete r15b
    bool.335.12.end:
    func.assert.335.5:
        if.38.27.335.5:
        cmp.38.27.335.5:
        cmp r15b, 0
        jne if.38.24.335.5.end
        if.38.27.335.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.335.5.end:
    func.assert.335.5.end:
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.341.12:
    cmp qword [rbp + 640], 10
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
    cmp qword [rbp + 648], 2
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
    cmp dword [rbp + 656], 16711680
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
    cmp.347.12:
    cmp qword [rbp + 640], -1
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
    cmp qword [rbp + 648], -2
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
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.351.12:
    cmp qword [rbp + 680], -1
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
    cmp qword [rbp + 688], -2
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
    cmp.353.12:
    cmp dword [rbp + 696], 16711680
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
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.356.12:
    cmp qword [rbp + 680], 1
    sete r15b
    bool.356.12.end:
    func.assert.356.5:
        if.38.27.356.5:
        cmp.38.27.356.5:
        cmp r15b, 0
        jne if.38.24.356.5.end
        if.38.27.356.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.356.5.end:
    func.assert.356.5.end:
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.358.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.358.12.end:
    func.assert.358.5:
        if.38.27.358.5:
        cmp.38.27.358.5:
        cmp r15b, 0
        jne if.38.24.358.5.end
        if.38.27.358.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.358.5.end:
    func.assert.358.5.end:
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.368.12:
    cmp qword [rbp + 712], 73
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
    mov dword [rbp + 748], 0
    func.object.at.369.13:
        func.point.at.120.16.369.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at.120.16.369.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at.369.13.end:
    cmp.370.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.370.12.end:
    func.assert.370.5:
        if.38.27.370.5:
        cmp.38.27.370.5:
        cmp r15b, 0
        jne if.38.24.370.5.end
        if.38.27.370.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.370.5.end:
    func.assert.370.5.end:
    func.point.fooz.372.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz.372.15.end:
    cmp.373.12:
        func.point.sum.373.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
        func.point.sum.373.22.end:
    cmp r14, 13
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
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.378.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.378.12.end:
    func.assert.378.5:
        if.38.27.378.5:
        cmp.38.27.378.5:
        cmp r15b, 0
        jne if.38.24.378.5.end
        if.38.27.378.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.378.5.end:
    func.assert.378.5.end:
    mov rcx, 8
    mov r15, 381
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 816]
    mov r15, 382
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 752]
    shl rcx, 3
    rep movsb
    cmp.387.12:
    cmp qword [rbp + 760], 65518
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
    cmp.388.12:
        mov rcx, 8
        mov r14, 389
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 752]
        mov r14, 390
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + 816]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.388.12.end:
    func.assert.388.5:
        if.38.27.388.5:
        cmp.38.27.388.5:
        cmp r15b, 0
        jne if.38.24.388.5.end
        if.38.27.388.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.388.5.end:
    func.assert.388.5.end:
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    cmp.395.12:
        mov r14, 2
    cmp r14, 2
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
    cmp qword [rbp + 1264], -1
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
    cmp.397.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.397.12.end:
    func.assert.397.5:
        if.38.27.397.5:
        cmp.38.27.397.5:
        cmp r15b, 0
        jne if.38.24.397.5.end
        if.38.27.397.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.397.5.end:
    func.assert.397.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print.401.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.401.5.end:
    loop.402.5:
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
        func.print.405.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.405.9.end:
        func.print.406.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.406.9.end:
        func.str.input.407.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
            sub byte [rbp + 1288], 1
        func.str.input.407.12.end:
        if.409.12:
        cmp.409.12:
        cmp byte [rbp + 1288], 0
        jle loop.402.5.end
        if.409.12.code:
        if.411.19:
        cmp.411.19:
        cmp byte [rbp + 1288], 4
        jg if.409.9.else
        if.411.19.code:
            func.print.412.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.412.13.end:
            jmp loop.402.5
        if.409.9.else:
            func.greet.415.13:
                func.print.100.5.415.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.100.5.415.13.end:
                func.str.print.101.10.415.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1288]
                    mov r15, 96
                    test rdx, rdx
                    cmovs rbp, r15
                    js baz_bounds_panic
                    cmp rdx, 127
                    cmovg rbp, r15
                    jg baz_bounds_panic
                    lea rsi, [rbp + 1289]
                    mov rax, 1
                    syscall
                func.str.print.101.10.415.13.end:
                func.print.102.5.415.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.102.5.415.13.end:
                func.print.103.5.415.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.103.5.415.13.end:
                add qword [rbp + 368], 1
            func.greet.415.13.end:
        if.409.9.end:
    jmp loop.402.5
    loop.402.5.end:
    func.print.419.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
        mov rax, 1
        syscall
    func.print.419.5.end:
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
    func.print.421.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.421.5.end:
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
    mov rdi, 1
    mov rdx, 3
    mov r15, 424
    test rdx, rdx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rdx, 13
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 1416]
    mov rax, 1
    syscall
    mov rdi, 1
    mov rdx, 1
    mov r15, 13
    sub r15, 1
    mov r14, 425
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    test rdx, rdx
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rdx
    add r13, r15
    cmp r13, 13
    cmovg rbp, r14
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
    if.143.8:
    cmp.143.8:
    cmp qword [rbx + 32], 0
    jge if.143.5.end
    if.143.8.code:
        mov byte [rbx + 40], 1
    if.143.5.end:
    if.146.8:
    cmp.146.8:
    cmp qword [rbx + 32], 0
    jle if.146.5.end
    if.146.8.code:
        neg qword [rbx + 32]
    if.146.5.end:
    mov qword [rbx + 48], 20
    loop.151.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 153
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
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
        if.155.12:
        cmp.155.12:
        cmp qword [rbx + 32], 0
        jne loop.151.5
        if.155.12.code:
        if.155.9.end:
    loop.151.5.end:
    if.158.8:
    cmp.158.8:
    cmp byte [rbx + 40], 0
    je if.158.5.end
    if.158.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 160
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.158.5.end:
    mov qword [rbx + 56], 0
    loop.164.5:
        mov r15, qword [rbx + 56]
        mov r14, 165
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        mov r13, 165
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 56], 1
        add qword [rbx + 48], 1
        if.168.12:
        cmp.168.12:
        cmp qword [rbx + 48], 20
        jne loop.164.5
        if.168.12.code:
        if.168.9.end:
    loop.164.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    mov r15, 171
    test rdx, rdx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rdx, 20
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.176.8:
    cmp.176.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.176.5.end
    if.176.8.code:
        ret
    if.176.5.end:
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
