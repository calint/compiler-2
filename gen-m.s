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
   mov r15,qword[rbp-8]
   neg r15
   sub r15,qword[rbp-16]
   add r15,qword[rbp-8]
   mov qword[rbp-24],r15
   mov qword[rbp-32],1
   mov r15,qword[rbp-8]
   neg r15
   mov r14,qword[rbp-16]
   neg r14
   imul r15,r14
   add r15,2
   add qword[rbp-32],r15
   mov r15,qword[rbp-8]
   mov qword[rbp-40],r15
   neg qword[rbp-40]
   neg qword[rbp-40]
   neg qword[rbp-40]
   mov r15,qword[rbp-8]
   mov qword[rbp-48],r15
   neg qword[rbp-48]
   neg qword[rbp-48]
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-8],-2
   jne if_14_5_end
   cmp_14_17:
   cmp qword[rbp-16],2
   jne if_14_5_end
   cmp_14_25:
   cmp qword[rbp-24],-2
   jne if_14_5_end
   cmp_14_34:
   cmp qword[rbp-32],-1
   jne if_14_5_end
   cmp_14_43:
   cmp qword[rbp-40],2
   jne if_14_5_end
   cmp_14_51:
   cmp qword[rbp-48],-2
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
