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
   mov qword[rbp-8],1
   mov rax,qword[rbp-8]
   pop rbp
   ret
main:
   mov qword[rbp-8],-2
   mov r15,qword[rbp-8]
   mov qword[rbp-16],r15
   neg qword[rbp-16]
   sub rsp,24
   push qword[rbp-8]
   call f
   add rsp,32
   not rax
   mov qword[rbp-24],rax
   if_15_8:
   cmp_15_13:
   cmp qword[rbp-8],-2
   jne if_15_8_code
   cmp_15_22:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_15_5_end
   if_15_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_15_28_end:
   if_15_5_end:
   if_16_8:
   cmp_16_8:
   cmp qword[rbp-8],-2
   jne if_16_8_code
   cmp_16_20:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_16_5_end
   if_16_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_16_29_end:
   if_16_5_end:
   if_17_8:
   cmp_17_13:
   cmp qword[rbp-8],-2
   je if_17_5_end
   cmp_17_21:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_17_27_end:
   if_17_5_end:
   if_18_8:
   cmp_18_8:
   cmp qword[rbp-8],-2
   je if_18_5_end
   cmp_18_21:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   je if_18_5_end
   if_18_8_code:  ; opt1
       mov rdi,4
       mov rax,60
       syscall
     exit_18_30_end:
   if_18_5_end:
   if_19_8:
   cmp_19_13:
   cmp qword[rbp-8],-2
   jne if_19_8_code
   cmp_19_27:
   cmp qword[rbp-8],-1
   jne if_19_5_end
   cmp_19_36:
   cmp qword[rbp-16],-1
   jne if_19_5_end
   if_19_8_code:  ; opt1
       mov rdi,5
       mov rax,60
       syscall
     exit_19_43_end:
   if_19_5_end:
   if_20_8:
   cmp_20_13:
   cmp qword[rbp-8],-2
   jne if_20_8_code
   cmp_20_27:
   cmp qword[rbp-8],-1
   jne if_20_5_end
   cmp_20_36:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   jne if_20_5_end
   if_20_8_code:  ; opt1
       mov rdi,6
       mov rax,60
       syscall
     exit_20_43_end:
   if_20_5_end:
   if_21_8:
   cmp_21_13:
   cmp qword[rbp-8],-2
   je if_21_5_end
   cmp_21_26:
   cmp qword[rbp-8],-2
   je if_21_8_code
   cmp_21_34:
   mov r15,qword[rbp-8]
   neg r15
   cmp qword[rbp-16],r15
   jne if_21_5_end
   if_21_8_code:  ; opt1
       mov rdi,7
       mov rax,60
       syscall
     exit_21_41_end:
   if_21_5_end:
   if_22_8:
   cmp_22_13:
   cmp qword[rbp-8],-2
   je if_22_5_end
   if_22_8_code:  ; opt1
       mov rdi,8
       mov rax,60
       syscall
     exit_22_19_end:
   if_22_5_end:
   if_23_8:
   cmp_23_8:
   cmp qword[rbp-8],-2
   je if_23_5_end
   if_23_8_code:  ; opt1
       mov rdi,9
       mov rax,60
       syscall
     exit_23_17_end:
   if_23_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_24_5_end:
