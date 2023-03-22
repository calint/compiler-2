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
   cmp_12_8:
   cmp qword[rbp-8],0
   jne cmp_12_23
   cmp_12_16:
   cmp qword[rbp-16],2
   je if_12_8_code  ; opt2
   cmp_12_23:
   cmp qword[rbp-24],0
   jne if_12_5_end
   cmp_12_31:
   cmp qword[rbp-32],4
   jne if_12_5_end
   if_12_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_12_35_end:
   if_12_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_13_5_end:
