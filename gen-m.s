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
   mov qword[rbp-14], 3
   mov dword[rbp-6], 4
   mov byte[rbp-2], 5
   mov byte[rbp-1], true
   if_42_8:
   cmp_42_8:
   cmp byte[rbp-1], 0
   jne if_42_5_end
   if_42_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_42_16_end:
   if_42_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_44_5_end:
   loop_45_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_46_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-22], rax
     read_47_19_end:
     sub qword[rbp-22], 1
     if_48_12:
     cmp_48_12:
     cmp qword[rbp-22], 0
     jne if_48_9_end
     if_48_12_code:  ; opt1
       jmp loop_45_5_end
     if_48_9_end:
     if_51_12:
     cmp_51_12:
     mov r15, qword[rbp-14]
     cmp qword[rbp-22], r15
     jg if_51_9_end
     if_51_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_52_13_end:
       jmp loop_45_5
     if_51_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_55_9_end:
       mov rdx, qword[rbp-22]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_56_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_57_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_58_9_end:
   jmp loop_45_5
   loop_45_5_end:
     mov rdi, 0
     mov rax, 60
     syscall
   exit_60_5_end:
