section .data
section .bss
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
   mov qword[rbp-8],0b1
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
     mov rbx,qword[rbp-40]
     mov rax,1
     int 0x80
   exit_13_5_end:
