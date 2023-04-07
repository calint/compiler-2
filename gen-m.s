true equ 1
false equ 0
section .data
align 4
section .bss
align 4
stk resd 1024
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
   add qword[rbp+16],1
   if_13_7:
   cmp_13_7:
   cmp qword[rbp+16],2
   je if_13_4_end
   if_13_7_code:  ; opt1
       mov rdi,2
       mov rax,60
       syscall
     exit_13_16_end:
   if_13_4_end:
   pop rbp
   ret
main:
   mov qword[rbp-8],0
     add qword[rbp-8],1
   foo_18_5_end:
   if_19_8:
   cmp_19_8:
   cmp qword[rbp-8],1
   je if_19_5_end
   if_19_8_code:  ; opt1
       mov rdi,1
       mov rax,60
       syscall
     exit_19_17_end:
   if_19_5_end:
   sub rsp,8
   push qword[rbp-8]
   call bar
   add rsp,16
   if_22_8:
   cmp_22_8:
   cmp qword[rbp-8],1
   je if_22_5_end
   if_22_8_code:  ; opt1
       mov rdi,3
       mov rax,60
       syscall
     exit_22_17_end:
   if_22_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_24_5_end:
