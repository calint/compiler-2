section .data
align 4
prompt db '  hello    enter name: '
prompt.len equ $-prompt
name db '............................................................'
name.len equ $-name
prompt2 db '  not a name: '
prompt2.len equ $-prompt2
len dq 0
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
     mov rcx,prompt
     mov rbx,1
     mov rax,4
     int 0x80
   print_30_5_end:
   loop_31_5:
       mov rdx,name.len
       mov rsi,name
       xor rax,rax
       xor rdi,rdi
       syscall
       mov qword[rbp-8],rax
     read_32_17_end:
     sub qword[rbp-8],1
     if_33_12:
     cmp_33_12:
     cmp qword[rbp-8],0
     jne if_35_17
     if_33_12_code:  ; opt1
         mov rdx,prompt.len
         mov rcx,prompt
         mov rbx,1
         mov rax,4
         int 0x80
       print_34_13_end:
     jmp if_33_9_end
     if_35_17:
     cmp_35_17:
     cmp qword[rbp-8],4
     jg if_else_33_9
     if_35_17_code:  ; opt1
         mov rdx,prompt2.len
         mov rcx,prompt2
         mov rbx,1
         mov rax,4
         int 0x80
       print_36_13_end:
     jmp if_33_9_end
     if_else_33_9:
           mov rdx,qword[rbp-8]
           add rdx,1
           mov rcx,name
           mov rbx,1
           mov rax,4
           int 0x80
         print_38_13_end:
         if_39_16:
         cmp_39_16:
             mov rdx,name.len
             mov rsi,name
             xor rax,rax
             xor rdi,rdi
             syscall
             mov r15,rax
           read_39_16_end:
         cmp r15,1
         jne if_39_13_end
         if_39_16_code:  ; opt1
           jmp loop_31_5_end
         if_39_13_end:
     if_33_9_end:
   jmp loop_31_5
   loop_31_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_43_5_end:
