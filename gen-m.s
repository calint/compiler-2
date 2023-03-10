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
jmp main
bar:
   loop_19_5:
     if_20_12:
     cmp_20_12:
     cmp qword[rsp+16],0
     jne if_20_9_end
     jmp if_20_12_code
     if_20_12_code:
       ret
     if_20_9_end:
       mov rcx,hello
       mov rdx,hello.len
       mov rbx,1
       mov rax,4
       int 0x80
     print_21_9_end:
     sub qword[rsp+16],1
   jmp loop_19_5
   loop_19_5_end:
   ret
foo:
     mov rcx,world
     mov rdx,world.len
     mov rbx,1
     mov rax,4
     int 0x80
   print_27_5_end:
   ret
main:
   mov qword[rsp-8],1
   mov qword[rsp-16],2
   mov qword[rsp-24],3
   mov r15,rsp
   sub rsp,24
   push r15
   push qword[r15-16]
   push qword[r15-8]
   call bar
   add rsp,16
   pop rsp
   mov r15,rsp
   sub rsp,24
   push r15
   call foo
   pop rsp
     mov rbx,0
     mov rax,1
     int 0x80
   exit_36_5_end:
