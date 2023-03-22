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
   mov qword[rbp-8],1
   mov qword[rbp-16],2
   mov qword[rbp-24],3
   mov qword[rbp-32],4
   if_13_8:
   cmp_13_8:
   cmp qword[rbp-8],0
   je if_13_8_code
   cmp_13_15:
   cmp qword[rbp-16],1
   jne if_13_5_end
   cmp_13_23:
   cmp qword[rbp-24],3
   je if_13_8_code
   cmp_13_30:
   cmp qword[rbp-32],4
   jne if_13_5_end
   if_13_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_13_34_end:
   if_13_5_end:
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-8],0
   je if_14_8_code
   cmp_14_16:
   cmp qword[rbp-16],1
   jne cmp_14_32
   cmp_14_24:
   cmp qword[rbp-24],3
   je if_14_8_code  ; opt2
   cmp_14_32:
   cmp qword[rbp-32],4
   jne if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_14_36_end:
   if_14_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_16_5_end:
