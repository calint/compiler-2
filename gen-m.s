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
fib:
   push rbp
   mov rbp,rsp
   if_8_8:
   cmp_8_8:
   cmp qword[rbp+16],0
   jne if_8_5_end
   if_8_8_code:  ; opt1
     mov rax,1
     pop rbp
     ret
   if_8_5_end:
   mov r15,qword[rbp+16]
   mov qword[rbp-8],r15
   sub rsp,8
   push r15
     mov r14,qword[rbp+16]
     sub r14,1
   push r14
   call fib
   add rsp,8
   pop r15
   add rsp,8
   mov r15,rax
   imul r15,qword[rbp-8]
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
main:
     push rbx
     push 5
     call fib
     add rsp,8
     pop rbx
     mov rbx,rax
     mov rax,1
     int 0x80
   exit_17_5_end:
