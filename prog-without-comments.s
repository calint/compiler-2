default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 256], 0
    cmp.182.12:
    cmp qword [rbp + 256], 0
    sete r15b
    bool.182.12.end:
    func.assert.182.5:
        if.37.27.182.5:
        cmp.37.27.182.5:
        cmp r15b, 0
        jne if.37.24.182.5.end
        if.37.27.182.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.182.5.end:
    func.assert.182.5.end:
    mov qword [rbp + 256], -1
    cmp.185.12:
    cmp qword [rbp + 256], -1
    sete r15b
    bool.185.12.end:
    func.assert.185.5:
        if.37.27.185.5:
        cmp.37.27.185.5:
        cmp r15b, 0
        jne if.37.24.185.5.end
        if.37.27.185.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.185.5.end:
    func.assert.185.5.end:
        func.assert.191.9:
            if.37.27.191.9:
            cmp.37.27.191.9:
            if.37.24.191.9.end:
        func.assert.191.9.end:
    func.assert.194.5:
        if.37.27.194.5:
        cmp.37.27.194.5:
        if.37.24.194.5.end:
    func.assert.194.5.end:
    cmp.196.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.196.12.end
    cmp.196.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.196.12.end:
    func.assert.196.5:
        if.37.27.196.5:
        cmp.37.27.196.5:
        cmp r15b, 0
        jne if.37.24.196.5.end
        if.37.27.196.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.196.5.end:
    func.assert.196.5.end:
    cmp.197.12:
    cmp byte [rbp + 96], 3
    sete r15b
    jne bool.197.12.end
    cmp.197.30:
    cmp byte [rbp + 99], 122
    sete r15b
    bool.197.12.end:
    func.assert.197.5:
        if.37.27.197.5:
        cmp.37.27.197.5:
        cmp r15b, 0
        jne if.37.24.197.5.end
        if.37.27.197.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.197.5.end:
    func.assert.197.5.end:
    mov qword [rbp + 264], 7
    cmp.200.12:
        mov r14, qword [rbp + 264]
        and r14, 3
    cmp r14, 3
    sete r15b
    bool.200.12.end:
    func.assert.200.5:
        if.37.27.200.5:
        cmp.37.27.200.5:
        cmp r15b, 0
        jne if.37.24.200.5.end
        if.37.27.200.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.200.5.end:
    func.assert.200.5.end:
    cmp.201.12:
        mov r14, qword [rbp + 264]
        or r14, 8
    cmp r14, 15
    sete r15b
    bool.201.12.end:
    func.assert.201.5:
        if.37.27.201.5:
        cmp.37.27.201.5:
        cmp r15b, 0
        jne if.37.24.201.5.end
        if.37.27.201.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.201.5.end:
    func.assert.201.5.end:
    cmp.202.12:
        mov r14, qword [rbp + 264]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.202.12.end:
    func.assert.202.5:
        if.37.27.202.5:
        cmp.37.27.202.5:
        cmp r15b, 0
        jne if.37.24.202.5.end
        if.37.27.202.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.202.5.end:
    func.assert.202.5.end:
    cmp.203.12:
        mov r14, qword [rbp + 264]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.203.12.end:
    func.assert.203.5:
        if.37.27.203.5:
        cmp.37.27.203.5:
        cmp r15b, 0
        jne if.37.24.203.5.end
        if.37.27.203.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.203.5.end:
    func.assert.203.5.end:
    cmp.204.12:
        mov r14, qword [rbp + 264]
        neg r14
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.204.12.end:
    func.assert.204.5:
        if.37.27.204.5:
        cmp.37.27.204.5:
        cmp r15b, 0
        jne if.37.24.204.5.end
        if.37.27.204.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.204.5.end:
    func.assert.204.5.end:
    cmp.207.12:
        mov r14, qword [rbp + 264]
        mov r13, qword [rbp + 264]
        sal r13, 1
        add r14, r13
    cmp r14, 21
    sete r15b
    bool.207.12.end:
    func.assert.207.5:
        if.37.27.207.5:
        cmp.37.27.207.5:
        cmp r15b, 0
        jne if.37.24.207.5.end
        if.37.27.207.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.207.5.end:
    func.assert.207.5.end:
    cmp.208.12:
        mov r14, qword [rbp + 264]
        add r14, qword [rbp + 264]
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.208.12.end:
    func.assert.208.5:
        if.37.27.208.5:
        cmp.37.27.208.5:
        cmp r15b, 0
        jne if.37.24.208.5.end
        if.37.27.208.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.208.5.end:
    func.assert.208.5.end:
    cmp.211.12:
    cmp qword [rbp + 264], 0
    setne r15b
    je bool.211.12.end
    cmp.211.23:
    cmp.211.24:
    cmp qword [rbp + 264], 7
    setge r15b
    jge bool.211.12.end
    cmp.211.34:
    cmp qword [rbp + 264], 0
    setl r15b
    bool.211.12.end:
    func.assert.211.5:
        if.37.27.211.5:
        cmp.37.27.211.5:
        cmp r15b, 0
        jne if.37.24.211.5.end
        if.37.27.211.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.211.5.end:
    func.assert.211.5.end:
    mov byte [rbp + 272], 100
    mov r15b, byte [rbp + 272]
    add r15b, byte [rbp + 272]
    mov byte [rbp + 272], r15b
    cmp.216.12:
    cmp byte [rbp + 272], -56
    sete r15b
    bool.216.12.end:
    func.assert.216.5:
        if.37.27.216.5:
        cmp.37.27.216.5:
        cmp r15b, 0
        jne if.37.24.216.5.end
        if.37.27.216.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.216.5.end:
    func.assert.216.5.end:
    movsx r15, byte [rbp + 272]
    mov qword [rbp + 280], r15
    cmp.220.12:
    cmp qword [rbp + 280], -56
    sete r15b
    bool.220.12.end:
    func.assert.220.5:
        if.37.27.220.5:
        cmp.37.27.220.5:
        cmp r15b, 0
        jne if.37.24.220.5.end
        if.37.27.220.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.220.5.end:
    func.assert.220.5.end:
    mov qword [rbp + 288], 65
    cmp.228.12:
    cmp qword [rbp + 288], 65
    sete r15b
    bool.228.12.end:
    func.assert.228.5:
        if.37.27.228.5:
        cmp.37.27.228.5:
        cmp r15b, 0
        jne if.37.24.228.5.end
        if.37.27.228.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.228.5.end:
    func.assert.228.5.end:
    mov qword [rbp + 296], 0
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 1
    mov r15, qword [rbp + 312]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 296], 2
    mov r15, qword [rbp + 312]
    add r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14, qword [rbp + 312]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 296]
    mov dword [rbp + r15 * 4 + 296], r13d
    cmp.236.12:
    cmp dword [rbp + 300], 2
    sete r15b
    bool.236.12.end:
    func.assert.236.5:
        if.37.27.236.5:
        cmp.37.27.236.5:
        cmp r15b, 0
        jne if.37.24.236.5.end
        if.37.27.236.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.236.5.end:
    func.assert.236.5.end:
    cmp.237.12:
    cmp dword [rbp + 304], 2
    sete r15b
    bool.237.12.end:
    func.assert.237.5:
        if.37.27.237.5:
        cmp.37.27.237.5:
        cmp r15b, 0
        jne if.37.24.237.5.end
        if.37.27.237.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.237.5.end:
    func.assert.237.5.end:
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
    mov rax, qword [rbp + r14 * 4 + 296]
    mov qword [rbp + 296], rax
    cmp.240.12:
    cmp dword [rbp + 296], 2
    sete r15b
    bool.240.12.end:
    func.assert.240.5:
        if.37.27.240.5:
        cmp.37.27.240.5:
        cmp r15b, 0
        jne if.37.24.240.5.end
        if.37.27.240.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.240.5.end:
    func.assert.240.5.end:
    mov qword [rbp + 320], 0
    mov qword [rbp + 328], 0
    mov qword [rbp + 336], 0
    mov qword [rbp + 344], 0
    mov r15, 4
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jg baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    cmp r15, 8
    jg baz_bounds_panic
    mov rax, qword [rbp + 296]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 328], rax
    cmp.245.14:
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
        lea rsi, [rbp + r15 * 4 + 296]
        mov r15, 1
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
        mov r14, rcx
        add r14, r15
        cmp r14, 8
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 320]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 352]
    bool.245.14.end:
    cmp.248.12:
    mov r15b, byte [rbp + 352]
    bool.248.12.end:
    func.assert.248.5:
        if.37.27.248.5:
        cmp.37.27.248.5:
        cmp r15b, 0
        jne if.37.24.248.5.end
        if.37.27.248.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.248.5.end:
    func.assert.248.5.end:
    mov dword [rbp + 328], -1
    cmp.251.12:
        mov rcx, 4
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 296]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 320]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.251.12.end:
    func.assert.251.5:
        if.37.27.251.5:
        cmp.37.27.251.5:
        cmp r15b, 0
        jne if.37.24.251.5.end
        if.37.27.251.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.251.5.end:
    func.assert.251.5.end:
    mov rax, qword [rbp + 296]
    mov qword [rbp + 356], rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 364], rax
    cmp.254.12:
        lea rsi, [rbp + 296]
        lea rdi, [rbp + 356]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.254.12.end:
    func.assert.254.5:
        if.37.27.254.5:
        cmp.37.27.254.5:
        cmp r15b, 0
        jne if.37.24.254.5.end
        if.37.27.254.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.254.5.end:
    func.assert.254.5.end:
    mov qword [rbp + 312], 3
    mov r15, qword [rbp + 312]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.262.16:
        mov r14d, dword [rbp + r15 * 4 + 296]
        mov dword [rbp + 372], r14d
        not dword [rbp + 372]
    func.inv.262.16.end:
    not dword [rbp + 372]
    mov r15, qword [rbp + 312]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 372]
    mov dword [rbp + r15 * 4 + 296], r14d
    cmp.264.12:
    mov r14, qword [rbp + 312]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 296], 2
    sete r15b
    bool.264.12.end:
    func.assert.264.5:
        if.37.27.264.5:
        cmp.37.27.264.5:
        cmp r15b, 0
        jne if.37.24.264.5.end
        if.37.27.264.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.264.5.end:
    func.assert.264.5.end:
    func.faz.266.5:
        mov dword [rbp + 300], 254
    func.faz.266.5.end:
    cmp.267.12:
    cmp dword [rbp + 300], 254
    sete r15b
    bool.267.12.end:
    func.assert.267.5:
        if.37.27.267.5:
        cmp.37.27.267.5:
        cmp r15b, 0
        jne if.37.24.267.5.end
        if.37.27.267.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.267.5.end:
    func.assert.267.5.end:
    mov qword [rbp + 376], 3
    mov qword [rbp + 384], 5
    lea r15, [rbp + 376]
    mov qword [rbp + 400], 0
    foo.270.5:
        mov r14, qword [rbp + 400]
        add qword [r15], r14
        add qword [r15], 2
        foo.270.5.continue:
            add r15, 8
            inc qword [rbp + 400]
            cmp qword [rbp + 400], 2
            jne foo.270.5
    foo.270.5.end:
    cmp.273.12:
    cmp qword [rbp + 376], 5
    sete r15b
    bool.273.12.end:
    func.assert.273.5:
        if.37.27.273.5:
        cmp.37.27.273.5:
        cmp r15b, 0
        jne if.37.24.273.5.end
        if.37.27.273.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.273.5.end:
    func.assert.273.5.end:
    cmp.274.12:
    cmp qword [rbp + 384], 8
    sete r15b
    bool.274.12.end:
    func.assert.274.5:
        if.37.27.274.5:
        cmp.37.27.274.5:
        cmp r15b, 0
        jne if.37.24.274.5.end
        if.37.27.274.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.274.5.end:
    func.assert.274.5.end:
    mov qword [rbp + 392], 0
    mov qword [rbp + 400], 0
    func.point.fooz.282.7:
        mov qword [rbp + 392], 2
        mov qword [rbp + 400], 11
    func.point.fooz.282.7.end:
    cmp.285.12:
    cmp qword [rbp + 392], 2
    sete r15b
    bool.285.12.end:
    func.assert.285.5:
        if.37.27.285.5:
        cmp.37.27.285.5:
        cmp r15b, 0
        jne if.37.24.285.5.end
        if.37.27.285.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.285.5.end:
    func.assert.285.5.end:
    cmp.286.12:
    cmp qword [rbp + 400], 11
    sete r15b
    bool.286.12.end:
    func.assert.286.5:
        if.37.27.286.5:
        cmp.37.27.286.5:
        cmp r15b, 0
        jne if.37.24.286.5.end
        if.37.27.286.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.286.5.end:
    func.assert.286.5.end:
    mov rax, qword [rbp + 392]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 400]
    mov qword [rbp + 416], rax
    cmp.291.12:
        lea rsi, [rbp + 392]
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    bool.291.12.end:
    func.assert.291.5:
        if.37.27.291.5:
        cmp.37.27.291.5:
        cmp r15b, 0
        jne if.37.24.291.5.end
        if.37.27.291.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.291.5.end:
    func.assert.291.5.end:
    mov qword [rbp + 408], 3
    cmp.296.12:
        lea rsi, [rbp + 392]
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    bool.296.12.end:
    func.assert.296.5:
        if.37.27.296.5:
        cmp.37.27.296.5:
        cmp r15b, 0
        jne if.37.24.296.5.end
        if.37.27.296.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.296.5.end:
    func.assert.296.5.end:
    mov qword [rbp + 424], 0
    func.bar.299.5:
        if.58.8.299.5:
        cmp.58.8.299.5:
        cmp qword [rbp + 424], 0
        je func.bar.299.5.end
        if.58.8.299.5.code:
        if.58.5.299.5.end:
        mov qword [rbp + 424], 255
    func.bar.299.5.end:
    cmp.300.12:
    cmp qword [rbp + 424], 0
    sete r15b
    bool.300.12.end:
    func.assert.300.5:
        if.37.27.300.5:
        cmp.37.27.300.5:
        cmp r15b, 0
        jne if.37.24.300.5.end
        if.37.27.300.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.300.5.end:
    func.assert.300.5.end:
    mov qword [rbp + 424], 1
    func.bar.303.5:
        if.58.8.303.5:
        cmp.58.8.303.5:
        cmp qword [rbp + 424], 0
        je func.bar.303.5.end
        if.58.8.303.5.code:
        if.58.5.303.5.end:
        mov qword [rbp + 424], 255
    func.bar.303.5.end:
    cmp.304.12:
    cmp qword [rbp + 424], 255
    sete r15b
    bool.304.12.end:
    func.assert.304.5:
        if.37.27.304.5:
        cmp.37.27.304.5:
        cmp r15b, 0
        jne if.37.24.304.5.end
        if.37.27.304.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.304.5.end:
    func.assert.304.5.end:
    mov qword [rbp + 432], 1
    func.baz.307.13:
        mov r15, qword [rbp + 432]
        mov qword [rbp + 440], r15
        sal qword [rbp + 440], 1
    func.baz.307.13.end:
    cmp.308.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.308.12.end:
    func.assert.308.5:
        if.37.27.308.5:
        cmp.37.27.308.5:
        cmp r15b, 0
        jne if.37.24.308.5.end
        if.37.27.308.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.308.5.end:
    func.assert.308.5.end:
    func.baz.310.9:
        mov qword [rbp + 440], 2
    func.baz.310.9.end:
    cmp.311.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.311.12.end:
    func.assert.311.5:
        if.37.27.311.5:
        cmp.37.27.311.5:
        cmp r15b, 0
        jne if.37.24.311.5.end
        if.37.27.311.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.311.5.end:
    func.assert.311.5.end:
    mov qword [rbp + 448], 5
    lea r15, [rbp + 464]
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
    lea r15, [rbp + 456]
    mov qword [rbp + 464], r15
    lea r15, [rbp + 448]
    mov qword [rbp + 472], r15
    lea rbx, [rbp + 464]
    call func.factorial
    cmp.315.12:
    cmp qword [rbp + 456], 120
    sete r15b
    bool.315.12.end:
    func.assert.315.5:
        if.37.27.315.5:
        cmp.37.27.315.5:
        cmp r15b, 0
        jne if.37.24.315.5.end
        if.37.27.315.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.315.5.end:
    func.assert.315.5.end:
    func.baz.317.20:
        mov qword [rbp + 464], 6
    func.baz.317.20.end:
    mov qword [rbp + 472], 0
    cmp.318.12:
    cmp qword [rbp + 464], 6
    sete r15b
    bool.318.12.end:
    func.assert.318.5:
        if.37.27.318.5:
        cmp.37.27.318.5:
        cmp r15b, 0
        jne if.37.24.318.5.end
        if.37.27.318.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.318.5.end:
    func.assert.318.5.end:
    func.point.at.320.14:
        mov qword [rbp + 480], -1
        mov qword [rbp + 488], -2
    func.point.at.320.14.end:
    cmp.324.12:
    cmp qword [rbp + 480], -1
    sete r15b
    bool.324.12.end:
    func.assert.324.5:
        if.37.27.324.5:
        cmp.37.27.324.5:
        cmp r15b, 0
        jne if.37.24.324.5.end
        if.37.27.324.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.324.5.end:
    func.assert.324.5.end:
    cmp.325.12:
    cmp qword [rbp + 488], -2
    sete r15b
    bool.325.12.end:
    func.assert.325.5:
        if.37.27.325.5:
        cmp.37.27.325.5:
        cmp r15b, 0
        jne if.37.24.325.5.end
        if.37.27.325.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.325.5.end:
    func.assert.325.5.end:
    func.point.x.327.8:
        mov qword [rbp + 480], 2
    func.point.x.327.8.end:
    cmp.328.12:
    cmp qword [rbp + 480], 2
    sete r15b
    bool.328.12.end:
    func.assert.328.5:
        if.37.27.328.5:
        cmp.37.27.328.5:
        cmp r15b, 0
        jne if.37.24.328.5.end
        if.37.27.328.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.328.5.end:
    func.assert.328.5.end:
    cmp.329.12:
        func.point.sum.329.15:
            mov r14, qword [rbp + 480]
            add r14, qword [rbp + 488]
        func.point.sum.329.15.end:
    cmp r14, 0
    sete r15b
    bool.329.12.end:
    func.assert.329.5:
        if.37.27.329.5:
        cmp.37.27.329.5:
        cmp r15b, 0
        jne if.37.24.329.5.end
        if.37.27.329.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.329.5.end:
    func.assert.329.5.end:
    mov qword [rbp + 496], 1
    mov qword [rbp + 504], 2
    mov r15, qword [rbp + 496]
    imul r15, 10
    mov qword [rbp + 512], r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 520], r15
    mov dword [rbp + 528], 16711680
    mov dword [rbp + 532], 0
    cmp.335.12:
    cmp qword [rbp + 512], 10
    sete r15b
    bool.335.12.end:
    func.assert.335.5:
        if.37.27.335.5:
        cmp.37.27.335.5:
        cmp r15b, 0
        jne if.37.24.335.5.end
        if.37.27.335.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.335.5.end:
    func.assert.335.5.end:
    cmp.336.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.336.12.end:
    func.assert.336.5:
        if.37.27.336.5:
        cmp.37.27.336.5:
        cmp r15b, 0
        jne if.37.24.336.5.end
        if.37.27.336.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.336.5.end:
    func.assert.336.5.end:
    cmp.337.12:
    cmp dword [rbp + 528], 16711680
    sete r15b
    bool.337.12.end:
    func.assert.337.5:
        if.37.27.337.5:
        cmp.37.27.337.5:
        cmp r15b, 0
        jne if.37.24.337.5.end
        if.37.27.337.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.337.5.end:
    func.assert.337.5.end:
    mov r15, qword [rbp + 496]
    mov qword [rbp + 536], r15
    neg qword [rbp + 536]
    mov r15, qword [rbp + 504]
    mov qword [rbp + 544], r15
    neg qword [rbp + 544]
    mov rax, qword [rbp + 536]
    mov qword [rbp + 512], rax
    mov rax, qword [rbp + 544]
    mov qword [rbp + 520], rax
    cmp.341.12:
    cmp qword [rbp + 512], -1
    sete r15b
    bool.341.12.end:
    func.assert.341.5:
        if.37.27.341.5:
        cmp.37.27.341.5:
        cmp r15b, 0
        jne if.37.24.341.5.end
        if.37.27.341.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.341.5.end:
    func.assert.341.5.end:
    cmp.342.12:
    cmp qword [rbp + 520], -2
    sete r15b
    bool.342.12.end:
    func.assert.342.5:
        if.37.27.342.5:
        cmp.37.27.342.5:
        cmp r15b, 0
        jne if.37.24.342.5.end
        if.37.27.342.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.342.5.end:
    func.assert.342.5.end:
    lea rsi, [rbp + 512]
    lea rdi, [rbp + 552]
    mov rcx, 24
    rep movsb
    cmp.345.12:
    cmp qword [rbp + 552], -1
    sete r15b
    bool.345.12.end:
    func.assert.345.5:
        if.37.27.345.5:
        cmp.37.27.345.5:
        cmp r15b, 0
        jne if.37.24.345.5.end
        if.37.27.345.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.345.5.end:
    func.assert.345.5.end:
    cmp.346.12:
    cmp qword [rbp + 560], -2
    sete r15b
    bool.346.12.end:
    func.assert.346.5:
        if.37.27.346.5:
        cmp.37.27.346.5:
        cmp r15b, 0
        jne if.37.24.346.5.end
        if.37.27.346.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.346.5.end:
    func.assert.346.5.end:
    cmp.347.12:
    cmp dword [rbp + 568], 16711680
    sete r15b
    bool.347.12.end:
    func.assert.347.5:
        if.37.27.347.5:
        cmp.37.27.347.5:
        cmp r15b, 0
        jne if.37.24.347.5.end
        if.37.27.347.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.347.5.end:
    func.assert.347.5.end:
    mov r15, qword [rbp + 496]
    mov qword [rbp + 552], r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 560], r15
    cmp.350.12:
    cmp qword [rbp + 552], 1
    sete r15b
    bool.350.12.end:
    func.assert.350.5:
        if.37.27.350.5:
        cmp.37.27.350.5:
        cmp r15b, 0
        jne if.37.24.350.5.end
        if.37.27.350.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.350.5.end:
    func.assert.350.5.end:
    mov r15, qword [rbp + 504]
    mov qword [rbp + 552], r15
    mov r15, qword [rbp + 496]
    mov qword [rbp + 560], r15
    cmp.352.12:
    cmp qword [rbp + 552], 2
    sete r15b
    bool.352.12.end:
    func.assert.352.5:
        if.37.27.352.5:
        cmp.37.27.352.5:
        cmp r15b, 0
        jne if.37.24.352.5.end
        if.37.27.352.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.352.5.end:
    func.assert.352.5.end:
    xor al, al
    lea rdi, [rbp + 576]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 584], 73
    cmp.362.12:
    cmp qword [rbp + 584], 73
    sete r15b
    bool.362.12.end:
    func.assert.362.5:
        if.37.27.362.5:
        cmp.37.27.362.5:
        cmp r15b, 0
        jne if.37.24.362.5.end
        if.37.27.362.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.362.5.end:
    func.assert.362.5.end:
    func.object.at.363.13:
        func.point.at.114.16.363.13:
            mov qword [rbp + 600], 2
            mov qword [rbp + 608], 74
        func.point.at.114.16.363.13.end:
        mov dword [rbp + 616], 16777215
    func.object.at.363.13.end:
    cmp.364.12:
    cmp qword [rbp + 608], 74
    sete r15b
    bool.364.12.end:
    func.assert.364.5:
        if.37.27.364.5:
        cmp.37.27.364.5:
        cmp r15b, 0
        jne if.37.24.364.5.end
        if.37.27.364.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.364.5.end:
    func.assert.364.5.end:
    func.point.fooz.366.15:
        mov qword [rbp + 600], 2
        mov qword [rbp + 608], 11
    func.point.fooz.366.15.end:
    cmp.367.12:
        func.point.sum.367.22:
            mov r14, qword [rbp + 600]
            add r14, qword [rbp + 608]
        func.point.sum.367.22.end:
    cmp r14, 13
    sete r15b
    bool.367.12.end:
    func.assert.367.5:
        if.37.27.367.5:
        cmp.37.27.367.5:
        cmp r15b, 0
        jne if.37.24.367.5.end
        if.37.27.367.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.367.5.end:
    func.assert.367.5.end:
    xor al, al
    lea rdi, [rbp + 624]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 696], 65518
    cmp.372.12:
    cmp qword [rbp + 696], 65518
    sete r15b
    bool.372.12.end:
    func.assert.372.5:
        if.37.27.372.5:
        cmp.37.27.372.5:
        cmp r15b, 0
        jne if.37.24.372.5.end
        if.37.27.372.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.372.5.end:
    func.assert.372.5.end:
    mov rcx, 8
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 688]
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 624]
    shl rcx, 3
    rep movsb
    cmp.381.12:
    cmp qword [rbp + 632], 65518
    sete r15b
    bool.381.12.end:
    func.assert.381.5:
        if.37.27.381.5:
        cmp.37.27.381.5:
        cmp r15b, 0
        jne if.37.24.381.5.end
        if.37.27.381.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.381.5.end:
    func.assert.381.5.end:
    cmp.382.12:
        mov rcx, 8
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 624]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 688]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.382.12.end:
    func.assert.382.5:
        if.37.27.382.5:
        cmp.37.27.382.5:
        cmp r15b, 0
        jne if.37.24.382.5.end
        if.37.27.382.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.382.5.end:
    func.assert.382.5.end:
    mov qword [rbp + 1136], -1
    mov qword [rbp + 1144], 2
    cmp.389.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.389.12.end:
    func.assert.389.5:
        if.37.27.389.5:
        cmp.37.27.389.5:
        cmp r15b, 0
        jne if.37.24.389.5.end
        if.37.27.389.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.389.5.end:
    func.assert.389.5.end:
    cmp.390.12:
    cmp qword [rbp + 1136], -1
    sete r15b
    bool.390.12.end:
    func.assert.390.5:
        if.37.27.390.5:
        cmp.37.27.390.5:
        cmp r15b, 0
        jne if.37.24.390.5.end
        if.37.27.390.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.390.5.end:
    func.assert.390.5.end:
    cmp.391.12:
    cmp qword [rbp + 1144], 2
    sete r15b
    bool.391.12.end:
    func.assert.391.5:
        if.37.27.391.5:
        cmp.37.27.391.5:
        cmp r15b, 0
        jne if.37.24.391.5.end
        if.37.27.391.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.37.24.391.5.end:
    func.assert.391.5.end:
    mov qword [rbp + 1152], 0
    xor al, al
    lea rdi, [rbp + 1160]
    mov rcx, 128
    rep stosb
    func.print.395.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.395.5.end:
    loop.396.5:
        add qword [rbp + 1152], 1
        lea r15, [rbp + 1288]
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
        lea r15, [rbp + 1152]
        mov qword [rbp + 1288], r15
        lea rbx, [rbp + 1288]
        call func.print_num
        func.print.399.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.399.9.end:
        func.print.400.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.400.9.end:
        func.str.input.401.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1161]
            mov rax, 0
            syscall
            mov qword [rbp + 1288], rax
            mov r15b, byte [rbp + 1288]
            mov byte [rbp + 1160], r15b
            sub byte [rbp + 1160], 1
        func.str.input.401.12.end:
        if.403.12:
        cmp.403.12:
        cmp byte [rbp + 1160], 0
        jle loop.396.5.end
        if.403.12.code:
        if.405.19:
        cmp.405.19:
        cmp byte [rbp + 1160], 4
        jg if.403.9.else
        if.405.19.code:
            func.print.406.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.406.13.end:
            jmp loop.396.5
        if.403.9.else:
            func.greet.409.13:
                func.print.94.5.409.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.94.5.409.13.end:
                func.str.print.95.10.409.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1160]
                    test rdx, rdx
                    js baz_bounds_panic
                    cmp rdx, 127
                    jg baz_bounds_panic
                    lea rsi, [rbp + 1161]
                    mov rax, 1
                    syscall
                func.str.print.95.10.409.13.end:
                func.print.96.5.409.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.96.5.409.13.end:
                func.print.97.5.409.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.97.5.409.13.end:
                add qword [rbp + 240], 1
            func.greet.409.13.end:
        if.403.9.end:
    jmp loop.396.5
    loop.396.5.end:
    func.print.413.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 224]
        mov rax, 1
        syscall
    func.print.413.5.end:
    lea r15, [rbp + 1288]
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
    lea r15, [rbp + 240]
    mov qword [rbp + 1288], r15
    lea rbx, [rbp + 1288]
    call func.print_num
    func.print.415.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.415.5.end:
    mov dword [rbp + 1288], 543521122
    mov dword [rbp + 1292], 1836020326
    mov dword [rbp + 1296], 2053202464
    mov byte [rbp + 1300], 10
    mov rdi, 1
    mov rdx, 3
    test rdx, rdx
    js baz_bounds_panic
    cmp rdx, 13
    jg baz_bounds_panic
    lea rsi, [rbp + 1288]
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
    lea rsi, [rbp + 1288]
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
    if.137.8:
    cmp.137.8:
    cmp qword [rbx + 32], 0
    jge if.137.5.end
    if.137.8.code:
        mov byte [rbx + 40], 1
    if.137.5.end:
    if.140.8:
    cmp.140.8:
    cmp qword [rbx + 32], 0
    jle if.140.5.end
    if.140.8.code:
        neg qword [rbx + 32]
    if.140.5.end:
    mov qword [rbx + 48], 20
    loop.145.5:
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
        if.149.12:
        cmp.149.12:
        cmp qword [rbx + 32], 0
        jne loop.145.5
        if.149.12.code:
        if.149.9.end:
    loop.145.5.end:
    if.152.8:
    cmp.152.8:
    cmp byte [rbx + 40], 0
    je if.152.5.end
    if.152.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.152.5.end:
    mov qword [rbx + 56], 0
    loop.158.5:
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
        if.162.12:
        cmp.162.12:
        cmp qword [rbx + 48], 20
        jne loop.158.5
        if.162.12.code:
        if.162.9.end:
    loop.158.5.end:
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
    if.170.8:
    cmp.170.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.170.5.end
    if.170.8.code:
        ret
    if.170.5.end:
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
