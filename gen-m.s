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
   mov rbp,rsp
   mov rcx,qword[rbp+16]
   mov rdx,qword[rbp+8]
   mov rbx,1
   mov rax,4
   int 0x80
   ret
foo:
   mov rbp,rsp
   mov qword[rbp-8],world.len
   sub rsp,8
   push world
     mov r15,qword[rbp-8]
     sub r15,1
   push r15
   call print
   add rsp,24
   mov rbp,rsp
   ret
bar:
   mov rbp,rsp
   mov r15,qword[rbp+8]
   mov qword[rbp-8],r15
   loop_25_5:
     mov qword[rbp-16],2
     if_27_12:
     cmp_27_12:
     cmp qword[rbp+8],0
     jne if_27_9_end
     jmp if_27_12_code
     if_27_12_code:
       jmp loop_25_5_end
     if_27_9_end:
     sub rsp,16
     push hello
     push hello.len
     call print
     add rsp,32
     mov rbp,rsp
     sub qword[rbp+8],1
     sub qword[rbp-8],1
     sub qword[rbp-16],1
   jmp loop_25_5
   loop_25_5_end:
   ret
main:
   mov qword[rbp-8],2
   sub rsp,8
   call foo
   add rsp,8
   mov rbp,rsp
   sub rsp,8
     mov r15,qword[rbp-8]
     add r15,1
   push r15
   call bar
   add rsp,16
   mov rbp,rsp
     mov rbx,0
     mov rax,1
     int 0x80
   exit_39_5_end:
