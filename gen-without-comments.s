default rel
section .bss
stk resd 65536
stk.end:
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
    cmp_26_12:
    cmp qword [rsp - 8], 0
    sete r15b
    bool_end_26_12:
    assert_26_5:
        if_7_29_26_5:
        cmp_7_29_26_5:
        cmp r15b, 0
        jne if_7_26_26_5_end
        if_7_29_26_5_code:
            mov rdi, 1
            exit_7_38_26_5:
                    mov rax, 60
                syscall
            exit_7_38_26_5_end:
        if_7_26_26_5_end:
    assert_26_5_end:
    mov qword [rsp - 8], 255
    cmp_28_12:
    cmp qword [rsp - 8], 255
    sete r15b
    bool_end_28_12:
    assert_28_5:
        if_7_29_28_5:
        cmp_7_29_28_5:
        cmp r15b, 0
        jne if_7_26_28_5_end
        if_7_29_28_5_code:
            mov rdi, 1
            exit_7_38_28_5:
                    mov rax, 60
                syscall
            exit_7_38_28_5_end:
        if_7_26_28_5_end:
    assert_28_5_end:
    cmp_30_12:
    cmp dword [rsp - 12], 32
    sete r15b
    bool_end_30_12:
    assert_30_5:
        if_7_29_30_5:
        cmp_7_29_30_5:
        cmp r15b, 0
        jne if_7_26_30_5_end
        if_7_29_30_5_code:
            mov rdi, 1
            exit_7_38_30_5:
                    mov rax, 60
                syscall
            exit_7_38_30_5_end:
        if_7_26_30_5_end:
    assert_30_5_end:
    mov dword [rsp - 12], 10
    cmp_32_12:
    cmp dword [rsp - 12], 10
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
    cmp_34_12:
    mov r14, 0
    cmp word [rsp + r14 * 2 - 20], 1
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
    cmp_35_12:
    mov r14, 1
    cmp word [rsp + r14 * 2 - 20], 2
    sete r15b
    bool_end_35_12:
    assert_35_5:
        if_7_29_35_5:
        cmp_7_29_35_5:
        cmp r15b, 0
        jne if_7_26_35_5_end
        if_7_29_35_5_code:
            mov rdi, 1
            exit_7_38_35_5:
                    mov rax, 60
                syscall
            exit_7_38_35_5_end:
        if_7_26_35_5_end:
    assert_35_5_end:
    cmp_36_12:
    mov r14, 2
    cmp word [rsp + r14 * 2 - 20], 3
    sete r15b
    bool_end_36_12:
    assert_36_5:
        if_7_29_36_5:
        cmp_7_29_36_5:
        cmp r15b, 0
        jne if_7_26_36_5_end
        if_7_29_36_5_code:
            mov rdi, 1
            exit_7_38_36_5:
                    mov rax, 60
                syscall
            exit_7_38_36_5_end:
        if_7_26_36_5_end:
    assert_36_5_end:
    cmp_37_12:
    mov r14, 3
    cmp word [rsp + r14 * 2 - 20], 4
    sete r15b
    bool_end_37_12:
    assert_37_5:
        if_7_29_37_5:
        cmp_7_29_37_5:
        cmp r15b, 0
        jne if_7_26_37_5_end
        if_7_29_37_5_code:
            mov rdi, 1
            exit_7_38_37_5:
                    mov rax, 60
                syscall
            exit_7_38_37_5_end:
        if_7_26_37_5_end:
    assert_37_5_end:
    mov r15, 0
    mov word [rsp + r15 * 2 - 20], 5
    mov r15, 1
    mov word [rsp + r15 * 2 - 20], 6
    mov r15, 2
    mov word [rsp + r15 * 2 - 20], 7
    mov r15, 3
    mov word [rsp + r15 * 2 - 20], 8
    cmp_42_12:
    mov r14, 0
    cmp word [rsp + r14 * 2 - 20], 5
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
    cmp_43_12:
    mov r14, 1
    cmp word [rsp + r14 * 2 - 20], 6
    sete r15b
    bool_end_43_12:
    assert_43_5:
        if_7_29_43_5:
        cmp_7_29_43_5:
        cmp r15b, 0
        jne if_7_26_43_5_end
        if_7_29_43_5_code:
            mov rdi, 1
            exit_7_38_43_5:
                    mov rax, 60
                syscall
            exit_7_38_43_5_end:
        if_7_26_43_5_end:
    assert_43_5_end:
    cmp_44_12:
    mov r14, 2
    cmp word [rsp + r14 * 2 - 20], 7
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
    cmp_45_12:
    mov r14, 3
    cmp word [rsp + r14 * 2 - 20], 8
    sete r15b
    bool_end_45_12:
    assert_45_5:
        if_7_29_45_5:
        cmp_7_29_45_5:
        cmp r15b, 0
        jne if_7_26_45_5_end
        if_7_29_45_5_code:
            mov rdi, 1
            exit_7_38_45_5:
                    mov rax, 60
                syscall
            exit_7_38_45_5_end:
        if_7_26_45_5_end:
    assert_45_5_end:
    cmp_47_12:
    cmp byte [rsp - 21], 0
    sete r15b
    bool_end_47_12:
    assert_47_5:
        if_7_29_47_5:
        cmp_7_29_47_5:
        cmp r15b, 0
        jne if_7_26_47_5_end
        if_7_29_47_5_code:
            mov rdi, 1
            exit_7_38_47_5:
                    mov rax, 60
                syscall
            exit_7_38_47_5_end:
        if_7_26_47_5_end:
    assert_47_5_end:
    mov byte [rsp - 21], 1
    cmp_49_12:
    cmp byte [rsp - 21], 1
    sete r15b
    bool_end_49_12:
    assert_49_5:
        if_7_29_49_5:
        cmp_7_29_49_5:
        cmp r15b, 0
        jne if_7_26_49_5_end
        if_7_29_49_5_code:
            mov rdi, 1
            exit_7_38_49_5:
                    mov rax, 60
                syscall
            exit_7_38_49_5_end:
        if_7_26_49_5_end:
    assert_49_5_end:
    cmp_51_12:
    cmp byte [rsp - 22], 1
    sete r15b
    bool_end_51_12:
    assert_51_5:
        if_7_29_51_5:
        cmp_7_29_51_5:
        cmp r15b, 0
        jne if_7_26_51_5_end
        if_7_29_51_5_code:
            mov rdi, 1
            exit_7_38_51_5:
                    mov rax, 60
                syscall
            exit_7_38_51_5_end:
        if_7_26_51_5_end:
    assert_51_5_end:
    mov byte [rsp - 22], 0
    cmp_53_12:
    cmp byte [rsp - 22], 0
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
    cmp_55_12:
    mov r14, 0
    cmp word [rsp + r14 * 2 - 28], 1
    sete r15b
    bool_end_55_12:
    assert_55_5:
        if_7_29_55_5:
        cmp_7_29_55_5:
        cmp r15b, 0
        jne if_7_26_55_5_end
        if_7_29_55_5_code:
            mov rdi, 1
            exit_7_38_55_5:
                    mov rax, 60
                syscall
            exit_7_38_55_5_end:
        if_7_26_55_5_end:
    assert_55_5_end:
    cmp_56_12:
    mov r14, 1
    cmp word [rsp + r14 * 2 - 28], 2
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
    mov r14, 2
    cmp word [rsp + r14 * 2 - 28], 3
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
    cmp_59_12:
    mov r14, 2
    cmp word [rsp + r14 * 2 - 38], 0
    sete r15b
    bool_end_59_12:
    assert_59_5:
        if_7_29_59_5:
        cmp_7_29_59_5:
        cmp r15b, 0
        jne if_7_26_59_5_end
        if_7_29_59_5_code:
            mov rdi, 1
            exit_7_38_59_5:
                    mov rax, 60
                syscall
            exit_7_38_59_5_end:
        if_7_26_59_5_end:
    assert_59_5_end:
    cmp_60_12:
    mov r14, 3
    cmp word [rsp + r14 * 2 - 38], 0
    sete r15b
    bool_end_60_12:
    assert_60_5:
        if_7_29_60_5:
        cmp_7_29_60_5:
        cmp r15b, 0
        jne if_7_26_60_5_end
        if_7_29_60_5_code:
            mov rdi, 1
            exit_7_38_60_5:
                    mov rax, 60
                syscall
            exit_7_38_60_5_end:
        if_7_26_60_5_end:
    assert_60_5_end:
    cmp_61_12:
    mov r14, 4
    cmp word [rsp + r14 * 2 - 38], 0
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
    mov r15, 127
    mov byte [rsp + r15 - 166], 254
    cmp_64_12:
    mov r14, 127
    cmp byte [rsp + r14 - 166], 254
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
    cmp_66_12:
        mov r14, 12
    cmp r14, 12
    sete r15b
    bool_end_66_12:
    assert_66_5:
        if_7_29_66_5:
        cmp_7_29_66_5:
        cmp r15b, 0
        jne if_7_26_66_5_end
        if_7_29_66_5_code:
            mov rdi, 1
            exit_7_38_66_5:
                    mov rax, 60
                syscall
            exit_7_38_66_5_end:
        if_7_26_66_5_end:
    assert_66_5_end:
    cmp_67_12:
    mov r14, 0
    cmp byte [rsp + r14 - 181], 104
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
    mov r14, 11
    cmp byte [rsp + r14 - 181], 10
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
    cmp_70_12:
        mov r14, 10
    cmp r14, 10
    sete r15b
    bool_end_70_12:
    assert_70_5:
        if_7_29_70_5:
        cmp_7_29_70_5:
        cmp r15b, 0
        jne if_7_26_70_5_end
        if_7_29_70_5_code:
            mov rdi, 1
            exit_7_38_70_5:
                    mov rax, 60
                syscall
            exit_7_38_70_5_end:
        if_7_26_70_5_end:
    assert_70_5_end:
    cmp_71_12:
    mov r14, 0
    cmp byte [rsp + r14 - 191], 97
    sete r15b
    bool_end_71_12:
    assert_71_5:
        if_7_29_71_5:
        cmp_7_29_71_5:
        cmp r15b, 0
        jne if_7_26_71_5_end
        if_7_29_71_5_code:
            mov rdi, 1
            exit_7_38_71_5:
                    mov rax, 60
                syscall
            exit_7_38_71_5_end:
        if_7_26_71_5_end:
    assert_71_5_end:
    cmp_72_12:
    mov r14, 9
    cmp byte [rsp + r14 - 191], 0
    sete r15b
    bool_end_72_12:
    assert_72_5:
        if_7_29_72_5:
        cmp_7_29_72_5:
        cmp r15b, 0
        jne if_7_26_72_5_end
        if_7_29_72_5_code:
            mov rdi, 1
            exit_7_38_72_5:
                    mov rax, 60
                syscall
            exit_7_38_72_5_end:
        if_7_26_72_5_end:
    assert_72_5_end:
    cmp_74_12:
    cmp qword [rsp - 207], -1
    sete r15b
    bool_end_74_12:
    assert_74_5:
        if_7_29_74_5:
        cmp_7_29_74_5:
        cmp r15b, 0
        jne if_7_26_74_5_end
        if_7_29_74_5_code:
            mov rdi, 1
            exit_7_38_74_5:
                    mov rax, 60
                syscall
            exit_7_38_74_5_end:
        if_7_26_74_5_end:
    assert_74_5_end:
    cmp_75_12:
    cmp qword [rsp - 199], -2
    sete r15b
    bool_end_75_12:
    assert_75_5:
        if_7_29_75_5:
        cmp_7_29_75_5:
        cmp r15b, 0
        jne if_7_26_75_5_end
        if_7_29_75_5_code:
            mov rdi, 1
            exit_7_38_75_5:
                    mov rax, 60
                syscall
            exit_7_38_75_5_end:
        if_7_26_75_5_end:
    assert_75_5_end:
    mov qword [rsp - 207], 3
    mov qword [rsp - 199], 4
    cmp_78_12:
    cmp qword [rsp - 207], 3
    sete r15b
    bool_end_78_12:
    assert_78_5:
        if_7_29_78_5:
        cmp_7_29_78_5:
        cmp r15b, 0
        jne if_7_26_78_5_end
        if_7_29_78_5_code:
            mov rdi, 1
            exit_7_38_78_5:
                    mov rax, 60
                syscall
            exit_7_38_78_5_end:
        if_7_26_78_5_end:
    assert_78_5_end:
    cmp_79_12:
    cmp qword [rsp - 199], 4
    sete r15b
    bool_end_79_12:
    assert_79_5:
        if_7_29_79_5:
        cmp_7_29_79_5:
        cmp r15b, 0
        jne if_7_26_79_5_end
        if_7_29_79_5_code:
            mov rdi, 1
            exit_7_38_79_5:
                    mov rax, 60
                syscall
            exit_7_38_79_5_end:
        if_7_26_79_5_end:
    assert_79_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
section .rodata
dat:
dq  -1
dq  -2
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
