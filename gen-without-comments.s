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
    cmp.146.12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool.146.12.end:
    func.assert.146.5:
        if.32.27.146.5:
        cmp.32.27.146.5:
        cmp r15b, 0
        jne if.32.24.146.5.end
        if.32.27.146.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.146.5.end:
    func.assert.146.5.end:
    mov qword [rbp + 240], -1
    cmp.150.12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool.150.12.end:
    func.assert.150.5:
        if.32.27.150.5:
        cmp.32.27.150.5:
        cmp r15b, 0
        jne if.32.24.150.5.end
        if.32.27.150.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.150.5.end:
    func.assert.150.5.end:
        cmp.156.16:
        bool.156.16.end:
        mov r15b, 1
        func.assert.156.9:
            if.32.27.156.9:
            cmp.32.27.156.9:
            cmp r15b, 0
            jne if.32.24.156.9.end
            if.32.27.156.9.code:
                mov rdi, 1
                mov rax, 60
                syscall
            if.32.24.156.9.end:
        func.assert.156.9.end:
    cmp.159.12:
    bool.159.12.end:
    mov r15b, 1
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
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 164
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 165
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 165
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp.166.12:
    mov r14, 1
    mov r13, 166
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.166.12.end:
    func.assert.166.5:
        if.32.27.166.5:
        cmp.32.27.166.5:
        cmp r15b, 0
        jne if.32.24.166.5.end
        if.32.27.166.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.166.5.end:
    func.assert.166.5.end:
    cmp.167.12:
    mov r14, 2
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.167.12.end:
    func.assert.167.5:
        if.32.27.167.5:
        cmp.32.27.167.5:
        cmp r15b, 0
        jne if.32.24.167.5.end
        if.32.27.167.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.167.5.end:
    func.assert.167.5.end:
    mov rcx, 2
    mov r15, 2
    mov r14, 169
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 169
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp.170.12:
    mov r14, 0
    mov r13, 170
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.170.12.end:
    func.assert.170.5:
        if.32.27.170.5:
        cmp.32.27.170.5:
        cmp r15b, 0
        jne if.32.24.170.5.end
        if.32.27.170.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.170.5.end:
    func.assert.170.5.end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 174
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 174
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp.175.19:
        mov rcx, 3
        mov r15, 1
        mov r14, 175
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        mov r13, rcx
        add r13, r15
        cmp r13, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + r15 * 4 + 224]
        mov r15, 1
        mov r14, 175
        test r15, r15
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
    bool.175.19.end:
    cmp.178.12:
    cmp byte [rbp + 288], 0
    setne r15b
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
    mov r15, 2
    mov r14, 180
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp.181.12:
        mov rcx, 4
        mov r14, 181
        test rcx, rcx
        cmovs rbp, r14
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r14
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r14, 181
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
    bool.181.12.end:
    func.assert.181.5:
        if.32.27.181.5:
        cmp.32.27.181.5:
        cmp r15b, 0
        jne if.32.24.181.5.end
        if.32.27.181.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.181.5.end:
    func.assert.181.5.end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 184
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 184
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    func.inv.184.16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    func.inv.184.16.end:
    not dword [rbp + r15 * 4 + 224]
    cmp.185.12:
    mov r14, qword [rbp + 248]
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool.185.12.end:
    func.assert.185.5:
        if.32.27.185.5:
        cmp.32.27.185.5:
        cmp r15b, 0
        jne if.32.24.185.5.end
        if.32.27.185.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.185.5.end:
    func.assert.185.5.end:
    func.faz.187.5:
        mov r15, 1
        mov r14, 69
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    func.faz.187.5.end:
    cmp.188.12:
    mov r14, 1
    mov r13, 188
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
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
    mov qword [rbp + 289], 3
    mov qword [rbp + 297], 5
    lea r15, [rbp + 289]
    mov qword [rbp + 313], 0
    foo.191.5:
        mov r14, qword [rbp + 313]
        add qword [r15], r14
        add qword [r15], 2
        foo.191.5.continue:
            add r15, 8
            inc qword [rbp + 313]
            cmp qword [rbp + 313], 2
            jne foo.191.5
    foo.191.5.end:
    cmp.194.12:
    mov r14, 0
    mov r13, 194
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
    cmp.195.12:
    mov r14, 1
    mov r13, 195
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
    mov qword [rbp + 305], 0
    mov qword [rbp + 313], 0
    func.fooz.202.5:
        mov qword [rbp + 305], 2
        mov qword [rbp + 313], 11
    func.fooz.202.5.end:
    cmp.203.12:
    cmp qword [rbp + 305], 2
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
    cmp qword [rbp + 313], 11
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
    mov rax, qword [rbp + 305]
    mov qword [rbp + 321], rax
    mov rax, qword [rbp + 313]
    mov qword [rbp + 329], rax
    cmp.207.12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
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
    mov qword [rbp + 321], 3
    cmp.212.12:
        lea rsi, [rbp + 305]
        lea rdi, [rbp + 321]
        mov rcx, 2
        repe cmpsq
        setne r15b
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
    mov qword [rbp + 337], 0
    func.bar.215.5:
        if.51.8.215.5:
        cmp.51.8.215.5:
        cmp qword [rbp + 337], 0
        jne if.51.5.215.5.end
        if.51.8.215.5.code:
            jmp func.bar.215.5.end
        if.51.5.215.5.end:
        mov qword [rbp + 337], 255
    func.bar.215.5.end:
    cmp.216.12:
    cmp qword [rbp + 337], 0
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
    mov qword [rbp + 337], 1
    func.bar.219.5:
        if.51.8.219.5:
        cmp.51.8.219.5:
        cmp qword [rbp + 337], 0
        jne if.51.5.219.5.end
        if.51.8.219.5.code:
            jmp func.bar.219.5.end
        if.51.5.219.5.end:
        mov qword [rbp + 337], 255
    func.bar.219.5.end:
    cmp.220.12:
    cmp qword [rbp + 337], 255
    sete r15b
    bool.220.12.end:
    func.assert.220.5:
        if.32.27.220.5:
        cmp.32.27.220.5:
        cmp r15b, 0
        jne if.32.24.220.5.end
        if.32.27.220.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.220.5.end:
    func.assert.220.5.end:
    mov qword [rbp + 345], 1
    func.baz.223.13:
        mov r15, qword [rbp + 345]
        imul r15, 2
        mov qword [rbp + 353], r15
    func.baz.223.13.end:
    cmp.224.12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool.224.12.end:
    func.assert.224.5:
        if.32.27.224.5:
        cmp.32.27.224.5:
        cmp r15b, 0
        jne if.32.24.224.5.end
        if.32.27.224.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.224.5.end:
    func.assert.224.5.end:
    func.baz.226.9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 353], r15
    func.baz.226.9.end:
    cmp.227.12:
    cmp qword [rbp + 353], 2
    sete r15b
    bool.227.12.end:
    func.assert.227.5:
        if.32.27.227.5:
        cmp.32.27.227.5:
        cmp r15b, 0
        jne if.32.24.227.5.end
        if.32.27.227.5.code:
            mov rdi, 1
            mov rax, 60
            syscall
        if.32.24.227.5.end:
    func.assert.227.5.end:
    func.baz.229.21:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 361], r15
    func.baz.229.21.end:
    mov qword [rbp + 369], 0
    cmp.230.12:
    cmp qword [rbp + 361], 6
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
    func.point_init.232.20:
        mov qword [rbp + 377], -1
        mov qword [rbp + 385], -2
    func.point_init.232.20.end:
    cmp.233.12:
    cmp qword [rbp + 377], -1
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
    cmp.234.12:
    cmp qword [rbp + 385], -2
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
    mov qword [rbp + 393], 1
    mov qword [rbp + 401], 2
    mov r15, qword [rbp + 393]
    imul r15, 10
    mov qword [rbp + 409], r15
    mov r15, qword [rbp + 401]
    mov qword [rbp + 417], r15
    mov dword [rbp + 425], 16711680
    cmp.240.12:
    cmp qword [rbp + 409], 10
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
    cmp.241.12:
    cmp qword [rbp + 417], 2
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
    cmp.242.12:
    cmp dword [rbp + 425], 16711680
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
    cmp.246.12:
    cmp qword [rbp + 409], -1
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
    cmp qword [rbp + 417], -2
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
    lea rsi, [rbp + 409]
    lea rdi, [rbp + 445]
    mov rcx, 20
    rep movsb
    cmp.250.12:
    cmp qword [rbp + 445], -1
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
    cmp qword [rbp + 453], -2
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
    cmp dword [rbp + 461], 16711680
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
    xor al, al
    lea rdi, [rbp + 465]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 473], 73
    cmp.258.12:
    mov r14, 0
    mov r13, 258
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    cmp qword [rbp + r14 + 473], 73
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
    mov r15, 1
    mov r14, 259
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 2
    cmovge rbp, r14
    jge baz_bounds_panic
    imul r15, 20
    func.object_init.259.13:
        mov qword [rbp + r15 + 465], 2
        mov qword [rbp + r15 + 473], 74
        mov dword [rbp + r15 + 481], 16777215
    func.object_init.259.13.end:
    cmp.260.12:
    mov r14, 1
    mov r13, 260
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    cmp qword [rbp + r14 + 473], 74
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
    xor al, al
    lea rdi, [rbp + 505]
    mov rcx, 512
    rep stosb
    mov r15, 1
    mov r14, 263
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    lea r15, [rbp + r15 + 505]
    mov r14, 1
    mov r13, 263
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp.264.12:
    mov r14, 1
    mov r13, 264
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 505]
    mov r13, 1
    mov r12, 264
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    mov r15, 1
    mov r14, 267
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 267
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 + 505]
    mov r15, 0
    mov r14, 268
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    shl r15, 6
    mov r14, 268
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [rbp + r15 + 505]
    shl rcx, 3
    rep movsb
    cmp.273.12:
    mov r14, 0
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    lea r14, [rbp + r14 + 505]
    mov r13, 1
    mov r12, 273
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
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
    cmp.274.12:
        mov rcx, 8
        mov r14, 0
        mov r13, 275
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 275
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + r14 + 505]
        mov r14, 1
        mov r13, 276
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 8
        cmovge rbp, r13
        jge baz_bounds_panic
        shl r14, 6
        mov r13, 276
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
    mov qword [rbp + 1017], -1
    mov qword [rbp + 1025], 2
    cmp.280.12:
        mov r14, 2
    cmp r14, 2
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
    mov r14, 0
    mov r13, 281
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], -1
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
    cmp.282.12:
    mov r14, 1
    mov r13, 282
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1017], 2
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
    mov qword [rbp + 1033], 0
    xor al, al
    lea rdi, [rbp + 1041]
    mov rcx, 128
    rep stosb
    func.print.286.5:
        mov rdi, 1
        lea rsi, [rbp]
        mov rdx, 21
        mov rax, 1
        syscall
    func.print.286.5.end:
    loop.287.5:
        add qword [rbp + 1033], 1
        lea r15, [rbp + 1033]
        mov qword [rbp + 1169], r15
        PUSH_REGS
        lea rbx, [rbp + 1169]
        call func.print_num
        POP_REGS
        func.print.290.9:
            mov rdi, 1
            lea rsi, [rbp + 61]
            mov rdx, 2
            mov rax, 1
            syscall
        func.print.290.9.end:
        func.print.291.9:
            mov rdi, 1
            lea rsi, [rbp + 21]
            mov rdx, 12
            mov rax, 1
            syscall
        func.print.291.9.end:
        func.str_in.292.9:
            mov rdi, 0
            lea rsi, [rbp + 1042]
            mov rdx, 127
            mov rax, 0
            syscall
            mov qword [rbp + 1169], rax
            mov r15b, byte [rbp + 1169]
            mov byte [rbp + 1041], r15b
            sub byte [rbp + 1041], 1
        func.str_in.292.9.end:
        if.293.12:
        cmp.293.12:
        cmp byte [rbp + 1041], 0
        jne if.295.19
        if.293.12.code:
            jmp loop.287.5.end
        jmp if.293.9.end
        if.295.19:
        cmp.295.19:
        cmp byte [rbp + 1041], 4
        jg if.293.9.else
        if.295.19.code:
            func.print.296.13:
                mov rdi, 1
                lea rsi, [rbp + 33]
                mov rdx, 20
                mov rax, 1
                syscall
            func.print.296.13.end:
            jmp loop.287.5
        jmp if.293.9.end
        if.293.9.else:
            func.print.299.13:
                mov rdi, 1
                lea rsi, [rbp + 53]
                mov rdx, 6
                mov rax, 1
                syscall
            func.print.299.13.end:
            func.str_out.300.13:
                mov rdi, 1
                lea rsi, [rbp + 1042]
                movsx rdx, byte [rbp + 1041]
                mov rax, 1
                syscall
            func.str_out.300.13.end:
            func.print.301.13:
                mov rdi, 1
                lea rsi, [rbp + 59]
                mov rdx, 1
                mov rax, 1
                syscall
            func.print.301.13.end:
            func.print.302.13:
                mov rdi, 1
                lea rsi, [rbp + 60]
                mov rdx, 1
                mov rax, 1
                syscall
            func.print.302.13.end:
        if.293.9.end:
    jmp loop.287.5
    loop.287.5.end:
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
    if.109.8:
    cmp.109.8:
    cmp qword [rbx + 28], 0
    jge if.109.5.end
    if.109.8.code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if.109.5.end:
    mov qword [rbx + 37], 20
    loop.115.5:
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
        mov r14, 120
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
        if.122.12:
        cmp.122.12:
        cmp qword [rbx + 28], 0
        jne if.122.9.end
        if.122.12.code:
            jmp loop.115.5.end
        if.122.9.end:
    jmp loop.115.5
    loop.115.5.end:
    if.125.8:
    cmp.125.8:
    cmp byte [rbx + 36], 0
    je if.125.5.end
    if.125.8.code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 127
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if.125.5.end:
    mov qword [rbx + 45], 0
    loop.131.5:
        mov r15, qword [rbx + 45]
        mov r14, 132
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 132
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
        if.135.12:
        cmp.135.12:
        cmp qword [rbx + 37], 20
        jne if.135.9.end
        if.135.12.code:
            jmp loop.131.5.end
        if.135.9.end:
    jmp loop.131.5
    loop.131.5.end:
    mov rdi, 1
    lea rsi, [rbx + 8]
    mov rdx, qword [rbx + 45]
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
