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
         c_27_14_end:
         mov r15,r14
         add r15,2
       b_27_12_end:
       mov rbx,r15
       add rbx,1
     a_27_10_end:
     mov rax,1
     int 0x80
   exit_27_5_end:
