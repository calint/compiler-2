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
   mov dword[rbp-23], 0
   mov byte[rbp-16], true
     cmp_58_12:
     cmp dword[rbp-23], 0
     jne false_58_12
     true_58_12:  ; opt1
     mov r15, 1
     jmp end_58_12
     false_58_12:
     mov r15, 0
     end_58_12:
     if_33_8_58_5:
     cmp_33_8_58_5:
     cmp r15, 0
     jne if_33_5_58_5_end
     if_33_8_58_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_58_5_end:
     if_33_5_58_5_end:
   assert_58_5_end:
     cmp_59_12:
     cmp byte[rbp-16], 0
     je false_59_12
     true_59_12:  ; opt1
     mov r15, 1
     jmp end_59_12
     false_59_12:
     mov r15, 0
     end_59_12:
     if_33_8_59_5:
     cmp_33_8_59_5:
     cmp r15, 0
     jne if_33_5_59_5_end
     if_33_8_59_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_59_5_end:
     if_33_5_59_5_end:
   assert_59_5_end:
     cmp_60_12:
     cmp dword[rbp-23], 0
     jne false_60_12
     true_60_12:  ; opt1
     mov r15, 1
     jmp end_60_12
     false_60_12:
     mov r15, 0
     end_60_12:
     if_33_8_60_5:
     cmp_33_8_60_5:
     cmp r15, 0
     jne if_33_5_60_5_end
     if_33_8_60_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_60_5_end:
     if_33_5_60_5_end:
   assert_60_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_62_5_end:
   loop_63_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_64_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-54], rax
     read_65_19_end:
     sub qword[rbp-54], 1
     if_66_12:
     cmp_66_12:
     cmp qword[rbp-54], 0
     jne if_66_9_end
     if_66_12_code:  ; opt1
       jmp loop_63_5_end
     if_66_9_end:
     if_69_12:
     cmp_69_12:
     mov r15, qword[rbp-31]
     cmp qword[rbp-54], r15
     jg if_69_9_end
     if_69_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_70_13_end:
       jmp loop_63_5
     if_69_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_73_9_end:
       mov rdx, qword[rbp-54]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_74_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_75_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_76_9_end:
   jmp loop_63_5
   loop_63_5_end:
mov rax, 60
mov rdi, 0
syscall
