section .data
hello db 'hello world',10,''
hello.len equ $-hello
one db 'one',10,''
one.len equ $-one
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
   print_19_5_end:
   mov dword[ebp+0],1
   mov dword[ebp+4],2
   mov dword[ebp+8],3
   mov dword[ebp+12],4
   mov dword[ebp+16],10
   loop_25_5:
     if_26_10:
     cmp_26_10:
     cmp dword[ebp+16],0
     jne if_26_7_end
     if_26_10_code:  ; opt1
       jmp loop_25_5_end
     if_26_7_end:
     mov edi,dword[ebp+0]
     add edi,dword[ebp+4]
     imul edi,dword[ebp+8]
     add edi,dword[ebp+12]
     imul edi,2
     mov dword[ebp+20],edi
     sub dword[ebp+16],1
     if_29_10:
     cmp_29_10:
     cmp dword[ebp+16],1
     je if_29_7_end
     if_29_10_code:  ; opt1
       jmp loop_25_5
     if_29_7_end:
       mov ecx,one
       mov edx,one.len
       mov ebx,1
       mov eax,4
       int 0x80
     print_30_7_end:
   jmp loop_25_5
   loop_25_5_end:
     mov ebx,0
     mov eax,1
     int 0x80
   exit_32_5_end:
