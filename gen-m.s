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
   cmp_19_16:
       if_8_8_19_16:
       cmp_8_8_19_16:
         mov r15,false
       if_8_5_19_16_end:
     f_19_16_end:
   cmp r15,0
   je false_19_9
   true_19_9:  ; opt1
   mov byte[rbp-1],1
   jmp end_19_9
   false_19_9:
   mov byte[rbp-1],0
   end_19_9:
   if_20_8:
   cmp_20_8:
   cmp byte[rbp-1],0
   je if_20_5_end
   if_20_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_20_10_end:
   if_20_5_end:
   cmp_22_7:
       if_8_8_22_7:
       cmp_8_8_22_7:
       if_else_8_5_22_7:  ; opt1
           mov r15,true
       if_8_5_22_7_end:
     f_22_7_end:
   cmp r15,0
   je false_22_5
   true_22_5:  ; opt1
   mov byte[rbp-1],1
   jmp end_22_5
   false_22_5:
   mov byte[rbp-1],0
   end_22_5:
   if_23_8:
   cmp_23_8:
   cmp byte[rbp-1],0
   jne if_23_5_end
   if_23_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_23_14_end:
   if_23_5_end:
   if_25_8:
   cmp_25_8:
   if_25_5_end:  ; opt1
   if_27_8:
   cmp_27_8:
     mov byte[rbp-1],true
   if_27_5_end:
   if_29_8:
   cmp_29_8:
   cmp byte[rbp-1],0
   jne if_29_5_end
   if_29_8_code:  ; opt1
       mov rdi,4
       mov rax,60
       syscall
     exit_29_14_end:
   if_29_5_end:
   if_31_8:
   cmp_31_8:
   if_else_31_5:  ; opt1
       mov byte[rbp-1],false
   if_31_5_end:
   if_33_8:
   cmp_33_8:
   cmp byte[rbp-1],0
   je if_33_5_end
   if_33_8_code:  ; opt1
       mov rdi,5
       mov rax,60
       syscall
     exit_33_10_end:
   if_33_5_end:
   if_35_8:
   cmp_35_8:
       if_12_8_35_12:
       cmp_12_8_35_12:
         mov r15,4
       if_12_5_35_12_end:
     f2_35_12_end:
   cmp r15,4
   je if_35_5_end
   if_35_8_code:  ; opt1
       mov rdi,6
       mov rax,60
       syscall
     exit_35_20_end:
   if_35_5_end:
   if_36_8:
   cmp_36_8:
       if_12_8_36_12:
       cmp_12_8_36_12:
       if_13_13_36_12:  ; opt1
       cmp_13_13_36_12:
         mov r15,5
       if_12_5_36_12_end:
     f2_36_12_end:
   cmp r15,5
   je if_36_5_end
   if_36_8_code:  ; opt1
       mov rdi,7
       mov rax,60
       syscall
     exit_36_20_end:
   if_36_5_end:
   if_37_8:
   cmp_37_8:
       if_12_8_37_12:
       cmp_12_8_37_12:
       if_13_13_37_12:  ; opt1
       cmp_13_13_37_12:
       if_14_13_37_12:  ; opt1
       cmp_14_13_37_12:
         mov r15,6
       if_12_5_37_12_end:
     f2_37_12_end:
   cmp r15,6
   je if_37_5_end
   if_37_8_code:  ; opt1
       mov rdi,8
       mov rax,60
       syscall
     exit_37_20_end:
   if_37_5_end:
   if_38_8:
   cmp_38_8:
       if_12_8_38_12:
       cmp_12_8_38_12:
       if_13_13_38_12:  ; opt1
       cmp_13_13_38_12:
       if_14_13_38_12:  ; opt1
       cmp_14_13_38_12:
       if_else_12_5_38_12:  ; opt1
           mov r15,7
       if_12_5_38_12_end:
     f2_38_12_end:
   cmp r15,7
   je if_38_5_end
   if_38_8_code:  ; opt1
       mov rdi,9
       mov rax,60
       syscall
     exit_38_20_end:
   if_38_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_41_5_end:
