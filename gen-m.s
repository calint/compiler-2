section .data
hello db 'hello world',10,''
hello.len equ $-hello
one db 'one',10,''
one.len equ $-one
num dd 4
section .bss
stk resd 256
stk.end:
section .text
bits 32
align 4
global _start
_start:
mov ebp,stk
mov esp,stk.end
     mov ecx,hello
     mov edx,hello.len
     mov ebx,1
     mov eax,4
     int 0x80
   print_20_5_end:
   mov dword[ebp+0],1
   mov dword[ebp+4],2
   mov dword[ebp+8],3
   mov dword[ebp+12],4
   mov dword[ebp+16],10
   loop_26_5:
     if_27_10:
     cmp_27_10:
     cmp dword[num],0
     jne if_27_7_end
     if_27_10_code:  ; opt1
       jmp loop_26_5_end
     if_27_7_end:
       mov ecx,one
       mov edx,dword[num]
       mov ebx,1
       mov eax,4
       int 0x80
     print_28_7_end:
     sub dword[num],1
   jmp loop_26_5
   loop_26_5_end:
     mov ebx,0
     mov eax,1
     int 0x80
   exit_31_5_end:
