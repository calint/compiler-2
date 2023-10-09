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
   mov qword[rbp-16], 3
   mov dword[rbp-8], 4
   mov word[rbp-4], 5
   mov byte[rbp-2], 6
   mov byte[rbp-1], true
   if_49_8:
   cmp_49_8:
   cmp byte[rbp-1], 0
   jne if_49_5_end
   if_49_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_49_20_end:
   if_49_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_51_5_end:
   loop_52_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_53_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-24], rax
     read_54_19_end:
     sub qword[rbp-24], 1
     if_55_12:
     cmp_55_12:
     cmp qword[rbp-24], 0
     jne if_55_9_end
     if_55_12_code:  ; opt1
       jmp loop_52_5_end
     if_55_9_end:
     if_58_12:
     cmp_58_12:
     mov r15, qword[rbp-16]
     cmp qword[rbp-24], r15
     jg if_58_9_end
     if_58_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_59_13_end:
       jmp loop_52_5
     if_58_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_62_9_end:
       mov rdx, qword[rbp-24]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_63_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_64_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_65_9_end:
   jmp loop_52_5
   loop_52_5_end:
mov rax, 60
mov rdi, 0
syscall
