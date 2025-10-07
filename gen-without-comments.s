section .bss
stk resd 0x10000
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz',10,''
hello.len equ $-hello
input: db '............................................................'
input.len equ $-input
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
nl: db '',10,''
nl.len equ $-nl
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    mov qword [rsp - 8], 1
    mov qword [rsp - 16], 2
    mov dword [rsp - 52], 0xff0000
        mov r15, qword [rsp - 8]
        mov qword [rsp - 48], r15
        mov r15, qword [rsp - 48]
        imul r15, 10
        mov qword [rsp - 48], r15
        mov r15, qword [rsp - 16]
        mov qword [rsp - 40], r15
    cmp_31_12:
    cmp qword [rsp - 48], 10
    jne bool_false_31_12
    jmp bool_true_31_12
    bool_true_31_12:
    mov r15, true
    jmp bool_end_31_12
    bool_false_31_12:
    mov r15, false
    bool_end_31_12:
    assert_31_5:
        if_19_8_31_5:
        cmp_19_8_31_5:
        cmp r15, false
        jne if_19_5_31_5_end
        jmp if_19_8_31_5_code
        if_19_8_31_5_code:
            mov rdi, 1
            exit_19_17_31_5:
                mov rax, 60
                syscall
            exit_19_17_31_5_end:
        if_19_5_31_5_end:
    assert_31_5_end:
    cmp_32_12:
    cmp qword [rsp - 40], 2
    jne bool_false_32_12
    jmp bool_true_32_12
    bool_true_32_12:
    mov r15, true
    jmp bool_end_32_12
    bool_false_32_12:
    mov r15, false
    bool_end_32_12:
    assert_32_5:
        if_19_8_32_5:
        cmp_19_8_32_5:
        cmp r15, false
        jne if_19_5_32_5_end
        jmp if_19_8_32_5_code
        if_19_8_32_5_code:
            mov rdi, 1
            exit_19_17_32_5:
                mov rax, 60
                syscall
            exit_19_17_32_5_end:
        if_19_5_32_5_end:
    assert_32_5_end:
    cmp_33_12:
    cmp dword [rsp - 52], 0xff0000
    jne bool_false_33_12
    jmp bool_true_33_12
    bool_true_33_12:
    mov r15, true
    jmp bool_end_33_12
    bool_false_33_12:
    mov r15, false
    bool_end_33_12:
    assert_33_5:
        if_19_8_33_5:
        cmp_19_8_33_5:
        cmp r15, false
        jne if_19_5_33_5_end
        jmp if_19_8_33_5_code
        if_19_8_33_5_code:
            mov rdi, 1
            exit_19_17_33_5:
                mov rax, 60
                syscall
            exit_19_17_33_5_end:
        if_19_5_33_5_end:
    assert_33_5_end:
    mov r15, qword [rsp - 8]
    mov qword [rsp - 68], r15
    neg qword [rsp - 68]
    mov r15, qword [rsp - 16]
    mov qword [rsp - 60], r15
    neg qword [rsp - 60]
    lea r15, [rsp - 52]
    mov r14, 1
    shl r14, 4
    add r15, r14
    add r15, 4
    lea rdi, [r15]
    lea rsi, [rsp - 68]
    mov rcx, 16
    rep movsb
    cmp_37_12:
        lea r13, [rsp - 52]
        mov r12, 1
        shl r12, 4
        add r13, r12
        add r13, 4
        mov r14, qword [r13]
    cmp r14, -1
    jne bool_false_37_12
    jmp bool_true_37_12
    bool_true_37_12:
    mov r15, true
    jmp bool_end_37_12
    bool_false_37_12:
    mov r15, false
    bool_end_37_12:
    assert_37_5:
        if_19_8_37_5:
        cmp_19_8_37_5:
        cmp r15, false
        jne if_19_5_37_5_end
        jmp if_19_8_37_5_code
        if_19_8_37_5_code:
            mov rdi, 1
            exit_19_17_37_5:
                mov rax, 60
                syscall
            exit_19_17_37_5_end:
        if_19_5_37_5_end:
    assert_37_5_end:
    cmp_38_12:
        lea r13, [rsp - 52]
        mov r12, 1
        shl r12, 4
        add r13, r12
        add r13, 12
        mov r14, qword [r13]
    cmp r14, -2
    jne bool_false_38_12
    jmp bool_true_38_12
    bool_true_38_12:
    mov r15, true
    jmp bool_end_38_12
    bool_false_38_12:
    mov r15, false
    bool_end_38_12:
    assert_38_5:
        if_19_8_38_5:
        cmp_19_8_38_5:
        cmp r15, false
        jne if_19_5_38_5_end
        jmp if_19_8_38_5_code
        if_19_8_38_5_code:
            mov rdi, 1
            exit_19_17_38_5:
                mov rax, 60
                syscall
            exit_19_17_38_5_end:
        if_19_5_38_5_end:
    assert_38_5_end:
    lea rdi, [rsp - 104]
    lea rsi, [rsp - 52]
    mov rcx, 36
    rep movsb
    cmp_41_12:
        lea r13, [rsp - 104]
        mov r12, 1
        shl r12, 4
        add r13, r12
        add r13, 4
        mov r14, qword [r13]
    cmp r14, -1
    jne bool_false_41_12
    jmp bool_true_41_12
    bool_true_41_12:
    mov r15, true
    jmp bool_end_41_12
    bool_false_41_12:
    mov r15, false
    bool_end_41_12:
    assert_41_5:
        if_19_8_41_5:
        cmp_19_8_41_5:
        cmp r15, false
        jne if_19_5_41_5_end
        jmp if_19_8_41_5_code
        if_19_8_41_5_code:
            mov rdi, 1
            exit_19_17_41_5:
                mov rax, 60
                syscall
            exit_19_17_41_5_end:
        if_19_5_41_5_end:
    assert_41_5_end:
    cmp_42_12:
        lea r13, [rsp - 104]
        mov r12, 1
        shl r12, 4
        add r13, r12
        add r13, 12
        mov r14, qword [r13]
    cmp r14, -2
    jne bool_false_42_12
    jmp bool_true_42_12
    bool_true_42_12:
    mov r15, true
    jmp bool_end_42_12
    bool_false_42_12:
    mov r15, false
    bool_end_42_12:
    assert_42_5:
        if_19_8_42_5:
        cmp_19_8_42_5:
        cmp r15, false
        jne if_19_5_42_5_end
        jmp if_19_8_42_5_code
        if_19_8_42_5_code:
            mov rdi, 1
            exit_19_17_42_5:
                mov rax, 60
                syscall
            exit_19_17_42_5_end:
        if_19_5_42_5_end:
    assert_42_5_end:
    cmp_43_12:
    cmp dword [rsp - 104], 0xff0000
    jne bool_false_43_12
    jmp bool_true_43_12
    bool_true_43_12:
    mov r15, true
    jmp bool_end_43_12
    bool_false_43_12:
    mov r15, false
    bool_end_43_12:
    assert_43_5:
        if_19_8_43_5:
        cmp_19_8_43_5:
        cmp r15, false
        jne if_19_5_43_5_end
        jmp if_19_8_43_5_code
        if_19_8_43_5_code:
            mov rdi, 1
            exit_19_17_43_5:
                mov rax, 60
                syscall
            exit_19_17_43_5_end:
        if_19_5_43_5_end:
    assert_43_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
