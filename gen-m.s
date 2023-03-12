section .data
align 4
hello db 'hello',10,''
hello.len equ $-hello
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
print:
   push rbp
   mov rbp,rsp
   mov rbx,1
   mov rax,4
   int 0x80
   pop rbp
   ret
bar:
   push rbp
   mov rbp,rsp
   mov rcx,hello
   mov rdx,hello.len
   call print
   pop rbp
   ret
main:
   mov qword[rbp-8],3
   mov qword[rbp-16],2
   loop_22_5:
     if_23_12:
     cmp_23_12:
     cmp qword[rbp-8],0
     jne if_23_9_end
     if_23_12_code:  ; opt1
       jmp loop_22_5_end
     if_23_9_end:
     sub rsp,16
     call bar
     add rsp,16
     sub qword[rbp-8],1
   jmp loop_22_5
   loop_22_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_27_5_end:
