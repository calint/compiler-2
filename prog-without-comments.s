default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 256], 0
    cmp.181.12:
    cmp qword [rbp + 256], 0
    sete r15b
    bool.181.12.end:
    func.assert.181.5:
        if.36.27.181.5:
        cmp.36.27.181.5:
        cmp r15b, 0
        jne if.36.24.181.5.end
        if.36.27.181.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.181.5.end:
    func.assert.181.5.end:
    mov qword [rbp + 256], -1
    cmp.184.12:
    cmp qword [rbp + 256], -1
    sete r15b
    bool.184.12.end:
    func.assert.184.5:
        if.36.27.184.5:
        cmp.36.27.184.5:
        cmp r15b, 0
        jne if.36.24.184.5.end
        if.36.27.184.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.184.5.end:
    func.assert.184.5.end:
        func.assert.190.9:
            if.36.27.190.9:
            cmp.36.27.190.9:
            if.36.24.190.9.end:
        func.assert.190.9.end:
    func.assert.193.5:
        if.36.27.193.5:
        cmp.36.27.193.5:
        if.36.24.193.5.end:
    func.assert.193.5.end:
    cmp.195.12:
    cmp qword [rbp + 64], 1
    sete r15b
    jne bool.195.12.end
    cmp.195.29:
    cmp qword [rbp + 88], 0
    sete r15b
    bool.195.12.end:
    func.assert.195.5:
        if.36.27.195.5:
        cmp.36.27.195.5:
        cmp r15b, 0
        jne if.36.24.195.5.end
        if.36.27.195.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.195.5.end:
    func.assert.195.5.end:
    cmp.196.12:
    cmp byte [rbp + 96], 3
    sete r15b
    jne bool.196.12.end
    cmp.196.30:
    cmp byte [rbp + 99], 122
    sete r15b
    bool.196.12.end:
    func.assert.196.5:
        if.36.27.196.5:
        cmp.36.27.196.5:
        cmp r15b, 0
        jne if.36.24.196.5.end
        if.36.27.196.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.196.5.end:
    func.assert.196.5.end:
    mov qword [rbp + 264], 7
    cmp.199.12:
        mov r14, qword [rbp + 264]
        and r14, 3
    cmp r14, 3
    sete r15b
    bool.199.12.end:
    func.assert.199.5:
        if.36.27.199.5:
        cmp.36.27.199.5:
        cmp r15b, 0
        jne if.36.24.199.5.end
        if.36.27.199.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.199.5.end:
    func.assert.199.5.end:
    cmp.200.12:
        mov r14, qword [rbp + 264]
        or r14, 8
    cmp r14, 15
    sete r15b
    bool.200.12.end:
    func.assert.200.5:
        if.36.27.200.5:
        cmp.36.27.200.5:
        cmp r15b, 0
        jne if.36.24.200.5.end
        if.36.27.200.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.200.5.end:
    func.assert.200.5.end:
    cmp.201.12:
        mov r14, qword [rbp + 264]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.201.12.end:
    func.assert.201.5:
        if.36.27.201.5:
        cmp.36.27.201.5:
        cmp r15b, 0
        jne if.36.24.201.5.end
        if.36.27.201.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.201.5.end:
    func.assert.201.5.end:
    cmp.202.12:
        mov r14, qword [rbp + 264]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.202.12.end:
    func.assert.202.5:
        if.36.27.202.5:
        cmp.36.27.202.5:
        cmp r15b, 0
        jne if.36.24.202.5.end
        if.36.27.202.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.202.5.end:
    func.assert.202.5.end:
    cmp.203.12:
        mov r14, qword [rbp + 264]
        neg r14
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.203.12.end:
    func.assert.203.5:
        if.36.27.203.5:
        cmp.36.27.203.5:
        cmp r15b, 0
        jne if.36.24.203.5.end
        if.36.27.203.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.203.5.end:
    func.assert.203.5.end:
    cmp.206.12:
        mov r14, qword [rbp + 264]
        mov r13, qword [rbp + 264]
        sal r13, 1
        add r14, r13
    cmp r14, 21
    sete r15b
    bool.206.12.end:
    func.assert.206.5:
        if.36.27.206.5:
        cmp.36.27.206.5:
        cmp r15b, 0
        jne if.36.24.206.5.end
        if.36.27.206.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.206.5.end:
    func.assert.206.5.end:
    cmp.207.12:
        mov r14, qword [rbp + 264]
        add r14, qword [rbp + 264]
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.207.12.end:
    func.assert.207.5:
        if.36.27.207.5:
        cmp.36.27.207.5:
        cmp r15b, 0
        jne if.36.24.207.5.end
        if.36.27.207.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.207.5.end:
    func.assert.207.5.end:
    cmp.210.12:
    cmp qword [rbp + 264], 0
    setne r15b
    je bool.210.12.end
    cmp.210.23:
    cmp.210.24:
    cmp qword [rbp + 264], 7
    setge r15b
    jge bool.210.12.end
    cmp.210.34:
    cmp qword [rbp + 264], 0
    setl r15b
    bool.210.12.end:
    func.assert.210.5:
        if.36.27.210.5:
        cmp.36.27.210.5:
        cmp r15b, 0
        jne if.36.24.210.5.end
        if.36.27.210.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.210.5.end:
    func.assert.210.5.end:
    mov byte [rbp + 272], 100
    mov r15b, byte [rbp + 272]
    add r15b, byte [rbp + 272]
    mov byte [rbp + 272], r15b
    cmp.215.12:
    cmp byte [rbp + 272], -56
    sete r15b
    bool.215.12.end:
    func.assert.215.5:
        if.36.27.215.5:
        cmp.36.27.215.5:
        cmp r15b, 0
        jne if.36.24.215.5.end
        if.36.27.215.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.215.5.end:
    func.assert.215.5.end:
    movsx r15, byte [rbp + 272]
    mov qword [rbp + 280], r15
    cmp.219.12:
    cmp qword [rbp + 280], -56
    sete r15b
    bool.219.12.end:
    func.assert.219.5:
        if.36.27.219.5:
        cmp.36.27.219.5:
        cmp r15b, 0
        jne if.36.24.219.5.end
        if.36.27.219.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.219.5.end:
    func.assert.219.5.end:
    mov qword [rbp + 288], 65
    cmp.227.12:
    cmp qword [rbp + 288], 65
    sete r15b
    bool.227.12.end:
    func.assert.227.5:
        if.36.27.227.5:
        cmp.36.27.227.5:
        cmp r15b, 0
        jne if.36.24.227.5.end
        if.36.27.227.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.227.5.end:
    func.assert.227.5.end:
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
    cmp.235.12:
    cmp dword [rbp + 300], 2
    sete r15b
    bool.235.12.end:
    func.assert.235.5:
        if.36.27.235.5:
        cmp.36.27.235.5:
        cmp r15b, 0
        jne if.36.24.235.5.end
        if.36.27.235.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.235.5.end:
    func.assert.235.5.end:
    cmp.236.12:
    cmp dword [rbp + 304], 2
    sete r15b
    bool.236.12.end:
    func.assert.236.5:
        if.36.27.236.5:
        cmp.36.27.236.5:
        cmp r15b, 0
        jne if.36.24.236.5.end
        if.36.27.236.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.236.5.end:
    func.assert.236.5.end:
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
    cmp.239.12:
    cmp dword [rbp + 296], 2
    sete r15b
    bool.239.12.end:
    func.assert.239.5:
        if.36.27.239.5:
        cmp.36.27.239.5:
        cmp r15b, 0
        jne if.36.24.239.5.end
        if.36.27.239.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.239.5.end:
    func.assert.239.5.end:
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
    cmp.244.14:
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
    bool.244.14.end:
    cmp.247.12:
    mov r15b, byte [rbp + 352]
    bool.247.12.end:
    func.assert.247.5:
        if.36.27.247.5:
        cmp.36.27.247.5:
        cmp r15b, 0
        jne if.36.24.247.5.end
        if.36.27.247.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.247.5.end:
    func.assert.247.5.end:
    mov dword [rbp + 328], -1
    cmp.250.12:
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
    bool.250.12.end:
    func.assert.250.5:
        if.36.27.250.5:
        cmp.36.27.250.5:
        cmp r15b, 0
        jne if.36.24.250.5.end
        if.36.27.250.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.250.5.end:
    func.assert.250.5.end:
    mov rax, qword [rbp + 296]
    mov qword [rbp + 356], rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 364], rax
    cmp.253.12:
        lea rsi, [rbp + 296]
        lea rdi, [rbp + 356]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.253.12.end:
    func.assert.253.5:
        if.36.27.253.5:
        cmp.36.27.253.5:
        cmp r15b, 0
        jne if.36.24.253.5.end
        if.36.27.253.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.253.5.end:
    func.assert.253.5.end:
    mov qword [rbp + 312], 3
    mov r15, qword [rbp + 312]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.261.16:
        mov r14d, dword [rbp + r15 * 4 + 296]
        mov dword [rbp + 372], r14d
        not dword [rbp + 372]
    func.inv.261.16.end:
    not dword [rbp + 372]
    mov r15, qword [rbp + 312]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 372]
    mov dword [rbp + r15 * 4 + 296], r14d
    cmp.263.12:
    mov r14, qword [rbp + 312]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 296], 2
    sete r15b
    bool.263.12.end:
    func.assert.263.5:
        if.36.27.263.5:
        cmp.36.27.263.5:
        cmp r15b, 0
        jne if.36.24.263.5.end
        if.36.27.263.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.263.5.end:
    func.assert.263.5.end:
    func.faz.265.5:
        mov dword [rbp + 300], 254
    func.faz.265.5.end:
    cmp.266.12:
    cmp dword [rbp + 300], 254
    sete r15b
    bool.266.12.end:
    func.assert.266.5:
        if.36.27.266.5:
        cmp.36.27.266.5:
        cmp r15b, 0
        jne if.36.24.266.5.end
        if.36.27.266.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.266.5.end:
    func.assert.266.5.end:
    mov qword [rbp + 376], 3
    mov qword [rbp + 384], 5
    lea r15, [rbp + 376]
    mov qword [rbp + 400], 0
    foo.269.5:
        mov r14, qword [rbp + 400]
        add qword [r15], r14
        add qword [r15], 2
        foo.269.5.continue:
            add r15, 8
            inc qword [rbp + 400]
            cmp qword [rbp + 400], 2
            jne foo.269.5
    foo.269.5.end:
    cmp.272.12:
    cmp qword [rbp + 376], 5
    sete r15b
    bool.272.12.end:
    func.assert.272.5:
        if.36.27.272.5:
        cmp.36.27.272.5:
        cmp r15b, 0
        jne if.36.24.272.5.end
        if.36.27.272.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.272.5.end:
    func.assert.272.5.end:
    cmp.273.12:
    cmp qword [rbp + 384], 8
    sete r15b
    bool.273.12.end:
    func.assert.273.5:
        if.36.27.273.5:
        cmp.36.27.273.5:
        cmp r15b, 0
        jne if.36.24.273.5.end
        if.36.27.273.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.273.5.end:
    func.assert.273.5.end:
    mov qword [rbp + 392], 0
    mov qword [rbp + 400], 0
    func.point.fooz.281.7:
        mov qword [rbp + 392], 2
        mov qword [rbp + 400], 11
    func.point.fooz.281.7.end:
    cmp.284.12:
    cmp qword [rbp + 392], 2
    sete r15b
    bool.284.12.end:
    func.assert.284.5:
        if.36.27.284.5:
        cmp.36.27.284.5:
        cmp r15b, 0
        jne if.36.24.284.5.end
        if.36.27.284.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.284.5.end:
    func.assert.284.5.end:
    cmp.285.12:
    cmp qword [rbp + 400], 11
    sete r15b
    bool.285.12.end:
    func.assert.285.5:
        if.36.27.285.5:
        cmp.36.27.285.5:
        cmp r15b, 0
        jne if.36.24.285.5.end
        if.36.27.285.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.285.5.end:
    func.assert.285.5.end:
    mov rax, qword [rbp + 392]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 400]
    mov qword [rbp + 416], rax
    cmp.290.12:
        lea rsi, [rbp + 392]
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    bool.290.12.end:
    func.assert.290.5:
        if.36.27.290.5:
        cmp.36.27.290.5:
        cmp r15b, 0
        jne if.36.24.290.5.end
        if.36.27.290.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.290.5.end:
    func.assert.290.5.end:
    mov qword [rbp + 408], 3
    cmp.295.12:
        lea rsi, [rbp + 392]
        lea rdi, [rbp + 408]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        setne r15b
    bool.295.12.end:
    func.assert.295.5:
        if.36.27.295.5:
        cmp.36.27.295.5:
        cmp r15b, 0
        jne if.36.24.295.5.end
        if.36.27.295.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.295.5.end:
    func.assert.295.5.end:
    mov qword [rbp + 424], 0
    func.bar.298.5:
        if.57.8.298.5:
        cmp.57.8.298.5:
        cmp qword [rbp + 424], 0
        je func.bar.298.5.end
        if.57.8.298.5.code:
        if.57.5.298.5.end:
        mov qword [rbp + 424], 255
    func.bar.298.5.end:
    cmp.299.12:
    cmp qword [rbp + 424], 0
    sete r15b
    bool.299.12.end:
    func.assert.299.5:
        if.36.27.299.5:
        cmp.36.27.299.5:
        cmp r15b, 0
        jne if.36.24.299.5.end
        if.36.27.299.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.299.5.end:
    func.assert.299.5.end:
    mov qword [rbp + 424], 1
    func.bar.302.5:
        if.57.8.302.5:
        cmp.57.8.302.5:
        cmp qword [rbp + 424], 0
        je func.bar.302.5.end
        if.57.8.302.5.code:
        if.57.5.302.5.end:
        mov qword [rbp + 424], 255
    func.bar.302.5.end:
    cmp.303.12:
    cmp qword [rbp + 424], 255
    sete r15b
    bool.303.12.end:
    func.assert.303.5:
        if.36.27.303.5:
        cmp.36.27.303.5:
        cmp r15b, 0
        jne if.36.24.303.5.end
        if.36.27.303.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.303.5.end:
    func.assert.303.5.end:
    mov qword [rbp + 432], 1
    func.baz.306.13:
        mov r15, qword [rbp + 432]
        mov qword [rbp + 440], r15
        sal qword [rbp + 440], 1
    func.baz.306.13.end:
    cmp.307.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.307.12.end:
    func.assert.307.5:
        if.36.27.307.5:
        cmp.36.27.307.5:
        cmp r15b, 0
        jne if.36.24.307.5.end
        if.36.27.307.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.307.5.end:
    func.assert.307.5.end:
    func.baz.309.9:
        mov qword [rbp + 440], 2
    func.baz.309.9.end:
    cmp.310.12:
    cmp qword [rbp + 440], 2
    sete r15b
    bool.310.12.end:
    func.assert.310.5:
        if.36.27.310.5:
        cmp.36.27.310.5:
        cmp r15b, 0
        jne if.36.24.310.5.end
        if.36.27.310.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.310.5.end:
    func.assert.310.5.end:
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
    cmp.314.12:
    cmp qword [rbp + 456], 120
    sete r15b
    bool.314.12.end:
    func.assert.314.5:
        if.36.27.314.5:
        cmp.36.27.314.5:
        cmp r15b, 0
        jne if.36.24.314.5.end
        if.36.27.314.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.314.5.end:
    func.assert.314.5.end:
    func.baz.316.20:
        mov qword [rbp + 464], 6
    func.baz.316.20.end:
    mov qword [rbp + 472], 0
    cmp.317.12:
    cmp qword [rbp + 464], 6
    sete r15b
    bool.317.12.end:
    func.assert.317.5:
        if.36.27.317.5:
        cmp.36.27.317.5:
        cmp r15b, 0
        jne if.36.24.317.5.end
        if.36.27.317.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.317.5.end:
    func.assert.317.5.end:
    func.point.at.319.14:
        mov qword [rbp + 480], -1
        mov qword [rbp + 488], -2
    func.point.at.319.14.end:
    cmp.323.12:
    cmp qword [rbp + 480], -1
    sete r15b
    bool.323.12.end:
    func.assert.323.5:
        if.36.27.323.5:
        cmp.36.27.323.5:
        cmp r15b, 0
        jne if.36.24.323.5.end
        if.36.27.323.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.323.5.end:
    func.assert.323.5.end:
    cmp.324.12:
    cmp qword [rbp + 488], -2
    sete r15b
    bool.324.12.end:
    func.assert.324.5:
        if.36.27.324.5:
        cmp.36.27.324.5:
        cmp r15b, 0
        jne if.36.24.324.5.end
        if.36.27.324.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.324.5.end:
    func.assert.324.5.end:
    func.point.x.326.8:
        mov qword [rbp + 480], 2
    func.point.x.326.8.end:
    cmp.327.12:
    cmp qword [rbp + 480], 2
    sete r15b
    bool.327.12.end:
    func.assert.327.5:
        if.36.27.327.5:
        cmp.36.27.327.5:
        cmp r15b, 0
        jne if.36.24.327.5.end
        if.36.27.327.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.327.5.end:
    func.assert.327.5.end:
    cmp.328.12:
        func.point.sum.328.15:
            mov r14, qword [rbp + 480]
            add r14, qword [rbp + 488]
        func.point.sum.328.15.end:
    cmp r14, 0
    sete r15b
    bool.328.12.end:
    func.assert.328.5:
        if.36.27.328.5:
        cmp.36.27.328.5:
        cmp r15b, 0
        jne if.36.24.328.5.end
        if.36.27.328.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.328.5.end:
    func.assert.328.5.end:
    mov qword [rbp + 496], 1
    mov qword [rbp + 504], 2
    mov r15, qword [rbp + 496]
    imul r15, 10
    mov qword [rbp + 512], r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 520], r15
    mov dword [rbp + 528], 16711680
    mov dword [rbp + 532], 0
    cmp.334.12:
    cmp qword [rbp + 512], 10
    sete r15b
    bool.334.12.end:
    func.assert.334.5:
        if.36.27.334.5:
        cmp.36.27.334.5:
        cmp r15b, 0
        jne if.36.24.334.5.end
        if.36.27.334.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.334.5.end:
    func.assert.334.5.end:
    cmp.335.12:
    cmp qword [rbp + 520], 2
    sete r15b
    bool.335.12.end:
    func.assert.335.5:
        if.36.27.335.5:
        cmp.36.27.335.5:
        cmp r15b, 0
        jne if.36.24.335.5.end
        if.36.27.335.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.335.5.end:
    func.assert.335.5.end:
    cmp.336.12:
    cmp dword [rbp + 528], 16711680
    sete r15b
    bool.336.12.end:
    func.assert.336.5:
        if.36.27.336.5:
        cmp.36.27.336.5:
        cmp r15b, 0
        jne if.36.24.336.5.end
        if.36.27.336.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.336.5.end:
    func.assert.336.5.end:
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
    cmp.340.12:
    cmp qword [rbp + 512], -1
    sete r15b
    bool.340.12.end:
    func.assert.340.5:
        if.36.27.340.5:
        cmp.36.27.340.5:
        cmp r15b, 0
        jne if.36.24.340.5.end
        if.36.27.340.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.340.5.end:
    func.assert.340.5.end:
    cmp.341.12:
    cmp qword [rbp + 520], -2
    sete r15b
    bool.341.12.end:
    func.assert.341.5:
        if.36.27.341.5:
        cmp.36.27.341.5:
        cmp r15b, 0
        jne if.36.24.341.5.end
        if.36.27.341.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.341.5.end:
    func.assert.341.5.end:
    lea rsi, [rbp + 512]
    lea rdi, [rbp + 552]
    mov rcx, 24
    rep movsb
    cmp.344.12:
    cmp qword [rbp + 552], -1
    sete r15b
    bool.344.12.end:
    func.assert.344.5:
        if.36.27.344.5:
        cmp.36.27.344.5:
        cmp r15b, 0
        jne if.36.24.344.5.end
        if.36.27.344.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.344.5.end:
    func.assert.344.5.end:
    cmp.345.12:
    cmp qword [rbp + 560], -2
    sete r15b
    bool.345.12.end:
    func.assert.345.5:
        if.36.27.345.5:
        cmp.36.27.345.5:
        cmp r15b, 0
        jne if.36.24.345.5.end
        if.36.27.345.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.345.5.end:
    func.assert.345.5.end:
    cmp.346.12:
    cmp dword [rbp + 568], 16711680
    sete r15b
    bool.346.12.end:
    func.assert.346.5:
        if.36.27.346.5:
        cmp.36.27.346.5:
        cmp r15b, 0
        jne if.36.24.346.5.end
        if.36.27.346.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.346.5.end:
    func.assert.346.5.end:
    mov r15, qword [rbp + 496]
    mov qword [rbp + 552], r15
    mov r15, qword [rbp + 504]
    mov qword [rbp + 560], r15
    cmp.349.12:
    cmp qword [rbp + 552], 1
    sete r15b
    bool.349.12.end:
    func.assert.349.5:
        if.36.27.349.5:
        cmp.36.27.349.5:
        cmp r15b, 0
        jne if.36.24.349.5.end
        if.36.27.349.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.349.5.end:
    func.assert.349.5.end:
    mov r15, qword [rbp + 504]
    mov qword [rbp + 552], r15
    mov r15, qword [rbp + 496]
    mov qword [rbp + 560], r15
    cmp.351.12:
    cmp qword [rbp + 552], 2
    sete r15b
    bool.351.12.end:
    func.assert.351.5:
        if.36.27.351.5:
        cmp.36.27.351.5:
        cmp r15b, 0
        jne if.36.24.351.5.end
        if.36.27.351.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.351.5.end:
    func.assert.351.5.end:
    xor al, al
    lea rdi, [rbp + 576]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 584], 73
    cmp.361.12:
    cmp qword [rbp + 584], 73
    sete r15b
    bool.361.12.end:
    func.assert.361.5:
        if.36.27.361.5:
        cmp.36.27.361.5:
        cmp r15b, 0
        jne if.36.24.361.5.end
        if.36.27.361.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.361.5.end:
    func.assert.361.5.end:
    func.object.at.362.13:
        func.point.at.113.16.362.13:
            mov qword [rbp + 600], 2
            mov qword [rbp + 608], 74
        func.point.at.113.16.362.13.end:
        mov dword [rbp + 616], 16777215
    func.object.at.362.13.end:
    cmp.363.12:
    cmp qword [rbp + 608], 74
    sete r15b
    bool.363.12.end:
    func.assert.363.5:
        if.36.27.363.5:
        cmp.36.27.363.5:
        cmp r15b, 0
        jne if.36.24.363.5.end
        if.36.27.363.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.363.5.end:
    func.assert.363.5.end:
    func.point.fooz.365.15:
        mov qword [rbp + 600], 2
        mov qword [rbp + 608], 11
    func.point.fooz.365.15.end:
    cmp.366.12:
        func.point.sum.366.22:
            mov r14, qword [rbp + 600]
            add r14, qword [rbp + 608]
        func.point.sum.366.22.end:
    cmp r14, 13
    sete r15b
    bool.366.12.end:
    func.assert.366.5:
        if.36.27.366.5:
        cmp.36.27.366.5:
        cmp r15b, 0
        jne if.36.24.366.5.end
        if.36.27.366.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.366.5.end:
    func.assert.366.5.end:
    xor al, al
    lea rdi, [rbp + 624]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 696], 65518
    cmp.371.12:
    cmp qword [rbp + 696], 65518
    sete r15b
    bool.371.12.end:
    func.assert.371.5:
        if.36.27.371.5:
        cmp.36.27.371.5:
        cmp r15b, 0
        jne if.36.24.371.5.end
        if.36.27.371.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.371.5.end:
    func.assert.371.5.end:
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
    cmp.380.12:
    cmp qword [rbp + 632], 65518
    sete r15b
    bool.380.12.end:
    func.assert.380.5:
        if.36.27.380.5:
        cmp.36.27.380.5:
        cmp r15b, 0
        jne if.36.24.380.5.end
        if.36.27.380.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.380.5.end:
    func.assert.380.5.end:
    cmp.381.12:
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
    bool.381.12.end:
    func.assert.381.5:
        if.36.27.381.5:
        cmp.36.27.381.5:
        cmp r15b, 0
        jne if.36.24.381.5.end
        if.36.27.381.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.381.5.end:
    func.assert.381.5.end:
    mov qword [rbp + 1136], -1
    mov qword [rbp + 1144], 2
    cmp.388.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.388.12.end:
    func.assert.388.5:
        if.36.27.388.5:
        cmp.36.27.388.5:
        cmp r15b, 0
        jne if.36.24.388.5.end
        if.36.27.388.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.388.5.end:
    func.assert.388.5.end:
    cmp.389.12:
    cmp qword [rbp + 1136], -1
    sete r15b
    bool.389.12.end:
    func.assert.389.5:
        if.36.27.389.5:
        cmp.36.27.389.5:
        cmp r15b, 0
        jne if.36.24.389.5.end
        if.36.27.389.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.389.5.end:
    func.assert.389.5.end:
    cmp.390.12:
    cmp qword [rbp + 1144], 2
    sete r15b
    bool.390.12.end:
    func.assert.390.5:
        if.36.27.390.5:
        cmp.36.27.390.5:
        cmp r15b, 0
        jne if.36.24.390.5.end
        if.36.27.390.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.36.24.390.5.end:
    func.assert.390.5.end:
    mov qword [rbp + 1152], 0
    xor al, al
    lea rdi, [rbp + 1160]
    mov rcx, 128
    rep stosb
    func.print.394.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.394.5.end:
    loop.395.5:
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
        func.print.398.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.398.9.end:
        func.print.399.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.399.9.end:
        func.str.input.400.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1161]
            mov rax, 0
            syscall
            mov qword [rbp + 1288], rax
            mov r15b, byte [rbp + 1288]
            mov byte [rbp + 1160], r15b
            sub byte [rbp + 1160], 1
        func.str.input.400.12.end:
        if.402.12:
        cmp.402.12:
        cmp byte [rbp + 1160], 0
        jle loop.395.5.end
        if.402.12.code:
        if.404.19:
        cmp.404.19:
        cmp byte [rbp + 1160], 4
        jg if.402.9.else
        if.404.19.code:
            func.print.405.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.405.13.end:
            jmp loop.395.5
        if.402.9.else:
            func.greet.408.13:
                func.print.93.5.408.13:
                    mov rdi, 1
                    mov rdx, 6
                    lea rsi, [rbp + 53]
                    mov rax, 1
                    syscall
                func.print.93.5.408.13.end:
                func.str.print.94.10.408.13:
                    mov rdi, 1
                    movsx rdx, byte [rbp + 1160]
                    test rdx, rdx
                    js baz_bounds_panic
                    cmp rdx, 127
                    jg baz_bounds_panic
                    lea rsi, [rbp + 1161]
                    mov rax, 1
                    syscall
                func.str.print.94.10.408.13.end:
                func.print.95.5.408.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 59]
                    mov rax, 1
                    syscall
                func.print.95.5.408.13.end:
                func.print.96.5.408.13:
                    mov rdi, 1
                    mov rdx, 1
                    lea rsi, [rbp + 60]
                    mov rax, 1
                    syscall
                func.print.96.5.408.13.end:
                add qword [rbp + 240], 1
            func.greet.408.13.end:
        if.402.9.end:
    jmp loop.395.5
    loop.395.5.end:
    func.print.412.5:
        mov rdi, 1
        mov rdx, 15
        lea rsi, [rbp + 224]
        mov rax, 1
        syscall
    func.print.412.5.end:
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
    func.print.414.5:
        mov rdi, 1
        mov rdx, 1
        lea rsi, [rbp + 60]
        mov rax, 1
        syscall
    func.print.414.5.end:
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
    if.136.8:
    cmp.136.8:
    cmp qword [rbx + 32], 0
    jge if.136.5.end
    if.136.8.code:
        mov byte [rbx + 40], 1
    if.136.5.end:
    if.139.8:
    cmp.139.8:
    cmp qword [rbx + 32], 0
    jle if.139.5.end
    if.139.8.code:
        neg qword [rbx + 32]
    if.139.5.end:
    mov qword [rbx + 48], 20
    loop.144.5:
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
        if.148.12:
        cmp.148.12:
        cmp qword [rbx + 32], 0
        jne loop.144.5
        if.148.12.code:
        if.148.9.end:
    loop.144.5.end:
    if.151.8:
    cmp.151.8:
    cmp byte [rbx + 40], 0
    je if.151.5.end
    if.151.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.151.5.end:
    mov qword [rbx + 56], 0
    loop.157.5:
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
        if.161.12:
        cmp.161.12:
        cmp qword [rbx + 48], 20
        jne loop.157.5
        if.161.12.code:
        if.161.9.end:
    loop.157.5.end:
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
    if.169.8:
    cmp.169.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.169.5.end
    if.169.8.code:
        ret
    if.169.5.end:
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
