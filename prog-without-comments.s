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
    cmp.220.12:
    cmp qword [rbp + 392], 0
    setne r15b
    je bool.220.12.end
    cmp.220.23:
    cmp.220.24:
    cmp qword [rbp + 392], 7
    setge r15b
    jge bool.220.12.end
    cmp.220.34:
    cmp qword [rbp + 392], 0
    setl r15b
    bool.220.12.end:
    func.assert.220.5:
        if.38.27.220.5:
        cmp.38.27.220.5:
        cmp r15b, 0
        jne if.38.24.220.5.end
        if.38.27.220.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.220.5.end:
    func.assert.220.5.end:
    mov byte [rbp + 400], 100
    mov r15b, byte [rbp + 400]
    add r15b, byte [rbp + 400]
    mov byte [rbp + 400], r15b
    cmp.225.12:
    cmp byte [rbp + 400], -56
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
    movsx r15, byte [rbp + 400]
    mov qword [rbp + 408], r15
    cmp.229.12:
    cmp qword [rbp + 408], -56
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
    mov qword [rbp + 416], 65
    cmp.237.12:
    cmp qword [rbp + 416], 65
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
    mov qword [rbp + 424], 0
    mov qword [rbp + 432], 0
    mov qword [rbp + 440], 1
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_243
    mov dword [rbp + r15 * 4 + 424], 2
    mov r15, qword [rbp + 440]
    add r15, 1
    cmp r15, 4
    jae baz_bounds_line_244
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_244
    mov r13d, dword [rbp + r14 * 4 + 424]
    mov dword [rbp + r15 * 4 + 424], r13d
    cmp.245.12:
    cmp dword [rbp + 428], 2
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
    cmp.246.12:
    cmp dword [rbp + 432], 2
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
    mov r15, 2
    mov r14, 2
    test r14, r14
    js baz_bounds_line_248
    test r15, r15
    js baz_bounds_line_248
    lea r13, [r15 + r14]
    cmp r13, 4
    jg baz_bounds_line_248
    cmp r15, 4
    ja baz_bounds_line_248
    mov rax, qword [rbp + r14 * 4 + 424]
    mov qword [rbp + 424], rax
    cmp.249.12:
    cmp dword [rbp + 424], 2
    sete r15b
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
    mov qword [rbp + 448], 0
    mov qword [rbp + 456], 0
    mov qword [rbp + 464], 0
    mov qword [rbp + 472], 0
    mov r15, 4
    cmp r15, 4
    ja baz_bounds_line_253
    cmp r15, 8
    ja baz_bounds_line_253
    mov rax, qword [rbp + 424]
    mov qword [rbp + 448], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 456], rax
    cmp.254.14:
        mov rcx, 3
        mov r15, 1
        test r15, r15
        js baz_bounds_line_254
        test rcx, rcx
        js baz_bounds_line_254
        lea r14, [rcx + r15]
        cmp r14, 4
        jg baz_bounds_line_254
        lea rsi, [rbp + r15 * 4 + 424]
        mov r15, 1
        test r15, r15
        js baz_bounds_line_254
        lea r14, [rcx + r15]
        cmp r14, 8
        jg baz_bounds_line_254
        lea rdi, [rbp + r15 * 4 + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 480]
    bool.254.14.end:
    cmp.257.12:
    mov r15b, byte [rbp + 480]
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
    mov dword [rbp + 456], -1
    cmp.260.12:
        mov rcx, 4
        cmp rcx, 4
        ja baz_bounds_line_260
        lea rsi, [rbp + 424]
        cmp rcx, 8
        ja baz_bounds_line_260
        lea rdi, [rbp + 448]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
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
    mov rax, qword [rbp + 424]
    mov qword [rbp + 484], rax
    mov rax, qword [rbp + 432]
    mov qword [rbp + 492], rax
    cmp.263.12:
        lea rsi, [rbp + 424]
        lea rdi, [rbp + 484]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    cmp r15b, 0
    bool.263.12.end:
    func.assert.263.5:
        if.38.27.263.5:
        cmp.38.27.263.5:
        cmp r15b, 0
        jne if.38.24.263.5.end
        if.38.27.263.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.263.5.end:
    func.assert.263.5.end:
    mov qword [rbp + 440], 3
    mov r15, qword [rbp + 440]
    sub r15, 1
    cmp r15, 4
    jae baz_bounds_line_271
    func.inv.271.16:
        mov r14d, dword [rbp + r15 * 4 + 424]
        mov dword [rbp + 500], r14d
        not dword [rbp + 500]
    func.inv.271.16.end:
    not dword [rbp + 500]
    mov r15, qword [rbp + 440]
    cmp r15, 4
    jae baz_bounds_line_272
    mov r14d, dword [rbp + 500]
    mov dword [rbp + r15 * 4 + 424], r14d
    cmp.273.12:
    mov r14, qword [rbp + 440]
    cmp r14, 4
    jae baz_bounds_line_273
    cmp dword [rbp + r14 * 4 + 424], 2
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
    func.faz.275.5:
        mov dword [rbp + 428], 254
    func.faz.275.5.end:
    cmp.276.12:
    cmp dword [rbp + 428], 254
    sete r15b
    bool.276.12.end:
    func.assert.276.5:
        if.38.27.276.5:
        cmp.38.27.276.5:
        cmp r15b, 0
        jne if.38.24.276.5.end
        if.38.27.276.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.276.5.end:
    func.assert.276.5.end:
    mov qword [rbp + 504], 3
    mov qword [rbp + 512], 5
    lea r15, [rbp + 504]
    mov r14, 0
    foo.279.5:
        add qword [r15], r14
        add qword [r15], 2
        foo.279.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.279.5
    foo.279.5.end:
    cmp.282.12:
    cmp qword [rbp + 504], 5
    sete r15b
    bool.282.12.end:
    func.assert.282.5:
        if.38.27.282.5:
        cmp.38.27.282.5:
        cmp r15b, 0
        jne if.38.24.282.5.end
        if.38.27.282.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.282.5.end:
    func.assert.282.5.end:
    cmp.283.12:
    cmp qword [rbp + 512], 8
    sete r15b
    bool.283.12.end:
    func.assert.283.5:
        if.38.27.283.5:
        cmp.38.27.283.5:
        cmp r15b, 0
        jne if.38.24.283.5.end
        if.38.27.283.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.283.5.end:
    func.assert.283.5.end:
    mov qword [rbp + 520], 0
    mov qword [rbp + 528], 0
    func.point.fooz.291.7:
        mov qword [rbp + 520], 2
        mov qword [rbp + 528], 11
    func.point.fooz.291.7.end:
    cmp.294.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.294.12.end:
    func.assert.294.5:
        if.38.27.294.5:
        cmp.38.27.294.5:
        cmp r15b, 0
        jne if.38.24.294.5.end
        if.38.27.294.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.294.5.end:
    func.assert.294.5.end:
    cmp.295.12:
    cmp qword [rbp + 528], 11
    sete r15b
    bool.295.12.end:
    func.assert.295.5:
        if.38.27.295.5:
        cmp.38.27.295.5:
        cmp r15b, 0
        jne if.38.24.295.5.end
        if.38.27.295.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.295.5.end:
    func.assert.295.5.end:
    mov rax, qword [rbp + 520]
    mov qword [rbp + 536], rax
    mov rax, qword [rbp + 528]
    mov qword [rbp + 544], rax
    cmp.300.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    cmp r15b, 0
    bool.300.12.end:
    func.assert.300.5:
        if.38.27.300.5:
        cmp.38.27.300.5:
        cmp r15b, 0
        jne if.38.24.300.5.end
        if.38.27.300.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.300.5.end:
    func.assert.300.5.end:
    mov qword [rbp + 536], 3
    cmp.305.12:
        lea rsi, [rbp + 520]
        lea rdi, [rbp + 536]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    cmp r15b, 0
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
    mov qword [rbp + 552], 0
    func.bar.308.5:
        if.60.8.308.5:
        cmp.60.8.308.5:
        cmp qword [rbp + 552], 0
        je func.bar.308.5.end
        if.60.8.308.5.code:
        if.60.5.308.5.end:
        mov qword [rbp + 552], 255
    func.bar.308.5.end:
    cmp.309.12:
    cmp qword [rbp + 552], 0
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
    mov qword [rbp + 552], 1
    func.bar.312.5:
        if.60.8.312.5:
        cmp.60.8.312.5:
        cmp qword [rbp + 552], 0
        je func.bar.312.5.end
        if.60.8.312.5.code:
        if.60.5.312.5.end:
        mov qword [rbp + 552], 255
    func.bar.312.5.end:
    cmp.313.12:
    cmp qword [rbp + 552], 255
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
    mov qword [rbp + 560], 1
    func.baz.316.13:
        mov r15, qword [rbp + 560]
        mov qword [rbp + 568], r15
        sal qword [rbp + 568], 1
    func.baz.316.13.end:
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
    func.baz.319.9:
        mov qword [rbp + 568], 2
    func.baz.319.9.end:
    cmp.320.12:
    cmp qword [rbp + 568], 2
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
    cmp.324.12:
    cmp qword [rbp + 584], 120
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
    func.baz.326.20:
        mov qword [rbp + 592], 6
    func.baz.326.20.end:
    mov qword [rbp + 600], 0
    cmp.327.12:
    cmp qword [rbp + 592], 6
    sete r15b
    bool.327.12.end:
    func.assert.327.5:
        if.38.27.327.5:
        cmp.38.27.327.5:
        cmp r15b, 0
        jne if.38.24.327.5.end
        if.38.27.327.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.327.5.end:
    func.assert.327.5.end:
    func.point.at.329.14:
        mov qword [rbp + 608], -1
        mov qword [rbp + 616], -2
    func.point.at.329.14.end:
    cmp.333.12:
    cmp qword [rbp + 608], -1
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
    cmp qword [rbp + 616], -2
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
    func.point.x.336.8:
        mov qword [rbp + 608], 2
    func.point.x.336.8.end:
    cmp.337.12:
    cmp qword [rbp + 608], 2
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
        func.point.sum.338.15:
            mov r14, qword [rbp + 608]
            add r14, qword [rbp + 616]
        func.point.sum.338.15.end:
    cmp r14, 0
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
    mov qword [rbp + 624], 1
    mov qword [rbp + 632], 2
    mov r15, qword [rbp + 624]
    imul r15, 10
    mov qword [rbp + 640], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 648], r15
    mov dword [rbp + 656], 16711680
    mov dword [rbp + 660], 0
    cmp.344.12:
    cmp qword [rbp + 640], 10
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
    cmp.345.12:
    cmp qword [rbp + 648], 2
    sete r15b
    bool.345.12.end:
    func.assert.345.5:
        if.38.27.345.5:
        cmp.38.27.345.5:
        cmp r15b, 0
        jne if.38.24.345.5.end
        if.38.27.345.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.345.5.end:
    func.assert.345.5.end:
    cmp.346.12:
    cmp dword [rbp + 656], 16711680
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
    cmp.350.12:
    cmp qword [rbp + 640], -1
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
    cmp qword [rbp + 648], -2
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
    lea rsi, [rbp + 640]
    lea rdi, [rbp + 680]
    mov rcx, 24
    rep movsb
    cmp.354.12:
    cmp qword [rbp + 680], -1
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
    cmp.355.12:
    cmp qword [rbp + 688], -2
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
    cmp.356.12:
    cmp dword [rbp + 696], 16711680
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
    mov r15, qword [rbp + 624]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 632]
    mov qword [rbp + 688], r15
    cmp.359.12:
    cmp qword [rbp + 680], 1
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
    mov r15, qword [rbp + 632]
    mov qword [rbp + 680], r15
    mov r15, qword [rbp + 624]
    mov qword [rbp + 688], r15
    cmp.361.12:
    cmp qword [rbp + 680], 2
    sete r15b
    bool.361.12.end:
    func.assert.361.5:
        if.38.27.361.5:
        cmp.38.27.361.5:
        cmp r15b, 0
        jne if.38.24.361.5.end
        if.38.27.361.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.361.5.end:
    func.assert.361.5.end:
    xor al, al
    lea rdi, [rbp + 704]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 712], 73
    cmp.371.12:
    cmp qword [rbp + 712], 73
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
    mov dword [rbp + 748], 0
    func.object.at.372.13:
        func.point.at.120.16.372.13:
            mov qword [rbp + 728], 2
            mov qword [rbp + 736], 74
        func.point.at.120.16.372.13.end:
        mov dword [rbp + 744], 16777215
    func.object.at.372.13.end:
    cmp.373.12:
    cmp qword [rbp + 736], 74
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
    func.point.fooz.375.15:
        mov qword [rbp + 728], 2
        mov qword [rbp + 736], 11
    func.point.fooz.375.15.end:
    cmp.376.12:
        func.point.sum.376.22:
            mov r14, qword [rbp + 728]
            add r14, qword [rbp + 736]
        func.point.sum.376.22.end:
    cmp r14, 13
    sete r15b
    bool.376.12.end:
    func.assert.376.5:
        if.38.27.376.5:
        cmp.38.27.376.5:
        cmp r15b, 0
        jne if.38.24.376.5.end
        if.38.27.376.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.376.5.end:
    func.assert.376.5.end:
    xor al, al
    lea rdi, [rbp + 752]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 824], 65518
    cmp.381.12:
    cmp qword [rbp + 824], 65518
    sete r15b
    bool.381.12.end:
    func.assert.381.5:
        if.38.27.381.5:
        cmp.38.27.381.5:
        cmp r15b, 0
        jne if.38.24.381.5.end
        if.38.27.381.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.381.5.end:
    func.assert.381.5.end:
    mov r15, 8
    cmp r15, 8
    ja baz_bounds_line_384
    cmp r15, 8
    ja baz_bounds_line_385
    lea rsi, [rbp + 816]
    lea rdi, [rbp + 752]
    mov rcx, 64
    rep movsb
    cmp.390.12:
    cmp qword [rbp + 760], 65518
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
        mov rcx, 8
        cmp rcx, 8
        ja baz_bounds_line_392
        lea rsi, [rbp + 752]
        cmp rcx, 8
        ja baz_bounds_line_393
        lea rdi, [rbp + 816]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
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
    mov qword [rbp + 1264], -1
    mov qword [rbp + 1272], 2
    func.assert.398.5:
        if.38.27.398.5:
        cmp.38.27.398.5:
        if.38.24.398.5.end:
    func.assert.398.5.end:
    cmp.399.12:
    cmp qword [rbp + 1264], -1
    sete r15b
    bool.399.12.end:
    func.assert.399.5:
        if.38.27.399.5:
        cmp.38.27.399.5:
        cmp r15b, 0
        jne if.38.24.399.5.end
        if.38.27.399.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.399.5.end:
    func.assert.399.5.end:
    cmp.400.12:
    cmp qword [rbp + 1272], 2
    sete r15b
    bool.400.12.end:
    func.assert.400.5:
        if.38.27.400.5:
        cmp.38.27.400.5:
        cmp r15b, 0
        jne if.38.24.400.5.end
        if.38.27.400.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.38.24.400.5.end:
    func.assert.400.5.end:
    mov qword [rbp + 1280], 0
    xor al, al
    lea rdi, [rbp + 1288]
    mov rcx, 128
    rep stosb
    func.print.404.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.404.5.end:
    loop.405.5:
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
        func.print.408.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.408.9.end:
        func.print.409.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.409.9.end:
        func.str.input.410.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1289]
            mov rax, 0
            syscall
            mov qword [rbp + 1416], rax
            mov r15b, byte [rbp + 1416]
            mov byte [rbp + 1288], r15b
            sub byte [rbp + 1288], 1
        func.str.input.410.12.end:
        if.412.12:
        cmp.412.12:
        cmp byte [rbp + 1288], 0
        jle loop.405.5.end
        if.412.12.code:
        if.414.19:
        cmp.414.19:
        cmp byte [rbp + 1288], 4
        jg if.412.9.else
        if.414.19.code:
            func.print.415.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.415.13.end:
            jmp loop.405.5
        if.412.9.else:
            func.greet.418.13:
                func.print.100.5.418.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.100.5.418.13.end:
                func.str.print.101.10.418.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1288]
                    cmp rdx, 127
                    ja baz_bounds_line_96
                    lea rsi, [rbp + 1289]
                    mov rax, 1
                    syscall
                func.str.print.101.10.418.13.end:
                func.print.102.5.418.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.102.5.418.13.end:
                func.print.103.5.418.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.103.5.418.13.end:
                add qword [rbp + 368], 1
            func.greet.418.13.end:
        if.412.9.end:
    jmp loop.405.5
    loop.405.5.end:
    func.print.422.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 352]
        mov rax, 1
        syscall
    func.print.422.5.end:
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
    func.print.424.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.424.5.end:
    mov dword [rbp + 1416], 543521122
    mov dword [rbp + 1420], 1836020326
    mov dword [rbp + 1424], 2053202464
    mov byte [rbp + 1428], 10
    mov rdi, 1
    mov rdx, 3
    cmp rdx, 13
    ja baz_bounds_line_427
    lea rsi, [rbp + 1416]
    mov rax, 1
    syscall
    mov rdi, 1
    mov rdx, 1
    mov r15, 12
    test r15, r15
    js baz_bounds_line_428
    test rdx, rdx
    js baz_bounds_line_428
    lea r14, [rdx + r15]
    cmp r14, 13
    jg baz_bounds_line_428
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
baz_bounds_line_243:
    mov rbp, 243
    jmp baz_bounds_panic
baz_bounds_line_244:
    mov rbp, 244
    jmp baz_bounds_panic
baz_bounds_line_248:
    mov rbp, 248
    jmp baz_bounds_panic
baz_bounds_line_253:
    mov rbp, 253
    jmp baz_bounds_panic
baz_bounds_line_254:
    mov rbp, 254
    jmp baz_bounds_panic
baz_bounds_line_260:
    mov rbp, 260
    jmp baz_bounds_panic
baz_bounds_line_271:
    mov rbp, 271
    jmp baz_bounds_panic
baz_bounds_line_272:
    mov rbp, 272
    jmp baz_bounds_panic
baz_bounds_line_273:
    mov rbp, 273
    jmp baz_bounds_panic
baz_bounds_line_384:
    mov rbp, 384
    jmp baz_bounds_panic
baz_bounds_line_385:
    mov rbp, 385
    jmp baz_bounds_panic
baz_bounds_line_392:
    mov rbp, 392
    jmp baz_bounds_panic
baz_bounds_line_393:
    mov rbp, 393
    jmp baz_bounds_panic
baz_bounds_line_427:
    mov rbp, 427
    jmp baz_bounds_panic
baz_bounds_line_428:
    mov rbp, 428
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
