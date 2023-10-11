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
main:
   mov qword[rbp-8], 2
   mov qword[rbp-16], 1
   mov r15, qword[rbp-8]
   neg r15
   not r15
   add qword[rbp-16], r15
   if_10_8:
   cmp_10_8:
   cmp qword[rbp-16], 2
   je if_10_5_end
   if_10_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_10_15_end:
   if_10_5_end:
   mov qword[rbp-24], 2
   mov r15, qword[rbp-24]
   mov qword[rbp-32], r15
   mov r15, qword[rbp-8]
   neg r15
   not r15
   or qword[rbp-32], r15
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-32], 3
   je if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi, 2
       mov rax, 60
       syscall
     exit_14_15_end:
   if_14_5_end:
   mov qword[rbp-40], 1
   mov rcx, qword[rbp-8]
   add rcx, qword[rbp-24]
   sal qword[rbp-40], cl
   if_17_8:
   cmp_17_8:
   cmp qword[rbp-40], 16
   je if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi, 3
       mov rax, 60
       syscall
     exit_17_16_end:
   if_17_5_end:
   mov qword[rbp-48], 1
   mov rcx, qword[rbp-8]
   neg rcx
   not rcx
   sal qword[rbp-48], cl
   if_21_8:
   cmp_21_8:
   cmp qword[rbp-48], 2
   je if_21_5_end
   if_21_8_code:  ; opt1
       mov rdi, 4
       mov rax, 60
       syscall
     exit_21_15_end:
   if_21_5_end:
   mov qword[rbp-56], 1
   mov r15, qword[rbp-8]
   add r15, qword[rbp-24]
   or qword[rbp-56], r15
   if_24_8:
   cmp_24_8:
   cmp qword[rbp-56], 6
   je if_24_5_end
   if_24_8_code:  ; opt1
       mov rdi, 5
       mov rax, 60
       syscall
     exit_24_15_end:
   if_24_5_end:
mov rax, 60
mov rdi, 0
syscall
