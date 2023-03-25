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
f1:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   mov qword[rbp-8],r15
   neg qword[rbp-8]
   mov rax,qword[rbp-8]
   pop rbp
   ret
f2:
   push rbp
   mov rbp,rsp
   mov r15,qword[rbp+16]
   neg r15
   imul r15,2
   mov qword[rbp-8],r15
   mov rax,qword[rbp-8]
   pop rbp
   ret
f3:
   push rbp
   mov rbp,rsp
   movsx r15,dword[rbp+16]
   neg r15
   imul r15,3
   mov dword[rbp-4],r15d
   movsx rax,dword[rbp-4]
   pop rbp
   ret
f4:
   push rbp
   mov rbp,rsp
   movsx r15,word[rbp+16]
   neg r15
   imul r15,4
   mov word[rbp-2],r15w
   movsx rax,word[rbp-2]
   pop rbp
   ret
f5:
   push rbp
   mov rbp,rsp
   movsx r15,byte[rbp+16]
   neg r15
   imul r15,4
   mov byte[rbp-1],r15b
   movsx rax,byte[rbp-1]
   pop rbp
   ret
main:
   sub rsp,8
   push 1
   call f1
   add rsp,16
   mov qword[rbp-8],rax
   if_29_8:
   cmp_29_8:
   cmp qword[rbp-8],-1
   je if_29_5_end
   jmp if_29_8_code
   if_29_8_code:
       mov rdi,1
       mov rax,60
       syscall
     exit_29_17_end:
   if_29_5_end:
   mov byte[rbp-9],1
   mov word[rbp-11],2
   mov dword[rbp-15],3
   mov qword[rbp-23],4
   sub rsp,24
   movsx r15,byte[rbp-9]
   push r15
   call f5
   add rsp,32
   mov byte[rbp-24],al
   if_37_8:
   cmp_37_8:
   cmp byte[rbp-24],-4
   je if_37_5_end
   jmp if_37_8_code
   if_37_8_code:
       mov rdi,2
       mov rax,60
       syscall
     exit_37_17_end:
   if_37_5_end:
   sub rsp,26
   movsx r15,word[rbp-11]
   push r15
   call f4
   add rsp,34
   mov word[rbp-26],ax
   if_40_8:
   cmp_40_8:
   cmp word[rbp-26],-8
   je if_40_5_end
   jmp if_40_8_code
   if_40_8_code:
       mov rdi,3
       mov rax,60
       syscall
     exit_40_17_end:
   if_40_5_end:
   sub rsp,30
   movsx r15,dword[rbp-15]
   push r15
   call f3
   add rsp,38
   mov dword[rbp-30],eax
   if_43_8:
   cmp_43_8:
   cmp dword[rbp-30],-9
   je if_43_5_end
   jmp if_43_8_code
   if_43_8_code:
       mov rdi,4
       mov rax,60
       syscall
     exit_43_17_end:
   if_43_5_end:
   sub rsp,30
   push qword[rbp-23]
   call f2
   add rsp,38
   mov qword[rbp-8],rax
   if_46_8:
   cmp_46_8:
   cmp qword[rbp-8],-9
   je if_46_5_end
   jmp if_46_8_code
   if_46_8_code:
       mov rdi,0
       mov rax,60
       syscall
     exit_46_17_end:
   if_46_5_end:
   sub rsp,30
   push qword[rbp-23]
   call f2
   add rsp,38
   mov byte[rbp-24],al
   if_49_8:
   cmp_49_8:
   cmp byte[rbp-24],-9
   je if_49_5_end
   jmp if_49_8_code
   if_49_8_code:
       mov rdi,0
       mov rax,60
       syscall
     exit_49_17_end:
   if_49_5_end:
     mov rdi,0
     mov rax,60
     syscall
   exit_51_5_end:
