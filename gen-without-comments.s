DEFAULT REL
section .bss
stk resd 65536
stk.end:
section .data
section .text
bits 64
global _start
_start:
lea rsi, [dat]
lea rdi, [stk.end]
sub rdi, dat.len
mov rcx, dat.len
cld
rep movsb
mov rsp, stk.end
main:
    cmp_23_12:
    cmp qword [rsp - 8], 0
    sete r15b
    bool_end_23_12:
    assert_23_5:
        if_7_29_23_5:
        cmp_7_29_23_5:
        cmp r15b, 0
        jne if_7_26_23_5_end
        if_7_29_23_5_code:
            mov rdi, 1
            exit_7_38_23_5:
                    mov rax, 60
                syscall
            exit_7_38_23_5_end:
        if_7_26_23_5_end:
    assert_23_5_end:
    mov qword [rsp - 8], 255
    cmp_25_12:
    cmp qword [rsp - 8], 255
    sete r15b
    bool_end_25_12:
    assert_25_5:
        if_7_29_25_5:
        cmp_7_29_25_5:
        cmp r15b, 0
        jne if_7_26_25_5_end
        if_7_29_25_5_code:
            mov rdi, 1
            exit_7_38_25_5:
                    mov rax, 60
                syscall
            exit_7_38_25_5_end:
        if_7_26_25_5_end:
    assert_25_5_end:
    cmp_27_12:
    cmp dword [rsp - 12], 32
    sete r15b
    bool_end_27_12:
    assert_27_5:
        if_7_29_27_5:
        cmp_7_29_27_5:
        cmp r15b, 0
        jne if_7_26_27_5_end
        if_7_29_27_5_code:
            mov rdi, 1
            exit_7_38_27_5:
                    mov rax, 60
                syscall
            exit_7_38_27_5_end:
        if_7_26_27_5_end:
    assert_27_5_end:
    mov dword [rsp - 12], 10
    cmp_29_12:
    cmp dword [rsp - 12], 10
    sete r15b
    bool_end_29_12:
    assert_29_5:
        if_7_29_29_5:
        cmp_7_29_29_5:
        cmp r15b, 0
        jne if_7_26_29_5_end
        if_7_29_29_5_code:
            mov rdi, 1
            exit_7_38_29_5:
                    mov rax, 60
                syscall
            exit_7_38_29_5_end:
        if_7_26_29_5_end:
    assert_29_5_end:
    cmp_31_12:
    mov r14, 0
    cmp word [rsp + r14 * 2 - 20], 1
    sete r15b
    bool_end_31_12:
    assert_31_5:
        if_7_29_31_5:
        cmp_7_29_31_5:
        cmp r15b, 0
        jne if_7_26_31_5_end
        if_7_29_31_5_code:
            mov rdi, 1
            exit_7_38_31_5:
                    mov rax, 60
                syscall
            exit_7_38_31_5_end:
        if_7_26_31_5_end:
    assert_31_5_end:
    cmp_32_12:
    mov r14, 1
    cmp word [rsp + r14 * 2 - 20], 2
    sete r15b
    bool_end_32_12:
    assert_32_5:
        if_7_29_32_5:
        cmp_7_29_32_5:
        cmp r15b, 0
        jne if_7_26_32_5_end
        if_7_29_32_5_code:
            mov rdi, 1
            exit_7_38_32_5:
                    mov rax, 60
                syscall
            exit_7_38_32_5_end:
        if_7_26_32_5_end:
    assert_32_5_end:
    cmp_33_12:
    mov r14, 2
    cmp word [rsp + r14 * 2 - 20], 3
    sete r15b
    bool_end_33_12:
    assert_33_5:
        if_7_29_33_5:
        cmp_7_29_33_5:
        cmp r15b, 0
        jne if_7_26_33_5_end
        if_7_29_33_5_code:
            mov rdi, 1
            exit_7_38_33_5:
                    mov rax, 60
                syscall
            exit_7_38_33_5_end:
        if_7_26_33_5_end:
    assert_33_5_end:
    cmp_34_12:
    mov r14, 3
    cmp word [rsp + r14 * 2 - 20], 4
    sete r15b
    bool_end_34_12:
    assert_34_5:
        if_7_29_34_5:
        cmp_7_29_34_5:
        cmp r15b, 0
        jne if_7_26_34_5_end
        if_7_29_34_5_code:
            mov rdi, 1
            exit_7_38_34_5:
                    mov rax, 60
                syscall
            exit_7_38_34_5_end:
        if_7_26_34_5_end:
    assert_34_5_end:
    mov r15, 0
    mov word [rsp + r15 * 2 - 20], 5
    mov r15, 1
    mov word [rsp + r15 * 2 - 20], 6
    mov r15, 2
    mov word [rsp + r15 * 2 - 20], 7
    mov r15, 3
    mov word [rsp + r15 * 2 - 20], 8
    cmp_39_12:
    mov r14, 0
    cmp word [rsp + r14 * 2 - 20], 5
    sete r15b
    bool_end_39_12:
    assert_39_5:
        if_7_29_39_5:
        cmp_7_29_39_5:
        cmp r15b, 0
        jne if_7_26_39_5_end
        if_7_29_39_5_code:
            mov rdi, 1
            exit_7_38_39_5:
                    mov rax, 60
                syscall
            exit_7_38_39_5_end:
        if_7_26_39_5_end:
    assert_39_5_end:
    cmp_40_12:
    mov r14, 1
    cmp word [rsp + r14 * 2 - 20], 6
    sete r15b
    bool_end_40_12:
    assert_40_5:
        if_7_29_40_5:
        cmp_7_29_40_5:
        cmp r15b, 0
        jne if_7_26_40_5_end
        if_7_29_40_5_code:
            mov rdi, 1
            exit_7_38_40_5:
                    mov rax, 60
                syscall
            exit_7_38_40_5_end:
        if_7_26_40_5_end:
    assert_40_5_end:
    cmp_41_12:
    mov r14, 2
    cmp word [rsp + r14 * 2 - 20], 7
    sete r15b
    bool_end_41_12:
    assert_41_5:
        if_7_29_41_5:
        cmp_7_29_41_5:
        cmp r15b, 0
        jne if_7_26_41_5_end
        if_7_29_41_5_code:
            mov rdi, 1
            exit_7_38_41_5:
                    mov rax, 60
                syscall
            exit_7_38_41_5_end:
        if_7_26_41_5_end:
    assert_41_5_end:
    cmp_42_12:
    mov r14, 3
    cmp word [rsp + r14 * 2 - 20], 8
    sete r15b
    bool_end_42_12:
    assert_42_5:
        if_7_29_42_5:
        cmp_7_29_42_5:
        cmp r15b, 0
        jne if_7_26_42_5_end
        if_7_29_42_5_code:
            mov rdi, 1
            exit_7_38_42_5:
                    mov rax, 60
                syscall
            exit_7_38_42_5_end:
        if_7_26_42_5_end:
    assert_42_5_end:
    cmp_44_12:
    cmp byte [rsp - 21], 0
    sete r15b
    bool_end_44_12:
    assert_44_5:
        if_7_29_44_5:
        cmp_7_29_44_5:
        cmp r15b, 0
        jne if_7_26_44_5_end
        if_7_29_44_5_code:
            mov rdi, 1
            exit_7_38_44_5:
                    mov rax, 60
                syscall
            exit_7_38_44_5_end:
        if_7_26_44_5_end:
    assert_44_5_end:
    mov byte [rsp - 21], 1
    cmp_46_12:
    cmp byte [rsp - 21], 1
    sete r15b
    bool_end_46_12:
    assert_46_5:
        if_7_29_46_5:
        cmp_7_29_46_5:
        cmp r15b, 0
        jne if_7_26_46_5_end
        if_7_29_46_5_code:
            mov rdi, 1
            exit_7_38_46_5:
                    mov rax, 60
                syscall
            exit_7_38_46_5_end:
        if_7_26_46_5_end:
    assert_46_5_end:
    cmp_48_12:
    cmp byte [rsp - 22], 1
    sete r15b
    bool_end_48_12:
    assert_48_5:
        if_7_29_48_5:
        cmp_7_29_48_5:
        cmp r15b, 0
        jne if_7_26_48_5_end
        if_7_29_48_5_code:
            mov rdi, 1
            exit_7_38_48_5:
                    mov rax, 60
                syscall
            exit_7_38_48_5_end:
        if_7_26_48_5_end:
    assert_48_5_end:
    mov byte [rsp - 22], 0
    cmp_50_12:
    cmp byte [rsp - 22], 0
    sete r15b
    bool_end_50_12:
    assert_50_5:
        if_7_29_50_5:
        cmp_7_29_50_5:
        cmp r15b, 0
        jne if_7_26_50_5_end
        if_7_29_50_5_code:
            mov rdi, 1
            exit_7_38_50_5:
                    mov rax, 60
                syscall
            exit_7_38_50_5_end:
        if_7_26_50_5_end:
    assert_50_5_end:
    cmp_52_12:
    mov r14, 0
    cmp word [rsp + r14 * 2 - 28], 1
    sete r15b
    bool_end_52_12:
    assert_52_5:
        if_7_29_52_5:
        cmp_7_29_52_5:
        cmp r15b, 0
        jne if_7_26_52_5_end
        if_7_29_52_5_code:
            mov rdi, 1
            exit_7_38_52_5:
                    mov rax, 60
                syscall
            exit_7_38_52_5_end:
        if_7_26_52_5_end:
    assert_52_5_end:
    cmp_53_12:
    mov r14, 1
    cmp word [rsp + r14 * 2 - 28], 2
    sete r15b
    bool_end_53_12:
    assert_53_5:
        if_7_29_53_5:
        cmp_7_29_53_5:
        cmp r15b, 0
        jne if_7_26_53_5_end
        if_7_29_53_5_code:
            mov rdi, 1
            exit_7_38_53_5:
                    mov rax, 60
                syscall
            exit_7_38_53_5_end:
        if_7_26_53_5_end:
    assert_53_5_end:
    cmp_54_12:
    mov r14, 2
    cmp word [rsp + r14 * 2 - 28], 3
    sete r15b
    bool_end_54_12:
    assert_54_5:
        if_7_29_54_5:
        cmp_7_29_54_5:
        cmp r15b, 0
        jne if_7_26_54_5_end
        if_7_29_54_5_code:
            mov rdi, 1
            exit_7_38_54_5:
                    mov rax, 60
                syscall
            exit_7_38_54_5_end:
        if_7_26_54_5_end:
    assert_54_5_end:
    cmp_56_12:
    mov r14, 2
    cmp word [rsp + r14 * 2 - 38], 0
    sete r15b
    bool_end_56_12:
    assert_56_5:
        if_7_29_56_5:
        cmp_7_29_56_5:
        cmp r15b, 0
        jne if_7_26_56_5_end
        if_7_29_56_5_code:
            mov rdi, 1
            exit_7_38_56_5:
                    mov rax, 60
                syscall
            exit_7_38_56_5_end:
        if_7_26_56_5_end:
    assert_56_5_end:
    cmp_57_12:
    mov r14, 3
    cmp word [rsp + r14 * 2 - 38], 0
    sete r15b
    bool_end_57_12:
    assert_57_5:
        if_7_29_57_5:
        cmp_7_29_57_5:
        cmp r15b, 0
        jne if_7_26_57_5_end
        if_7_29_57_5_code:
            mov rdi, 1
            exit_7_38_57_5:
                    mov rax, 60
                syscall
            exit_7_38_57_5_end:
        if_7_26_57_5_end:
    assert_57_5_end:
    cmp_58_12:
    mov r14, 4
    cmp word [rsp + r14 * 2 - 38], 0
    sete r15b
    bool_end_58_12:
    assert_58_5:
        if_7_29_58_5:
        cmp_7_29_58_5:
        cmp r15b, 0
        jne if_7_26_58_5_end
        if_7_29_58_5_code:
            mov rdi, 1
            exit_7_38_58_5:
                    mov rax, 60
                syscall
            exit_7_38_58_5_end:
        if_7_26_58_5_end:
    assert_58_5_end:
    mov r15, 127
    mov byte [rsp + r15 - 166], 254
    cmp_61_12:
    mov r14, 127
    cmp byte [rsp + r14 - 166], 254
    sete r15b
    bool_end_61_12:
    assert_61_5:
        if_7_29_61_5:
        cmp_7_29_61_5:
        cmp r15b, 0
        jne if_7_26_61_5_end
        if_7_29_61_5_code:
            mov rdi, 1
            exit_7_38_61_5:
                    mov rax, 60
                syscall
            exit_7_38_61_5_end:
        if_7_26_61_5_end:
    assert_61_5_end:
    cmp_63_12:
        mov r14, 12
    cmp r14, 12
    sete r15b
    bool_end_63_12:
    assert_63_5:
        if_7_29_63_5:
        cmp_7_29_63_5:
        cmp r15b, 0
        jne if_7_26_63_5_end
        if_7_29_63_5_code:
            mov rdi, 1
            exit_7_38_63_5:
                    mov rax, 60
                syscall
            exit_7_38_63_5_end:
        if_7_26_63_5_end:
    assert_63_5_end:
    cmp_64_12:
    mov r14, 0
    cmp byte [rsp + r14 - 181], 104
    sete r15b
    bool_end_64_12:
    assert_64_5:
        if_7_29_64_5:
        cmp_7_29_64_5:
        cmp r15b, 0
        jne if_7_26_64_5_end
        if_7_29_64_5_code:
            mov rdi, 1
            exit_7_38_64_5:
                    mov rax, 60
                syscall
            exit_7_38_64_5_end:
        if_7_26_64_5_end:
    assert_64_5_end:
    cmp_65_12:
    mov r14, 11
    cmp byte [rsp + r14 - 181], 10
    sete r15b
    bool_end_65_12:
    assert_65_5:
        if_7_29_65_5:
        cmp_7_29_65_5:
        cmp r15b, 0
        jne if_7_26_65_5_end
        if_7_29_65_5_code:
            mov rdi, 1
            exit_7_38_65_5:
                    mov rax, 60
                syscall
            exit_7_38_65_5_end:
        if_7_26_65_5_end:
    assert_65_5_end:
    cmp_67_12:
        mov r14, 10
    cmp r14, 10
    sete r15b
    bool_end_67_12:
    assert_67_5:
        if_7_29_67_5:
        cmp_7_29_67_5:
        cmp r15b, 0
        jne if_7_26_67_5_end
        if_7_29_67_5_code:
            mov rdi, 1
            exit_7_38_67_5:
                    mov rax, 60
                syscall
            exit_7_38_67_5_end:
        if_7_26_67_5_end:
    assert_67_5_end:
    cmp_68_12:
    mov r14, 0
    cmp byte [rsp + r14 - 191], 97
    sete r15b
    bool_end_68_12:
    assert_68_5:
        if_7_29_68_5:
        cmp_7_29_68_5:
        cmp r15b, 0
        jne if_7_26_68_5_end
        if_7_29_68_5_code:
            mov rdi, 1
            exit_7_38_68_5:
                    mov rax, 60
                syscall
            exit_7_38_68_5_end:
        if_7_26_68_5_end:
    assert_68_5_end:
    cmp_69_12:
    mov r14, 9
    cmp byte [rsp + r14 - 191], 0
    sete r15b
    bool_end_69_12:
    assert_69_5:
        if_7_29_69_5:
        cmp_7_29_69_5:
        cmp r15b, 0
        jne if_7_26_69_5_end
        if_7_29_69_5_code:
            mov rdi, 1
            exit_7_38_69_5:
                    mov rax, 60
                syscall
            exit_7_38_69_5_end:
        if_7_26_69_5_end:
    assert_69_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
section .rodata
dat:
db 'a'
times 9 db 0
db 'hello world', 10, ''
db 1, 0
db 0
times 128 db 0
dw 1, 2
times 3 dw 0
dw 1, 2, 3
db 1
db 0
dw 1, 2, 3, 4
dd 32
dq 0
dat.len equ $ - dat
