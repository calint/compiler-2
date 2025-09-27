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
main:
     mov qword[rbp-16], 0
     mov qword[rbp-8], 0
     mov qword[rbp-16], 0b10
     mov qword[rbp-8], 0xb
   foo_64_5_end:
     cmp_65_12:
     cmp qword[rbp-16], 2
     jne bool_false_65_12
     bool_true_65_12:  ; opt1
     mov r15, true
     jmp bool_end_65_12
     bool_false_65_12:
     mov r15, false
     bool_end_65_12:
     if_36_8_65_5:
     cmp_36_8_65_5:
     cmp r15, 0
     jne if_36_5_65_5_end
     if_36_8_65_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_65_5_end:
     if_36_5_65_5_end:
   assert_65_5_end:
     cmp_66_12:
     cmp qword[rbp-8], 0xb
     jne bool_false_66_12
     bool_true_66_12:  ; opt1
     mov r15, true
     jmp bool_end_66_12
     bool_false_66_12:
     mov r15, false
     bool_end_66_12:
     if_36_8_66_5:
     cmp_36_8_66_5:
     cmp r15, 0
     jne if_36_5_66_5_end
     if_36_8_66_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_66_5_end:
     if_36_5_66_5_end:
   assert_66_5_end:
   mov qword[rbp-24], 0
     mov qword[rbp-24], 0xff
   bar_69_5_end:
     cmp_70_12:
     cmp qword[rbp-24], 0xff
     jne bool_false_70_12
     bool_true_70_12:  ; opt1
     mov r15, true
     jmp bool_end_70_12
     bool_false_70_12:
     mov r15, false
     bool_end_70_12:
     if_36_8_70_5:
     cmp_36_8_70_5:
     cmp r15, 0
     jne if_36_5_70_5_end
     if_36_8_70_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_70_5_end:
     if_36_5_70_5_end:
   assert_70_5_end:
   mov qword[rbp-32], 1
     mov r15, qword[rbp-32]
     imul r15, 2
     mov qword[rbp-40], r15
   baz_73_13_end:
     cmp_74_12:
     cmp qword[rbp-40], 2
     jne bool_false_74_12
     bool_true_74_12:  ; opt1
     mov r15, true
     jmp bool_end_74_12
     bool_false_74_12:
     mov r15, false
     bool_end_74_12:
     if_36_8_74_5:
     cmp_36_8_74_5:
     cmp r15, 0
     jne if_36_5_74_5_end
     if_36_8_74_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_74_5_end:
     if_36_5_74_5_end:
   assert_74_5_end:
   mov qword[rbp-48], 1
   mov qword[rbp-56], 2
       mov r15, qword[rbp-48]
       imul r15, 10
       mov qword[rbp-76], r15
       mov r15, qword[rbp-56]
       mov qword[rbp-68], r15
     mov dword[rbp-60], 0xff0000
     cmp_80_12:
     cmp qword[rbp-76], 10
     jne bool_false_80_12
     bool_true_80_12:  ; opt1
     mov r15, true
     jmp bool_end_80_12
     bool_false_80_12:
     mov r15, false
     bool_end_80_12:
     if_36_8_80_5:
     cmp_36_8_80_5:
     cmp r15, 0
     jne if_36_5_80_5_end
     if_36_8_80_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_80_5_end:
     if_36_5_80_5_end:
   assert_80_5_end:
     cmp_81_12:
     cmp qword[rbp-68], 2
     jne bool_false_81_12
     bool_true_81_12:  ; opt1
     mov r15, true
     jmp bool_end_81_12
     bool_false_81_12:
     mov r15, false
     bool_end_81_12:
     if_36_8_81_5:
     cmp_36_8_81_5:
     cmp r15, 0
     jne if_36_5_81_5_end
     if_36_8_81_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_81_5_end:
     if_36_5_81_5_end:
   assert_81_5_end:
     cmp_82_12:
     cmp dword[rbp-60], 0xff0000
     jne bool_false_82_12
     bool_true_82_12:  ; opt1
     mov r15, true
     jmp bool_end_82_12
     bool_false_82_12:
     mov r15, false
     bool_end_82_12:
     if_36_8_82_5:
     cmp_36_8_82_5:
     cmp r15, 0
     jne if_36_5_82_5_end
     if_36_8_82_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_82_5_end:
     if_36_5_82_5_end:
   assert_82_5_end:
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
     cmp_86_12:
     cmp qword[rbp-76], -1
     jne bool_false_86_12
     bool_true_86_12:  ; opt1
     mov r15, true
     jmp bool_end_86_12
     bool_false_86_12:
     mov r15, false
     bool_end_86_12:
     if_36_8_86_5:
     cmp_36_8_86_5:
     cmp r15, 0
     jne if_36_5_86_5_end
     if_36_8_86_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_86_5_end:
     if_36_5_86_5_end:
   assert_86_5_end:
     cmp_87_12:
     cmp qword[rbp-68], -2
     jne bool_false_87_12
     bool_true_87_12:  ; opt1
     mov r15, true
     jmp bool_end_87_12
     bool_false_87_12:
     mov r15, false
     bool_end_87_12:
     if_36_8_87_5:
     cmp_36_8_87_5:
     cmp r15, 0
     jne if_36_5_87_5_end
     if_36_8_87_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_87_5_end:
     if_36_5_87_5_end:
   assert_87_5_end:
       mov r15, qword[rbp-76]
       mov qword[rbp-112], r15
       mov r15, qword[rbp-68]
       mov qword[rbp-104], r15
     mov r15d, dword[rbp-60]
     mov dword[rbp-96], r15d
     cmp_90_12:
     cmp qword[rbp-112], -1
     jne bool_false_90_12
     bool_true_90_12:  ; opt1
     mov r15, true
     jmp bool_end_90_12
     bool_false_90_12:
     mov r15, false
     bool_end_90_12:
     if_36_8_90_5:
     cmp_36_8_90_5:
     cmp r15, 0
     jne if_36_5_90_5_end
     if_36_8_90_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_90_5_end:
     if_36_5_90_5_end:
   assert_90_5_end:
     cmp_91_12:
     cmp qword[rbp-104], -2
     jne bool_false_91_12
     bool_true_91_12:  ; opt1
     mov r15, true
     jmp bool_end_91_12
     bool_false_91_12:
     mov r15, false
     bool_end_91_12:
     if_36_8_91_5:
     cmp_36_8_91_5:
     cmp r15, 0
     jne if_36_5_91_5_end
     if_36_8_91_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_91_5_end:
     if_36_5_91_5_end:
   assert_91_5_end:
     cmp_92_12:
     cmp dword[rbp-96], 0xff0000
     jne bool_false_92_12
     bool_true_92_12:  ; opt1
     mov r15, true
     jmp bool_end_92_12
     bool_false_92_12:
     mov r15, false
     bool_end_92_12:
     if_36_8_92_5:
     cmp_36_8_92_5:
     cmp r15, 0
     jne if_36_5_92_5_end
     if_36_8_92_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_92_5_end:
     if_36_5_92_5_end:
   assert_92_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_94_5_end:
   loop_95_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_96_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-120], rax
     read_97_19_end:
     sub qword[rbp-120], 1
     if_98_12:
     cmp_98_12:
     cmp qword[rbp-120], 0
     jne if_100_19
     if_98_12_code:  ; opt1
       jmp loop_95_5_end
     jmp if_98_9_end
     if_100_19:
     cmp_100_19:
     cmp qword[rbp-120], 4
     jg if_else_98_9
     if_100_19_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_101_13_end:
       jmp loop_95_5
     jmp if_98_9_end
     if_else_98_9:
           mov rdx, prompt3.len
           mov rsi, prompt3
           mov rax, 1
           mov rdi, 1
           syscall
         print_104_13_end:
           mov rdx, qword[rbp-120]
           mov rsi, input
           mov rax, 1
           mov rdi, 1
           syscall
         print_105_13_end:
           mov rdx, dot.len
           mov rsi, dot
           mov rax, 1
           mov rdi, 1
           syscall
         print_106_13_end:
           mov rdx, nl.len
           mov rsi, nl
           mov rax, 1
           mov rdi, 1
           syscall
         print_107_13_end:
     if_98_9_end:
   jmp loop_95_5
   loop_95_5_end:
mov rax, 60
mov rdi, 0
syscall
