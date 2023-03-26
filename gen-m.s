true equ 1
false equ 0
section .data
align 4
hello: db 'hello world'
hello.len equ $-hello
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
   mov byte[rbp-1],1
   mov byte[rbp-2],2
   cmp_13_17:
   mov r15b,byte[rbp-2]
   cmp byte[rbp-1],r15b
   jge false_13_9
   jmp true_13_9
   true_13_9:
   mov byte[rbp-3],1
   jmp end_13_9
   false_13_9:
   mov byte[rbp-3],0
   end_13_9:
   if_14_8:
   cmp_14_8:
   cmp byte[rbp-3],0
   jne if_14_5_end
   jmp if_14_8_code
   if_14_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_14_15_end:
   if_14_5_end:
   cmp_16_17:
   cmp byte[rbp-1],1
   jne false_16_9
   jmp true_16_9
   true_16_9:
   mov byte[rbp-4],1
   jmp end_16_9
   false_16_9:
   mov byte[rbp-4],0
   end_16_9:
   cmp_17_8:
   cmp byte[rbp-2],0
   jne false_17_5
   jmp true_17_5
   true_17_5:
   mov byte[rbp-4],1
   jmp end_17_5
   false_17_5:
   mov byte[rbp-4],0
   end_17_5:
   if_18_8:
   cmp_18_8:
   cmp byte[rbp-4],0
   je if_18_5_end
   jmp if_18_8_code
   if_18_8_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_18_11_end:
   if_18_5_end:
   mov byte[rbp-1],-1
   add byte[rbp-1],2
   if_21_8:
   cmp_21_8:
   cmp byte[rbp-1],1
   je if_21_5_end
   jmp if_21_8_code
   if_21_8_code:
       mov rdi,3
       mov rax,60
       syscall
     exit_21_18_end:
   if_21_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_23_5_end:
