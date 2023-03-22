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
   if_12_8:
   cmp_12_8:
   cmp qword[rbp-8],0
   jne if_12_5_end
   if_12_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_12_14_end:
   if_12_5_end:
   if_13_8:
   cmp_13_13:
   cmp qword[rbp-8],0
   je if_13_8_code
   cmp_13_19:
   cmp qword[rbp-16],0
   jne if_13_5_end
   if_13_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_13_22_end:
   if_13_5_end:
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-8],0
   je if_14_5_end
   cmp_14_14:
   cmp qword[rbp-16],0
   je if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_14_16_end:
   if_14_5_end:
     mov rdi,3
     mov rax,60
     syscall
   exit_15_5_end:
