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
   mov qword[rbp+40],2
     loop_30_5_44_5:
       if_31_12_44_5:
       cmp_31_12_44_5:
       cmp qword[rbp+40],0
       jne if_31_9_44_5_end
       jmp if_31_12_44_5_code
       if_31_12_44_5_code:
         jmp foo_44_5_end
       if_31_9_44_5_end:
         mov rcx,prompt
         mov rdx,prompt.len
         mov rbx,1
         mov rax,4
         int 0x80
       print_44_5_32_9_end:
       sub qword[rbp+40],1
     jmp loop_30_5_44_5
     loop_30_5_44_5_end:
   foo_44_5_end:
   mov qword[rbp+40],1
     loop_30_5_46_5:
       if_31_12_46_5:
       cmp_31_12_46_5:
       cmp qword[rbp+40],0
       jne if_31_9_46_5_end
       jmp if_31_12_46_5_code
       if_31_12_46_5_code:
         jmp foo_46_5_end
       if_31_9_46_5_end:
         mov rcx,prompt
         mov rdx,prompt.len
         mov rbx,1
         mov rax,4
         int 0x80
       print_46_5_32_9_end:
       sub qword[rbp+40],1
     jmp loop_30_5_46_5
     loop_30_5_46_5_end:
   foo_46_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_47_5_end:
