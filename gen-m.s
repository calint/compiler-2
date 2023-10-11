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
mov rax, 60
mov rdi, 0
syscall
