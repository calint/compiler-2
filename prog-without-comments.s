default rel
section .text
bits 64
global _start
_start:
mov rbp, strict qword vars
main:
    mov qword [rbp], 0
    cmp.190.12:
    cmp qword [rbp], 0
    sete r15b
    bool.190.12.end:
    func.assert..190.5:
        if.38.24.190.5:
        cmp.38.27.190.5:
        je if.38.24.190.5.end
        if.38.24.190.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.190.5.end:
    func.assert..190.5.end:
    mov qword [rbp], -1
    cmp.193.12:
    cmp qword [rbp], -1
    sete r15b
    bool.193.12.end:
    func.assert..193.5:
        if.38.24.193.5:
        cmp.38.27.193.5:
        je if.38.24.193.5.end
        if.38.24.193.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.193.5.end:
    func.assert..193.5.end:
        func.assert..199.9:
            if.38.24.199.9:
            cmp.38.27.199.9:
            if.38.24.199.9.end:
        func.assert..199.9.end:
    func.assert..202.5:
        if.38.24.202.5:
        cmp.38.27.202.5:
        if.38.24.202.5.end:
    func.assert..202.5.end:
    cmp.204.12:
    cmp qword [rbp - 320], 1
    sete r15b
    jne bool.204.12.end
    cmp.204.29:
    cmp qword [rbp - 296], 0
    sete r15b
    bool.204.12.end:
    func.assert..204.5:
        if.38.24.204.5:
        cmp.38.27.204.5:
        test r15b, r15b
        jne if.38.24.204.5.end
        if.38.24.204.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.204.5.end:
    func.assert..204.5.end:
    cmp.205.12:
    cmp byte [rbp - 160], 3
    sete r15b
    jne bool.205.12.end
    cmp.205.30:
    cmp byte [rbp - 157], 122
    sete r15b
    bool.205.12.end:
    func.assert..205.5:
        if.38.24.205.5:
        cmp.38.27.205.5:
        test r15b, r15b
        jne if.38.24.205.5.end
        if.38.24.205.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.205.5.end:
    func.assert..205.5.end:
    mov qword [rbp + 8], 7
    cmp.208.12:
        mov r14, qword [rbp + 8]
        and r14, 3
    cmp r14, 3
    sete r15b
    bool.208.12.end:
    func.assert..208.5:
        if.38.24.208.5:
        cmp.38.27.208.5:
        je if.38.24.208.5.end
        if.38.24.208.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.208.5.end:
    func.assert..208.5.end:
    cmp.209.12:
        mov r14, qword [rbp + 8]
        or r14, 8
    cmp r14, 15
    sete r15b
    bool.209.12.end:
    func.assert..209.5:
        if.38.24.209.5:
        cmp.38.27.209.5:
        je if.38.24.209.5.end
        if.38.24.209.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.209.5.end:
    func.assert..209.5.end:
    cmp.210.12:
        mov r14, qword [rbp + 8]
        xor r14, 1
    cmp r14, 6
    sete r15b
    bool.210.12.end:
    func.assert..210.5:
        if.38.24.210.5:
        cmp.38.27.210.5:
        je if.38.24.210.5.end
        if.38.24.210.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.210.5.end:
    func.assert..210.5.end:
    cmp.211.12:
        mov r14, qword [rbp + 8]
        sal r14, 2
    cmp r14, 28
    sete r15b
    bool.211.12.end:
    func.assert..211.5:
        if.38.24.211.5:
        cmp.38.27.211.5:
        je if.38.24.211.5.end
        if.38.24.211.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.211.5.end:
    func.assert..211.5.end:
    cmp.212.12:
        mov r14, qword [rbp + 8]
        neg r14
        jo baz_overflow_line_212
        sar r14, 1
    cmp r14, -4
    sete r15b
    bool.212.12.end:
    func.assert..212.5:
        if.38.24.212.5:
        cmp.38.27.212.5:
        je if.38.24.212.5.end
        if.38.24.212.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.212.5.end:
    func.assert..212.5.end:
    cmp.215.12:
        mov r14, qword [rbp + 8]
        mov r13, qword [rbp + 8]
        sal r13, 1
        add r14, r13
        jo baz_overflow_line_215
    cmp r14, 21
    sete r15b
    bool.215.12.end:
    func.assert..215.5:
        if.38.24.215.5:
        cmp.38.27.215.5:
        je if.38.24.215.5.end
        if.38.24.215.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.215.5.end:
    func.assert..215.5.end:
    cmp.216.12:
        mov r14, qword [rbp + 8]
        add r14, qword [rbp + 8]
        jo baz_overflow_line_216
        sal r14, 1
    cmp r14, 28
    sete r15b
    bool.216.12.end:
    func.assert..216.5:
        if.38.24.216.5:
        cmp.38.27.216.5:
        je if.38.24.216.5.end
        if.38.24.216.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.216.5.end:
    func.assert..216.5.end:
    cmp.221.12:
    cmp qword [rbp + 8], 0
    setne r15b
    je bool.221.12.end
    cmp.221.23:
    cmp.221.24:
    cmp qword [rbp + 8], 7
    setge r15b
    jge bool.221.12.end
    cmp.221.34:
    cmp qword [rbp + 8], 0
    setl r15b
    bool.221.12.end:
    func.assert..221.5:
        if.38.24.221.5:
        cmp.38.27.221.5:
        test r15b, r15b
        jne if.38.24.221.5.end
        if.38.24.221.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.221.5.end:
    func.assert..221.5.end:
    mov byte [rbp + 16], 100
    mov r15b, byte [rbp + 16]
    add r15b, byte [rbp + 16]
    mov byte [rbp + 16], r15b
    cmp.226.12:
    cmp byte [rbp + 16], -56
    sete r15b
    bool.226.12.end:
    func.assert..226.5:
        if.38.24.226.5:
        cmp.38.27.226.5:
        je if.38.24.226.5.end
        if.38.24.226.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.226.5.end:
    func.assert..226.5.end:
    movsx r15w, byte [rbp + 16]
    movsx r15, r15w
    mov qword [rbp + 24], r15
    cmp.231.12:
    cmp qword [rbp + 24], -56
    sete r15b
    bool.231.12.end:
    func.assert..231.5:
        if.38.24.231.5:
        cmp.38.27.231.5:
        je if.38.24.231.5.end
        if.38.24.231.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.231.5.end:
    func.assert..231.5.end:
    mov qword [rbp + 32], 65
    cmp.239.12:
    cmp qword [rbp + 32], 65
    sete r15b
    bool.239.12.end:
    func.assert..239.5:
        if.38.24.239.5:
        cmp.38.27.239.5:
        je if.38.24.239.5.end
        if.38.24.239.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.239.5.end:
    func.assert..239.5.end:
    mov qword [rbp + 40], 0
    mov qword [rbp + 48], 0
    mov qword [rbp + 56], 1
    mov r15, qword [rbp + 56]
    cmp r15, 4
    jae baz_bounds_line_245
    mov dword [rbp + r15 * 4 + 40], 2
    mov r15, qword [rbp + 56]
    inc r15
    jo baz_overflow_line_246
    cmp r15, 4
    jae baz_bounds_line_246
    mov r14, qword [rbp + 56]
    cmp r14, 4
    jae baz_bounds_line_246
    mov r14d, dword [rbp + r14 * 4 + 40]
    mov dword [rbp + r15 * 4 + 40], r14d
    cmp.247.12:
    cmp dword [rbp + 44], 2
    sete r15b
    bool.247.12.end:
    func.assert..247.5:
        if.38.24.247.5:
        cmp.38.27.247.5:
        je if.38.24.247.5.end
        if.38.24.247.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.247.5.end:
    func.assert..247.5.end:
    cmp.248.12:
    cmp dword [rbp + 48], 2
    sete r15b
    bool.248.12.end:
    func.assert..248.5:
        if.38.24.248.5:
        cmp.38.27.248.5:
        je if.38.24.248.5.end
        if.38.24.248.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.248.5.end:
    func.assert..248.5.end:
    mov r15, qword [rbp + 48]
    mov qword [rbp + 40], r15
    cmp.251.12:
    cmp dword [rbp + 40], 2
    sete r15b
    bool.251.12.end:
    func.assert..251.5:
        if.38.24.251.5:
        cmp.38.27.251.5:
        je if.38.24.251.5.end
        if.38.24.251.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.251.5.end:
    func.assert..251.5.end:
    mov qword [rbp + 64], 0
    mov qword [rbp + 72], 0
    mov qword [rbp + 80], 0
    mov qword [rbp + 88], 0
    mov r15, qword [rbp + 40]
    mov qword [rbp + 64], r15
    mov r15, qword [rbp + 48]
    mov qword [rbp + 72], r15
    lea rsi, [rbp + 44]
    lea rdi, [rbp + 68]
    cmpsq
    jne .Lbaz_equal.0
    cmpsd
    .Lbaz_equal.0:
    sete byte [rbp + 96]
    func.assert..259.5:
        if.38.24.259.5:
        cmp.38.27.259.5:
        je if.38.24.259.5.end
        if.38.24.259.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.259.5.end:
    func.assert..259.5.end:
    mov dword [rbp + 72], -1
    cmp.262.12:
        lea rsi, [rbp + 40]
        lea rdi, [rbp + 64]
        cmpsq
        jne .Lbaz_equal.1
        cmpsq
        .Lbaz_equal.1:
        sete r15b
    test r15b, r15b
    sete r15b
    bool.262.12.end:
    func.assert..262.5:
        if.38.24.262.5:
        cmp.38.27.262.5:
        je if.38.24.262.5.end
        if.38.24.262.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.262.5.end:
    func.assert..262.5.end:
    mov r15, qword [rbp + 40]
    mov qword [rbp + 100], r15
    mov r15, qword [rbp + 48]
    mov qword [rbp + 108], r15
    cmp.265.12:
        lea rsi, [rbp + 40]
        lea rdi, [rbp + 100]
        cmpsq
        jne .Lbaz_equal.2
        cmpsq
        .Lbaz_equal.2:
        sete r15b
    bool.265.12.end:
    func.assert..265.5:
        if.38.24.265.5:
        cmp.38.27.265.5:
        je if.38.24.265.5.end
        if.38.24.265.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.265.5.end:
    func.assert..265.5.end:
    mov qword [rbp + 56], 3
    mov r15, qword [rbp + 56]
    dec r15
    jo baz_overflow_line_272
    cmp r15, 4
    jae baz_bounds_line_272
    func.inv..272.16:
        mov r14d, dword [rbp + r15 * 4 + 40]
        mov dword [rbp + 116], r14d
        not dword [rbp + 116]
    func.inv..272.16.end:
    not dword [rbp + 116]
    mov r15, qword [rbp + 56]
    cmp r15, 4
    jae baz_bounds_line_273
    mov r14d, dword [rbp + 116]
    mov dword [rbp + r15 * 4 + 40], r14d
    cmp.274.12:
    mov r14, qword [rbp + 56]
    cmp r14, 4
    jae baz_bounds_line_274
    cmp dword [rbp + r14 * 4 + 40], 2
    sete r15b
    bool.274.12.end:
    func.assert..274.5:
        if.38.24.274.5:
        cmp.38.27.274.5:
        je if.38.24.274.5.end
        if.38.24.274.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.274.5.end:
    func.assert..274.5.end:
    func.faz..276.5:
        mov dword [rbp + 44], 254
    func.faz..276.5.end:
    cmp.277.12:
    cmp dword [rbp + 44], 254
    sete r15b
    bool.277.12.end:
    func.assert..277.5:
        if.38.24.277.5:
        cmp.38.27.277.5:
        je if.38.24.277.5.end
        if.38.24.277.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.277.5.end:
    func.assert..277.5.end:
    mov qword [rbp + 120], 3
    mov qword [rbp + 128], 5
    lea r15, [rbp + 120]
    xor r14d, r14d
    foo.280.5:
        add qword [r15], r14
        jo baz_overflow_line_281
        add qword [r15], 2
        jo baz_overflow_line_281
        foo.280.5.continue:
            add r15, 8
            inc r14
            cmp r14, 2
            jne foo.280.5
    foo.280.5.end:
    cmp.283.12:
    cmp qword [rbp + 120], 5
    sete r15b
    bool.283.12.end:
    func.assert..283.5:
        if.38.24.283.5:
        cmp.38.27.283.5:
        je if.38.24.283.5.end
        if.38.24.283.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.283.5.end:
    func.assert..283.5.end:
    cmp.284.12:
    cmp qword [rbp + 128], 8
    sete r15b
    bool.284.12.end:
    func.assert..284.5:
        if.38.24.284.5:
        cmp.38.27.284.5:
        je if.38.24.284.5.end
        if.38.24.284.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.284.5.end:
    func.assert..284.5.end:
    mov qword [rbp + 136], 0
    mov qword [rbp + 144], 0
    func.point.fooz..292.7:
        mov qword [rbp + 136], 2
        mov qword [rbp + 144], 11
    func.point.fooz..292.7.end:
    cmp.295.12:
    cmp qword [rbp + 136], 2
    sete r15b
    bool.295.12.end:
    func.assert..295.5:
        if.38.24.295.5:
        cmp.38.27.295.5:
        je if.38.24.295.5.end
        if.38.24.295.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.295.5.end:
    func.assert..295.5.end:
    cmp.296.12:
    cmp qword [rbp + 144], 11
    sete r15b
    bool.296.12.end:
    func.assert..296.5:
        if.38.24.296.5:
        cmp.38.27.296.5:
        je if.38.24.296.5.end
        if.38.24.296.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.296.5.end:
    func.assert..296.5.end:
    mov r15, qword [rbp + 136]
    mov qword [rbp + 152], r15
    mov r15, qword [rbp + 144]
    mov qword [rbp + 160], r15
    cmp.301.12:
        lea rsi, [rbp + 136]
        lea rdi, [rbp + 152]
        cmpsq
        jne .Lbaz_equal.3
        cmpsq
        .Lbaz_equal.3:
        sete r15b
    bool.301.12.end:
    func.assert..301.5:
        if.38.24.301.5:
        cmp.38.27.301.5:
        je if.38.24.301.5.end
        if.38.24.301.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.301.5.end:
    func.assert..301.5.end:
    mov qword [rbp + 152], 3
    cmp.306.12:
        lea rsi, [rbp + 136]
        lea rdi, [rbp + 152]
        cmpsq
        jne .Lbaz_equal.4
        cmpsq
        .Lbaz_equal.4:
        setne r15b
    bool.306.12.end:
    func.assert..306.5:
        if.38.24.306.5:
        cmp.38.27.306.5:
        jne if.38.24.306.5.end
        if.38.24.306.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.306.5.end:
    func.assert..306.5.end:
    mov qword [rbp + 168], 0
    func.bar..309.5:
        if.60.5.309.5:
        cmp.60.8.309.5:
        cmp qword [rbp + 168], 0
        je func.bar..309.5.end
        if.60.5.309.5.code:
        if.60.5.309.5.end:
        mov qword [rbp + 168], 255
    func.bar..309.5.end:
    cmp.310.12:
    cmp qword [rbp + 168], 0
    sete r15b
    bool.310.12.end:
    func.assert..310.5:
        if.38.24.310.5:
        cmp.38.27.310.5:
        je if.38.24.310.5.end
        if.38.24.310.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.310.5.end:
    func.assert..310.5.end:
    mov qword [rbp + 168], 1
    func.bar..313.5:
        if.60.5.313.5:
        cmp.60.8.313.5:
        cmp qword [rbp + 168], 0
        je func.bar..313.5.end
        if.60.5.313.5.code:
        if.60.5.313.5.end:
        mov qword [rbp + 168], 255
    func.bar..313.5.end:
    cmp.314.12:
    cmp qword [rbp + 168], 255
    sete r15b
    bool.314.12.end:
    func.assert..314.5:
        if.38.24.314.5:
        cmp.38.27.314.5:
        je if.38.24.314.5.end
        if.38.24.314.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.314.5.end:
    func.assert..314.5.end:
    mov qword [rbp + 176], 1
    func.baz..317.13:
        mov r15, qword [rbp + 176]
        imul r15, 2
        jo baz_overflow_line_67
        mov qword [rbp + 184], r15
    func.baz..317.13.end:
    cmp.318.12:
    cmp qword [rbp + 184], 2
    sete r15b
    bool.318.12.end:
    func.assert..318.5:
        if.38.24.318.5:
        cmp.38.27.318.5:
        je if.38.24.318.5.end
        if.38.24.318.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.318.5.end:
    func.assert..318.5.end:
    func.baz..320.9:
        mov qword [rbp + 184], 2
    func.baz..320.9.end:
    cmp.321.12:
    cmp qword [rbp + 184], 2
    sete r15b
    bool.321.12.end:
    func.assert..321.5:
        if.38.24.321.5:
        cmp.38.27.321.5:
        je if.38.24.321.5.end
        if.38.24.321.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.321.5.end:
    func.assert..321.5.end:
    mov qword [rbp + 192], 5
    mov r15, strict qword vars.end - size.func.factorial - 208
    cmp r15, rbp
    jl baz_frame_overflow
    lea r15, [rbp + 200]
    mov qword [rbp + 208], r15
    lea r15, [rbp + 192]
    mov qword [rbp + 216], r15
    lea rbx, [rbp + 208]
    call func.factorial
    cmp.325.12:
    cmp qword [rbp + 200], 120
    sete r15b
    bool.325.12.end:
    func.assert..325.5:
        if.38.24.325.5:
        cmp.38.27.325.5:
        je if.38.24.325.5.end
        if.38.24.325.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.325.5.end:
    func.assert..325.5.end:
    func.baz..327.20:
        mov qword [rbp + 208], 6
    func.baz..327.20.end:
    mov qword [rbp + 216], 0
    cmp.328.12:
    cmp qword [rbp + 208], 6
    sete r15b
    bool.328.12.end:
    func.assert..328.5:
        if.38.24.328.5:
        cmp.38.27.328.5:
        je if.38.24.328.5.end
        if.38.24.328.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.328.5.end:
    func.assert..328.5.end:
    func.point.at..330.14:
        mov qword [rbp + 224], -1
        mov qword [rbp + 232], -2
    func.point.at..330.14.end:
    cmp.334.12:
    cmp qword [rbp + 224], -1
    sete r15b
    bool.334.12.end:
    func.assert..334.5:
        if.38.24.334.5:
        cmp.38.27.334.5:
        je if.38.24.334.5.end
        if.38.24.334.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.334.5.end:
    func.assert..334.5.end:
    cmp.335.12:
    cmp qword [rbp + 232], -2
    sete r15b
    bool.335.12.end:
    func.assert..335.5:
        if.38.24.335.5:
        cmp.38.27.335.5:
        je if.38.24.335.5.end
        if.38.24.335.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.335.5.end:
    func.assert..335.5.end:
    func.point.x..337.8:
        mov qword [rbp + 224], 2
    func.point.x..337.8.end:
    cmp.338.12:
    cmp qword [rbp + 224], 2
    sete r15b
    bool.338.12.end:
    func.assert..338.5:
        if.38.24.338.5:
        cmp.38.27.338.5:
        je if.38.24.338.5.end
        if.38.24.338.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.338.5.end:
    func.assert..338.5.end:
    cmp.339.12:
        func.point.sum..339.15:
            mov r14, qword [rbp + 224]
            add r14, qword [rbp + 232]
            jo baz_overflow_line_55
        func.point.sum..339.15.end:
    test r14, r14
    sete r15b
    bool.339.12.end:
    func.assert..339.5:
        if.38.24.339.5:
        cmp.38.27.339.5:
        je if.38.24.339.5.end
        if.38.24.339.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.339.5.end:
    func.assert..339.5.end:
    mov qword [rbp + 240], 1
    mov qword [rbp + 248], 2
    mov r15, qword [rbp + 240]
    imul r15, 10
    jo baz_overflow_line_344
    mov qword [rbp + 256], r15
    mov r15, qword [rbp + 248]
    mov qword [rbp + 264], r15
    mov dword [rbp + 272], 16711680
    mov dword [rbp + 276], 0
    cmp.345.12:
    cmp qword [rbp + 256], 10
    sete r15b
    bool.345.12.end:
    func.assert..345.5:
        if.38.24.345.5:
        cmp.38.27.345.5:
        je if.38.24.345.5.end
        if.38.24.345.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.345.5.end:
    func.assert..345.5.end:
    cmp.346.12:
    cmp qword [rbp + 264], 2
    sete r15b
    bool.346.12.end:
    func.assert..346.5:
        if.38.24.346.5:
        cmp.38.27.346.5:
        je if.38.24.346.5.end
        if.38.24.346.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.346.5.end:
    func.assert..346.5.end:
    cmp.347.12:
    cmp dword [rbp + 272], 16711680
    sete r15b
    bool.347.12.end:
    func.assert..347.5:
        if.38.24.347.5:
        cmp.38.27.347.5:
        je if.38.24.347.5.end
        if.38.24.347.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.347.5.end:
    func.assert..347.5.end:
    mov r15, qword [rbp + 240]
    mov qword [rbp + 280], r15
    neg qword [rbp + 280]
    jo baz_overflow_line_349
    mov r15, qword [rbp + 248]
    mov qword [rbp + 288], r15
    neg qword [rbp + 288]
    jo baz_overflow_line_349
    mov r15, qword [rbp + 280]
    mov qword [rbp + 256], r15
    mov r15, qword [rbp + 288]
    mov qword [rbp + 264], r15
    cmp.351.12:
    cmp qword [rbp + 256], -1
    sete r15b
    bool.351.12.end:
    func.assert..351.5:
        if.38.24.351.5:
        cmp.38.27.351.5:
        je if.38.24.351.5.end
        if.38.24.351.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.351.5.end:
    func.assert..351.5.end:
    cmp.352.12:
    cmp qword [rbp + 264], -2
    sete r15b
    bool.352.12.end:
    func.assert..352.5:
        if.38.24.352.5:
        cmp.38.27.352.5:
        je if.38.24.352.5.end
        if.38.24.352.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.352.5.end:
    func.assert..352.5.end:
    lea rsi, [rbp + 256]
    lea rdi, [rbp + 296]
    mov rcx, 24
    rep movsb
    cmp.355.12:
    cmp qword [rbp + 296], -1
    sete r15b
    bool.355.12.end:
    func.assert..355.5:
        if.38.24.355.5:
        cmp.38.27.355.5:
        je if.38.24.355.5.end
        if.38.24.355.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.355.5.end:
    func.assert..355.5.end:
    cmp.356.12:
    cmp qword [rbp + 304], -2
    sete r15b
    bool.356.12.end:
    func.assert..356.5:
        if.38.24.356.5:
        cmp.38.27.356.5:
        je if.38.24.356.5.end
        if.38.24.356.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.356.5.end:
    func.assert..356.5.end:
    cmp.357.12:
    cmp dword [rbp + 312], 16711680
    sete r15b
    bool.357.12.end:
    func.assert..357.5:
        if.38.24.357.5:
        cmp.38.27.357.5:
        je if.38.24.357.5.end
        if.38.24.357.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.357.5.end:
    func.assert..357.5.end:
    mov r15, qword [rbp + 240]
    mov qword [rbp + 296], r15
    mov r15, qword [rbp + 248]
    mov qword [rbp + 304], r15
    cmp.360.12:
    cmp qword [rbp + 296], 1
    sete r15b
    bool.360.12.end:
    func.assert..360.5:
        if.38.24.360.5:
        cmp.38.27.360.5:
        je if.38.24.360.5.end
        if.38.24.360.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.360.5.end:
    func.assert..360.5.end:
    mov r15, qword [rbp + 248]
    mov qword [rbp + 296], r15
    mov r15, qword [rbp + 240]
    mov qword [rbp + 304], r15
    cmp.362.12:
    cmp qword [rbp + 296], 2
    sete r15b
    bool.362.12.end:
    func.assert..362.5:
        if.38.24.362.5:
        cmp.38.27.362.5:
        je if.38.24.362.5.end
        if.38.24.362.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.362.5.end:
    func.assert..362.5.end:
    xor al, al
    lea rdi, [rbp + 320]
    mov rcx, 48
    rep stosb
    mov qword [rbp + 328], 73
    cmp.372.12:
    cmp qword [rbp + 328], 73
    sete r15b
    bool.372.12.end:
    func.assert..372.5:
        if.38.24.372.5:
        cmp.38.27.372.5:
        je if.38.24.372.5.end
        if.38.24.372.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.372.5.end:
    func.assert..372.5.end:
    mov dword [rbp + 364], 0
    func.object.at..373.13:
        func.point.at..120.16.373.13:
            mov qword [rbp + 344], 2
            mov qword [rbp + 352], 74
        func.point.at..120.16.373.13.end:
        mov dword [rbp + 360], 16777215
    func.object.at..373.13.end:
    cmp.374.12:
    cmp qword [rbp + 352], 74
    sete r15b
    bool.374.12.end:
    func.assert..374.5:
        if.38.24.374.5:
        cmp.38.27.374.5:
        je if.38.24.374.5.end
        if.38.24.374.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.374.5.end:
    func.assert..374.5.end:
    func.point.fooz..376.15:
        mov qword [rbp + 344], 2
        mov qword [rbp + 352], 11
    func.point.fooz..376.15.end:
    cmp.377.12:
        func.point.sum..377.22:
            mov r14, qword [rbp + 344]
            add r14, qword [rbp + 352]
            jo baz_overflow_line_55
        func.point.sum..377.22.end:
    cmp r14, 13
    sete r15b
    bool.377.12.end:
    func.assert..377.5:
        if.38.24.377.5:
        cmp.38.27.377.5:
        je if.38.24.377.5.end
        if.38.24.377.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.377.5.end:
    func.assert..377.5.end:
    xor al, al
    lea rdi, [rbp + 368]
    mov rcx, 512
    rep stosb
    mov qword [rbp + 440], 65518
    cmp.382.12:
    cmp qword [rbp + 440], 65518
    sete r15b
    bool.382.12.end:
    func.assert..382.5:
        if.38.24.382.5:
        cmp.38.27.382.5:
        je if.38.24.382.5.end
        if.38.24.382.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.382.5.end:
    func.assert..382.5.end:
    lea rsi, [rbp + 432]
    lea rdi, [rbp + 368]
    mov rcx, 64
    rep movsb
    cmp.391.12:
    cmp qword [rbp + 376], 65518
    sete r15b
    bool.391.12.end:
    func.assert..391.5:
        if.38.24.391.5:
        cmp.38.27.391.5:
        je if.38.24.391.5.end
        if.38.24.391.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.391.5.end:
    func.assert..391.5.end:
    lea rsi, [rbp + 368]
    lea rdi, [rbp + 432]
    mov rcx, 8
    repe cmpsq
    sete r15b
    func.assert..392.5:
        if.38.24.392.5:
        cmp.38.27.392.5:
        je if.38.24.392.5.end
        if.38.24.392.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.392.5.end:
    func.assert..392.5.end:
    mov qword [rbp + 880], -1
    mov qword [rbp + 888], 2
    func.assert..399.5:
        if.38.24.399.5:
        cmp.38.27.399.5:
        if.38.24.399.5.end:
    func.assert..399.5.end:
    cmp.400.12:
    cmp qword [rbp + 880], -1
    sete r15b
    bool.400.12.end:
    func.assert..400.5:
        if.38.24.400.5:
        cmp.38.27.400.5:
        je if.38.24.400.5.end
        if.38.24.400.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.400.5.end:
    func.assert..400.5.end:
    cmp.401.12:
    cmp qword [rbp + 888], 2
    sete r15b
    bool.401.12.end:
    func.assert..401.5:
        if.38.24.401.5:
        cmp.38.27.401.5:
        je if.38.24.401.5.end
        if.38.24.401.5.code:
            mov rdi, 1
            jmp baz_exit
        if.38.24.401.5.end:
    func.assert..401.5.end:
    mov qword [rbp + 896], 0
    xor al, al
    lea rdi, [rbp + 904]
    mov rcx, 128
    rep stosb
    func.print..405.5:
        mov rdi, 1
        lea rsi, [rbp - 384]
        mov rdx, 21
        call baz_write
    func.print..405.5.end:
    loop.406.5:
        inc qword [rbp + 896]
        jo baz_overflow_line_407
        mov r15, strict qword vars.end - size.func.print_num - 1032
        cmp r15, rbp
        jl baz_frame_overflow
        lea r15, [rbp + 896]
        mov qword [rbp + 1032], r15
        lea rbx, [rbp + 1032]
        call func.print_num
        func.print..409.9:
            mov rdi, 1
            lea rsi, [rbp - 323]
            mov rdx, 2
            call baz_write
        func.print..409.9.end:
        func.print..410.9:
            mov rdi, 1
            lea rsi, [rbp - 363]
            mov rdx, 12
            call baz_write
        func.print..410.9.end:
        func.str.input..411.12:
            xor edi, edi
            lea rsi, [rbp + 905]
            mov rdx, 127
            call baz_read
            mov qword [rbp + 1032], rax
            mov r15b, byte [rbp + 1032]
            mov byte [rbp + 904], r15b
            dec byte [rbp + 904]
        func.str.input..411.12.end:
        if.413.9:
        cmp.413.12:
        cmp byte [rbp + 904], 0
        jle loop.406.5.end
        if.413.9.code:
        if.413.9.1:
        cmp.415.19:
        cmp byte [rbp + 904], 4
        jg if.413.9.else
        if.413.9.1.code:
            func.print..416.13:
                mov rdi, 1
                lea rsi, [rbp - 351]
                mov rdx, 20
                call baz_write
            func.print..416.13.end:
            jmp loop.406.5
        if.413.9.else:
            func.greet..419.13:
                func.print..100.5.419.13:
                    mov rdi, 1
                    lea rsi, [rbp - 331]
                    mov rdx, 6
                    call baz_write
                func.print..100.5.419.13.end:
                func.str.print..101.10.419.13:
                    mov rdi, 1
                    lea rsi, [rbp + 905]
                    movsx rdx, byte [rbp + 904]
                    cmp rdx, 127
                    ja baz_bounds_line_96
                    call baz_write
                func.str.print..101.10.419.13.end:
                func.print..102.5.419.13:
                    mov rdi, 1
                    lea rsi, [rbp - 325]
                    mov rdx, 1
                    call baz_write
                func.print..102.5.419.13.end:
                func.print..103.5.419.13:
                    mov rdi, 1
                    lea rsi, [rbp - 324]
                    mov rdx, 1
                    call baz_write
                func.print..103.5.419.13.end:
                inc qword [rbp - 16]
                jo baz_overflow_line_104
            func.greet..419.13.end:
        if.413.9.end:
    jmp loop.406.5
    loop.406.5.end:
    func.print..423.5:
        mov rdi, 1
        lea rsi, [rbp - 32]
        mov rdx, 15
        call baz_write
    func.print..423.5.end:
    mov r15, strict qword vars.end - size.func.print_num - 1032
    cmp r15, rbp
    jl baz_frame_overflow
    lea r15, [rbp - 16]
    mov qword [rbp + 1032], r15
    lea rbx, [rbp + 1032]
    call func.print_num
    func.print..425.5:
        mov rdi, 1
        lea rsi, [rbp - 324]
        mov rdx, 1
        call baz_write
    func.print..425.5.end:
    mov dword [rbp + 1032], 543521122
    mov dword [rbp + 1036], 1836020326
    mov dword [rbp + 1040], 2053202464
    mov byte [rbp + 1044], 10
    mov rdi, 1
    lea rsi, [rbp + 1032]
    mov rdx, 3
    call baz_write
    mov rdi, 1
    lea rsi, [rbp + 1032]
    mov rdx, 1
    add rsi, 12
    call baz_write
    xor edi, edi
    jmp baz_exit
func.factorial:
    mov r15, qword [rbx]
    mov qword [r15], 1
    if.178.5:
    cmp.178.8:
    mov r15, qword [rbx + 8]
    cmp qword [r15], 1
    jg if.178.5.end
    if.178.5.code:
        ret
    if.178.5.end:
    mov r15, qword [rbx + 8]
    mov r15, qword [r15]
    mov qword [rbx + 16], r15
    dec qword [rbx + 16]
    jo baz_overflow_line_180
    mov r15, strict qword vars.end - size.func.factorial - 32
    cmp r15, rbx
    jl baz_frame_overflow
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
    jo baz_overflow_line_182
    mov qword [r15], r14
    ret
size.func.factorial equ 32
func.print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r15, qword [r15]
    mov qword [rbx + 32], r15
    mov byte [rbx + 40], 0
    if.145.5:
    cmp.145.8:
    cmp qword [rbx + 32], 0
    jge if.145.5.end
    if.145.5.code:
        mov byte [rbx + 40], 1
    if.145.5.end:
    if.148.5:
    cmp.148.8:
    cmp qword [rbx + 32], 0
    jle if.148.5.end
    if.148.5.code:
        neg qword [rbx + 32]
        jo baz_overflow_line_149
    if.148.5.end:
    mov qword [rbx + 48], 20
    loop.153.5:
        dec qword [rbx + 48]
        jo baz_overflow_line_154
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_155
        mov r14, 48
        mov rax, qword [rbx + 32]
        mov r12, 10
        cqo
        idiv r12
        mov r13, rdx
        sub r14, r13
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 32]
        mov r15, 10
        cqo
        idiv r15
        mov qword [rbx + 32], rax
        if.157.9:
        cmp.157.12:
        cmp qword [rbx + 32], 0
        jne loop.153.5
        if.157.9.code:
        if.157.9.end:
    loop.153.5.end:
    if.160.5:
    cmp.160.8:
    cmp byte [rbx + 40], 0
    je if.160.5.end
    if.160.5.code:
        dec qword [rbx + 48]
        jo baz_overflow_line_161
        mov r15, qword [rbx + 48]
        cmp r15, 20
        jae baz_bounds_line_162
        mov byte [rbx + r15 + 8], 45
    if.160.5.end:
    mov qword [rbx + 56], 0
    loop.166.5:
        mov r15, qword [rbx + 56]
        cmp r15, 20
        jae baz_bounds_line_167
        mov r14, qword [rbx + 48]
        cmp r14, 20
        jae baz_bounds_line_167
        mov r14b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r14b
        inc qword [rbx + 56]
        jo baz_overflow_line_168
        inc qword [rbx + 48]
        jo baz_overflow_line_169
        if.170.9:
        cmp.170.12:
        cmp qword [rbx + 48], 20
        jne loop.166.5
        if.170.9.code:
        if.170.9.end:
    loop.166.5.end:
    mov rdi, 1
    lea rsi, [rbx + 8]
    mov rdx, qword [rbx + 56]
    cmp rdx, 20
    ja baz_bounds_line_173
    call baz_write
    ret
size.func.print_num equ 64
baz_frame_overflow:
    mov rax, 1
    mov rdi, 2
    lea rsi, [baz_frame_overflow_message]
    mov rdx, 23
    syscall
    mov rdi, 254
    jmp baz_exit
section .rodata
baz_frame_overflow_message:
db `\npanic: frame overflow\n`
section .text
baz_overflow_line_55:
    mov rbp, 55
    jmp baz_overflow_panic_file_0
baz_overflow_line_67:
    mov rbp, 67
    jmp baz_overflow_panic_file_0
baz_overflow_line_104:
    mov rbp, 104
    jmp baz_overflow_panic_file_0
baz_overflow_line_149:
    mov rbp, 149
    jmp baz_overflow_panic_file_0
baz_overflow_line_154:
    mov rbp, 154
    jmp baz_overflow_panic_file_0
baz_overflow_line_161:
    mov rbp, 161
    jmp baz_overflow_panic_file_0
baz_overflow_line_168:
    mov rbp, 168
    jmp baz_overflow_panic_file_0
baz_overflow_line_169:
    mov rbp, 169
    jmp baz_overflow_panic_file_0
baz_overflow_line_180:
    mov rbp, 180
    jmp baz_overflow_panic_file_0
baz_overflow_line_182:
    mov rbp, 182
    jmp baz_overflow_panic_file_0
baz_overflow_line_212:
    mov rbp, 212
    jmp baz_overflow_panic_file_0
baz_overflow_line_215:
    mov rbp, 215
    jmp baz_overflow_panic_file_0
baz_overflow_line_216:
    mov rbp, 216
    jmp baz_overflow_panic_file_0
baz_overflow_line_246:
    mov rbp, 246
    jmp baz_overflow_panic_file_0
baz_overflow_line_272:
    mov rbp, 272
    jmp baz_overflow_panic_file_0
baz_overflow_line_281:
    mov rbp, 281
    jmp baz_overflow_panic_file_0
baz_overflow_line_344:
    mov rbp, 344
    jmp baz_overflow_panic_file_0
baz_overflow_line_349:
    mov rbp, 349
    jmp baz_overflow_panic_file_0
baz_overflow_line_407:
    mov rbp, 407
baz_overflow_panic_file_0:
    mov rax, 1
    mov rdi, 2
    lea rsi, [baz_overflow_panic_file_0_message]
    mov rdx, 29
    syscall
    mov rbx, 249
    jmp baz_report_line
section .rodata
baz_overflow_panic_file_0_message:
db `\npanic: overflow at prog.baz:`
section .text
baz_bounds_line_96:
    mov rbp, 96
    jmp baz_bounds_panic_file_0
baz_bounds_line_155:
    mov rbp, 155
    jmp baz_bounds_panic_file_0
baz_bounds_line_162:
    mov rbp, 162
    jmp baz_bounds_panic_file_0
baz_bounds_line_167:
    mov rbp, 167
    jmp baz_bounds_panic_file_0
baz_bounds_line_173:
    mov rbp, 173
    jmp baz_bounds_panic_file_0
baz_bounds_line_245:
    mov rbp, 245
    jmp baz_bounds_panic_file_0
baz_bounds_line_246:
    mov rbp, 246
    jmp baz_bounds_panic_file_0
baz_bounds_line_272:
    mov rbp, 272
    jmp baz_bounds_panic_file_0
baz_bounds_line_273:
    mov rbp, 273
    jmp baz_bounds_panic_file_0
baz_bounds_line_274:
    mov rbp, 274
baz_bounds_panic_file_0:
    mov rax, 1
    mov rdi, 2
    lea rsi, [baz_bounds_panic_file_0_message]
    mov rdx, 27
    syscall
    mov rbx, 255
section .rodata
baz_bounds_panic_file_0_message:
db `\npanic: bounds at prog.baz:`
section .text
baz_report_line:
    mov rax, rbp
    lea rdi, [rsp - 1]
    mov byte [rdi], 10
    dec rdi
    mov rcx, 10
.convert_loop:
    xor edx, edx
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jne .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdx, rsp
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rdi, rbx
baz_exit:
    mov rax, 60
    syscall
baz_read:
    test rdi, rdi
    jne baz_bad_descriptor
    xor eax, eax
    syscall
    ret
baz_write:
    lea rax, [rdi - 1]
    cmp rax, 2
    jae baz_bad_descriptor
    mov eax, 1
    syscall
    ret
baz_bad_descriptor:
    mov rax, -9
    ret
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
