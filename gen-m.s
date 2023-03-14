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
   add r15,0x1
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
b:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   add r15,0b10
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
c:
   push rbp
   mov rbp,rsp
   mov qword[rbp-16],1
   mov qword[rbp-24],2
   mov r15,qword[rbp-16]
   add r15,qword[rbp-24]
   mov qword[rbp-32],r15
   mov r15,qword[rbp+16]
   add r15,qword[rbp-32]
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
main:
   mov qword[rbp-8],1
   mov qword[rbp-16],2
   mov qword[rbp-24],3
     sub rsp,24
         push 1
         call c
         add rsp,8
         mov r14,rax
       push r14
       call b
       add rsp,8
       mov r15,rax
     push r15
     call a
     add rsp,32
     mov rbx,rax
     mov rax,1
     int 0x80
   exit_27_5_end:
