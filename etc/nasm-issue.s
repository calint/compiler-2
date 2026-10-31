; NASM 3.02: "unable to find valid values for all labels" on plain code with
; only jumps and labels
;
; NASM version 3.02 (Arch Linux package nasm 3.02-1), x86_64 Linux.
;
; This file (machine-generated, reduced from a compiler's output) fails with
; the default optimization level:
;
;     $ nasm -f elf64 nasm-issue.s -o nasm-issue.o
;     nasm-issue.s: error: unable to find valid values for all labels after
;     1005 passes; stalled for 1001, giving up.
;
; The same with '-f bin' (after adding 'bits 64'), '-O2', '-O3', '-Ox' and
; '-O9999'. With '-O0' or '-O1' it assembles.
;
; The file contains only instructions, conditional and unconditional jumps
; ('jo', 'jg', 'jle', 'jl', 'jmp') and labels. Every jump target is defined.
; There is no '$', 'align', 'times', 'equ' or any other construct whose size
; depends on a label value, so growing jumps from short to near should
; converge. Many 'jo' jumps to the same targets sit near the 127-byte limit
; of a short jump.
;
; Expected: the file assembles, with each jump short or near.

        jo baz_overflow_line_30
        jo baz_overflow_line_33
                    jo baz_overflow_line_16
                    jo baz_overflow_line_16
                    jo baz_overflow_line_17
                        jo baz_overflow_line_19
                        jo baz_overflow_line_20
                            jo baz_overflow_line_4
                            jo baz_overflow_line_5
                                jo baz_overflow_line_8
                                jo baz_overflow_line_10
                jo baz_overflow_line_37
            jle if.37.13.1
                and r15, 31
                mov r14, qword [rbp + 40]
                and r14, 31
                imul r15, r14
                jo baz_overflow_line_38
                xor r15, qword [rbp + 48]
                mov qword [rbp + 56], r15
                and qword [rbp + 56], 1023
                    jo baz_overflow_line_4
                    and r14, 65535
                    mov qword [rbp], r14
                    mov qword [rbp + 72], 9
                    mov r14, 6
                    add r14, r15
                    jo baz_overflow_line_5
                    and r14, 1023
                    and r14, 31
                    imul r14, qword [rbp + 72]
                    mov qword [rbp + 72], r14
                    jo baz_overflow_line_5
                    mov qword [rbp + 80], 0
                    loop.7.5.39.26:
                        inc qword [rbp + 80]
                        jo baz_overflow_line_8
                        cmp qword [rbp + 80], 3
                        jg loop.7.5.39.26.end
                        mov r14, qword [rbp]
                        xor r14, r15
                        imul r14, 31
                        jo baz_overflow_line_10
                        and r14, 65535
                        mov qword [rbp], r14
                    jmp loop.7.5.39.26
                    loop.7.5.39.26.end:
                mov qword [rbp + 72], 6
                mov r15, qword [rbp + 24]
                add r15, qword [rbp + 40]
                jo baz_overflow_line_41
                and r15, 1023
                and r15, 31
                imul r15, qword [rbp + 72]
                mov qword [rbp + 72], r15
                jo baz_overflow_line_41
            jmp if.37.13.end
            if.37.13.1:
                jo baz_overflow_line_42
                jo baz_overflow_line_42
            jg if.37.13.end
                    jo baz_overflow_line_43
                    and r15, 31
                    mov r14, qword [rbp + 48]
                    add r14, 6
                    jo baz_overflow_line_43
                    and r14, 1023
                    and r14, 31
                    imul r15, r14
                    jo baz_overflow_line_43
                cmp r15, 6
                    jo baz_overflow_line_43
                    mov r14, qword [rbp + 32]
                    add r14, 8
                    jo baz_overflow_line_43
                and r15, 1023
                    mov qword [rbp + 56], 0
                    mov r14, qword [rbp]
                    xor r14, r15
                    imul r14, 31
                    jo baz_overflow_line_4
                    and r14, 65535
                    mov qword [rbp], r14
                    mov qword [rbp + 64], 9
                    mov r14, 6
                    add r14, r15
                    jo baz_overflow_line_5
                    and r14, 1023
                    and r14, 31
                    imul r14, qword [rbp + 64]
                    mov qword [rbp + 64], r14
                    jo baz_overflow_line_5
                    mov qword [rbp + 72], 0
                    loop.7.5.44.27:
                        inc qword [rbp + 72]
                        jo baz_overflow_line_8
                        mov r14, qword [rbp]
                        xor r14, r15
                        imul r14, 31
                        jo baz_overflow_line_10
                        and r14, 65535
                        mov qword [rbp], r14
                    jmp loop.7.5.44.27
                mov r15, qword [rbp + 24]
                and r15, 31
                mov r14, qword [rbp + 48]
                and r14, 31
                imul r15, r14
                jo baz_overflow_line_45
                mov r14, qword [rbp + 24]
                add r14, qword [rbp + 32]
                jo baz_overflow_line_45
                and r14, 1023
                add r15, r14
                jo baz_overflow_line_45
                and r15, 1023
                mov qword [rbp + 48], r15
            if.37.13.end:
    jo baz_overflow_line_49
    jo baz_overflow_line_49
            jo baz_overflow_line_16
            jo baz_overflow_line_16
            jo baz_overflow_line_16
            jo baz_overflow_line_17
            and r8, 1023
            mov r11, r10
            jo baz_overflow_line_17
            xor r8, r11
            and r8, 1023
            xor r9, r8
            imul r9, 31
            jo baz_overflow_line_17
            and r9, 65535
            mov qword [rbp], r9
                mov r9, 2
                mov r8, r14
                and r8, 31
                imul r9, r8
                jo baz_overflow_line_19
            cmp r9, 2
            jl if.19.9.49.15.else
                mov r9, qword [rbp + 56]
                mov qword [rbp + 48], r9
                add qword [rbp + 48], r14
                jo baz_overflow_line_20
                and qword [rbp + 48], 1023
                and qword [rbp + 48], 5
                and qword [rbp + 48], 1023
            jmp if.19.9.49.15.end
            if.19.9.49.15.else:
                    mov qword [rbp + 64], 0
                    mov r9, qword [rbp]
                    xor r9, r13
                    imul r9, 31
                    jo baz_overflow_line_4
                    and r9, 65535
                    mov qword [rbp], r9
                    mov qword [rbp + 72], 9
                    mov r9, 6
                    add r9, r13
                    jo baz_overflow_line_5
                    and r9, 1023
                    and r9, 31
                    imul r9, qword [rbp + 72]
                    mov qword [rbp + 72], r9
                    jo baz_overflow_line_5
                    mov qword [rbp + 80], 0
                    loop.7.5.22.22.49.15:
                        inc qword [rbp + 80]
                        jo baz_overflow_line_8
                        cmp qword [rbp + 80], 3
                        jg loop.7.5.22.22.49.15.end
                        mov r9, qword [rbp]
                        xor r9, r13
                        imul r9, 31
                        jo baz_overflow_line_10
                        and r9, 65535
                        mov qword [rbp], r9
                    jmp loop.7.5.22.22.49.15
                    loop.7.5.22.22.49.15.end:
            if.19.9.49.15.end:
    xor r15, r14
    imul r15, 31
    jo baz_overflow_line_49
    and r15, 65535
    mov qword [rbp], r15
    mov rdi, qword [rbp]
    xor rdi, qword [rbp + 16]
    xor rdi, qword [rbp + 24]
    xor rdi, qword [rbp + 32]
    and rdi, 127
    jmp baz_exit
baz_overflow_line_4:
    mov rbp, 4
    jmp baz_overflow_panic_file_0
baz_overflow_line_5:
    mov rbp, 5
    jmp baz_overflow_panic_file_0
baz_overflow_line_8:
    mov rbp, 8
    jmp baz_overflow_panic_file_0
baz_overflow_line_10:
    mov rbp, 10
    jmp baz_overflow_panic_file_0
baz_overflow_line_16:
    mov rbp, 16
    jmp baz_overflow_panic_file_0
baz_overflow_line_17:
    mov rbp, 17
    jmp baz_overflow_panic_file_0
baz_overflow_line_19:
    mov rbp, 19
    jmp baz_overflow_panic_file_0
baz_overflow_line_20:
    mov rbp, 20
    jmp baz_overflow_panic_file_0
baz_overflow_line_30:
    mov rbp, 30
    jmp baz_overflow_panic_file_0
baz_overflow_line_33:
    mov rbp, 33
    jmp baz_overflow_panic_file_0
    mov rbp, 35
    jmp baz_overflow_panic_file_0
    mov rbp, 36
    jmp baz_overflow_panic_file_0
baz_overflow_line_37:
    mov rbp, 37
    jmp baz_overflow_panic_file_0
baz_overflow_line_38:
    mov rbp, 38
    jmp baz_overflow_panic_file_0
baz_overflow_line_41:
    mov rbp, 41
    jmp baz_overflow_panic_file_0
baz_overflow_line_42:
    mov rbp, 42
    jmp baz_overflow_panic_file_0
baz_overflow_line_43:
    mov rbp, 43
    jmp baz_overflow_panic_file_0
baz_overflow_line_45:
    mov rbp, 45
    jmp baz_overflow_panic_file_0
baz_overflow_line_49:
    mov rbp, 49
baz_overflow_panic_file_0:
baz_exit:
