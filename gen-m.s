section .data
align 4
len dq 2
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
   mov qword[rbp-8],-1
   neg qword[rbp-8]
   mov r15,qword[len]
   mov qword[rbp-16],r15
   neg qword[rbp-16]
   neg qword[rbp-16]
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-8],1
   jne if_14_5_end
   cmp_14_16:
   cmp qword[rbp-16],2
   jne if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_15_9_end:
   if_14_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_16_5_end:
