default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 384], 0
    cmp.189.12:
    cmp qword [rbp + 384], 0
    sete r15b
    bool.189.12.end:
    func.assert.189.5:
        if.38.27.189.5:
        cmp.38.27.189.5:
        cmp r15b, 0
        jne if.38.24.189.5.end
        if.38.27.189.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.189.5.end:
    func.assert.189.5.end:
    mov qword [rbp + 384], -1
    cmp.192.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.192.12.end:
    func.assert.192.5:
        if.38.27.192.5:
        cmp.38.27.192.5:
        cmp r15b, 0
        jne if.38.24.192.5.end
        if.38.27.192.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.192.5.end:
    func.assert.192.5.end:
        func.assert.198.9:
            if.38.27.198.9:
            cmp.38.27.198.9:
            if.38.24.198.9.end:
        func.assert.198.9.end:
    func.assert.201.5:
        if.38.27.201.5:
        cmp.38.27.201.5:
        if.38.24.201.5.end:
    func.assert.201.5.end:
    cmp.203.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.203.12.end
    cmp.203.29:
    cmp qword [rbp + 88], 0
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
    cmp byte [rbp + 224], 3
    sete r15b
    jne bool.204.12.end
    cmp.204.30:
    cmp byte [rbp + 227], 122
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
    mov qword [rbp + 392], 7
    cmp.207.12:
        mov r14, qword [rbp + 392]
        and r14, 3
    cmp r14, 3
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
        or r14, 8
    cmp r14, 15
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
        xor r14, 1
    cmp r14, 6
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
        sal r14, 2
    cmp r14, 28
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
    cmp.211.12:
        mov r14, qword [rbp + 392]
        neg r14
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.211.12.end:
    func.assert.211.5:
        if.38.27.211.5:
        cmp.38.27.211.5:
        cmp r15b, 0
        jne if.38.24.211.5.end
        if.38.27.211.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.211.5.end:
    func.assert.211.5.end:
    cmp.214.12:
        mov r14, qword [rbp + 392]
        mov r13, qword [rbp + 392]
        sal r13, 1
        add r14, r13
    cmp r14, 21
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
    cmp.215.12:
        mov r14, qword [rbp + 392]
        add r14, qword [rbp + 392]
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.215.12.end:
    func.assert.215.5:
        if.38.27.215.5:
        cmp.38.27.215.5:
        cmp r15b, 0
        jne if.38.24.215.5.end
        if.38.27.215.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.215.5.end:
    func.assert.215.5.end:
    cmp.218.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.218.12.end
    cmp.218.23:
    cmp.218.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.218.12.end
    cmp.218.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.218.12.end:
    func.assert.218.5:
        if.38.27.218.5:
        cmp.38.27.218.5:
        cmp r15b, 0
        jne if.38.24.218.5.end
        if.38.27.218.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.218.5.end:
    func.assert.218.5.end:
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.223.12:
    cmp byte [rbp + 400], -56
    sete r15b
    bool.223.12.end:
    func.assert.223.5:
        if.38.27.223.5:
        cmp.38.27.223.5:
        cmp r15b, 0
        jne if.38.24.223.5.end
        if.38.27.223.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.223.5.end:
    func.assert.223.5.end:
    movsx r15, byte [rbp + 400]
    mov qword [rbp + 408], r15
    cmp.227.12:
    cmp qword [rbp + 408], -56
    sete r15b
    bool.227.12.end:
    func.assert.227.5:
        if.38.27.227.5:
        cmp.38.27.227.5:
        cmp r15b, 0
        jne if.38.24.227.5.end
        if.38.27.227.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.227.5.end:
    func.assert.227.5.end:
    mov qword [rbp + 416], 65
    cmp.235.12:
    cmp qword [rbp + 416], 65
    sete r15b
    bool.235.12.end:
    func.assert.235.5:
        if.38.27.235.5:
        cmp.38.27.235.5:
        cmp r15b, 0
        jne if.38.24.235.5.end
        if.38.27.235.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.235.5.end:
    func.assert.235.5.end:
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
    mov qword [rbp + 440], 1
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_241
    mov dword [rbp + r15 * 4 + 424], 2
    mov r15, qword [rbp + 440]
    add r15, 1
    cmp r15, 4
    jae baz_bounds_line_242
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_242
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
    cmp.243.12:
    cmp dword [rbp + 428], 2
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
    cmp.244.12:
    cmp dword [rbp + 432], 2
    sete r15b
    bool.244.12.end:
    func.assert.244.5:
        if.38.27.244.5:
        cmp.38.27.244.5:
        cmp r15b, 0
        jne if.38.24.244.5.end
        if.38.27.244.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.244.5.end:
    func.assert.244.5.end:
    mov r15, 2
    mov r14, 2
    test r14, r14
    js baz_bounds_line_246
    test r15, r15
    js baz_bounds_line_246
    lea r13, [r15 + r14]
    cmp r13, 4
    jg baz_bounds_line_246
    cmp r15, 4
    ja baz_bounds_line_246
    mov rax, qword [rbp + r14 * 4 + 424]
    mov qword [rbp + 424], rax
    cmp.247.12:
    cmp dword [rbp + 424], 2
    sete r15b
    bool.247.12.end:
    func.assert.247.5:
        if.38.27.247.5:
        cmp.38.27.247.5:
        cmp r15b, 0
        jne if.38.24.247.5.end
        if.38.27.247.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.247.5.end:
    func.assert.247.5.end:
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
    mov r15, 4
    cmp r15, 4
    ja baz_bounds_line_251
    cmp r15, 8
    ja baz_bounds_line_251
    mov rax, qword [rbp + 424]
    mov qword [rbp + 448], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 456], rax
    cmp.252.14:
        mov rcx, 3
        mov r15, 1
        test r15, r15
        js baz_bounds_line_252
        test rcx, rcx
        js baz_bounds_line_252
        lea r14, [rcx + r15]
        cmp r14, 4
        jg baz_bounds_line_252
        lea rsi, [rbp + r15 * 4 + 424]
        mov r15, 1
        test r15, r15
        js baz_bounds_line_252
        lea r14, [rcx + r15]
        cmp r14, 8
        jg baz_bounds_line_252
        lea rdi, [rbp + r15 * 4 + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 480]
    bool.252.14.end:
    cmp.255.12:
    mov r15b, byte [rbp + 480]
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
    mov dword [rbp + 456], -1
    cmp.258.12:
        mov rcx, 4
        cmp rcx, 4
        ja baz_bounds_line_258
        lea rsi, [rbp + 424]
        cmp rcx, 8
        ja baz_bounds_line_258
        lea rdi, [rbp + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.258.12.end:
    func.assert.258.5:
        if.38.27.258.5:
        cmp.38.27.258.5:
        cmp r15b, 0
        jne if.38.24.258.5.end
        if.38.27.258.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.258.5.end:
    func.assert.258.5.end:
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
    cmp.261.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    cmp r15b, 0
    bool.261.12.end:
    func.assert.261.5:
        if.38.27.261.5:
        cmp.38.27.261.5:
        cmp r15b, 0
        jne if.38.24.261.5.end
        if.38.27.261.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.261.5.end:
    func.assert.261.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    sub r15, 1
    cmp r15, 4
    jae baz_bounds_line_269
    func.inv.269.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv.269.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_270
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.271.12:
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_271
    cmp dword [rbp + r14 * 4 + 424], 2
    sete r15b
    bool.271.12.end:
    func.assert.271.5:
        if.38.27.271.5:
        cmp.38.27.271.5:
        cmp r15b, 0
        jne if.38.24.271.5.end
        if.38.27.271.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.271.5.end:
    func.assert.271.5.end:
    func.faz.273.5:
        mov dword [rbp + 428], 254
    func.faz.273.5.end:
    cmp.274.12:
    cmp dword [rbp + 428], 254
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
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov r14, 0
    foo.277.5:
        add qword [r15], r14
        add qword [r15], 2
        foo.277.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.277.5
    foo.277.5.end:
    cmp.280.12:
    cmp qword [rbp + 504], 5
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
    cmp.281.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.281.12.end:
    func.assert.281.5:
        if.38.27.281.5:
        cmp.38.27.281.5:
        cmp r15b, 0
        jne if.38.24.281.5.end
        if.38.27.281.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.281.5.end:
    func.assert.281.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz.289.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz.289.7.end:
    cmp.292.12:
    cmp qword [rbp + 520], 2
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
    cmp.293.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.293.12.end:
    func.assert.293.5:
        if.38.27.293.5:
        cmp.38.27.293.5:
        cmp r15b, 0
        jne if.38.24.293.5.end
        if.38.27.293.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.293.5.end:
    func.assert.293.5.end:
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
    cmp.298.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    cmp r15b, 0
    bool.298.12.end:
    func.assert.298.5:
        if.38.27.298.5:
        cmp.38.27.298.5:
        cmp r15b, 0
        jne if.38.24.298.5.end
        if.38.27.298.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.298.5.end:
    func.assert.298.5.end:
    mov qword [rbp + 536], 3
    cmp.303.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    cmp r15b, 0
    bool.303.12.end:
    func.assert.303.5:
        if.38.27.303.5:
        cmp.38.27.303.5:
        cmp r15b, 0
        jne if.38.24.303.5.end
        if.38.27.303.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.303.5.end:
    func.assert.303.5.end:
    mov qword [rbp + 552], 0
    func.bar.306.5:
        if.60.8.306.5:
        cmp.60.8.306.5:
        cmp qword [rbp + 552], 0
        je func.bar.306.5.end
        if.60.8.306.5.code:
        if.60.5.306.5.end:
        mov qword [rbp + 552], 255
    func.bar.306.5.end:
    cmp.307.12:
    cmp qword [rbp + 552], 0
    sete r15b
    bool.307.12.end:
    func.assert.307.5:
        if.38.27.307.5:
        cmp.38.27.307.5:
        cmp r15b, 0
        jne if.38.24.307.5.end
        if.38.27.307.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.307.5.end:
    func.assert.307.5.end:
    mov qword [rbp + 552], 1
    func.bar.310.5:
        if.60.8.310.5:
        cmp.60.8.310.5:
        cmp qword [rbp + 552], 0
        je func.bar.310.5.end
        if.60.8.310.5.code:
        if.60.5.310.5.end:
        mov qword [rbp + 552], 255
    func.bar.310.5.end:
    cmp.311.12:
    cmp qword [rbp + 552], 255
    sete r15b
    bool.311.12.end:
    func.assert.311.5:
        if.38.27.311.5:
        cmp.38.27.311.5:
        cmp r15b, 0
        jne if.38.24.311.5.end
        if.38.27.311.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.311.5.end:
    func.assert.311.5.end:
    mov qword [rbp + 560], 1
    func.baz.314.13:
        mov r15, qword [rbp + 560]
        mov qword [rbp + 568], r15
        sal qword [rbp + 568], 1
    func.baz.314.13.end:
    cmp.315.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.315.12.end:
    func.assert.315.5:
        if.38.27.315.5:
        cmp.38.27.315.5:
        cmp r15b, 0
        jne if.38.24.315.5.end
        if.38.27.315.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.315.5.end:
    func.assert.315.5.end:
    func.baz.317.9:
        mov qword [rbp + 568], 2
    func.baz.317.9.end:
    cmp.318.12:
    cmp qword [rbp + 568], 2
    sete r15b
    bool.318.12.end:
    func.assert.318.5:
        if.38.27.318.5:
        cmp.38.27.318.5:
        cmp r15b, 0
        jne if.38.24.318.5.end
        if.38.27.318.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.318.5.end:
    func.assert.318.5.end:
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
    cmp.322.12:
    cmp qword [rbp + 584], 120
    sete r15b
    bool.322.12.end:
    func.assert.322.5:
        if.38.27.322.5:
        cmp.38.27.322.5:
        cmp r15b, 0
        jne if.38.24.322.5.end
        if.38.27.322.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.322.5.end:
    func.assert.322.5.end:
    func.baz.324.20:
        mov qword [rbp + 592], 6
    func.baz.324.20.end:
    mov qword [rbp + 600], 0
    cmp.325.12:
    cmp qword [rbp + 592], 6
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
    func.point.at.327.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at.327.14.end:
    cmp.331.12:
    cmp qword [rbp + 608], -1
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
    cmp.332.12:
    cmp qword [rbp + 616], -2
    sete r15b
    bool.332.12.end:
    func.assert.332.5:
        if.38.27.332.5:
        cmp.38.27.332.5:
        cmp r15b, 0
        jne if.38.24.332.5.end
        if.38.27.332.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.332.5.end:
    func.assert.332.5.end:
    func.point.x.334.8:
        mov qword [rbp + 608], 2
    func.point.x.334.8.end:
    cmp.335.12:
    cmp qword [rbp + 608], 2
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
    cmp.336.12:
        func.point.sum.336.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
        func.point.sum.336.15.end:
    cmp r14, 0
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
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.342.12:
    cmp qword [rbp + 640], 10
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
    cmp qword [rbp + 648], 2
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
    cmp.344.12:
    cmp dword [rbp + 656], 16711680
    sete r15b
    bool.344.12.end:
    func.assert.344.5:
        if.38.27.344.5:
        cmp.38.27.344.5:
        cmp r15b, 0
        jne if.38.24.344.5.end
        if.38.27.344.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.344.5.end:
    func.assert.344.5.end:
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
    cmp.348.12:
    cmp qword [rbp + 640], -1
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
    cmp.349.12:
    cmp qword [rbp + 648], -2
    sete r15b
    bool.349.12.end:
    func.assert.349.5:
        if.38.27.349.5:
        cmp.38.27.349.5:
        cmp r15b, 0
        jne if.38.24.349.5.end
        if.38.27.349.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.349.5.end:
    func.assert.349.5.end:
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.352.12:
    cmp qword [rbp + 680], -1
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
    cmp qword [rbp + 688], -2
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
    cmp.354.12:
    cmp dword [rbp + 696], 16711680
    sete r15b
    bool.354.12.end:
    func.assert.354.5:
        if.38.27.354.5:
        cmp.38.27.354.5:
        cmp r15b, 0
        jne if.38.24.354.5.end
        if.38.27.354.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.354.5.end:
    func.assert.354.5.end:
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.357.12:
    cmp qword [rbp + 680], 1
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
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.359.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.359.12.end:
    func.assert.359.5:
        if.38.27.359.5:
        cmp.38.27.359.5:
        cmp r15b, 0
        jne if.38.24.359.5.end
        if.38.27.359.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.359.5.end:
    func.assert.359.5.end:
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.369.12:
    cmp qword [rbp + 712], 73
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
    mov dword [rbp + 748], 0
    func.object.at.370.13:
        func.point.at.120.16.370.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at.120.16.370.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at.370.13.end:
    cmp.371.12:
    cmp qword [rbp + 736], 74
    sete r15b
    bool.371.12.end:
    func.assert.371.5:
        if.38.27.371.5:
        cmp.38.27.371.5:
        cmp r15b, 0
        jne if.38.24.371.5.end
        if.38.27.371.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.371.5.end:
    func.assert.371.5.end:
    func.point.fooz.373.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz.373.15.end:
    cmp.374.12:
        func.point.sum.374.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
        func.point.sum.374.22.end:
    cmp r14, 13
    sete r15b
    bool.374.12.end:
    func.assert.374.5:
        if.38.27.374.5:
        cmp.38.27.374.5:
        cmp r15b, 0
        jne if.38.24.374.5.end
        if.38.27.374.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.374.5.end:
    func.assert.374.5.end:
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.379.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.379.12.end:
    func.assert.379.5:
        if.38.27.379.5:
        cmp.38.27.379.5:
        cmp r15b, 0
        jne if.38.24.379.5.end
        if.38.27.379.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.379.5.end:
    func.assert.379.5.end:
    mov r15, 8
    cmp r15, 8
    ja baz_bounds_line_382
    cmp r15, 8
    ja baz_bounds_line_383
    lea rsi, [rbp + 816]
    lea rdi, [rbp + 752]
    mov rcx, 64
    rep movsb
    cmp.388.12:
    cmp qword [rbp + 760], 65518
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
    cmp.389.12:
        mov rcx, 8
        cmp rcx, 8
        ja baz_bounds_line_390
        lea rsi, [rbp + 752]
        cmp rcx, 8
        ja baz_bounds_line_391
        lea rdi, [rbp + 816]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.389.12.end:
    func.assert.389.5:
        if.38.27.389.5:
        cmp.38.27.389.5:
        cmp r15b, 0
        jne if.38.24.389.5.end
        if.38.27.389.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.389.5.end:
    func.assert.389.5.end:
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    func.assert.396.5:
        if.38.27.396.5:
        cmp.38.27.396.5:
        if.38.24.396.5.end:
    func.assert.396.5.end:
    cmp.397.12:
    cmp qword [rbp + 1264], -1
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
    cmp.398.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.398.12.end:
    func.assert.398.5:
        if.38.27.398.5:
        cmp.38.27.398.5:
        cmp r15b, 0
        jne if.38.24.398.5.end
        if.38.27.398.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.398.5.end:
    func.assert.398.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print.402.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.402.5.end:
    loop.403.5:
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
        func.print.406.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.406.9.end:
        func.print.407.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.407.9.end:
        func.str.input.408.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
            sub byte [rbp + 1288], 1
        func.str.input.408.12.end:
        if.410.12:
        cmp.410.12:
        cmp byte [rbp + 1288], 0
        jle loop.403.5.end
        if.410.12.code:
        if.412.19:
        cmp.412.19:
        cmp byte [rbp + 1288], 4
        jg if.410.9.else
        if.412.19.code:
            func.print.413.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.413.13.end:
            jmp loop.403.5
        if.410.9.else:
            func.greet.416.13:
                func.print.100.5.416.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.100.5.416.13.end:
                func.str.print.101.10.416.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1288]
                    cmp rdx, 127
                    ja baz_bounds_line_96
                    lea rsi, [rbp + 1289]
                    mov rax, 1
                    syscall
                func.str.print.101.10.416.13.end:
                func.print.102.5.416.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.102.5.416.13.end:
                func.print.103.5.416.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.103.5.416.13.end:
                add qword [rbp + 368], 1
            func.greet.416.13.end:
        if.410.9.end:
    jmp loop.403.5
    loop.403.5.end:
    func.print.420.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
        mov rax, 1
        syscall
    func.print.420.5.end:
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
    func.print.422.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.422.5.end:
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
    mov rdi, 1
    mov rdx, 3
    cmp rdx, 13
    ja baz_bounds_line_425
    lea rsi, [rbp + 1416]
    mov rax, 1
    syscall
    mov rdi, 1
    mov rdx, 1
    mov r15, 12
    test r15, r15
    js baz_bounds_line_426
    test rdx, rdx
    js baz_bounds_line_426
    lea r14, [rdx + r15]
    cmp r14, 13
    jg baz_bounds_line_426
    lea rsi, [rbp + 1416]
    add rsi, r15
    mov rax, 1
    syscall
    mov rdi, 0
    mov rax, 60
    syscall
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.177.8:
    cmp.177.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.177.5.end
    if.177.8.code:
        ret
    if.177.5.end:
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
func.print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 32], r14
    mov byte [rbx + 40], 0
    if.144.8:
    cmp.144.8:
    cmp qword [rbx + 32], 0
    jge if.144.5.end
    if.144.8.code:
        mov byte [rbx + 40], 1
    if.144.5.end:
    if.147.8:
    cmp.147.8:
    cmp qword [rbx + 32], 0
    jle if.147.5.end
    if.147.8.code:
        neg qword [rbx + 32]
    if.147.5.end:
    mov qword [rbx + 48], 20
    loop.152.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_154
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
        if.156.12:
        cmp.156.12:
        cmp qword [rbx + 32], 0
        jne loop.152.5
        if.156.12.code:
        if.156.9.end:
    loop.152.5.end:
    if.159.8:
    cmp.159.8:
    cmp byte [rbx + 40], 0
    je if.159.5.end
    if.159.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_161
        mov byte [rbx + r15 + 8], 45
    if.159.5.end:
    mov qword [rbx + 56], 0
    loop.165.5:
        mov r15, qword [rbx + 56]
        cmp r15, 20
        jae baz_bounds_line_166
        mov r14, qword [rbx + 48]
        cmp r14, 20
        jae baz_bounds_line_166
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 56], 1
        add qword [rbx + 48], 1
        if.169.12:
        cmp.169.12:
        cmp qword [rbx + 48], 20
        jne loop.165.5
        if.169.12.code:
        if.169.9.end:
    loop.165.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    cmp rdx, 20
    ja baz_bounds_line_172
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
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
baz_bounds_line_96:
    mov rbp, 96
    jmp baz_bounds_panic
baz_bounds_line_154:
    mov rbp, 154
    jmp baz_bounds_panic
baz_bounds_line_161:
    mov rbp, 161
    jmp baz_bounds_panic
baz_bounds_line_166:
    mov rbp, 166
    jmp baz_bounds_panic
baz_bounds_line_172:
    mov rbp, 172
    jmp baz_bounds_panic
baz_bounds_line_241:
    mov rbp, 241
    jmp baz_bounds_panic
baz_bounds_line_242:
    mov rbp, 242
    jmp baz_bounds_panic
baz_bounds_line_246:
    mov rbp, 246
    jmp baz_bounds_panic
baz_bounds_line_251:
    mov rbp, 251
    jmp baz_bounds_panic
baz_bounds_line_252:
    mov rbp, 252
    jmp baz_bounds_panic
baz_bounds_line_258:
    mov rbp, 258
    jmp baz_bounds_panic
baz_bounds_line_269:
    mov rbp, 269
    jmp baz_bounds_panic
baz_bounds_line_270:
    mov rbp, 270
    jmp baz_bounds_panic
baz_bounds_line_271:
    mov rbp, 271
    jmp baz_bounds_panic
baz_bounds_line_382:
    mov rbp, 382
    jmp baz_bounds_panic
baz_bounds_line_383:
    mov rbp, 383
    jmp baz_bounds_panic
baz_bounds_line_390:
    mov rbp, 390
    jmp baz_bounds_panic
baz_bounds_line_391:
    mov rbp, 391
    jmp baz_bounds_panic
baz_bounds_line_425:
    mov rbp, 425
    jmp baz_bounds_panic
baz_bounds_line_426:
    mov rbp, 426
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
