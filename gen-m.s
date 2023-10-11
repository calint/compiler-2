true equ 1
false equ 0
section .data
align 4
section .bss
align 4
stk resd 1024
stk.end:
section .text
align 4
bits 64
global _start
_start:
mov rsp,stk.end
mov rbp,rsp
jmp main
assert:
   push rbp
   mov rbp, rsp
   if_8_8:
   cmp_8_8:
   cmp byte[rbp+16], 0
   jne if_8_5_end
   if_8_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_8_17_end:
   if_8_5_end:
   pop rbp
   ret
main:
   mov dword[rbp-4], 10
     movsx r15, dword[rbp-4]
     mov qword[rbp-19], r15
     mov r15d, dword[rbp-4]
     mov dword[rbp-11], r15d
     add dword[rbp-11], 1
     mov word[rbp-7], 12
     mov byte[rbp-5], 13
   sub rsp, 19
     cmp_18_12:
     cmp qword[rbp-19], 10
     jne false_18_12
     true_18_12:  ; opt1
     mov r15, 1
     jmp end_18_12
     false_18_12:
     mov r15, 0
     end_18_12:
   push r15
   call assert
   add rsp, 27
   sub rsp, 19
     cmp_19_12:
     cmp dword[rbp-11], 11
     jne false_19_12
     true_19_12:  ; opt1
     mov r15, 1
     jmp end_19_12
     false_19_12:
     mov r15, 0
     end_19_12:
   push r15
   call assert
   add rsp, 27
   sub rsp, 19
     cmp_20_12:
     cmp word[rbp-7], 12
     jne false_20_12
     true_20_12:  ; opt1
     mov r15, 1
     jmp end_20_12
     false_20_12:
     mov r15, 0
     end_20_12:
   push r15
   call assert
   add rsp, 27
   sub rsp, 19
     cmp_21_12:
     cmp byte[rbp-5], 13
     jne false_21_12
     true_21_12:  ; opt1
     mov r15, 1
     jmp end_21_12
     false_21_12:
     mov r15, 0
     end_21_12:
   push r15
   call assert
   add rsp, 27
       movsx r15, dword[rbp-4]
       mov qword[rbp-50], r15
       movsx r15, dword[rbp-4]
       add qword[rbp-50], r15
       mov dword[rbp-42], 2
       mov word[rbp-38], 3
       mov byte[rbp-36], 4
       mov qword[rbp-35], 5
       mov dword[rbp-27], 6
       mov word[rbp-23], 7
       mov byte[rbp-21], 8
     mov byte[rbp-20], true
   sub rsp, 50
     cmp_24_12:
     cmp qword[rbp-50], 20
     jne false_24_12
     true_24_12:  ; opt1
     mov r15, 1
     jmp end_24_12
     false_24_12:
     mov r15, 0
     end_24_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_25_12:
     cmp dword[rbp-42], 2
     jne false_25_12
     true_25_12:  ; opt1
     mov r15, 1
     jmp end_25_12
     false_25_12:
     mov r15, 0
     end_25_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_26_12:
     cmp word[rbp-38], 3
     jne false_26_12
     true_26_12:  ; opt1
     mov r15, 1
     jmp end_26_12
     false_26_12:
     mov r15, 0
     end_26_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_27_12:
     cmp byte[rbp-36], 4
     jne false_27_12
     true_27_12:  ; opt1
     mov r15, 1
     jmp end_27_12
     false_27_12:
     mov r15, 0
     end_27_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_28_12:
     cmp qword[rbp-35], 5
     jne false_28_12
     true_28_12:  ; opt1
     mov r15, 1
     jmp end_28_12
     false_28_12:
     mov r15, 0
     end_28_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_29_12:
     cmp dword[rbp-27], 6
     jne false_29_12
     true_29_12:  ; opt1
     mov r15, 1
     jmp end_29_12
     false_29_12:
     mov r15, 0
     end_29_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_30_12:
     cmp word[rbp-23], 7
     jne false_30_12
     true_30_12:  ; opt1
     mov r15, 1
     jmp end_30_12
     false_30_12:
     mov r15, 0
     end_30_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_31_12:
     cmp byte[rbp-21], 8
     jne false_31_12
     true_31_12:  ; opt1
     mov r15, 1
     jmp end_31_12
     false_31_12:
     mov r15, 0
     end_31_12:
   push r15
   call assert
   add rsp, 58
   sub rsp, 50
     cmp_32_12:
     cmp byte[rbp-20], true
     jne false_32_12
     true_32_12:  ; opt1
     mov r15, 1
     jmp end_32_12
     false_32_12:
     mov r15, 0
     end_32_12:
   push r15
   call assert
   add rsp, 58
mov rax, 60
mov rdi, 0
syscall
