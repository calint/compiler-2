section .data
prompt db '  hello    enter name: '
prompt.len equ $-prompt
name db '............................................................'
name.len equ $-name
prompt2 db '  not a name: '
prompt2.len equ $-prompt2
len dq 0
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
read:
   push rbp
   mov rbp,rsp
   xor rax,rax
   xor rdi,rdi
   syscall
   pop rbp
   ret
exit:
   push rbp
   mov rbp,rsp
   mov rbx,qword[rbp+16]
   mov rax,1
   int 0x80
   pop rbp
   ret
foo:
   push rbp
   mov rbp,rsp
   if_30_8:
   cmp_30_8:
   cmp qword[rbp+16],0
   jne if_30_5_end
   jmp if_30_8_code
   if_30_8_code:
     pop rbp
     ret
   if_30_5_end:
   mov rcx,prompt
   mov rdx,prompt.len
   call print
   pop rbp
   ret
main:
   mov qword[rbp-8],0b1
   mov qword[rbp-16],2
   mov qword[rbp-24],3
   mov qword[rbp-32],4
   mov r15,qword[rbp-8]
   add r15,qword[rbp-16]
   add r15,qword[rbp-24]
   add r15,qword[rbp-32]
   mov qword[rbp-40],r15
   mov qword[rbp-48],1
   sub rsp,48
   push qword[rbp-48]
   call foo
   add rsp,56
   mov qword[rbp-48],0
   sub rsp,48
   push qword[rbp-48]
   call foo
   add rsp,56
   loop_45_5:
     sub rsp,56
     mov rsi,name
     mov rdx,name.len
     call read
     add rsp,56
     mov qword[rbp-56],rax
     sub qword[rbp-56],1
     if_47_12:
     cmp_47_12:
     cmp qword[rbp-56],0
     jne if_49_17
     jmp if_47_12_code
     if_47_12_code:
       sub rsp,56
       mov rcx,prompt
       mov rdx,prompt.len
       call print
       add rsp,56
       jmp if_47_9_end
     if_49_17:
     cmp_49_17:
     cmp qword[rbp-56],4
     jg if_else_47_9
     jmp if_49_17_code
     if_49_17_code:
       sub rsp,56
       mov rcx,prompt2
       mov rdx,prompt2.len
       call print
       add rsp,56
       jmp if_47_9_end
     if_else_47_9:
         sub rsp,56
         mov rcx,name
           mov rdx,qword[rbp-56]
           add rdx,1
         call print
         add rsp,56
         if_53_16:
         cmp_53_16:
           sub rsp,56
           mov rsi,name
           mov rdx,name.len
           call read
           add rsp,56
           mov r15,rax
         cmp r15,1
         jne if_53_13_end
         jmp if_53_16_code
         if_53_16_code:
           jmp loop_45_5_end
         if_53_13_end:
     if_47_9_end:
   jmp loop_45_5
   loop_45_5_end:
   sub rsp,48
   push 0
   call exit
   add rsp,56
