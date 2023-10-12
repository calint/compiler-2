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
     mov qword[rbp-16], 0xa
     mov qword[rbp-8], 0xb
   foo_47_5_end:
   sub rsp, 16
     cmp_48_12:
     cmp qword[rbp-16], 0xa
     jne false_48_12
     true_48_12:  ; opt1
     mov r15, 1
     jmp end_48_12
     false_48_12:
     mov r15, 0
     end_48_12:
   push r15
   call assert
   add rsp, 24
   sub rsp, 16
     cmp_49_12:
     cmp qword[rbp-8], 0xb
     jne false_49_12
     true_49_12:  ; opt1
     mov r15, 1
     jmp end_49_12
     false_49_12:
     mov r15, 0
     end_49_12:
   push r15
   call assert
   add rsp, 24
   mov qword[rbp-24], 1
   mov qword[rbp-32], 2
       mov r15, qword[rbp-24]
       imul r15, 10
       mov qword[rbp-52], r15
       mov r15, qword[rbp-32]
       mov qword[rbp-44], r15
     mov dword[rbp-36], 0xff0000
   sub rsp, 52
     cmp_55_12:
     cmp qword[rbp-52], 10
     jne false_55_12
     true_55_12:  ; opt1
     mov r15, 1
     jmp end_55_12
     false_55_12:
     mov r15, 0
     end_55_12:
   push r15
   call assert
   add rsp, 60
   sub rsp, 52
     cmp_56_12:
     cmp qword[rbp-44], 2
     jne false_56_12
     true_56_12:  ; opt1
     mov r15, 1
     jmp end_56_12
     false_56_12:
     mov r15, 0
     end_56_12:
   push r15
   call assert
   add rsp, 60
   sub rsp, 52
     cmp_57_12:
     cmp dword[rbp-36], 0xff0000
     jne false_57_12
     true_57_12:  ; opt1
     mov r15, 1
     jmp end_57_12
     false_57_12:
     mov r15, 0
     end_57_12:
   push r15
   call assert
   add rsp, 60
     mov r15, qword[rbp-24]
     mov qword[rbp-68], r15
     neg qword[rbp-68]
     mov r15, qword[rbp-32]
     mov qword[rbp-60], r15
     neg qword[rbp-60]
     mov r15, qword[rbp-68]
     mov qword[rbp-52], r15
     mov r15, qword[rbp-60]
     mov qword[rbp-44], r15
   sub rsp, 68
     cmp_61_12:
     cmp qword[rbp-52], -1
     jne false_61_12
     true_61_12:  ; opt1
     mov r15, 1
     jmp end_61_12
     false_61_12:
     mov r15, 0
     end_61_12:
   push r15
   call assert
   add rsp, 76
   sub rsp, 68
     cmp_62_12:
     cmp qword[rbp-44], -2
     jne false_62_12
     true_62_12:  ; opt1
     mov r15, 1
     jmp end_62_12
     false_62_12:
     mov r15, 0
     end_62_12:
   push r15
   call assert
   add rsp, 76
       mov r15, qword[rbp-52]
       mov qword[rbp-88], r15
       mov r15, qword[rbp-44]
       mov qword[rbp-80], r15
     mov r15d, dword[rbp-36]
     mov dword[rbp-72], r15d
   sub rsp, 88
     cmp_65_12:
     cmp qword[rbp-88], -1
     jne false_65_12
     true_65_12:  ; opt1
     mov r15, 1
     jmp end_65_12
     false_65_12:
     mov r15, 0
     end_65_12:
   push r15
   call assert
   add rsp, 96
   sub rsp, 88
     cmp_66_12:
     cmp qword[rbp-80], -2
     jne false_66_12
     true_66_12:  ; opt1
     mov r15, 1
     jmp end_66_12
     false_66_12:
     mov r15, 0
     end_66_12:
   push r15
   call assert
   add rsp, 96
   sub rsp, 88
     cmp_67_12:
     cmp dword[rbp-72], 0xff0000
     jne false_67_12
     true_67_12:  ; opt1
     mov r15, 1
     jmp end_67_12
     false_67_12:
     mov r15, 0
     end_67_12:
   push r15
   call assert
   add rsp, 96
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_69_5_end:
   loop_70_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_71_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-96], rax
     read_72_19_end:
     sub qword[rbp-96], 1
     if_73_12:
     cmp_73_12:
     cmp qword[rbp-96], 0
     jne if_73_9_end
     if_73_12_code:  ; opt1
       jmp loop_70_5_end
     if_73_9_end:
     if_76_12:
     cmp_76_12:
     cmp qword[rbp-96], 4
     jg if_76_9_end
     if_76_12_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_77_13_end:
       jmp loop_70_5
     if_76_9_end:
       mov rdx, prompt3.len
       mov rsi, prompt3
       mov rax, 1
       mov rdi, 1
       syscall
     print_80_9_end:
       mov rdx, qword[rbp-96]
       mov rsi, input
       mov rax, 1
       mov rdi, 1
       syscall
     print_81_9_end:
       mov rdx, dot.len
       mov rsi, dot
       mov rax, 1
       mov rdi, 1
       syscall
     print_82_9_end:
       mov rdx, nl.len
       mov rsi, nl
       mov rax, 1
       mov rdi, 1
       syscall
     print_83_9_end:
   jmp loop_70_5
   loop_70_5_end:
mov rax, 60
mov rdi, 0
syscall
