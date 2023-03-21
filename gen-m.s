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
   mov qword[rbp-8],-2
   mov r15,qword[rbp-8]
   mov qword[rbp-16],r15
   neg qword[rbp-16]
   if_11_8:
   cmp_11_13:
   cmp qword[rbp-8],-2
   jne if_11_8_code
   cmp_11_22:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_11_5_end
   if_11_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_11_28_end:
   if_11_5_end:
   if_12_8:
   cmp_12_8:
   cmp qword[rbp-8],-2
   jne if_12_8_code
   cmp_12_20:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_12_5_end
   if_12_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_12_29_end:
   if_12_5_end:
   if_13_8:
   cmp_13_13:
   cmp qword[rbp-8],-2
   je if_13_5_end
   cmp_13_21:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_13_5_end
   if_13_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_13_27_end:
   if_13_5_end:
   if_14_8:
   cmp_14_8:
   cmp qword[rbp-8],-2
   je if_14_5_end
   cmp_14_21:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_14_5_end
   if_14_8_code:  ; opt1
       mov rdi,4
       mov rax,60
       syscall
     exit_14_30_end:
   if_14_5_end:
   if_15_8:
   cmp_15_13:
   cmp qword[rbp-8],-2
   jne if_15_8_code
   cmp_15_27:
   cmp qword[rbp-8],-1
   jne if_15_5_end
   cmp_15_36:
   cmp qword[rbp-16],-1
   jne if_15_5_end
   if_15_8_code:  ; opt1
       mov rdi,5
       mov rax,60
       syscall
     exit_15_43_end:
   if_15_5_end:
   if_16_8:
   cmp_16_13:
   cmp qword[rbp-8],-2
   jne if_16_8_code
   cmp_16_27:
   cmp qword[rbp-8],-1
   jne if_16_5_end
   cmp_16_36:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   jne if_16_5_end
   if_16_8_code:  ; opt1
       mov rdi,6
       mov rax,60
       syscall
     exit_16_43_end:
   if_16_5_end:
   if_17_8:
   cmp_17_13:
   cmp qword[rbp-8],-2
   je if_17_5_end
   cmp_17_26:
   cmp qword[rbp-8],-2
   je if_17_8_code
   cmp_17_34:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   jne if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi,6
       mov rax,60
       syscall
     exit_17_41_end:
   if_17_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_18_5_end:
