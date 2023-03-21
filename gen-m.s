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
   mov qword[rbp-8],-2
   mov r15,qword[rbp-8]
   mov qword[rbp-16],r15
   neg qword[rbp-16]
   if_11_8:
   cmp_11_8:
   cmp qword[rbp-8],-1
   jne if_11_8_code
   cmp_11_20:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_11_5_end
   if_11_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_11_29_end:
   if_11_5_end:
   if_12_8:
   cmp_12_13:
   cmp qword[rbp-8],-1
   jne if_12_5_end
   cmp_12_22:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   jne if_12_5_end
   if_12_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_12_28_end:
   if_12_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_14_5_end:
