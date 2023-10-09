true equ 1
false equ 0
section .data
align 4
hello: db 'hello world from baz',10,''
hello.len equ $-hello
input: db '............................................................'
input.len equ $-input
prompt1: db 'enter name: '
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
nl: db '',10,''
nl.len equ $-nl
dot: db '.'
dot.len equ $-dot
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
main:
   mov qword[rbp-12], 3
   mov dword[rbp-4], 4
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_39_5_end:
   loop_40_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_41_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-20], rax
     read_42_19_end:
     sub qword[rbp-20], 1
     if_43_12:
     cmp_43_12:
     cmp qword[rbp-20], 0
     jne if_43_9_end
     if_43_12_code:  ; opt1
       jmp loop_40_5_end
     if_43_9_end:
     if_46_12:
     cmp_46_12:
     mov r15, qword[rbp-12]
     cmp qword[rbp-20], r15
     jg if_46_9_end
     if_46_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_47_13_end:
       jmp loop_40_5
     if_46_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_50_9_end:
       mov rdx, qword[rbp-20]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_51_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_52_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_53_9_end:
   jmp loop_40_5
   loop_40_5_end:
     mov rdi, 0
     mov rax, 60
     syscall
   exit_55_5_end:
