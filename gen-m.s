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
foo:
   mov qword[rsp-8],world.len
     mov rcx,world
     mov rdx,qword[rsp-8]
     mov rbx,1
     mov rax,4
     int 0x80
   print_20_5_end:
   ret
bar:
   loop_24_5:
     if_25_12:
     cmp_25_12:
     cmp qword[rsp+8],0
     jne if_25_9_end
     jmp if_25_12_code
     if_25_12_code:
       jmp loop_24_5_end
     if_25_9_end:
       mov rcx,hello
       mov rdx,hello.len
       mov rbx,1
       mov rax,4
       int 0x80
     print_26_9_end:
     sub qword[rsp+8],1
   jmp loop_24_5
   loop_24_5_end:
   ret
main:
   mov qword[rsp-8],2
   sub rsp,8
   call foo
   add rsp,8
   mov r15,rsp
   sub rsp,8
     mov r14,qword[rsp-8]
     add r14,1
   push r14
   call bar
   add rsp,16
     mov rbx,0
     mov rax,1
     int 0x80
   exit_35_5_end:
