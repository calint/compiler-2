default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 224], 0
    cmp.160.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.160.12.end:
    func.assert.160.5:
        if.32.27.160.5:
        cmp.32.27.160.5:
        cmp r15b, 0
        jne if.32.24.160.5.end
        if.32.27.160.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.160.5.end:
    func.assert.160.5.end:
    mov qword [rbp + 224], -1
    cmp.163.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.163.12.end:
    func.assert.163.5:
        if.32.27.163.5:
        cmp.32.27.163.5:
        cmp r15b, 0
        jne if.32.24.163.5.end
        if.32.27.163.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.163.5.end:
    func.assert.163.5.end:
        func.assert.169.9:
            if.32.27.169.9:
            cmp.32.27.169.9:
            if.32.24.169.9.end:
        func.assert.169.9.end:
    func.assert.172.5:
        if.32.27.172.5:
        cmp.32.27.172.5:
        if.32.24.172.5.end:
    func.assert.172.5.end:
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 232], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 232]
    mov dword [rbp + r15 * 4 + 232], r13d
    cmp.182.12:
    cmp dword [rbp + 236], 2
    sete r15b
    bool.182.12.end:
    func.assert.182.5:
        if.32.27.182.5:
        cmp.32.27.182.5:
        cmp r15b, 0
        jne if.32.24.182.5.end
        if.32.27.182.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.182.5.end:
    func.assert.182.5.end:
    cmp.183.12:
    cmp dword [rbp + 240], 2
    sete r15b
    bool.183.12.end:
    func.assert.183.5:
        if.32.27.183.5:
        cmp.32.27.183.5:
        cmp r15b, 0
        jne if.32.24.183.5.end
        if.32.27.183.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.183.5.end:
    func.assert.183.5.end:
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
    mov rax, qword [rbp + r14 * 4 + 232]
    mov qword [rbp + 232], rax
    cmp.186.12:
    cmp dword [rbp + 232], 2
    sete r15b
    bool.186.12.end:
    func.assert.186.5:
        if.32.27.186.5:
        cmp.32.27.186.5:
        cmp r15b, 0
        jne if.32.24.186.5.end
        if.32.27.186.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.186.5.end:
    func.assert.186.5.end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov r15, 4
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jg baz_bounds_panic
    test r15, r15
    js baz_bounds_panic
    cmp r15, 8
    jg baz_bounds_panic
    mov rax, qword [rbp + 232]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 240]
    mov qword [rbp + 264], rax
    cmp.191.19:
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
        lea rsi, [rbp + r15 * 4 + 232]
        mov r15, 1
        test r15, r15
        js baz_bounds_panic
        test rcx, rcx
        js baz_bounds_panic
        mov r14, rcx
        add r14, r15
        cmp r14, 8
        jg baz_bounds_panic
        lea rdi, [rbp + r15 * 4 + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 288]
    bool.191.19.end:
    cmp.194.12:
    mov r15b, byte [rbp + 288]
    bool.194.12.end:
    func.assert.194.5:
        if.32.27.194.5:
        cmp.32.27.194.5:
        cmp r15b, 0
        jne if.32.24.194.5.end
        if.32.27.194.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.194.5.end:
    func.assert.194.5.end:
    mov dword [rbp + 264], -1
    cmp.197.12:
        mov rcx, 4
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 4
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.197.12.end:
    func.assert.197.5:
        if.32.27.197.5:
        cmp.32.27.197.5:
        cmp r15b, 0
        jne if.32.24.197.5.end
        if.32.27.197.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.197.5.end:
    func.assert.197.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.205.20:
        mov r14d, dword [rbp + r15 * 4 + 232]
        mov dword [rbp + 292], r14d
        not dword [rbp + 292]
    func.inv.205.20.end:
    not dword [rbp + 292]
    mov r15, qword [rbp + 248]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 292]
    mov dword [rbp + r15 * 4 + 232], r14d
    cmp.207.12:
    mov r14, qword [rbp + 248]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.207.12.end:
    func.assert.207.5:
        if.32.27.207.5:
        cmp.32.27.207.5:
        cmp r15b, 0
        jne if.32.24.207.5.end
        if.32.27.207.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.207.5.end:
    func.assert.207.5.end:
    func.faz.209.5:
        mov dword [rbp + 236], 254
    func.faz.209.5.end:
    cmp.210.12:
    cmp dword [rbp + 236], 254
    sete r15b
    bool.210.12.end:
    func.assert.210.5:
        if.32.27.210.5:
        cmp.32.27.210.5:
        cmp r15b, 0
        jne if.32.24.210.5.end
        if.32.27.210.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.210.5.end:
    func.assert.210.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.213.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.213.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.213.5
    foo.213.5.end:
    cmp.216.12:
    cmp qword [rbp + 296], 5
    sete r15b
    bool.216.12.end:
    func.assert.216.5:
        if.32.27.216.5:
        cmp.32.27.216.5:
        cmp r15b, 0
        jne if.32.24.216.5.end
        if.32.27.216.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.216.5.end:
    func.assert.216.5.end:
    cmp.217.12:
    cmp qword [rbp + 304], 8
    sete r15b
    bool.217.12.end:
    func.assert.217.5:
        if.32.27.217.5:
        cmp.32.27.217.5:
        cmp r15b, 0
        jne if.32.24.217.5.end
        if.32.27.217.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.217.5.end:
    func.assert.217.5.end:
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.226.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.226.7.end:
    cmp.229.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.229.12.end:
    func.assert.229.5:
        if.32.27.229.5:
        cmp.32.27.229.5:
        cmp r15b, 0
        jne if.32.24.229.5.end
        if.32.27.229.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.229.5.end:
    func.assert.229.5.end:
    cmp.230.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.230.12.end:
    func.assert.230.5:
        if.32.27.230.5:
        cmp.32.27.230.5:
        cmp r15b, 0
        jne if.32.24.230.5.end
        if.32.27.230.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.230.5.end:
    func.assert.230.5.end:
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.235.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.235.12.end:
    func.assert.235.5:
        if.32.27.235.5:
        cmp.32.27.235.5:
        cmp r15b, 0
        jne if.32.24.235.5.end
        if.32.27.235.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.235.5.end:
    func.assert.235.5.end:
    mov qword [rbp + 328], 3
    cmp.240.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        setne r15b
    bool.240.12.end:
    func.assert.240.5:
        if.32.27.240.5:
        cmp.32.27.240.5:
        cmp r15b, 0
        jne if.32.24.240.5.end
        if.32.27.240.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.240.5.end:
    func.assert.240.5.end:
    mov qword [rbp + 344], 0
    func.bar.243.5:
        if.55.8.243.5:
        cmp.55.8.243.5:
        cmp qword [rbp + 344], 0
        je func.bar.243.5.end
        if.55.8.243.5.code:
        if.55.5.243.5.end:
        mov qword [rbp + 344], 255
    func.bar.243.5.end:
    cmp.244.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.244.12.end:
    func.assert.244.5:
        if.32.27.244.5:
        cmp.32.27.244.5:
        cmp r15b, 0
        jne if.32.24.244.5.end
        if.32.27.244.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.244.5.end:
    func.assert.244.5.end:
    mov qword [rbp + 344], 1
    func.bar.247.5:
        if.55.8.247.5:
        cmp.55.8.247.5:
        cmp qword [rbp + 344], 0
        je func.bar.247.5.end
        if.55.8.247.5.code:
        if.55.5.247.5.end:
        mov qword [rbp + 344], 255
    func.bar.247.5.end:
    cmp.248.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.248.12.end:
    func.assert.248.5:
        if.32.27.248.5:
        cmp.32.27.248.5:
        cmp r15b, 0
        jne if.32.24.248.5.end
        if.32.27.248.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.248.5.end:
    func.assert.248.5.end:
    mov qword [rbp + 352], 1
    func.baz.251.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.251.13.end:
    cmp.252.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.252.12.end:
    func.assert.252.5:
        if.32.27.252.5:
        cmp.32.27.252.5:
        cmp r15b, 0
        jne if.32.24.252.5.end
        if.32.27.252.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.252.5.end:
    func.assert.252.5.end:
    func.baz.254.9:
        mov qword [rbp + 360], 2
    func.baz.254.9.end:
    cmp.255.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.255.12.end:
    func.assert.255.5:
        if.32.27.255.5:
        cmp.32.27.255.5:
        cmp r15b, 0
        jne if.32.24.255.5.end
        if.32.27.255.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.255.5.end:
    func.assert.255.5.end:
    func.baz.257.21:
        mov qword [rbp + 368], 6
    func.baz.257.21.end:
    mov qword [rbp + 376], 0
    cmp.258.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.258.12.end:
    func.assert.258.5:
        if.32.27.258.5:
        cmp.32.27.258.5:
        cmp r15b, 0
        jne if.32.24.258.5.end
        if.32.27.258.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.258.5.end:
    func.assert.258.5.end:
    func.point.at.260.14:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point.at.260.14.end:
    cmp.264.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.264.12.end:
    func.assert.264.5:
        if.32.27.264.5:
        cmp.32.27.264.5:
        cmp r15b, 0
        jne if.32.24.264.5.end
        if.32.27.264.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.264.5.end:
    func.assert.264.5.end:
    cmp.265.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.265.12.end:
    func.assert.265.5:
        if.32.27.265.5:
        cmp.32.27.265.5:
        cmp r15b, 0
        jne if.32.24.265.5.end
        if.32.27.265.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.265.5.end:
    func.assert.265.5.end:
    func.point.x.267.8:
        mov qword [rbp + 384], 2
    func.point.x.267.8.end:
    cmp.268.12:
    cmp qword [rbp + 384], 2
    sete r15b
    bool.268.12.end:
    func.assert.268.5:
        if.32.27.268.5:
        cmp.32.27.268.5:
        cmp r15b, 0
        jne if.32.24.268.5.end
        if.32.27.268.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.268.5.end:
    func.assert.268.5.end:
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.274.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.274.12.end:
    func.assert.274.5:
        if.32.27.274.5:
        cmp.32.27.274.5:
        cmp r15b, 0
        jne if.32.24.274.5.end
        if.32.27.274.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.274.5.end:
    func.assert.274.5.end:
    cmp.275.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.275.12.end:
    func.assert.275.5:
        if.32.27.275.5:
        cmp.32.27.275.5:
        cmp r15b, 0
        jne if.32.24.275.5.end
        if.32.27.275.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.275.5.end:
    func.assert.275.5.end:
    cmp.276.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.276.12.end:
    func.assert.276.5:
        if.32.27.276.5:
        cmp.32.27.276.5:
        cmp r15b, 0
        jne if.32.24.276.5.end
        if.32.27.276.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.276.5.end:
    func.assert.276.5.end:
    mov r15, qword [rbp + 400]
    mov qword [rbp + 440], r15
    neg qword [rbp + 440]
    mov r15, qword [rbp + 408]
    mov qword [rbp + 448], r15
    neg qword [rbp + 448]
    mov rax, qword [rbp + 440]
    mov qword [rbp + 416], rax
    mov rax, qword [rbp + 448]
    mov qword [rbp + 424], rax
    cmp.280.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.280.12.end:
    func.assert.280.5:
        if.32.27.280.5:
        cmp.32.27.280.5:
        cmp r15b, 0
        jne if.32.24.280.5.end
        if.32.27.280.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.280.5.end:
    func.assert.280.5.end:
    cmp.281.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.281.12.end:
    func.assert.281.5:
        if.32.27.281.5:
        cmp.32.27.281.5:
        cmp r15b, 0
        jne if.32.24.281.5.end
        if.32.27.281.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.281.5.end:
    func.assert.281.5.end:
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.284.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.284.12.end:
    func.assert.284.5:
        if.32.27.284.5:
        cmp.32.27.284.5:
        cmp r15b, 0
        jne if.32.24.284.5.end
        if.32.27.284.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.284.5.end:
    func.assert.284.5.end:
    cmp.285.12:
    cmp qword [rbp + 464], -2
    sete r15b
    bool.285.12.end:
    func.assert.285.5:
        if.32.27.285.5:
        cmp.32.27.285.5:
        cmp r15b, 0
        jne if.32.24.285.5.end
        if.32.27.285.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.285.5.end:
    func.assert.285.5.end:
    cmp.286.12:
    cmp dword [rbp + 472], 16711680
    sete r15b
    bool.286.12.end:
    func.assert.286.5:
        if.32.27.286.5:
        cmp.32.27.286.5:
        cmp r15b, 0
        jne if.32.24.286.5.end
        if.32.27.286.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.286.5.end:
    func.assert.286.5.end:
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.291.12:
    cmp qword [rbp + 488], 73
    sete r15b
    bool.291.12.end:
    func.assert.291.5:
        if.32.27.291.5:
        cmp.32.27.291.5:
        cmp r15b, 0
        jne if.32.24.291.5.end
        if.32.27.291.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.291.5.end:
    func.assert.291.5.end:
    func.object.at.292.13:
        func.point.at.102.16.292.13:
            mov qword [rbp + 504], 2
            mov qword [rbp + 512], 74
        func.point.at.102.16.292.13.end:
        mov dword [rbp + 520], 16777215
    func.object.at.292.13.end:
    cmp.293.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.293.12.end:
    func.assert.293.5:
        if.32.27.293.5:
        cmp.32.27.293.5:
        cmp r15b, 0
        jne if.32.24.293.5.end
        if.32.27.293.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.293.5.end:
    func.assert.293.5.end:
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 600], 65518
    cmp.297.12:
    cmp qword [rbp + 600], 65518
    sete r15b
    bool.297.12.end:
    func.assert.297.5:
        if.32.27.297.5:
        cmp.32.27.297.5:
        cmp r15b, 0
        jne if.32.24.297.5.end
        if.32.27.297.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.297.5.end:
    func.assert.297.5.end:
    mov rcx, 8
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rsi, [rbp + 592]
    test rcx, rcx
    js baz_bounds_panic
    cmp rcx, 8
    jg baz_bounds_panic
    lea rdi, [rbp + 528]
    shl rcx, 3
    rep movsb
    cmp.306.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.306.12.end:
    func.assert.306.5:
        if.32.27.306.5:
        cmp.32.27.306.5:
        cmp r15b, 0
        jne if.32.24.306.5.end
        if.32.27.306.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.306.5.end:
    func.assert.306.5.end:
    cmp.307.12:
        mov rcx, 8
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rsi, [rbp + 528]
        test rcx, rcx
        js baz_bounds_panic
        cmp rcx, 8
        jg baz_bounds_panic
        lea rdi, [rbp + 592]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.307.12.end:
    func.assert.307.5:
        if.32.27.307.5:
        cmp.32.27.307.5:
        cmp r15b, 0
        jne if.32.24.307.5.end
        if.32.27.307.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.307.5.end:
    func.assert.307.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.313.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.313.12.end:
    func.assert.313.5:
        if.32.27.313.5:
        cmp.32.27.313.5:
        cmp r15b, 0
        jne if.32.24.313.5.end
        if.32.27.313.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.313.5.end:
    func.assert.313.5.end:
    cmp.314.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.314.12.end:
    func.assert.314.5:
        if.32.27.314.5:
        cmp.32.27.314.5:
        cmp r15b, 0
        jne if.32.24.314.5.end
        if.32.27.314.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.314.5.end:
    func.assert.314.5.end:
    cmp.315.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.315.12.end:
    func.assert.315.5:
        if.32.27.315.5:
        cmp.32.27.315.5:
        cmp r15b, 0
        jne if.32.24.315.5.end
        if.32.27.315.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.315.5.end:
    func.assert.315.5.end:
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.319.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.319.5.end:
    loop.320.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1192]
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
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        lea rbx, [rbp + 1192]
        call func.print_num
        func.print.323.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.323.9.end:
        func.print.324.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.324.9.end:
        func.str.input.325.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.325.12.end:
        if.327.12:
        cmp.327.12:
        cmp byte [rbp + 1064], 0
        jle loop.320.5.end
        if.327.12.code:
        if.329.19:
        cmp.329.19:
        cmp byte [rbp + 1064], 4
        jg if.327.9.else
        if.329.19.code:
            func.print.330.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.330.13.end:
            jmp loop.320.5
        if.327.9.else:
            func.print.333.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.333.13.end:
            func.str.output.334.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 1064]
                test rdx, rdx
                js baz_bounds_panic
                cmp rdx, 127
                jg baz_bounds_panic
                lea rsi, [rbp + 1065]
                mov rax, 1
                syscall
            func.str.output.334.16.end:
            func.print.335.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.335.13.end:
            func.print.336.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.336.13.end:
        if.327.9.end:
    jmp loop.320.5
    loop.320.5.end:
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
    if.126.8:
    cmp.126.8:
    cmp qword [rbx + 32], 0
    jge if.126.5.end
    if.126.8.code:
        mov byte [rbx + 40], 1
    if.126.5.end:
    if.129.8:
    cmp.129.8:
    cmp qword [rbx + 32], 0
    jle if.129.5.end
    if.129.8.code:
        neg qword [rbx + 32]
    if.129.5.end:
    mov qword [rbx + 48], 20
    loop.134.5:
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
        if.138.12:
        cmp.138.12:
        cmp qword [rbx + 32], 0
        jne loop.134.5
        if.138.12.code:
        if.138.9.end:
    loop.134.5.end:
    if.141.8:
    cmp.141.8:
    cmp byte [rbx + 40], 0
    je if.141.5.end
    if.141.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.141.5.end:
    mov qword [rbx + 56], 0
    loop.147.5:
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
        if.151.12:
        cmp.151.12:
        cmp qword [rbx + 48], 20
        jne loop.147.5
        if.151.12.code:
        if.151.9.end:
    loop.147.5.end:
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
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 65536
vars.end:
