section .data
align 4
section .bss
align 4
stk resd 256
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
   add qword[rbp-8],2
   if_9_8:
   cmp_9_8:
   cmp qword[rbp-8],3
   jne if_9_5_end
   jmp if_9_8_code
   if_9_8_code:
       mov rbx,0
       mov rax,1
       int 0x80
     exit_10_9_end:
   if_9_5_end:
     mov rbx,1
     mov rax,1
     int 0x80
   exit_11_5_end:
