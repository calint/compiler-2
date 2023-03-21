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
   mov qword[rbp-8],2
   mov qword[rbp-16],~2
   mov r15,qword[rbp-8]
   and r15,qword[rbp-16]
   mov qword[rbp-24],r15
   if_11_8:
   cmp_11_8:
   cmp qword[rbp-24],0
   je if_11_5_end
   if_11_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_11_16_end:
   if_11_5_end:
   mov r15,qword[rbp-8]
   or r15,qword[rbp-16]
   mov qword[rbp-32],r15
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-32],0xffffffffffffffff
   je if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_14_33_end:
   if_14_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-40],r15
   mov r15,qword[rbp-8]
   xor r15,qword[rbp-40]
   mov qword[rbp-48],r15
   if_18_8:
   cmp_18_8:
   cmp qword[rbp-48],0
   je if_18_5_end
   if_18_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_18_16_end:
   if_18_5_end:
   mov qword[rbp-56],1
   mov r15,2
   imul r15,qword[rbp-8]
   and r15,2
   add qword[rbp-56],r15
   if_21_8:
   cmp_21_8:
   cmp qword[rbp-56],1
   je if_21_5_end
   if_21_8_code:  ; opt1
       mov rdi,4
       mov rax,60
       syscall
     exit_21_16_end:
   if_21_5_end:
   mov qword[rbp-64],1
   mov qword[rbp-72],1
   mov r15,qword[rbp-64]
   not r15
   or r15,qword[rbp-72]
   mov qword[rbp-80],r15
   if_26_8:
   cmp_26_8:
   cmp qword[rbp-80],0xffffffffffffffff
   je if_26_5_end
   if_26_8_code:  ; opt1
       mov rdi,5
       mov rax,60
       syscall
     exit_26_33_end:
   if_26_5_end:
   mov qword[rbp-88],1
   mov r15,2
   mov r14,qword[rbp-64]
   and r14,1
   imul r15,r14
   add qword[rbp-88],r15
   if_29_8:
   cmp_29_8:
   cmp qword[rbp-88],3
   je if_29_5_end
   if_29_8_code:  ; opt1
       mov rdi,6
       mov rax,60
       syscall
     exit_29_16_end:
   if_29_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_31_5_end:
