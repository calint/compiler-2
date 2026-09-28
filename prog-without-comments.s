default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 224], 0
    cmp.154.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.154.12.end:
    func.assert.154.5:
        if.32.27.154.5:
        cmp.32.27.154.5:
        cmp r15b, 0
        jne if.32.24.154.5.end
        if.32.27.154.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.154.5.end:
    func.assert.154.5.end:
    mov qword [rbp + 224], -1
    cmp.157.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.157.12.end:
    func.assert.157.5:
        if.32.27.157.5:
        cmp.32.27.157.5:
        cmp r15b, 0
        jne if.32.24.157.5.end
        if.32.27.157.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.157.5.end:
    func.assert.157.5.end:
        func.assert.163.9:
            if.32.27.163.9:
            cmp.32.27.163.9:
            if.32.24.163.9.end:
        func.assert.163.9.end:
    func.assert.166.5:
        if.32.27.166.5:
        cmp.32.27.166.5:
        if.32.24.166.5.end:
    func.assert.166.5.end:
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
    cmp.176.12:
    cmp dword [rbp + 236], 2
    sete r15b
    bool.176.12.end:
    func.assert.176.5:
        if.32.27.176.5:
        cmp.32.27.176.5:
        cmp r15b, 0
        jne if.32.24.176.5.end
        if.32.27.176.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.176.5.end:
    func.assert.176.5.end:
    cmp.177.12:
    cmp dword [rbp + 240], 2
    sete r15b
    bool.177.12.end:
    func.assert.177.5:
        if.32.27.177.5:
        cmp.32.27.177.5:
        cmp r15b, 0
        jne if.32.24.177.5.end
        if.32.27.177.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.177.5.end:
    func.assert.177.5.end:
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
    cmp.180.12:
    cmp dword [rbp + 232], 2
    sete r15b
    bool.180.12.end:
    func.assert.180.5:
        if.32.27.180.5:
        cmp.32.27.180.5:
        cmp r15b, 0
        jne if.32.24.180.5.end
        if.32.27.180.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.180.5.end:
    func.assert.180.5.end:
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
    cmp.185.19:
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
    bool.185.19.end:
    cmp.188.12:
    mov r15b, byte [rbp + 288]
    bool.188.12.end:
    func.assert.188.5:
        if.32.27.188.5:
        cmp.32.27.188.5:
        cmp r15b, 0
        jne if.32.24.188.5.end
        if.32.27.188.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.188.5.end:
    func.assert.188.5.end:
    mov dword [rbp + 264], -1
    cmp.191.12:
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
    bool.191.12.end:
    func.assert.191.5:
        if.32.27.191.5:
        cmp.32.27.191.5:
        cmp r15b, 0
        jne if.32.24.191.5.end
        if.32.27.191.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.191.5.end:
    func.assert.191.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    sub r15, 1
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    func.inv.198.20:
        mov r14d, dword [rbp + r15 * 4 + 232]
        mov dword [rbp + 292], r14d
        not dword [rbp + 292]
    func.inv.198.20.end:
    not dword [rbp + 292]
    mov r15, qword [rbp + 248]
    test r15, r15
    js baz_bounds_panic
    cmp r15, 4
    jge baz_bounds_panic
    mov r14d, dword [rbp + 292]
    mov dword [rbp + r15 * 4 + 232], r14d
    cmp.200.12:
    mov r14, qword [rbp + 248]
    test r14, r14
    js baz_bounds_panic
    cmp r14, 4
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.200.12.end:
    func.assert.200.5:
        if.32.27.200.5:
        cmp.32.27.200.5:
        cmp r15b, 0
        jne if.32.24.200.5.end
        if.32.27.200.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.200.5.end:
    func.assert.200.5.end:
    func.faz.202.5:
        mov dword [rbp + 236], 254
    func.faz.202.5.end:
    cmp.203.12:
    cmp dword [rbp + 236], 254
    sete r15b
    bool.203.12.end:
    func.assert.203.5:
        if.32.27.203.5:
        cmp.32.27.203.5:
        cmp r15b, 0
        jne if.32.24.203.5.end
        if.32.27.203.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.203.5.end:
    func.assert.203.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.206.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.206.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.206.5
    foo.206.5.end:
    cmp.209.12:
    cmp qword [rbp + 296], 5
    sete r15b
    bool.209.12.end:
    func.assert.209.5:
        if.32.27.209.5:
        cmp.32.27.209.5:
        cmp r15b, 0
        jne if.32.24.209.5.end
        if.32.27.209.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.209.5.end:
    func.assert.209.5.end:
    cmp.210.12:
    cmp qword [rbp + 304], 8
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
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.219.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.219.7.end:
    cmp.222.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.222.12.end:
    func.assert.222.5:
        if.32.27.222.5:
        cmp.32.27.222.5:
        cmp r15b, 0
        jne if.32.24.222.5.end
        if.32.27.222.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.222.5.end:
    func.assert.222.5.end:
    cmp.223.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.223.12.end:
    func.assert.223.5:
        if.32.27.223.5:
        cmp.32.27.223.5:
        cmp r15b, 0
        jne if.32.24.223.5.end
        if.32.27.223.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.223.5.end:
    func.assert.223.5.end:
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.228.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
        sete r15b
    bool.228.12.end:
    func.assert.228.5:
        if.32.27.228.5:
        cmp.32.27.228.5:
        cmp r15b, 0
        jne if.32.24.228.5.end
        if.32.27.228.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.228.5.end:
    func.assert.228.5.end:
    mov qword [rbp + 328], 3
    cmp.233.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        setne r15b
    bool.233.12.end:
    func.assert.233.5:
        if.32.27.233.5:
        cmp.32.27.233.5:
        cmp r15b, 0
        jne if.32.24.233.5.end
        if.32.27.233.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.233.5.end:
    func.assert.233.5.end:
    mov qword [rbp + 344], 0
    func.bar.236.5:
        if.55.8.236.5:
        cmp.55.8.236.5:
        cmp qword [rbp + 344], 0
        je func.bar.236.5.end
        if.55.8.236.5.code:
        if.55.5.236.5.end:
        mov qword [rbp + 344], 255
    func.bar.236.5.end:
    cmp.237.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.237.12.end:
    func.assert.237.5:
        if.32.27.237.5:
        cmp.32.27.237.5:
        cmp r15b, 0
        jne if.32.24.237.5.end
        if.32.27.237.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.237.5.end:
    func.assert.237.5.end:
    mov qword [rbp + 344], 1
    func.bar.240.5:
        if.55.8.240.5:
        cmp.55.8.240.5:
        cmp qword [rbp + 344], 0
        je func.bar.240.5.end
        if.55.8.240.5.code:
        if.55.5.240.5.end:
        mov qword [rbp + 344], 255
    func.bar.240.5.end:
    cmp.241.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.241.12.end:
    func.assert.241.5:
        if.32.27.241.5:
        cmp.32.27.241.5:
        cmp r15b, 0
        jne if.32.24.241.5.end
        if.32.27.241.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.241.5.end:
    func.assert.241.5.end:
    mov qword [rbp + 352], 1
    func.baz.244.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.244.13.end:
    cmp.245.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.245.12.end:
    func.assert.245.5:
        if.32.27.245.5:
        cmp.32.27.245.5:
        cmp r15b, 0
        jne if.32.24.245.5.end
        if.32.27.245.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.245.5.end:
    func.assert.245.5.end:
    func.baz.247.9:
        mov qword [rbp + 360], 2
    func.baz.247.9.end:
    cmp.248.12:
    cmp qword [rbp + 360], 2
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
    func.baz.250.21:
        mov qword [rbp + 368], 6
    func.baz.250.21.end:
    mov qword [rbp + 376], 0
    cmp.251.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.251.12.end:
    func.assert.251.5:
        if.32.27.251.5:
        cmp.32.27.251.5:
        cmp r15b, 0
        jne if.32.24.251.5.end
        if.32.27.251.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.251.5.end:
    func.assert.251.5.end:
    func.point.at.253.14:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point.at.253.14.end:
    cmp.257.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.257.12.end:
    func.assert.257.5:
        if.32.27.257.5:
        cmp.32.27.257.5:
        cmp r15b, 0
        jne if.32.24.257.5.end
        if.32.27.257.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.257.5.end:
    func.assert.257.5.end:
    cmp.258.12:
    cmp qword [rbp + 392], -2
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
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.264.12:
    cmp qword [rbp + 416], 10
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
    cmp qword [rbp + 424], 2
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
    cmp.266.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.266.12.end:
    func.assert.266.5:
        if.32.27.266.5:
        cmp.32.27.266.5:
        cmp r15b, 0
        jne if.32.24.266.5.end
        if.32.27.266.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.266.5.end:
    func.assert.266.5.end:
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
    cmp.270.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.270.12.end:
    func.assert.270.5:
        if.32.27.270.5:
        cmp.32.27.270.5:
        cmp r15b, 0
        jne if.32.24.270.5.end
        if.32.27.270.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.270.5.end:
    func.assert.270.5.end:
    cmp.271.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.271.12.end:
    func.assert.271.5:
        if.32.27.271.5:
        cmp.32.27.271.5:
        cmp r15b, 0
        jne if.32.24.271.5.end
        if.32.27.271.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.271.5.end:
    func.assert.271.5.end:
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.274.12:
    cmp qword [rbp + 456], -1
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
    cmp qword [rbp + 464], -2
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
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.281.12:
    cmp qword [rbp + 488], 73
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
    func.object.at.282.13:
        func.point.at.96.16.282.13:
            mov qword [rbp + 504], 2
            mov qword [rbp + 512], 74
        func.point.at.96.16.282.13.end:
        mov dword [rbp + 520], 16777215
    func.object.at.282.13.end:
    cmp.283.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.283.12.end:
    func.assert.283.5:
        if.32.27.283.5:
        cmp.32.27.283.5:
        cmp r15b, 0
        jne if.32.24.283.5.end
        if.32.27.283.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.283.5.end:
    func.assert.283.5.end:
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 600], 65518
    cmp.287.12:
    cmp qword [rbp + 600], 65518
    sete r15b
    bool.287.12.end:
    func.assert.287.5:
        if.32.27.287.5:
        cmp.32.27.287.5:
        cmp r15b, 0
        jne if.32.24.287.5.end
        if.32.27.287.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.287.5.end:
    func.assert.287.5.end:
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
    cmp.296.12:
    cmp qword [rbp + 536], 65518
    sete r15b
    bool.296.12.end:
    func.assert.296.5:
        if.32.27.296.5:
        cmp.32.27.296.5:
        cmp r15b, 0
        jne if.32.24.296.5.end
        if.32.27.296.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.296.5.end:
    func.assert.296.5.end:
    cmp.297.12:
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
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.303.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.303.12.end:
    func.assert.303.5:
        if.32.27.303.5:
        cmp.32.27.303.5:
        cmp r15b, 0
        jne if.32.24.303.5.end
        if.32.27.303.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.303.5.end:
    func.assert.303.5.end:
    cmp.304.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.304.12.end:
    func.assert.304.5:
        if.32.27.304.5:
        cmp.32.27.304.5:
        cmp r15b, 0
        jne if.32.24.304.5.end
        if.32.27.304.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.304.5.end:
    func.assert.304.5.end:
    cmp.305.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.305.12.end:
    func.assert.305.5:
        if.32.27.305.5:
        cmp.32.27.305.5:
        cmp r15b, 0
        jne if.32.24.305.5.end
        if.32.27.305.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.305.5.end:
    func.assert.305.5.end:
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.309.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.309.5.end:
    loop.310.5:
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
        func.print.313.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.313.9.end:
        func.print.314.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.314.9.end:
        func.str.input.315.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.315.12.end:
        if.317.12:
        cmp.317.12:
        cmp byte [rbp + 1064], 0
        jle loop.310.5.end
        if.317.12.code:
        if.319.19:
        cmp.319.19:
        cmp byte [rbp + 1064], 4
        jg if.317.9.else
        if.319.19.code:
            func.print.320.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.320.13.end:
            jmp loop.310.5
        if.317.9.else:
            func.print.323.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.323.13.end:
            func.str.output.324.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 1064]
                test rdx, rdx
                js baz_bounds_panic
                cmp rdx, 127
                jg baz_bounds_panic
                lea rsi, [rbp + 1065]
                mov rax, 1
                syscall
            func.str.output.324.16.end:
            func.print.325.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.325.13.end:
            func.print.326.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.326.13.end:
        if.317.9.end:
    jmp loop.310.5
    loop.310.5.end:
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
    if.120.8:
    cmp.120.8:
    cmp qword [rbx + 32], 0
    jge if.120.5.end
    if.120.8.code:
        mov byte [rbx + 40], 1
    if.120.5.end:
    if.123.8:
    cmp.123.8:
    cmp qword [rbx + 32], 0
    jle if.123.5.end
    if.123.8.code:
        neg qword [rbx + 32]
    if.123.5.end:
    mov qword [rbx + 48], 20
    loop.128.5:
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
        if.132.12:
        cmp.132.12:
        cmp qword [rbx + 32], 0
        jne loop.128.5
        if.132.12.code:
        if.132.9.end:
    loop.128.5.end:
    if.135.8:
    cmp.135.8:
    cmp byte [rbx + 40], 0
    je if.135.5.end
    if.135.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        test r15, r15
        js baz_bounds_panic
        cmp r15, 20
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.135.5.end:
    mov qword [rbx + 56], 0
    loop.141.5:
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
        if.145.12:
        cmp.145.12:
        cmp qword [rbx + 48], 20
        jne loop.141.5
        if.145.12.code:
        if.145.9.end:
    loop.141.5.end:
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
resb 131072
vars.end:
