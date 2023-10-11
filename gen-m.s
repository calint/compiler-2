true equ 1
false equ 0
section .data
align 4
hello: db 'hello world from baz',10,''
hello.len equ $-hello
input: db '............................................................'
input.len equ $-input
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot_nl: db '.',10,''
dot_nl.len equ $-dot_nl
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
assert:
   push rbp
   mov rbp, rsp
   if_32_8:
   cmp_32_8:
   cmp byte[rbp+16], 0
   jne if_32_5_end
   if_32_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_32_17_end:
   if_32_5_end:
   pop rbp
   ret
main:
   mov qword[rbp-15], 3
   mov dword[rbp-7], 4
   mov word[rbp-3], 5
   mov byte[rbp-1], 6
   mov qword[rbp-46], 1
   mov qword[rbp-31], 4
   mov dword[rbp-23], 0
   mov byte[rbp-16], true
   sub rsp, 46
     cmp_57_12:
     cmp dword[rbp-23], 0
     jne false_57_12
     true_57_12:  ; opt1
     mov r15, 1
     jmp end_57_12
     false_57_12:
     mov r15, 0
     end_57_12:
   push r15
   call assert
   add rsp, 54
   sub rsp, 46
     cmp_58_12:
     cmp byte[rbp-16], 0
     je false_58_12
     true_58_12:  ; opt1
     mov r15, 1
     jmp end_58_12
     false_58_12:
     mov r15, 0
     end_58_12:
   push r15
   call assert
   add rsp, 54
   sub rsp, 46
     cmp_59_12:
     cmp dword[rbp-23], 0
     jne false_59_12
     true_59_12:  ; opt1
     mov r15, 1
     jmp end_59_12
     false_59_12:
     mov r15, 0
     end_59_12:
   push r15
   call assert
   add rsp, 54
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_61_5_end:
   loop_62_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_63_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-54], rax
     read_64_19_end:
     sub qword[rbp-54], 1
     if_65_12:
     cmp_65_12:
     cmp qword[rbp-54], 0
     jne if_65_9_end
     if_65_12_code:  ; opt1
       jmp loop_62_5_end
     if_65_9_end:
     if_68_12:
     cmp_68_12:
     mov r15, qword[rbp-31]
     cmp qword[rbp-54], r15
     jg if_68_9_end
     if_68_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_69_13_end:
       jmp loop_62_5
     if_68_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_72_9_end:
       mov rdx, qword[rbp-54]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_73_9_end:
       mov rdx, dot_nl.len
       mov rsi, dot_nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_74_9_end:
   jmp loop_62_5
   loop_62_5_end:
mov rax, 60
mov rdi, 0
syscall
