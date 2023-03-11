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
   mov rax,qword[rbp+16]
     mov r14,qword[rbp+16]
     sub r14,1
   push r14
   call fib
   add rsp,8
   mov r15,rax
   imul rax,r15
   pop rbp
   ret
main:
   mov qword[rbp-8],1
   if_17_8:
   cmp_17_8:
   cmp qword[rbp-8],1
   jne if_19_14
   if_17_8_code:  ; opt1
     mov qword[rbp-16],2
     jmp if_17_5_end
   if_19_14:
   cmp_19_14:
   cmp qword[rbp-8],2
   jne if_else_17_5
   if_19_14_code:  ; opt1
     mov qword[rbp-16],3
     jmp if_17_5_end
   if_else_17_5:
       mov qword[rbp-16],3
   if_17_5_end:
     sub rsp,32
     push 5
     call fib
     add rsp,40
     mov rbx,rax
     mov rax,1
     int 0x80
   exit_24_5_end:
