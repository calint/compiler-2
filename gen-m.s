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
   je if_10_5_end
   if_10_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_10_16_end:
   if_10_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   not qword[rbp-24]
   if_12_8:
   cmp_12_8:
   cmp qword[rbp-24],1
   je if_12_5_end
   if_12_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_12_16_end:
   if_12_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-32],r15
   not qword[rbp-32]
   sub qword[rbp-32],-1
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-32],2
   je if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_14_16_end:
   if_14_5_end:
   mov qword[rbp-40],1
   mov r15,qword[rbp-8]
   not r15
   add r15,qword[rbp-40]
   mov qword[rbp-48],r15
   if_17_8:
   cmp_17_8:
   cmp qword[rbp-48],2
   je if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi,4
       mov rax,60
       syscall
     exit_17_16_end:
   if_17_5_end:
   mov qword[rbp-56],2
   mov r15,qword[rbp-56]
   mov r14,qword[rbp-8]
   not r14
   imul r15,r14
   mov qword[rbp-64],r15
   if_20_8:
   cmp_20_8:
   cmp qword[rbp-64],2
   je if_20_5_end
   if_20_8_code:  ; opt1
       mov rdi,5
       mov rax,60
       syscall
     exit_20_16_end:
   if_20_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_21_5_end:
