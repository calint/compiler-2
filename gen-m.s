section .data
section .bss
stk resd 256
stk.end:
section .text
align 4
bits 64
global _start
_start:
mov rbp,stk
mov rsp,stk.end
   mov qword[rbp+0],1
   mov qword[rbp+8],2
   mov qword[rbp+16],3
   mov qword[rbp+24],4
   if_12_8:
   cmp_12_9:
   cmp qword[rbp+0],1
   jne cmp_12_26
   cmp_12_17:
   cmp qword[rbp+8],2
   je if_12_8_code  ; opt2
   cmp_12_26:
   cmp qword[rbp+16],3
   jne if_12_5_end
   cmp_12_34:
   cmp qword[rbp+24],4
   jne if_12_5_end
   if_12_8_code:  ; opt1
       mov rbx,0
       mov rax,1
       int 0x80
     exit_13_9_end:
   if_12_5_end:
     mov rbx,1
     mov rax,1
     int 0x80
   exit_14_5_end:
