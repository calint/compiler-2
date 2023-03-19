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
foo:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   neg r15
   imul r15,2
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
bar:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   neg r15
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
main:
   mov qword[rbp-8],1
   sub rsp,16
     mov r14,qword[rbp-8]
     neg r14
     push r14
     call bar
     add rsp,8
     neg rax
     mov r15,rax
   push r15
   call foo
   add rsp,24
   neg rax
   mov qword[rbp-16],rax
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
