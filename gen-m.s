section .data
align 4
x dq 1
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
   mov qword[x],2
     mov rbx,qword[x]
     mov rax,1
     int 0x80
   exit_11_5_end:
