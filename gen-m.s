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
   mov qword[rbp-8],1
   mov qword[rbp-16],2
   mov qword[rbp-24],3
   mov qword[rbp-32],4
   if_12_8:
   cmp_12_9:
   cmp qword[rbp-8],1
   jne cmp_12_34
   cmp_12_18:
   cmp qword[rbp-16],2
   je if_12_8_code
   cmp_12_25:
   cmp qword[rbp-24],3
   je if_12_8_code  ; opt2
   cmp_12_34:
   cmp qword[rbp-32],4
   jne if_12_5_end
   if_12_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_13_9_end:
   if_12_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_14_5_end:
