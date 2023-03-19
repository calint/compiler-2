section .data
align 4
name db '............................................................'
name.len equ $-name
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
   mov qword[rbp-8],-1
   mov r15,qword[rbp-8]
   neg r15
   imul r15,qword[rbp-8]
   neg r15
   neg r15
   mov qword[rbp-16],r15
     mov rdx,name.len
     mov rcx,name
     mov rax,3
     mov rbx,0
     int 0x80
     mov qword[rbp-24],rax
   read_29_13_end:
   sub qword[rbp-24],1
     mov rdx,qword[rbp-24]
     mov rcx,name
     mov rax,4
     mov rbx,1
     int 0x80
   print_30_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_31_5_end:
