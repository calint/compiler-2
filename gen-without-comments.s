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
     mov qword[rbp-16], 0
     mov qword[rbp-8], 0
     mov qword[rbp-16], 0b10
     mov qword[rbp-8], 0xb
   foo_53_5_end:
   sub rsp, 16
     cmp_54_12:
     cmp qword[rbp-16], 2
     jne bool_false_54_12
     bool_true_54_12:  ; opt1
     mov r15, true
     jmp bool_end_54_12
     bool_false_54_12:
     mov r15, false
     bool_end_54_12:
   push r15
   call assert
   add rsp, 24
   sub rsp, 16
     cmp_55_12:
     cmp qword[rbp-8], 0xb
     jne bool_false_55_12
     bool_true_55_12:  ; opt1
     mov r15, true
     jmp bool_end_55_12
     bool_false_55_12:
     mov r15, false
     bool_end_55_12:
   push r15
   call assert
   add rsp, 24
   mov qword[rbp-24], 0
     mov qword[rbp-24], 0xff
   bar_58_5_end:
   sub rsp, 24
     cmp_59_12:
     cmp qword[rbp-24], 0xff
     jne bool_false_59_12
     bool_true_59_12:  ; opt1
     mov r15, true
     jmp bool_end_59_12
     bool_false_59_12:
     mov r15, false
     bool_end_59_12:
   push r15
   call assert
   add rsp, 32
   mov qword[rbp-32], 1
   mov qword[rbp-40], 2
       mov r15, qword[rbp-32]
       imul r15, 10
       mov qword[rbp-60], r15
       mov r15, qword[rbp-40]
       mov qword[rbp-52], r15
     mov dword[rbp-44], 0xff0000
   sub rsp, 60
     cmp_65_12:
     cmp qword[rbp-60], 10
     jne bool_false_65_12
     bool_true_65_12:  ; opt1
     mov r15, true
     jmp bool_end_65_12
     bool_false_65_12:
     mov r15, false
     bool_end_65_12:
   push r15
   call assert
   add rsp, 68
   sub rsp, 60
     cmp_66_12:
     cmp qword[rbp-52], 2
     jne bool_false_66_12
     bool_true_66_12:  ; opt1
     mov r15, true
     jmp bool_end_66_12
     bool_false_66_12:
     mov r15, false
     bool_end_66_12:
   push r15
   call assert
   add rsp, 68
   sub rsp, 60
     cmp_67_12:
     cmp dword[rbp-44], 0xff0000
     jne bool_false_67_12
     bool_true_67_12:  ; opt1
     mov r15, true
     jmp bool_end_67_12
     bool_false_67_12:
     mov r15, false
     bool_end_67_12:
   push r15
   call assert
   add rsp, 68
     mov r15, qword[rbp-32]
     mov qword[rbp-76], r15
     neg qword[rbp-76]
     mov r15, qword[rbp-40]
     mov qword[rbp-68], r15
     neg qword[rbp-68]
     mov r15, qword[rbp-76]
     mov qword[rbp-60], r15
     mov r15, qword[rbp-68]
     mov qword[rbp-52], r15
   sub rsp, 76
     cmp_71_12:
     cmp qword[rbp-60], -1
     jne bool_false_71_12
     bool_true_71_12:  ; opt1
     mov r15, true
     jmp bool_end_71_12
     bool_false_71_12:
     mov r15, false
     bool_end_71_12:
   push r15
   call assert
   add rsp, 84
   sub rsp, 76
     cmp_72_12:
     cmp qword[rbp-52], -2
     jne bool_false_72_12
     bool_true_72_12:  ; opt1
     mov r15, true
     jmp bool_end_72_12
     bool_false_72_12:
     mov r15, false
     bool_end_72_12:
   push r15
   call assert
   add rsp, 84
       mov r15, qword[rbp-60]
       mov qword[rbp-96], r15
       mov r15, qword[rbp-52]
       mov qword[rbp-88], r15
     mov r15d, dword[rbp-44]
     mov dword[rbp-80], r15d
   sub rsp, 96
     cmp_75_12:
     cmp qword[rbp-96], -1
     jne bool_false_75_12
     bool_true_75_12:  ; opt1
     mov r15, true
     jmp bool_end_75_12
     bool_false_75_12:
     mov r15, false
     bool_end_75_12:
   push r15
   call assert
   add rsp, 104
   sub rsp, 96
     cmp_76_12:
     cmp qword[rbp-88], -2
     jne bool_false_76_12
     bool_true_76_12:  ; opt1
     mov r15, true
     jmp bool_end_76_12
     bool_false_76_12:
     mov r15, false
     bool_end_76_12:
   push r15
   call assert
   add rsp, 104
   sub rsp, 96
     cmp_77_12:
     cmp dword[rbp-80], 0xff0000
     jne bool_false_77_12
     bool_true_77_12:  ; opt1
     mov r15, true
     jmp bool_end_77_12
     bool_false_77_12:
     mov r15, false
     bool_end_77_12:
   push r15
   call assert
   add rsp, 104
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_79_5_end:
   loop_80_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_81_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-104], rax
     read_82_19_end:
     sub qword[rbp-104], 1
     if_83_12:
     cmp_83_12:
     cmp qword[rbp-104], 0
     jne if_83_9_end
     if_83_12_code:  ; opt1
       jmp loop_80_5_end
     if_83_9_end:
     if_86_12:
     cmp_86_12:
     cmp qword[rbp-104], 4
     jg if_86_9_end
     if_86_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_87_13_end:
       jmp loop_80_5
     if_86_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_90_9_end:
       mov rdx, qword[rbp-104]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_91_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_92_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_93_9_end:
   jmp loop_80_5
   loop_80_5_end:
mov rax, 60
mov rdi, 0
syscall
