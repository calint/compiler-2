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
     mov rdx,name.len
     mov rsi,name
     mov rax,3
     mov rbx,0
     mov rcx,rsi
     int 0x80
     mov qword[rbp-8],rax
   read_27_13_end:
   sub qword[rbp-8],1
     mov rdx,qword[rbp-8]
     mov rcx,name
     mov rax,4
     mov rbx,1
     int 0x80
   print_28_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_29_5_end:
