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
f:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   mov qword[rbp-8],r15
   add qword[rbp-8],1
   mov rax,qword[rbp-8]
   pop rbp
   ret
main:
   mov qword[rbp-8],1
   mov qword[rbp-16],2
   mov qword[rbp-24],3
   mov qword[rbp-32],4
   if_17_8:
   cmp_17_9:
   cmp qword[rbp-8],1
   jne cmp_17_26
   cmp_17_17:
   cmp qword[rbp-16],2
   je if_17_8_code  ; opt2
   cmp_17_26:
   cmp qword[rbp-24],3
   jne if_17_5_end
   cmp_17_34:
   cmp qword[rbp-32],4
   jne if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_18_9_end:
   if_17_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_19_5_end:
