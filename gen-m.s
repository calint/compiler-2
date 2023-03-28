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
f:
   push rbp
   mov rbp,rsp
   cmp_8_9:
   mov r15b,byte[rbp+24]
   cmp byte[rbp+16],r15b
   jne false_8_5
   jmp true_8_5
   true_8_5:
   mov byte[rbp-1],1
   jmp end_8_5
   false_8_5:
   mov byte[rbp-1],0
   end_8_5:
   movsx rax,byte[rbp-1]
   pop rbp
   ret
main:
   mov byte[rbp-1],1
   mov byte[rbp-2],2
   cmp_18_16:
     sub rsp,3
     movsx r14,byte[rbp-2]
     push r14
     movsx r14,byte[rbp-1]
     push r14
     call f
     add rsp,19
     mov r15,rax
   cmp r15,0
   je false_18_9
   jmp true_18_9
   true_18_9:
   mov byte[rbp-3],1
   jmp end_18_9
   false_18_9:
   mov byte[rbp-3],0
   end_18_9:
     mov rdi,0
     mov rax,60
     syscall
   exit_20_5_end:
