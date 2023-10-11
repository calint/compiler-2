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
dot: db '.'
dot.len equ $-dot
nl: db '',10,''
nl.len equ $-nl
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
   if_33_8:
   cmp_33_8:
   cmp byte[rbp+16], 0
   jne if_33_5_end
   if_33_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_33_17_end:
   if_33_5_end:
   pop rbp
   ret
main:
       mov qword[rbp-20], 1
       mov qword[rbp-12], 2
     mov dword[rbp-4], 0xff0000
   sub rsp, 20
     cmp_42_12:
     cmp qword[rbp-20], 1
     jne false_42_12
     true_42_12:  ; opt1
     mov r15, 1
     jmp end_42_12
     false_42_12:
     mov r15, 0
     end_42_12:
   push r15
   call assert
   add rsp, 28
   sub rsp, 20
     cmp_43_12:
     cmp qword[rbp-12], 2
     jne false_43_12
     true_43_12:  ; opt1
     mov r15, 1
     jmp end_43_12
     false_43_12:
     mov r15, 0
     end_43_12:
   push r15
   call assert
   add rsp, 28
   sub rsp, 20
     cmp_44_12:
     cmp dword[rbp-4], 0xff0000
     jne false_44_12
     true_44_12:  ; opt1
     mov r15, 1
     jmp end_44_12
     false_44_12:
     mov r15, 0
     end_44_12:
   push r15
   call assert
   add rsp, 28
     mov qword[rbp-36], -1
     mov qword[rbp-28], -2
     mov r15, qword[rbp-36]
     mov qword[rbp-20], r15
     mov r15, qword[rbp-28]
     mov qword[rbp-12], r15
   sub rsp, 36
     cmp_48_12:
     cmp qword[rbp-20], -1
     jne false_48_12
     true_48_12:  ; opt1
     mov r15, 1
     jmp end_48_12
     false_48_12:
     mov r15, 0
     end_48_12:
   push r15
   call assert
   add rsp, 44
   sub rsp, 36
     cmp_49_12:
     cmp qword[rbp-12], -2
     jne false_49_12
     true_49_12:  ; opt1
     mov r15, 1
     jmp end_49_12
     false_49_12:
     mov r15, 0
     end_49_12:
   push r15
   call assert
   add rsp, 44
       mov r15, qword[rbp-20]
       mov qword[rbp-56], r15
       mov r15, qword[rbp-12]
       mov qword[rbp-48], r15
     mov r15d, dword[rbp-4]
     mov dword[rbp-40], r15d
   sub rsp, 56
     cmp_52_12:
     cmp qword[rbp-56], -1
     jne false_52_12
     true_52_12:  ; opt1
     mov r15, 1
     jmp end_52_12
     false_52_12:
     mov r15, 0
     end_52_12:
   push r15
   call assert
   add rsp, 64
   sub rsp, 56
     cmp_53_12:
     cmp qword[rbp-48], -2
     jne false_53_12
     true_53_12:  ; opt1
     mov r15, 1
     jmp end_53_12
     false_53_12:
     mov r15, 0
     end_53_12:
   push r15
   call assert
   add rsp, 64
   sub rsp, 56
     cmp_54_12:
     cmp dword[rbp-40], 0xff0000
     jne false_54_12
     true_54_12:  ; opt1
     mov r15, 1
     jmp end_54_12
     false_54_12:
     mov r15, 0
     end_54_12:
   push r15
   call assert
   add rsp, 64
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_58_5_end:
   loop_59_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_60_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-64], rax
     read_61_19_end:
     sub qword[rbp-64], 1
     if_62_12:
     cmp_62_12:
     cmp qword[rbp-64], 0
     jne if_62_9_end
     if_62_12_code:  ; opt1
       jmp loop_59_5_end
     if_62_9_end:
     if_65_12:
     cmp_65_12:
     cmp qword[rbp-64], 4
     jg if_65_9_end
     if_65_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_66_13_end:
       jmp loop_59_5
     if_65_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_69_9_end:
       mov rdx, qword[rbp-64]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_70_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_71_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_72_9_end:
   jmp loop_59_5
   loop_59_5_end:
mov rax, 60
mov rdi, 0
syscall
