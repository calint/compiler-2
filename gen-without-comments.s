DEFAULT REL
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 1]
    mov rcx, 1
    xor rax, rax
    rep stosb
    mov r15, 0
    cmp r15, 1
    mov r14, 64
    cmovge rbp, r14
    jge panic_bounds
    mov byte [rsp + r15 - 1], 10
    mov qword [rsp - 9], 123
    mov dword [rsp - 13], 321
    mov word [rsp - 15], 420
    mov qword [rsp - 23], 840
    mov qword [rsp - 31], -1
    mov qword [rsp - 39], 2147483647
    mov qword [rsp - 47], -2147483648
    print_num_74_3:
        lea rdi, [rsp - 67]
        mov rcx, 20
        xor rax, rax
        rep stosb
        mov r15, qword [rsp - 9]
        mov qword [rsp - 75], r15
        mov byte [rsp - 76], false
        if_33_8_74_3:
        cmp_33_8_74_3:
            mov r15, qword [rsp - 75]
            mov r14, 0
        cmp r15, r14
        jge if_33_5_74_3_end
        jmp if_33_8_74_3_code
        if_33_8_74_3_code:
            mov byte [rsp - 76], true
            neg qword [rsp - 75]
        if_33_5_74_3_end:
        mov qword [rsp - 84], 20
        loop_39_5_74_3:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 41
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 48
            mov r14, qword [rsp - 75]
            mov rax, r14
            cqo
            mov r13, 10
            idiv r13
            mov r14, rdx
            add byte [rsp + r15 - 67], r14b
            mov rax, qword [rsp - 75]
            cqo
            mov r15, 10
            idiv r15
            mov qword [rsp - 75], rax
            if_43_12_74_3:
            cmp_43_12_74_3:
                mov r15, qword [rsp - 75]
                mov r14, 0
            cmp r15, r14
            jne if_43_9_74_3_end
            jmp if_43_12_74_3_code
            if_43_12_74_3_code:
                jmp loop_39_5_74_3_end
            if_43_9_74_3_end:
        jmp loop_39_5_74_3
        loop_39_5_74_3_end:
        if_46_8_74_3:
        cmp_46_8_74_3:
            movsx r15, byte [rsp - 76]
        test r15, r15
        je if_46_5_74_3_end
        jmp if_46_8_74_3_code
        if_46_8_74_3_code:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 48
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 45
        if_46_5_74_3_end:
        mov qword [rsp - 92], 0
        loop_52_5_74_3:
            mov r15, qword [rsp - 92]
            cmp r15, 20
            mov r14, 53
            cmovge rbp, r14
            jge panic_bounds
            mov r14, qword [rsp - 84]
            cmp r14, 20
            mov r13, 53
            cmovge rbp, r13
            jge panic_bounds
            mov r13b, byte [rsp + r14 - 67]
            mov byte [rsp + r15 - 67], r13b
            add qword [rsp - 92], 1
            add qword [rsp - 84], 1
            if_56_12_74_3:
            cmp_56_12_74_3:
                mov r15, qword [rsp - 84]
                mov r14, 20
            cmp r15, r14
            jne if_56_9_74_3_end
            jmp if_56_12_74_3_code
            if_56_12_74_3_code:
                jmp loop_52_5_74_3_end
            if_56_9_74_3_end:
        jmp loop_52_5_74_3
        loop_52_5_74_3_end:
        mov rdx, qword [rsp - 92]
        lea rsi, [rsp - 67]
        print_59_5_74_3:
            mov rax, 1
            mov rdi, 1
            syscall
        print_59_5_74_3_end:
    print_num_74_3_end:
    mov rdx, 1
    lea rsi, [rsp - 1]
    print_75_3:
        mov rax, 1
        mov rdi, 1
        syscall
    print_75_3_end:
    print_num_76_3:
        lea rdi, [rsp - 67]
        mov rcx, 20
        xor rax, rax
        rep stosb
        movsx r15, dword [rsp - 13]
        mov qword [rsp - 75], r15
        mov byte [rsp - 76], false
        if_33_8_76_3:
        cmp_33_8_76_3:
            mov r15, qword [rsp - 75]
            mov r14, 0
        cmp r15, r14
        jge if_33_5_76_3_end
        jmp if_33_8_76_3_code
        if_33_8_76_3_code:
            mov byte [rsp - 76], true
            neg qword [rsp - 75]
        if_33_5_76_3_end:
        mov qword [rsp - 84], 20
        loop_39_5_76_3:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 41
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 48
            mov r14, qword [rsp - 75]
            mov rax, r14
            cqo
            mov r13, 10
            idiv r13
            mov r14, rdx
            add byte [rsp + r15 - 67], r14b
            mov rax, qword [rsp - 75]
            cqo
            mov r15, 10
            idiv r15
            mov qword [rsp - 75], rax
            if_43_12_76_3:
            cmp_43_12_76_3:
                mov r15, qword [rsp - 75]
                mov r14, 0
            cmp r15, r14
            jne if_43_9_76_3_end
            jmp if_43_12_76_3_code
            if_43_12_76_3_code:
                jmp loop_39_5_76_3_end
            if_43_9_76_3_end:
        jmp loop_39_5_76_3
        loop_39_5_76_3_end:
        if_46_8_76_3:
        cmp_46_8_76_3:
            movsx r15, byte [rsp - 76]
        test r15, r15
        je if_46_5_76_3_end
        jmp if_46_8_76_3_code
        if_46_8_76_3_code:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 48
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 45
        if_46_5_76_3_end:
        mov qword [rsp - 92], 0
        loop_52_5_76_3:
            mov r15, qword [rsp - 92]
            cmp r15, 20
            mov r14, 53
            cmovge rbp, r14
            jge panic_bounds
            mov r14, qword [rsp - 84]
            cmp r14, 20
            mov r13, 53
            cmovge rbp, r13
            jge panic_bounds
            mov r13b, byte [rsp + r14 - 67]
            mov byte [rsp + r15 - 67], r13b
            add qword [rsp - 92], 1
            add qword [rsp - 84], 1
            if_56_12_76_3:
            cmp_56_12_76_3:
                mov r15, qword [rsp - 84]
                mov r14, 20
            cmp r15, r14
            jne if_56_9_76_3_end
            jmp if_56_12_76_3_code
            if_56_12_76_3_code:
                jmp loop_52_5_76_3_end
            if_56_9_76_3_end:
        jmp loop_52_5_76_3
        loop_52_5_76_3_end:
        mov rdx, qword [rsp - 92]
        lea rsi, [rsp - 67]
        print_59_5_76_3:
            mov rax, 1
            mov rdi, 1
            syscall
        print_59_5_76_3_end:
    print_num_76_3_end:
    mov rdx, 1
    lea rsi, [rsp - 1]
    print_77_3:
        mov rax, 1
        mov rdi, 1
        syscall
    print_77_3_end:
    print_num_78_3:
        lea rdi, [rsp - 67]
        mov rcx, 20
        xor rax, rax
        rep stosb
        movsx r15, word [rsp - 15]
        mov qword [rsp - 75], r15
        mov byte [rsp - 76], false
        if_33_8_78_3:
        cmp_33_8_78_3:
            mov r15, qword [rsp - 75]
            mov r14, 0
        cmp r15, r14
        jge if_33_5_78_3_end
        jmp if_33_8_78_3_code
        if_33_8_78_3_code:
            mov byte [rsp - 76], true
            neg qword [rsp - 75]
        if_33_5_78_3_end:
        mov qword [rsp - 84], 20
        loop_39_5_78_3:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 41
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 48
            mov r14, qword [rsp - 75]
            mov rax, r14
            cqo
            mov r13, 10
            idiv r13
            mov r14, rdx
            add byte [rsp + r15 - 67], r14b
            mov rax, qword [rsp - 75]
            cqo
            mov r15, 10
            idiv r15
            mov qword [rsp - 75], rax
            if_43_12_78_3:
            cmp_43_12_78_3:
                mov r15, qword [rsp - 75]
                mov r14, 0
            cmp r15, r14
            jne if_43_9_78_3_end
            jmp if_43_12_78_3_code
            if_43_12_78_3_code:
                jmp loop_39_5_78_3_end
            if_43_9_78_3_end:
        jmp loop_39_5_78_3
        loop_39_5_78_3_end:
        if_46_8_78_3:
        cmp_46_8_78_3:
            movsx r15, byte [rsp - 76]
        test r15, r15
        je if_46_5_78_3_end
        jmp if_46_8_78_3_code
        if_46_8_78_3_code:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 48
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 45
        if_46_5_78_3_end:
        mov qword [rsp - 92], 0
        loop_52_5_78_3:
            mov r15, qword [rsp - 92]
            cmp r15, 20
            mov r14, 53
            cmovge rbp, r14
            jge panic_bounds
            mov r14, qword [rsp - 84]
            cmp r14, 20
            mov r13, 53
            cmovge rbp, r13
            jge panic_bounds
            mov r13b, byte [rsp + r14 - 67]
            mov byte [rsp + r15 - 67], r13b
            add qword [rsp - 92], 1
            add qword [rsp - 84], 1
            if_56_12_78_3:
            cmp_56_12_78_3:
                mov r15, qword [rsp - 84]
                mov r14, 20
            cmp r15, r14
            jne if_56_9_78_3_end
            jmp if_56_12_78_3_code
            if_56_12_78_3_code:
                jmp loop_52_5_78_3_end
            if_56_9_78_3_end:
        jmp loop_52_5_78_3
        loop_52_5_78_3_end:
        mov rdx, qword [rsp - 92]
        lea rsi, [rsp - 67]
        print_59_5_78_3:
            mov rax, 1
            mov rdi, 1
            syscall
        print_59_5_78_3_end:
    print_num_78_3_end:
    mov rdx, 1
    lea rsi, [rsp - 1]
    print_79_3:
        mov rax, 1
        mov rdi, 1
        syscall
    print_79_3_end:
    print_num_80_3:
        lea rdi, [rsp - 67]
        mov rcx, 20
        xor rax, rax
        rep stosb
        mov r15, qword [rsp - 23]
        mov qword [rsp - 75], r15
        mov byte [rsp - 76], false
        if_33_8_80_3:
        cmp_33_8_80_3:
            mov r15, qword [rsp - 75]
            mov r14, 0
        cmp r15, r14
        jge if_33_5_80_3_end
        jmp if_33_8_80_3_code
        if_33_8_80_3_code:
            mov byte [rsp - 76], true
            neg qword [rsp - 75]
        if_33_5_80_3_end:
        mov qword [rsp - 84], 20
        loop_39_5_80_3:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 41
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 48
            mov r14, qword [rsp - 75]
            mov rax, r14
            cqo
            mov r13, 10
            idiv r13
            mov r14, rdx
            add byte [rsp + r15 - 67], r14b
            mov rax, qword [rsp - 75]
            cqo
            mov r15, 10
            idiv r15
            mov qword [rsp - 75], rax
            if_43_12_80_3:
            cmp_43_12_80_3:
                mov r15, qword [rsp - 75]
                mov r14, 0
            cmp r15, r14
            jne if_43_9_80_3_end
            jmp if_43_12_80_3_code
            if_43_12_80_3_code:
                jmp loop_39_5_80_3_end
            if_43_9_80_3_end:
        jmp loop_39_5_80_3
        loop_39_5_80_3_end:
        if_46_8_80_3:
        cmp_46_8_80_3:
            movsx r15, byte [rsp - 76]
        test r15, r15
        je if_46_5_80_3_end
        jmp if_46_8_80_3_code
        if_46_8_80_3_code:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 48
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 45
        if_46_5_80_3_end:
        mov qword [rsp - 92], 0
        loop_52_5_80_3:
            mov r15, qword [rsp - 92]
            cmp r15, 20
            mov r14, 53
            cmovge rbp, r14
            jge panic_bounds
            mov r14, qword [rsp - 84]
            cmp r14, 20
            mov r13, 53
            cmovge rbp, r13
            jge panic_bounds
            mov r13b, byte [rsp + r14 - 67]
            mov byte [rsp + r15 - 67], r13b
            add qword [rsp - 92], 1
            add qword [rsp - 84], 1
            if_56_12_80_3:
            cmp_56_12_80_3:
                mov r15, qword [rsp - 84]
                mov r14, 20
            cmp r15, r14
            jne if_56_9_80_3_end
            jmp if_56_12_80_3_code
            if_56_12_80_3_code:
                jmp loop_52_5_80_3_end
            if_56_9_80_3_end:
        jmp loop_52_5_80_3
        loop_52_5_80_3_end:
        mov rdx, qword [rsp - 92]
        lea rsi, [rsp - 67]
        print_59_5_80_3:
            mov rax, 1
            mov rdi, 1
            syscall
        print_59_5_80_3_end:
    print_num_80_3_end:
    mov rdx, 1
    lea rsi, [rsp - 1]
    print_81_3:
        mov rax, 1
        mov rdi, 1
        syscall
    print_81_3_end:
    print_num_82_3:
        lea rdi, [rsp - 67]
        mov rcx, 20
        xor rax, rax
        rep stosb
        mov r15, qword [rsp - 31]
        mov qword [rsp - 75], r15
        mov byte [rsp - 76], false
        if_33_8_82_3:
        cmp_33_8_82_3:
            mov r15, qword [rsp - 75]
            mov r14, 0
        cmp r15, r14
        jge if_33_5_82_3_end
        jmp if_33_8_82_3_code
        if_33_8_82_3_code:
            mov byte [rsp - 76], true
            neg qword [rsp - 75]
        if_33_5_82_3_end:
        mov qword [rsp - 84], 20
        loop_39_5_82_3:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 41
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 48
            mov r14, qword [rsp - 75]
            mov rax, r14
            cqo
            mov r13, 10
            idiv r13
            mov r14, rdx
            add byte [rsp + r15 - 67], r14b
            mov rax, qword [rsp - 75]
            cqo
            mov r15, 10
            idiv r15
            mov qword [rsp - 75], rax
            if_43_12_82_3:
            cmp_43_12_82_3:
                mov r15, qword [rsp - 75]
                mov r14, 0
            cmp r15, r14
            jne if_43_9_82_3_end
            jmp if_43_12_82_3_code
            if_43_12_82_3_code:
                jmp loop_39_5_82_3_end
            if_43_9_82_3_end:
        jmp loop_39_5_82_3
        loop_39_5_82_3_end:
        if_46_8_82_3:
        cmp_46_8_82_3:
            movsx r15, byte [rsp - 76]
        test r15, r15
        je if_46_5_82_3_end
        jmp if_46_8_82_3_code
        if_46_8_82_3_code:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 48
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 45
        if_46_5_82_3_end:
        mov qword [rsp - 92], 0
        loop_52_5_82_3:
            mov r15, qword [rsp - 92]
            cmp r15, 20
            mov r14, 53
            cmovge rbp, r14
            jge panic_bounds
            mov r14, qword [rsp - 84]
            cmp r14, 20
            mov r13, 53
            cmovge rbp, r13
            jge panic_bounds
            mov r13b, byte [rsp + r14 - 67]
            mov byte [rsp + r15 - 67], r13b
            add qword [rsp - 92], 1
            add qword [rsp - 84], 1
            if_56_12_82_3:
            cmp_56_12_82_3:
                mov r15, qword [rsp - 84]
                mov r14, 20
            cmp r15, r14
            jne if_56_9_82_3_end
            jmp if_56_12_82_3_code
            if_56_12_82_3_code:
                jmp loop_52_5_82_3_end
            if_56_9_82_3_end:
        jmp loop_52_5_82_3
        loop_52_5_82_3_end:
        mov rdx, qword [rsp - 92]
        lea rsi, [rsp - 67]
        print_59_5_82_3:
            mov rax, 1
            mov rdi, 1
            syscall
        print_59_5_82_3_end:
    print_num_82_3_end:
    mov rdx, 1
    lea rsi, [rsp - 1]
    print_83_3:
        mov rax, 1
        mov rdi, 1
        syscall
    print_83_3_end:
    print_num_84_3:
        lea rdi, [rsp - 67]
        mov rcx, 20
        xor rax, rax
        rep stosb
        mov r15, qword [rsp - 39]
        mov qword [rsp - 75], r15
        mov byte [rsp - 76], false
        if_33_8_84_3:
        cmp_33_8_84_3:
            mov r15, qword [rsp - 75]
            mov r14, 0
        cmp r15, r14
        jge if_33_5_84_3_end
        jmp if_33_8_84_3_code
        if_33_8_84_3_code:
            mov byte [rsp - 76], true
            neg qword [rsp - 75]
        if_33_5_84_3_end:
        mov qword [rsp - 84], 20
        loop_39_5_84_3:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 41
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 48
            mov r14, qword [rsp - 75]
            mov rax, r14
            cqo
            mov r13, 10
            idiv r13
            mov r14, rdx
            add byte [rsp + r15 - 67], r14b
            mov rax, qword [rsp - 75]
            cqo
            mov r15, 10
            idiv r15
            mov qword [rsp - 75], rax
            if_43_12_84_3:
            cmp_43_12_84_3:
                mov r15, qword [rsp - 75]
                mov r14, 0
            cmp r15, r14
            jne if_43_9_84_3_end
            jmp if_43_12_84_3_code
            if_43_12_84_3_code:
                jmp loop_39_5_84_3_end
            if_43_9_84_3_end:
        jmp loop_39_5_84_3
        loop_39_5_84_3_end:
        if_46_8_84_3:
        cmp_46_8_84_3:
            movsx r15, byte [rsp - 76]
        test r15, r15
        je if_46_5_84_3_end
        jmp if_46_8_84_3_code
        if_46_8_84_3_code:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 48
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 45
        if_46_5_84_3_end:
        mov qword [rsp - 92], 0
        loop_52_5_84_3:
            mov r15, qword [rsp - 92]
            cmp r15, 20
            mov r14, 53
            cmovge rbp, r14
            jge panic_bounds
            mov r14, qword [rsp - 84]
            cmp r14, 20
            mov r13, 53
            cmovge rbp, r13
            jge panic_bounds
            mov r13b, byte [rsp + r14 - 67]
            mov byte [rsp + r15 - 67], r13b
            add qword [rsp - 92], 1
            add qword [rsp - 84], 1
            if_56_12_84_3:
            cmp_56_12_84_3:
                mov r15, qword [rsp - 84]
                mov r14, 20
            cmp r15, r14
            jne if_56_9_84_3_end
            jmp if_56_12_84_3_code
            if_56_12_84_3_code:
                jmp loop_52_5_84_3_end
            if_56_9_84_3_end:
        jmp loop_52_5_84_3
        loop_52_5_84_3_end:
        mov rdx, qword [rsp - 92]
        lea rsi, [rsp - 67]
        print_59_5_84_3:
            mov rax, 1
            mov rdi, 1
            syscall
        print_59_5_84_3_end:
    print_num_84_3_end:
    mov rdx, 1
    lea rsi, [rsp - 1]
    print_85_3:
        mov rax, 1
        mov rdi, 1
        syscall
    print_85_3_end:
    print_num_86_3:
        lea rdi, [rsp - 67]
        mov rcx, 20
        xor rax, rax
        rep stosb
        mov r15, qword [rsp - 47]
        mov qword [rsp - 75], r15
        mov byte [rsp - 76], false
        if_33_8_86_3:
        cmp_33_8_86_3:
            mov r15, qword [rsp - 75]
            mov r14, 0
        cmp r15, r14
        jge if_33_5_86_3_end
        jmp if_33_8_86_3_code
        if_33_8_86_3_code:
            mov byte [rsp - 76], true
            neg qword [rsp - 75]
        if_33_5_86_3_end:
        mov qword [rsp - 84], 20
        loop_39_5_86_3:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 41
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 48
            mov r14, qword [rsp - 75]
            mov rax, r14
            cqo
            mov r13, 10
            idiv r13
            mov r14, rdx
            add byte [rsp + r15 - 67], r14b
            mov rax, qword [rsp - 75]
            cqo
            mov r15, 10
            idiv r15
            mov qword [rsp - 75], rax
            if_43_12_86_3:
            cmp_43_12_86_3:
                mov r15, qword [rsp - 75]
                mov r14, 0
            cmp r15, r14
            jne if_43_9_86_3_end
            jmp if_43_12_86_3_code
            if_43_12_86_3_code:
                jmp loop_39_5_86_3_end
            if_43_9_86_3_end:
        jmp loop_39_5_86_3
        loop_39_5_86_3_end:
        if_46_8_86_3:
        cmp_46_8_86_3:
            movsx r15, byte [rsp - 76]
        test r15, r15
        je if_46_5_86_3_end
        jmp if_46_8_86_3_code
        if_46_8_86_3_code:
            sub qword [rsp - 84], 1
            mov r15, qword [rsp - 84]
            cmp r15, 20
            mov r14, 48
            cmovge rbp, r14
            jge panic_bounds
            mov byte [rsp + r15 - 67], 45
        if_46_5_86_3_end:
        mov qword [rsp - 92], 0
        loop_52_5_86_3:
            mov r15, qword [rsp - 92]
            cmp r15, 20
            mov r14, 53
            cmovge rbp, r14
            jge panic_bounds
            mov r14, qword [rsp - 84]
            cmp r14, 20
            mov r13, 53
            cmovge rbp, r13
            jge panic_bounds
            mov r13b, byte [rsp + r14 - 67]
            mov byte [rsp + r15 - 67], r13b
            add qword [rsp - 92], 1
            add qword [rsp - 84], 1
            if_56_12_86_3:
            cmp_56_12_86_3:
                mov r15, qword [rsp - 84]
                mov r14, 20
            cmp r15, r14
            jne if_56_9_86_3_end
            jmp if_56_12_86_3_code
            if_56_12_86_3_code:
                jmp loop_52_5_86_3_end
            if_56_9_86_3_end:
        jmp loop_52_5_86_3
        loop_52_5_86_3_end:
        mov rdx, qword [rsp - 92]
        lea rsi, [rsp - 67]
        print_59_5_86_3:
            mov rax, 1
            mov rdi, 1
            syscall
        print_59_5_86_3_end:
    print_num_86_3_end:
    mov rdx, 1
    lea rsi, [rsp - 1]
    print_87_3:
        mov rax, 1
        mov rdi, 1
        syscall
    print_87_3_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    lea rdi, [num_buffer + 19]
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
    lea rdx, [num_buffer + 20]
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
