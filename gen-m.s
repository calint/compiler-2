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
foo:
   push rbp
   mov rbp,rsp
   if_8_8:
   cmp_8_8:
   cmp byte[rbp+16],0
   je if_else_8_5
   jmp if_8_8_code
   if_8_8_code:
     mov byte[rbp-1],false
   jmp if_8_5_end
   if_else_8_5:
       mov byte[rbp-1],true
   if_8_5_end:
   movsx rax,byte[rbp-1]
   pop rbp
   ret
main:
   mov byte[rbp-1],1
   mov byte[rbp-2],2
   cmp_15_17:
   mov r15b,byte[rbp-2]
   cmp byte[rbp-1],r15b
   jge false_15_9
   jmp true_15_9
   true_15_9:
   mov byte[rbp-3],1
   jmp end_15_9
   false_15_9:
   mov byte[rbp-3],0
   end_15_9:
   cmp_17_17:
     sub rsp,4
     movsx r14,byte[rbp-3]
     push r14
     call foo
     add rsp,12
     mov r15,rax
   cmp r15,0
   je false_17_9
   jmp true_17_9
   true_17_9:
   mov byte[rbp-4],1
   jmp end_17_9
   false_17_9:
   mov byte[rbp-4],0
   end_17_9:
   if_18_8:
   cmp_18_8:
   cmp byte[rbp-4],0
   je if_18_5_end
   jmp if_18_8_code
   if_18_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_18_11_end:
   if_18_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_26_5_end:
