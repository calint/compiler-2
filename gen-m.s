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
   mov qword[rbp-9],1
   cmp_15_16:
       cmp_15_18:
       cmp byte[rbp-1],0
       jne false_15_18
       jmp true_15_18
       true_15_18:
       mov r14,1
       jmp end_15_18
       false_15_18:
       mov r14,0
       end_15_18:
       if_8_8_15_16:
       cmp_8_8_15_16:
       cmp r14,0
       je if_else_8_5_15_16
       jmp if_8_8_15_16_code
       if_8_8_15_16_code:
         mov r15,false
       jmp if_8_5_15_16_end
       if_else_8_5_15_16:
           mov r15,true
       if_8_5_15_16_end:
     f_15_16_end:
   cmp r15,0
   je false_15_9
   jmp true_15_9
   true_15_9:
   mov byte[rbp-10],1
   jmp end_15_9
   false_15_9:
   mov byte[rbp-10],0
   end_15_9:
   if_16_8:
   cmp_16_8:
   cmp byte[rbp-10],0
   jne if_16_5_end
   jmp if_16_8_code
   if_16_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_16_14_end:
   if_16_5_end:
   cmp_18_7:
       cmp_18_9:
       cmp byte[rbp-1],0
       je false_18_9
       jmp true_18_9
       true_18_9:
       mov r14,1
       jmp end_18_9
       false_18_9:
       mov r14,0
       end_18_9:
       if_8_8_18_7:
       cmp_8_8_18_7:
       cmp r14,0
       je if_else_8_5_18_7
       jmp if_8_8_18_7_code
       if_8_8_18_7_code:
         mov r15,false
       jmp if_8_5_18_7_end
       if_else_8_5_18_7:
           mov r15,true
       if_8_5_18_7_end:
     f_18_7_end:
   cmp r15,0
   je false_18_5
   jmp true_18_5
   true_18_5:
   mov byte[rbp-10],1
   jmp end_18_5
   false_18_5:
   mov byte[rbp-10],0
   end_18_5:
   if_19_8:
   cmp_19_8:
   cmp byte[rbp-10],0
   je if_19_5_end
   jmp if_19_8_code
   if_19_8_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_19_10_end:
   if_19_5_end:
   cmp_21_7:
       cmp_21_9:
       cmp qword[rbp-9],0
       je false_21_9
       jmp true_21_9
       true_21_9:
       mov r14,1
       jmp end_21_9
       false_21_9:
       mov r14,0
       end_21_9:
       if_8_8_21_7:
       cmp_8_8_21_7:
       cmp r14,0
       je if_else_8_5_21_7
       jmp if_8_8_21_7_code
       if_8_8_21_7_code:
         mov r15,false
       jmp if_8_5_21_7_end
       if_else_8_5_21_7:
           mov r15,true
       if_8_5_21_7_end:
     f_21_7_end:
   cmp r15,0
   je false_21_5
   jmp true_21_5
   true_21_5:
   mov byte[rbp-10],1
   jmp end_21_5
   false_21_5:
   mov byte[rbp-10],0
   end_21_5:
   if_22_8:
   cmp_22_8:
   cmp byte[rbp-10],0
   je if_22_5_end
   jmp if_22_8_code
   if_22_8_code:
       mov rdi,3
       mov rax,60
       syscall
     exit_22_10_end:
   if_22_5_end:
   cmp_24_7:
       if_8_8_24_7:
       cmp_8_8_24_7:
       cmp true,0
       je if_else_8_5_24_7
       jmp if_8_8_24_7_code
       if_8_8_24_7_code:
         mov r15,false
       jmp if_8_5_24_7_end
       if_else_8_5_24_7:
           mov r15,true
       if_8_5_24_7_end:
     f_24_7_end:
   cmp r15,0
   je false_24_5
   jmp true_24_5
   true_24_5:
   mov byte[rbp-10],1
   jmp end_24_5
   false_24_5:
   mov byte[rbp-10],0
   end_24_5:
   if_25_8:
   cmp_25_8:
   cmp byte[rbp-10],0
   je if_25_5_end
   jmp if_25_8_code
   if_25_8_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_25_10_end:
   if_25_5_end:
   cmp_27_7:
       if_8_8_27_7:
       cmp_8_8_27_7:
       cmp false,0
       je if_else_8_5_27_7
       jmp if_8_8_27_7_code
       if_8_8_27_7_code:
         mov r15,false
       jmp if_8_5_27_7_end
       if_else_8_5_27_7:
           mov r15,true
       if_8_5_27_7_end:
     f_27_7_end:
   cmp r15,0
   je false_27_5
   jmp true_27_5
   true_27_5:
   mov byte[rbp-10],1
   jmp end_27_5
   false_27_5:
   mov byte[rbp-10],0
   end_27_5:
   if_28_8:
   cmp_28_8:
   cmp byte[rbp-10],0
   jne if_28_5_end
   jmp if_28_8_code
   if_28_8_code:
       mov rdi,5
       mov rax,60
       syscall
     exit_28_14_end:
   if_28_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_30_5_end:
