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
   foo_65_5_end:
     cmp_66_12:
     cmp qword[rbp-16], 2
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
     cmp_67_12:
     cmp qword[rbp-8], 0xb
     jne bool_false_67_12
     bool_true_67_12:  ; opt1
     mov r15, true
     jmp bool_end_67_12
     bool_false_67_12:
     mov r15, false
     bool_end_67_12:
     if_36_8_67_5:
     cmp_36_8_67_5:
     cmp r15, 0
     jne if_36_5_67_5_end
     if_36_8_67_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_67_5_end:
     if_36_5_67_5_end:
   assert_67_5_end:
   mov qword[rbp-24], 0
     if_54_8_70_5:
     cmp_54_8_70_5:
     cmp qword[rbp-24], 0
     jne if_54_5_70_5_end
     if_54_8_70_5_code:  ; opt1
       jmp bar_70_5_end
     if_54_5_70_5_end:
     mov qword[rbp-24], 0xff
   bar_70_5_end:
     cmp_71_12:
     cmp qword[rbp-24], 0
     jne bool_false_71_12
     bool_true_71_12:  ; opt1
     mov r15, true
     jmp bool_end_71_12
     bool_false_71_12:
     mov r15, false
     bool_end_71_12:
     if_36_8_71_5:
     cmp_36_8_71_5:
     cmp r15, 0
     jne if_36_5_71_5_end
     if_36_8_71_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_71_5_end:
     if_36_5_71_5_end:
   assert_71_5_end:
   mov qword[rbp-24], 1
     if_54_8_74_5:
     cmp_54_8_74_5:
     cmp qword[rbp-24], 0
     jne if_54_5_74_5_end
     if_54_8_74_5_code:  ; opt1
       jmp bar_74_5_end
     if_54_5_74_5_end:
     mov qword[rbp-24], 0xff
   bar_74_5_end:
     cmp_75_12:
     cmp qword[rbp-24], 0xff
     jne bool_false_75_12
     bool_true_75_12:  ; opt1
     mov r15, true
     jmp bool_end_75_12
     bool_false_75_12:
     mov r15, false
     bool_end_75_12:
     if_36_8_75_5:
     cmp_36_8_75_5:
     cmp r15, 0
     jne if_36_5_75_5_end
     if_36_8_75_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_75_5_end:
     if_36_5_75_5_end:
   assert_75_5_end:
   mov qword[rbp-32], 1
     mov r15, qword[rbp-32]
     imul r15, 2
     mov qword[rbp-40], r15
   baz_78_13_end:
     cmp_79_12:
     cmp qword[rbp-40], 2
     jne bool_false_79_12
     bool_true_79_12:  ; opt1
     mov r15, true
     jmp bool_end_79_12
     bool_false_79_12:
     mov r15, false
     bool_end_79_12:
     if_36_8_79_5:
     cmp_36_8_79_5:
     cmp r15, 0
     jne if_36_5_79_5_end
     if_36_8_79_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_79_5_end:
     if_36_5_79_5_end:
   assert_79_5_end:
   mov qword[rbp-48], 1
   mov qword[rbp-56], 2
       mov r15, qword[rbp-48]
       imul r15, 10
       mov qword[rbp-76], r15
       mov r15, qword[rbp-56]
       mov qword[rbp-68], r15
     mov dword[rbp-60], 0xff0000
     cmp_85_12:
     cmp qword[rbp-76], 10
     jne bool_false_85_12
     bool_true_85_12:  ; opt1
     mov r15, true
     jmp bool_end_85_12
     bool_false_85_12:
     mov r15, false
     bool_end_85_12:
     if_36_8_85_5:
     cmp_36_8_85_5:
     cmp r15, 0
     jne if_36_5_85_5_end
     if_36_8_85_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_85_5_end:
     if_36_5_85_5_end:
   assert_85_5_end:
     cmp_86_12:
     cmp qword[rbp-68], 2
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
     cmp dword[rbp-60], 0xff0000
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
     cmp_91_12:
     cmp qword[rbp-76], -1
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
     cmp qword[rbp-68], -2
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
       mov r15, qword[rbp-76]
       mov qword[rbp-112], r15
       mov r15, qword[rbp-68]
       mov qword[rbp-104], r15
     mov r15d, dword[rbp-60]
     mov dword[rbp-96], r15d
     cmp_95_12:
     cmp qword[rbp-112], -1
     jne bool_false_95_12
     bool_true_95_12:  ; opt1
     mov r15, true
     jmp bool_end_95_12
     bool_false_95_12:
     mov r15, false
     bool_end_95_12:
     if_36_8_95_5:
     cmp_36_8_95_5:
     cmp r15, 0
     jne if_36_5_95_5_end
     if_36_8_95_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_95_5_end:
     if_36_5_95_5_end:
   assert_95_5_end:
     cmp_96_12:
     cmp qword[rbp-104], -2
     jne bool_false_96_12
     bool_true_96_12:  ; opt1
     mov r15, true
     jmp bool_end_96_12
     bool_false_96_12:
     mov r15, false
     bool_end_96_12:
     if_36_8_96_5:
     cmp_36_8_96_5:
     cmp r15, 0
     jne if_36_5_96_5_end
     if_36_8_96_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_96_5_end:
     if_36_5_96_5_end:
   assert_96_5_end:
     cmp_97_12:
     cmp dword[rbp-96], 0xff0000
     jne bool_false_97_12
     bool_true_97_12:  ; opt1
     mov r15, true
     jmp bool_end_97_12
     bool_false_97_12:
     mov r15, false
     bool_end_97_12:
     if_36_8_97_5:
     cmp_36_8_97_5:
     cmp r15, 0
     jne if_36_5_97_5_end
     if_36_8_97_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_97_5_end:
     if_36_5_97_5_end:
   assert_97_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_99_5_end:
   loop_100_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_101_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-120], rax
     read_102_19_end:
     sub qword[rbp-120], 1
     if_103_12:
     cmp_103_12:
     cmp qword[rbp-120], 0
     jne if_105_19
     if_103_12_code:  ; opt1
       jmp loop_100_5_end
     jmp if_103_9_end
     if_105_19:
     cmp_105_19:
     cmp qword[rbp-120], 4
     jg if_else_103_9
     if_105_19_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_106_13_end:
       jmp loop_100_5
     jmp if_103_9_end
     if_else_103_9:
           mov rdx, prompt3.len
           mov rsi, prompt3
           mov rax, 1
           mov rdi, 1
           syscall
         print_109_13_end:
           mov rdx, qword[rbp-120]
           mov rsi, input
           mov rax, 1
           mov rdi, 1
           syscall
         print_110_13_end:
           mov rdx, dot.len
           mov rsi, dot
           mov rax, 1
           mov rdi, 1
           syscall
         print_111_13_end:
           mov rdx, nl.len
           mov rsi, nl
           mov rax, 1
           mov rdi, 1
           syscall
         print_112_13_end:
     if_103_9_end:
   jmp loop_100_5
   loop_100_5_end:
mov rax, 60
mov rdi, 0
syscall
