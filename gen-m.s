section .data
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
bar:
   push hello
   push hello.len
   call print
   add rsp,16
   ret
main:
   mov qword[rsp-8],1
   mov qword[rsp-16],2
   loop_24_5:
     if_25_12:
     cmp_25_12:
     cmp qword[rsp-8],0
     jne if_25_9_end
     jmp if_25_12_code
     if_25_12_code:
       jmp loop_24_5_end
     if_25_9_end:
     mov r15,rsp
     sub rsp,16
     push r15
     call bar
     pop rsp
     sub qword[rsp-8],1
   jmp loop_24_5
   loop_24_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_29_5_end:
