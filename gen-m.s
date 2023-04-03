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
   mov byte[rbp-1],true
   mov byte[rbp-2],true
   mov byte[rbp-3],false
   if_12_8:
   cmp_12_8:
   mov r15b,byte[rbp-2]
   cmp byte[rbp-1],r15b
   je if_12_5_end
   jmp if_12_8_code
   if_12_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_12_15_end:
   if_12_5_end:
   if_14_8:
   cmp_14_8:
   mov r15b,byte[rbp-3]
   cmp byte[rbp-1],r15b
   jne if_14_5_end
   jmp if_14_8_code
   if_14_8_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_14_19_end:
   if_14_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_16_5_end:
