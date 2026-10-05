default rel
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    xor al, al
    lea rdi, [rbp + 261424]
    mov rcx, 152
    rep stosb
    func.printer.set_silenced.982.9:
        cmp.55.21.982.9:
        bool.55.21.982.9.end:
        mov byte [rbp + 911], 1
    func.printer.set_silenced.982.9.end:
    func.run_creation_script.983.5:
        mov qword [rbp + 261576], 0
        mov qword [rbp + 261584], 0
        lea r15, [rbp + 261338]
        mov r14, 0
        foo.970.5.983.5:
            if.971.12.983.5:
            cmp.971.12.983.5:
            cmp byte [r15], 10
            jne if.971.9.983.5.end
            if.971.12.983.5.code:
                mov r13, r14
                sub r13, qword [rbp + 261576]
                func.tokenizer.set_line.972.16.983.5:
                    mov qword [rbp + 261560], 0
                    mov qword [rbp + 261568], 0
                    mov rcx, r13
                    mov r12, qword [rbp + 261576]
                    test r12, r12
                    js baz_bounds_line_398
                    test rcx, rcx
                    js baz_bounds_line_398
                    lea r10, [rcx + r12]
                    cmp r10, 81
                    jg baz_bounds_line_398
                    lea rsi, [rbp + r12 + 261338]
                    cmp rcx, 127
                    ja baz_bounds_line_398
                    lea rdi, [rbp + 261424]
                    rep movsb
                    mov qword [rbp + 261552], r13
                func.tokenizer.set_line.972.16.983.5.end:
                lea r13, [rbp + 261600]
                lea r12, [vars]
                cmp r13, r12
                jb baz_frame_overflow
                mov r12, strict qword vars.end
                cmp r13, r12
                ja baz_frame_overflow
                sub r12, r13
                mov r13, size.func.parse_input
                cmp r13, r12
                ja baz_frame_overflow
                lea r13, [rbp + 261584]
                mov qword [rbp + 261600], r13
                lea r13, [rbp + 261424]
                mov qword [rbp + 261608], r13
                push r15
                push r14
                lea rbx, [rbp + 261600]
                call func.parse_input
                pop r14
                pop r15
                mov qword [rbp + 261576], r14
                add qword [rbp + 261576], 1
            if.971.9.983.5.end:
            foo.970.5.983.5.continue:
                add r15, 1
                inc r14
                cmp r14, 81
                jne foo.970.5.983.5
        foo.970.5.983.5.end:
    func.run_creation_script.983.5.end:
    func.printer.set_silenced.984.9:
        cmp.55.21.984.9:
        bool.55.21.984.9.end:
        mov byte [rbp + 911], 0
    func.printer.set_silenced.984.9.end:
    lea r15, [rbp + 928]
    mov r14, qword [rbp + 88480]
    cmp r14, 32
    ja baz_bounds_line_987
    mov r13, 0
    cmp r14, 0
    jle foo.987.5.end
    foo.987.5:
        mov qword [r15 + 2728], 0
        foo.987.5.continue:
            add r15, 2736
            inc r13
            cmp r13, r14
            jne foo.987.5
    foo.987.5.end:
    func.printer.print_all.991.9:
        func.printer.print.69.10.991.9:
            func.printer.print_at.65.10.69.10.991.9:
                if.59.8.65.10.69.10.991.9:
                cmp.59.8.65.10.69.10.991.9:
                cmp byte [rbp + 911], 0
                jne func.printer.print_at.65.10.69.10.991.9.end
                if.59.8.65.10.69.10.991.9.code:
                if.59.5.65.10.69.10.991.9.end:
                mov rdi, 1
                mov rdx, 824
                mov r15, 0
                test r15, r15
                js baz_bounds_line_61
                test rdx, rdx
                js baz_bounds_line_61
                lea r14, [rdx + r15]
                cmp r14, 824
                jg baz_bounds_line_61
                lea rsi, [rbp]
                add rsi, r15
                mov rax, 1
                syscall
            func.printer.print_at.65.10.69.10.991.9.end:
        func.printer.print.69.10.991.9.end:
    func.printer.print_all.991.9.end:
    func.printer.print_all.992.9:
        func.printer.print.69.10.992.9:
            func.printer.print_at.65.10.69.10.992.9:
                if.59.8.65.10.69.10.992.9:
                cmp.59.8.65.10.69.10.992.9:
                cmp byte [rbp + 911], 0
                jne func.printer.print_at.65.10.69.10.992.9.end
                if.59.8.65.10.69.10.992.9.code:
                if.59.5.65.10.69.10.992.9.end:
                mov rdi, 1
                mov rdx, 41
                mov r15, 0
                test r15, r15
                js baz_bounds_line_61
                test rdx, rdx
                js baz_bounds_line_61
                lea r14, [rdx + r15]
                cmp r14, 41
                jg baz_bounds_line_61
                lea rsi, [rbp + 824]
                add rsi, r15
                mov rax, 1
                syscall
            func.printer.print_at.65.10.69.10.992.9.end:
        func.printer.print.69.10.992.9.end:
    func.printer.print_all.992.9.end:
    mov qword [rbp + 261576], 0
    loop.995.5:
        if.996.12:
        cmp.996.12:
        mov r15, qword [rbp + 88480]
        cmp qword [rbp + 261576], r15
        jne if.996.9.end
        if.996.12.code:
            mov qword [rbp + 261576], 0
        if.996.9.end:
        mov r15, qword [rbp + 261576]
        cmp r15, 32
        jae baz_bounds_line_999
        imul r15, 2736
        func.entity.print_messages.999.29:
            lea r14, [rbp + r15 + 1224]
            mov r13, qword [rbp + r15 + 3656]
            cmp r13, 16
            ja baz_bounds_line_252
            mov r12, 0
            cmp r13, 0
            jle foo.252.5.999.29.end
            foo.252.5.999.29:
                func.message.print.253.11.999.29:
                    mov r10, qword [r14]
                    cmp r10, 32
                    jae baz_bounds_line_238
                    imul r10, 2736
                    func.name.print.238.36.253.11.999.29:
                        func.printer.print.89.9.238.36.253.11.999.29:
                            func.printer.print_at.65.10.89.9.238.36.253.11.999.29:
                                if.59.8.65.10.89.9.238.36.253.11.999.29:
                                cmp.59.8.65.10.89.9.238.36.253.11.999.29:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.89.9.238.36.253.11.999.29.end
                                if.59.8.65.10.89.9.238.36.253.11.999.29.code:
                                if.59.5.65.10.89.9.238.36.253.11.999.29.end:
                                mov rdi, 1
                                mov rdx, qword [rbp + r10 + 944]
                                mov r9, 0
                                test r9, r9
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r8, [rdx + r9]
                                cmp r8, 16
                                jg baz_bounds_line_61
                                lea rsi, [rbp + r10 + 928]
                                add rsi, r9
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.89.9.238.36.253.11.999.29.end:
                        func.printer.print.89.9.238.36.253.11.999.29.end:
                    func.name.print.238.36.253.11.999.29.end:
                    if.239.8.253.11.999.29:
                    cmp.239.8.253.11.999.29:
                    cmp qword [r14 + 8], 1
                    jne if.239.5.253.11.999.29.end
                    if.239.8.253.11.999.29.code:
                        func.printer.print_all.240.13.253.11.999.29:
                            func.printer.print.69.10.240.13.253.11.999.29:
                                func.printer.print_at.65.10.69.10.240.13.253.11.999.29:
                                    if.59.8.65.10.69.10.240.13.253.11.999.29:
                                    cmp.59.8.65.10.69.10.240.13.253.11.999.29:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.240.13.253.11.999.29.end
                                    if.59.8.65.10.69.10.240.13.253.11.999.29.code:
                                    if.59.5.65.10.69.10.240.13.253.11.999.29.end:
                                    mov rdi, 1
                                    mov rdx, 6
                                    mov r10, 0
                                    test r10, r10
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r9, [rdx + r10]
                                    cmp r9, 6
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 920]
                                    add rsi, r10
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.240.13.253.11.999.29.end:
                            func.printer.print.69.10.240.13.253.11.999.29.end:
                        func.printer.print_all.240.13.253.11.999.29.end:
                    if.239.5.253.11.999.29.end:
                    if.242.8.253.11.999.29:
                    cmp.242.8.253.11.999.29:
                    cmp qword [r14 + 8], 0
                    jne if.242.5.253.11.999.29.end
                    if.242.8.253.11.999.29.code:
                        func.printer.print_all.243.13.253.11.999.29:
                            func.printer.print.69.10.243.13.253.11.999.29:
                                func.printer.print_at.65.10.69.10.243.13.253.11.999.29:
                                    if.59.8.65.10.69.10.243.13.253.11.999.29:
                                    cmp.59.8.65.10.69.10.243.13.253.11.999.29:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.243.13.253.11.999.29.end
                                    if.59.8.65.10.69.10.243.13.253.11.999.29.code:
                                    if.59.5.65.10.69.10.243.13.253.11.999.29.end:
                                    mov rdi, 1
                                    mov rdx, 8
                                    mov r10, 0
                                    test r10, r10
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r9, [rdx + r10]
                                    cmp r9, 8
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 912]
                                    add rsi, r10
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.243.13.253.11.999.29.end:
                            func.printer.print.69.10.243.13.253.11.999.29.end:
                        func.printer.print_all.243.13.253.11.999.29.end:
                    if.242.5.253.11.999.29.end:
                    if.245.8.253.11.999.29:
                    cmp.245.8.253.11.999.29:
                    cmp qword [r14 + 8], 2
                    jne if.245.5.253.11.999.29.end
                    if.245.8.253.11.999.29.code:
                        func.printer.print_all.246.13.253.11.999.29:
                            func.printer.print.69.10.246.13.253.11.999.29:
                                func.printer.print_at.65.10.69.10.246.13.253.11.999.29:
                                    if.59.8.65.10.69.10.246.13.253.11.999.29:
                                    cmp.59.8.65.10.69.10.246.13.253.11.999.29:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.246.13.253.11.999.29.end
                                    if.59.8.65.10.69.10.246.13.253.11.999.29.code:
                                    if.59.5.65.10.69.10.246.13.253.11.999.29.end:
                                    mov rdi, 1
                                    mov rdx, 1
                                    mov r10, 0
                                    test r10, r10
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r9, [rdx + r10]
                                    cmp r9, 1
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 926]
                                    add rsi, r10
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.246.13.253.11.999.29.end:
                            func.printer.print.69.10.246.13.253.11.999.29.end:
                        func.printer.print_all.246.13.253.11.999.29.end:
                    if.245.5.253.11.999.29.end:
                    func.str.print.248.15.253.11.999.29:
                        func.printer.print.89.9.248.15.253.11.999.29:
                            func.printer.print_at.65.10.89.9.248.15.253.11.999.29:
                                if.59.8.65.10.89.9.248.15.253.11.999.29:
                                cmp.59.8.65.10.89.9.248.15.253.11.999.29:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.89.9.248.15.253.11.999.29.end
                                if.59.8.65.10.89.9.248.15.253.11.999.29.code:
                                if.59.5.65.10.89.9.248.15.253.11.999.29.end:
                                mov rdi, 1
                                mov rdx, qword [r14 + 144]
                                mov r10, 0
                                test r10, r10
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r9, [rdx + r10]
                                cmp r9, 127
                                jg baz_bounds_line_61
                                lea rsi, [r14 + 16]
                                add rsi, r10
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.89.9.248.15.253.11.999.29.end:
                        func.printer.print.89.9.248.15.253.11.999.29.end:
                    func.str.print.248.15.253.11.999.29.end:
                func.message.print.253.11.999.29.end:
                func.printer.println.254.13.999.29:
                    func.printer.print_all.73.10.254.13.999.29:
                        func.printer.print.69.10.73.10.254.13.999.29:
                            func.printer.print_at.65.10.69.10.73.10.254.13.999.29:
                                if.59.8.65.10.69.10.73.10.254.13.999.29:
                                cmp.59.8.65.10.69.10.73.10.254.13.999.29:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.73.10.254.13.999.29.end
                                if.59.8.65.10.69.10.73.10.254.13.999.29.code:
                                if.59.5.65.10.69.10.73.10.254.13.999.29.end:
                                mov rdi, 1
                                mov rdx, 1
                                mov r10, 0
                                test r10, r10
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r9, [rdx + r10]
                                cmp r9, 1
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 879]
                                add rsi, r10
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.73.10.254.13.999.29.end:
                        func.printer.print.69.10.73.10.254.13.999.29.end:
                    func.printer.print_all.73.10.254.13.999.29.end:
                func.printer.println.254.13.999.29.end:
                foo.252.5.999.29.continue:
                    add r14, 152
                    inc r12
                    cmp r12, r13
                    jne foo.252.5.999.29
            foo.252.5.999.29.end:
            mov qword [rbp + r15 + 3656], 0
        func.entity.print_messages.999.29.end:
        func.printer.println.1000.13:
            func.printer.print_all.73.10.1000.13:
                func.printer.print.69.10.73.10.1000.13:
                    func.printer.print_at.65.10.69.10.73.10.1000.13:
                        if.59.8.65.10.69.10.73.10.1000.13:
                        cmp.59.8.65.10.69.10.73.10.1000.13:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.73.10.1000.13.end
                        if.59.8.65.10.69.10.73.10.1000.13.code:
                        if.59.5.65.10.69.10.73.10.1000.13.end:
                        mov rdi, 1
                        mov rdx, 1
                        mov r15, 0
                        test r15, r15
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r14, [rdx + r15]
                        cmp r14, 1
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 879]
                        add rsi, r15
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.73.10.1000.13.end:
                func.printer.print.69.10.73.10.1000.13.end:
            func.printer.print_all.73.10.1000.13.end:
        func.printer.println.1000.13.end:
        mov r14, qword [rbp + 261576]
        cmp r14, 32
        jae baz_bounds_line_1001
        imul r14, 2736
        mov r15, qword [rbp + r14 + 952]
        cmp r15, 128
        jae baz_bounds_line_1001
        imul r15, 960
        func.room.print.1001.50:
            if.300.8.1001.50:
            cmp.300.8.1001.50:
            cmp qword [rbp + r15 + 137680], 0
            je if.300.5.1001.50.end
            if.300.8.1001.50.code:
                func.printer.print_all.301.13.1001.50:
                    func.printer.print.69.10.301.13.1001.50:
                        func.printer.print_at.65.10.69.10.301.13.1001.50:
                            if.59.8.65.10.69.10.301.13.1001.50:
                            cmp.59.8.65.10.69.10.301.13.1001.50:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.301.13.1001.50.end
                            if.59.8.65.10.69.10.301.13.1001.50.code:
                            if.59.5.65.10.69.10.301.13.1001.50.end:
                            mov rdi, 1
                            mov rdx, 7
                            mov r14, 0
                            test r14, r14
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r13, [rdx + r14]
                            cmp r13, 7
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 137656]
                            add rsi, r14
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.301.13.1001.50.end:
                    func.printer.print.69.10.301.13.1001.50.end:
                func.printer.print_all.301.13.1001.50.end:
                func.name.print.302.19.1001.50:
                    func.printer.print.89.9.302.19.1001.50:
                        func.printer.print_at.65.10.89.9.302.19.1001.50:
                            if.59.8.65.10.89.9.302.19.1001.50:
                            cmp.59.8.65.10.89.9.302.19.1001.50:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.89.9.302.19.1001.50.end
                            if.59.8.65.10.89.9.302.19.1001.50.code:
                            if.59.5.65.10.89.9.302.19.1001.50.end:
                            mov rdi, 1
                            mov rdx, qword [rbp + r15 + 137680]
                            mov r14, 0
                            test r14, r14
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r13, [rdx + r14]
                            cmp r13, 16
                            jg baz_bounds_line_61
                            lea rsi, [rbp + r15 + 137664]
                            add rsi, r14
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.89.9.302.19.1001.50.end:
                    func.printer.print.89.9.302.19.1001.50.end:
                func.name.print.302.19.1001.50.end:
                func.printer.println.303.13.1001.50:
                    func.printer.print_all.73.10.303.13.1001.50:
                        func.printer.print.69.10.73.10.303.13.1001.50:
                            func.printer.print_at.65.10.69.10.73.10.303.13.1001.50:
                                if.59.8.65.10.69.10.73.10.303.13.1001.50:
                                cmp.59.8.65.10.69.10.73.10.303.13.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.73.10.303.13.1001.50.end
                                if.59.8.65.10.69.10.73.10.303.13.1001.50.code:
                                if.59.5.65.10.69.10.73.10.303.13.1001.50.end:
                                mov rdi, 1
                                mov rdx, 1
                                mov r14, 0
                                test r14, r14
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r13, [rdx + r14]
                                cmp r13, 1
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 879]
                                add rsi, r14
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.73.10.303.13.1001.50.end:
                        func.printer.print.69.10.73.10.303.13.1001.50.end:
                    func.printer.print_all.73.10.303.13.1001.50.end:
                func.printer.println.303.13.1001.50.end:
            if.300.5.1001.50.end:
            if.305.8.1001.50:
            cmp.305.8.1001.50:
            cmp qword [rbp + r15 + 137816], 0
            je if.305.5.1001.50.end
            if.305.8.1001.50.code:
                func.str.print.306.26.1001.50:
                    func.printer.print.89.9.306.26.1001.50:
                        func.printer.print_at.65.10.89.9.306.26.1001.50:
                            if.59.8.65.10.89.9.306.26.1001.50:
                            cmp.59.8.65.10.89.9.306.26.1001.50:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.89.9.306.26.1001.50.end
                            if.59.8.65.10.89.9.306.26.1001.50.code:
                            if.59.5.65.10.89.9.306.26.1001.50.end:
                            mov rdi, 1
                            mov rdx, qword [rbp + r15 + 137816]
                            mov r14, 0
                            test r14, r14
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r13, [rdx + r14]
                            cmp r13, 127
                            jg baz_bounds_line_61
                            lea rsi, [rbp + r15 + 137688]
                            add rsi, r14
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.89.9.306.26.1001.50.end:
                    func.printer.print.89.9.306.26.1001.50.end:
                func.str.print.306.26.1001.50.end:
                func.printer.println.307.13.1001.50:
                    func.printer.print_all.73.10.307.13.1001.50:
                        func.printer.print.69.10.73.10.307.13.1001.50:
                            func.printer.print_at.65.10.69.10.73.10.307.13.1001.50:
                                if.59.8.65.10.69.10.73.10.307.13.1001.50:
                                cmp.59.8.65.10.69.10.73.10.307.13.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.73.10.307.13.1001.50.end
                                if.59.8.65.10.69.10.73.10.307.13.1001.50.code:
                                if.59.5.65.10.69.10.73.10.307.13.1001.50.end:
                                mov rdi, 1
                                mov rdx, 1
                                mov r14, 0
                                test r14, r14
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r13, [rdx + r14]
                                cmp r13, 1
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 879]
                                add rsi, r14
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.73.10.307.13.1001.50.end:
                        func.printer.print.69.10.73.10.307.13.1001.50.end:
                    func.printer.print_all.73.10.307.13.1001.50.end:
                func.printer.println.307.13.1001.50.end:
            if.305.5.1001.50.end:
            mov byte [rbp + 261584], 0
            lea r14, [rbp + r15 + 137960]
            mov r13, qword [rbp + r15 + 138216]
            cmp r13, 32
            ja baz_bounds_line_310
            mov r12, 0
            cmp r13, 0
            jle foo.310.5.1001.50.end
            foo.310.5.1001.50:
                if.311.12.1001.50:
                cmp.311.12.1001.50:
                mov r10, qword [rbp + 261576]
                cmp qword [r14], r10
                je foo.310.5.1001.50.continue
                if.311.12.1001.50.code:
                if.311.9.1001.50.end:
                if.312.12.1001.50:
                cmp.312.12.1001.50:
                cmp byte [rbp + 261584], 0
                jne if.312.9.1001.50.else
                if.312.12.1001.50.code:
                    func.printer.print_all.313.17.1001.50:
                        func.printer.print.69.10.313.17.1001.50:
                            func.printer.print_at.65.10.69.10.313.17.1001.50:
                                if.59.8.65.10.69.10.313.17.1001.50:
                                cmp.59.8.65.10.69.10.313.17.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.313.17.1001.50.end
                                if.59.8.65.10.69.10.313.17.1001.50.code:
                                if.59.5.65.10.69.10.313.17.1001.50.end:
                                mov rdi, 1
                                mov rdx, 4
                                mov r10, 0
                                test r10, r10
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r9, [rdx + r10]
                                cmp r9, 4
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 865]
                                add rsi, r10
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.313.17.1001.50.end:
                        func.printer.print.69.10.313.17.1001.50.end:
                    func.printer.print_all.313.17.1001.50.end:
                jmp if.312.9.1001.50.end
                if.312.9.1001.50.else:
                    func.printer.print_all.315.17.1001.50:
                        func.printer.print.69.10.315.17.1001.50:
                            func.printer.print_at.65.10.69.10.315.17.1001.50:
                                if.59.8.65.10.69.10.315.17.1001.50:
                                cmp.59.8.65.10.69.10.315.17.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.315.17.1001.50.end
                                if.59.8.65.10.69.10.315.17.1001.50.code:
                                if.59.5.65.10.69.10.315.17.1001.50.end:
                                mov rdi, 1
                                mov rdx, 2
                                mov r10, 0
                                test r10, r10
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r9, [rdx + r10]
                                cmp r9, 2
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 883]
                                add rsi, r10
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.315.17.1001.50.end:
                        func.printer.print.69.10.315.17.1001.50.end:
                    func.printer.print_all.315.17.1001.50.end:
                if.312.9.1001.50.end:
                mov r10, qword [r14]
                cmp r10, 32
                jae baz_bounds_line_317
                imul r10, 2736
                func.entity.print.317.27.1001.50:
                    func.printer.print.224.9.317.27.1001.50:
                        func.printer.print_at.65.10.224.9.317.27.1001.50:
                            if.59.8.65.10.224.9.317.27.1001.50:
                            cmp.59.8.65.10.224.9.317.27.1001.50:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.224.9.317.27.1001.50.end
                            if.59.8.65.10.224.9.317.27.1001.50.code:
                            if.59.5.65.10.224.9.317.27.1001.50.end:
                            mov rdi, 1
                            mov rdx, qword [rbp + r10 + 944]
                            mov r9, 0
                            test r9, r9
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r8, [rdx + r9]
                            cmp r8, 16
                            jg baz_bounds_line_61
                            lea rsi, [rbp + r10 + 928]
                            add rsi, r9
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.224.9.317.27.1001.50.end:
                    func.printer.print.224.9.317.27.1001.50.end:
                func.entity.print.317.27.1001.50.end:
                mov byte [rbp + 261584], 1
                foo.310.5.1001.50.continue:
                    add r14, 8
                    inc r12
                    cmp r12, r13
                    jne foo.310.5.1001.50
            foo.310.5.1001.50.end:
            lea r14, [rbp + r15 + 138360]
            mov r13, qword [rbp + r15 + 138616]
            cmp r13, 32
            ja baz_bounds_line_320
            mov r12, 0
            cmp r13, 0
            jle foo.320.5.1001.50.end
            foo.320.5.1001.50:
                if.321.12.1001.50:
                cmp.321.12.1001.50:
                cmp byte [rbp + 261584], 0
                jne if.321.9.1001.50.else
                if.321.12.1001.50.code:
                    func.printer.print_all.322.17.1001.50:
                        func.printer.print.69.10.322.17.1001.50:
                            func.printer.print_at.65.10.69.10.322.17.1001.50:
                                if.59.8.65.10.69.10.322.17.1001.50:
                                cmp.59.8.65.10.69.10.322.17.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.322.17.1001.50.end
                                if.59.8.65.10.69.10.322.17.1001.50.code:
                                if.59.5.65.10.69.10.322.17.1001.50.end:
                                mov rdi, 1
                                mov rdx, 4
                                mov r10, 0
                                test r10, r10
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r9, [rdx + r10]
                                cmp r9, 4
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 865]
                                add rsi, r10
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.322.17.1001.50.end:
                        func.printer.print.69.10.322.17.1001.50.end:
                    func.printer.print_all.322.17.1001.50.end:
                jmp if.321.9.1001.50.end
                if.321.9.1001.50.else:
                    func.printer.print_all.324.17.1001.50:
                        func.printer.print.69.10.324.17.1001.50:
                            func.printer.print_at.65.10.69.10.324.17.1001.50:
                                if.59.8.65.10.69.10.324.17.1001.50:
                                cmp.59.8.65.10.69.10.324.17.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.324.17.1001.50.end
                                if.59.8.65.10.69.10.324.17.1001.50.code:
                                if.59.5.65.10.69.10.324.17.1001.50.end:
                                mov rdi, 1
                                mov rdx, 2
                                mov r10, 0
                                test r10, r10
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r9, [rdx + r10]
                                cmp r9, 2
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 883]
                                add rsi, r10
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.324.17.1001.50.end:
                        func.printer.print.69.10.324.17.1001.50.end:
                    func.printer.print_all.324.17.1001.50.end:
                if.321.9.1001.50.end:
                mov r10, qword [r14]
                cmp r10, 1024
                jae baz_bounds_line_326
                imul r10, 24
                func.name.print.326.31.1001.50:
                    func.printer.print.89.9.326.31.1001.50:
                        func.printer.print_at.65.10.89.9.326.31.1001.50:
                            if.59.8.65.10.89.9.326.31.1001.50:
                            cmp.59.8.65.10.89.9.326.31.1001.50:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.89.9.326.31.1001.50.end
                            if.59.8.65.10.89.9.326.31.1001.50.code:
                            if.59.5.65.10.89.9.326.31.1001.50.end:
                            mov rdi, 1
                            mov rdx, qword [rbp + r10 + 113088]
                            mov r9, 0
                            test r9, r9
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r8, [rdx + r9]
                            cmp r8, 16
                            jg baz_bounds_line_61
                            lea rsi, [rbp + r10 + 113072]
                            add rsi, r9
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.89.9.326.31.1001.50.end:
                    func.printer.print.89.9.326.31.1001.50.end:
                func.name.print.326.31.1001.50.end:
                mov byte [rbp + 261584], 1
                foo.320.5.1001.50.continue:
                    add r14, 8
                    inc r12
                    cmp r12, r13
                    jne foo.320.5.1001.50
            foo.320.5.1001.50.end:
            if.329.8.1001.50:
            cmp.329.8.1001.50:
            cmp byte [rbp + 261584], 0
            je if.329.5.1001.50.end
            if.329.8.1001.50.code:
                func.printer.println.330.13.1001.50:
                    func.printer.print_all.73.10.330.13.1001.50:
                        func.printer.print.69.10.73.10.330.13.1001.50:
                            func.printer.print_at.65.10.69.10.73.10.330.13.1001.50:
                                if.59.8.65.10.69.10.73.10.330.13.1001.50:
                                cmp.59.8.65.10.69.10.73.10.330.13.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.73.10.330.13.1001.50.end
                                if.59.8.65.10.69.10.73.10.330.13.1001.50.code:
                                if.59.5.65.10.69.10.73.10.330.13.1001.50.end:
                                mov rdi, 1
                                mov rdx, 1
                                mov r14, 0
                                test r14, r14
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r13, [rdx + r14]
                                cmp r13, 1
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 879]
                                add rsi, r14
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.73.10.330.13.1001.50.end:
                        func.printer.print.69.10.73.10.330.13.1001.50.end:
                    func.printer.print_all.73.10.330.13.1001.50.end:
                func.printer.println.330.13.1001.50.end:
            if.329.5.1001.50.end:
            if.332.7.1001.50:
            cmp.332.7.1001.50:
            cmp.332.8.1001.50:
            cmp qword [rbp + r15 + 138352], 0
            je if.332.5.1001.50.end
            if.332.7.1001.50.code:
                func.printer.print_all.333.13.1001.50:
                    func.printer.print.69.10.333.13.1001.50:
                        func.printer.print_at.65.10.69.10.333.13.1001.50:
                            if.59.8.65.10.69.10.333.13.1001.50:
                            cmp.59.8.65.10.69.10.333.13.1001.50:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.333.13.1001.50.end
                            if.59.8.65.10.69.10.333.13.1001.50.code:
                            if.59.5.65.10.69.10.333.13.1001.50.end:
                            mov rdi, 1
                            mov rdx, 7
                            mov r14, 0
                            test r14, r14
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r13, [rdx + r14]
                            cmp r13, 7
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 869]
                            add rsi, r14
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.333.13.1001.50.end:
                    func.printer.print.69.10.333.13.1001.50.end:
                func.printer.print_all.333.13.1001.50.end:
                lea r14, [rbp + r15 + 138224]
                mov r13, qword [rbp + r15 + 138352]
                cmp r13, 8
                ja baz_bounds_line_334
                mov r12, 0
                cmp r13, 0
                jle foo.334.9.1001.50.end
                foo.334.9.1001.50:
                    if.335.16.1001.50:
                    cmp.335.16.1001.50:
                    cmp r12, 0
                    je if.335.13.1001.50.end
                    if.335.16.1001.50.code:
                        func.printer.print_all.335.27.1001.50:
                            func.printer.print.69.10.335.27.1001.50:
                                func.printer.print_at.65.10.69.10.335.27.1001.50:
                                    if.59.8.65.10.69.10.335.27.1001.50:
                                    cmp.59.8.65.10.69.10.335.27.1001.50:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.335.27.1001.50.end
                                    if.59.8.65.10.69.10.335.27.1001.50.code:
                                    if.59.5.65.10.69.10.335.27.1001.50.end:
                                    mov rdi, 1
                                    mov rdx, 2
                                    mov r10, 0
                                    test r10, r10
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r9, [rdx + r10]
                                    cmp r9, 2
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 883]
                                    add rsi, r10
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.335.27.1001.50.end:
                            func.printer.print.69.10.335.27.1001.50.end:
                        func.printer.print_all.335.27.1001.50.end:
                    if.335.13.1001.50.end:
                    mov r10, qword [r14]
                    cmp r10, 1024
                    jae baz_bounds_line_336
                    imul r10, 24
                    func.name.print.336.46.1001.50:
                        func.printer.print.89.9.336.46.1001.50:
                            func.printer.print_at.65.10.89.9.336.46.1001.50:
                                if.59.8.65.10.89.9.336.46.1001.50:
                                cmp.59.8.65.10.89.9.336.46.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.89.9.336.46.1001.50.end
                                if.59.8.65.10.89.9.336.46.1001.50.code:
                                if.59.5.65.10.89.9.336.46.1001.50.end:
                                mov rdi, 1
                                mov rdx, qword [rbp + r10 + 88504]
                                mov r9, 0
                                test r9, r9
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r8, [rdx + r9]
                                cmp r8, 16
                                jg baz_bounds_line_61
                                lea rsi, [rbp + r10 + 88488]
                                add rsi, r9
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.89.9.336.46.1001.50.end:
                        func.printer.print.89.9.336.46.1001.50.end:
                    func.name.print.336.46.1001.50.end:
                    foo.334.9.1001.50.continue:
                        add r14, 16
                        inc r12
                        cmp r12, r13
                        jne foo.334.9.1001.50
                foo.334.9.1001.50.end:
                func.printer.println.338.13.1001.50:
                    func.printer.print_all.73.10.338.13.1001.50:
                        func.printer.print.69.10.73.10.338.13.1001.50:
                            func.printer.print_at.65.10.69.10.73.10.338.13.1001.50:
                                if.59.8.65.10.69.10.73.10.338.13.1001.50:
                                cmp.59.8.65.10.69.10.73.10.338.13.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.73.10.338.13.1001.50.end
                                if.59.8.65.10.69.10.73.10.338.13.1001.50.code:
                                if.59.5.65.10.69.10.73.10.338.13.1001.50.end:
                                mov rdi, 1
                                mov rdx, 1
                                mov r14, 0
                                test r14, r14
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r13, [rdx + r14]
                                cmp r13, 1
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 879]
                                add rsi, r14
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.73.10.338.13.1001.50.end:
                        func.printer.print.69.10.73.10.338.13.1001.50.end:
                    func.printer.print_all.73.10.338.13.1001.50.end:
                func.printer.println.338.13.1001.50.end:
            if.332.5.1001.50.end:
            if.340.8.1001.50:
            cmp.340.8.1001.50:
            cmp qword [rbp + r15 + 137952], 0
            je if.340.5.1001.50.end
            if.340.8.1001.50.code:
                func.str.print.341.19.1001.50:
                    func.printer.print.89.9.341.19.1001.50:
                        func.printer.print_at.65.10.89.9.341.19.1001.50:
                            if.59.8.65.10.89.9.341.19.1001.50:
                            cmp.59.8.65.10.89.9.341.19.1001.50:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.89.9.341.19.1001.50.end
                            if.59.8.65.10.89.9.341.19.1001.50.code:
                            if.59.5.65.10.89.9.341.19.1001.50.end:
                            mov rdi, 1
                            mov rdx, qword [rbp + r15 + 137952]
                            mov r14, 0
                            test r14, r14
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r13, [rdx + r14]
                            cmp r13, 127
                            jg baz_bounds_line_61
                            lea rsi, [rbp + r15 + 137824]
                            add rsi, r14
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.89.9.341.19.1001.50.end:
                    func.printer.print.89.9.341.19.1001.50.end:
                func.str.print.341.19.1001.50.end:
                func.printer.println.342.13.1001.50:
                    func.printer.print_all.73.10.342.13.1001.50:
                        func.printer.print.69.10.73.10.342.13.1001.50:
                            func.printer.print_at.65.10.69.10.73.10.342.13.1001.50:
                                if.59.8.65.10.69.10.73.10.342.13.1001.50:
                                cmp.59.8.65.10.69.10.73.10.342.13.1001.50:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.73.10.342.13.1001.50.end
                                if.59.8.65.10.69.10.73.10.342.13.1001.50.code:
                                if.59.5.65.10.69.10.73.10.342.13.1001.50.end:
                                mov rdi, 1
                                mov rdx, 1
                                mov r14, 0
                                test r14, r14
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r13, [rdx + r14]
                                cmp r13, 1
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 879]
                                add rsi, r14
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.73.10.342.13.1001.50.end:
                        func.printer.print.69.10.73.10.342.13.1001.50.end:
                    func.printer.print_all.73.10.342.13.1001.50.end:
                func.printer.println.342.13.1001.50.end:
            if.340.5.1001.50.end:
        func.room.print.1001.50.end:
        mov r15, qword [rbp + 261576]
        cmp r15, 32
        jae baz_bounds_line_1002
        imul r15, 2736
        func.name.print.1002.34:
            func.printer.print.89.9.1002.34:
                func.printer.print_at.65.10.89.9.1002.34:
                    if.59.8.65.10.89.9.1002.34:
                    cmp.59.8.65.10.89.9.1002.34:
                    cmp byte [rbp + 911], 0
                    jne func.printer.print_at.65.10.89.9.1002.34.end
                    if.59.8.65.10.89.9.1002.34.code:
                    if.59.5.65.10.89.9.1002.34.end:
                    mov rdi, 1
                    mov rdx, qword [rbp + r15 + 944]
                    mov r14, 0
                    test r14, r14
                    js baz_bounds_line_61
                    test rdx, rdx
                    js baz_bounds_line_61
                    lea r13, [rdx + r14]
                    cmp r13, 16
                    jg baz_bounds_line_61
                    lea rsi, [rbp + r15 + 928]
                    add rsi, r14
                    mov rax, 1
                    syscall
                func.printer.print_at.65.10.89.9.1002.34.end:
            func.printer.print.89.9.1002.34.end:
        func.name.print.1002.34.end:
        func.printer.print_all.1003.13:
            func.printer.print.69.10.1003.13:
                func.printer.print_at.65.10.69.10.1003.13:
                    if.59.8.65.10.69.10.1003.13:
                    cmp.59.8.65.10.69.10.1003.13:
                    cmp byte [rbp + 911], 0
                    jne func.printer.print_at.65.10.69.10.1003.13.end
                    if.59.8.65.10.69.10.1003.13.code:
                    if.59.5.65.10.69.10.1003.13.end:
                    mov rdi, 1
                    mov rdx, 3
                    mov r15, 0
                    test r15, r15
                    js baz_bounds_line_61
                    test rdx, rdx
                    js baz_bounds_line_61
                    lea r14, [rdx + r15]
                    cmp r14, 3
                    jg baz_bounds_line_61
                    lea rsi, [rbp + 876]
                    add rsi, r15
                    mov rax, 1
                    syscall
                func.printer.print_at.65.10.69.10.1003.13.end:
            func.printer.print.69.10.1003.13.end:
        func.printer.print_all.1003.13.end:
        func.tokenizer.input.1004.12:
            mov qword [rbp + 261560], 0
            mov qword [rbp + 261568], 0
            func.str.input.392.14.1004.12:
                mov qword [rbp + 261584], 0
                loop.106.5.392.14.1004.12:
                    if.107.12.392.14.1004.12:
                    cmp.107.12.392.14.1004.12:
                    cmp qword [rbp + 261584], 127
                    je loop.106.5.392.14.1004.12.end
                    if.107.12.392.14.1004.12.code:
                    if.107.9.392.14.1004.12.end:
                    if.108.12.392.14.1004.12:
                    cmp.108.12.392.14.1004.12:
                        mov rdi, 0
                        mov rdx, 1
                        mov r14, qword [rbp + 261584]
                        test r14, r14
                        js baz_bounds_line_108
                        test rdx, rdx
                        js baz_bounds_line_108
                        lea r13, [rdx + r14]
                        cmp r13, 127
                        jg baz_bounds_line_108
                        lea rsi, [rbp + 261424]
                        add rsi, r14
                        mov rax, 0
                        syscall
                        mov r15, rax
                    cmp r15, 0
                    je loop.106.5.392.14.1004.12.end
                    if.108.12.392.14.1004.12.code:
                    if.108.9.392.14.1004.12.end:
                    if.109.12.392.14.1004.12:
                    cmp.109.12.392.14.1004.12:
                    mov r15, qword [rbp + 261584]
                    cmp r15, 127
                    jae baz_bounds_line_109
                    cmp byte [rbp + r15 + 261424], 127
                    jne if.109.9.392.14.1004.12.end
                    if.109.12.392.14.1004.12.code:
                        if.110.16.392.14.1004.12:
                        cmp.110.16.392.14.1004.12:
                        cmp qword [rbp + 261584], 0
                        jle if.110.13.392.14.1004.12.end
                        if.110.16.392.14.1004.12.code:
                            sub qword [rbp + 261584], 1
                            func.printer.print_all.112.21.392.14.1004.12:
                                func.printer.print.69.10.112.21.392.14.1004.12:
                                    func.printer.print_at.65.10.69.10.112.21.392.14.1004.12:
                                        if.59.8.65.10.69.10.112.21.392.14.1004.12:
                                        cmp.59.8.65.10.69.10.112.21.392.14.1004.12:
                                        cmp byte [rbp + 911], 0
                                        jne func.printer.print_at.65.10.69.10.112.21.392.14.1004.12.end
                                        if.59.8.65.10.69.10.112.21.392.14.1004.12.code:
                                        if.59.5.65.10.69.10.112.21.392.14.1004.12.end:
                                        mov rdi, 1
                                        mov rdx, 3
                                        mov r15, 0
                                        test r15, r15
                                        js baz_bounds_line_61
                                        test rdx, rdx
                                        js baz_bounds_line_61
                                        lea r14, [rdx + r15]
                                        cmp r14, 3
                                        jg baz_bounds_line_61
                                        lea rsi, [rbp + 885]
                                        add rsi, r15
                                        mov rax, 1
                                        syscall
                                    func.printer.print_at.65.10.69.10.112.21.392.14.1004.12.end:
                                func.printer.print.69.10.112.21.392.14.1004.12.end:
                            func.printer.print_all.112.21.392.14.1004.12.end:
                        if.110.13.392.14.1004.12.end:
                        jmp loop.106.5.392.14.1004.12
                    if.109.9.392.14.1004.12.end:
                    func.printer.print_at.116.13.392.14.1004.12:
                        if.59.8.116.13.392.14.1004.12:
                        cmp.59.8.116.13.392.14.1004.12:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.116.13.392.14.1004.12.end
                        if.59.8.116.13.392.14.1004.12.code:
                        if.59.5.116.13.392.14.1004.12.end:
                        mov rdi, 1
                        mov rdx, 1
                        mov r15, qword [rbp + 261584]
                        test r15, r15
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r14, [rdx + r15]
                        cmp r14, 127
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 261424]
                        add rsi, r15
                        mov rax, 1
                        syscall
                    func.printer.print_at.116.13.392.14.1004.12.end:
                    if.117.12.392.14.1004.12:
                    cmp.117.12.392.14.1004.12:
                    mov r15, qword [rbp + 261584]
                    cmp r15, 127
                    jae baz_bounds_line_117
                    cmp byte [rbp + r15 + 261424], 10
                    je loop.106.5.392.14.1004.12.end
                    if.117.12.392.14.1004.12.code:
                    if.117.9.392.14.1004.12.end:
                    add qword [rbp + 261584], 1
                jmp loop.106.5.392.14.1004.12
                loop.106.5.392.14.1004.12.end:
                mov r15, qword [rbp + 261584]
                mov qword [rbp + 261552], r15
            func.str.input.392.14.1004.12.end:
        func.tokenizer.input.1004.12.end:
        func.printer.println.1005.13:
            func.printer.print_all.73.10.1005.13:
                func.printer.print.69.10.73.10.1005.13:
                    func.printer.print_at.65.10.69.10.73.10.1005.13:
                        if.59.8.65.10.69.10.73.10.1005.13:
                        cmp.59.8.65.10.69.10.73.10.1005.13:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.73.10.1005.13.end
                        if.59.8.65.10.69.10.73.10.1005.13.code:
                        if.59.5.65.10.69.10.73.10.1005.13.end:
                        mov rdi, 1
                        mov rdx, 1
                        mov r15, 0
                        test r15, r15
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r14, [rdx + r15]
                        cmp r14, 1
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 879]
                        add rsi, r15
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.73.10.1005.13.end:
                func.printer.print.69.10.73.10.1005.13.end:
            func.printer.print_all.73.10.1005.13.end:
        func.printer.println.1005.13.end:
        lea r15, [rbp + 261584]
        lea r14, [vars]
        cmp r15, r14
        jb baz_frame_overflow
        mov r14, strict qword vars.end
        cmp r15, r14
        ja baz_frame_overflow
        sub r14, r15
        mov r15, size.func.parse_input
        cmp r15, r14
        ja baz_frame_overflow
        lea r15, [rbp + 261576]
        mov qword [rbp + 261584], r15
        lea r15, [rbp + 261424]
        mov qword [rbp + 261592], r15
        lea rbx, [rbp + 261584]
        call func.parse_input
        func.printer.print_all.1007.13:
            func.printer.print.69.10.1007.13:
                func.printer.print_at.65.10.69.10.1007.13:
                    if.59.8.65.10.69.10.1007.13:
                    cmp.59.8.65.10.69.10.1007.13:
                    cmp byte [rbp + 911], 0
                    jne func.printer.print_at.65.10.69.10.1007.13.end
                    if.59.8.65.10.69.10.1007.13.code:
                    if.59.5.65.10.69.10.1007.13.end:
                    mov rdi, 1
                    mov rdx, 3
                    mov r15, 0
                    test r15, r15
                    js baz_bounds_line_61
                    test rdx, rdx
                    js baz_bounds_line_61
                    lea r14, [rdx + r15]
                    cmp r14, 3
                    jg baz_bounds_line_61
                    lea rsi, [rbp + 880]
                    add rsi, r15
                    mov rax, 1
                    syscall
                func.printer.print_at.65.10.69.10.1007.13.end:
            func.printer.print.69.10.1007.13.end:
        func.printer.print_all.1007.13.end:
        add qword [rbp + 261576], 1
    jmp loop.995.5
    loop.995.5.end:
func.parse_input:
    mov r15, qword [rbx + 8]
    func.tokenizer.first.911.8:
        func.tokenizer.skip_whitespace.416.10.911.8:
            mov r14, qword [r15 + 144]
            mov qword [r15 + 136], r14
            loop.404.5.416.10.911.8:
                if.405.12.416.10.911.8:
                cmp.405.12.416.10.911.8:
                mov r14, qword [r15 + 128]
                cmp qword [r15 + 136], r14
                jl if.405.9.416.10.911.8.end
                if.405.12.416.10.911.8.code:
                    mov r14, qword [r15 + 136]
                    mov qword [r15 + 144], r14
                    jmp func.tokenizer.skip_whitespace.416.10.911.8.end
                if.405.9.416.10.911.8.end:
                if.409.12.416.10.911.8:
                cmp.409.12.416.10.911.8:
                mov r14, qword [r15 + 136]
                cmp r14, 127
                jae baz_bounds_line_409
                cmp byte [r15 + r14], 32
                jne loop.404.5.416.10.911.8.end
                if.409.12.416.10.911.8.code:
                if.409.9.416.10.911.8.end:
                add qword [r15 + 136], 1
            jmp loop.404.5.416.10.911.8
            loop.404.5.416.10.911.8.end:
            mov r14, qword [r15 + 136]
            mov qword [r15 + 144], r14
        func.tokenizer.skip_whitespace.416.10.911.8.end:
        mov r14, qword [r15 + 136]
        mov qword [r15 + 144], r14
        loop.418.5.911.8:
            if.419.12.911.8:
            cmp.419.12.911.8:
            mov r14, qword [r15 + 128]
            cmp qword [r15 + 144], r14
            jge loop.418.5.911.8.end
            if.419.12.911.8.code:
            if.419.9.911.8.end:
            if.420.12.911.8:
            cmp.420.12.911.8:
            mov r14, qword [r15 + 144]
            cmp r14, 127
            jae baz_bounds_line_420
            cmp byte [r15 + r14], 32
            je loop.418.5.911.8.end
            if.420.12.911.8.code:
            if.420.9.911.8.end:
            add qword [r15 + 144], 1
        jmp loop.418.5.911.8
        loop.418.5.911.8.end:
    func.tokenizer.first.911.8.end:
    if.913.8:
    cmp.913.8:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.913.11:
            cmp.444.11.913.11:
                func.tokenizer.len.444.16.913.11:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.913.11.end:
            cmp r13, 2
            sete r15b
            jne bool.444.11.913.11.end
            cmp.445.11.913.11:
                mov rcx, 2
                cmp rcx, 2
                ja baz_bounds_line_445
                lea rsi, [rbp + 261317]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.913.11.end:
        func.tokenizer.is_array.913.11.end:
    cmp r15b, 0
    je if.916.15
    if.913.8.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_go.914.9:
            if.499.8.914.9:
            cmp.499.8.914.9:
                func.tokenizer.next_or_say.499.15.914.9:
                    func.tokenizer.next.476.10.499.15.914.9:
                        func.tokenizer.skip_whitespace.426.10.476.10.499.15.914.9:
                            mov r12, qword [r14 + 144]
                            mov qword [r14 + 136], r12
                            loop.404.5.426.10.476.10.499.15.914.9:
                                if.405.12.426.10.476.10.499.15.914.9:
                                cmp.405.12.426.10.476.10.499.15.914.9:
                                mov r12, qword [r14 + 128]
                                cmp qword [r14 + 136], r12
                                jl if.405.9.426.10.476.10.499.15.914.9.end
                                if.405.12.426.10.476.10.499.15.914.9.code:
                                    mov r12, qword [r14 + 136]
                                    mov qword [r14 + 144], r12
                                    jmp func.tokenizer.skip_whitespace.426.10.476.10.499.15.914.9.end
                                if.405.9.426.10.476.10.499.15.914.9.end:
                                if.409.12.426.10.476.10.499.15.914.9:
                                cmp.409.12.426.10.476.10.499.15.914.9:
                                mov r12, qword [r14 + 136]
                                cmp r12, 127
                                jae baz_bounds_line_409
                                cmp byte [r14 + r12], 32
                                jne loop.404.5.426.10.476.10.499.15.914.9.end
                                if.409.12.426.10.476.10.499.15.914.9.code:
                                if.409.9.426.10.476.10.499.15.914.9.end:
                                add qword [r14 + 136], 1
                            jmp loop.404.5.426.10.476.10.499.15.914.9
                            loop.404.5.426.10.476.10.499.15.914.9.end:
                            mov r12, qword [r14 + 136]
                            mov qword [r14 + 144], r12
                        func.tokenizer.skip_whitespace.426.10.476.10.499.15.914.9.end:
                        mov r12, qword [r14 + 136]
                        mov qword [r14 + 144], r12
                        loop.428.5.476.10.499.15.914.9:
                            if.429.12.476.10.499.15.914.9:
                            cmp.429.12.476.10.499.15.914.9:
                            mov r12, qword [r14 + 128]
                            cmp qword [r14 + 144], r12
                            jge loop.428.5.476.10.499.15.914.9.end
                            if.429.12.476.10.499.15.914.9.code:
                            if.429.9.476.10.499.15.914.9.end:
                            if.430.12.476.10.499.15.914.9:
                            cmp.430.12.476.10.499.15.914.9:
                            mov r12, qword [r14 + 144]
                            cmp r12, 127
                            jae baz_bounds_line_430
                            cmp byte [r14 + r12], 32
                            je loop.428.5.476.10.499.15.914.9.end
                            if.430.12.476.10.499.15.914.9.code:
                            if.430.9.476.10.499.15.914.9.end:
                            add qword [r14 + 144], 1
                        jmp loop.428.5.476.10.499.15.914.9
                        loop.428.5.476.10.499.15.914.9.end:
                    func.tokenizer.next.476.10.499.15.914.9.end:
                    cmp.477.11.499.15.914.9:
                        func.tokenizer.is_empty.477.20.499.15.914.9:
                            cmp.454.11.477.20.499.15.914.9:
                            mov r12, qword [r14 + 144]
                            cmp qword [r14 + 136], r12
                            sete r13b
                            bool.454.11.477.20.499.15.914.9.end:
                        func.tokenizer.is_empty.477.20.499.15.914.9.end:
                    cmp r13b, 0
                    sete r13b
                    bool.477.11.499.15.914.9.end:
                    if.478.8.499.15.914.9:
                    cmp.478.8.499.15.914.9:
                    cmp r13b, 0
                    jne if.478.5.499.15.914.9.end
                    if.478.8.499.15.914.9.code:
                        func.printer.print_all.478.20.499.15.914.9:
                            func.printer.print.69.10.478.20.499.15.914.9:
                                func.printer.print_at.65.10.69.10.478.20.499.15.914.9:
                                    if.59.8.65.10.69.10.478.20.499.15.914.9:
                                    cmp.59.8.65.10.69.10.478.20.499.15.914.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.478.20.499.15.914.9.end
                                    if.59.8.65.10.69.10.478.20.499.15.914.9.code:
                                    if.59.5.65.10.69.10.478.20.499.15.914.9.end:
                                    mov rdi, 1
                                    mov rdx, 9
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 9
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260560]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.478.20.499.15.914.9.end:
                            func.printer.print.69.10.478.20.499.15.914.9.end:
                        func.printer.print_all.478.20.499.15.914.9.end:
                    if.478.5.499.15.914.9.end:
                func.tokenizer.next_or_say.499.15.914.9.end:
            cmp r13b, 0
            je func.action_go.914.9.end
            if.499.8.914.9.code:
            if.499.5.914.9.end:
            if.501.8.914.9:
            cmp.501.8.914.9:
                func.tokenizer.is_array.501.11.914.9:
                    cmp.444.11.501.11.914.9:
                        func.tokenizer.len.444.16.501.11.914.9:
                            mov r12, qword [r14 + 144]
                            sub r12, qword [r14 + 136]
                        func.tokenizer.len.444.16.501.11.914.9.end:
                    cmp r12, 4
                    sete r13b
                    jne bool.444.11.501.11.914.9.end
                    cmp.445.11.501.11.914.9:
                        mov rcx, 4
                        cmp rcx, 4
                        ja baz_bounds_line_445
                        lea rsi, [rbp + 260585]
                        mov r12, qword [r14 + 136]
                        test r12, r12
                        js baz_bounds_line_445
                        lea r10, [rcx + r12]
                        cmp r10, 127
                        jg baz_bounds_line_445
                        lea rdi, [r14 + r12]
                        test rcx, rcx
                        repe cmpsb
                        sete r13b
                    bool.444.11.501.11.914.9.end:
                func.tokenizer.is_array.501.11.914.9.end:
            cmp r13b, 0
            je if.501.5.914.9.end
            if.501.8.914.9.code:
                mov rdi, 0
                mov rax, 60
                syscall
            if.501.5.914.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_504
            imul r13, 2736
            mov r12, qword [rbp + r13 + 952]
            mov qword [rbx + 16], r12
            mov r13, qword [rbx + 16]
            cmp r13, 128
            jae baz_bounds_line_506
            imul r13, 960
            mov r12, qword [rbp + r13 + 138352]
            mov qword [rbx + 24], r12
            mov r13, qword [rbx + 24]
            mov qword [rbx + 32], r13
            mov r12, qword [rbx + 16]
            cmp r12, 128
            jae baz_bounds_line_508
            imul r12, 960
            lea r13, [rbp + r12 + 138224]
            mov r12, qword [rbx + 24]
            cmp r12, 8
            ja baz_bounds_line_508
            mov r10, 0
            cmp r12, 0
            jle foo.508.5.914.9.end
            foo.508.5.914.9:
                if.509.12.914.9:
                cmp.509.12.914.9:
                    mov r8, qword [r13]
                    cmp r8, 1024
                    jae baz_bounds_line_509
                    imul r8, 24
                    func.tokenizer.is.name.509.15.914.9:
                        cmp.449.11.509.15.914.9:
                            func.tokenizer.len.449.16.509.15.914.9:
                                mov r11, qword [r14 + 144]
                                sub r11, qword [r14 + 136]
                            func.tokenizer.len.449.16.509.15.914.9.end:
                        cmp r11, qword [rbp + r8 + 88504]
                        sete r9b
                        jne bool.449.11.509.15.914.9.end
                        cmp.450.11.509.15.914.9:
                            mov rcx, qword [rbp + r8 + 88504]
                            cmp rcx, 16
                            ja baz_bounds_line_450
                            lea rsi, [rbp + r8 + 88488]
                            mov r11, qword [r14 + 136]
                            test r11, r11
                            js baz_bounds_line_450
                            lea rdx, [rcx + r11]
                            cmp rdx, 127
                            jg baz_bounds_line_450
                            lea rdi, [r14 + r11]
                            test rcx, rcx
                            repe cmpsb
                            sete r9b
                        bool.449.11.509.15.914.9.end:
                    func.tokenizer.is.name.509.15.914.9.end:
                cmp r9b, 0
                je if.509.9.914.9.end
                if.509.12.914.9.code:
                    mov qword [rbx + 32], r10
                    jmp foo.508.5.914.9.end
                if.509.9.914.9.end:
                foo.508.5.914.9.continue:
                    add r13, 16
                    inc r10
                    cmp r10, r12
                    jne foo.508.5.914.9
            foo.508.5.914.9.end:
            if.515.8.914.9:
            cmp.515.8.914.9:
            mov r13, qword [rbx + 24]
            cmp qword [rbx + 32], r13
            jne if.515.5.914.9.end
            if.515.8.914.9.code:
                func.printer.print_all.516.13.914.9:
                    func.printer.print.69.10.516.13.914.9:
                        func.printer.print_at.65.10.69.10.516.13.914.9:
                            if.59.8.65.10.69.10.516.13.914.9:
                            cmp.59.8.65.10.69.10.516.13.914.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.516.13.914.9.end
                            if.59.8.65.10.69.10.516.13.914.9.code:
                            if.59.5.65.10.69.10.516.13.914.9.end:
                            mov rdi, 1
                            mov rdx, 16
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 16
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260569]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.516.13.914.9.end:
                    func.printer.print.69.10.516.13.914.9.end:
                func.printer.print_all.516.13.914.9.end:
                jmp func.action_go.914.9.end
            if.515.5.914.9.end:
            mov r13, qword [rbx + 16]
            cmp r13, 128
            jae baz_bounds_line_521
            imul r13, 960
            lea r13, [rbp + r13 + 138224]
            mov r12, qword [rbx + 32]
            cmp r12, 8
            jae baz_bounds_line_521
            shl r12, 4
            mov r10, qword [r13 + r12 + 8]
            mov qword [rbx + 40], r10
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_522
            imul r13, 2736
            mov r12, qword [rbx + 40]
            mov qword [rbp + r13 + 952], r12
            mov r13, qword [rbx + 40]
            cmp r13, 128
            jae baz_bounds_line_524
            imul r13, 960
            func.id_list.push.524.54.914.9:
                func.id_list.reserve.157.16.524.54.914.9:
                    mov r12, qword [rbp + r13 + 138216]
                    mov qword [rbx + 48], r12
                    add qword [rbp + r13 + 138216], 1
                func.id_list.reserve.157.16.524.54.914.9.end:
                lea r12, [rbp + r13 + 137960]
                mov r10, qword [rbx + 48]
                cmp r10, 32
                jae baz_bounds_line_158
                mov r9, qword [r15]
                mov qword [r12 + r10 * 8], r9
            func.id_list.push.524.54.914.9.end:
            mov r13, qword [rbx + 16]
            cmp r13, 128
            jae baz_bounds_line_525
            imul r13, 960
            func.id_list.delete_item.525.39.914.9:
                func.id_list.index_of.185.25.525.39.914.9:
                    mov r12, qword [rbp + r13 + 138216]
                    mov qword [rbx + 56], r12
                    lea r12, [rbp + r13 + 137960]
                    mov r10, qword [rbp + r13 + 138216]
                    cmp r10, 32
                    ja baz_bounds_line_176
                    mov r9, 0
                    cmp r10, 0
                    jle foo.176.5.185.25.525.39.914.9.end
                    foo.176.5.185.25.525.39.914.9:
                        if.177.12.185.25.525.39.914.9:
                        cmp.177.12.185.25.525.39.914.9:
                        mov r8, qword [r15]
                        cmp qword [r12], r8
                        jne if.177.9.185.25.525.39.914.9.end
                        if.177.12.185.25.525.39.914.9.code:
                            mov qword [rbx + 56], r9
                            jmp foo.176.5.185.25.525.39.914.9.end
                        if.177.9.185.25.525.39.914.9.end:
                        foo.176.5.185.25.525.39.914.9.continue:
                            add r12, 8
                            inc r9
                            cmp r9, r10
                            jne foo.176.5.185.25.525.39.914.9
                    foo.176.5.185.25.525.39.914.9.end:
                func.id_list.index_of.185.25.525.39.914.9.end:
                if.186.8.525.39.914.9:
                cmp.186.8.525.39.914.9:
                mov r12, qword [rbp + r13 + 138216]
                cmp qword [rbx + 56], r12
                je func.id_list.delete_item.525.39.914.9.end
                if.186.8.525.39.914.9.code:
                if.186.5.525.39.914.9.end:
                lea r12, [rbp + r13 + 137960]
                func.id_list.delete_index.188.10.525.39.914.9:
                    mov rcx, qword [r12 + 256]
                    sub rcx, qword [rbx + 56]
                    sub rcx, 1
                    mov r10, qword [rbx + 56]
                    add r10, 1
                    test r10, r10
                    js baz_bounds_line_164
                    test rcx, rcx
                    js baz_bounds_line_164
                    lea r9, [rcx + r10]
                    cmp r9, 32
                    jg baz_bounds_line_164
                    lea rsi, [r12 + r10 * 8]
                    mov r10, qword [rbx + 56]
                    test r10, r10
                    js baz_bounds_line_165
                    lea r9, [rcx + r10]
                    cmp r9, 32
                    jg baz_bounds_line_165
                    lea rdi, [r12 + r10 * 8]
                    shl rcx, 3
                    rep movsb
                    sub qword [r12 + 256], 1
                func.id_list.delete_index.188.10.525.39.914.9.end:
            func.id_list.delete_item.525.39.914.9.end:
            func.printer.print_all.527.9.914.9:
                func.printer.print.69.10.527.9.914.9:
                    func.printer.print_at.65.10.69.10.527.9.914.9:
                        if.59.8.65.10.69.10.527.9.914.9:
                        cmp.59.8.65.10.69.10.527.9.914.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.527.9.914.9.end
                        if.59.8.65.10.69.10.527.9.914.9.code:
                        if.59.5.65.10.69.10.527.9.914.9.end:
                        mov rdi, 1
                        mov rdx, 8
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 8
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260552]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.527.9.914.9.end:
                func.printer.print.69.10.527.9.914.9.end:
            func.printer.print_all.527.9.914.9.end:
            func.tokenizer.print.528.8.914.9:
                func.tokenizer.len.440.39.528.8.914.9:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.440.39.528.8.914.9.end:
                func.printer.print_at.440.9.528.8.914.9:
                    if.59.8.440.9.528.8.914.9:
                    cmp.59.8.440.9.528.8.914.9:
                    cmp byte [rbp + 911], 0
                    jne func.printer.print_at.440.9.528.8.914.9.end
                    if.59.8.440.9.528.8.914.9.code:
                    if.59.5.440.9.528.8.914.9.end:
                    mov rdi, 1
                    mov rdx, r13
                    mov r12, qword [r14 + 136]
                    test r12, r12
                    js baz_bounds_line_61
                    test rdx, rdx
                    js baz_bounds_line_61
                    lea r10, [rdx + r12]
                    cmp r10, 127
                    jg baz_bounds_line_61
                    lea rsi, [r14]
                    add rsi, r12
                    mov rax, 1
                    syscall
                func.printer.print_at.440.9.528.8.914.9.end:
            func.tokenizer.print.528.8.914.9.end:
            func.printer.println.529.9.914.9:
                func.printer.print_all.73.10.529.9.914.9:
                    func.printer.print.69.10.73.10.529.9.914.9:
                        func.printer.print_at.65.10.69.10.73.10.529.9.914.9:
                            if.59.8.65.10.69.10.73.10.529.9.914.9:
                            cmp.59.8.65.10.69.10.73.10.529.9.914.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.73.10.529.9.914.9.end
                            if.59.8.65.10.69.10.73.10.529.9.914.9.code:
                            if.59.5.65.10.69.10.73.10.529.9.914.9.end:
                            mov rdi, 1
                            mov rdx, 1
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 1
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 879]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.73.10.529.9.914.9.end:
                    func.printer.print.69.10.73.10.529.9.914.9.end:
                func.printer.print_all.73.10.529.9.914.9.end:
            func.printer.println.529.9.914.9.end:
            xor al, al
            lea rdi, [rbx + 56]
            mov rcx, 136
            rep stosb
            func.str.add.533.15.914.9:
                mov r13, 8
                cmp r13, 8
                ja baz_bounds_line_93
                mov r12, qword [rbx + 184]
                test r12, r12
                js baz_bounds_line_93
                lea r10, [r13 + r12]
                cmp r10, 127
                jg baz_bounds_line_93
                mov rax, qword [rbp + 260552]
                mov qword [rbx + r12 + 56], rax
                add qword [rbx + 184], 8
            func.str.add.533.15.914.9.end:
            mov r12, qword [rbx + 16]
            cmp r12, 128
            jae baz_bounds_line_536
            imul r12, 960
            lea r12, [rbp + r12 + 138224]
            mov r10, qword [rbx + 32]
            cmp r10, 8
            jae baz_bounds_line_536
            shl r10, 4
            mov r13, qword [r12 + r10]
            cmp r13, 1024
            jae baz_bounds_line_536
            imul r13, 24
            func.str.append.name.534.15.914.9:
                mov rcx, qword [rbp + r13 + 88504]
                cmp rcx, 16
                ja baz_bounds_line_99
                lea rsi, [rbp + r13 + 88488]
                mov r12, qword [rbx + 184]
                test r12, r12
                js baz_bounds_line_99
                lea r10, [rcx + r12]
                cmp r10, 127
                jg baz_bounds_line_99
                lea rdi, [rbx + r12 + 56]
                rep movsb
                mov r12, qword [rbp + r13 + 88504]
                add qword [rbx + 184], r12
            func.str.append.name.534.15.914.9.end:
            func.notify_room.540.5.914.9:
                mov r13, qword [rbx + 16]
                cmp r13, 128
                jae baz_bounds_line_482
                imul r13, 960
                mov r12, qword [rbp + r13 + 138216]
                mov qword [rbx + 192], r12
                mov r12, qword [rbx + 16]
                cmp r12, 128
                jae baz_bounds_line_483
                imul r12, 960
                lea r13, [rbp + r12 + 137960]
                mov r12, qword [rbx + 192]
                cmp r12, 32
                ja baz_bounds_line_483
                mov r10, 0
                cmp r12, 0
                jle foo.483.5.540.5.914.9.end
                foo.483.5.540.5.914.9:
                    if.484.12.540.5.914.9:
                    cmp.484.12.540.5.914.9:
                    mov r9, qword [r15]
                    cmp qword [r13], r9
                    je foo.483.5.540.5.914.9.continue
                    if.484.12.540.5.914.9.code:
                    if.484.9.540.5.914.9.end:
                    if.486.12.540.5.914.9:
                    cmp.486.12.540.5.914.9:
                    mov r9, qword [r13]
                    cmp r9, 32
                    jae baz_bounds_line_486
                    imul r9, 2736
                    cmp qword [rbp + r9 + 3656], 16
                    je foo.483.5.540.5.914.9.continue
                    if.486.12.540.5.914.9.code:
                    if.486.9.540.5.914.9.end:
                    mov r9, qword [r13]
                    cmp r9, 32
                    jae baz_bounds_line_488
                    imul r9, 2736
                    func.messages.add.488.36.540.5.914.9:
                        func.messages.reserve.212.19.488.36.540.5.914.9:
                            mov r8, qword [rbp + r9 + 3656]
                            mov qword [rbx + 208], r8
                            add qword [rbp + r9 + 3656], 1
                        func.messages.reserve.212.19.488.36.540.5.914.9.end:
                        lea r8, [rbp + r9 + 1224]
                        mov r11, qword [rbx + 208]
                        cmp r11, 16
                        jae baz_bounds_line_213
                        imul r11, 152
                        mov rdx, qword [r15]
                        mov qword [r8 + r11], rdx
                        mov qword [r8 + r11 + 8], 2
                        lea rsi, [rbx + 56]
                        lea rdi, [r8 + r11 + 16]
                        mov rcx, 136
                        rep movsb
                    func.messages.add.488.36.540.5.914.9.end:
                    foo.483.5.540.5.914.9.continue:
                        add r13, 8
                        inc r10
                        cmp r10, r12
                        jne foo.483.5.540.5.914.9
                foo.483.5.540.5.914.9.end:
            func.notify_room.540.5.914.9.end:
            mov r13, qword [rbx + 40]
            cmp r13, 128
            jae baz_bounds_line_542
            imul r13, 960
            mov r12, qword [rbp + r13 + 138352]
            mov qword [rbx + 192], r12
            mov r13, qword [rbx + 192]
            mov qword [rbx + 200], r13
            mov r12, qword [rbx + 40]
            cmp r12, 128
            jae baz_bounds_line_544
            imul r12, 960
            lea r13, [rbp + r12 + 138224]
            mov r12, qword [rbx + 192]
            cmp r12, 8
            ja baz_bounds_line_544
            mov r10, 0
            cmp r12, 0
            jle foo.544.5.914.9.end
            foo.544.5.914.9:
                if.545.12.914.9:
                cmp.545.12.914.9:
                mov r9, qword [rbx + 16]
                cmp qword [r13 + 8], r9
                jne if.545.9.914.9.end
                if.545.12.914.9.code:
                    mov qword [rbx + 200], r10
                    jmp foo.544.5.914.9.end
                if.545.9.914.9.end:
                foo.544.5.914.9.continue:
                    add r13, 16
                    inc r10
                    cmp r10, r12
                    jne foo.544.5.914.9
            foo.544.5.914.9.end:
            cmp.552.15.914.9:
            mov r12, qword [rbx + 192]
            cmp qword [rbx + 200], r12
            setne r13b
            bool.552.15.914.9.end:
            func.assert.552.5.914.9:
                if.44.33.552.5.914.9:
                cmp.44.33.552.5.914.9:
                cmp r13b, 0
                jne if.44.30.552.5.914.9.end
                if.44.33.552.5.914.9.code:
                    mov rdi, 2
                    mov rax, 60
                    syscall
                if.44.30.552.5.914.9.end:
            func.assert.552.5.914.9.end:
            xor al, al
            lea rdi, [rbx + 208]
            mov rcx, 136
            rep stosb
            func.str.add.555.18.914.9:
                mov r13, 13
                cmp r13, 13
                ja baz_bounds_line_93
                mov r12, qword [rbx + 336]
                test r12, r12
                js baz_bounds_line_93
                lea r10, [r13 + r12]
                cmp r10, 127
                jg baz_bounds_line_93
                mov rax, qword [rbp + 260589]
                mov qword [rbx + r12 + 208], rax
                mov eax, dword [rbp + 260597]
                mov dword [rbx + r12 + 216], eax
                mov al, byte [rbp + 260601]
                mov byte [rbx + r12 + 220], al
                add qword [rbx + 336], 13
            func.str.add.555.18.914.9.end:
            mov r12, qword [rbx + 40]
            cmp r12, 128
            jae baz_bounds_line_558
            imul r12, 960
            lea r12, [rbp + r12 + 138224]
            mov r10, qword [rbx + 200]
            cmp r10, 8
            jae baz_bounds_line_558
            shl r10, 4
            mov r13, qword [r12 + r10]
            cmp r13, 1024
            jae baz_bounds_line_558
            imul r13, 24
            func.str.append.name.556.18.914.9:
                mov rcx, qword [rbp + r13 + 88504]
                cmp rcx, 16
                ja baz_bounds_line_99
                lea rsi, [rbp + r13 + 88488]
                mov r12, qword [rbx + 336]
                test r12, r12
                js baz_bounds_line_99
                lea r10, [rcx + r12]
                cmp r10, 127
                jg baz_bounds_line_99
                lea rdi, [rbx + r12 + 208]
                rep movsb
                mov r12, qword [rbp + r13 + 88504]
                add qword [rbx + 336], r12
            func.str.append.name.556.18.914.9.end:
            func.notify_room.562.5.914.9:
                mov r13, qword [rbx + 40]
                cmp r13, 128
                jae baz_bounds_line_482
                imul r13, 960
                mov r12, qword [rbp + r13 + 138216]
                mov qword [rbx + 344], r12
                mov r12, qword [rbx + 40]
                cmp r12, 128
                jae baz_bounds_line_483
                imul r12, 960
                lea r13, [rbp + r12 + 137960]
                mov r12, qword [rbx + 344]
                cmp r12, 32
                ja baz_bounds_line_483
                mov r10, 0
                cmp r12, 0
                jle foo.483.5.562.5.914.9.end
                foo.483.5.562.5.914.9:
                    if.484.12.562.5.914.9:
                    cmp.484.12.562.5.914.9:
                    mov r9, qword [r15]
                    cmp qword [r13], r9
                    je foo.483.5.562.5.914.9.continue
                    if.484.12.562.5.914.9.code:
                    if.484.9.562.5.914.9.end:
                    if.486.12.562.5.914.9:
                    cmp.486.12.562.5.914.9:
                    mov r9, qword [r13]
                    cmp r9, 32
                    jae baz_bounds_line_486
                    imul r9, 2736
                    cmp qword [rbp + r9 + 3656], 16
                    je foo.483.5.562.5.914.9.continue
                    if.486.12.562.5.914.9.code:
                    if.486.9.562.5.914.9.end:
                    mov r9, qword [r13]
                    cmp r9, 32
                    jae baz_bounds_line_488
                    imul r9, 2736
                    func.messages.add.488.36.562.5.914.9:
                        func.messages.reserve.212.19.488.36.562.5.914.9:
                            mov r8, qword [rbp + r9 + 3656]
                            mov qword [rbx + 360], r8
                            add qword [rbp + r9 + 3656], 1
                        func.messages.reserve.212.19.488.36.562.5.914.9.end:
                        lea r8, [rbp + r9 + 1224]
                        mov r11, qword [rbx + 360]
                        cmp r11, 16
                        jae baz_bounds_line_213
                        imul r11, 152
                        mov rdx, qword [r15]
                        mov qword [r8 + r11], rdx
                        mov qword [r8 + r11 + 8], 2
                        lea rsi, [rbx + 208]
                        lea rdi, [r8 + r11 + 16]
                        mov rcx, 136
                        rep movsb
                    func.messages.add.488.36.562.5.914.9.end:
                    foo.483.5.562.5.914.9.continue:
                        add r13, 8
                        inc r10
                        cmp r10, r12
                        jne foo.483.5.562.5.914.9
                foo.483.5.562.5.914.9.end:
            func.notify_room.562.5.914.9.end:
        func.action_go.914.9.end:
    jmp if.913.5.end
    if.916.15:
    cmp.916.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.916.18:
            cmp.444.11.916.18:
                func.tokenizer.len.444.16.916.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.916.18.end:
            cmp r13, 2
            sete r15b
            jne bool.444.11.916.18.end
            cmp.445.11.916.18:
                mov rcx, 2
                cmp rcx, 2
                ja baz_bounds_line_445
                lea rsi, [rbp + 261326]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.916.18.end:
        func.tokenizer.is_array.916.18.end:
    cmp r15b, 0
    je if.919.15
    if.916.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_new_room.917.9:
            if.585.8.917.9:
            cmp.585.8.917.9:
                func.tokenizer.next_or_say.585.15.917.9:
                    func.tokenizer.next.476.10.585.15.917.9:
                        func.tokenizer.skip_whitespace.426.10.476.10.585.15.917.9:
                            mov r12, qword [r14 + 144]
                            mov qword [r14 + 136], r12
                            loop.404.5.426.10.476.10.585.15.917.9:
                                if.405.12.426.10.476.10.585.15.917.9:
                                cmp.405.12.426.10.476.10.585.15.917.9:
                                mov r12, qword [r14 + 128]
                                cmp qword [r14 + 136], r12
                                jl if.405.9.426.10.476.10.585.15.917.9.end
                                if.405.12.426.10.476.10.585.15.917.9.code:
                                    mov r12, qword [r14 + 136]
                                    mov qword [r14 + 144], r12
                                    jmp func.tokenizer.skip_whitespace.426.10.476.10.585.15.917.9.end
                                if.405.9.426.10.476.10.585.15.917.9.end:
                                if.409.12.426.10.476.10.585.15.917.9:
                                cmp.409.12.426.10.476.10.585.15.917.9:
                                mov r12, qword [r14 + 136]
                                cmp r12, 127
                                jae baz_bounds_line_409
                                cmp byte [r14 + r12], 32
                                jne loop.404.5.426.10.476.10.585.15.917.9.end
                                if.409.12.426.10.476.10.585.15.917.9.code:
                                if.409.9.426.10.476.10.585.15.917.9.end:
                                add qword [r14 + 136], 1
                            jmp loop.404.5.426.10.476.10.585.15.917.9
                            loop.404.5.426.10.476.10.585.15.917.9.end:
                            mov r12, qword [r14 + 136]
                            mov qword [r14 + 144], r12
                        func.tokenizer.skip_whitespace.426.10.476.10.585.15.917.9.end:
                        mov r12, qword [r14 + 136]
                        mov qword [r14 + 144], r12
                        loop.428.5.476.10.585.15.917.9:
                            if.429.12.476.10.585.15.917.9:
                            cmp.429.12.476.10.585.15.917.9:
                            mov r12, qword [r14 + 128]
                            cmp qword [r14 + 144], r12
                            jge loop.428.5.476.10.585.15.917.9.end
                            if.429.12.476.10.585.15.917.9.code:
                            if.429.9.476.10.585.15.917.9.end:
                            if.430.12.476.10.585.15.917.9:
                            cmp.430.12.476.10.585.15.917.9:
                            mov r12, qword [r14 + 144]
                            cmp r12, 127
                            jae baz_bounds_line_430
                            cmp byte [r14 + r12], 32
                            je loop.428.5.476.10.585.15.917.9.end
                            if.430.12.476.10.585.15.917.9.code:
                            if.430.9.476.10.585.15.917.9.end:
                            add qword [r14 + 144], 1
                        jmp loop.428.5.476.10.585.15.917.9
                        loop.428.5.476.10.585.15.917.9.end:
                    func.tokenizer.next.476.10.585.15.917.9.end:
                    cmp.477.11.585.15.917.9:
                        func.tokenizer.is_empty.477.20.585.15.917.9:
                            cmp.454.11.477.20.585.15.917.9:
                            mov r12, qword [r14 + 144]
                            cmp qword [r14 + 136], r12
                            sete r13b
                            bool.454.11.477.20.585.15.917.9.end:
                        func.tokenizer.is_empty.477.20.585.15.917.9.end:
                    cmp r13b, 0
                    sete r13b
                    bool.477.11.585.15.917.9.end:
                    if.478.8.585.15.917.9:
                    cmp.478.8.585.15.917.9:
                    cmp r13b, 0
                    jne if.478.5.585.15.917.9.end
                    if.478.8.585.15.917.9.code:
                        func.printer.print_all.478.20.585.15.917.9:
                            func.printer.print.69.10.478.20.585.15.917.9:
                                func.printer.print_at.65.10.69.10.478.20.585.15.917.9:
                                    if.59.8.65.10.69.10.478.20.585.15.917.9:
                                    cmp.59.8.65.10.69.10.478.20.585.15.917.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.478.20.585.15.917.9.end
                                    if.59.8.65.10.69.10.478.20.585.15.917.9.code:
                                    if.59.5.65.10.69.10.478.20.585.15.917.9.end:
                                    mov rdi, 1
                                    mov rdx, 30
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 30
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260602]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.478.20.585.15.917.9.end:
                            func.printer.print.69.10.478.20.585.15.917.9.end:
                        func.printer.print_all.478.20.585.15.917.9.end:
                    if.478.5.585.15.917.9.end:
                func.tokenizer.next_or_say.585.15.917.9.end:
            cmp r13b, 0
            je func.action_new_room.917.9.end
            if.585.8.917.9.code:
            if.585.5.917.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_587
            imul r13, 2736
            mov r12, qword [rbp + r13 + 952]
            mov qword [rbx + 16], r12
            mov r13, qword [rbx + 16]
            cmp r13, 128
            jae baz_bounds_line_589
            imul r13, 960
            mov r12, qword [rbp + r13 + 138352]
            mov qword [rbx + 24], r12
            func.find_link_name_or_make.591.24.917.9:
                mov r13, qword [rbp + 113064]
                mov qword [rbx + 32], r13
                lea r13, [rbp + 88488]
                mov r12, qword [rbp + 113064]
                cmp r12, 1024
                ja baz_bounds_line_567
                mov r10, 0
                cmp r12, 0
                jle foo.567.5.591.24.917.9.end
                foo.567.5.591.24.917.9:
                    if.568.12.591.24.917.9:
                    cmp.568.12.591.24.917.9:
                        func.tokenizer.is.name.568.15.591.24.917.9:
                            cmp.449.11.568.15.591.24.917.9:
                                func.tokenizer.len.449.16.568.15.591.24.917.9:
                                    mov r8, qword [r14 + 144]
                                    sub r8, qword [r14 + 136]
                                func.tokenizer.len.449.16.568.15.591.24.917.9.end:
                            cmp r8, qword [r13 + 16]
                            sete r9b
                            jne bool.449.11.568.15.591.24.917.9.end
                            cmp.450.11.568.15.591.24.917.9:
                                mov rcx, qword [r13 + 16]
                                cmp rcx, 16
                                ja baz_bounds_line_450
                                lea rsi, [r13]
                                mov r8, qword [r14 + 136]
                                test r8, r8
                                js baz_bounds_line_450
                                lea r11, [rcx + r8]
                                cmp r11, 127
                                jg baz_bounds_line_450
                                lea rdi, [r14 + r8]
                                test rcx, rcx
                                repe cmpsb
                                sete r9b
                            bool.449.11.568.15.591.24.917.9.end:
                        func.tokenizer.is.name.568.15.591.24.917.9.end:
                    cmp r9b, 0
                    je if.568.9.591.24.917.9.end
                    if.568.12.591.24.917.9.code:
                        mov qword [rbx + 32], r10
                        jmp foo.567.5.591.24.917.9.end
                    if.568.9.591.24.917.9.end:
                    foo.567.5.591.24.917.9.continue:
                        add r13, 24
                        inc r10
                        cmp r10, r12
                        jne foo.567.5.591.24.917.9
                foo.567.5.591.24.917.9.end:
                if.574.8.591.24.917.9:
                cmp.574.8.591.24.917.9:
                mov r13, qword [rbp + 113064]
                cmp qword [rbx + 32], r13
                jne func.find_link_name_or_make.591.24.917.9.end
                if.574.8.591.24.917.9.code:
                if.574.5.591.24.917.9.end:
                func.link_names.reserve.576.22.591.24.917.9:
                    mov r13, qword [rbp + 113064]
                    mov qword [rbx + 32], r13
                    add qword [rbp + 113064], 1
                func.link_names.reserve.576.22.591.24.917.9.end:
                mov r13, qword [rbx + 32]
                cmp r13, 1024
                jae baz_bounds_line_577
                imul r13, 24
                func.tokenizer.token.name.577.32.591.24.917.9:
                    func.tokenizer.len.458.20.577.32.591.24.917.9:
                        mov r12, qword [r14 + 144]
                        sub r12, qword [r14 + 136]
                        mov qword [rbp + r13 + 88504], r12
                    func.tokenizer.len.458.20.577.32.591.24.917.9.end:
                    mov qword [rbp + r13 + 88488], 0
                    mov qword [rbp + r13 + 88496], 0
                    mov rcx, qword [rbp + r13 + 88504]
                    mov r12, qword [r14 + 136]
                    test r12, r12
                    js baz_bounds_line_460
                    test rcx, rcx
                    js baz_bounds_line_460
                    lea r10, [rcx + r12]
                    cmp r10, 127
                    jg baz_bounds_line_460
                    lea rsi, [r14 + r12]
                    cmp rcx, 16
                    ja baz_bounds_line_460
                    lea rdi, [rbp + r13 + 88488]
                    rep movsb
                func.tokenizer.token.name.577.32.591.24.917.9.end:
            func.find_link_name_or_make.591.24.917.9.end:
            if.593.8.917.9:
            cmp.593.8.917.9:
                func.tokenizer.next_or_say.593.15.917.9:
                    func.tokenizer.next.476.10.593.15.917.9:
                        func.tokenizer.skip_whitespace.426.10.476.10.593.15.917.9:
                            mov r12, qword [r14 + 144]
                            mov qword [r14 + 136], r12
                            loop.404.5.426.10.476.10.593.15.917.9:
                                if.405.12.426.10.476.10.593.15.917.9:
                                cmp.405.12.426.10.476.10.593.15.917.9:
                                mov r12, qword [r14 + 128]
                                cmp qword [r14 + 136], r12
                                jl if.405.9.426.10.476.10.593.15.917.9.end
                                if.405.12.426.10.476.10.593.15.917.9.code:
                                    mov r12, qword [r14 + 136]
                                    mov qword [r14 + 144], r12
                                    jmp func.tokenizer.skip_whitespace.426.10.476.10.593.15.917.9.end
                                if.405.9.426.10.476.10.593.15.917.9.end:
                                if.409.12.426.10.476.10.593.15.917.9:
                                cmp.409.12.426.10.476.10.593.15.917.9:
                                mov r12, qword [r14 + 136]
                                cmp r12, 127
                                jae baz_bounds_line_409
                                cmp byte [r14 + r12], 32
                                jne loop.404.5.426.10.476.10.593.15.917.9.end
                                if.409.12.426.10.476.10.593.15.917.9.code:
                                if.409.9.426.10.476.10.593.15.917.9.end:
                                add qword [r14 + 136], 1
                            jmp loop.404.5.426.10.476.10.593.15.917.9
                            loop.404.5.426.10.476.10.593.15.917.9.end:
                            mov r12, qword [r14 + 136]
                            mov qword [r14 + 144], r12
                        func.tokenizer.skip_whitespace.426.10.476.10.593.15.917.9.end:
                        mov r12, qword [r14 + 136]
                        mov qword [r14 + 144], r12
                        loop.428.5.476.10.593.15.917.9:
                            if.429.12.476.10.593.15.917.9:
                            cmp.429.12.476.10.593.15.917.9:
                            mov r12, qword [r14 + 128]
                            cmp qword [r14 + 144], r12
                            jge loop.428.5.476.10.593.15.917.9.end
                            if.429.12.476.10.593.15.917.9.code:
                            if.429.9.476.10.593.15.917.9.end:
                            if.430.12.476.10.593.15.917.9:
                            cmp.430.12.476.10.593.15.917.9:
                            mov r12, qword [r14 + 144]
                            cmp r12, 127
                            jae baz_bounds_line_430
                            cmp byte [r14 + r12], 32
                            je loop.428.5.476.10.593.15.917.9.end
                            if.430.12.476.10.593.15.917.9.code:
                            if.430.9.476.10.593.15.917.9.end:
                            add qword [r14 + 144], 1
                        jmp loop.428.5.476.10.593.15.917.9
                        loop.428.5.476.10.593.15.917.9.end:
                    func.tokenizer.next.476.10.593.15.917.9.end:
                    cmp.477.11.593.15.917.9:
                        func.tokenizer.is_empty.477.20.593.15.917.9:
                            cmp.454.11.477.20.593.15.917.9:
                            mov r12, qword [r14 + 144]
                            cmp qword [r14 + 136], r12
                            sete r13b
                            bool.454.11.477.20.593.15.917.9.end:
                        func.tokenizer.is_empty.477.20.593.15.917.9.end:
                    cmp r13b, 0
                    sete r13b
                    bool.477.11.593.15.917.9.end:
                    if.478.8.593.15.917.9:
                    cmp.478.8.593.15.917.9:
                    cmp r13b, 0
                    jne if.478.5.593.15.917.9.end
                    if.478.8.593.15.917.9.code:
                        func.printer.print_all.478.20.593.15.917.9:
                            func.printer.print.69.10.478.20.593.15.917.9:
                                func.printer.print_at.65.10.69.10.478.20.593.15.917.9:
                                    if.59.8.65.10.69.10.478.20.593.15.917.9:
                                    cmp.59.8.65.10.69.10.478.20.593.15.917.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.478.20.593.15.917.9.end
                                    if.59.8.65.10.69.10.478.20.593.15.917.9.code:
                                    if.59.5.65.10.69.10.478.20.593.15.917.9.end:
                                    mov rdi, 1
                                    mov rdx, 36
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 36
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260632]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.478.20.593.15.917.9.end:
                            func.printer.print.69.10.478.20.593.15.917.9.end:
                        func.printer.print_all.478.20.593.15.917.9.end:
                    if.478.5.593.15.917.9.end:
                func.tokenizer.next_or_say.593.15.917.9.end:
            cmp r13b, 0
            je func.action_new_room.917.9.end
            if.593.8.917.9.code:
            if.593.5.917.9.end:
            func.find_link_name_or_make.595.29.917.9:
                mov r13, qword [rbp + 113064]
                mov qword [rbx + 40], r13
                lea r13, [rbp + 88488]
                mov r12, qword [rbp + 113064]
                cmp r12, 1024
                ja baz_bounds_line_567
                mov r10, 0
                cmp r12, 0
                jle foo.567.5.595.29.917.9.end
                foo.567.5.595.29.917.9:
                    if.568.12.595.29.917.9:
                    cmp.568.12.595.29.917.9:
                        func.tokenizer.is.name.568.15.595.29.917.9:
                            cmp.449.11.568.15.595.29.917.9:
                                func.tokenizer.len.449.16.568.15.595.29.917.9:
                                    mov r8, qword [r14 + 144]
                                    sub r8, qword [r14 + 136]
                                func.tokenizer.len.449.16.568.15.595.29.917.9.end:
                            cmp r8, qword [r13 + 16]
                            sete r9b
                            jne bool.449.11.568.15.595.29.917.9.end
                            cmp.450.11.568.15.595.29.917.9:
                                mov rcx, qword [r13 + 16]
                                cmp rcx, 16
                                ja baz_bounds_line_450
                                lea rsi, [r13]
                                mov r8, qword [r14 + 136]
                                test r8, r8
                                js baz_bounds_line_450
                                lea r11, [rcx + r8]
                                cmp r11, 127
                                jg baz_bounds_line_450
                                lea rdi, [r14 + r8]
                                test rcx, rcx
                                repe cmpsb
                                sete r9b
                            bool.449.11.568.15.595.29.917.9.end:
                        func.tokenizer.is.name.568.15.595.29.917.9.end:
                    cmp r9b, 0
                    je if.568.9.595.29.917.9.end
                    if.568.12.595.29.917.9.code:
                        mov qword [rbx + 40], r10
                        jmp foo.567.5.595.29.917.9.end
                    if.568.9.595.29.917.9.end:
                    foo.567.5.595.29.917.9.continue:
                        add r13, 24
                        inc r10
                        cmp r10, r12
                        jne foo.567.5.595.29.917.9
                foo.567.5.595.29.917.9.end:
                if.574.8.595.29.917.9:
                cmp.574.8.595.29.917.9:
                mov r13, qword [rbp + 113064]
                cmp qword [rbx + 40], r13
                jne func.find_link_name_or_make.595.29.917.9.end
                if.574.8.595.29.917.9.code:
                if.574.5.595.29.917.9.end:
                func.link_names.reserve.576.22.595.29.917.9:
                    mov r13, qword [rbp + 113064]
                    mov qword [rbx + 40], r13
                    add qword [rbp + 113064], 1
                func.link_names.reserve.576.22.595.29.917.9.end:
                mov r13, qword [rbx + 40]
                cmp r13, 1024
                jae baz_bounds_line_577
                imul r13, 24
                func.tokenizer.token.name.577.32.595.29.917.9:
                    func.tokenizer.len.458.20.577.32.595.29.917.9:
                        mov r12, qword [r14 + 144]
                        sub r12, qword [r14 + 136]
                        mov qword [rbp + r13 + 88504], r12
                    func.tokenizer.len.458.20.577.32.595.29.917.9.end:
                    mov qword [rbp + r13 + 88488], 0
                    mov qword [rbp + r13 + 88496], 0
                    mov rcx, qword [rbp + r13 + 88504]
                    mov r12, qword [r14 + 136]
                    test r12, r12
                    js baz_bounds_line_460
                    test rcx, rcx
                    js baz_bounds_line_460
                    lea r10, [rcx + r12]
                    cmp r10, 127
                    jg baz_bounds_line_460
                    lea rsi, [r14 + r12]
                    cmp rcx, 16
                    ja baz_bounds_line_460
                    lea rdi, [rbp + r13 + 88488]
                    rep movsb
                func.tokenizer.token.name.577.32.595.29.917.9.end:
            func.find_link_name_or_make.595.29.917.9.end:
            func.rooms.reserve.597.29.917.9:
                mov r13, qword [rbp + 260544]
                mov qword [rbx + 48], r13
                add qword [rbp + 260544], 1
            func.rooms.reserve.597.29.917.9.end:
            mov r13, qword [rbx + 16]
            cmp r13, 128
            jae baz_bounds_line_600
            imul r13, 960
            lea r13, [rbp + r13 + 138224]
            mov r12, qword [rbx + 24]
            cmp r12, 8
            jae baz_bounds_line_600
            shl r12, 4
            mov r10, qword [rbx + 32]
            mov qword [r13 + r12], r10
            mov r10, qword [rbx + 48]
            mov qword [r13 + r12 + 8], r10
            mov r13, qword [rbx + 16]
            cmp r13, 128
            jae baz_bounds_line_604
            imul r13, 960
            mov r12, qword [rbx + 24]
            mov qword [rbp + r13 + 138352], r12
            add qword [rbp + r13 + 138352], 1
            mov r13, qword [rbx + 48]
            cmp r13, 128
            jae baz_bounds_line_607
            imul r13, 960
            mov r12, qword [rbx + 40]
            mov qword [rbp + r13 + 138224], r12
            mov r12, qword [rbx + 16]
            mov qword [rbp + r13 + 138232], r12
            mov r13, qword [rbx + 48]
            cmp r13, 128
            jae baz_bounds_line_611
            imul r13, 960
            mov qword [rbp + r13 + 138352], 1
            func.printer.print_all.613.9.917.9:
                func.printer.print.69.10.613.9.917.9:
                    func.printer.print_at.65.10.69.10.613.9.917.9:
                        if.59.8.65.10.69.10.613.9.917.9:
                        cmp.59.8.65.10.69.10.613.9.917.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.613.9.917.9.end
                        if.59.8.65.10.69.10.613.9.917.9.code:
                        if.59.5.65.10.69.10.613.9.917.9.end:
                        mov rdi, 1
                        mov rdx, 17
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 17
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260668]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.613.9.917.9.end:
                func.printer.print.69.10.613.9.917.9.end:
            func.printer.print_all.613.9.917.9.end:
        func.action_new_room.917.9.end:
    jmp if.913.5.end
    if.919.15:
    cmp.919.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.919.18:
            cmp.444.11.919.18:
                func.tokenizer.len.444.16.919.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.919.18.end:
            cmp r13, 2
            sete r15b
            jne bool.444.11.919.18.end
            cmp.445.11.919.18:
                mov rcx, 2
                cmp rcx, 2
                ja baz_bounds_line_445
                lea rsi, [rbp + 261331]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.919.18.end:
        func.tokenizer.is_array.919.18.end:
    cmp r15b, 0
    je if.922.15
    if.919.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_set_room_description.920.9:
            func.tokenizer.skip_whitespace.619.8.920.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                loop.404.5.619.8.920.9:
                    if.405.12.619.8.920.9:
                    cmp.405.12.619.8.920.9:
                    mov r13, qword [r14 + 128]
                    cmp qword [r14 + 136], r13
                    jl if.405.9.619.8.920.9.end
                    if.405.12.619.8.920.9.code:
                        mov r13, qword [r14 + 136]
                        mov qword [r14 + 144], r13
                        jmp func.tokenizer.skip_whitespace.619.8.920.9.end
                    if.405.9.619.8.920.9.end:
                    if.409.12.619.8.920.9:
                    cmp.409.12.619.8.920.9:
                    mov r13, qword [r14 + 136]
                    cmp r13, 127
                    jae baz_bounds_line_409
                    cmp byte [r14 + r13], 32
                    jne loop.404.5.619.8.920.9.end
                    if.409.12.619.8.920.9.code:
                    if.409.9.619.8.920.9.end:
                    add qword [r14 + 136], 1
                jmp loop.404.5.619.8.920.9
                loop.404.5.619.8.920.9.end:
                mov r13, qword [r14 + 136]
                mov qword [r14 + 144], r13
            func.tokenizer.skip_whitespace.619.8.920.9.end:
            func.tokenizer.to_end.620.8.920.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                mov r13, qword [r14 + 128]
                mov qword [r14 + 144], r13
            func.tokenizer.to_end.620.8.920.9.end:
            mov r12, qword [r15]
            cmp r12, 32
            jae baz_bounds_line_621
            imul r12, 2736
            mov r13, qword [rbp + r12 + 952]
            cmp r13, 128
            jae baz_bounds_line_621
            imul r13, 960
            mov byte [rbp + r13 + 137815], 0
            func.tokenizer.token.str.621.63.920.9:
                func.tokenizer.len.458.20.621.63.920.9:
                    mov r12, qword [r14 + 144]
                    sub r12, qword [r14 + 136]
                    mov qword [rbp + r13 + 137816], r12
                func.tokenizer.len.458.20.621.63.920.9.end:
                xor al, al
                lea rdi, [rbp + r13 + 137688]
                mov rcx, 127
                rep stosb
                mov rcx, qword [rbp + r13 + 137816]
                mov r12, qword [r14 + 136]
                test r12, r12
                js baz_bounds_line_460
                test rcx, rcx
                js baz_bounds_line_460
                lea r10, [rcx + r12]
                cmp r10, 127
                jg baz_bounds_line_460
                lea rsi, [r14 + r12]
                cmp rcx, 127
                ja baz_bounds_line_460
                lea rdi, [rbp + r13 + 137688]
                rep movsb
            func.tokenizer.token.str.621.63.920.9.end:
            func.printer.print_all.623.9.920.9:
                func.printer.print.69.10.623.9.920.9:
                    func.printer.print_at.65.10.69.10.623.9.920.9:
                        if.59.8.65.10.69.10.623.9.920.9:
                        cmp.59.8.65.10.69.10.623.9.920.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.623.9.920.9.end
                        if.59.8.65.10.69.10.623.9.920.9.code:
                        if.59.5.65.10.69.10.623.9.920.9.end:
                        mov rdi, 1
                        mov rdx, 21
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 21
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260685]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.623.9.920.9.end:
                func.printer.print.69.10.623.9.920.9.end:
            func.printer.print_all.623.9.920.9.end:
        func.action_set_room_description.920.9.end:
    jmp if.913.5.end
    if.922.15:
    cmp.922.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.922.18:
            cmp.444.11.922.18:
                func.tokenizer.len.444.16.922.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.922.18.end:
            cmp r13, 3
            sete r15b
            jne bool.444.11.922.18.end
            cmp.445.11.922.18:
                mov rcx, 3
                cmp rcx, 3
                ja baz_bounds_line_445
                lea rsi, [rbp + 261333]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.922.18.end:
        func.tokenizer.is_array.922.18.end:
    cmp r15b, 0
    je if.925.15
    if.922.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_set_room_note.923.9:
            func.tokenizer.skip_whitespace.627.8.923.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                loop.404.5.627.8.923.9:
                    if.405.12.627.8.923.9:
                    cmp.405.12.627.8.923.9:
                    mov r13, qword [r14 + 128]
                    cmp qword [r14 + 136], r13
                    jl if.405.9.627.8.923.9.end
                    if.405.12.627.8.923.9.code:
                        mov r13, qword [r14 + 136]
                        mov qword [r14 + 144], r13
                        jmp func.tokenizer.skip_whitespace.627.8.923.9.end
                    if.405.9.627.8.923.9.end:
                    if.409.12.627.8.923.9:
                    cmp.409.12.627.8.923.9:
                    mov r13, qword [r14 + 136]
                    cmp r13, 127
                    jae baz_bounds_line_409
                    cmp byte [r14 + r13], 32
                    jne loop.404.5.627.8.923.9.end
                    if.409.12.627.8.923.9.code:
                    if.409.9.627.8.923.9.end:
                    add qword [r14 + 136], 1
                jmp loop.404.5.627.8.923.9
                loop.404.5.627.8.923.9.end:
                mov r13, qword [r14 + 136]
                mov qword [r14 + 144], r13
            func.tokenizer.skip_whitespace.627.8.923.9.end:
            func.tokenizer.to_end.628.8.923.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                mov r13, qword [r14 + 128]
                mov qword [r14 + 144], r13
            func.tokenizer.to_end.628.8.923.9.end:
            mov r12, qword [r15]
            cmp r12, 32
            jae baz_bounds_line_629
            imul r12, 2736
            mov r13, qword [rbp + r12 + 952]
            cmp r13, 128
            jae baz_bounds_line_629
            imul r13, 960
            mov byte [rbp + r13 + 137951], 0
            func.tokenizer.token.str.629.56.923.9:
                func.tokenizer.len.458.20.629.56.923.9:
                    mov r12, qword [r14 + 144]
                    sub r12, qword [r14 + 136]
                    mov qword [rbp + r13 + 137952], r12
                func.tokenizer.len.458.20.629.56.923.9.end:
                xor al, al
                lea rdi, [rbp + r13 + 137824]
                mov rcx, 127
                rep stosb
                mov rcx, qword [rbp + r13 + 137952]
                mov r12, qword [r14 + 136]
                test r12, r12
                js baz_bounds_line_460
                test rcx, rcx
                js baz_bounds_line_460
                lea r10, [rcx + r12]
                cmp r10, 127
                jg baz_bounds_line_460
                lea rsi, [r14 + r12]
                cmp rcx, 127
                ja baz_bounds_line_460
                lea rdi, [rbp + r13 + 137824]
                rep movsb
            func.tokenizer.token.str.629.56.923.9.end:
        func.action_set_room_note.923.9.end:
    jmp if.913.5.end
    if.925.15:
    cmp.925.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.925.18:
            cmp.444.11.925.18:
                func.tokenizer.len.444.16.925.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.925.18.end:
            cmp r13, 3
            sete r15b
            jne bool.444.11.925.18.end
            cmp.445.11.925.18:
                mov rcx, 3
                cmp rcx, 3
                ja baz_bounds_line_445
                lea rsi, [rbp + 261328]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.925.18.end:
        func.tokenizer.is_array.925.18.end:
    cmp r15b, 0
    je if.928.15
    if.925.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_set_room_name.926.9:
            if.650.8.926.9:
            cmp.650.8.926.9:
                func.tokenizer.next_name_or_say.650.15.926.9:
                    mov r13b, 0
                    if.639.8.650.15.926.9:
                    cmp.639.8.650.15.926.9:
                        func.tokenizer.next_or_say.639.17.650.15.926.9:
                            func.tokenizer.next.476.10.639.17.650.15.926.9:
                                func.tokenizer.skip_whitespace.426.10.476.10.639.17.650.15.926.9:
                                    mov r10, qword [r14 + 144]
                                    mov qword [r14 + 136], r10
                                    loop.404.5.426.10.476.10.639.17.650.15.926.9:
                                        if.405.12.426.10.476.10.639.17.650.15.926.9:
                                        cmp.405.12.426.10.476.10.639.17.650.15.926.9:
                                        mov r10, qword [r14 + 128]
                                        cmp qword [r14 + 136], r10
                                        jl if.405.9.426.10.476.10.639.17.650.15.926.9.end
                                        if.405.12.426.10.476.10.639.17.650.15.926.9.code:
                                            mov r10, qword [r14 + 136]
                                            mov qword [r14 + 144], r10
                                            jmp func.tokenizer.skip_whitespace.426.10.476.10.639.17.650.15.926.9.end
                                        if.405.9.426.10.476.10.639.17.650.15.926.9.end:
                                        if.409.12.426.10.476.10.639.17.650.15.926.9:
                                        cmp.409.12.426.10.476.10.639.17.650.15.926.9:
                                        mov r10, qword [r14 + 136]
                                        cmp r10, 127
                                        jae baz_bounds_line_409
                                        cmp byte [r14 + r10], 32
                                        jne loop.404.5.426.10.476.10.639.17.650.15.926.9.end
                                        if.409.12.426.10.476.10.639.17.650.15.926.9.code:
                                        if.409.9.426.10.476.10.639.17.650.15.926.9.end:
                                        add qword [r14 + 136], 1
                                    jmp loop.404.5.426.10.476.10.639.17.650.15.926.9
                                    loop.404.5.426.10.476.10.639.17.650.15.926.9.end:
                                    mov r10, qword [r14 + 136]
                                    mov qword [r14 + 144], r10
                                func.tokenizer.skip_whitespace.426.10.476.10.639.17.650.15.926.9.end:
                                mov r10, qword [r14 + 136]
                                mov qword [r14 + 144], r10
                                loop.428.5.476.10.639.17.650.15.926.9:
                                    if.429.12.476.10.639.17.650.15.926.9:
                                    cmp.429.12.476.10.639.17.650.15.926.9:
                                    mov r10, qword [r14 + 128]
                                    cmp qword [r14 + 144], r10
                                    jge loop.428.5.476.10.639.17.650.15.926.9.end
                                    if.429.12.476.10.639.17.650.15.926.9.code:
                                    if.429.9.476.10.639.17.650.15.926.9.end:
                                    if.430.12.476.10.639.17.650.15.926.9:
                                    cmp.430.12.476.10.639.17.650.15.926.9:
                                    mov r10, qword [r14 + 144]
                                    cmp r10, 127
                                    jae baz_bounds_line_430
                                    cmp byte [r14 + r10], 32
                                    je loop.428.5.476.10.639.17.650.15.926.9.end
                                    if.430.12.476.10.639.17.650.15.926.9.code:
                                    if.430.9.476.10.639.17.650.15.926.9.end:
                                    add qword [r14 + 144], 1
                                jmp loop.428.5.476.10.639.17.650.15.926.9
                                loop.428.5.476.10.639.17.650.15.926.9.end:
                            func.tokenizer.next.476.10.639.17.650.15.926.9.end:
                            cmp.477.11.639.17.650.15.926.9:
                                func.tokenizer.is_empty.477.20.639.17.650.15.926.9:
                                    cmp.454.11.477.20.639.17.650.15.926.9:
                                    mov r10, qword [r14 + 144]
                                    cmp qword [r14 + 136], r10
                                    sete r12b
                                    bool.454.11.477.20.639.17.650.15.926.9.end:
                                func.tokenizer.is_empty.477.20.639.17.650.15.926.9.end:
                            cmp r12b, 0
                            sete r12b
                            bool.477.11.639.17.650.15.926.9.end:
                            if.478.8.639.17.650.15.926.9:
                            cmp.478.8.639.17.650.15.926.9:
                            cmp r12b, 0
                            jne if.478.5.639.17.650.15.926.9.end
                            if.478.8.639.17.650.15.926.9.code:
                                func.printer.print_all.478.20.639.17.650.15.926.9:
                                    func.printer.print.69.10.478.20.639.17.650.15.926.9:
                                        func.printer.print_at.65.10.69.10.478.20.639.17.650.15.926.9:
                                            if.59.8.65.10.69.10.478.20.639.17.650.15.926.9:
                                            cmp.59.8.65.10.69.10.478.20.639.17.650.15.926.9:
                                            cmp byte [rbp + 911], 0
                                            jne func.printer.print_at.65.10.69.10.478.20.639.17.650.15.926.9.end
                                            if.59.8.65.10.69.10.478.20.639.17.650.15.926.9.code:
                                            if.59.5.65.10.69.10.478.20.639.17.650.15.926.9.end:
                                            mov rdi, 1
                                            mov rdx, 10
                                            mov r10, 0
                                            test r10, r10
                                            js baz_bounds_line_61
                                            test rdx, rdx
                                            js baz_bounds_line_61
                                            lea r9, [rdx + r10]
                                            cmp r9, 10
                                            jg baz_bounds_line_61
                                            lea rsi, [rbp + 260706]
                                            add rsi, r10
                                            mov rax, 1
                                            syscall
                                        func.printer.print_at.65.10.69.10.478.20.639.17.650.15.926.9.end:
                                    func.printer.print.69.10.478.20.639.17.650.15.926.9.end:
                                func.printer.print_all.478.20.639.17.650.15.926.9.end:
                            if.478.5.639.17.650.15.926.9.end:
                        func.tokenizer.next_or_say.639.17.650.15.926.9.end:
                    cmp r12b, 0
                    je func.tokenizer.next_name_or_say.650.15.926.9.end
                    if.639.8.650.15.926.9.code:
                    if.639.5.650.15.926.9.end:
                    if.641.8.650.15.926.9:
                    cmp.641.8.650.15.926.9:
                        func.tokenizer.len.641.13.650.15.926.9:
                            mov r12, qword [r14 + 144]
                            sub r12, qword [r14 + 136]
                        func.tokenizer.len.641.13.650.15.926.9.end:
                    cmp r12, 16
                    jle if.641.5.650.15.926.9.end
                    if.641.8.650.15.926.9.code:
                        func.printer.print_all.642.13.650.15.926.9:
                            func.printer.print.69.10.642.13.650.15.926.9:
                                func.printer.print_at.65.10.69.10.642.13.650.15.926.9:
                                    if.59.8.65.10.69.10.642.13.650.15.926.9:
                                    cmp.59.8.65.10.69.10.642.13.650.15.926.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.642.13.650.15.926.9.end
                                    if.59.8.65.10.69.10.642.13.650.15.926.9.code:
                                    if.59.5.65.10.69.10.642.13.650.15.926.9.end:
                                    mov rdi, 1
                                    mov rdx, 14
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 14
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260716]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.642.13.650.15.926.9.end:
                            func.printer.print.69.10.642.13.650.15.926.9.end:
                        func.printer.print_all.642.13.650.15.926.9.end:
                        jmp func.tokenizer.next_name_or_say.650.15.926.9.end
                    if.641.5.650.15.926.9.end:
                    mov r13b, 1
                func.tokenizer.next_name_or_say.650.15.926.9.end:
            cmp r13b, 0
            je func.action_set_room_name.926.9.end
            if.650.8.926.9.code:
            if.650.5.926.9.end:
            mov r12, qword [r15]
            cmp r12, 32
            jae baz_bounds_line_652
            imul r12, 2736
            mov r13, qword [rbp + r12 + 952]
            cmp r13, 128
            jae baz_bounds_line_652
            imul r13, 960
            func.tokenizer.token.name.652.56.926.9:
                func.tokenizer.len.458.20.652.56.926.9:
                    mov r12, qword [r14 + 144]
                    sub r12, qword [r14 + 136]
                    mov qword [rbp + r13 + 137680], r12
                func.tokenizer.len.458.20.652.56.926.9.end:
                mov qword [rbp + r13 + 137664], 0
                mov qword [rbp + r13 + 137672], 0
                mov rcx, qword [rbp + r13 + 137680]
                mov r12, qword [r14 + 136]
                test r12, r12
                js baz_bounds_line_460
                test rcx, rcx
                js baz_bounds_line_460
                lea r10, [rcx + r12]
                cmp r10, 127
                jg baz_bounds_line_460
                lea rsi, [r14 + r12]
                cmp rcx, 16
                ja baz_bounds_line_460
                lea rdi, [rbp + r13 + 137664]
                rep movsb
            func.tokenizer.token.name.652.56.926.9.end:
            func.printer.print_all.654.9.926.9:
                func.printer.print.69.10.654.9.926.9:
                    func.printer.print_at.65.10.69.10.654.9.926.9:
                        if.59.8.65.10.69.10.654.9.926.9:
                        cmp.59.8.65.10.69.10.654.9.926.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.654.9.926.9.end
                        if.59.8.65.10.69.10.654.9.926.9.code:
                        if.59.5.65.10.69.10.654.9.926.9.end:
                        mov rdi, 1
                        mov rdx, 14
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 14
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260730]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.654.9.926.9.end:
                func.printer.print.69.10.654.9.926.9.end:
            func.printer.print_all.654.9.926.9.end:
        func.action_set_room_name.926.9.end:
    jmp if.913.5.end
    if.928.15:
    cmp.928.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.928.18:
            cmp.444.11.928.18:
                func.tokenizer.len.444.16.928.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.928.18.end:
            cmp r13, 2
            sete r15b
            jne bool.444.11.928.18.end
            cmp.445.11.928.18:
                mov rcx, 2
                cmp rcx, 2
                ja baz_bounds_line_445
                lea rsi, [rbp + 261336]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.928.18.end:
        func.tokenizer.is_array.928.18.end:
    cmp r15b, 0
    je if.931.15
    if.928.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_new_entity.929.9:
            if.660.8.929.9:
            cmp.660.8.929.9:
                func.tokenizer.next_name_or_say.660.15.929.9:
                    mov r13b, 0
                    if.639.8.660.15.929.9:
                    cmp.639.8.660.15.929.9:
                        func.tokenizer.next_or_say.639.17.660.15.929.9:
                            func.tokenizer.next.476.10.639.17.660.15.929.9:
                                func.tokenizer.skip_whitespace.426.10.476.10.639.17.660.15.929.9:
                                    mov r10, qword [r14 + 144]
                                    mov qword [r14 + 136], r10
                                    loop.404.5.426.10.476.10.639.17.660.15.929.9:
                                        if.405.12.426.10.476.10.639.17.660.15.929.9:
                                        cmp.405.12.426.10.476.10.639.17.660.15.929.9:
                                        mov r10, qword [r14 + 128]
                                        cmp qword [r14 + 136], r10
                                        jl if.405.9.426.10.476.10.639.17.660.15.929.9.end
                                        if.405.12.426.10.476.10.639.17.660.15.929.9.code:
                                            mov r10, qword [r14 + 136]
                                            mov qword [r14 + 144], r10
                                            jmp func.tokenizer.skip_whitespace.426.10.476.10.639.17.660.15.929.9.end
                                        if.405.9.426.10.476.10.639.17.660.15.929.9.end:
                                        if.409.12.426.10.476.10.639.17.660.15.929.9:
                                        cmp.409.12.426.10.476.10.639.17.660.15.929.9:
                                        mov r10, qword [r14 + 136]
                                        cmp r10, 127
                                        jae baz_bounds_line_409
                                        cmp byte [r14 + r10], 32
                                        jne loop.404.5.426.10.476.10.639.17.660.15.929.9.end
                                        if.409.12.426.10.476.10.639.17.660.15.929.9.code:
                                        if.409.9.426.10.476.10.639.17.660.15.929.9.end:
                                        add qword [r14 + 136], 1
                                    jmp loop.404.5.426.10.476.10.639.17.660.15.929.9
                                    loop.404.5.426.10.476.10.639.17.660.15.929.9.end:
                                    mov r10, qword [r14 + 136]
                                    mov qword [r14 + 144], r10
                                func.tokenizer.skip_whitespace.426.10.476.10.639.17.660.15.929.9.end:
                                mov r10, qword [r14 + 136]
                                mov qword [r14 + 144], r10
                                loop.428.5.476.10.639.17.660.15.929.9:
                                    if.429.12.476.10.639.17.660.15.929.9:
                                    cmp.429.12.476.10.639.17.660.15.929.9:
                                    mov r10, qword [r14 + 128]
                                    cmp qword [r14 + 144], r10
                                    jge loop.428.5.476.10.639.17.660.15.929.9.end
                                    if.429.12.476.10.639.17.660.15.929.9.code:
                                    if.429.9.476.10.639.17.660.15.929.9.end:
                                    if.430.12.476.10.639.17.660.15.929.9:
                                    cmp.430.12.476.10.639.17.660.15.929.9:
                                    mov r10, qword [r14 + 144]
                                    cmp r10, 127
                                    jae baz_bounds_line_430
                                    cmp byte [r14 + r10], 32
                                    je loop.428.5.476.10.639.17.660.15.929.9.end
                                    if.430.12.476.10.639.17.660.15.929.9.code:
                                    if.430.9.476.10.639.17.660.15.929.9.end:
                                    add qword [r14 + 144], 1
                                jmp loop.428.5.476.10.639.17.660.15.929.9
                                loop.428.5.476.10.639.17.660.15.929.9.end:
                            func.tokenizer.next.476.10.639.17.660.15.929.9.end:
                            cmp.477.11.639.17.660.15.929.9:
                                func.tokenizer.is_empty.477.20.639.17.660.15.929.9:
                                    cmp.454.11.477.20.639.17.660.15.929.9:
                                    mov r10, qword [r14 + 144]
                                    cmp qword [r14 + 136], r10
                                    sete r12b
                                    bool.454.11.477.20.639.17.660.15.929.9.end:
                                func.tokenizer.is_empty.477.20.639.17.660.15.929.9.end:
                            cmp r12b, 0
                            sete r12b
                            bool.477.11.639.17.660.15.929.9.end:
                            if.478.8.639.17.660.15.929.9:
                            cmp.478.8.639.17.660.15.929.9:
                            cmp r12b, 0
                            jne if.478.5.639.17.660.15.929.9.end
                            if.478.8.639.17.660.15.929.9.code:
                                func.printer.print_all.478.20.639.17.660.15.929.9:
                                    func.printer.print.69.10.478.20.639.17.660.15.929.9:
                                        func.printer.print_at.65.10.69.10.478.20.639.17.660.15.929.9:
                                            if.59.8.65.10.69.10.478.20.639.17.660.15.929.9:
                                            cmp.59.8.65.10.69.10.478.20.639.17.660.15.929.9:
                                            cmp byte [rbp + 911], 0
                                            jne func.printer.print_at.65.10.69.10.478.20.639.17.660.15.929.9.end
                                            if.59.8.65.10.69.10.478.20.639.17.660.15.929.9.code:
                                            if.59.5.65.10.69.10.478.20.639.17.660.15.929.9.end:
                                            mov rdi, 1
                                            mov rdx, 10
                                            mov r10, 0
                                            test r10, r10
                                            js baz_bounds_line_61
                                            test rdx, rdx
                                            js baz_bounds_line_61
                                            lea r9, [rdx + r10]
                                            cmp r9, 10
                                            jg baz_bounds_line_61
                                            lea rsi, [rbp + 260706]
                                            add rsi, r10
                                            mov rax, 1
                                            syscall
                                        func.printer.print_at.65.10.69.10.478.20.639.17.660.15.929.9.end:
                                    func.printer.print.69.10.478.20.639.17.660.15.929.9.end:
                                func.printer.print_all.478.20.639.17.660.15.929.9.end:
                            if.478.5.639.17.660.15.929.9.end:
                        func.tokenizer.next_or_say.639.17.660.15.929.9.end:
                    cmp r12b, 0
                    je func.tokenizer.next_name_or_say.660.15.929.9.end
                    if.639.8.660.15.929.9.code:
                    if.639.5.660.15.929.9.end:
                    if.641.8.660.15.929.9:
                    cmp.641.8.660.15.929.9:
                        func.tokenizer.len.641.13.660.15.929.9:
                            mov r12, qword [r14 + 144]
                            sub r12, qword [r14 + 136]
                        func.tokenizer.len.641.13.660.15.929.9.end:
                    cmp r12, 16
                    jle if.641.5.660.15.929.9.end
                    if.641.8.660.15.929.9.code:
                        func.printer.print_all.642.13.660.15.929.9:
                            func.printer.print.69.10.642.13.660.15.929.9:
                                func.printer.print_at.65.10.69.10.642.13.660.15.929.9:
                                    if.59.8.65.10.69.10.642.13.660.15.929.9:
                                    cmp.59.8.65.10.69.10.642.13.660.15.929.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.642.13.660.15.929.9.end
                                    if.59.8.65.10.69.10.642.13.660.15.929.9.code:
                                    if.59.5.65.10.69.10.642.13.660.15.929.9.end:
                                    mov rdi, 1
                                    mov rdx, 14
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 14
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260716]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.642.13.660.15.929.9.end:
                            func.printer.print.69.10.642.13.660.15.929.9.end:
                        func.printer.print_all.642.13.660.15.929.9.end:
                        jmp func.tokenizer.next_name_or_say.660.15.929.9.end
                    if.641.5.660.15.929.9.end:
                    mov r13b, 1
                func.tokenizer.next_name_or_say.660.15.929.9.end:
            cmp r13b, 0
            je func.action_new_entity.929.9.end
            if.660.8.929.9.code:
            if.660.5.929.9.end:
            func.tokenizer.token.name.662.19.929.9:
                func.tokenizer.len.458.20.662.19.929.9:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                    mov qword [rbx + 32], r13
                func.tokenizer.len.458.20.662.19.929.9.end:
                mov qword [rbx + 16], 0
                mov qword [rbx + 24], 0
                mov rcx, qword [rbx + 32]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_460
                test rcx, rcx
                js baz_bounds_line_460
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_460
                lea rsi, [r14 + r13]
                cmp rcx, 16
                ja baz_bounds_line_460
                lea rdi, [rbx + 16]
                rep movsb
            func.tokenizer.token.name.662.19.929.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_664
            imul r13, 2736
            mov r12, qword [rbp + r13 + 952]
            mov qword [rbx + 40], r12
            func.entities.add.666.30.929.9:
                func.entities.reserve.261.22.666.30.929.9:
                    mov r13, qword [rbp + 88480]
                    mov qword [rbx + 48], r13
                    add qword [rbp + 88480], 1
                func.entities.reserve.261.22.666.30.929.9.end:
                mov r13, qword [rbx + 48]
                cmp r13, 32
                jae baz_bounds_line_262
                imul r13, 2736
                lea rsi, [rbx + 16]
                lea rdi, [rbp + r13 + 928]
                mov rcx, 24
                rep movsb
                mov r12, qword [rbx + 40]
                mov qword [rbp + r13 + 952], r12
                xor al, al
                lea rdi, [rbp + r13 + 960]
                mov rcx, 2704
                rep stosb
            func.entities.add.666.30.929.9.end:
            mov r13, qword [rbx + 40]
            cmp r13, 128
            jae baz_bounds_line_668
            imul r13, 960
            func.id_list.push.668.51.929.9:
                func.id_list.reserve.157.16.668.51.929.9:
                    mov r12, qword [rbp + r13 + 138216]
                    mov qword [rbx + 56], r12
                    add qword [rbp + r13 + 138216], 1
                func.id_list.reserve.157.16.668.51.929.9.end:
                lea r12, [rbp + r13 + 137960]
                mov r10, qword [rbx + 56]
                cmp r10, 32
                jae baz_bounds_line_158
                mov r9, qword [rbx + 48]
                mov qword [r12 + r10 * 8], r9
            func.id_list.push.668.51.929.9.end:
            func.printer.print_all.670.9.929.9:
                func.printer.print.69.10.670.9.929.9:
                    func.printer.print_at.65.10.69.10.670.9.929.9:
                        if.59.8.65.10.69.10.670.9.929.9:
                        cmp.59.8.65.10.69.10.670.9.929.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.670.9.929.9.end
                        if.59.8.65.10.69.10.670.9.929.9.code:
                        if.59.5.65.10.69.10.670.9.929.9.end:
                        mov rdi, 1
                        mov rdx, 19
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 19
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260744]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.670.9.929.9.end:
                func.printer.print.69.10.670.9.929.9.end:
            func.printer.print_all.670.9.929.9.end:
        func.action_new_entity.929.9.end:
    jmp if.913.5.end
    if.931.15:
    cmp.931.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.931.18:
            cmp.444.11.931.18:
                func.tokenizer.len.444.16.931.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.931.18.end:
            cmp r13, 2
            sete r15b
            jne bool.444.11.931.18.end
            cmp.445.11.931.18:
                mov rcx, 2
                cmp rcx, 2
                ja baz_bounds_line_445
                lea rsi, [rbp + 261320]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.931.18.end:
        func.tokenizer.is_array.931.18.end:
    cmp r15b, 0
    je if.934.15
    if.931.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_new_object.932.9:
            if.676.8.932.9:
            cmp.676.8.932.9:
                func.tokenizer.next_name_or_say.676.15.932.9:
                    mov r13b, 0
                    if.639.8.676.15.932.9:
                    cmp.639.8.676.15.932.9:
                        func.tokenizer.next_or_say.639.17.676.15.932.9:
                            func.tokenizer.next.476.10.639.17.676.15.932.9:
                                func.tokenizer.skip_whitespace.426.10.476.10.639.17.676.15.932.9:
                                    mov r10, qword [r14 + 144]
                                    mov qword [r14 + 136], r10
                                    loop.404.5.426.10.476.10.639.17.676.15.932.9:
                                        if.405.12.426.10.476.10.639.17.676.15.932.9:
                                        cmp.405.12.426.10.476.10.639.17.676.15.932.9:
                                        mov r10, qword [r14 + 128]
                                        cmp qword [r14 + 136], r10
                                        jl if.405.9.426.10.476.10.639.17.676.15.932.9.end
                                        if.405.12.426.10.476.10.639.17.676.15.932.9.code:
                                            mov r10, qword [r14 + 136]
                                            mov qword [r14 + 144], r10
                                            jmp func.tokenizer.skip_whitespace.426.10.476.10.639.17.676.15.932.9.end
                                        if.405.9.426.10.476.10.639.17.676.15.932.9.end:
                                        if.409.12.426.10.476.10.639.17.676.15.932.9:
                                        cmp.409.12.426.10.476.10.639.17.676.15.932.9:
                                        mov r10, qword [r14 + 136]
                                        cmp r10, 127
                                        jae baz_bounds_line_409
                                        cmp byte [r14 + r10], 32
                                        jne loop.404.5.426.10.476.10.639.17.676.15.932.9.end
                                        if.409.12.426.10.476.10.639.17.676.15.932.9.code:
                                        if.409.9.426.10.476.10.639.17.676.15.932.9.end:
                                        add qword [r14 + 136], 1
                                    jmp loop.404.5.426.10.476.10.639.17.676.15.932.9
                                    loop.404.5.426.10.476.10.639.17.676.15.932.9.end:
                                    mov r10, qword [r14 + 136]
                                    mov qword [r14 + 144], r10
                                func.tokenizer.skip_whitespace.426.10.476.10.639.17.676.15.932.9.end:
                                mov r10, qword [r14 + 136]
                                mov qword [r14 + 144], r10
                                loop.428.5.476.10.639.17.676.15.932.9:
                                    if.429.12.476.10.639.17.676.15.932.9:
                                    cmp.429.12.476.10.639.17.676.15.932.9:
                                    mov r10, qword [r14 + 128]
                                    cmp qword [r14 + 144], r10
                                    jge loop.428.5.476.10.639.17.676.15.932.9.end
                                    if.429.12.476.10.639.17.676.15.932.9.code:
                                    if.429.9.476.10.639.17.676.15.932.9.end:
                                    if.430.12.476.10.639.17.676.15.932.9:
                                    cmp.430.12.476.10.639.17.676.15.932.9:
                                    mov r10, qword [r14 + 144]
                                    cmp r10, 127
                                    jae baz_bounds_line_430
                                    cmp byte [r14 + r10], 32
                                    je loop.428.5.476.10.639.17.676.15.932.9.end
                                    if.430.12.476.10.639.17.676.15.932.9.code:
                                    if.430.9.476.10.639.17.676.15.932.9.end:
                                    add qword [r14 + 144], 1
                                jmp loop.428.5.476.10.639.17.676.15.932.9
                                loop.428.5.476.10.639.17.676.15.932.9.end:
                            func.tokenizer.next.476.10.639.17.676.15.932.9.end:
                            cmp.477.11.639.17.676.15.932.9:
                                func.tokenizer.is_empty.477.20.639.17.676.15.932.9:
                                    cmp.454.11.477.20.639.17.676.15.932.9:
                                    mov r10, qword [r14 + 144]
                                    cmp qword [r14 + 136], r10
                                    sete r12b
                                    bool.454.11.477.20.639.17.676.15.932.9.end:
                                func.tokenizer.is_empty.477.20.639.17.676.15.932.9.end:
                            cmp r12b, 0
                            sete r12b
                            bool.477.11.639.17.676.15.932.9.end:
                            if.478.8.639.17.676.15.932.9:
                            cmp.478.8.639.17.676.15.932.9:
                            cmp r12b, 0
                            jne if.478.5.639.17.676.15.932.9.end
                            if.478.8.639.17.676.15.932.9.code:
                                func.printer.print_all.478.20.639.17.676.15.932.9:
                                    func.printer.print.69.10.478.20.639.17.676.15.932.9:
                                        func.printer.print_at.65.10.69.10.478.20.639.17.676.15.932.9:
                                            if.59.8.65.10.69.10.478.20.639.17.676.15.932.9:
                                            cmp.59.8.65.10.69.10.478.20.639.17.676.15.932.9:
                                            cmp byte [rbp + 911], 0
                                            jne func.printer.print_at.65.10.69.10.478.20.639.17.676.15.932.9.end
                                            if.59.8.65.10.69.10.478.20.639.17.676.15.932.9.code:
                                            if.59.5.65.10.69.10.478.20.639.17.676.15.932.9.end:
                                            mov rdi, 1
                                            mov rdx, 10
                                            mov r10, 0
                                            test r10, r10
                                            js baz_bounds_line_61
                                            test rdx, rdx
                                            js baz_bounds_line_61
                                            lea r9, [rdx + r10]
                                            cmp r9, 10
                                            jg baz_bounds_line_61
                                            lea rsi, [rbp + 260706]
                                            add rsi, r10
                                            mov rax, 1
                                            syscall
                                        func.printer.print_at.65.10.69.10.478.20.639.17.676.15.932.9.end:
                                    func.printer.print.69.10.478.20.639.17.676.15.932.9.end:
                                func.printer.print_all.478.20.639.17.676.15.932.9.end:
                            if.478.5.639.17.676.15.932.9.end:
                        func.tokenizer.next_or_say.639.17.676.15.932.9.end:
                    cmp r12b, 0
                    je func.tokenizer.next_name_or_say.676.15.932.9.end
                    if.639.8.676.15.932.9.code:
                    if.639.5.676.15.932.9.end:
                    if.641.8.676.15.932.9:
                    cmp.641.8.676.15.932.9:
                        func.tokenizer.len.641.13.676.15.932.9:
                            mov r12, qword [r14 + 144]
                            sub r12, qword [r14 + 136]
                        func.tokenizer.len.641.13.676.15.932.9.end:
                    cmp r12, 16
                    jle if.641.5.676.15.932.9.end
                    if.641.8.676.15.932.9.code:
                        func.printer.print_all.642.13.676.15.932.9:
                            func.printer.print.69.10.642.13.676.15.932.9:
                                func.printer.print_at.65.10.69.10.642.13.676.15.932.9:
                                    if.59.8.65.10.69.10.642.13.676.15.932.9:
                                    cmp.59.8.65.10.69.10.642.13.676.15.932.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.642.13.676.15.932.9.end
                                    if.59.8.65.10.69.10.642.13.676.15.932.9.code:
                                    if.59.5.65.10.69.10.642.13.676.15.932.9.end:
                                    mov rdi, 1
                                    mov rdx, 14
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 14
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260716]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.642.13.676.15.932.9.end:
                            func.printer.print.69.10.642.13.676.15.932.9.end:
                        func.printer.print_all.642.13.676.15.932.9.end:
                        jmp func.tokenizer.next_name_or_say.676.15.932.9.end
                    if.641.5.676.15.932.9.end:
                    mov r13b, 1
                func.tokenizer.next_name_or_say.676.15.932.9.end:
            cmp r13b, 0
            je func.action_new_object.932.9.end
            if.676.8.932.9.code:
            if.676.5.932.9.end:
            func.tokenizer.token.name.678.19.932.9:
                func.tokenizer.len.458.20.678.19.932.9:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                    mov qword [rbx + 32], r13
                func.tokenizer.len.458.20.678.19.932.9.end:
                mov qword [rbx + 16], 0
                mov qword [rbx + 24], 0
                mov rcx, qword [rbx + 32]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_460
                test rcx, rcx
                js baz_bounds_line_460
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_460
                lea rsi, [r14 + r13]
                cmp rcx, 16
                ja baz_bounds_line_460
                lea rdi, [rbx + 16]
                rep movsb
            func.tokenizer.token.name.678.19.932.9.end:
            func.objects.add.680.29.932.9:
                func.objects.reserve.281.22.680.29.932.9:
                    mov r13, qword [rbp + 137648]
                    mov qword [rbx + 40], r13
                    add qword [rbp + 137648], 1
                func.objects.reserve.281.22.680.29.932.9.end:
                mov r13, qword [rbx + 40]
                cmp r13, 1024
                jae baz_bounds_line_282
                imul r13, 24
                lea rsi, [rbx + 16]
                lea rdi, [rbp + r13 + 113072]
                mov rcx, 24
                rep movsb
            func.objects.add.680.29.932.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_682
            imul r13, 2736
            func.id_list.push.682.49.932.9:
                func.id_list.reserve.157.16.682.49.932.9:
                    mov r12, qword [rbp + r13 + 1216]
                    mov qword [rbx + 48], r12
                    add qword [rbp + r13 + 1216], 1
                func.id_list.reserve.157.16.682.49.932.9.end:
                lea r12, [rbp + r13 + 960]
                mov r10, qword [rbx + 48]
                cmp r10, 32
                jae baz_bounds_line_158
                mov r9, qword [rbx + 40]
                mov qword [r12 + r10 * 8], r9
            func.id_list.push.682.49.932.9.end:
            func.printer.print_all.684.9.932.9:
                func.printer.print.69.10.684.9.932.9:
                    func.printer.print_at.65.10.69.10.684.9.932.9:
                        if.59.8.65.10.69.10.684.9.932.9:
                        cmp.59.8.65.10.69.10.684.9.932.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.684.9.932.9.end
                        if.59.8.65.10.69.10.684.9.932.9.code:
                        if.59.5.65.10.69.10.684.9.932.9.end:
                        mov rdi, 1
                        mov rdx, 19
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 19
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260763]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.684.9.932.9.end:
                func.printer.print.69.10.684.9.932.9.end:
            func.printer.print_all.684.9.932.9.end:
        func.action_new_object.932.9.end:
    jmp if.913.5.end
    if.934.15:
    cmp.934.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.934.18:
            cmp.444.11.934.18:
                func.tokenizer.len.444.16.934.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.934.18.end:
            cmp r13, 1
            sete r15b
            jne bool.444.11.934.18.end
            cmp.445.11.934.18:
                mov rcx, 1
                cmp rcx, 1
                ja baz_bounds_line_445
                lea rsi, [rbp + 261319]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.934.18.end:
        func.tokenizer.is_array.934.18.end:
    cmp r15b, 0
    je if.937.15
    if.934.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_inventory.935.9:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_691
            imul r13, 2736
            mov r12, qword [rbp + r13 + 1216]
            mov qword [rbx + 16], r12
            if.692.9.935.9:
            cmp.692.9.935.9:
            cmp qword [rbx + 16], 0
            jne if.692.5.935.9.end
            if.692.9.935.9.code:
                func.printer.print_all.693.13.935.9:
                    func.printer.print.69.10.693.13.935.9:
                        func.printer.print_at.65.10.69.10.693.13.935.9:
                            if.59.8.65.10.69.10.693.13.935.9:
                            cmp.59.8.65.10.69.10.693.13.935.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.693.13.935.9.end
                            if.59.8.65.10.69.10.693.13.935.9.code:
                            if.59.5.65.10.69.10.693.13.935.9.end:
                            mov rdi, 1
                            mov rdx, 15
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 15
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260790]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.693.13.935.9.end:
                    func.printer.print.69.10.693.13.935.9.end:
                func.printer.print_all.693.13.935.9.end:
                jmp func.action_inventory.935.9.end
            if.692.5.935.9.end:
            mov byte [rbx + 24], 0
            mov r12, qword [r15]
            cmp r12, 32
            jae baz_bounds_line_699
            imul r12, 2736
            lea r13, [rbp + r12 + 960]
            mov r12, qword [rbx + 16]
            cmp r12, 32
            ja baz_bounds_line_699
            mov r10, 0
            cmp r12, 0
            jle foo.699.5.935.9.end
            foo.699.5.935.9:
                if.701.12.935.9:
                cmp.701.12.935.9:
                cmp byte [rbx + 24], 0
                jne if.701.9.935.9.else
                if.701.12.935.9.code:
                    func.printer.print_all.702.17.935.9:
                        func.printer.print.69.10.702.17.935.9:
                            func.printer.print_at.65.10.69.10.702.17.935.9:
                                if.59.8.65.10.69.10.702.17.935.9:
                                cmp.59.8.65.10.69.10.702.17.935.9:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.702.17.935.9.end
                                if.59.8.65.10.69.10.702.17.935.9.code:
                                if.59.5.65.10.69.10.702.17.935.9.end:
                                mov rdi, 1
                                mov rdx, 8
                                mov r9, 0
                                test r9, r9
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r8, [rdx + r9]
                                cmp r8, 8
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 260782]
                                add rsi, r9
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.702.17.935.9.end:
                        func.printer.print.69.10.702.17.935.9.end:
                    func.printer.print_all.702.17.935.9.end:
                    mov byte [rbx + 24], 1
                jmp if.701.9.935.9.end
                if.701.9.935.9.else:
                    func.printer.print_all.705.17.935.9:
                        func.printer.print.69.10.705.17.935.9:
                            func.printer.print_at.65.10.69.10.705.17.935.9:
                                if.59.8.65.10.69.10.705.17.935.9:
                                cmp.59.8.65.10.69.10.705.17.935.9:
                                cmp byte [rbp + 911], 0
                                jne func.printer.print_at.65.10.69.10.705.17.935.9.end
                                if.59.8.65.10.69.10.705.17.935.9.code:
                                if.59.5.65.10.69.10.705.17.935.9.end:
                                mov rdi, 1
                                mov rdx, 2
                                mov r9, 0
                                test r9, r9
                                js baz_bounds_line_61
                                test rdx, rdx
                                js baz_bounds_line_61
                                lea r8, [rdx + r9]
                                cmp r8, 2
                                jg baz_bounds_line_61
                                lea rsi, [rbp + 883]
                                add rsi, r9
                                mov rax, 1
                                syscall
                            func.printer.print_at.65.10.69.10.705.17.935.9.end:
                        func.printer.print.69.10.705.17.935.9.end:
                    func.printer.print_all.705.17.935.9.end:
                if.701.9.935.9.end:
                mov r9, qword [r13]
                cmp r9, 1024
                jae baz_bounds_line_708
                imul r9, 24
                func.name.print.708.31.935.9:
                    func.printer.print.89.9.708.31.935.9:
                        func.printer.print_at.65.10.89.9.708.31.935.9:
                            if.59.8.65.10.89.9.708.31.935.9:
                            cmp.59.8.65.10.89.9.708.31.935.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.89.9.708.31.935.9.end
                            if.59.8.65.10.89.9.708.31.935.9.code:
                            if.59.5.65.10.89.9.708.31.935.9.end:
                            mov rdi, 1
                            mov rdx, qword [rbp + r9 + 113088]
                            mov r8, 0
                            test r8, r8
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r11, [rdx + r8]
                            cmp r11, 16
                            jg baz_bounds_line_61
                            lea rsi, [rbp + r9 + 113072]
                            add rsi, r8
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.89.9.708.31.935.9.end:
                    func.printer.print.89.9.708.31.935.9.end:
                func.name.print.708.31.935.9.end:
                foo.699.5.935.9.continue:
                    add r13, 8
                    inc r10
                    cmp r10, r12
                    jne foo.699.5.935.9
            foo.699.5.935.9.end:
            func.printer.println.711.9.935.9:
                func.printer.print_all.73.10.711.9.935.9:
                    func.printer.print.69.10.73.10.711.9.935.9:
                        func.printer.print_at.65.10.69.10.73.10.711.9.935.9:
                            if.59.8.65.10.69.10.73.10.711.9.935.9:
                            cmp.59.8.65.10.69.10.73.10.711.9.935.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.73.10.711.9.935.9.end
                            if.59.8.65.10.69.10.73.10.711.9.935.9.code:
                            if.59.5.65.10.69.10.73.10.711.9.935.9.end:
                            mov rdi, 1
                            mov rdx, 1
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 1
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 879]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.73.10.711.9.935.9.end:
                    func.printer.print.69.10.73.10.711.9.935.9.end:
                func.printer.print_all.73.10.711.9.935.9.end:
            func.printer.println.711.9.935.9.end:
        func.action_inventory.935.9.end:
    jmp if.913.5.end
    if.937.15:
    cmp.937.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.937.18:
            cmp.444.11.937.18:
                func.tokenizer.len.444.16.937.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.937.18.end:
            cmp r13, 1
            sete r15b
            jne bool.444.11.937.18.end
            cmp.445.11.937.18:
                mov rcx, 1
                cmp rcx, 1
                ja baz_bounds_line_445
                lea rsi, [rbp + 261322]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.937.18.end:
        func.tokenizer.is_array.937.18.end:
    cmp r15b, 0
    je if.940.15
    if.937.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_drop.938.9:
            if.719.8.938.9:
            cmp.719.8.938.9:
                func.tokenizer.next_or_say.719.15.938.9:
                    func.tokenizer.next.476.10.719.15.938.9:
                        func.tokenizer.skip_whitespace.426.10.476.10.719.15.938.9:
                            mov r12, qword [r14 + 144]
                            mov qword [r14 + 136], r12
                            loop.404.5.426.10.476.10.719.15.938.9:
                                if.405.12.426.10.476.10.719.15.938.9:
                                cmp.405.12.426.10.476.10.719.15.938.9:
                                mov r12, qword [r14 + 128]
                                cmp qword [r14 + 136], r12
                                jl if.405.9.426.10.476.10.719.15.938.9.end
                                if.405.12.426.10.476.10.719.15.938.9.code:
                                    mov r12, qword [r14 + 136]
                                    mov qword [r14 + 144], r12
                                    jmp func.tokenizer.skip_whitespace.426.10.476.10.719.15.938.9.end
                                if.405.9.426.10.476.10.719.15.938.9.end:
                                if.409.12.426.10.476.10.719.15.938.9:
                                cmp.409.12.426.10.476.10.719.15.938.9:
                                mov r12, qword [r14 + 136]
                                cmp r12, 127
                                jae baz_bounds_line_409
                                cmp byte [r14 + r12], 32
                                jne loop.404.5.426.10.476.10.719.15.938.9.end
                                if.409.12.426.10.476.10.719.15.938.9.code:
                                if.409.9.426.10.476.10.719.15.938.9.end:
                                add qword [r14 + 136], 1
                            jmp loop.404.5.426.10.476.10.719.15.938.9
                            loop.404.5.426.10.476.10.719.15.938.9.end:
                            mov r12, qword [r14 + 136]
                            mov qword [r14 + 144], r12
                        func.tokenizer.skip_whitespace.426.10.476.10.719.15.938.9.end:
                        mov r12, qword [r14 + 136]
                        mov qword [r14 + 144], r12
                        loop.428.5.476.10.719.15.938.9:
                            if.429.12.476.10.719.15.938.9:
                            cmp.429.12.476.10.719.15.938.9:
                            mov r12, qword [r14 + 128]
                            cmp qword [r14 + 144], r12
                            jge loop.428.5.476.10.719.15.938.9.end
                            if.429.12.476.10.719.15.938.9.code:
                            if.429.9.476.10.719.15.938.9.end:
                            if.430.12.476.10.719.15.938.9:
                            cmp.430.12.476.10.719.15.938.9:
                            mov r12, qword [r14 + 144]
                            cmp r12, 127
                            jae baz_bounds_line_430
                            cmp byte [r14 + r12], 32
                            je loop.428.5.476.10.719.15.938.9.end
                            if.430.12.476.10.719.15.938.9.code:
                            if.430.9.476.10.719.15.938.9.end:
                            add qword [r14 + 144], 1
                        jmp loop.428.5.476.10.719.15.938.9
                        loop.428.5.476.10.719.15.938.9.end:
                    func.tokenizer.next.476.10.719.15.938.9.end:
                    cmp.477.11.719.15.938.9:
                        func.tokenizer.is_empty.477.20.719.15.938.9:
                            cmp.454.11.477.20.719.15.938.9:
                            mov r12, qword [r14 + 144]
                            cmp qword [r14 + 136], r12
                            sete r13b
                            bool.454.11.477.20.719.15.938.9.end:
                        func.tokenizer.is_empty.477.20.719.15.938.9.end:
                    cmp r13b, 0
                    sete r13b
                    bool.477.11.719.15.938.9.end:
                    if.478.8.719.15.938.9:
                    cmp.478.8.719.15.938.9:
                    cmp r13b, 0
                    jne if.478.5.719.15.938.9.end
                    if.478.8.719.15.938.9.code:
                        func.printer.print_all.478.20.719.15.938.9:
                            func.printer.print.69.10.478.20.719.15.938.9:
                                func.printer.print_at.65.10.69.10.478.20.719.15.938.9:
                                    if.59.8.65.10.69.10.478.20.719.15.938.9:
                                    cmp.59.8.65.10.69.10.478.20.719.15.938.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.478.20.719.15.938.9.end
                                    if.59.8.65.10.69.10.478.20.719.15.938.9.code:
                                    if.59.5.65.10.69.10.478.20.719.15.938.9.end:
                                    mov rdi, 1
                                    mov rdx, 10
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 10
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260805]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.478.20.719.15.938.9.end:
                            func.printer.print.69.10.478.20.719.15.938.9.end:
                        func.printer.print_all.478.20.719.15.938.9.end:
                    if.478.5.719.15.938.9.end:
                func.tokenizer.next_or_say.719.15.938.9.end:
            cmp r13b, 0
            je func.action_drop.938.9.end
            if.719.8.938.9.code:
            if.719.5.938.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_722
            imul r13, 2736
            mov r12, qword [rbp + r13 + 1216]
            mov qword [rbx + 16], r12
            mov r13, qword [rbx + 16]
            mov qword [rbx + 24], r13
            mov r12, qword [r15]
            cmp r12, 32
            jae baz_bounds_line_724
            imul r12, 2736
            lea r13, [rbp + r12 + 960]
            mov r12, qword [rbx + 16]
            cmp r12, 32
            ja baz_bounds_line_724
            mov r10, 0
            cmp r12, 0
            jle foo.724.5.938.9.end
            foo.724.5.938.9:
                if.725.12.938.9:
                cmp.725.12.938.9:
                    mov r8, qword [r13]
                    cmp r8, 1024
                    jae baz_bounds_line_725
                    imul r8, 24
                    func.tokenizer.is.name.725.15.938.9:
                        cmp.449.11.725.15.938.9:
                            func.tokenizer.len.449.16.725.15.938.9:
                                mov r11, qword [r14 + 144]
                                sub r11, qword [r14 + 136]
                            func.tokenizer.len.449.16.725.15.938.9.end:
                        cmp r11, qword [rbp + r8 + 113088]
                        sete r9b
                        jne bool.449.11.725.15.938.9.end
                        cmp.450.11.725.15.938.9:
                            mov rcx, qword [rbp + r8 + 113088]
                            cmp rcx, 16
                            ja baz_bounds_line_450
                            lea rsi, [rbp + r8 + 113072]
                            mov r11, qword [r14 + 136]
                            test r11, r11
                            js baz_bounds_line_450
                            lea rdx, [rcx + r11]
                            cmp rdx, 127
                            jg baz_bounds_line_450
                            lea rdi, [r14 + r11]
                            test rcx, rcx
                            repe cmpsb
                            sete r9b
                        bool.449.11.725.15.938.9.end:
                    func.tokenizer.is.name.725.15.938.9.end:
                cmp r9b, 0
                je if.725.9.938.9.end
                if.725.12.938.9.code:
                    mov qword [rbx + 24], r10
                    jmp foo.724.5.938.9.end
                if.725.9.938.9.end:
                foo.724.5.938.9.continue:
                    add r13, 8
                    inc r10
                    cmp r10, r12
                    jne foo.724.5.938.9
            foo.724.5.938.9.end:
            if.731.8.938.9:
            cmp.731.8.938.9:
            mov r13, qword [rbx + 16]
            cmp qword [rbx + 24], r13
            jne if.731.5.938.9.end
            if.731.8.938.9.code:
                func.printer.print_all.732.13.938.9:
                    func.printer.print.69.10.732.13.938.9:
                        func.printer.print_at.65.10.69.10.732.13.938.9:
                            if.59.8.65.10.69.10.732.13.938.9:
                            cmp.59.8.65.10.69.10.732.13.938.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.732.13.938.9.end
                            if.59.8.65.10.69.10.732.13.938.9.code:
                            if.59.5.65.10.69.10.732.13.938.9.end:
                            mov rdi, 1
                            mov rdx, 19
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 19
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260815]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.732.13.938.9.end:
                    func.printer.print.69.10.732.13.938.9.end:
                func.printer.print_all.732.13.938.9.end:
                jmp func.action_drop.938.9.end
            if.731.5.938.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_737
            imul r13, 2736
            lea r13, [rbp + r13 + 960]
            mov r12, qword [rbx + 24]
            cmp r12, 32
            jae baz_bounds_line_737
            mov r10, qword [r13 + r12 * 8]
            mov qword [rbx + 32], r10
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_738
            imul r13, 2736
            mov r12, qword [rbp + r13 + 952]
            mov qword [rbx + 40], r12
            mov r13, qword [rbx + 40]
            cmp r13, 128
            jae baz_bounds_line_739
            imul r13, 960
            func.id_list.push.739.50.938.9:
                func.id_list.reserve.157.16.739.50.938.9:
                    mov r12, qword [rbp + r13 + 138616]
                    mov qword [rbx + 48], r12
                    add qword [rbp + r13 + 138616], 1
                func.id_list.reserve.157.16.739.50.938.9.end:
                lea r12, [rbp + r13 + 138360]
                mov r10, qword [rbx + 48]
                cmp r10, 32
                jae baz_bounds_line_158
                mov r9, qword [rbx + 32]
                mov qword [r12 + r10 * 8], r9
            func.id_list.push.739.50.938.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_740
            imul r13, 2736
            lea r12, [rbp + r13 + 960]
            func.id_list.delete_index.740.33.938.9:
                mov rcx, qword [r12 + 256]
                sub rcx, qword [rbx + 24]
                sub rcx, 1
                mov r10, qword [rbx + 24]
                add r10, 1
                test r10, r10
                js baz_bounds_line_164
                test rcx, rcx
                js baz_bounds_line_164
                lea r9, [rcx + r10]
                cmp r9, 32
                jg baz_bounds_line_164
                lea rsi, [r12 + r10 * 8]
                mov r10, qword [rbx + 24]
                test r10, r10
                js baz_bounds_line_165
                lea r9, [rcx + r10]
                cmp r9, 32
                jg baz_bounds_line_165
                lea rdi, [r12 + r10 * 8]
                shl rcx, 3
                rep movsb
                sub qword [r12 + 256], 1
            func.id_list.delete_index.740.33.938.9.end:
            func.printer.print_all.742.9.938.9:
                func.printer.print.69.10.742.9.938.9:
                    func.printer.print_at.65.10.69.10.742.9.938.9:
                        if.59.8.65.10.69.10.742.9.938.9:
                        cmp.59.8.65.10.69.10.742.9.938.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.742.9.938.9.end
                        if.59.8.65.10.69.10.742.9.938.9.code:
                        if.59.5.65.10.69.10.742.9.938.9.end:
                        mov rdi, 1
                        mov rdx, 8
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 8
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260834]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.742.9.938.9.end:
                func.printer.print.69.10.742.9.938.9.end:
            func.printer.print_all.742.9.938.9.end:
            func.tokenizer.print.743.8.938.9:
                func.tokenizer.len.440.39.743.8.938.9:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.440.39.743.8.938.9.end:
                func.printer.print_at.440.9.743.8.938.9:
                    if.59.8.440.9.743.8.938.9:
                    cmp.59.8.440.9.743.8.938.9:
                    cmp byte [rbp + 911], 0
                    jne func.printer.print_at.440.9.743.8.938.9.end
                    if.59.8.440.9.743.8.938.9.code:
                    if.59.5.440.9.743.8.938.9.end:
                    mov rdi, 1
                    mov rdx, r13
                    mov r12, qword [r14 + 136]
                    test r12, r12
                    js baz_bounds_line_61
                    test rdx, rdx
                    js baz_bounds_line_61
                    lea r10, [rdx + r12]
                    cmp r10, 127
                    jg baz_bounds_line_61
                    lea rsi, [r14]
                    add rsi, r12
                    mov rax, 1
                    syscall
                func.printer.print_at.440.9.743.8.938.9.end:
            func.tokenizer.print.743.8.938.9.end:
            func.printer.println.744.9.938.9:
                func.printer.print_all.73.10.744.9.938.9:
                    func.printer.print.69.10.73.10.744.9.938.9:
                        func.printer.print_at.65.10.69.10.73.10.744.9.938.9:
                            if.59.8.65.10.69.10.73.10.744.9.938.9:
                            cmp.59.8.65.10.69.10.73.10.744.9.938.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.73.10.744.9.938.9.end
                            if.59.8.65.10.69.10.73.10.744.9.938.9.code:
                            if.59.5.65.10.69.10.73.10.744.9.938.9.end:
                            mov rdi, 1
                            mov rdx, 1
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 1
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 879]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.73.10.744.9.938.9.end:
                    func.printer.print.69.10.73.10.744.9.938.9.end:
                func.printer.print_all.73.10.744.9.938.9.end:
            func.printer.println.744.9.938.9.end:
        func.action_drop.938.9.end:
    jmp if.913.5.end
    if.940.15:
    cmp.940.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.940.18:
            cmp.444.11.940.18:
                func.tokenizer.len.444.16.940.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.940.18.end:
            cmp r13, 1
            sete r15b
            jne bool.444.11.940.18.end
            cmp.445.11.940.18:
                mov rcx, 1
                cmp rcx, 1
                ja baz_bounds_line_445
                lea rsi, [rbp + 261323]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.940.18.end:
        func.tokenizer.is_array.940.18.end:
    cmp r15b, 0
    je if.943.15
    if.940.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_give.941.9:
            if.754.8.941.9:
            cmp.754.8.941.9:
                func.tokenizer.next_or_say.754.15.941.9:
                    func.tokenizer.next.476.10.754.15.941.9:
                        func.tokenizer.skip_whitespace.426.10.476.10.754.15.941.9:
                            mov r12, qword [r14 + 144]
                            mov qword [r14 + 136], r12
                            loop.404.5.426.10.476.10.754.15.941.9:
                                if.405.12.426.10.476.10.754.15.941.9:
                                cmp.405.12.426.10.476.10.754.15.941.9:
                                mov r12, qword [r14 + 128]
                                cmp qword [r14 + 136], r12
                                jl if.405.9.426.10.476.10.754.15.941.9.end
                                if.405.12.426.10.476.10.754.15.941.9.code:
                                    mov r12, qword [r14 + 136]
                                    mov qword [r14 + 144], r12
                                    jmp func.tokenizer.skip_whitespace.426.10.476.10.754.15.941.9.end
                                if.405.9.426.10.476.10.754.15.941.9.end:
                                if.409.12.426.10.476.10.754.15.941.9:
                                cmp.409.12.426.10.476.10.754.15.941.9:
                                mov r12, qword [r14 + 136]
                                cmp r12, 127
                                jae baz_bounds_line_409
                                cmp byte [r14 + r12], 32
                                jne loop.404.5.426.10.476.10.754.15.941.9.end
                                if.409.12.426.10.476.10.754.15.941.9.code:
                                if.409.9.426.10.476.10.754.15.941.9.end:
                                add qword [r14 + 136], 1
                            jmp loop.404.5.426.10.476.10.754.15.941.9
                            loop.404.5.426.10.476.10.754.15.941.9.end:
                            mov r12, qword [r14 + 136]
                            mov qword [r14 + 144], r12
                        func.tokenizer.skip_whitespace.426.10.476.10.754.15.941.9.end:
                        mov r12, qword [r14 + 136]
                        mov qword [r14 + 144], r12
                        loop.428.5.476.10.754.15.941.9:
                            if.429.12.476.10.754.15.941.9:
                            cmp.429.12.476.10.754.15.941.9:
                            mov r12, qword [r14 + 128]
                            cmp qword [r14 + 144], r12
                            jge loop.428.5.476.10.754.15.941.9.end
                            if.429.12.476.10.754.15.941.9.code:
                            if.429.9.476.10.754.15.941.9.end:
                            if.430.12.476.10.754.15.941.9:
                            cmp.430.12.476.10.754.15.941.9:
                            mov r12, qword [r14 + 144]
                            cmp r12, 127
                            jae baz_bounds_line_430
                            cmp byte [r14 + r12], 32
                            je loop.428.5.476.10.754.15.941.9.end
                            if.430.12.476.10.754.15.941.9.code:
                            if.430.9.476.10.754.15.941.9.end:
                            add qword [r14 + 144], 1
                        jmp loop.428.5.476.10.754.15.941.9
                        loop.428.5.476.10.754.15.941.9.end:
                    func.tokenizer.next.476.10.754.15.941.9.end:
                    cmp.477.11.754.15.941.9:
                        func.tokenizer.is_empty.477.20.754.15.941.9:
                            cmp.454.11.477.20.754.15.941.9:
                            mov r12, qword [r14 + 144]
                            cmp qword [r14 + 136], r12
                            sete r13b
                            bool.454.11.477.20.754.15.941.9.end:
                        func.tokenizer.is_empty.477.20.754.15.941.9.end:
                    cmp r13b, 0
                    sete r13b
                    bool.477.11.754.15.941.9.end:
                    if.478.8.754.15.941.9:
                    cmp.478.8.754.15.941.9:
                    cmp r13b, 0
                    jne if.478.5.754.15.941.9.end
                    if.478.8.754.15.941.9.code:
                        func.printer.print_all.478.20.754.15.941.9:
                            func.printer.print.69.10.478.20.754.15.941.9:
                                func.printer.print_at.65.10.69.10.478.20.754.15.941.9:
                                    if.59.8.65.10.69.10.478.20.754.15.941.9:
                                    cmp.59.8.65.10.69.10.478.20.754.15.941.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.478.20.754.15.941.9.end
                                    if.59.8.65.10.69.10.478.20.754.15.941.9.code:
                                    if.59.5.65.10.69.10.478.20.754.15.941.9.end:
                                    mov rdi, 1
                                    mov rdx, 10
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 10
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260842]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.478.20.754.15.941.9.end:
                            func.printer.print.69.10.478.20.754.15.941.9.end:
                        func.printer.print_all.478.20.754.15.941.9.end:
                    if.478.5.754.15.941.9.end:
                func.tokenizer.next_or_say.754.15.941.9.end:
            cmp r13b, 0
            je func.action_give.941.9.end
            if.754.8.941.9.code:
            if.754.5.941.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_757
            imul r13, 2736
            mov r12, qword [rbp + r13 + 1216]
            mov qword [rbx + 16], r12
            mov r13, qword [rbx + 16]
            mov qword [rbx + 24], r13
            mov r12, qword [r15]
            cmp r12, 32
            jae baz_bounds_line_759
            imul r12, 2736
            lea r13, [rbp + r12 + 960]
            mov r12, qword [rbx + 16]
            cmp r12, 32
            ja baz_bounds_line_759
            mov r10, 0
            cmp r12, 0
            jle foo.759.5.941.9.end
            foo.759.5.941.9:
                if.760.12.941.9:
                cmp.760.12.941.9:
                    mov r8, qword [r13]
                    cmp r8, 1024
                    jae baz_bounds_line_760
                    imul r8, 24
                    func.tokenizer.is.name.760.15.941.9:
                        cmp.449.11.760.15.941.9:
                            func.tokenizer.len.449.16.760.15.941.9:
                                mov r11, qword [r14 + 144]
                                sub r11, qword [r14 + 136]
                            func.tokenizer.len.449.16.760.15.941.9.end:
                        cmp r11, qword [rbp + r8 + 113088]
                        sete r9b
                        jne bool.449.11.760.15.941.9.end
                        cmp.450.11.760.15.941.9:
                            mov rcx, qword [rbp + r8 + 113088]
                            cmp rcx, 16
                            ja baz_bounds_line_450
                            lea rsi, [rbp + r8 + 113072]
                            mov r11, qword [r14 + 136]
                            test r11, r11
                            js baz_bounds_line_450
                            lea rdx, [rcx + r11]
                            cmp rdx, 127
                            jg baz_bounds_line_450
                            lea rdi, [r14 + r11]
                            test rcx, rcx
                            repe cmpsb
                            sete r9b
                        bool.449.11.760.15.941.9.end:
                    func.tokenizer.is.name.760.15.941.9.end:
                cmp r9b, 0
                je if.760.9.941.9.end
                if.760.12.941.9.code:
                    mov qword [rbx + 24], r10
                    jmp foo.759.5.941.9.end
                if.760.9.941.9.end:
                foo.759.5.941.9.continue:
                    add r13, 8
                    inc r10
                    cmp r10, r12
                    jne foo.759.5.941.9
            foo.759.5.941.9.end:
            if.766.8.941.9:
            cmp.766.8.941.9:
            mov r13, qword [rbx + 16]
            cmp qword [rbx + 24], r13
            jne if.766.5.941.9.end
            if.766.8.941.9.code:
                func.printer.print_all.767.13.941.9:
                    func.printer.print.69.10.767.13.941.9:
                        func.printer.print_at.65.10.69.10.767.13.941.9:
                            if.59.8.65.10.69.10.767.13.941.9:
                            cmp.59.8.65.10.69.10.767.13.941.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.767.13.941.9.end
                            if.59.8.65.10.69.10.767.13.941.9.code:
                            if.59.5.65.10.69.10.767.13.941.9.end:
                            mov rdi, 1
                            mov rdx, 19
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 19
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260815]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.767.13.941.9.end:
                    func.printer.print.69.10.767.13.941.9.end:
                func.printer.print_all.767.13.941.9.end:
                jmp func.action_give.941.9.end
            if.766.5.941.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_771
            imul r13, 2736
            lea r13, [rbp + r13 + 960]
            mov r12, qword [rbx + 24]
            cmp r12, 32
            jae baz_bounds_line_771
            mov r10, qword [r13 + r12 * 8]
            mov qword [rbx + 32], r10
            mov r13, qword [rbx + 32]
            cmp r13, 1024
            jae baz_bounds_line_772
            imul r13, 24
            lea rsi, [rbp + r13 + 113072]
            lea rdi, [rbx + 40]
            mov rcx, 24
            rep movsb
            if.774.8.941.9:
            cmp.774.8.941.9:
                func.tokenizer.next_or_say.774.15.941.9:
                    func.tokenizer.next.476.10.774.15.941.9:
                        func.tokenizer.skip_whitespace.426.10.476.10.774.15.941.9:
                            mov r12, qword [r14 + 144]
                            mov qword [r14 + 136], r12
                            loop.404.5.426.10.476.10.774.15.941.9:
                                if.405.12.426.10.476.10.774.15.941.9:
                                cmp.405.12.426.10.476.10.774.15.941.9:
                                mov r12, qword [r14 + 128]
                                cmp qword [r14 + 136], r12
                                jl if.405.9.426.10.476.10.774.15.941.9.end
                                if.405.12.426.10.476.10.774.15.941.9.code:
                                    mov r12, qword [r14 + 136]
                                    mov qword [r14 + 144], r12
                                    jmp func.tokenizer.skip_whitespace.426.10.476.10.774.15.941.9.end
                                if.405.9.426.10.476.10.774.15.941.9.end:
                                if.409.12.426.10.476.10.774.15.941.9:
                                cmp.409.12.426.10.476.10.774.15.941.9:
                                mov r12, qword [r14 + 136]
                                cmp r12, 127
                                jae baz_bounds_line_409
                                cmp byte [r14 + r12], 32
                                jne loop.404.5.426.10.476.10.774.15.941.9.end
                                if.409.12.426.10.476.10.774.15.941.9.code:
                                if.409.9.426.10.476.10.774.15.941.9.end:
                                add qword [r14 + 136], 1
                            jmp loop.404.5.426.10.476.10.774.15.941.9
                            loop.404.5.426.10.476.10.774.15.941.9.end:
                            mov r12, qword [r14 + 136]
                            mov qword [r14 + 144], r12
                        func.tokenizer.skip_whitespace.426.10.476.10.774.15.941.9.end:
                        mov r12, qword [r14 + 136]
                        mov qword [r14 + 144], r12
                        loop.428.5.476.10.774.15.941.9:
                            if.429.12.476.10.774.15.941.9:
                            cmp.429.12.476.10.774.15.941.9:
                            mov r12, qword [r14 + 128]
                            cmp qword [r14 + 144], r12
                            jge loop.428.5.476.10.774.15.941.9.end
                            if.429.12.476.10.774.15.941.9.code:
                            if.429.9.476.10.774.15.941.9.end:
                            if.430.12.476.10.774.15.941.9:
                            cmp.430.12.476.10.774.15.941.9:
                            mov r12, qword [r14 + 144]
                            cmp r12, 127
                            jae baz_bounds_line_430
                            cmp byte [r14 + r12], 32
                            je loop.428.5.476.10.774.15.941.9.end
                            if.430.12.476.10.774.15.941.9.code:
                            if.430.9.476.10.774.15.941.9.end:
                            add qword [r14 + 144], 1
                        jmp loop.428.5.476.10.774.15.941.9
                        loop.428.5.476.10.774.15.941.9.end:
                    func.tokenizer.next.476.10.774.15.941.9.end:
                    cmp.477.11.774.15.941.9:
                        func.tokenizer.is_empty.477.20.774.15.941.9:
                            cmp.454.11.477.20.774.15.941.9:
                            mov r12, qword [r14 + 144]
                            cmp qword [r14 + 136], r12
                            sete r13b
                            bool.454.11.477.20.774.15.941.9.end:
                        func.tokenizer.is_empty.477.20.774.15.941.9.end:
                    cmp r13b, 0
                    sete r13b
                    bool.477.11.774.15.941.9.end:
                    if.478.8.774.15.941.9:
                    cmp.478.8.774.15.941.9:
                    cmp r13b, 0
                    jne if.478.5.774.15.941.9.end
                    if.478.8.774.15.941.9.code:
                        func.printer.print_all.478.20.774.15.941.9:
                            func.printer.print.69.10.478.20.774.15.941.9:
                                func.printer.print_at.65.10.69.10.478.20.774.15.941.9:
                                    if.59.8.65.10.69.10.478.20.774.15.941.9:
                                    cmp.59.8.65.10.69.10.478.20.774.15.941.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.478.20.774.15.941.9.end
                                    if.59.8.65.10.69.10.478.20.774.15.941.9.code:
                                    if.59.5.65.10.69.10.478.20.774.15.941.9.end:
                                    mov rdi, 1
                                    mov rdx, 13
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 13
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260852]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.478.20.774.15.941.9.end:
                            func.printer.print.69.10.478.20.774.15.941.9.end:
                        func.printer.print_all.478.20.774.15.941.9.end:
                    if.478.5.774.15.941.9.end:
                func.tokenizer.next_or_say.774.15.941.9.end:
            cmp r13b, 0
            je func.action_give.941.9.end
            if.774.8.941.9.code:
            if.774.5.941.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_777
            imul r13, 2736
            mov r12, qword [rbp + r13 + 952]
            mov qword [rbx + 64], r12
            mov r13, qword [rbx + 64]
            cmp r13, 128
            jae baz_bounds_line_778
            imul r13, 960
            mov r12, qword [rbp + r13 + 138216]
            mov qword [rbx + 72], r12
            mov r13, qword [rbx + 72]
            mov qword [rbx + 80], r13
            mov r12, qword [rbx + 64]
            cmp r12, 128
            jae baz_bounds_line_780
            imul r12, 960
            lea r13, [rbp + r12 + 137960]
            mov r12, qword [rbx + 72]
            cmp r12, 32
            ja baz_bounds_line_780
            mov r10, 0
            cmp r12, 0
            jle foo.780.5.941.9.end
            foo.780.5.941.9:
                if.781.12.941.9:
                cmp.781.12.941.9:
                mov r9, qword [r15]
                cmp qword [r13], r9
                je foo.780.5.941.9.continue
                if.781.12.941.9.code:
                if.781.9.941.9.end:
                if.782.12.941.9:
                cmp.782.12.941.9:
                    mov r8, qword [r13]
                    cmp r8, 32
                    jae baz_bounds_line_782
                    imul r8, 2736
                    func.tokenizer.is.name.782.15.941.9:
                        cmp.449.11.782.15.941.9:
                            func.tokenizer.len.449.16.782.15.941.9:
                                mov r11, qword [r14 + 144]
                                sub r11, qword [r14 + 136]
                            func.tokenizer.len.449.16.782.15.941.9.end:
                        cmp r11, qword [rbp + r8 + 944]
                        sete r9b
                        jne bool.449.11.782.15.941.9.end
                        cmp.450.11.782.15.941.9:
                            mov rcx, qword [rbp + r8 + 944]
                            cmp rcx, 16
                            ja baz_bounds_line_450
                            lea rsi, [rbp + r8 + 928]
                            mov r11, qword [r14 + 136]
                            test r11, r11
                            js baz_bounds_line_450
                            lea rdx, [rcx + r11]
                            cmp rdx, 127
                            jg baz_bounds_line_450
                            lea rdi, [r14 + r11]
                            test rcx, rcx
                            repe cmpsb
                            sete r9b
                        bool.449.11.782.15.941.9.end:
                    func.tokenizer.is.name.782.15.941.9.end:
                cmp r9b, 0
                je if.782.9.941.9.end
                if.782.12.941.9.code:
                    mov r9, qword [r13]
                    mov qword [rbx + 80], r9
                    jmp foo.780.5.941.9.end
                if.782.9.941.9.end:
                foo.780.5.941.9.continue:
                    add r13, 8
                    inc r10
                    cmp r10, r12
                    jne foo.780.5.941.9
            foo.780.5.941.9.end:
            if.788.8.941.9:
            cmp.788.8.941.9:
            mov r13, qword [rbx + 72]
            cmp qword [rbx + 80], r13
            jne if.788.5.941.9.end
            if.788.8.941.9.code:
                func.tokenizer.print.789.12.941.9:
                    func.tokenizer.len.440.39.789.12.941.9:
                        mov r13, qword [r14 + 144]
                        sub r13, qword [r14 + 136]
                    func.tokenizer.len.440.39.789.12.941.9.end:
                    func.printer.print_at.440.9.789.12.941.9:
                        if.59.8.440.9.789.12.941.9:
                        cmp.59.8.440.9.789.12.941.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.440.9.789.12.941.9.end
                        if.59.8.440.9.789.12.941.9.code:
                        if.59.5.440.9.789.12.941.9.end:
                        mov rdi, 1
                        mov rdx, r13
                        mov r12, qword [r14 + 136]
                        test r12, r12
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r10, [rdx + r12]
                        cmp r10, 127
                        jg baz_bounds_line_61
                        lea rsi, [r14]
                        add rsi, r12
                        mov rax, 1
                        syscall
                    func.printer.print_at.440.9.789.12.941.9.end:
                func.tokenizer.print.789.12.941.9.end:
                func.printer.print_all.790.13.941.9:
                    func.printer.print.69.10.790.13.941.9:
                        func.printer.print_at.65.10.69.10.790.13.941.9:
                            if.59.8.65.10.69.10.790.13.941.9:
                            cmp.59.8.65.10.69.10.790.13.941.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.790.13.941.9.end
                            if.59.8.65.10.69.10.790.13.941.9.code:
                            if.59.5.65.10.69.10.790.13.941.9.end:
                            mov rdi, 1
                            mov rdx, 13
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 13
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260865]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.790.13.941.9.end:
                    func.printer.print.69.10.790.13.941.9.end:
                func.printer.print_all.790.13.941.9.end:
                jmp func.action_give.941.9.end
            if.788.5.941.9.end:
            mov r13, qword [rbx + 80]
            cmp r13, 32
            jae baz_bounds_line_795
            imul r13, 2736
            func.id_list.push.795.55.941.9:
                func.id_list.reserve.157.16.795.55.941.9:
                    mov r12, qword [rbp + r13 + 1216]
                    mov qword [rbx + 88], r12
                    add qword [rbp + r13 + 1216], 1
                func.id_list.reserve.157.16.795.55.941.9.end:
                lea r12, [rbp + r13 + 960]
                mov r10, qword [rbx + 88]
                cmp r10, 32
                jae baz_bounds_line_158
                mov r9, qword [rbx + 32]
                mov qword [r12 + r10 * 8], r9
            func.id_list.push.795.55.941.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_796
            imul r13, 2736
            lea r12, [rbp + r13 + 960]
            func.id_list.delete_index.796.33.941.9:
                mov rcx, qword [r12 + 256]
                sub rcx, qword [rbx + 24]
                sub rcx, 1
                mov r10, qword [rbx + 24]
                add r10, 1
                test r10, r10
                js baz_bounds_line_164
                test rcx, rcx
                js baz_bounds_line_164
                lea r9, [rcx + r10]
                cmp r9, 32
                jg baz_bounds_line_164
                lea rsi, [r12 + r10 * 8]
                mov r10, qword [rbx + 24]
                test r10, r10
                js baz_bounds_line_165
                lea r9, [rcx + r10]
                cmp r9, 32
                jg baz_bounds_line_165
                lea rdi, [r12 + r10 * 8]
                shl rcx, 3
                rep movsb
                sub qword [r12 + 256], 1
            func.id_list.delete_index.796.33.941.9.end:
            func.printer.print_all.798.9.941.9:
                func.printer.print.69.10.798.9.941.9:
                    func.printer.print_at.65.10.69.10.798.9.941.9:
                        if.59.8.65.10.69.10.798.9.941.9:
                        cmp.59.8.65.10.69.10.798.9.941.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.798.9.941.9.end
                        if.59.8.65.10.69.10.798.9.941.9.code:
                        if.59.5.65.10.69.10.798.9.941.9.end:
                        mov rdi, 1
                        mov rdx, 5
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 5
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260878]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.798.9.941.9.end:
                func.printer.print.69.10.798.9.941.9.end:
            func.printer.print_all.798.9.941.9.end:
            func.name.print.799.17.941.9:
                func.printer.print.89.9.799.17.941.9:
                    func.printer.print_at.65.10.89.9.799.17.941.9:
                        if.59.8.65.10.89.9.799.17.941.9:
                        cmp.59.8.65.10.89.9.799.17.941.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.89.9.799.17.941.9.end
                        if.59.8.65.10.89.9.799.17.941.9.code:
                        if.59.5.65.10.89.9.799.17.941.9.end:
                        mov rdi, 1
                        mov rdx, qword [rbx + 56]
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 16
                        jg baz_bounds_line_61
                        lea rsi, [rbx + 40]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.89.9.799.17.941.9.end:
                func.printer.print.89.9.799.17.941.9.end:
            func.name.print.799.17.941.9.end:
            func.printer.print_all.800.9.941.9:
                func.printer.print.69.10.800.9.941.9:
                    func.printer.print_at.65.10.69.10.800.9.941.9:
                        if.59.8.65.10.69.10.800.9.941.9:
                        cmp.59.8.65.10.69.10.800.9.941.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.800.9.941.9.end
                        if.59.8.65.10.69.10.800.9.941.9.code:
                        if.59.5.65.10.69.10.800.9.941.9.end:
                        mov rdi, 1
                        mov rdx, 4
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 4
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260883]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.800.9.941.9.end:
                func.printer.print.69.10.800.9.941.9.end:
            func.printer.print_all.800.9.941.9.end:
            mov r13, qword [rbx + 80]
            cmp r13, 32
            jae baz_bounds_line_801
            imul r13, 2736
            func.name.print.801.36.941.9:
                func.printer.print.89.9.801.36.941.9:
                    func.printer.print_at.65.10.89.9.801.36.941.9:
                        if.59.8.65.10.89.9.801.36.941.9:
                        cmp.59.8.65.10.89.9.801.36.941.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.89.9.801.36.941.9.end
                        if.59.8.65.10.89.9.801.36.941.9.code:
                        if.59.5.65.10.89.9.801.36.941.9.end:
                        mov rdi, 1
                        mov rdx, qword [rbp + r13 + 944]
                        mov r12, 0
                        test r12, r12
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r10, [rdx + r12]
                        cmp r10, 16
                        jg baz_bounds_line_61
                        lea rsi, [rbp + r13 + 928]
                        add rsi, r12
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.89.9.801.36.941.9.end:
                func.printer.print.89.9.801.36.941.9.end:
            func.name.print.801.36.941.9.end:
            func.printer.println.802.9.941.9:
                func.printer.print_all.73.10.802.9.941.9:
                    func.printer.print.69.10.73.10.802.9.941.9:
                        func.printer.print_at.65.10.69.10.73.10.802.9.941.9:
                            if.59.8.65.10.69.10.73.10.802.9.941.9:
                            cmp.59.8.65.10.69.10.73.10.802.9.941.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.73.10.802.9.941.9.end
                            if.59.8.65.10.69.10.73.10.802.9.941.9.code:
                            if.59.5.65.10.69.10.73.10.802.9.941.9.end:
                            mov rdi, 1
                            mov rdx, 1
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 1
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 879]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.73.10.802.9.941.9.end:
                    func.printer.print.69.10.73.10.802.9.941.9.end:
                func.printer.print_all.73.10.802.9.941.9.end:
            func.printer.println.802.9.941.9.end:
            xor al, al
            lea rdi, [rbx + 96]
            mov rcx, 136
            rep stosb
            func.str.add.805.15.941.9:
                mov r13, 5
                cmp r13, 5
                ja baz_bounds_line_93
                mov r12, qword [rbx + 224]
                test r12, r12
                js baz_bounds_line_93
                lea r10, [r13 + r12]
                cmp r10, 127
                jg baz_bounds_line_93
                mov eax, dword [rbp + 260878]
                mov dword [rbx + r12 + 96], eax
                mov al, byte [rbp + 260882]
                mov byte [rbx + r12 + 100], al
                add qword [rbx + 224], 5
            func.str.add.805.15.941.9.end:
            mov r13, qword [rbx + 80]
            cmp r13, 32
            jae baz_bounds_line_806
            imul r13, 2736
            func.str.append.name.806.15.941.9:
                mov rcx, qword [rbp + r13 + 944]
                cmp rcx, 16
                ja baz_bounds_line_99
                lea rsi, [rbp + r13 + 928]
                mov r12, qword [rbx + 224]
                test r12, r12
                js baz_bounds_line_99
                lea r10, [rcx + r12]
                cmp r10, 127
                jg baz_bounds_line_99
                lea rdi, [rbx + r12 + 96]
                rep movsb
                mov r12, qword [rbp + r13 + 944]
                add qword [rbx + 224], r12
            func.str.append.name.806.15.941.9.end:
            func.str.add.807.15.941.9:
                mov r13, 1
                cmp r13, 1
                ja baz_bounds_line_93
                mov r12, qword [rbx + 224]
                test r12, r12
                js baz_bounds_line_93
                lea r10, [r13 + r12]
                cmp r10, 127
                jg baz_bounds_line_93
                mov al, byte [rbp + 926]
                mov byte [rbx + r12 + 96], al
                add qword [rbx + 224], 1
            func.str.add.807.15.941.9.end:
            func.str.append.name.808.15.941.9:
                mov rcx, qword [rbx + 56]
                cmp rcx, 16
                ja baz_bounds_line_99
                lea rsi, [rbx + 40]
                mov r13, qword [rbx + 224]
                test r13, r13
                js baz_bounds_line_99
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_99
                lea rdi, [rbx + r13 + 96]
                rep movsb
                mov r13, qword [rbx + 56]
                add qword [rbx + 224], r13
            func.str.append.name.808.15.941.9.end:
            func.notify_room.810.5.941.9:
                mov r13, qword [rbx + 64]
                cmp r13, 128
                jae baz_bounds_line_482
                imul r13, 960
                mov r12, qword [rbp + r13 + 138216]
                mov qword [rbx + 232], r12
                mov r12, qword [rbx + 64]
                cmp r12, 128
                jae baz_bounds_line_483
                imul r12, 960
                lea r13, [rbp + r12 + 137960]
                mov r12, qword [rbx + 232]
                cmp r12, 32
                ja baz_bounds_line_483
                mov r10, 0
                cmp r12, 0
                jle foo.483.5.810.5.941.9.end
                foo.483.5.810.5.941.9:
                    if.484.12.810.5.941.9:
                    cmp.484.12.810.5.941.9:
                    mov r9, qword [r15]
                    cmp qword [r13], r9
                    je foo.483.5.810.5.941.9.continue
                    if.484.12.810.5.941.9.code:
                    if.484.9.810.5.941.9.end:
                    if.486.12.810.5.941.9:
                    cmp.486.12.810.5.941.9:
                    mov r9, qword [r13]
                    cmp r9, 32
                    jae baz_bounds_line_486
                    imul r9, 2736
                    cmp qword [rbp + r9 + 3656], 16
                    je foo.483.5.810.5.941.9.continue
                    if.486.12.810.5.941.9.code:
                    if.486.9.810.5.941.9.end:
                    mov r9, qword [r13]
                    cmp r9, 32
                    jae baz_bounds_line_488
                    imul r9, 2736
                    func.messages.add.488.36.810.5.941.9:
                        func.messages.reserve.212.19.488.36.810.5.941.9:
                            mov r8, qword [rbp + r9 + 3656]
                            mov qword [rbx + 248], r8
                            add qword [rbp + r9 + 3656], 1
                        func.messages.reserve.212.19.488.36.810.5.941.9.end:
                        lea r8, [rbp + r9 + 1224]
                        mov r11, qword [rbx + 248]
                        cmp r11, 16
                        jae baz_bounds_line_213
                        imul r11, 152
                        mov rdx, qword [r15]
                        mov qword [r8 + r11], rdx
                        mov qword [r8 + r11 + 8], 2
                        lea rsi, [rbx + 96]
                        lea rdi, [r8 + r11 + 16]
                        mov rcx, 136
                        rep movsb
                    func.messages.add.488.36.810.5.941.9.end:
                    foo.483.5.810.5.941.9.continue:
                        add r13, 8
                        inc r10
                        cmp r10, r12
                        jne foo.483.5.810.5.941.9
                foo.483.5.810.5.941.9.end:
            func.notify_room.810.5.941.9.end:
        func.action_give.941.9.end:
    jmp if.913.5.end
    if.943.15:
    cmp.943.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.943.18:
            cmp.444.11.943.18:
                func.tokenizer.len.444.16.943.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.943.18.end:
            cmp r13, 1
            sete r15b
            jne bool.444.11.943.18.end
            cmp.445.11.943.18:
                mov rcx, 1
                cmp rcx, 1
                ja baz_bounds_line_445
                lea rsi, [rbp + 261324]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.943.18.end:
        func.tokenizer.is_array.943.18.end:
    cmp r15b, 0
    je if.946.15
    if.943.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_tell.944.9:
            if.819.8.944.9:
            cmp.819.8.944.9:
                func.tokenizer.next_or_say.819.15.944.9:
                    func.tokenizer.next.476.10.819.15.944.9:
                        func.tokenizer.skip_whitespace.426.10.476.10.819.15.944.9:
                            mov r12, qword [r14 + 144]
                            mov qword [r14 + 136], r12
                            loop.404.5.426.10.476.10.819.15.944.9:
                                if.405.12.426.10.476.10.819.15.944.9:
                                cmp.405.12.426.10.476.10.819.15.944.9:
                                mov r12, qword [r14 + 128]
                                cmp qword [r14 + 136], r12
                                jl if.405.9.426.10.476.10.819.15.944.9.end
                                if.405.12.426.10.476.10.819.15.944.9.code:
                                    mov r12, qword [r14 + 136]
                                    mov qword [r14 + 144], r12
                                    jmp func.tokenizer.skip_whitespace.426.10.476.10.819.15.944.9.end
                                if.405.9.426.10.476.10.819.15.944.9.end:
                                if.409.12.426.10.476.10.819.15.944.9:
                                cmp.409.12.426.10.476.10.819.15.944.9:
                                mov r12, qword [r14 + 136]
                                cmp r12, 127
                                jae baz_bounds_line_409
                                cmp byte [r14 + r12], 32
                                jne loop.404.5.426.10.476.10.819.15.944.9.end
                                if.409.12.426.10.476.10.819.15.944.9.code:
                                if.409.9.426.10.476.10.819.15.944.9.end:
                                add qword [r14 + 136], 1
                            jmp loop.404.5.426.10.476.10.819.15.944.9
                            loop.404.5.426.10.476.10.819.15.944.9.end:
                            mov r12, qword [r14 + 136]
                            mov qword [r14 + 144], r12
                        func.tokenizer.skip_whitespace.426.10.476.10.819.15.944.9.end:
                        mov r12, qword [r14 + 136]
                        mov qword [r14 + 144], r12
                        loop.428.5.476.10.819.15.944.9:
                            if.429.12.476.10.819.15.944.9:
                            cmp.429.12.476.10.819.15.944.9:
                            mov r12, qword [r14 + 128]
                            cmp qword [r14 + 144], r12
                            jge loop.428.5.476.10.819.15.944.9.end
                            if.429.12.476.10.819.15.944.9.code:
                            if.429.9.476.10.819.15.944.9.end:
                            if.430.12.476.10.819.15.944.9:
                            cmp.430.12.476.10.819.15.944.9:
                            mov r12, qword [r14 + 144]
                            cmp r12, 127
                            jae baz_bounds_line_430
                            cmp byte [r14 + r12], 32
                            je loop.428.5.476.10.819.15.944.9.end
                            if.430.12.476.10.819.15.944.9.code:
                            if.430.9.476.10.819.15.944.9.end:
                            add qword [r14 + 144], 1
                        jmp loop.428.5.476.10.819.15.944.9
                        loop.428.5.476.10.819.15.944.9.end:
                    func.tokenizer.next.476.10.819.15.944.9.end:
                    cmp.477.11.819.15.944.9:
                        func.tokenizer.is_empty.477.20.819.15.944.9:
                            cmp.454.11.477.20.819.15.944.9:
                            mov r12, qword [r14 + 144]
                            cmp qword [r14 + 136], r12
                            sete r13b
                            bool.454.11.477.20.819.15.944.9.end:
                        func.tokenizer.is_empty.477.20.819.15.944.9.end:
                    cmp r13b, 0
                    sete r13b
                    bool.477.11.819.15.944.9.end:
                    if.478.8.819.15.944.9:
                    cmp.478.8.819.15.944.9:
                    cmp r13b, 0
                    jne if.478.5.819.15.944.9.end
                    if.478.8.819.15.944.9.code:
                        func.printer.print_all.478.20.819.15.944.9:
                            func.printer.print.69.10.478.20.819.15.944.9:
                                func.printer.print_at.65.10.69.10.478.20.819.15.944.9:
                                    if.59.8.65.10.69.10.478.20.819.15.944.9:
                                    cmp.59.8.65.10.69.10.478.20.819.15.944.9:
                                    cmp byte [rbp + 911], 0
                                    jne func.printer.print_at.65.10.69.10.478.20.819.15.944.9.end
                                    if.59.8.65.10.69.10.478.20.819.15.944.9.code:
                                    if.59.5.65.10.69.10.478.20.819.15.944.9.end:
                                    mov rdi, 1
                                    mov rdx, 10
                                    mov r12, 0
                                    test r12, r12
                                    js baz_bounds_line_61
                                    test rdx, rdx
                                    js baz_bounds_line_61
                                    lea r10, [rdx + r12]
                                    cmp r10, 10
                                    jg baz_bounds_line_61
                                    lea rsi, [rbp + 260887]
                                    add rsi, r12
                                    mov rax, 1
                                    syscall
                                func.printer.print_at.65.10.69.10.478.20.819.15.944.9.end:
                            func.printer.print.69.10.478.20.819.15.944.9.end:
                        func.printer.print_all.478.20.819.15.944.9.end:
                    if.478.5.819.15.944.9.end:
                func.tokenizer.next_or_say.819.15.944.9.end:
            cmp r13b, 0
            je func.action_tell.944.9.end
            if.819.8.944.9.code:
            if.819.5.944.9.end:
            mov r13, qword [rbp + 88480]
            mov qword [rbx + 16], r13
            lea r13, [rbp + 928]
            mov r12, qword [rbp + 88480]
            cmp r12, 32
            ja baz_bounds_line_823
            mov r10, 0
            cmp r12, 0
            jle foo.823.5.944.9.end
            foo.823.5.944.9:
                if.824.12.944.9:
                cmp.824.12.944.9:
                    func.tokenizer.is.name.824.15.944.9:
                        cmp.449.11.824.15.944.9:
                            func.tokenizer.len.449.16.824.15.944.9:
                                mov r8, qword [r14 + 144]
                                sub r8, qword [r14 + 136]
                            func.tokenizer.len.449.16.824.15.944.9.end:
                        cmp r8, qword [r13 + 16]
                        sete r9b
                        jne bool.449.11.824.15.944.9.end
                        cmp.450.11.824.15.944.9:
                            mov rcx, qword [r13 + 16]
                            cmp rcx, 16
                            ja baz_bounds_line_450
                            lea rsi, [r13]
                            mov r8, qword [r14 + 136]
                            test r8, r8
                            js baz_bounds_line_450
                            lea r11, [rcx + r8]
                            cmp r11, 127
                            jg baz_bounds_line_450
                            lea rdi, [r14 + r8]
                            test rcx, rcx
                            repe cmpsb
                            sete r9b
                        bool.449.11.824.15.944.9.end:
                    func.tokenizer.is.name.824.15.944.9.end:
                cmp r9b, 0
                je if.824.9.944.9.end
                if.824.12.944.9.code:
                    mov qword [rbx + 16], r10
                    jmp foo.823.5.944.9.end
                if.824.9.944.9.end:
                foo.823.5.944.9.continue:
                    add r13, 2736
                    inc r10
                    cmp r10, r12
                    jne foo.823.5.944.9
            foo.823.5.944.9.end:
            if.830.8.944.9:
            cmp.830.8.944.9:
            mov r13, qword [rbp + 88480]
            cmp qword [rbx + 16], r13
            jne if.830.5.944.9.end
            if.830.8.944.9.code:
                func.tokenizer.print.831.12.944.9:
                    func.tokenizer.len.440.39.831.12.944.9:
                        mov r13, qword [r14 + 144]
                        sub r13, qword [r14 + 136]
                    func.tokenizer.len.440.39.831.12.944.9.end:
                    func.printer.print_at.440.9.831.12.944.9:
                        if.59.8.440.9.831.12.944.9:
                        cmp.59.8.440.9.831.12.944.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.440.9.831.12.944.9.end
                        if.59.8.440.9.831.12.944.9.code:
                        if.59.5.440.9.831.12.944.9.end:
                        mov rdi, 1
                        mov rdx, r13
                        mov r12, qword [r14 + 136]
                        test r12, r12
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r10, [rdx + r12]
                        cmp r10, 127
                        jg baz_bounds_line_61
                        lea rsi, [r14]
                        add rsi, r12
                        mov rax, 1
                        syscall
                    func.printer.print_at.440.9.831.12.944.9.end:
                func.tokenizer.print.831.12.944.9.end:
                func.printer.print_all.832.13.944.9:
                    func.printer.print.69.10.832.13.944.9:
                        func.printer.print_at.65.10.69.10.832.13.944.9:
                            if.59.8.65.10.69.10.832.13.944.9:
                            cmp.59.8.65.10.69.10.832.13.944.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.832.13.944.9.end
                            if.59.8.65.10.69.10.832.13.944.9.code:
                            if.59.5.65.10.69.10.832.13.944.9.end:
                            mov rdi, 1
                            mov rdx, 16
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 16
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260907]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.832.13.944.9.end:
                    func.printer.print.69.10.832.13.944.9.end:
                func.printer.print_all.832.13.944.9.end:
                jmp func.action_tell.944.9.end
            if.830.5.944.9.end:
            if.836.8.944.9:
            cmp.836.8.944.9:
            mov r13, qword [rbx + 16]
            cmp r13, 32
            jae baz_bounds_line_836
            imul r13, 2736
            cmp qword [rbp + r13 + 3656], 16
            jne if.836.5.944.9.end
            if.836.8.944.9.code:
                func.tokenizer.print.837.12.944.9:
                    func.tokenizer.len.440.39.837.12.944.9:
                        mov r13, qword [r14 + 144]
                        sub r13, qword [r14 + 136]
                    func.tokenizer.len.440.39.837.12.944.9.end:
                    func.printer.print_at.440.9.837.12.944.9:
                        if.59.8.440.9.837.12.944.9:
                        cmp.59.8.440.9.837.12.944.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.440.9.837.12.944.9.end
                        if.59.8.440.9.837.12.944.9.code:
                        if.59.5.440.9.837.12.944.9.end:
                        mov rdi, 1
                        mov rdx, r13
                        mov r12, qword [r14 + 136]
                        test r12, r12
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r10, [rdx + r12]
                        cmp r10, 127
                        jg baz_bounds_line_61
                        lea rsi, [r14]
                        add rsi, r12
                        mov rax, 1
                        syscall
                    func.printer.print_at.440.9.837.12.944.9.end:
                func.tokenizer.print.837.12.944.9.end:
                func.printer.print_all.838.13.944.9:
                    func.printer.print.69.10.838.13.944.9:
                        func.printer.print_at.65.10.69.10.838.13.944.9:
                            if.59.8.65.10.69.10.838.13.944.9:
                            cmp.59.8.65.10.69.10.838.13.944.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.838.13.944.9.end
                            if.59.8.65.10.69.10.838.13.944.9.code:
                            if.59.5.65.10.69.10.838.13.944.9.end:
                            mov rdi, 1
                            mov rdx, 23
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 23
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260923]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.838.13.944.9.end:
                    func.printer.print.69.10.838.13.944.9.end:
                func.printer.print_all.838.13.944.9.end:
                jmp func.action_tell.944.9.end
            if.836.5.944.9.end:
            func.tokenizer.skip_whitespace.843.8.944.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                loop.404.5.843.8.944.9:
                    if.405.12.843.8.944.9:
                    cmp.405.12.843.8.944.9:
                    mov r13, qword [r14 + 128]
                    cmp qword [r14 + 136], r13
                    jl if.405.9.843.8.944.9.end
                    if.405.12.843.8.944.9.code:
                        mov r13, qword [r14 + 136]
                        mov qword [r14 + 144], r13
                        jmp func.tokenizer.skip_whitespace.843.8.944.9.end
                    if.405.9.843.8.944.9.end:
                    if.409.12.843.8.944.9:
                    cmp.409.12.843.8.944.9:
                    mov r13, qword [r14 + 136]
                    cmp r13, 127
                    jae baz_bounds_line_409
                    cmp byte [r14 + r13], 32
                    jne loop.404.5.843.8.944.9.end
                    if.409.12.843.8.944.9.code:
                    if.409.9.843.8.944.9.end:
                    add qword [r14 + 136], 1
                jmp loop.404.5.843.8.944.9
                loop.404.5.843.8.944.9.end:
                mov r13, qword [r14 + 136]
                mov qword [r14 + 144], r13
            func.tokenizer.skip_whitespace.843.8.944.9.end:
            func.tokenizer.to_end.844.8.944.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                mov r13, qword [r14 + 128]
                mov qword [r14 + 144], r13
            func.tokenizer.to_end.844.8.944.9.end:
            if.846.8.944.9:
            cmp.846.8.944.9:
                func.tokenizer.is_empty.846.11.944.9:
                    cmp.454.11.846.11.944.9:
                    mov r12, qword [r14 + 144]
                    cmp qword [r14 + 136], r12
                    sete r13b
                    bool.454.11.846.11.944.9.end:
                func.tokenizer.is_empty.846.11.944.9.end:
            cmp r13b, 0
            je if.846.5.944.9.end
            if.846.8.944.9.code:
                func.printer.print_all.847.13.944.9:
                    func.printer.print.69.10.847.13.944.9:
                        func.printer.print_at.65.10.69.10.847.13.944.9:
                            if.59.8.65.10.69.10.847.13.944.9:
                            cmp.59.8.65.10.69.10.847.13.944.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.847.13.944.9.end
                            if.59.8.65.10.69.10.847.13.944.9.code:
                            if.59.5.65.10.69.10.847.13.944.9.end:
                            mov rdi, 1
                            mov rdx, 10
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 10
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260897]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.847.13.944.9.end:
                    func.printer.print.69.10.847.13.944.9.end:
                func.printer.print_all.847.13.944.9.end:
                jmp func.action_tell.944.9.end
            if.846.5.944.9.end:
            mov byte [rbx + 151], 0
            func.tokenizer.token.str.851.19.944.9:
                func.tokenizer.len.458.20.851.19.944.9:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                    mov qword [rbx + 152], r13
                func.tokenizer.len.458.20.851.19.944.9.end:
                xor al, al
                lea rdi, [rbx + 24]
                mov rcx, 127
                rep stosb
                mov rcx, qword [rbx + 152]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_460
                test rcx, rcx
                js baz_bounds_line_460
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_460
                lea rsi, [r14 + r13]
                cmp rcx, 127
                ja baz_bounds_line_460
                lea rdi, [rbx + 24]
                rep movsb
            func.tokenizer.token.str.851.19.944.9.end:
            mov r13, qword [rbx + 16]
            cmp r13, 32
            jae baz_bounds_line_853
            imul r13, 2736
            func.messages.add.853.40.944.9:
                func.messages.reserve.212.19.853.40.944.9:
                    mov r12, qword [rbp + r13 + 3656]
                    mov qword [rbx + 160], r12
                    add qword [rbp + r13 + 3656], 1
                func.messages.reserve.212.19.853.40.944.9.end:
                lea r12, [rbp + r13 + 1224]
                mov r10, qword [rbx + 160]
                cmp r10, 16
                jae baz_bounds_line_213
                imul r10, 152
                mov r9, qword [r15]
                mov qword [r12 + r10], r9
                mov qword [r12 + r10 + 8], 0
                lea rsi, [rbx + 24]
                lea rdi, [r12 + r10 + 16]
                mov rcx, 136
                rep movsb
            func.messages.add.853.40.944.9.end:
        func.action_tell.944.9.end:
    jmp if.913.5.end
    if.946.15:
    cmp.946.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.946.18:
            cmp.444.11.946.18:
                func.tokenizer.len.444.16.946.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.946.18.end:
            cmp r13, 1
            sete r15b
            jne bool.444.11.946.18.end
            cmp.445.11.946.18:
                mov rcx, 1
                cmp rcx, 1
                ja baz_bounds_line_445
                lea rsi, [rbp + 261325]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.946.18.end:
        func.tokenizer.is_array.946.18.end:
    cmp r15b, 0
    je if.949.15
    if.946.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_say.947.9:
            func.tokenizer.skip_whitespace.860.8.947.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                loop.404.5.860.8.947.9:
                    if.405.12.860.8.947.9:
                    cmp.405.12.860.8.947.9:
                    mov r13, qword [r14 + 128]
                    cmp qword [r14 + 136], r13
                    jl if.405.9.860.8.947.9.end
                    if.405.12.860.8.947.9.code:
                        mov r13, qword [r14 + 136]
                        mov qword [r14 + 144], r13
                        jmp func.tokenizer.skip_whitespace.860.8.947.9.end
                    if.405.9.860.8.947.9.end:
                    if.409.12.860.8.947.9:
                    cmp.409.12.860.8.947.9:
                    mov r13, qword [r14 + 136]
                    cmp r13, 127
                    jae baz_bounds_line_409
                    cmp byte [r14 + r13], 32
                    jne loop.404.5.860.8.947.9.end
                    if.409.12.860.8.947.9.code:
                    if.409.9.860.8.947.9.end:
                    add qword [r14 + 136], 1
                jmp loop.404.5.860.8.947.9
                loop.404.5.860.8.947.9.end:
                mov r13, qword [r14 + 136]
                mov qword [r14 + 144], r13
            func.tokenizer.skip_whitespace.860.8.947.9.end:
            func.tokenizer.to_end.861.8.947.9:
                mov r13, qword [r14 + 144]
                mov qword [r14 + 136], r13
                mov r13, qword [r14 + 128]
                mov qword [r14 + 144], r13
            func.tokenizer.to_end.861.8.947.9.end:
            if.863.8.947.9:
            cmp.863.8.947.9:
                func.tokenizer.is_empty.863.11.947.9:
                    cmp.454.11.863.11.947.9:
                    mov r12, qword [r14 + 144]
                    cmp qword [r14 + 136], r12
                    sete r13b
                    bool.454.11.863.11.947.9.end:
                func.tokenizer.is_empty.863.11.947.9.end:
            cmp r13b, 0
            je if.863.5.947.9.end
            if.863.8.947.9.code:
                func.printer.print_all.864.13.947.9:
                    func.printer.print.69.10.864.13.947.9:
                        func.printer.print_at.65.10.69.10.864.13.947.9:
                            if.59.8.65.10.69.10.864.13.947.9:
                            cmp.59.8.65.10.69.10.864.13.947.9:
                            cmp byte [rbp + 911], 0
                            jne func.printer.print_at.65.10.69.10.864.13.947.9.end
                            if.59.8.65.10.69.10.864.13.947.9.code:
                            if.59.5.65.10.69.10.864.13.947.9.end:
                            mov rdi, 1
                            mov rdx, 9
                            mov r13, 0
                            test r13, r13
                            js baz_bounds_line_61
                            test rdx, rdx
                            js baz_bounds_line_61
                            lea r12, [rdx + r13]
                            cmp r12, 9
                            jg baz_bounds_line_61
                            lea rsi, [rbp + 260946]
                            add rsi, r13
                            mov rax, 1
                            syscall
                        func.printer.print_at.65.10.69.10.864.13.947.9.end:
                    func.printer.print.69.10.864.13.947.9.end:
                func.printer.print_all.864.13.947.9.end:
                jmp func.action_say.947.9.end
            if.863.5.947.9.end:
            mov byte [rbx + 143], 0
            func.tokenizer.token.str.868.19.947.9:
                func.tokenizer.len.458.20.868.19.947.9:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                    mov qword [rbx + 144], r13
                func.tokenizer.len.458.20.868.19.947.9.end:
                xor al, al
                lea rdi, [rbx + 16]
                mov rcx, 127
                rep stosb
                mov rcx, qword [rbx + 144]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_460
                test rcx, rcx
                js baz_bounds_line_460
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_460
                lea rsi, [r14 + r13]
                cmp rcx, 127
                ja baz_bounds_line_460
                lea rdi, [rbx + 16]
                rep movsb
            func.tokenizer.token.str.868.19.947.9.end:
            mov r13, qword [r15]
            cmp r13, 32
            jae baz_bounds_line_870
            imul r13, 2736
            func.notify_room.870.5.947.9:
                mov r12, qword [rbp + r13 + 952]
                cmp r12, 128
                jae baz_bounds_line_482
                imul r12, 960
                mov r10, qword [rbp + r12 + 138216]
                mov qword [rbx + 152], r10
                mov r10, qword [rbp + r13 + 952]
                cmp r10, 128
                jae baz_bounds_line_483
                imul r10, 960
                lea r12, [rbp + r10 + 137960]
                mov r10, qword [rbx + 152]
                cmp r10, 32
                ja baz_bounds_line_483
                mov r9, 0
                cmp r10, 0
                jle foo.483.5.870.5.947.9.end
                foo.483.5.870.5.947.9:
                    if.484.12.870.5.947.9:
                    cmp.484.12.870.5.947.9:
                    mov r8, qword [r15]
                    cmp qword [r12], r8
                    je foo.483.5.870.5.947.9.continue
                    if.484.12.870.5.947.9.code:
                    if.484.9.870.5.947.9.end:
                    if.486.12.870.5.947.9:
                    cmp.486.12.870.5.947.9:
                    mov r8, qword [r12]
                    cmp r8, 32
                    jae baz_bounds_line_486
                    imul r8, 2736
                    cmp qword [rbp + r8 + 3656], 16
                    je foo.483.5.870.5.947.9.continue
                    if.486.12.870.5.947.9.code:
                    if.486.9.870.5.947.9.end:
                    mov r8, qword [r12]
                    cmp r8, 32
                    jae baz_bounds_line_488
                    imul r8, 2736
                    func.messages.add.488.36.870.5.947.9:
                        func.messages.reserve.212.19.488.36.870.5.947.9:
                            mov r11, qword [rbp + r8 + 3656]
                            mov qword [rbx + 168], r11
                            add qword [rbp + r8 + 3656], 1
                        func.messages.reserve.212.19.488.36.870.5.947.9.end:
                        lea r11, [rbp + r8 + 1224]
                        mov rdx, qword [rbx + 168]
                        cmp rdx, 16
                        jae baz_bounds_line_213
                        imul rdx, 152
                        mov rax, qword [r15]
                        mov qword [r11 + rdx], rax
                        mov qword [r11 + rdx + 8], 1
                        lea rsi, [rbx + 16]
                        lea rdi, [r11 + rdx + 16]
                        mov rcx, 136
                        rep movsb
                    func.messages.add.488.36.870.5.947.9.end:
                    foo.483.5.870.5.947.9.continue:
                        add r12, 8
                        inc r9
                        cmp r9, r10
                        jne foo.483.5.870.5.947.9
                foo.483.5.870.5.947.9.end:
            func.notify_room.870.5.947.9.end:
        func.action_say.947.9.end:
    jmp if.913.5.end
    if.949.15:
    cmp.949.15:
        mov r14, qword [rbx + 8]
        func.tokenizer.is_array.949.18:
            cmp.444.11.949.18:
                func.tokenizer.len.444.16.949.18:
                    mov r13, qword [r14 + 144]
                    sub r13, qword [r14 + 136]
                func.tokenizer.len.444.16.949.18.end:
            cmp r13, 4
            sete r15b
            jne bool.444.11.949.18.end
            cmp.445.11.949.18:
                mov rcx, 4
                cmp rcx, 4
                ja baz_bounds_line_445
                lea rsi, [rbp + 261313]
                mov r13, qword [r14 + 136]
                test r13, r13
                js baz_bounds_line_445
                lea r12, [rcx + r13]
                cmp r12, 127
                jg baz_bounds_line_445
                lea rdi, [r14 + r13]
                test rcx, rcx
                repe cmpsb
                sete r15b
            bool.444.11.949.18.end:
        func.tokenizer.is_array.949.18.end:
    cmp r15b, 0
    je if.913.5.else
    if.949.15.code:
        mov r15, qword [rbx]
        mov r14, qword [rbx + 8]
        func.action_help.950.9:
            func.printer.print_all.891.9.950.9:
                func.printer.print.69.10.891.9.950.9:
                    func.printer.print_at.65.10.69.10.891.9.950.9:
                        if.59.8.65.10.69.10.891.9.950.9:
                        cmp.59.8.65.10.69.10.891.9.950.9:
                        cmp byte [rbp + 911], 0
                        jne func.printer.print_at.65.10.69.10.891.9.950.9.end
                        if.59.8.65.10.69.10.891.9.950.9.code:
                        if.59.5.65.10.69.10.891.9.950.9.end:
                        mov rdi, 1
                        mov rdx, 358
                        mov r13, 0
                        test r13, r13
                        js baz_bounds_line_61
                        test rdx, rdx
                        js baz_bounds_line_61
                        lea r12, [rdx + r13]
                        cmp r12, 358
                        jg baz_bounds_line_61
                        lea rsi, [rbp + 260955]
                        add rsi, r13
                        mov rax, 1
                        syscall
                    func.printer.print_at.65.10.69.10.891.9.950.9.end:
                func.printer.print.69.10.891.9.950.9.end:
            func.printer.print_all.891.9.950.9.end:
        func.action_help.950.9.end:
    jmp if.913.5.end
    if.913.5.else:
        func.printer.print_all.953.13:
            func.printer.print.69.10.953.13:
                func.printer.print_at.65.10.69.10.953.13:
                    if.59.8.65.10.69.10.953.13:
                    cmp.59.8.65.10.69.10.953.13:
                    cmp byte [rbp + 911], 0
                    jne func.printer.print_at.65.10.69.10.953.13.end
                    if.59.8.65.10.69.10.953.13.code:
                    if.59.5.65.10.69.10.953.13.end:
                    mov rdi, 1
                    mov rdx, 15
                    mov r15, 0
                    test r15, r15
                    js baz_bounds_line_61
                    test rdx, rdx
                    js baz_bounds_line_61
                    lea r14, [rdx + r15]
                    cmp r14, 15
                    jg baz_bounds_line_61
                    lea rsi, [rbp + 888]
                    add rsi, r15
                    mov rax, 1
                    syscall
                func.printer.print_at.65.10.69.10.953.13.end:
            func.printer.print.69.10.953.13.end:
        func.printer.print_all.953.13.end:
    if.913.5.end:
    ret
size.func.parse_input equ 2760
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
baz_bounds_line_61:
    mov rbp, 61
    jmp baz_bounds_panic
baz_bounds_line_93:
    mov rbp, 93
    jmp baz_bounds_panic
baz_bounds_line_99:
    mov rbp, 99
    jmp baz_bounds_panic
baz_bounds_line_108:
    mov rbp, 108
    jmp baz_bounds_panic
baz_bounds_line_109:
    mov rbp, 109
    jmp baz_bounds_panic
baz_bounds_line_117:
    mov rbp, 117
    jmp baz_bounds_panic
baz_bounds_line_158:
    mov rbp, 158
    jmp baz_bounds_panic
baz_bounds_line_164:
    mov rbp, 164
    jmp baz_bounds_panic
baz_bounds_line_165:
    mov rbp, 165
    jmp baz_bounds_panic
baz_bounds_line_176:
    mov rbp, 176
    jmp baz_bounds_panic
baz_bounds_line_213:
    mov rbp, 213
    jmp baz_bounds_panic
baz_bounds_line_238:
    mov rbp, 238
    jmp baz_bounds_panic
baz_bounds_line_252:
    mov rbp, 252
    jmp baz_bounds_panic
baz_bounds_line_262:
    mov rbp, 262
    jmp baz_bounds_panic
baz_bounds_line_282:
    mov rbp, 282
    jmp baz_bounds_panic
baz_bounds_line_310:
    mov rbp, 310
    jmp baz_bounds_panic
baz_bounds_line_317:
    mov rbp, 317
    jmp baz_bounds_panic
baz_bounds_line_320:
    mov rbp, 320
    jmp baz_bounds_panic
baz_bounds_line_326:
    mov rbp, 326
    jmp baz_bounds_panic
baz_bounds_line_334:
    mov rbp, 334
    jmp baz_bounds_panic
baz_bounds_line_336:
    mov rbp, 336
    jmp baz_bounds_panic
baz_bounds_line_398:
    mov rbp, 398
    jmp baz_bounds_panic
baz_bounds_line_409:
    mov rbp, 409
    jmp baz_bounds_panic
baz_bounds_line_420:
    mov rbp, 420
    jmp baz_bounds_panic
baz_bounds_line_430:
    mov rbp, 430
    jmp baz_bounds_panic
baz_bounds_line_445:
    mov rbp, 445
    jmp baz_bounds_panic
baz_bounds_line_450:
    mov rbp, 450
    jmp baz_bounds_panic
baz_bounds_line_460:
    mov rbp, 460
    jmp baz_bounds_panic
baz_bounds_line_482:
    mov rbp, 482
    jmp baz_bounds_panic
baz_bounds_line_483:
    mov rbp, 483
    jmp baz_bounds_panic
baz_bounds_line_486:
    mov rbp, 486
    jmp baz_bounds_panic
baz_bounds_line_488:
    mov rbp, 488
    jmp baz_bounds_panic
baz_bounds_line_504:
    mov rbp, 504
    jmp baz_bounds_panic
baz_bounds_line_506:
    mov rbp, 506
    jmp baz_bounds_panic
baz_bounds_line_508:
    mov rbp, 508
    jmp baz_bounds_panic
baz_bounds_line_509:
    mov rbp, 509
    jmp baz_bounds_panic
baz_bounds_line_521:
    mov rbp, 521
    jmp baz_bounds_panic
baz_bounds_line_522:
    mov rbp, 522
    jmp baz_bounds_panic
baz_bounds_line_524:
    mov rbp, 524
    jmp baz_bounds_panic
baz_bounds_line_525:
    mov rbp, 525
    jmp baz_bounds_panic
baz_bounds_line_536:
    mov rbp, 536
    jmp baz_bounds_panic
baz_bounds_line_542:
    mov rbp, 542
    jmp baz_bounds_panic
baz_bounds_line_544:
    mov rbp, 544
    jmp baz_bounds_panic
baz_bounds_line_558:
    mov rbp, 558
    jmp baz_bounds_panic
baz_bounds_line_567:
    mov rbp, 567
    jmp baz_bounds_panic
baz_bounds_line_577:
    mov rbp, 577
    jmp baz_bounds_panic
baz_bounds_line_587:
    mov rbp, 587
    jmp baz_bounds_panic
baz_bounds_line_589:
    mov rbp, 589
    jmp baz_bounds_panic
baz_bounds_line_600:
    mov rbp, 600
    jmp baz_bounds_panic
baz_bounds_line_604:
    mov rbp, 604
    jmp baz_bounds_panic
baz_bounds_line_607:
    mov rbp, 607
    jmp baz_bounds_panic
baz_bounds_line_611:
    mov rbp, 611
    jmp baz_bounds_panic
baz_bounds_line_621:
    mov rbp, 621
    jmp baz_bounds_panic
baz_bounds_line_629:
    mov rbp, 629
    jmp baz_bounds_panic
baz_bounds_line_652:
    mov rbp, 652
    jmp baz_bounds_panic
baz_bounds_line_664:
    mov rbp, 664
    jmp baz_bounds_panic
baz_bounds_line_668:
    mov rbp, 668
    jmp baz_bounds_panic
baz_bounds_line_682:
    mov rbp, 682
    jmp baz_bounds_panic
baz_bounds_line_691:
    mov rbp, 691
    jmp baz_bounds_panic
baz_bounds_line_699:
    mov rbp, 699
    jmp baz_bounds_panic
baz_bounds_line_708:
    mov rbp, 708
    jmp baz_bounds_panic
baz_bounds_line_722:
    mov rbp, 722
    jmp baz_bounds_panic
baz_bounds_line_724:
    mov rbp, 724
    jmp baz_bounds_panic
baz_bounds_line_725:
    mov rbp, 725
    jmp baz_bounds_panic
baz_bounds_line_737:
    mov rbp, 737
    jmp baz_bounds_panic
baz_bounds_line_738:
    mov rbp, 738
    jmp baz_bounds_panic
baz_bounds_line_739:
    mov rbp, 739
    jmp baz_bounds_panic
baz_bounds_line_740:
    mov rbp, 740
    jmp baz_bounds_panic
baz_bounds_line_757:
    mov rbp, 757
    jmp baz_bounds_panic
baz_bounds_line_759:
    mov rbp, 759
    jmp baz_bounds_panic
baz_bounds_line_760:
    mov rbp, 760
    jmp baz_bounds_panic
baz_bounds_line_771:
    mov rbp, 771
    jmp baz_bounds_panic
baz_bounds_line_772:
    mov rbp, 772
    jmp baz_bounds_panic
baz_bounds_line_777:
    mov rbp, 777
    jmp baz_bounds_panic
baz_bounds_line_778:
    mov rbp, 778
    jmp baz_bounds_panic
baz_bounds_line_780:
    mov rbp, 780
    jmp baz_bounds_panic
baz_bounds_line_782:
    mov rbp, 782
    jmp baz_bounds_panic
baz_bounds_line_795:
    mov rbp, 795
    jmp baz_bounds_panic
baz_bounds_line_796:
    mov rbp, 796
    jmp baz_bounds_panic
baz_bounds_line_801:
    mov rbp, 801
    jmp baz_bounds_panic
baz_bounds_line_806:
    mov rbp, 806
    jmp baz_bounds_panic
baz_bounds_line_823:
    mov rbp, 823
    jmp baz_bounds_panic
baz_bounds_line_836:
    mov rbp, 836
    jmp baz_bounds_panic
baz_bounds_line_853:
    mov rbp, 853
    jmp baz_bounds_panic
baz_bounds_line_870:
    mov rbp, 870
    jmp baz_bounds_panic
baz_bounds_line_987:
    mov rbp, 987
    jmp baz_bounds_panic
baz_bounds_line_999:
    mov rbp, 999
    jmp baz_bounds_panic
baz_bounds_line_1001:
    mov rbp, 1001
    jmp baz_bounds_panic
baz_bounds_line_1002:
    mov rbp, 1002
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
db `                                   oOo.o.\n          frameless osca          oOo.oOo\n       __________________________  .oOo.\n      O\\        -_   .. \\    ___ \\   ||\n     O  \\                \\   \\ \\\\ \\ //\\\\\n    o   /\\     rv32i      \\   \\|\\\\ \\\n   .   //\\\\      fpga      \\   ||   \\\n    .  \\\\/\\\\       baz      \\  \\_\\   \\\n     .  \\\\//\\________________\\________\\\n      .  \\/_/, \\\\\\--\\\\..\\\\ - /\\_____  /\n       .  \\ \\ . \\\\\\__\\\\__\\\\./ / \\__/ /\n        .  \\ \\ , \\    \\\\ ///./ ,/./ /\n         .  \\ \\___\\ sticky notes / /\n          .  \\/\\________________/ /\n     ./\\.  . / /                 /\n     /--\\   .\\/_________________/\n          ___.                 .\n         |o o|. . . . . . . . .\n         /| |\\ . .\n     ____       . .\n    |O  O|       . .\n    |_ -_|        . .\n     /||\\\n       ___\n      /- -\\\n     /\\_-_/\\\n       | |\n\n`
db `\nwelcome to adventure #7\n    type 'help'\n`
db `u c `
db `exits: `
db ` > `
db `\n`
db `--\n`
db `, `
db `\b \b`
db `not understood\n`
db `\n\nFLAG\n\n`
times 1 db 0
db ` told u `
db ` said `
db ` `
times 1 db 0
db `u`
times 15 db 0
dq 1
dq 0
times 2704 db 0
times 84816 db 0
dq 1
times 24584 db 0
times 24584 db 0
db `u r in `
times 1 db 0
times 24 db 0
times 136 db 0
times 136 db 0
dq 0
times 31 dq 0
dq 1
times 400 db 0
times 121920 db 0
dq 1
db `went to `
db `go where\n`
db `cannot go there\n`
db `home`
db `arrived from `
db `what is exit name to new room\n`
db `what is exit name back to this room\n`
db `new room created\n`
db `new room description\n`
db `what name\n`
db `name too long\n`
db `new room name\n`
db `new entity created\n`
db `new object created\n`
db `u have: `
db `u have nothing\n`
db `drop what\n`
db `u do not have that\n`
db `dropped `
db `give what\n`
db `give to whom\n`
db ` is not here\n`
db `gave `
db ` to `
db `tell whom\n`
db `tell what\n`
db ` does not exist\n`
db ` has too many messages\n`
db `say what\n`
db `  help: this message\n  go to: go <exit>\n  inventory: i\n  create new object: on <name>\n  drop object: d <object>\n  give object: g <object> <entity>\n  tell entity: t <entity> <message>\n  say to room: s <text>\n  new room: rn <to exit> <from exit>\n  set room name: rnm <text>\n  set room description: rd <text>\n  set room note: rnt <text>\n  new entity: en <name>\n`
db `help`
db `go`
db `i`
db `on`
db `d`
db `g`
db `t`
db `s`
db `rn`
db `rnm`
db `rd`
db `rnt`
db `en`
db `rnm roome\nrnt todo: find an exit\nrn none roome\ngo none\nrnm office\ngo roome\nen me\n`
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
resb 131072
vars.end:
