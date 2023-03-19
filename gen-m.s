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
     mov rsi,name
     mov rax,0
     mov rdi,0
     syscall
     mov qword[rbp-24],rax
   read_29_13_end:
   sub qword[rbp-24],1
     mov rdx,qword[rbp-24]
     mov rcx,name
     mov rax,1
     mov rdi,1
     mov rsi,rcx
     syscall
   print_30_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_31_5_end:
