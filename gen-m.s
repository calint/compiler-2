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
   mov qword[rbp-8],-1
   mov qword[rbp-16],-2
   mov qword[rbp-24],1
   mov r15,qword[rbp-8]
   neg r15
   imul r15,qword[rbp-16]
   neg r15
   mov r14,3
   imul r14,qword[rbp-8]
   neg r14
   add r15,r14
   add qword[rbp-24],r15
   if_11_8:
   cmp_11_8:
   cmp qword[rbp-24],6
   jne if_11_5_end
   jmp if_11_8_code
   if_11_8_code:
       mov rbx,0
       mov rax,1
       int 0x80
     exit_12_9_end:
   if_11_5_end:
     mov rbx,1
     mov rax,1
     int 0x80
   exit_13_5_end:
