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
   mov byte[rbp-1],true
   mov qword[rbp-9],1
   mov r15b,byte[rbp-1]
   mov byte[rbp-10],r15b
   cmp_16_16:
     sub rsp,11
       cmp_16_18:
       cmp byte[rbp-1],0
       jne false_16_18
       jmp true_16_18
       true_16_18:
       mov r14,1
       jmp end_16_18
       false_16_18:
       mov r14,0
       end_16_18:
     push r14
     call f
     add rsp,19
     mov r15,rax
   cmp r15,0
   je false_16_9
   jmp true_16_9
   true_16_9:
   mov byte[rbp-11],1
   jmp end_16_9
   false_16_9:
   mov byte[rbp-11],0
   end_16_9:
   if_17_8:
   cmp_17_8:
   cmp byte[rbp-11],0
   jne if_17_5_end
   jmp if_17_8_code
   if_17_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_17_14_end:
   if_17_5_end:
   cmp_19_7:
     sub rsp,11
     movsx r14,byte[rbp-1]
     push r14
     call f
     add rsp,19
     mov r15,rax
   cmp r15,0
   je false_19_5
   jmp true_19_5
   true_19_5:
   mov byte[rbp-11],1
   jmp end_19_5
   false_19_5:
   mov byte[rbp-11],0
   end_19_5:
   if_20_8:
   cmp_20_8:
   cmp byte[rbp-11],0
   je if_20_5_end
   jmp if_20_8_code
   if_20_8_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_20_10_end:
   if_20_5_end:
   cmp_22_7:
     sub rsp,11
       cmp_22_9:
       cmp qword[rbp-9],0
       je false_22_9
       jmp true_22_9
       true_22_9:
       mov r14,1
       jmp end_22_9
       false_22_9:
       mov r14,0
       end_22_9:
     push r14
     call f
     add rsp,19
     mov r15,rax
   cmp r15,0
   je false_22_5
   jmp true_22_5
   true_22_5:
   mov byte[rbp-11],1
   jmp end_22_5
   false_22_5:
   mov byte[rbp-11],0
   end_22_5:
   if_23_8:
   cmp_23_8:
   cmp byte[rbp-11],0
   je if_23_5_end
   jmp if_23_8_code
   if_23_8_code:
       mov rdi,3
       mov rax,60
       syscall
     exit_23_10_end:
   if_23_5_end:
   cmp_25_7:
     sub rsp,11
     push true
     call f
     add rsp,19
     mov r15,rax
   cmp r15,0
   je false_25_5
   jmp true_25_5
   true_25_5:
   mov byte[rbp-11],1
   jmp end_25_5
   false_25_5:
   mov byte[rbp-11],0
   end_25_5:
   if_26_8:
   cmp_26_8:
   cmp byte[rbp-11],0
   je if_26_5_end
   jmp if_26_8_code
   if_26_8_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_26_10_end:
   if_26_5_end:
   cmp_28_7:
     sub rsp,11
     push false
     call f
     add rsp,19
     mov r15,rax
   cmp r15,0
   je false_28_5
   jmp true_28_5
   true_28_5:
   mov byte[rbp-11],1
   jmp end_28_5
   false_28_5:
   mov byte[rbp-11],0
   end_28_5:
   if_29_8:
   cmp_29_8:
   cmp byte[rbp-11],0
   jne if_29_5_end
   jmp if_29_8_code
   if_29_8_code:
       mov rdi,5
       mov rax,60
       syscall
     exit_29_14_end:
   if_29_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_31_5_end:
