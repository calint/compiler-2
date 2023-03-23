section .data
align 4
a db '....'
a.len equ $-a
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
           mov qword[rbp-32],1
           mov qword[rbp-40],2
           mov r13,qword[rbp-32]
           add r13,qword[rbp-40]
           mov qword[rbp-48],r13
           mov r14,1
           add r14,qword[rbp-48]
         c_28_14_end:
         mov r15,r14
         add r15,2
       b_28_12_end:
       mov rdi,r15
       add rdi,1
     a_28_10_end:
     mov rax,60
     syscall
   exit_28_5_end:
