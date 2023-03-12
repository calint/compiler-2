section .data
align 4
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
a:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   add r15,1
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
b:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   add r15,2
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
c:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   add r15,3
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
main:
     push rbx
         push 1
         call c
         add rsp,8
         mov r14,rax
         push 2
         call c
         add rsp,8
         mov r13,rax
         add r14,r13
       push r14
       call b
       add rsp,8
       mov r15,rax
     push r15
     call a
     add rsp,8
     pop rbx
     mov rbx,rax
     mov rax,1
     int 0x80
   exit_20_5_end:
