true equ 1
false equ 0
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
   mov r15,qword[rbp-16]
   add r15,qword[rbp-24]
   imul r15,qword[rbp-32]
   mov r14,qword[rbp-24]
   mov r13,qword[rbp-8]
   add r13,qword[rbp-16]
   imul r14,r13
   add r15,r14
   imul r15,2
   mov qword[rbp-40],r15
     mov rdi,qword[rbp-40]
     mov rax,60
     syscall
   exit_13_5_end:
