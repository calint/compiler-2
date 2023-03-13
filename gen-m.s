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
     push rbx
       push r15
         mov r14,qword[rbp-24]
         push r14
         push r13
         push 1
         call c
         add rsp,8
         pop r13
         pop r14
         mov r13,rax
         add r14,r13
         push r14
         push r13
           mov r12,qword[rbp-8]
           add r12,qword[rbp-16]
         push r12
         call c
         add rsp,8
         pop r13
         pop r14
         mov r13,rax
         add r14,r13
       push r14
       call b
       add rsp,8
       pop r15
       mov r15,rax
     push r15
     call a
     add rsp,8
     pop rbx
     add rsp,24
     mov rbx,rax
     mov rax,1
     int 0x80
   exit_26_5_end:
