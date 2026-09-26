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
    cmp.152.12:
    cmp qword [rbp + 224], 0
    sete r15b
    bool.152.12.end:
    func.assert.152.5:
        if.32.27.152.5:
        cmp.32.27.152.5:
        cmp r15b, 0
        jne if.32.24.152.5.end
        if.32.27.152.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.152.5.end:
    func.assert.152.5.end:
    mov qword [rbp + 224], -1
    cmp.155.12:
    cmp qword [rbp + 224], -1
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
        func.assert.161.9:
            if.32.27.161.9:
            cmp.32.27.161.9:
            if.32.24.161.9.end:
        func.assert.161.9.end:
    func.assert.164.5:
        if.32.27.164.5:
        cmp.32.27.164.5:
        if.32.24.164.5.end:
    func.assert.164.5.end:
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 172
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 232], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 173
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 232]
    mov dword [rbp + r15 * 4 + 232], r13d
    cmp.174.12:
    mov r14, 1
    mov r13, 174
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.174.12.end:
    func.assert.174.5:
        if.32.27.174.5:
        cmp.32.27.174.5:
        cmp r15b, 0
        jne if.32.24.174.5.end
        if.32.27.174.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.174.5.end:
    func.assert.174.5.end:
    cmp.175.12:
    mov r14, 2
    mov r13, 175
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
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
    mov r15, 2
    mov r14, 2
    mov r13, 177
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
    mov r13, 177
    test r15, r15
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r13
    jg baz_bounds_panic
    mov rax, qword [rbp + r14 * 4 + 232]
    mov qword [rbp + 232], rax
    cmp.178.12:
    mov r14, 0
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
    sete r15b
    bool.178.12.end:
    func.assert.178.5:
        if.32.27.178.5:
        cmp.32.27.178.5:
        cmp r15b, 0
        jne if.32.24.178.5.end
        if.32.27.178.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.178.5.end:
    func.assert.178.5.end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov r15, 4
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    mov r14, 182
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
    cmp.183.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 183
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
        mov r14, 183
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
    bool.183.19.end:
    cmp.186.12:
    cmp byte [rbp + 288], 0
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
    mov r15, 2
    mov r14, 188
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp.189.12:
        mov rcx, 4
        mov r14, 189
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 232]
        mov r14, 189
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
    bool.189.12.end:
    func.assert.189.5:
        if.32.27.189.5:
        cmp.32.27.189.5:
        cmp r15b, 0
        jne if.32.24.189.5.end
        if.32.27.189.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.189.5.end:
    func.assert.189.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 192
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 192
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.192.16:
        mov r13d, dword [rbp + r14 * 4 + 232]
        mov dword [rbp + r15 * 4 + 232], r13d
        not dword [rbp + r15 * 4 + 232]
    func.inv.192.16.end:
    not dword [rbp + r15 * 4 + 232]
    cmp.193.12:
    mov r14, qword [rbp + 248]
    mov r13, 193
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 2
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
    func.faz.195.5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 232], 254
    func.faz.195.5.end:
    cmp.196.12:
    mov r14, 1
    mov r13, 196
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 232], 254
    sete r15b
    bool.196.12.end:
    func.assert.196.5:
        if.32.27.196.5:
        cmp.32.27.196.5:
        cmp r15b, 0
        jne if.32.24.196.5.end
        if.32.27.196.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.196.5.end:
    func.assert.196.5.end:
    mov dword [rbp + 296], 3
    mov dword [rbp + 300], 0
    mov dword [rbp + 304], 5
    mov dword [rbp + 308], 0
    lea r15, [rbp + 296]
    mov qword [rbp + 320], 0
    foo.199.5:
        mov r14, qword [rbp + 320]
        add qword [r15], r14
        add qword [r15], 2
        foo.199.5.continue:
            add r15, 8
            inc qword [rbp + 320]
            cmp qword [rbp + 320], 2
            jne foo.199.5
    foo.199.5.end:
    cmp.202.12:
    mov r14, 0
    mov r13, 202
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
    bool.202.12.end:
    func.assert.202.5:
        if.32.27.202.5:
        cmp.32.27.202.5:
        cmp r15b, 0
        jne if.32.24.202.5.end
        if.32.27.202.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.202.5.end:
    func.assert.202.5.end:
    cmp.203.12:
    mov r14, 1
    mov r13, 203
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
    mov qword [rbp + 312], 0
    mov qword [rbp + 320], 0
    func.point.fooz.212.7:
        mov qword [rbp + 312], 2
        mov qword [rbp + 320], 11
    func.point.fooz.212.7.end:
    cmp.215.12:
    cmp qword [rbp + 312], 2
    sete r15b
    bool.215.12.end:
    func.assert.215.5:
        if.32.27.215.5:
        cmp.32.27.215.5:
        cmp r15b, 0
        jne if.32.24.215.5.end
        if.32.27.215.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.215.5.end:
    func.assert.215.5.end:
    cmp.216.12:
    cmp qword [rbp + 320], 11
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
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    mov rax, qword [rbp + 320]
    mov qword [rbp + 336], rax
    cmp.221.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
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
    mov qword [rbp + 328], 3
    cmp.226.12:
        lea rsi, [rbp + 312]
        lea rdi, [rbp + 328]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool.226.12.end:
    func.assert.226.5:
        if.32.27.226.5:
        cmp.32.27.226.5:
        cmp r15b, 0
        jne if.32.24.226.5.end
        if.32.27.226.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.226.5.end:
    func.assert.226.5.end:
    mov qword [rbp + 344], 0
    func.bar.229.5:
        if.55.8.229.5:
        cmp.55.8.229.5:
        cmp qword [rbp + 344], 0
        je func.bar.229.5.end
        if.55.8.229.5.code:
        if.55.5.229.5.end:
        mov qword [rbp + 344], 255
    func.bar.229.5.end:
    cmp.230.12:
    cmp qword [rbp + 344], 0
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
    mov qword [rbp + 344], 1
    func.bar.233.5:
        if.55.8.233.5:
        cmp.55.8.233.5:
        cmp qword [rbp + 344], 0
        je func.bar.233.5.end
        if.55.8.233.5.code:
        if.55.5.233.5.end:
        mov qword [rbp + 344], 255
    func.bar.233.5.end:
    cmp.234.12:
    cmp qword [rbp + 344], 255
    sete r15b
    bool.234.12.end:
    func.assert.234.5:
        if.32.27.234.5:
        cmp.32.27.234.5:
        cmp r15b, 0
        jne if.32.24.234.5.end
        if.32.27.234.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.234.5.end:
    func.assert.234.5.end:
    mov qword [rbp + 352], 1
    func.baz.237.13:
        mov r15, qword [rbp + 352]
        mov qword [rbp + 360], r15
        sal qword [rbp + 360], 1
    func.baz.237.13.end:
    cmp.238.12:
    cmp qword [rbp + 360], 2
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
    func.baz.240.9:
        mov qword [rbp + 360], 1
        sal qword [rbp + 360], 1
    func.baz.240.9.end:
    cmp.241.12:
    cmp qword [rbp + 360], 2
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
    func.baz.243.21:
        mov qword [rbp + 368], 3
        sal qword [rbp + 368], 1
    func.baz.243.21.end:
    mov qword [rbp + 376], 0
    cmp.244.12:
    cmp qword [rbp + 368], 6
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
    func.point_init.246.20:
        mov qword [rbp + 384], -1
        mov qword [rbp + 392], -2
    func.point_init.246.20.end:
    cmp.249.12:
    cmp qword [rbp + 384], -1
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
    cmp qword [rbp + 392], -2
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
    mov qword [rbp + 400], 1
    mov qword [rbp + 408], 2
    mov r15, qword [rbp + 400]
    imul r15, 10
    mov qword [rbp + 416], r15
    mov r15, qword [rbp + 408]
    mov qword [rbp + 424], r15
    mov dword [rbp + 432], 16711680
    mov dword [rbp + 436], 0
    cmp.256.12:
    cmp qword [rbp + 416], 10
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
    cmp qword [rbp + 424], 2
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
    cmp dword [rbp + 432], 16711680
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
    cmp.262.12:
    cmp qword [rbp + 416], -1
    sete r15b
    bool.262.12.end:
    func.assert.262.5:
        if.32.27.262.5:
        cmp.32.27.262.5:
        cmp r15b, 0
        jne if.32.24.262.5.end
        if.32.27.262.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.262.5.end:
    func.assert.262.5.end:
    cmp.263.12:
    cmp qword [rbp + 424], -2
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
    lea rsi, [rbp + 416]
    lea rdi, [rbp + 456]
    mov rcx, 24
    rep movsb
    cmp.266.12:
    cmp qword [rbp + 456], -1
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
    cmp.267.12:
    cmp qword [rbp + 464], -2
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
    cmp.268.12:
    cmp dword [rbp + 472], 16711680
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
    xor al, al
    lea rdi, [rbp + 480]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 488], 73
    cmp.274.12:
    mov r14, 0
    mov r13, 274
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 73
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
    mov r15, 1
    mov r14, 275
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
    imul r15, 24
    func.object_init.275.13:
        mov qword [rbp + r15 + 480], 2
        mov qword [rbp + r15 + 488], 74
        mov dword [rbp + r15 + 496], 16777215
    func.object_init.275.13.end:
    cmp.276.12:
    mov r14, 1
    mov r13, 276
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 24
    cmp qword [rbp + r14 + 488], 74
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
    lea rdi, [rbp + 528]
    mov rcx, 512
    rep stosb
    mov r15, 1
    mov r14, 279
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    lea r15, [rbp + r15 + 528]
    mov r14, 1
    mov r13, 279
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp.280.12:
    mov r14, 1
    mov r13, 280
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 280
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    mov r15, 1
    mov r14, 283
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 283
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 + 528]
    mov r15, 0
    mov r14, 284
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 284
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [rbp + r15 + 528]
    shl rcx, 3
    rep movsb
    cmp.289.12:
    mov r14, 0
    mov r13, 289
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 528]
    mov r13, 1
    mov r12, 289
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
        mov rcx, 8
        mov r14, 0
        mov r13, 291
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 291
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 + 528]
        mov r14, 1
        mov r13, 292
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 292
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
    mov dword [rbp + 1040], -1
    mov dword [rbp + 1044], -1
    mov dword [rbp + 1048], 2
    mov dword [rbp + 1052], 0
    cmp.296.12:
        mov r14, 2
    cmp r14, 2
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
    mov r14, 0
    mov r13, 297
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], -1
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
    cmp.298.12:
    mov r14, 1
    mov r13, 298
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1040], 2
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
    mov qword [rbp + 1056], 0
    xor al, al
    lea rdi, [rbp + 1064]
    mov rcx, 128
    rep stosb
    func.print.302.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.302.5.end:
    loop.303.5:
        add qword [rbp + 1056], 1
        lea r15, [rbp + 1056]
        mov qword [rbp + 1192], r15
        PUSH_REGS
        lea rbx, [rbp + 1192]
        call func.print_num
        POP_REGS
        func.print.306.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.306.9.end:
        func.print.307.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.307.9.end:
        func.str.input.308.12:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1065]
            mov rax, 0
            syscall
            mov qword [rbp + 1192], rax
            mov r15b, byte [rbp + 1192]
            mov byte [rbp + 1064], r15b
            sub byte [rbp + 1064], 1
        func.str.input.308.12.end:
        if.310.12:
        cmp.310.12:
        cmp byte [rbp + 1064], 0
        jle loop.303.5.end
        if.310.12.code:
        if.312.19:
        cmp.312.19:
        cmp byte [rbp + 1064], 4
        jg if.310.9.else
        if.312.19.code:
            func.print.313.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.313.13.end:
            jmp loop.303.5
        if.310.9.else:
            func.print.316.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.316.13.end:
            func.str.output.317.16:
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
            func.str.output.317.16.end:
            func.print.318.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.318.13.end:
            func.print.319.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.319.13.end:
        if.310.9.end:
    jmp loop.303.5
    loop.303.5.end:
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
    if.118.8:
    cmp.118.8:
    cmp qword [rbx + 32], 0
    jge if.118.5.end
    if.118.8.code:
        mov byte [rbx + 40], 1
    if.118.5.end:
    if.121.8:
    cmp.121.8:
    cmp qword [rbx + 32], 0
    jle if.121.5.end
    if.121.8.code:
        neg qword [rbx + 32]
    if.121.5.end:
    mov qword [rbx + 48], 20
    loop.126.5:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 128
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
        if.130.12:
        cmp.130.12:
        cmp qword [rbx + 32], 0
        jne loop.126.5
        if.130.12.code:
        if.130.9.end:
    loop.126.5.end:
    if.133.8:
    cmp.133.8:
    cmp byte [rbx + 40], 0
    je if.133.5.end
    if.133.8.code:
        sub qword [rbx + 48], 1
        mov r15, qword [rbx + 48]
        mov r14, 135
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.133.5.end:
    mov qword [rbx + 56], 0
    loop.139.5:
        mov r15, qword [rbx + 56]
        mov r14, 140
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 48]
        mov r13, 140
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
        if.143.12:
        cmp.143.12:
        cmp qword [rbx + 48], 20
        jne loop.139.5
        if.143.12.code:
        if.143.9.end:
    loop.139.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 56]
    mov r15, 146
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
