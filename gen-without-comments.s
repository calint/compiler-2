section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $-hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
nl: db '', 10,''
nl.len equ $-nl
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    cmp r15, 4
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    cmp r15, 4
    jge panic_bounds
    mov r14, qword [rsp - 24]
    cmp r14, 4
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_79_12:
        mov r13, 1
        cmp r13, 4
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_79_12
    bool_true_79_12:
    mov r15, true
    jmp bool_end_79_12
    bool_false_79_12:
    mov r15, false
    bool_end_79_12:
    assert_79_5:
        if_19_8_79_5:
        cmp_19_8_79_5:
        cmp r15, false
        jne if_19_5_79_5_end
        if_19_8_79_5_code:
            mov rdi, 1
            exit_19_17_79_5:
                mov rax, 60
                syscall
            exit_19_17_79_5_end:
        if_19_5_79_5_end:
    assert_79_5_end:
    cmp_80_12:
        mov r13, 2
        cmp r13, 4
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_80_12
    bool_true_80_12:
    mov r15, true
    jmp bool_end_80_12
    bool_false_80_12:
    mov r15, false
    bool_end_80_12:
    assert_80_5:
        if_19_8_80_5:
        cmp_19_8_80_5:
        cmp r15, false
        jne if_19_5_80_5_end
        if_19_8_80_5_code:
            mov rdi, 1
            exit_19_17_80_5:
                mov rax, 60
                syscall
            exit_19_17_80_5_end:
        if_19_5_80_5_end:
    assert_80_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, rcx
    add r14, r15
    cmp r14, 4
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    cmp rcx, 4
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_84_12:
        mov r13, 0
        cmp r13, 4
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_84_12
    bool_true_84_12:
    mov r15, true
    jmp bool_end_84_12
    bool_false_84_12:
    mov r15, false
    bool_end_84_12:
    assert_84_5:
        if_19_8_84_5:
        cmp_19_8_84_5:
        cmp r15, false
        jne if_19_5_84_5_end
        if_19_8_84_5_code:
            mov rdi, 1
            exit_19_17_84_5:
                mov rax, 60
                syscall
            exit_19_17_84_5_end:
        if_19_5_84_5_end:
    assert_84_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    cmp rcx, 4
    jg panic_bounds
    lea rsi, [rsp - 16]
    cmp rcx, 8
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_88_12:
        mov rcx, 4
        cmp rcx, 4
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_88_12
        mov r14, false
        jmp cmps_end_88_12
        cmps_eq_88_12:
        mov r14, true
        cmps_end_88_12:
    cmp r14, false
    je bool_false_88_12
    bool_true_88_12:
    mov r15, true
    jmp bool_end_88_12
    bool_false_88_12:
    mov r15, false
    bool_end_88_12:
    assert_88_5:
        if_19_8_88_5:
        cmp_19_8_88_5:
        cmp r15, false
        jne if_19_5_88_5_end
        if_19_8_88_5_code:
            mov rdi, 1
            exit_19_17_88_5:
                mov rax, 60
                syscall
            exit_19_17_88_5_end:
        if_19_5_88_5_end:
    assert_88_5_end:
    mov r15, 2
    cmp r15, 8
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_91_12:
        mov rcx, 4
        cmp rcx, 4
        jg panic_bounds
        lea rsi, [rsp - 16]
        cmp rcx, 8
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_91_16
        mov r14, false
        jmp cmps_end_91_16
        cmps_eq_91_16:
        mov r14, true
        cmps_end_91_16:
    cmp r14, false
    jne bool_false_91_12
    bool_true_91_12:
    mov r15, true
    jmp bool_end_91_12
    bool_false_91_12:
    mov r15, false
    bool_end_91_12:
    assert_91_5:
        if_19_8_91_5:
        cmp_19_8_91_5:
        cmp r15, false
        jne if_19_5_91_5_end
        if_19_8_91_5_code:
            mov rdi, 1
            exit_19_17_91_5:
                mov rax, 60
                syscall
            exit_19_17_91_5_end:
        if_19_5_91_5_end:
    assert_91_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    cmp r15, 4
    jge panic_bounds
    mov r13, qword [rsp - 24]
    sub r13, 1
    cmp r13, 4
    jge panic_bounds
    movsx r14, dword [rsp + r13 * 4 - 16]
    inv_94_16:
        mov dword [rsp + r15 * 4 - 16], r14d
        not dword [rsp + r15 * 4 - 16]
    inv_94_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_95_12:
        mov r13, qword [rsp - 24]
        cmp r13, 4
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_95_12
    bool_true_95_12:
    mov r15, true
    jmp bool_end_95_12
    bool_false_95_12:
    mov r15, false
    bool_end_95_12:
    assert_95_5:
        if_19_8_95_5:
        cmp_19_8_95_5:
        cmp r15, false
        jne if_19_5_95_5_end
        if_19_8_95_5_code:
            mov rdi, 1
            exit_19_17_95_5:
                mov rax, 60
                syscall
            exit_19_17_95_5_end:
        if_19_5_95_5_end:
    assert_95_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_98_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_98_5_end:
    cmp_99_12:
    cmp qword [rsp - 72], 2
    jne bool_false_99_12
    bool_true_99_12:
    mov r15, true
    jmp bool_end_99_12
    bool_false_99_12:
    mov r15, false
    bool_end_99_12:
    assert_99_5:
        if_19_8_99_5:
        cmp_19_8_99_5:
        cmp r15, false
        jne if_19_5_99_5_end
        if_19_8_99_5_code:
            mov rdi, 1
            exit_19_17_99_5:
                mov rax, 60
                syscall
            exit_19_17_99_5_end:
        if_19_5_99_5_end:
    assert_99_5_end:
    cmp_100_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_100_12
    bool_true_100_12:
    mov r15, true
    jmp bool_end_100_12
    bool_false_100_12:
    mov r15, false
    bool_end_100_12:
    assert_100_5:
        if_19_8_100_5:
        cmp_19_8_100_5:
        cmp r15, false
        jne if_19_5_100_5_end
        if_19_8_100_5_code:
            mov rdi, 1
            exit_19_17_100_5:
                mov rax, 60
                syscall
            exit_19_17_100_5_end:
        if_19_5_100_5_end:
    assert_100_5_end:
    mov qword [rsp - 80], 0
    bar_103_5:
        if_57_8_103_5:
        cmp_57_8_103_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_103_5_end
        if_57_8_103_5_code:
            jmp bar_103_5_end
        if_57_5_103_5_end:
        mov qword [rsp - 80], 0xff
    bar_103_5_end:
    cmp_104_12:
    cmp qword [rsp - 80], 0
    jne bool_false_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_19_8_104_5:
        cmp_19_8_104_5:
        cmp r15, false
        jne if_19_5_104_5_end
        if_19_8_104_5_code:
            mov rdi, 1
            exit_19_17_104_5:
                mov rax, 60
                syscall
            exit_19_17_104_5_end:
        if_19_5_104_5_end:
    assert_104_5_end:
    mov qword [rsp - 80], 1
    bar_107_5:
        if_57_8_107_5:
        cmp_57_8_107_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_107_5_end
        if_57_8_107_5_code:
            jmp bar_107_5_end
        if_57_5_107_5_end:
        mov qword [rsp - 80], 0xff
    bar_107_5_end:
    cmp_108_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
    assert_108_5:
        if_19_8_108_5:
        cmp_19_8_108_5:
        cmp r15, false
        jne if_19_5_108_5_end
        if_19_8_108_5_code:
            mov rdi, 1
            exit_19_17_108_5:
                mov rax, 60
                syscall
            exit_19_17_108_5_end:
        if_19_5_108_5_end:
    assert_108_5_end:
    mov qword [rsp - 88], 1
    baz_111_13:
        mov r15, qword [rsp - 88]
        mov qword [rsp - 96], r15
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
    baz_111_13_end:
    cmp_112_12:
    cmp qword [rsp - 96], 2
    jne bool_false_112_12
    bool_true_112_12:
    mov r15, true
    jmp bool_end_112_12
    bool_false_112_12:
    mov r15, false
    bool_end_112_12:
    assert_112_5:
        if_19_8_112_5:
        cmp_19_8_112_5:
        cmp r15, false
        jne if_19_5_112_5_end
        if_19_8_112_5_code:
            mov rdi, 1
            exit_19_17_112_5:
                mov rax, 60
                syscall
            exit_19_17_112_5_end:
        if_19_5_112_5_end:
    assert_112_5_end:
    baz_114_9:
        mov qword [rsp - 96], 1
        mov r15, qword [rsp - 96]
        imul r15, 2
        mov qword [rsp - 96], r15
    baz_114_9_end:
    cmp_115_12:
    cmp qword [rsp - 96], 2
    jne bool_false_115_12
    bool_true_115_12:
    mov r15, true
    jmp bool_end_115_12
    bool_false_115_12:
    mov r15, false
    bool_end_115_12:
    assert_115_5:
        if_19_8_115_5:
        cmp_19_8_115_5:
        cmp r15, false
        jne if_19_5_115_5_end
        if_19_8_115_5_code:
            mov rdi, 1
            exit_19_17_115_5:
                mov rax, 60
                syscall
            exit_19_17_115_5_end:
        if_19_5_115_5_end:
    assert_115_5_end:
    baz_117_23:
        mov qword [rsp - 112], 2
        mov r15, qword [rsp - 112]
        imul r15, 2
        mov qword [rsp - 112], r15
    baz_117_23_end:
    mov qword [rsp - 104], 0
    cmp_118_12:
    cmp qword [rsp - 112], 4
    jne bool_false_118_12
    bool_true_118_12:
    mov r15, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15, false
    bool_end_118_12:
    assert_118_5:
        if_19_8_118_5:
        cmp_19_8_118_5:
        cmp r15, false
        jne if_19_5_118_5_end
        if_19_8_118_5_code:
            mov rdi, 1
            exit_19_17_118_5:
                mov rax, 60
                syscall
            exit_19_17_118_5_end:
        if_19_5_118_5_end:
    assert_118_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 148]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_124_12:
    cmp qword [rsp - 148], 10
    jne bool_false_124_12
    bool_true_124_12:
    mov r15, true
    jmp bool_end_124_12
    bool_false_124_12:
    mov r15, false
    bool_end_124_12:
    assert_124_5:
        if_19_8_124_5:
        cmp_19_8_124_5:
        cmp r15, false
        jne if_19_5_124_5_end
        if_19_8_124_5_code:
            mov rdi, 1
            exit_19_17_124_5:
                mov rax, 60
                syscall
            exit_19_17_124_5_end:
        if_19_5_124_5_end:
    assert_124_5_end:
    cmp_125_12:
    cmp qword [rsp - 140], 2
    jne bool_false_125_12
    bool_true_125_12:
    mov r15, true
    jmp bool_end_125_12
    bool_false_125_12:
    mov r15, false
    bool_end_125_12:
    assert_125_5:
        if_19_8_125_5:
        cmp_19_8_125_5:
        cmp r15, false
        jne if_19_5_125_5_end
        if_19_8_125_5_code:
            mov rdi, 1
            exit_19_17_125_5:
                mov rax, 60
                syscall
            exit_19_17_125_5_end:
        if_19_5_125_5_end:
    assert_125_5_end:
    cmp_126_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_126_12
    bool_true_126_12:
    mov r15, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15, false
    bool_end_126_12:
    assert_126_5:
        if_19_8_126_5:
        cmp_19_8_126_5:
        cmp r15, false
        jne if_19_5_126_5_end
        if_19_8_126_5_code:
            mov rdi, 1
            exit_19_17_126_5:
                mov rax, 60
                syscall
            exit_19_17_126_5_end:
        if_19_5_126_5_end:
    assert_126_5_end:
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
    neg qword [rsp - 164]
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
    neg qword [rsp - 156]
    lea rdi, [rsp - 148]
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
    cmp_130_12:
    cmp qword [rsp - 148], -1
    jne bool_false_130_12
    bool_true_130_12:
    mov r15, true
    jmp bool_end_130_12
    bool_false_130_12:
    mov r15, false
    bool_end_130_12:
    assert_130_5:
        if_19_8_130_5:
        cmp_19_8_130_5:
        cmp r15, false
        jne if_19_5_130_5_end
        if_19_8_130_5_code:
            mov rdi, 1
            exit_19_17_130_5:
                mov rax, 60
                syscall
            exit_19_17_130_5_end:
        if_19_5_130_5_end:
    assert_130_5_end:
    cmp_131_12:
    cmp qword [rsp - 140], -2
    jne bool_false_131_12
    bool_true_131_12:
    mov r15, true
    jmp bool_end_131_12
    bool_false_131_12:
    mov r15, false
    bool_end_131_12:
    assert_131_5:
        if_19_8_131_5:
        cmp_19_8_131_5:
        cmp r15, false
        jne if_19_5_131_5_end
        if_19_8_131_5_code:
            mov rdi, 1
            exit_19_17_131_5:
                mov rax, 60
                syscall
            exit_19_17_131_5_end:
        if_19_5_131_5_end:
    assert_131_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_134_12:
    cmp qword [rsp - 184], -1
    jne bool_false_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
    assert_134_5:
        if_19_8_134_5:
        cmp_19_8_134_5:
        cmp r15, false
        jne if_19_5_134_5_end
        if_19_8_134_5_code:
            mov rdi, 1
            exit_19_17_134_5:
                mov rax, 60
                syscall
            exit_19_17_134_5_end:
        if_19_5_134_5_end:
    assert_134_5_end:
    cmp_135_12:
    cmp qword [rsp - 176], -2
    jne bool_false_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
    assert_135_5:
        if_19_8_135_5:
        cmp_19_8_135_5:
        cmp r15, false
        jne if_19_5_135_5_end
        if_19_8_135_5_code:
            mov rdi, 1
            exit_19_17_135_5:
                mov rax, 60
                syscall
            exit_19_17_135_5_end:
        if_19_5_135_5_end:
    assert_135_5_end:
    cmp_136_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_136_12
    bool_true_136_12:
    mov r15, true
    jmp bool_end_136_12
    bool_false_136_12:
    mov r15, false
    bool_end_136_12:
    assert_136_5:
        if_19_8_136_5:
        cmp_19_8_136_5:
        cmp r15, false
        jne if_19_5_136_5_end
        if_19_8_136_5_code:
            mov rdi, 1
            exit_19_17_136_5:
                mov rax, 60
                syscall
            exit_19_17_136_5_end:
        if_19_5_136_5_end:
    assert_136_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_141_12:
        lea r13, [rsp - 204]
        mov r12, 0
        cmp r12, 1
        jge panic_bounds
        imul r12, 20
        add r13, r12
        add r13, 8
        mov r14, qword [r13]
    cmp r14, 73
    jne bool_false_141_12
    bool_true_141_12:
    mov r15, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15, false
    bool_end_141_12:
    assert_141_5:
        if_19_8_141_5:
        cmp_19_8_141_5:
        cmp r15, false
        jne if_19_5_141_5_end
        if_19_8_141_5_code:
            mov rdi, 1
            exit_19_17_141_5:
                mov rax, 60
                syscall
            exit_19_17_141_5_end:
        if_19_5_141_5_end:
    assert_141_5_end:
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
    mov rdx, hello.len
    mov rsi, hello
    print_145_5:
        mov rax, 1
        mov rdi, 1
        syscall
    print_145_5_end:
    loop_146_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_147_9:
            mov rax, 1
            mov rdi, 1
            syscall
        print_147_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_149_19:
            mov rax, 0
            mov rdi, 0
            syscall
            mov qword [rsp - 292], rax
        read_149_19_end:
        sub qword [rsp - 292], 1
        if_153_12:
        cmp_153_12:
        cmp qword [rsp - 292], 0
        jne if_155_19
        if_153_12_code:
            jmp loop_146_5_end
        jmp if_153_9_end
        if_155_19:
        cmp_155_19:
        cmp qword [rsp - 292], 4
        jg if_else_153_9
        if_155_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_156_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_156_13_end:
            jmp loop_146_5
        jmp if_153_9_end
        if_else_153_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_159_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_159_13_end:
            mov rdx, qword [rsp - 292]
            lea rsi, [rsp - 284]
            print_160_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_160_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_161_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_161_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_162_13:
                mov rax, 1
                mov rdi, 1
                syscall
            print_162_13_end:
        if_153_9_end:
    jmp loop_146_5
    loop_146_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 60
    mov rdi, 255
    syscall
