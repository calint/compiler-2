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
   mov r15,qword[rbp-8]
   neg r15
   imul r15,qword[rbp-8]
   neg r15
   neg r15
   mov qword[rbp-16],r15
   if_10_8:
   cmp_10_8:
   cmp qword[rbp-16],-1
   jne if_10_5_end
   cmp_10_17:
   cmp qword[rbp-8],-1
   jne if_10_5_end
   if_10_8_code:  ; opt1
       mov rbx,0
       mov rax,1
       int 0x80
     exit_11_9_end:
   if_10_5_end:
     mov rbx,1
     mov rax,1
     int 0x80
   exit_12_5_end:
