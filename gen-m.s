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
   cmp_12_16:
       if_8_8_12_16:
       cmp_8_8_12_16:
         mov r15,false
       if_8_5_12_16_end:
     f_12_16_end:
   cmp r15,0
   je false_12_9
   jmp true_12_9
   true_12_9:
   mov byte[rbp-1],1
   jmp end_12_9
   false_12_9:
   mov byte[rbp-1],0
   end_12_9:
   if_13_8:
   cmp_13_8:
   cmp byte[rbp-1],false
   jne if_13_5_end
   jmp if_13_8_code
   if_13_8_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_14_9_end:
   if_13_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_16_5_end:
