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
   mov qword[rbp-15], 3
   mov dword[rbp-7], 4
   mov word[rbp-3], 5
   mov byte[rbp-1], 6
   mov qword[rbp-46], 1
   mov qword[rbp-31], 4
   mov byte[rbp-16], true
   if_53_8:
   cmp_53_8:
   cmp byte[rbp-16], 0
   jne if_53_5_end
   if_53_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_53_22_end:
   if_53_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_55_5_end:
   loop_56_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_57_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-54], rax
     read_58_19_end:
     sub qword[rbp-54], 1
     if_59_12:
     cmp_59_12:
     cmp qword[rbp-54], 0
     jne if_59_9_end
     if_59_12_code:  ; opt1
       jmp loop_56_5_end
     if_59_9_end:
     if_62_12:
     cmp_62_12:
     mov r15, qword[rbp-31]
     cmp qword[rbp-54], r15
     jg if_62_9_end
     if_62_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_63_13_end:
       jmp loop_56_5
     if_62_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_66_9_end:
       mov rdx, qword[rbp-54]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_67_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_68_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_69_9_end:
   jmp loop_56_5
   loop_56_5_end:
mov rax, 60
mov rdi, 0
syscall
