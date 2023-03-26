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
f:
   push rbp
   mov rbp,rsp
   cmp_25_9:
   cmp byte[rbp+16],0
   jne false_25_5
   jmp true_25_5
   true_25_5:
   mov byte[rbp-1],1
   jmp end_25_5
   false_25_5:
   mov byte[rbp-1],0
   end_25_5:
   movsx rax,byte[rbp-1]
   pop rbp
   ret
main:
   mov byte[rbp-1],1
   mov byte[rbp-2],2
   cmp_5_17:
   mov r15b,byte[rbp-2]
   cmp byte[rbp-1],r15b
   jge false_5_9
   jmp true_5_9
   true_5_9:
   mov byte[rbp-3],1
   jmp end_5_9
   false_5_9:
   mov byte[rbp-3],0
   end_5_9:
   if_6_8:
   cmp_6_8:
   cmp byte[rbp-3],0
   jne if_6_5_end
   jmp if_6_8_code
   if_6_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_6_15_end:
   if_6_5_end:
   cmp_8_17:
   cmp byte[rbp-1],1
   jne false_8_9
   jmp true_8_9
   true_8_9:
   mov byte[rbp-4],1
   jmp end_8_9
   false_8_9:
   mov byte[rbp-4],0
   end_8_9:
   cmp_9_8:
   cmp byte[rbp-2],0
   jne false_9_5
   jmp true_9_5
   true_9_5:
   mov byte[rbp-4],1
   jmp end_9_5
   false_9_5:
   mov byte[rbp-4],0
   end_9_5:
   if_10_8:
   cmp_10_8:
   cmp byte[rbp-4],0
   je if_10_5_end
   jmp if_10_8_code
   if_10_8_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_10_11_end:
   if_10_5_end:
   mov byte[rbp-1],-1
   add byte[rbp-1],2
   if_13_8:
   cmp_13_8:
   cmp byte[rbp-1],1
   je if_13_5_end
   jmp if_13_8_code
   if_13_8_code:
       mov rdi,3
       mov rax,60
       syscall
     exit_13_18_end:
   if_13_5_end:
   if_15_8:
   cmp_15_8:
     sub rsp,4
     push false
     call f
     add rsp,12
     mov r15,rax
   cmp r15,0
   jne if_15_5_end
   jmp if_15_8_code
   if_15_8_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_15_21_end:
   if_15_5_end:
   mov qword[rbp-12],hello
     mov rdi,0
     mov rax,60
     syscall
   exit_21_5_end:
