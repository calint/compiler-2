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
   mov qword[rbp-8],4
   mov qword[rbp-16],2
   mov qword[rbp-24],0
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   neg qword[rbp-24]
   mov r15,qword[rbp-16]
   add r15,1
   neg r15
   add r15,1
   neg r15
   mov rax,qword[rbp-24]
   cqo
   idiv r15
   mov qword[rbp-24],rax
   if_13_8:
   cmp_13_8:
   cmp qword[rbp-24],-2
   je if_13_5_end
   if_13_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_13_17_end:
   if_13_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   neg qword[rbp-24]
   mov r15,qword[rbp-16]
   neg r15
   mov rax,qword[rbp-24]
   cqo
   idiv r15
   mov qword[rbp-24],rax
   if_16_8:
   cmp_16_8:
   cmp qword[rbp-24],2
   je if_16_5_end
   if_16_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_16_16_end:
   if_16_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   mov r15,qword[rbp-16]
   neg r15
   add r15,1
   neg r15
   mov rax,qword[rbp-24]
   cqo
   idiv r15
   mov qword[rbp-24],rax
   if_19_8:
   cmp_19_8:
   cmp qword[rbp-24],4
   je if_19_5_end
   if_19_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_19_16_end:
   if_19_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   mov r15,qword[rbp-16]
   add r15,1
   neg r15
   mov rax,qword[rbp-24]
   cqo
   idiv r15
   mov qword[rbp-24],rax
   if_22_8:
   cmp_22_8:
   cmp qword[rbp-24],-1
   je if_22_5_end
   if_22_8_code:  ; opt1
       mov rdi,4
       mov rax,60
       syscall
     exit_22_17_end:
   if_22_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   mov rax,qword[rbp-24]
   cqo
   idiv qword[rbp-16]
   mov qword[rbp-24],rax
   if_25_8:
   cmp_25_8:
   cmp qword[rbp-24],2
   je if_25_5_end
   if_25_8_code:  ; opt1
       mov rdi,5
       mov rax,60
       syscall
     exit_25_16_end:
   if_25_5_end:
   mov qword[rbp-8],3
   if_28_8:
   cmp_28_8:
     mov r15,qword[rbp-8]
     mov rax,r15
     cqo
     mov r14,2
     idiv r14
     mov r15,rdx
   cmp r15,1
   je if_28_5_end
   if_28_8_code:  ; opt1
       mov rdi,6
       mov rax,60
       syscall
     exit_28_18_end:
   if_28_5_end:
   mov qword[rbp-24],1
   mov r15,3
   mov rax,r15
   cqo
   mov r14,2
   idiv r14
   mov r15,rdx
   add r15,1
   add qword[rbp-24],r15
   if_31_8:
   cmp_31_8:
   cmp qword[rbp-24],3
   je if_31_5_end
   if_31_8_code:  ; opt1
       mov rdi,7
       mov rax,60
       syscall
     exit_31_16_end:
   if_31_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   neg qword[rbp-24]
   mov rax,qword[rbp-24]
   cqo
   idiv qword[rbp-16]
   mov qword[rbp-24],rax
   if_34_8:
   cmp_34_8:
   cmp qword[rbp-24],-1
   je if_34_5_end
   if_34_8_code:  ; opt1
       mov rdi,8
       mov rax,60
       syscall
     exit_34_17_end:
   if_34_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   mov rax,qword[rbp-24]
   cqo
   idiv qword[rbp-16]
   mov qword[rbp-24],rax
   if_37_8:
   cmp_37_8:
   cmp qword[rbp-24],1
   je if_37_5_end
   if_37_8_code:  ; opt1
       mov rdi,9
       mov rax,60
       syscall
     exit_37_16_end:
   if_37_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   mov r15,qword[rbp-16]
   neg r15
   mov rax,qword[rbp-24]
   cqo
   idiv r15
   mov qword[rbp-24],rax
   if_40_8:
   cmp_40_8:
   cmp qword[rbp-24],-1
   je if_40_5_end
   if_40_8_code:  ; opt1
       mov rdi,10
       mov rax,60
       syscall
     exit_40_17_end:
   if_40_5_end:
   mov r15,qword[rbp-8]
   mov qword[rbp-24],r15
   mov r15,qword[rbp-16]
   add r15,1
   neg r15
   mov rax,qword[rbp-24]
   cqo
   idiv r15
   mov qword[rbp-24],rax
   if_43_8:
   cmp_43_8:
   cmp qword[rbp-24],-1
   je if_43_5_end
   if_43_8_code:  ; opt1
       mov rdi,11
       mov rax,60
       syscall
     exit_43_17_end:
   if_43_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_45_5_end:
