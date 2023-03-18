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
   mov qword[rbp-8],2
     mov r14,qword[rbp-8]
     neg r14
     mov r15,r14
     neg r15
     imul r15,2
   f_13_11_end:
   mov qword[rbp-16],r15
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-16],4
   jne if_14_5_end
   jmp if_14_8_code
   if_14_8_code:
       mov rbx,0
       mov rax,1
       int 0x80
     exit_15_9_end:
   if_14_5_end:
     mov rbx,1
     mov rax,1
     int 0x80
   exit_16_5_end:
