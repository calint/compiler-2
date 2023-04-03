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
   cmp_10_17:
   cmp byte[rbp-2],0
   jne false_10_9
   jmp true_10_9
   true_10_9:
   mov byte[rbp-3],1
   jmp end_10_9
   false_10_9:
   mov byte[rbp-3],0
   end_10_9:
   if_14_8:
   cmp_14_8:
   mov r15b,byte[rbp-2]
   cmp byte[rbp-1],r15b
   je if_14_5_end
   jmp if_14_8_code
   if_14_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_14_15_end:
   if_14_5_end:
   if_16_8:
   cmp_16_8:
   mov r15b,byte[rbp-3]
   cmp byte[rbp-1],r15b
   jne if_16_5_end
   jmp if_16_8_code
   if_16_8_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_16_19_end:
   if_16_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_18_5_end:
