true equ 1
false equ 0
section .data
align 4
fld: dq 42
str: db 'hello world'
str.len equ $-str
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
f:
   push rbp
   mov rbp,rsp
   mov qword[rbp-40],1
   if_15_5:
   cmp_15_5:
   cmp qword[rbp-40],1
   je if_15_2_end
   jmp if_15_5_code
   if_15_5_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_15_21_end:
   if_15_2_end:
   mov qword[rbp-40],1
   mov qword[rbp-32],2
   mov qword[rbp-24],4
   mov qword[rbp-16],str
   mov qword[rbp-8],str.len
   if_22_5:
   cmp_22_5:
   cmp qword[fld],42
   je if_22_2_end
   jmp if_22_5_code
   if_22_5_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_22_16_end:
   if_22_2_end:
   if_23_5:
   cmp_23_5:
   cmp qword[rbp-40],1
   je if_23_2_end
   jmp if_23_5_code
   if_23_5_code:
       mov rdi,3
       mov rax,60
       syscall
     exit_23_21_end:
   if_23_2_end:
   if_24_5:
   cmp_24_5:
   cmp qword[rbp-32],2
   je if_24_2_end
   jmp if_24_5_code
   if_24_5_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_24_21_end:
   if_24_2_end:
   if_25_5:
   cmp_25_5:
   cmp qword[rbp-24],4
   je if_25_2_end
   jmp if_25_5_code
   if_25_5_code:
       mov rdi,5
       mov rax,60
       syscall
     exit_25_21_end:
   if_25_2_end:
   if_26_5:
   cmp_26_5:
   cmp qword[rbp-16],str
   je if_26_2_end
   jmp if_26_5_code
   if_26_5_code:
       mov rdi,6
       mov rax,60
       syscall
     exit_26_23_end:
   if_26_2_end:
   if_27_5:
   cmp_27_5:
   cmp qword[rbp-8],str.len
   je if_27_2_end
   jmp if_27_5_code
   if_27_5_code:
       mov rdi,7
       mov rax,60
       syscall
     exit_27_27_end:
   if_27_2_end:
   pop rbp
   ret
main:
   mov qword[rbp-40],1
   if_32_5:
   cmp_32_5:
   cmp qword[rbp-40],1
   je if_32_2_end
   jmp if_32_5_code
   if_32_5_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_32_21_end:
   if_32_2_end:
   mov qword[rbp-40],1
   mov qword[rbp-32],2
   mov qword[rbp-24],4
   mov qword[rbp-16],str
   mov qword[rbp-8],str.len
   if_39_5:
   cmp_39_5:
   cmp qword[fld],42
   je if_39_2_end
   jmp if_39_5_code
   if_39_5_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_39_16_end:
   if_39_2_end:
   if_40_5:
   cmp_40_5:
   cmp qword[rbp-40],1
   je if_40_2_end
   jmp if_40_5_code
   if_40_5_code:
       mov rdi,3
       mov rax,60
       syscall
     exit_40_21_end:
   if_40_2_end:
   if_41_5:
   cmp_41_5:
   cmp qword[rbp-32],2
   je if_41_2_end
   jmp if_41_5_code
   if_41_5_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_41_21_end:
   if_41_2_end:
   if_42_5:
   cmp_42_5:
   cmp qword[rbp-24],4
   je if_42_2_end
   jmp if_42_5_code
   if_42_5_code:
       mov rdi,5
       mov rax,60
       syscall
     exit_42_21_end:
   if_42_2_end:
   if_43_5:
   cmp_43_5:
   cmp qword[rbp-16],str
   je if_43_2_end
   jmp if_43_5_code
   if_43_5_code:
       mov rdi,6
       mov rax,60
       syscall
     exit_43_23_end:
   if_43_2_end:
   if_44_5:
   cmp_44_5:
   cmp qword[rbp-8],str.len
   je if_44_2_end
   jmp if_44_5_code
   if_44_5_code:
       mov rdi,7
       mov rax,60
       syscall
     exit_44_27_end:
   if_44_2_end:
   sub rsp,40
   call f
   add rsp,40
     mov rdi,0
     mov rax,60
     syscall
   exit_47_2_end:
