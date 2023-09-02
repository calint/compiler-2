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
   mov r15b,byte[rbp-1]
   mov byte[rbp-2],r15b
     mov rdi,0
     mov rax,60
     syscall
   exit_11_5_end:
