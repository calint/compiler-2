section .data
align 4
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
   mov qword[rbp-39],1
   if_12_5:
   cmp_12_5:
   cmp qword[rbp-39],1
   je if_12_2_end
   jmp if_12_5_code
   if_12_5_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_12_21_end:
   if_12_2_end:
   mov byte[rbp-40],-2
   movsx r15,byte[rbp-40]
   mov qword[rbp-31],r15
   if_18_5:
   cmp_18_5:
   cmp qword[rbp-31],-2
   je if_18_2_end
   jmp if_18_5_code
   if_18_5_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_18_22_end:
   if_18_2_end:
   if_19_5:
   cmp_19_5:
   movsx r15,byte[rbp-40]
   cmp qword[rbp-31],r15
   je if_19_2_end
   jmp if_19_5_code
   if_19_5_code:
       mov rdi,5
       mov rax,60
       syscall
     exit_19_23_end:
   if_19_2_end:
   mov r15b,byte[rbp-40]
   mov byte[rbp-7],r15b
   if_22_5:
   cmp_22_5:
   cmp byte[rbp-7],-2
   je if_22_2_end
   jmp if_22_5_code
   if_22_5_code:
       mov rdi,6
       mov rax,60
       syscall
     exit_22_22_end:
   if_22_2_end:
   if_23_5:
   cmp_23_5:
   mov r15b,byte[rbp-40]
   cmp byte[rbp-7],r15b
   je if_23_2_end
   jmp if_23_5_code
   if_23_5_code:
       mov rdi,7
       mov rax,60
       syscall
     exit_23_23_end:
   if_23_2_end:
   mov r15b,byte[rbp-7]
   mov byte[rbp-40],r15b
   if_28_5:
   cmp_28_5:
   mov r15b,byte[rbp-7]
   cmp byte[rbp-40],r15b
   je if_28_2_end
   jmp if_28_5_code
   if_28_5_code:
       mov rdi,8
       mov rax,60
       syscall
     exit_28_23_end:
   if_28_2_end:
   mov word[rbp-42],-1
   mov r15w,word[rbp-42]
   mov word[rbp-6],r15w
   if_32_5:
   cmp_32_5:
   cmp word[rbp-6],-1
   je if_32_2_end
   jmp if_32_5_code
   if_32_5_code:
       mov rdi,9
       mov rax,60
       syscall
     exit_32_20_end:
   if_32_2_end:
   if_33_5:
   cmp_33_5:
   mov r15w,word[rbp-42]
   cmp word[rbp-6],r15w
   je if_33_2_end
   jmp if_33_5_code
   if_33_5_code:
       mov rdi,10
       mov rax,60
       syscall
     exit_33_19_end:
   if_33_2_end:
   movsx r15,byte[rbp-40]
   mov word[rbp-6],r15w
   if_36_5:
   cmp_36_5:
   cmp word[rbp-6],-2
   je if_36_2_end
   jmp if_36_5_code
   if_36_5_code:
       mov rdi,9
       mov rax,60
       syscall
     exit_36_20_end:
   if_36_2_end:
   if_37_5:
   cmp_37_5:
   movsx r15w,byte[rbp-40]
   cmp word[rbp-6],r15w
   je if_37_2_end
   jmp if_37_5_code
   if_37_5_code:
       mov rdi,10
       mov rax,60
       syscall
     exit_37_21_end:
   if_37_2_end:
   mov dword[rbp-46],-3
   mov r15d,dword[rbp-46]
   mov dword[rbp-4],r15d
   if_41_5:
   cmp_41_5:
   cmp dword[rbp-4],-3
   je if_41_2_end
   jmp if_41_5_code
   if_41_5_code:
       mov rdi,11
       mov rax,60
       syscall
     exit_41_21_end:
   if_41_2_end:
   if_42_5:
   cmp_42_5:
   mov r15d,dword[rbp-46]
   cmp dword[rbp-4],r15d
   je if_42_2_end
   jmp if_42_5_code
   if_42_5_code:
       mov rdi,12
       mov rax,60
       syscall
     exit_42_21_end:
   if_42_2_end:
   movsx r15,byte[rbp-40]
   mov dword[rbp-4],r15d
   if_45_5:
   cmp_45_5:
   cmp dword[rbp-4],-2
   je if_45_2_end
   jmp if_45_5_code
   if_45_5_code:
       mov rdi,9
       mov rax,60
       syscall
     exit_45_21_end:
   if_45_2_end:
   if_46_5:
   cmp_46_5:
   movsx r15d,byte[rbp-40]
   cmp dword[rbp-4],r15d
   je if_46_2_end
   jmp if_46_5_code
   if_46_5_code:
       mov rdi,10
       mov rax,60
       syscall
     exit_46_22_end:
   if_46_2_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_48_2_end:
