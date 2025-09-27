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
   foo_57_5_end:
   sub rsp, 16
     cmp_58_12:
     cmp qword[rbp-16], 2
     jne bool_false_58_12
     bool_true_58_12:  ; opt1
     mov r15, true
     jmp bool_end_58_12
     bool_false_58_12:
     mov r15, false
     bool_end_58_12:
   push r15
   call assert
   add rsp, 24
   sub rsp, 16
     cmp_59_12:
     cmp qword[rbp-8], 0xb
     jne bool_false_59_12
     bool_true_59_12:  ; opt1
     mov r15, true
     jmp bool_end_59_12
     bool_false_59_12:
     mov r15, false
     bool_end_59_12:
   push r15
   call assert
   add rsp, 24
   mov qword[rbp-24], 0
     mov qword[rbp-24], 0xff
   bar_62_5_end:
   sub rsp, 24
     cmp_63_12:
     cmp qword[rbp-24], 0xff
     jne bool_false_63_12
     bool_true_63_12:  ; opt1
     mov r15, true
     jmp bool_end_63_12
     bool_false_63_12:
     mov r15, false
     bool_end_63_12:
   push r15
   call assert
   add rsp, 32
   mov qword[rbp-32], 1
     mov r15, qword[rbp-32]
     imul r15, 2
     mov qword[rbp-40], r15
   baz_66_13_end:
   sub rsp, 40
     cmp_67_12:
     cmp qword[rbp-40], 2
     jne bool_false_67_12
     bool_true_67_12:  ; opt1
     mov r15, true
     jmp bool_end_67_12
     bool_false_67_12:
     mov r15, false
     bool_end_67_12:
   push r15
   call assert
   add rsp, 48
   mov qword[rbp-48], 1
   mov qword[rbp-56], 2
       mov r15, qword[rbp-48]
       imul r15, 10
       mov qword[rbp-76], r15
       mov r15, qword[rbp-56]
       mov qword[rbp-68], r15
     mov dword[rbp-60], 0xff0000
   sub rsp, 76
     cmp_73_12:
     cmp qword[rbp-76], 10
     jne bool_false_73_12
     bool_true_73_12:  ; opt1
     mov r15, true
     jmp bool_end_73_12
     bool_false_73_12:
     mov r15, false
     bool_end_73_12:
   push r15
   call assert
   add rsp, 84
   sub rsp, 76
     cmp_74_12:
     cmp qword[rbp-68], 2
     jne bool_false_74_12
     bool_true_74_12:  ; opt1
     mov r15, true
     jmp bool_end_74_12
     bool_false_74_12:
     mov r15, false
     bool_end_74_12:
   push r15
   call assert
   add rsp, 84
   sub rsp, 76
     cmp_75_12:
     cmp dword[rbp-60], 0xff0000
     jne bool_false_75_12
     bool_true_75_12:  ; opt1
     mov r15, true
     jmp bool_end_75_12
     bool_false_75_12:
     mov r15, false
     bool_end_75_12:
   push r15
   call assert
   add rsp, 84
     mov r15, qword[rbp-48]
     mov qword[rbp-92], r15
     neg qword[rbp-92]
     mov r15, qword[rbp-56]
     mov qword[rbp-84], r15
     neg qword[rbp-84]
     mov r15, qword[rbp-92]
     mov qword[rbp-76], r15
     mov r15, qword[rbp-84]
     mov qword[rbp-68], r15
   sub rsp, 92
     cmp_79_12:
     cmp qword[rbp-76], -1
     jne bool_false_79_12
     bool_true_79_12:  ; opt1
     mov r15, true
     jmp bool_end_79_12
     bool_false_79_12:
     mov r15, false
     bool_end_79_12:
   push r15
   call assert
   add rsp, 100
   sub rsp, 92
     cmp_80_12:
     cmp qword[rbp-68], -2
     jne bool_false_80_12
     bool_true_80_12:  ; opt1
     mov r15, true
     jmp bool_end_80_12
     bool_false_80_12:
     mov r15, false
     bool_end_80_12:
   push r15
   call assert
   add rsp, 100
       mov r15, qword[rbp-76]
       mov qword[rbp-112], r15
       mov r15, qword[rbp-68]
       mov qword[rbp-104], r15
     mov r15d, dword[rbp-60]
     mov dword[rbp-96], r15d
   sub rsp, 112
     cmp_83_12:
     cmp qword[rbp-112], -1
     jne bool_false_83_12
     bool_true_83_12:  ; opt1
     mov r15, true
     jmp bool_end_83_12
     bool_false_83_12:
     mov r15, false
     bool_end_83_12:
   push r15
   call assert
   add rsp, 120
   sub rsp, 112
     cmp_84_12:
     cmp qword[rbp-104], -2
     jne bool_false_84_12
     bool_true_84_12:  ; opt1
     mov r15, true
     jmp bool_end_84_12
     bool_false_84_12:
     mov r15, false
     bool_end_84_12:
   push r15
   call assert
   add rsp, 120
   sub rsp, 112
     cmp_85_12:
     cmp dword[rbp-96], 0xff0000
     jne bool_false_85_12
     bool_true_85_12:  ; opt1
     mov r15, true
     jmp bool_end_85_12
     bool_false_85_12:
     mov r15, false
     bool_end_85_12:
   push r15
   call assert
   add rsp, 120
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_87_5_end:
   loop_88_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_89_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-120], rax
     read_90_19_end:
     sub qword[rbp-120], 1
     if_91_12:
     cmp_91_12:
     cmp qword[rbp-120], 0
     jne if_91_9_end
     if_91_12_code:  ; opt1
       jmp loop_88_5_end
     if_91_9_end:
     if_94_12:
     cmp_94_12:
     cmp qword[rbp-120], 4
     jg if_94_9_end
     if_94_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_95_13_end:
       jmp loop_88_5
     if_94_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_98_9_end:
       mov rdx, qword[rbp-120]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_99_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_100_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_101_9_end:
   jmp loop_88_5
   loop_88_5_end:
mov rax, 60
mov rdi, 0
syscall
