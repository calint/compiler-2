section .data
prompt db '  hello    enter name: '
prompt.len equ $-prompt
name db '............................................................'
name.len equ $-name
prompt2 db '  not a name: '
prompt2.len equ $-prompt2
len dq 0
hello db 'hello',10,''
hello.len equ $-hello
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
print:
   mov rcx,qword[rsp+16]
   mov rdx,qword[rsp+8]
   mov rbx,1
   mov rax,4
   int 0x80
   ret
main:
   mov qword[rsp-8],1
   mov r15,rsp
   sub rsp,8
   push r15
   push hello
   push hello.len
   call print
   add rsp,16
   pop rsp
     mov rbx,0
     mov rax,1
     int 0x80
   exit_24_5_end:
