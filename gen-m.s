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
   mov qword[rbp-8],1
   mov r15,qword[rbp-8]
   mov qword[rbp-16],r15
   sal qword[rbp-16],1
   if_10_8:
   cmp_10_8:
   cmp qword[rbp-16],2
   je if_10_5_end
   if_10_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_10_16_end:
   if_10_5_end:
   sar qword[rbp-16],1
   if_13_8:
   cmp_13_8:
   cmp qword[rbp-16],1
   je if_13_5_end
   if_13_8_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_13_16_end:
   if_13_5_end:
   mov qword[rbp-24],2
   mov r15,qword[rbp-8]
   mov qword[rbp-32],r15
   mov rcx,qword[rbp-24]
   sal qword[rbp-32],cl
   if_17_8:
   cmp_17_8:
   cmp qword[rbp-32],4
   je if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_17_16_end:
   if_17_5_end:
   mov r15,qword[rbp-32]
   mov qword[rbp-40],r15
   mov rcx,qword[rbp-24]
   sar qword[rbp-40],cl
   if_20_8:
   cmp_20_8:
   cmp qword[rbp-40],1
   je if_20_5_end
   if_20_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_20_16_end:
   if_20_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_22_5_end:
