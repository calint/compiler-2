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
   mov qword[rbp-8], 1
   mov qword[rbp-16], 2
       mov r15, qword[rbp-8]
       mov qword[rbp-36], r15
       mov r15, qword[rbp-36]
       imul r15, 10
       mov qword[rbp-36], r15
       mov r15, qword[rbp-16]
       mov qword[rbp-28], r15
     mov dword[rbp-20], 0xff0000
   sub rsp, 36
     cmp_45_12:
     cmp qword[rbp-36], 10
     jne false_45_12
     true_45_12:  ; opt1
     mov r15, 1
     jmp end_45_12
     false_45_12:
     mov r15, 0
     end_45_12:
   push r15
   call assert
   add rsp, 44
   sub rsp, 36
     cmp_46_12:
     cmp qword[rbp-28], 2
     jne false_46_12
     true_46_12:  ; opt1
     mov r15, 1
     jmp end_46_12
     false_46_12:
     mov r15, 0
     end_46_12:
   push r15
   call assert
   add rsp, 44
   sub rsp, 36
     cmp_47_12:
     cmp dword[rbp-20], 0xff0000
     jne false_47_12
     true_47_12:  ; opt1
     mov r15, 1
     jmp end_47_12
     false_47_12:
     mov r15, 0
     end_47_12:
   push r15
   call assert
   add rsp, 44
     mov qword[rbp-52], -1
     mov qword[rbp-44], -2
     mov r15, qword[rbp-52]
     mov qword[rbp-36], r15
     mov r15, qword[rbp-44]
     mov qword[rbp-28], r15
   sub rsp, 52
     cmp_51_12:
     cmp qword[rbp-36], -1
     jne false_51_12
     true_51_12:  ; opt1
     mov r15, 1
     jmp end_51_12
     false_51_12:
     mov r15, 0
     end_51_12:
   push r15
   call assert
   add rsp, 60
   sub rsp, 52
     cmp_52_12:
     cmp qword[rbp-28], -2
     jne false_52_12
     true_52_12:  ; opt1
     mov r15, 1
     jmp end_52_12
     false_52_12:
     mov r15, 0
     end_52_12:
   push r15
   call assert
   add rsp, 60
       mov r15, qword[rbp-36]
       mov qword[rbp-72], r15
       mov r15, qword[rbp-28]
       mov qword[rbp-64], r15
     mov r15d, dword[rbp-20]
     mov dword[rbp-56], r15d
   sub rsp, 72
     cmp_55_12:
     cmp qword[rbp-72], -1
     jne false_55_12
     true_55_12:  ; opt1
     mov r15, 1
     jmp end_55_12
     false_55_12:
     mov r15, 0
     end_55_12:
   push r15
   call assert
   add rsp, 80
   sub rsp, 72
     cmp_56_12:
     cmp qword[rbp-64], -2
     jne false_56_12
     true_56_12:  ; opt1
     mov r15, 1
     jmp end_56_12
     false_56_12:
     mov r15, 0
     end_56_12:
   push r15
   call assert
   add rsp, 80
   sub rsp, 72
     cmp_57_12:
     cmp dword[rbp-56], 0xff0000
     jne false_57_12
     true_57_12:  ; opt1
     mov r15, 1
     jmp end_57_12
     false_57_12:
     mov r15, 0
     end_57_12:
   push r15
   call assert
   add rsp, 80
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_59_5_end:
   loop_60_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_61_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-80], rax
     read_62_19_end:
     sub qword[rbp-80], 1
     if_63_12:
     cmp_63_12:
     cmp qword[rbp-80], 0
     jne if_63_9_end
     if_63_12_code:  ; opt1
       jmp loop_60_5_end
     if_63_9_end:
     if_66_12:
     cmp_66_12:
     cmp qword[rbp-80], 4
     jg if_66_9_end
     if_66_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_67_13_end:
       jmp loop_60_5
     if_66_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_70_9_end:
       mov rdx, qword[rbp-80]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_71_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_72_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_73_9_end:
   jmp loop_60_5
   loop_60_5_end:
mov rax, 60
mov rdi, 0
syscall
