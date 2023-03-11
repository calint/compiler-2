section .data
section .bss
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
   mov qword[rbp-16],1
   mov qword[rbp-24],3
   mov qword[rbp-32],3
   if_12_8:
   cmp_12_8:
   mov r15,qword[rbp-16]
   cmp qword[rbp-8],r15
   jne if_12_5_end
   cmp_12_16:
   mov r15,qword[rbp-32]
   cmp qword[rbp-24],r15
   jne if_12_5_end
   jmp if_12_8_code
   if_12_8_code:
       mov rbx,0
       mov rax,1
       int 0x80
     exit_13_9_end:
   if_12_5_end:
     mov rbx,1
     mov rax,1
     int 0x80
   exit_14_5_end:
