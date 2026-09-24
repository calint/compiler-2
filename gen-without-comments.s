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
    cmp.148.12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool.148.12.end:
    func.assert.148.5:
        if.32.27.148.5:
        cmp.32.27.148.5:
        cmp r15b, 0
        jne if.32.24.148.5.end
        if.32.27.148.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.148.5.end:
    func.assert.148.5.end:
    mov qword [rbp + 240], -1
    cmp.152.12:
    cmp qword [rbp + 240], -1
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
        cmp.158.16:
        bool.158.16.end:
        mov r15b, 1
        func.assert.158.9:
            if.32.27.158.9:
            cmp.32.27.158.9:
            cmp r15b, 0
            jne if.32.24.158.9.end
            if.32.27.158.9.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.32.24.158.9.end:
        func.assert.158.9.end:
    cmp.161.12:
    bool.161.12.end:
    mov r15b, 1
    func.assert.161.5:
        if.32.27.161.5:
        cmp.32.27.161.5:
        cmp r15b, 0
        jne if.32.24.161.5.end
        if.32.27.161.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.161.5.end:
    func.assert.161.5.end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 166
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp.168.12:
    mov r14, 1
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.168.12.end:
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
    cmp.169.12:
    mov r14, 2
    mov r13, 169
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.169.12.end:
    func.assert.169.5:
        if.32.27.169.5:
        cmp.32.27.169.5:
        cmp r15b, 0
        jne if.32.24.169.5.end
        if.32.27.169.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.169.5.end:
    func.assert.169.5.end:
    mov rcx, 2
    mov r15, 2
    mov r14, 171
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
    mov r15, 171
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp.172.12:
    mov r14, 0
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
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 176
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 176
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp.177.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 177
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
        mov r14, 177
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
    bool.177.19.end:
    cmp.180.12:
    cmp byte [rbp + 288], 0
    setne r15b
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
    mov r15, 2
    mov r14, 182
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp.183.12:
        mov rcx, 4
        mov r14, 183
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 183
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
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 186
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.186.16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    func.inv.186.16.end:
    not dword [rbp + r15 * 4 + 224]
    cmp.187.12:
    mov r14, qword [rbp + 248]
    mov r13, 187
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
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
    func.faz.189.5:
        mov r15, 1
        mov r14, 70
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    func.faz.189.5.end:
    cmp.190.12:
    mov r14, 1
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
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
    mov qword [rbp + 289], 3
    mov qword [rbp + 297], 5
    lea r15, [rbp + 289]
    mov qword [rbp + 313], 0
    foo.193.5:
        mov r14, qword [rbp + 313]
        add qword [r15], r14
        add qword [r15], 2
        foo.193.5.continue:
            add r15, 8
            inc qword [rbp + 313]
            cmp qword [rbp + 313], 2
            jne foo.193.5
    foo.193.5.end:
    cmp.196.12:
    mov r14, 0
    mov r13, 196
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
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
    cmp.197.12:
    mov r14, 1
    mov r13, 197
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 289], r13
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
    mov qword [rbp + 305], 0
    mov qword [rbp + 313], 0
    func.fooz.204.5:
        mov qword [rbp + 305], 2
        mov qword [rbp + 313], 11
    func.fooz.204.5.end:
    cmp.205.12:
    cmp qword [rbp + 305], 2
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
    cmp.206.12:
    cmp qword [rbp + 313], 11
    sete r15b
    bool.206.12.end:
    func.assert.206.5:
        if.32.27.206.5:
        cmp.32.27.206.5:
        cmp r15b, 0
        jne if.32.24.206.5.end
        if.32.27.206.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.206.5.end:
    func.assert.206.5.end:
    mov rax, qword [rbp + 305]
    mov qword [rbp + 321], rax
    mov rax, qword [rbp + 313]
    mov qword [rbp + 329], rax
    cmp.209.12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
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
    mov qword [rbp + 321], 3
    cmp.214.12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        setne r15b
    bool.214.12.end:
    func.assert.214.5:
        if.32.27.214.5:
        cmp.32.27.214.5:
        cmp r15b, 0
        jne if.32.24.214.5.end
        if.32.27.214.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.214.5.end:
    func.assert.214.5.end:
    mov qword [rbp + 337], 0
    func.bar.217.5:
        if.52.8.217.5:
        cmp.52.8.217.5:
        cmp qword [rbp + 337], 0
        jne if.52.5.217.5.end
        if.52.8.217.5.code:
            jmp func.bar.217.5.end
        if.52.5.217.5.end:
        mov qword [rbp + 337], 255
    func.bar.217.5.end:
    cmp.218.12:
    cmp qword [rbp + 337], 0
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
    mov qword [rbp + 337], 1
    func.bar.221.5:
        if.52.8.221.5:
        cmp.52.8.221.5:
        cmp qword [rbp + 337], 0
        jne if.52.5.221.5.end
        if.52.8.221.5.code:
            jmp func.bar.221.5.end
        if.52.5.221.5.end:
        mov qword [rbp + 337], 255
    func.bar.221.5.end:
    cmp.222.12:
    cmp qword [rbp + 337], 255
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
    mov qword [rbp + 345], 1
    func.baz.225.13:
        mov r15, qword [rbp + 345]
        mov qword [rbp + 353], r15
        sal qword [rbp + 353], 1
    func.baz.225.13.end:
    cmp.226.12:
    cmp qword [rbp + 353], 2
    sete r15b
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
    func.baz.228.9:
        mov qword [rbp + 353], 1
        sal qword [rbp + 353], 1
    func.baz.228.9.end:
    cmp.229.12:
    cmp qword [rbp + 353], 2
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
    func.baz.231.21:
        mov qword [rbp + 361], 3
        sal qword [rbp + 361], 1
    func.baz.231.21.end:
    mov qword [rbp + 369], 0
    cmp.232.12:
    cmp qword [rbp + 361], 6
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
    func.point_init.234.20:
        mov qword [rbp + 377], -1
        mov qword [rbp + 385], -2
    func.point_init.234.20.end:
    cmp.235.12:
    cmp qword [rbp + 377], -1
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
    cmp.236.12:
    cmp qword [rbp + 385], -2
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
    mov qword [rbp + 393], 1
    mov qword [rbp + 401], 2
    mov r15, qword [rbp + 393]
    imul r15, 10
    mov qword [rbp + 409], r15
    mov r15, qword [rbp + 401]
    mov qword [rbp + 417], r15
    mov dword [rbp + 425], 16711680
    cmp.242.12:
    cmp qword [rbp + 409], 10
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
    cmp qword [rbp + 417], 2
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
    cmp.244.12:
    cmp dword [rbp + 425], 16711680
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
    mov r15, qword [rbp + 393]
    mov qword [rbp + 429], r15
    neg qword [rbp + 429]
    mov r15, qword [rbp + 401]
    mov qword [rbp + 437], r15
    neg qword [rbp + 437]
    mov rax, qword [rbp + 429]
    mov qword [rbp + 409], rax
    mov rax, qword [rbp + 437]
    mov qword [rbp + 417], rax
    cmp.248.12:
    cmp qword [rbp + 409], -1
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
    cmp.249.12:
    cmp qword [rbp + 417], -2
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
    lea rsi, [rbp + 409]
    lea rdi, [rbp + 445]
    mov rcx, 20
    rep movsb
    cmp.252.12:
    cmp qword [rbp + 445], -1
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
    cmp qword [rbp + 453], -2
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
    cmp.254.12:
    cmp dword [rbp + 461], 16711680
    sete r15b
    bool.254.12.end:
    func.assert.254.5:
        if.32.27.254.5:
        cmp.32.27.254.5:
        cmp r15b, 0
        jne if.32.24.254.5.end
        if.32.27.254.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.254.5.end:
    func.assert.254.5.end:
    xor al, al
    lea rdi, [rbp + 465]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 473], 73
    cmp.260.12:
    mov r14, 0
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    cmp qword [rbp + r14 + 473], 73
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
    mov r15, 1
    mov r14, 261
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
    imul r15, 20
    func.object_init.261.13:
        mov qword [rbp + r15 + 465], 2
        mov qword [rbp + r15 + 473], 74
        mov dword [rbp + r15 + 481], 16777215
    func.object_init.261.13.end:
    cmp.262.12:
    mov r14, 1
    mov r13, 262
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    cmp qword [rbp + r14 + 473], 74
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
    xor al, al
    lea rdi, [rbp + 505]
    mov rcx, 512
    rep stosb
    mov r15, 1
    mov r14, 265
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    lea r15, [rbp + r15 + 505]
    mov r14, 1
    mov r13, 265
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp.266.12:
    mov r14, 1
    mov r13, 266
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 505]
    mov r13, 1
    mov r12, 266
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    mov r15, 1
    mov r14, 269
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 269
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 + 505]
    mov r15, 0
    mov r14, 270
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 270
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [rbp + r15 + 505]
    shl rcx, 3
    rep movsb
    cmp.275.12:
    mov r14, 0
    mov r13, 275
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 505]
    mov r13, 1
    mov r12, 275
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
        mov rcx, 8
        mov r14, 0
        mov r13, 277
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 277
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 + 505]
        mov r14, 1
        mov r13, 278
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 278
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + r14 + 505]
        shl rcx, 3
        test rcx, rcx
        repe cmpsb
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
    mov qword [rbp + 1017], -1
    mov qword [rbp + 1025], 2
    cmp.282.12:
        mov r14, 2
    cmp r14, 2
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
    mov r14, 0
    mov r13, 283
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], -1
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
    cmp.284.12:
    mov r14, 1
    mov r13, 284
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], 2
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
    mov qword [rbp + 1033], 0
    xor al, al
    lea rdi, [rbp + 1041]
    mov rcx, 128
    rep stosb
    func.print.288.5:
        mov rdi, 1
        mov rdx, 21
        lea rsi, [rbp]
        mov rax, 1
        syscall
    func.print.288.5.end:
    loop.289.5:
        add qword [rbp + 1033], 1
        lea r15, [rbp + 1033]
        mov qword [rbp + 1169], r15
        PUSH_REGS
        lea rbx, [rbp + 1169]
        call func.print_num
        POP_REGS
        func.print.292.9:
            mov rdi, 1
            mov rdx, 2
            lea rsi, [rbp + 61]
            mov rax, 1
            syscall
        func.print.292.9.end:
        func.print.293.9:
            mov rdi, 1
            mov rdx, 12
            lea rsi, [rbp + 21]
            mov rax, 1
            syscall
        func.print.293.9.end:
        func.str_in.294.9:
            mov rdi, 0
            mov rdx, 127
            lea rsi, [rbp + 1042]
            mov rax, 0
            syscall
            mov qword [rbp + 1169], rax
            mov r15b, byte [rbp + 1169]
            mov byte [rbp + 1041], r15b
            sub byte [rbp + 1041], 1
        func.str_in.294.9.end:
        if.295.12:
        cmp.295.12:
        cmp byte [rbp + 1041], 0
        jne if.297.19
        if.295.12.code:
            jmp loop.289.5.end
        jmp if.295.9.end
        if.297.19:
        cmp.297.19:
        cmp byte [rbp + 1041], 4
        jg if.295.9.else
        if.297.19.code:
            func.print.298.13:
                mov rdi, 1
                mov rdx, 20
                lea rsi, [rbp + 33]
                mov rax, 1
                syscall
            func.print.298.13.end:
            jmp loop.289.5
        jmp if.295.9.end
        if.295.9.else:
            func.print.301.13:
                mov rdi, 1
                mov rdx, 6
                lea rsi, [rbp + 53]
                mov rax, 1
                syscall
            func.print.301.13.end:
            func.str_out.302.13:
                mov rdi, 1
                movsx rdx, byte [rbp + 1041]
                mov r15, 81
                test rdx, rdx
                cmovs rbp, r15
                js baz_bounds_panic
                cmp rdx, 127
                cmovg rbp, r15
                jg baz_bounds_panic
                lea rsi, [rbp + 1042]
                mov rax, 1
                syscall
            func.str_out.302.13.end:
            func.print.303.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 59]
                mov rax, 1
                syscall
            func.print.303.13.end:
            func.print.304.13:
                mov rdi, 1
                mov rdx, 1
                lea rsi, [rbp + 60]
                mov rax, 1
                syscall
            func.print.304.13.end:
        if.295.9.end:
    jmp loop.289.5
    loop.289.5.end:
    mov rax, 60
    mov rdi, 0
    syscall
func.print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
    mov byte [rbx + 36], 0
    if.111.8:
    cmp.111.8:
    cmp qword [rbx + 28], 0
    jge if.111.5.end
    if.111.8.code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if.111.5.end:
    mov qword [rbx + 37], 20
    loop.117.5:
        sub qword [rbx + 37], 1
        mov qword [rbx + 45], 48
        mov r15, qword [rbx + 28]
        mov rax, r15
        cqo
        mov r14, 10
        idiv r14
        mov r15, rdx
        add qword [rbx + 45], r15
        mov r15, qword [rbx + 37]
        mov r14, 122
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 28]
        cqo
        mov r15, 10
        idiv r15
        mov qword [rbx + 28], rax
        if.124.12:
        cmp.124.12:
        cmp qword [rbx + 28], 0
        jne if.124.9.end
        if.124.12.code:
            jmp loop.117.5.end
        if.124.9.end:
    jmp loop.117.5
    loop.117.5.end:
    if.127.8:
    cmp.127.8:
    cmp byte [rbx + 36], 0
    je if.127.5.end
    if.127.8.code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 129
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.127.5.end:
    mov qword [rbx + 45], 0
    loop.133.5:
        mov r15, qword [rbx + 45]
        mov r14, 134
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 134
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 45], 1
        add qword [rbx + 37], 1
        if.137.12:
        cmp.137.12:
        cmp qword [rbx + 37], 20
        jne if.137.9.end
        if.137.12.code:
            jmp loop.133.5.end
        if.137.9.end:
    jmp loop.133.5
    loop.133.5.end:
    mov rdi, 1
    mov rdx, qword [rbx + 45]
    mov r15, 140
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
func.print_num.size equ 53
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
    msg_panic: db 'panic: bounds at line '
    msg_panic_len equ $ - msg_panic
section .bss
    num_buffer: resb 21
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
dq 1
times 24 db 0
db 3
times 127 db 0
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
vars resb 131072
vars.end:
