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
   if_9_8:
   cmp_9_8:
     mov r15,2
     neg r15
     neg r15
   cmp qword[rbp-8],r15
   jne if_9_5_end
   if_9_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_10_9_end:
   if_9_5_end:
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
   if_15_8:
   cmp_15_8:
   cmp qword[rbp-8],-2
   jne if_15_5_end
   cmp_15_17:
   cmp qword[rbp-16],2
   jne if_15_5_end
   cmp_15_25:
   cmp qword[rbp-24],-2
   jne if_15_5_end
   cmp_15_34:
   cmp qword[rbp-32],-1
   jne if_15_5_end
   cmp_15_43:
     mov r15,2
     neg r15
     neg r15
   cmp qword[rbp-40],r15
   jne if_15_5_end
   if_15_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_16_9_end:
   if_15_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_17_5_end:
