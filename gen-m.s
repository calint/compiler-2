section .data
align 4
name db '............................................................'
name.len equ $-name
section .bss
align 4
stk resd 256
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
       mov r13,qword[rbp-8]
       neg r13
       mov r14,r13
       neg r14
     bar_19_17_end:
     neg r14
     mov r15,r14
     neg r15
     imul r15,2
   foo_19_12_end:
   neg r15
   mov qword[rbp-16],r15
   if_20_7:
   cmp_20_8:
   cmp qword[rbp-16],-2
   jne if_20_5_end
   if_20_7_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_21_9_end:
   if_20_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_22_5_end:
