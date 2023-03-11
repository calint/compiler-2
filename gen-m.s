section .data
hello db 'hello',10,''
hello.len equ $-hello
world db 'world',10,''
world.len equ $-world
section .bss
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
print:
   push rbp
   mov rbp,rsp
   mov rbx,1
   mov rax,4
   int 0x80
   pop rbp
   ret
foo:
   push rbp
   mov rbp,rsp
   mov qword[rbp-8],world.len
   sub rsp,8
   mov rcx,world
     mov rdx,qword[rbp-8]
     sub rdx,1
   call print
   add rsp,8
   pop rbp
   ret
bar:
   push rbp
   mov rbp,rsp
   mov rdx,qword[rbp+24]
   mov qword[rbp-8],rdx
   loop_25_5:
     mov qword[rbp-16],2
     sub rsp,16
     mov rcx,hello
       mov rdx,hello.len
       sub rdx,qword[rbp+16]
     call print
     add rsp,16
     if_28_12:
     cmp_28_12:
     cmp qword[rbp+16],0
     jne if_28_9_end
     jmp if_28_12_code
     if_28_12_code:
       jmp loop_25_5_end
     if_28_9_end:
     sub qword[rbp+16],1
     sub qword[rbp-8],1
     sub qword[rbp-16],1
   jmp loop_25_5
   loop_25_5_end:
   pop rbp
   ret
f:
   push rbp
   mov rbp,rsp
   mov rax,qword[rbp+16]
   pop rbp
   ret
main:
   mov rcx,hello
   mov rdx,hello.len
   call print
   mov qword[rbp-8],2
   sub rsp,8
   call foo
   add rsp,8
   mov qword[rbp-16],3
   sub rsp,16
   push qword[rbp-16]
     mov rdx,qword[rbp-8]
     push qword[rbp-8]
     call f
     add rsp,8
     mov rcx,rax
     add rdx,rcx
   push rdx
   call bar
   add rsp,32
     mov rbx,qword[rbp-8]
     add rbx,qword[rbp-16]
     mov rax,1
     int 0x80
   exit_45_5_end:
