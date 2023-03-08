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
mov rbp,stk
mov rsp,stk.end
   mov qword[rbp+0],1
   mov qword[rbp+8],2
   mov qword[rbp+16],3
   mov qword[rbp+24],4
   mov r15,qword[rbp+0]
   add r15,qword[rbp+8]
   add r15,qword[rbp+16]
   add r15,qword[rbp+24]
   mov qword[rbp+32],r15
     mov rcx,prompt
     mov rdx,prompt.len
     mov rbx,1
     mov rax,4
     int 0x80
   print_35_5_end:
   loop_36_5:
       mov rsi,name
       mov rdx,name.len
       xor rax,rax
       xor rdi,rdi
       syscall
       mov qword[rbp+40],rax
     read_37_17_end:
     sub qword[rbp+40],1
     if_38_12:
     cmp_38_12:
     cmp qword[rbp+40],0
     jne if_40_17
     if_38_12_code:  ; opt1
         mov rcx,prompt
         mov rdx,prompt.len
         mov rbx,1
         mov rax,4
         int 0x80
       print_39_13_end:
       jmp if_38_9_end
     if_40_17:
     cmp_40_17:
     cmp qword[rbp+40],4
     jg if_else_38_9
     if_40_17_code:  ; opt1
         mov rcx,prompt2
         mov rdx,prompt2.len
         mov rbx,1
         mov rax,4
         int 0x80
       print_41_13_end:
       jmp if_38_9_end
     if_else_38_9:
           mov rdx,qword[rbp+40]
           add rdx,1
           mov rcx,name
           mov rbx,1
           mov rax,4
           int 0x80
         print_43_13_end:
         if_44_16:
         cmp_44_16:
             mov rsi,name
             mov rdx,name.len
             xor rax,rax
             xor rdi,rdi
             syscall
             mov rdx,rax
           read_44_16_end:
         cmp rdx,1
         jne if_44_13_end
         if_44_16_code:  ; opt1
           jmp loop_36_5_end
         if_44_13_end:
     if_38_9_end:
   jmp loop_36_5
   loop_36_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_48_5_end:
