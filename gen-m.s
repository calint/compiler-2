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
   mov qword[rsp-0],1
   mov qword[rsp-8],2
   mov qword[rsp-16],3
   mov qword[rsp-24],4
   mov r15,qword[rsp-0]
   add r15,qword[rsp-8]
   add r15,qword[rsp-16]
   add r15,qword[rsp-24]
   mov qword[rsp-32],r15
   mov qword[rsp-40],2
     loop_35_5_49_5:
       if_36_12_49_5:
       cmp_36_12_49_5:
       cmp qword[rsp-40],0
       jne if_36_9_49_5_end
       if_36_12_49_5_code:  ; opt1
         jmp foo_49_5_end
       if_36_9_49_5_end:
           mov rcx,hello
           mov rdx,hello.len
           mov rbx,1
           mov rax,4
           int 0x80
         print_31_5_37_9_49_5_end:
       bar_37_9_49_5_end:
       sub qword[rsp-40],1
     jmp loop_35_5_49_5
     loop_35_5_49_5_end:
   foo_49_5_end:
   mov qword[rsp-40],1
     loop_35_5_51_5:
       if_36_12_51_5:
       cmp_36_12_51_5:
       cmp qword[rsp-40],0
       jne if_36_9_51_5_end
       if_36_12_51_5_code:  ; opt1
         jmp foo_51_5_end
       if_36_9_51_5_end:
           mov rcx,hello
           mov rdx,hello.len
           mov rbx,1
           mov rax,4
           int 0x80
         print_31_5_37_9_51_5_end:
       bar_37_9_51_5_end:
       sub qword[rsp-40],1
     jmp loop_35_5_51_5
     loop_35_5_51_5_end:
   foo_51_5_end:
     mov rbx,0
     mov rax,1
     int 0x80
   exit_52_5_end:
