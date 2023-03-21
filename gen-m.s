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
   mov qword[rbp-8],4
   mov qword[rbp-16],2
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   xor rdx,rdx
   mov rax,qword[rbp-24]
   idiv qword[rbp-16]
   mov qword[rbp-24],rax
   if_11_8:
   cmp_11_8:
   cmp qword[rbp-24],2
   je if_11_5_end
   if_11_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_11_16_end:
   if_11_5_end:
   mov qword[rbp-8],3
   if_14_8:
   cmp_14_8:
     mov r15,qword[rbp-8]
     xor rdx,rdx
     mov rax,r15
     mov r14,2
     idiv r14
     mov r15,rdx
   cmp r15,1
   je if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_14_18_end:
   if_14_5_end:
   mov qword[rbp-24],1
   mov r15,3
   xor rdx,rdx
   mov rax,r15
   mov r14,2
   idiv r14
   mov r15,rdx
   add r15,1
   add qword[rbp-24],r15
   if_17_8:
   cmp_17_8:
   cmp qword[rbp-24],3
   je if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_17_16_end:
   if_17_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_22_5_end:
