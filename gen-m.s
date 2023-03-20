section .data
align 4
len dq -2
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
   mov qword[rbp-8],r15
   neg qword[rbp-8]
   mov rax,qword[rbp-8]
   pop rbp
   ret
main:
   mov qword[rbp-8],-1
   sub rsp,16
   mov r15,qword[rbp-8]
   neg r15
   push r15
   call foo
   add rsp,24
   neg rax
   not rax
   mov qword[rbp-16],rax
   mov r15,qword[len]
   mov qword[rbp-24],r15
   neg qword[rbp-24]
   not qword[rbp-24]
   if_17_8:
   cmp_17_8:
   cmp qword[rbp-8],-1
   jne if_17_5_end
   cmp_17_17:
   cmp qword[rbp-16],~1
   jne if_17_5_end
   cmp_17_26:
   cmp qword[rbp-24],~2
   jne if_17_5_end
   if_17_8_code:  ; opt1
       mov rdi,0
       mov rax,60
       syscall
     exit_18_9_end:
   if_17_5_end:
     mov rdi,1
     mov rax,60
     syscall
   exit_19_5_end:
