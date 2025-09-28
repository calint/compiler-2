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
     mov qword[rsp-16], 0
     mov qword[rsp-8], 0
     mov qword[rsp-16], 0b10
     mov qword[rsp-8], 0xb
   foo_65_5_end:
     cmp_66_12:
     cmp qword[rsp-16], 2
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
     cmp qword[rsp-8], 0xb
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
   mov qword[rsp-24], 0
     if_54_8_70_5:
     cmp_54_8_70_5:
     cmp qword[rsp-24], 0
     jne if_54_5_70_5_end
     if_54_8_70_5_code:  ; opt1
       jmp bar_70_5_end
     if_54_5_70_5_end:
     mov qword[rsp-24], 0xff
   bar_70_5_end:
     cmp_71_12:
     cmp qword[rsp-24], 0
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
   mov qword[rsp-24], 1
     if_54_8_74_5:
     cmp_54_8_74_5:
     cmp qword[rsp-24], 0
     jne if_54_5_74_5_end
     if_54_8_74_5_code:  ; opt1
       jmp bar_74_5_end
     if_54_5_74_5_end:
     mov qword[rsp-24], 0xff
   bar_74_5_end:
     cmp_75_12:
     cmp qword[rsp-24], 0xff
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
   mov qword[rsp-32], 1
     mov r15, qword[rsp-32]
     imul r15, 2
     mov qword[rsp-40], r15
   baz_78_13_end:
     cmp_79_12:
     cmp qword[rsp-40], 2
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
     mov r15, 1
     imul r15, 2
     mov qword[rsp-40], r15
   baz_81_9_end:
     cmp_82_12:
     cmp qword[rsp-40], 2
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
       mov r15, 2
       imul r15, 2
       mov qword[rsp-56], r15
     baz_84_23_end:
     mov qword[rsp-48], 0
     cmp_85_12:
     cmp qword[rsp-56], 4
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
   mov qword[rsp-64], 1
   mov qword[rsp-72], 2
       mov r15, qword[rsp-64]
       imul r15, 10
       mov qword[rsp-92], r15
       mov r15, qword[rsp-72]
       mov qword[rsp-84], r15
     mov dword[rsp-76], 0xff0000
     cmp_91_12:
     cmp qword[rsp-92], 10
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
     cmp qword[rsp-84], 2
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
     cmp_93_12:
     cmp dword[rsp-76], 0xff0000
     jne bool_false_93_12
     bool_true_93_12:  ; opt1
     mov r15, true
     jmp bool_end_93_12
     bool_false_93_12:
     mov r15, false
     bool_end_93_12:
     if_36_8_93_5:
     cmp_36_8_93_5:
     cmp r15, 0
     jne if_36_5_93_5_end
     if_36_8_93_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_93_5_end:
     if_36_5_93_5_end:
   assert_93_5_end:
     mov r15, qword[rsp-64]
     mov qword[rsp-108], r15
     neg qword[rsp-108]
     mov r15, qword[rsp-72]
     mov qword[rsp-100], r15
     neg qword[rsp-100]
     mov r15, qword[rsp-108]
     mov qword[rsp-92], r15
     mov r15, qword[rsp-100]
     mov qword[rsp-84], r15
     cmp_97_12:
     cmp qword[rsp-92], -1
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
     cmp_98_12:
     cmp qword[rsp-84], -2
     jne bool_false_98_12
     bool_true_98_12:  ; opt1
     mov r15, true
     jmp bool_end_98_12
     bool_false_98_12:
     mov r15, false
     bool_end_98_12:
     if_36_8_98_5:
     cmp_36_8_98_5:
     cmp r15, 0
     jne if_36_5_98_5_end
     if_36_8_98_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_98_5_end:
     if_36_5_98_5_end:
   assert_98_5_end:
       mov r15, qword[rsp-92]
       mov qword[rsp-128], r15
       mov r15, qword[rsp-84]
       mov qword[rsp-120], r15
     mov r15d, dword[rsp-76]
     mov dword[rsp-112], r15d
     cmp_101_12:
     cmp qword[rsp-128], -1
     jne bool_false_101_12
     bool_true_101_12:  ; opt1
     mov r15, true
     jmp bool_end_101_12
     bool_false_101_12:
     mov r15, false
     bool_end_101_12:
     if_36_8_101_5:
     cmp_36_8_101_5:
     cmp r15, 0
     jne if_36_5_101_5_end
     if_36_8_101_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_101_5_end:
     if_36_5_101_5_end:
   assert_101_5_end:
     cmp_102_12:
     cmp qword[rsp-120], -2
     jne bool_false_102_12
     bool_true_102_12:  ; opt1
     mov r15, true
     jmp bool_end_102_12
     bool_false_102_12:
     mov r15, false
     bool_end_102_12:
     if_36_8_102_5:
     cmp_36_8_102_5:
     cmp r15, 0
     jne if_36_5_102_5_end
     if_36_8_102_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_102_5_end:
     if_36_5_102_5_end:
   assert_102_5_end:
     cmp_103_12:
     cmp dword[rsp-112], 0xff0000
     jne bool_false_103_12
     bool_true_103_12:  ; opt1
     mov r15, true
     jmp bool_end_103_12
     bool_false_103_12:
     mov r15, false
     bool_end_103_12:
     if_36_8_103_5:
     cmp_36_8_103_5:
     cmp r15, 0
     jne if_36_5_103_5_end
     if_36_8_103_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_36_17_103_5_end:
     if_36_5_103_5_end:
   assert_103_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_105_5_end:
   loop_106_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_107_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rsp-136], rax
     read_108_19_end:
     sub qword[rsp-136], 1
     if_109_12:
     cmp_109_12:
     cmp qword[rsp-136], 0
     jne if_111_19
     if_109_12_code:  ; opt1
       jmp loop_106_5_end
     jmp if_109_9_end
     if_111_19:
     cmp_111_19:
     cmp qword[rsp-136], 4
     jg if_else_109_9
     if_111_19_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_112_13_end:
       jmp loop_106_5
     jmp if_109_9_end
     if_else_109_9:
           mov rdx, prompt3.len
           mov rsi, prompt3
           mov rax, 1
           mov rdi, 1
           syscall
         print_115_13_end:
           mov rdx, qword[rsp-136]
           mov rsi, input
           mov rax, 1
           mov rdi, 1
           syscall
         print_116_13_end:
           mov rdx, dot.len
           mov rsi, dot
           mov rax, 1
           mov rdi, 1
           syscall
         print_117_13_end:
           mov rdx, nl.len
           mov rsi, nl
           mov rax, 1
           mov rdi, 1
           syscall
         print_118_13_end:
     if_109_9_end:
   jmp loop_106_5
   loop_106_5_end:
mov rax, 60
mov rdi, 0
syscall
