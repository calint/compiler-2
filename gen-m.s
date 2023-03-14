section .data
align 4
hello db 'hello',10,''
hello.len equ $-hello
world db 'world',10,''
world.len equ $-world
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
bar:
   push rbp
   mov rbp,rsp
   loop_19_5:
     if_20_12:
     cmp_20_12:
     cmp qword[rbp+24],0
     jne if_20_9_end
     if_20_12_code:  ; opt1
       pop rbp
       ret
     if_20_9_end:
       mov rdx,hello.len
       mov rcx,hello
       mov rbx,1
       mov rax,4
       int 0x80
     print_21_9_end:
     sub qword[rbp+24],1
   jmp loop_19_5
   loop_19_5_end:
   pop rbp
   ret
foo:
   push rbp
   mov rbp,rsp
     mov rdx,world.len
     mov rcx,world
     mov rbx,1
     mov rax,4
     int 0x80
   print_27_5_end:
   pop rbp
   ret
main:
   mov qword[rbp-8],1
   mov qword[rbp-16],2
   mov qword[rbp-24],3
   sub rsp,24
   push qword[rbp-16]
   push qword[rbp-8]
   call bar
   add rsp,40
   sub rsp,24
   call foo
   add rsp,24
     mov rbx,0
     mov rax,1
     int 0x80
   exit_41_5_end:
