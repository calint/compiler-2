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
   true_12_9:  ; opt1
   mov byte[rbp-1],1
   jmp end_12_9
   false_12_9:
   mov byte[rbp-1],0
   end_12_9:
   if_13_8:
   cmp_13_8:
   cmp byte[rbp-1],0
   je if_13_5_end
   if_13_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_13_10_end:
   if_13_5_end:
   cmp_15_7:
       if_8_8_15_7:
       cmp_8_8_15_7:
       if_else_8_5_15_7:  ; opt1
           mov r15,true
       if_8_5_15_7_end:
     f_15_7_end:
   cmp r15,0
   je false_15_5
   true_15_5:  ; opt1
   mov byte[rbp-1],1
   jmp end_15_5
   false_15_5:
   mov byte[rbp-1],0
   end_15_5:
   if_16_8:
   cmp_16_8:
   cmp byte[rbp-1],0
   jne if_16_5_end
   if_16_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_16_14_end:
   if_16_5_end:
   if_18_8:
   cmp_18_8:
   if_18_5_end:  ; opt1
   if_20_8:
   cmp_20_8:
     mov byte[rbp-1],true
   if_20_5_end:
   if_22_8:
   cmp_22_8:
   cmp byte[rbp-1],0
   jne if_22_5_end
   if_22_8_code:  ; opt1
       mov rdi,4
       mov rax,60
       syscall
     exit_22_14_end:
   if_22_5_end:
   if_24_8:
   cmp_24_8:
   if_else_24_5:  ; opt1
       mov byte[rbp-1],false
   if_24_5_end:
   if_26_8:
   cmp_26_8:
   cmp byte[rbp-1],0
   je if_26_5_end
   if_26_8_code:  ; opt1
       mov rdi,5
       mov rax,60
       syscall
     exit_26_10_end:
   if_26_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_28_5_end:
