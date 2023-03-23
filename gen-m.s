section .data
align 4
len dq -2
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
     mov rdx,qword[rbp-8]
     neg rdx
     mov qword[rbp-16],rdx
     neg qword[rbp-16]
   foo_19_13_end:
   neg qword[rbp-16]
   not qword[rbp-16]
   mov r15,qword[len]
   mov qword[rbp-24],r15
   neg qword[rbp-24]
   not qword[rbp-24]
     mov r15,qword[len]
     mov qword[rbp-32],r15
     neg qword[rbp-32]
   bar_21_12_end:
   not qword[rbp-32]
   if_22_8:
   cmp_22_8:
   mov r15,qword[len]
   neg r15
   not r15
   cmp qword[rbp-32],r15
   jne if_22_5_end
   cmp_22_20:
   mov r15,qword[rbp-16]
   cmp qword[rbp-8],r15
   je if_22_5_end
   cmp_22_32:
   cmp qword[rbp-8],-1
   jne if_22_5_end
   cmp_22_41:
   cmp qword[rbp-16],~1
   jne if_22_5_end
   cmp_22_50:
   cmp qword[rbp-24],~2
   jne if_22_5_end
   if_22_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_23_9_end:
   if_22_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_24_5_end:
