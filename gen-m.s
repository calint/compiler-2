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
   mov qword[rbp-8],0xfffffffffffffffe
   mov qword[rbp-16],2
   mov r15,qword[rbp-8]
   not r15
   imul r15,qword[rbp-16]
   mov qword[rbp-16],r15
   if_10_8:
   cmp_10_8:
   cmp qword[rbp-16],2
   jne if_10_5_end
   if_10_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_11_9_end:
   if_10_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_12_5_end:
