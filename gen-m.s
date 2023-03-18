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
   mov qword[rbp-8],-2
   mov r15,qword[rbp-8]
   neg r15
   mov qword[rbp-16],r15
   if_10_8:
   cmp_10_8:
     mov r15,qword[rbp-8]
     neg r15
     add r15,2
     mov r14,qword[rbp-16]
     neg r14
     add r14,6
   cmp r15,r14
   jne if_10_5_end
   jmp if_10_8_code
   if_10_8_code:
       mov rbx,0
       mov rax,1
       int 0x80
     exit_11_9_end:
   if_10_5_end:
     mov rbx,1
     mov rax,1
     int 0x80
   exit_12_5_end:
