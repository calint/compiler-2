section .data
align 4
prompt db '  hello    enter name: '
prompt.len equ $-prompt
name db '............................................................'
name.len equ $-name
prompt2 db '  not a name: '
prompt2.len equ $-prompt2
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
main:
     mov rdx,prompt.len
     mov rsi,prompt
     mov rax,1
     mov rdi,1
     syscall
   print_29_5_end:
   loop_30_5:
       mov rdx,name.len
       mov rsi,name
       mov rax,0
       mov rdi,0
       syscall
       mov qword[rbp-8],rax
     read_31_17_end:
     sub qword[rbp-8],1
     if_32_12:
     cmp_32_12:
     cmp qword[rbp-8],0
     jne if_34_17
     if_32_12_code:  ; opt1
         mov rdx,prompt.len
         mov rsi,prompt
         mov rax,1
         mov rdi,1
         syscall
       print_33_13_end:
     jmp if_32_9_end
     if_34_17:
     cmp_34_17:
     cmp qword[rbp-8],4
     jg if_else_32_9
     if_34_17_code:  ; opt1
         mov rdx,prompt2.len
         mov rsi,prompt2
         mov rax,1
         mov rdi,1
         syscall
       print_35_13_end:
     jmp if_32_9_end
     if_else_32_9:
           mov rdx,qword[rbp-8]
           add rdx,1
           mov rsi,name
           mov rax,1
           mov rdi,1
           syscall
         print_37_13_end:
         if_38_16:
         cmp_38_16:
             mov rdx,name.len
             mov rsi,name
             mov rax,0
             mov rdi,0
             syscall
             mov r15,rax
           read_38_16_end:
         cmp r15,1
         jne if_38_13_end
         if_38_16_code:  ; opt1
           jmp loop_30_5_end
         if_38_13_end:
     if_32_9_end:
   jmp loop_30_5
   loop_30_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_42_5_end:
