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
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_33_5_end:
   loop_34_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_35_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-8], rax
     read_36_19_end:
     sub qword[rbp-8], 1
     if_37_12:
     cmp_37_12:
     cmp qword[rbp-8], 0
     jne if_37_9_end
     if_37_12_code:  ; opt1
       jmp loop_34_5_end
     if_37_9_end:
     if_40_12:
     cmp_40_12:
     cmp qword[rbp-8], 4
     jg if_40_9_end
     if_40_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_41_13_end:
       jmp loop_34_5
     if_40_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_44_9_end:
       mov rdx, qword[rbp-8]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_45_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_46_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_47_9_end:
   jmp loop_34_5
   loop_34_5_end:
     mov rdi, 0
     mov rax, 60
     syscall
   exit_49_5_end:
