default rel
%macro PUSH_REGS 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro POP_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    cmp.155.12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool.155.12.end:
    func.assert.155.5:
        if.32.27.155.5:
        cmp.32.27.155.5:
        cmp r15b, 0
        jne if.32.24.155.5.end
        if.32.27.155.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.155.5.end:
    func.assert.155.5.end:
    mov qword [rbp + 240], -1
    cmp.159.12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool.159.12.end:
    func.assert.159.5:
        if.32.27.159.5:
        cmp.32.27.159.5:
        cmp r15b, 0
        jne if.32.24.159.5.end
        if.32.27.159.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.159.5.end:
    func.assert.159.5.end:
        cmp.165.16:
        bool.165.16.end:
        mov r15b, 1
        func.assert.165.9:
            if.32.27.165.9:
            cmp.32.27.165.9:
            cmp r15b, 0
            jne if.32.24.165.9.end
            if.32.27.165.9.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.32.24.165.9.end:
        func.assert.165.9.end:
    cmp.168.12:
    bool.168.12.end:
    mov r15b, 1
    func.assert.168.5:
        if.32.27.168.5:
        cmp.32.27.168.5:
        cmp r15b, 0
        jne if.32.24.168.5.end
        if.32.27.168.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.168.5.end:
    func.assert.168.5.end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, qword [rbp + 248]
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp.175.12:
    mov r14, 1
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.175.12.end:
    func.assert.175.5:
        if.32.27.175.5:
        cmp.32.27.175.5:
        cmp r15b, 0
        jne if.32.24.175.5.end
        if.32.27.175.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.175.5.end:
    func.assert.175.5.end:
    cmp.176.12:
    mov r14, 2
    cmp dword [rbp + r14 * 4 + 224], 2
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
    mov r15, 2
    mov rax, qword [rbp + r15 * 4 + 224]
    mov qword [rbp + 224], rax
    cmp.179.12:
    mov r14, 0
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.179.12.end:
    func.assert.179.5:
        if.32.27.179.5:
        cmp.32.27.179.5:
        cmp r15b, 0
        jne if.32.24.179.5.end
        if.32.27.179.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.179.5.end:
    func.assert.179.5.end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rax, qword [rbp + 224]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 232]
    mov qword [rbp + 264], rax
    cmp.184.19:
        mov rcx, 3
        mov r15, 1
        lea rsi, [rbp + r15 * 4 + 224]
        mov r15, 1
        lea rdi, [rbp + r15 * 4 + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        sete byte [rbp + 288]
    bool.184.19.end:
    cmp.187.12:
    cmp byte [rbp + 288], 0
    setne r15b
    bool.187.12.end:
    func.assert.187.5:
        if.32.27.187.5:
        cmp.32.27.187.5:
        cmp r15b, 0
        jne if.32.24.187.5.end
        if.32.27.187.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.187.5.end:
    func.assert.187.5.end:
    mov r15, 2
    mov dword [rbp + r15 * 4 + 256], -1
    cmp.190.12:
        mov rcx, 4
        lea rsi, [rbp + 224]
        lea rdi, [rbp + 256]
        shl rcx, 2
        test rcx, rcx
        repe cmpsb
        setne r15b
    bool.190.12.end:
    func.assert.190.5:
        if.32.27.190.5:
        cmp.32.27.190.5:
        cmp r15b, 0
        jne if.32.24.190.5.end
        if.32.27.190.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.190.5.end:
    func.assert.190.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, qword [rbp + 248]
    sub r14, 1
    func.inv.193.16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    func.inv.193.16.end:
    not dword [rbp + r15 * 4 + 224]
    cmp.194.12:
    mov r14, qword [rbp + 248]
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
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
    func.faz.196.5:
        mov r15, 1
        mov dword [rbp + r15 * 4 + 224], 254
    func.faz.196.5.end:
    cmp.197.12:
    mov r14, 1
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
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
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.200.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.200.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.200.5
    foo.200.5.end:
    cmp.203.12:
    mov r14, 0
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
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
    cmp.204.12:
    mov r14, 1
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
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
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.211.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.211.7.end:
    cmp.212.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.212.12.end:
    func.assert.212.5:
        if.32.27.212.5:
        cmp.32.27.212.5:
        cmp r15b, 0
        jne if.32.24.212.5.end
        if.32.27.212.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.212.5.end:
    func.assert.212.5.end:
    cmp.213.12:
    cmp qword [rbp + 320], 11
    sete r15b
    bool.213.12.end:
    func.assert.213.5:
        if.32.27.213.5:
        cmp.32.27.213.5:
        cmp r15b, 0
        jne if.32.24.213.5.end
        if.32.27.213.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.213.5.end:
    func.assert.213.5.end:
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.216.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
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
    mov qword [rbp + 328], 3
    cmp.221.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool.221.12.end:
    func.assert.221.5:
        if.32.27.221.5:
        cmp.32.27.221.5:
        cmp r15b, 0
        jne if.32.24.221.5.end
        if.32.27.221.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.221.5.end:
    func.assert.221.5.end:
    mov qword [rbp + 344], 0
    func.bar.224.5:
        if.55.8.224.5:
        cmp.55.8.224.5:
        cmp qword [rbp + 344], 0
        jne if.55.5.224.5.end
        if.55.8.224.5.code:
            jmp func.bar.224.5.end
        if.55.5.224.5.end:
        mov qword [rbp + 344], 255
    func.bar.224.5.end:
    cmp.225.12:
    cmp qword [rbp + 344], 0
    sete r15b
    bool.225.12.end:
    func.assert.225.5:
        if.32.27.225.5:
        cmp.32.27.225.5:
        cmp r15b, 0
        jne if.32.24.225.5.end
        if.32.27.225.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.225.5.end:
    func.assert.225.5.end:
    mov qword [rbp + 344], 1
    func.bar.228.5:
        if.55.8.228.5:
        cmp.55.8.228.5:
        cmp qword [rbp + 344], 0
        jne if.55.5.228.5.end
        if.55.8.228.5.code:
            jmp func.bar.228.5.end
        if.55.5.228.5.end:
        mov qword [rbp + 344], 255
    func.bar.228.5.end:
    cmp.229.12:
    cmp qword [rbp + 344], 255
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
    mov qword [rbp + 352], 1
    func.baz.232.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.232.13.end:
    cmp.233.12:
    cmp qword [rbp + 360], 2
    sete r15b
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
    func.baz.235.9:
        mov qword [rbp + 360], 1
        sal qword [rbp + 360], 1
    func.baz.235.9.end:
    cmp.236.12:
    cmp qword [rbp + 360], 2
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
    func.baz.238.21:
        mov qword [rbp + 368], 3
        sal qword [rbp + 368], 1
    func.baz.238.21.end:
    mov qword [rbp + 376], 0
    cmp.239.12:
    cmp qword [rbp + 368], 6
    sete r15b
    bool.239.12.end:
    func.assert.239.5:
        if.32.27.239.5:
        cmp.32.27.239.5:
        cmp r15b, 0
        jne if.32.24.239.5.end
        if.32.27.239.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.239.5.end:
    func.assert.239.5.end:
    func.point_init.241.20:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point_init.241.20.end:
    cmp.242.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.242.12.end:
    func.assert.242.5:
        if.32.27.242.5:
        cmp.32.27.242.5:
        cmp r15b, 0
        jne if.32.24.242.5.end
        if.32.27.242.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.242.5.end:
    func.assert.242.5.end:
    cmp.243.12:
    cmp qword [rbp + 392], -2
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
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.249.12:
    cmp qword [rbp + 416], 10
    sete r15b
    bool.249.12.end:
    func.assert.249.5:
        if.32.27.249.5:
        cmp.32.27.249.5:
        cmp r15b, 0
        jne if.32.24.249.5.end
        if.32.27.249.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.249.5.end:
    func.assert.249.5.end:
    cmp.250.12:
    cmp qword [rbp + 424], 2
    sete r15b
    bool.250.12.end:
    func.assert.250.5:
        if.32.27.250.5:
        cmp.32.27.250.5:
        cmp r15b, 0
        jne if.32.24.250.5.end
        if.32.27.250.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.250.5.end:
    func.assert.250.5.end:
    cmp.251.12:
    cmp dword [rbp + 432], 16711680
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
    cmp.255.12:
    cmp qword [rbp + 416], -1
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
    cmp.256.12:
    cmp qword [rbp + 424], -2
    sete r15b
    bool.256.12.end:
    func.assert.256.5:
        if.32.27.256.5:
        cmp.32.27.256.5:
        cmp r15b, 0
        jne if.32.24.256.5.end
        if.32.27.256.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.256.5.end:
    func.assert.256.5.end:
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.259.12:
    cmp qword [rbp + 456], -1
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
    cmp qword [rbp + 464], -2
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
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.267.12:
    mov r14, 0
    imul r14, 24
    cmp qword [rbp + r14 + 488], 73
    sete r15b
    bool.267.12.end:
    func.assert.267.5:
        if.32.27.267.5:
        cmp.32.27.267.5:
        cmp r15b, 0
        jne if.32.24.267.5.end
        if.32.27.267.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.267.5.end:
    func.assert.267.5.end:
    mov r15, 1
    imul r15, 24
    func.object_init.268.13:
        mov qword [rbp + r15 + 480], 2
        mov qword [rbp + r15 + 488], 74
        mov dword [rbp + r15 + 496], 16777215
    func.object_init.268.13.end:
    cmp.269.12:
    mov r14, 1
    imul r14, 24
    cmp qword [rbp + r14 + 488], 74
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
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov r15, 1
    shl r15, 6
    lea r15, [rbp + r15 + 528]
    mov r14, 1
    mov qword [r15 + r14 * 8], 65518
    cmp.273.12:
    mov r14, 1
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool.273.12.end:
    func.assert.273.5:
        if.32.27.273.5:
        cmp.32.27.273.5:
        cmp r15b, 0
        jne if.32.24.273.5.end
        if.32.27.273.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.273.5.end:
    func.assert.273.5.end:
    mov rcx, 8
    mov r15, 1
    shl r15, 6
    lea rsi, [rbp + r15 + 528]
    mov r15, 0
    shl r15, 6
    lea rdi, [rbp + r15 + 528]
    shl rcx, 3
    rep movsb
    cmp.282.12:
    mov r14, 0
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    cmp qword [r14 + r13 * 8], 65518
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
    cmp.283.12:
        mov rcx, 8
        mov r14, 0
        shl r14, 6
        lea rsi, [rbp + r14 + 528]
        mov r14, 1
        shl r14, 6
        lea rdi, [rbp + r14 + 528]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
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
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.289.12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool.289.12.end:
    func.assert.289.5:
        if.32.27.289.5:
        cmp.32.27.289.5:
        cmp r15b, 0
        jne if.32.24.289.5.end
        if.32.27.289.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.289.5.end:
    func.assert.289.5.end:
    cmp.290.12:
    mov r14, 0
    cmp qword [rbp + r14 * 8 + 1040], -1
    sete r15b
    bool.290.12.end:
    func.assert.290.5:
        if.32.27.290.5:
        cmp.32.27.290.5:
        cmp r15b, 0
        jne if.32.24.290.5.end
        if.32.27.290.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.290.5.end:
    func.assert.290.5.end:
    cmp.291.12:
    mov r14, 1
    cmp qword [rbp + r14 * 8 + 1040], 2
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
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.295.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.295.5.end:
    loop.296.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        PUSH_REGS
        lea rbx, [rbp + 1192]
        call func.print_num
        POP_REGS
        func.print.299.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.299.9.end:
        func.print.300.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.300.9.end:
        func.str.input.301.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.301.12.end:
        if.303.12:
        cmp.303.12:
        cmp byte [rbp + 1064], 0
        jg if.305.19
        if.303.12.code:
            jmp loop.296.5.end
        if.305.19:
        cmp.305.19:
        cmp byte [rbp + 1064], 4
        jg if.303.9.else
        if.305.19.code:
            func.print.306.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.306.13.end:
            jmp loop.296.5
        if.303.9.else:
            func.print.309.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.309.13.end:
            func.str.output.310.16:
                mov rdi, 1
                movsx rdx, byte [rbp + 1064]
                lea rsi, [rbp + 1065]
                mov rax, 1
                syscall
            func.str.output.310.16.end:
            func.print.311.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.311.13.end:
            func.print.312.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.312.13.end:
        if.303.9.end:
    jmp loop.296.5
    loop.296.5.end:
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
    if.119.8:
    cmp.119.8:
    cmp qword [rbx + 32], 0
    jge if.119.5.end
    if.119.8.code:
        mov byte [rbx + 40], 1
    if.119.5.end:
    if.122.8:
    cmp.122.8:
    cmp qword [rbx + 32], 0
    jle if.122.5.end
    if.122.8.code:
        neg qword [rbx + 32]
    if.122.5.end:
    mov qword [rbx + 48], 20
    loop.127.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
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
        if.131.12:
        cmp.131.12:
        cmp qword [rbx + 32], 0
        jne if.131.9.end
        if.131.12.code:
            jmp loop.127.5.end
        if.131.9.end:
    jmp loop.127.5
    loop.127.5.end:
    if.134.8:
    cmp.134.8:
    cmp byte [rbx + 40], 0
    je if.134.5.end
    if.134.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov byte [rbx + r15 + 8], 45
    if.134.5.end:
    mov qword [rbx + 56], 0
    loop.140.5:
        mov r15, qword [rbx + 56]
        mov r14, qword [rbx + 48]
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 56], 1
        add qword [rbx + 48], 1
        if.144.12:
        cmp.144.12:
        cmp qword [rbx + 48], 20
        jne if.144.9.end
        if.144.12.code:
            jmp loop.140.5.end
        if.144.9.end:
    jmp loop.140.5
    loop.140.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    lea rsi, [rbx + 8]
    mov rax, 1
    syscall
    ret
size.func.print_num equ 64
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
