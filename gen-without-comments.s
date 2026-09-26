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
    cmp.151.12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool.151.12.end:
    func.assert.151.5:
        if.32.27.151.5:
        cmp.32.27.151.5:
        cmp r15b, 0
        jne if.32.24.151.5.end
        if.32.27.151.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.151.5.end:
    func.assert.151.5.end:
    mov qword [rbp + 240], -1
    cmp.155.12:
    cmp qword [rbp + 240], -1
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
        cmp.161.16:
        bool.161.16.end:
        mov r15b, 1
        func.assert.161.9:
            if.32.27.161.9:
            cmp.32.27.161.9:
            cmp r15b, 0
            jne if.32.24.161.9.end
            if.32.27.161.9.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.32.24.161.9.end:
        func.assert.161.9.end:
    cmp.164.12:
    bool.164.12.end:
    mov r15b, 1
    func.assert.164.5:
        if.32.27.164.5:
        cmp.32.27.164.5:
        cmp r15b, 0
        jne if.32.24.164.5.end
        if.32.27.164.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.164.5.end:
    func.assert.164.5.end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 170
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 170
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp.171.12:
    mov r14, 1
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.171.12.end:
    func.assert.171.5:
        if.32.27.171.5:
        cmp.32.27.171.5:
        cmp r15b, 0
        jne if.32.24.171.5.end
        if.32.27.171.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.171.5.end:
    func.assert.171.5.end:
    cmp.172.12:
    mov r14, 2
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.172.12.end:
    func.assert.172.5:
        if.32.27.172.5:
        cmp.32.27.172.5:
        cmp r15b, 0
        jne if.32.24.172.5.end
        if.32.27.172.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.172.5.end:
    func.assert.172.5.end:
    mov r15, 2
    mov r14, 2
    mov r13, 174
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
    mov r13, 174
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
    mov rax, qword [rbp + r14 * 4 + 224]
    mov qword [rbp + 224], rax
    cmp.175.12:
    mov r14, 0
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
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
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov r15, 4
    mov r14, 179
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    mov r14, 179
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    mov rax, qword [rbp + 224]
    mov qword [rbp + 256], rax
    mov rax, qword [rbp + 232]
    mov qword [rbp + 264], rax
    cmp.180.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 180
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
        lea rsi, [rbp + r15 * 4 + 224]
        mov r15, 1
        mov r14, 180
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
    bool.180.19.end:
    cmp.183.12:
    cmp byte [rbp + 288], 0
    setne r15b
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
    mov r14, 185
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp.186.12:
        mov rcx, 4
        mov r14, 186
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 186
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
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 189
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.189.16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    func.inv.189.16.end:
    not dword [rbp + r15 * 4 + 224]
    cmp.190.12:
    mov r14, qword [rbp + 248]
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
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
    func.faz.192.5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    func.faz.192.5.end:
    cmp.193.12:
    mov r14, 1
    mov r13, 193
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool.193.12.end:
    func.assert.193.5:
        if.32.27.193.5:
        cmp.32.27.193.5:
        cmp r15b, 0
        jne if.32.24.193.5.end
        if.32.27.193.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.193.5.end:
    func.assert.193.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.196.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.196.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.196.5
    foo.196.5.end:
    cmp.199.12:
    mov r14, 0
    mov r13, 199
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
    sete r15b
    bool.199.12.end:
    func.assert.199.5:
        if.32.27.199.5:
        cmp.32.27.199.5:
        cmp r15b, 0
        jne if.32.24.199.5.end
        if.32.27.199.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.199.5.end:
    func.assert.199.5.end:
    cmp.200.12:
    mov r14, 1
    mov r13, 200
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 296], r13
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
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.207.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.207.7.end:
    cmp.208.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.208.12.end:
    func.assert.208.5:
        if.32.27.208.5:
        cmp.32.27.208.5:
        cmp r15b, 0
        jne if.32.24.208.5.end
        if.32.27.208.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.208.5.end:
    func.assert.208.5.end:
    cmp.209.12:
    cmp qword [rbp + 320], 11
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
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.212.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
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
    mov qword [rbp + 328], 3
    cmp.217.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
        setne r15b
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
    mov qword [rbp + 344], 0
    func.bar.220.5:
        if.55.8.220.5:
        cmp.55.8.220.5:
        cmp qword [rbp + 344], 0
        jne if.55.5.220.5.end
        if.55.8.220.5.code:
            jmp func.bar.220.5.end
        if.55.5.220.5.end:
        mov qword [rbp + 344], 255
    func.bar.220.5.end:
    cmp.221.12:
    cmp qword [rbp + 344], 0
    sete r15b
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
    mov qword [rbp + 344], 1
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
    cmp qword [rbp + 344], 255
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
    mov qword [rbp + 352], 1
    func.baz.228.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.228.13.end:
    cmp.229.12:
    cmp qword [rbp + 360], 2
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
    func.baz.231.9:
        mov qword [rbp + 360], 1
        sal qword [rbp + 360], 1
    func.baz.231.9.end:
    cmp.232.12:
    cmp qword [rbp + 360], 2
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
    func.baz.234.21:
        mov qword [rbp + 368], 3
        sal qword [rbp + 368], 1
    func.baz.234.21.end:
    mov qword [rbp + 376], 0
    cmp.235.12:
    cmp qword [rbp + 368], 6
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
    func.point_init.237.20:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point_init.237.20.end:
    cmp.238.12:
    cmp qword [rbp + 384], -1
    sete r15b
    bool.238.12.end:
    func.assert.238.5:
        if.32.27.238.5:
        cmp.32.27.238.5:
        cmp r15b, 0
        jne if.32.24.238.5.end
        if.32.27.238.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.238.5.end:
    func.assert.238.5.end:
    cmp.239.12:
    cmp qword [rbp + 392], -2
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
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.245.12:
    cmp qword [rbp + 416], 10
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
    cmp.246.12:
    cmp qword [rbp + 424], 2
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
    cmp.247.12:
    cmp dword [rbp + 432], 16711680
    sete r15b
    bool.247.12.end:
    func.assert.247.5:
        if.32.27.247.5:
        cmp.32.27.247.5:
        cmp r15b, 0
        jne if.32.24.247.5.end
        if.32.27.247.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.247.5.end:
    func.assert.247.5.end:
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
    cmp.251.12:
    cmp qword [rbp + 416], -1
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
    cmp.252.12:
    cmp qword [rbp + 424], -2
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
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.255.12:
    cmp qword [rbp + 456], -1
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
    cmp qword [rbp + 464], -2
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
    cmp.257.12:
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.263.12:
    mov r14, 0
    mov r13, 263
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 73
    sete r15b
    bool.263.12.end:
    func.assert.263.5:
        if.32.27.263.5:
        cmp.32.27.263.5:
        cmp r15b, 0
        jne if.32.24.263.5.end
        if.32.27.263.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.263.5.end:
    func.assert.263.5.end:
    mov r15, 1
    mov r14, 264
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
    imul r15, 24
    func.object_init.264.13:
        mov qword [rbp + r15 + 480], 2
        mov qword [rbp + r15 + 488], 74
        mov dword [rbp + r15 + 496], 16777215
    func.object_init.264.13.end:
    cmp.265.12:
    mov r14, 1
    mov r13, 265
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 74
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
    xor al, al
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov r15, 1
    mov r14, 268
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    lea r15, [rbp + r15 + 528]
    mov r14, 1
    mov r13, 268
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp.269.12:
    mov r14, 1
    mov r13, 269
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 269
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    mov r15, 1
    mov r14, 272
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 272
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 + 528]
    mov r15, 0
    mov r14, 273
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 273
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [rbp + r15 + 528]
    shl rcx, 3
    rep movsb
    cmp.278.12:
    mov r14, 0
    mov r13, 278
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 278
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    cmp.279.12:
        mov rcx, 8
        mov r14, 0
        mov r13, 280
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 280
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 + 528]
        mov r14, 1
        mov r13, 281
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 281
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + r14 + 528]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
        sete r15b
    bool.279.12.end:
    func.assert.279.5:
        if.32.27.279.5:
        cmp.32.27.279.5:
        cmp r15b, 0
        jne if.32.24.279.5.end
        if.32.27.279.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.279.5.end:
    func.assert.279.5.end:
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.285.12:
        mov r14, 2
    cmp r14, 2
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
    mov r14, 0
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], -1
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
    cmp.287.12:
    mov r14, 1
    mov r13, 287
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], 2
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
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.291.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.291.5.end:
    loop.292.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        PUSH_REGS
        lea rbx, [rbp + 1192]
        call func.print_num
        POP_REGS
        func.print.295.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.295.9.end:
        func.print.296.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.296.9.end:
        func.str.input.297.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.297.12.end:
        if.299.12:
        cmp.299.12:
        cmp byte [rbp + 1064], 0
        jg if.301.19
        if.299.12.code:
            jmp loop.292.5.end
        if.301.19:
        cmp.301.19:
        cmp byte [rbp + 1064], 4
        jg if.299.9.else
        if.301.19.code:
            func.print.302.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.302.13.end:
            jmp loop.292.5
        if.299.9.else:
            func.print.305.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.305.13.end:
            func.str.output.306.16:
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
            func.str.output.306.16.end:
            func.print.307.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.307.13.end:
            func.print.308.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.308.13.end:
        if.299.9.end:
    jmp loop.292.5
    loop.292.5.end:
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
    if.114.8:
    cmp.114.8:
    cmp qword [rbx + 32], 0
    jge if.114.5.end
    if.114.8.code:
        mov byte [rbx + 40], 1
        neg qword [rbx + 32]
    if.114.5.end:
    mov qword [rbx + 48], 20
    loop.120.5:
        sub qword [rbx + 48], 1
        mov qword [rbx + 56], 48
        mov r15, qword [rbx + 32]
        mov rax, r15
        cqo
        mov r14, 10
        idiv r14
        mov r15, rdx
        add qword [rbx + 56], r15
        mov r15, qword [rbx + 48]
        mov r14, 125
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14b, byte [rbx + 56]
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 32]
        cqo
        mov r15, 10
        idiv r15
        mov qword [rbx + 32], rax
        if.127.12:
        cmp.127.12:
        cmp qword [rbx + 32], 0
        jne if.127.9.end
        if.127.12.code:
            jmp loop.120.5.end
        if.127.9.end:
    jmp loop.120.5
    loop.120.5.end:
    if.130.8:
    cmp.130.8:
    cmp byte [rbx + 40], 0
    je if.130.5.end
    if.130.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 132
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.130.5.end:
    mov qword [rbx + 56], 0
    loop.136.5:
        mov r15, qword [rbx + 56]
        mov r14, 137
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        mov r13, 137
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
        if.140.12:
        cmp.140.12:
        cmp qword [rbx + 48], 20
        jne if.140.9.end
        if.140.12.code:
            jmp loop.136.5.end
        if.140.9.end:
    jmp loop.136.5
    loop.136.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    mov r15, 143
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
