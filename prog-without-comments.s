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
    mov r14, 174
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 232], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 175
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
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
    mov r13, 179
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
    mov r13, 179
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
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
    mov r14, 184
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    mov r14, 184
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    mov rax, qword [rbp + 232]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 240]
    mov qword [rbp + 264], rax
    cmp.185.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 185
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
        lea rsi, [rbp + r15 * 4 + 232]
        mov r15, 1
        mov r14, 185
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
        mov r14, 191
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
        mov r14, 191
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
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
    mov r14, 194
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 194
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.194.16:
        mov r13d, dword [rbp + r14 * 4 + 232]
        mov dword [rbp + r15 * 4 + 232], r13d
        not dword [rbp + r15 * 4 + 232]
    func.inv.194.16.end:
    not dword [rbp + r15 * 4 + 232]
    cmp.195.12:
    mov r14, qword [rbp + 248]
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.195.12.end:
    func.assert.195.5:
        if.32.27.195.5:
        cmp.32.27.195.5:
        cmp r15b, 0
        jne if.32.24.195.5.end
        if.32.27.195.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.195.5.end:
    func.assert.195.5.end:
    func.faz.197.5:
        mov dword [rbp + 236], 254
    func.faz.197.5.end:
    cmp.198.12:
    cmp dword [rbp + 236], 254
    sete r15b
    bool.198.12.end:
    func.assert.198.5:
        if.32.27.198.5:
        cmp.32.27.198.5:
        cmp r15b, 0
        jne if.32.24.198.5.end
        if.32.27.198.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.198.5.end:
    func.assert.198.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.201.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.201.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.201.5
    foo.201.5.end:
    cmp.204.12:
    cmp qword [rbp + 296], 5
    sete r15b
    bool.204.12.end:
    func.assert.204.5:
        if.32.27.204.5:
        cmp.32.27.204.5:
        cmp r15b, 0
        jne if.32.24.204.5.end
        if.32.27.204.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.204.5.end:
    func.assert.204.5.end:
    cmp.205.12:
    cmp qword [rbp + 304], 8
    sete r15b
    bool.205.12.end:
    func.assert.205.5:
        if.32.27.205.5:
        cmp.32.27.205.5:
        cmp r15b, 0
        jne if.32.24.205.5.end
        if.32.27.205.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.205.5.end:
    func.assert.205.5.end:
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.214.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.214.7.end:
    cmp.217.12:
    cmp qword [rbp + 312], 2
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
    cmp.218.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.218.12.end:
    func.assert.218.5:
        if.32.27.218.5:
        cmp.32.27.218.5:
        cmp r15b, 0
        jne if.32.24.218.5.end
        if.32.27.218.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.218.5.end:
    func.assert.218.5.end:
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.223.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.0
        cmpsq
        .Lbaz_equal.0:
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
    mov qword [rbp + 328], 3
    cmp.228.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        setne r15b
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
    mov qword [rbp + 344], 0
    func.bar.231.5:
        if.55.8.231.5:
        cmp.55.8.231.5:
        cmp qword [rbp + 344], 0
        je func.bar.231.5.end
        if.55.8.231.5.code:
        if.55.5.231.5.end:
        mov qword [rbp + 344], 255
    func.bar.231.5.end:
    cmp.232.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.232.12.end:
    func.assert.232.5:
        if.32.27.232.5:
        cmp.32.27.232.5:
        cmp r15b, 0
        jne if.32.24.232.5.end
        if.32.27.232.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.232.5.end:
    func.assert.232.5.end:
    mov qword [rbp + 344], 1
    func.bar.235.5:
        if.55.8.235.5:
        cmp.55.8.235.5:
        cmp qword [rbp + 344], 0
        je func.bar.235.5.end
        if.55.8.235.5.code:
        if.55.5.235.5.end:
        mov qword [rbp + 344], 255
    func.bar.235.5.end:
    cmp.236.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.236.12.end:
    func.assert.236.5:
        if.32.27.236.5:
        cmp.32.27.236.5:
        cmp r15b, 0
        jne if.32.24.236.5.end
        if.32.27.236.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.236.5.end:
    func.assert.236.5.end:
    mov qword [rbp + 352], 1
    func.baz.239.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.239.13.end:
    cmp.240.12:
    cmp qword [rbp + 360], 2
    sete r15b
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
    func.baz.242.9:
        mov qword [rbp + 360], 2
    func.baz.242.9.end:
    cmp.243.12:
    cmp qword [rbp + 360], 2
    sete r15b
    bool.243.12.end:
    func.assert.243.5:
        if.32.27.243.5:
        cmp.32.27.243.5:
        cmp r15b, 0
        jne if.32.24.243.5.end
        if.32.27.243.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.243.5.end:
    func.assert.243.5.end:
    func.baz.245.21:
        mov qword [rbp + 368], 6
    func.baz.245.21.end:
    mov qword [rbp + 376], 0
    cmp.246.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.246.12.end:
    func.assert.246.5:
        if.32.27.246.5:
        cmp.32.27.246.5:
        cmp r15b, 0
        jne if.32.24.246.5.end
        if.32.27.246.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.246.5.end:
    func.assert.246.5.end:
    func.point.at.248.14:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point.at.248.14.end:
    cmp.252.12:
    cmp qword [rbp + 384], -1
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
    cmp.253.12:
    cmp qword [rbp + 392], -2
    sete r15b
    bool.253.12.end:
    func.assert.253.5:
        if.32.27.253.5:
        cmp.32.27.253.5:
        cmp r15b, 0
        jne if.32.24.253.5.end
        if.32.27.253.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.253.5.end:
    func.assert.253.5.end:
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.259.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.259.12.end:
    func.assert.259.5:
        if.32.27.259.5:
        cmp.32.27.259.5:
        cmp r15b, 0
        jne if.32.24.259.5.end
        if.32.27.259.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.259.5.end:
    func.assert.259.5.end:
    cmp.260.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.260.12.end:
    func.assert.260.5:
        if.32.27.260.5:
        cmp.32.27.260.5:
        cmp r15b, 0
        jne if.32.24.260.5.end
        if.32.27.260.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.260.5.end:
    func.assert.260.5.end:
    cmp.261.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.261.12.end:
    func.assert.261.5:
        if.32.27.261.5:
        cmp.32.27.261.5:
        cmp r15b, 0
        jne if.32.24.261.5.end
        if.32.27.261.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.261.5.end:
    func.assert.261.5.end:
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
    cmp.265.12:
    cmp qword [rbp + 416], -1
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
    cmp qword [rbp + 424], -2
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
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.269.12:
    cmp qword [rbp + 456], -1
    sete r15b
    bool.269.12.end:
    func.assert.269.5:
        if.32.27.269.5:
        cmp.32.27.269.5:
        cmp r15b, 0
        jne if.32.24.269.5.end
        if.32.27.269.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.269.5.end:
    func.assert.269.5.end:
    cmp.270.12:
    cmp qword [rbp + 464], -2
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
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.276.12:
    cmp qword [rbp + 488], 73
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
    func.object.at.277.13:
        func.point.at.96.16.277.13:
            mov qword [rbp + 504], 2
            mov qword [rbp + 512], 74
        func.point.at.96.16.277.13.end:
        mov dword [rbp + 520], 16777215
    func.object.at.277.13.end:
    cmp.278.12:
    cmp qword [rbp + 512], 74
    sete r15b
    bool.278.12.end:
    func.assert.278.5:
        if.32.27.278.5:
        cmp.32.27.278.5:
        cmp r15b, 0
        jne if.32.24.278.5.end
        if.32.27.278.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.278.5.end:
    func.assert.278.5.end:
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 600], 65518
    cmp.282.12:
    cmp qword [rbp + 600], 65518
    sete r15b
    bool.282.12.end:
    func.assert.282.5:
        if.32.27.282.5:
        cmp.32.27.282.5:
        cmp r15b, 0
        jne if.32.24.282.5.end
        if.32.27.282.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.282.5.end:
    func.assert.282.5.end:
    mov rcx, 8
    mov r15, 285
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 592]
    mov r15, 286
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 528]
    shl rcx, 3
    rep movsb
    cmp.291.12:
    cmp qword [rbp + 536], 65518
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
    cmp.292.12:
        mov rcx, 8
        mov r14, 293
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 528]
        mov r14, 294
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rdi, [rbp + 592]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.292.12.end:
    func.assert.292.5:
        if.32.27.292.5:
        cmp.32.27.292.5:
        cmp r15b, 0
        jne if.32.24.292.5.end
        if.32.27.292.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.292.5.end:
    func.assert.292.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.298.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.298.12.end:
    func.assert.298.5:
        if.32.27.298.5:
        cmp.32.27.298.5:
        cmp r15b, 0
        jne if.32.24.298.5.end
        if.32.27.298.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.298.5.end:
    func.assert.298.5.end:
    cmp.299.12:
    cmp qword [rbp + 1040], -1
    sete r15b
    bool.299.12.end:
    func.assert.299.5:
        if.32.27.299.5:
        cmp.32.27.299.5:
        cmp r15b, 0
        jne if.32.24.299.5.end
        if.32.27.299.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.299.5.end:
    func.assert.299.5.end:
    cmp.300.12:
    cmp qword [rbp + 1048], 2
    sete r15b
    bool.300.12.end:
    func.assert.300.5:
        if.32.27.300.5:
        cmp.32.27.300.5:
        cmp r15b, 0
        jne if.32.24.300.5.end
        if.32.27.300.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.300.5.end:
    func.assert.300.5.end:
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.304.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.304.5.end:
    loop.305.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        lea rbx, [rbp + 1192]
        call func.print_num
        func.print.308.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.308.9.end:
        func.print.309.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.309.9.end:
        func.str.input.310.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.310.12.end:
        if.312.12:
        cmp.312.12:
        cmp byte [rbp + 1064], 0
        jle loop.305.5.end
        if.312.12.code:
        if.314.19:
        cmp.314.19:
        cmp byte [rbp + 1064], 4
        jg if.312.9.else
        if.314.19.code:
            func.print.315.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.315.13.end:
            jmp loop.305.5
        if.312.9.else:
            func.print.318.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.318.13.end:
            func.str.output.319.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 1064]
                mov r15, 84
                test rdx, rdx
                cmovs rbp, r15
                js baz_bounds_panic
                cmp rdx, 127
                cmovg rbp, r15
                jg baz_bounds_panic
                lea rsi, [rbp + 1065]
                mov rax, 1
                syscall
            func.str.output.319.16.end:
            func.print.320.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.320.13.end:
            func.print.321.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.321.13.end:
        if.312.9.end:
    jmp loop.305.5
    loop.305.5.end:
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
        mov r14, 130
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
        mov r14, 137
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.135.5.end:
    mov qword [rbx + 56], 0
    loop.141.5:
        mov r15, qword [rbx + 56]
        mov r14, 142
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        mov r13, 142
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
        if.145.12:
        cmp.145.12:
        cmp qword [rbx + 48], 20
        jne loop.141.5
        if.145.12.code:
        if.145.9.end:
    loop.141.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    mov r15, 148
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
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 131072
vars.end:
