true equ 1
false equ 0
section .data
align 4
fld: dq 42
str: db 'hello world'
str.len equ $-str
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
   mov qword[rbp-40],1
   if_15_5:
   cmp_15_5:
   cmp qword[rbp-40],1
   je if_15_2_end
   jmp if_15_5_code
   if_15_5_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_15_21_end:
   if_15_2_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_16_2_end:
