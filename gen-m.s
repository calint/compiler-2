section .data
hello db 'hello world',10,''
hello.len equ $-hello
section .bss
stk resd 256
stk.end:
section .text
bits 32
global _start
_start:
mov ebp,stk
mov esp,stk.end
     mov ecx,hello
     mov edx,hello.len
     mov ebx,1
     mov eax,4
     int 0x80
   print_18_5_end:
   mov dword[ebp+0],1
   mov dword[ebp+4],2
   mov dword[ebp+8],3
   mov dword[ebp+12],4
   mov edi,dword[ebp+0]
   add edi,dword[ebp+4]
   add edi,dword[ebp+8]
   add edi,dword[ebp+12]
   mov dword[ebp+16],edi
   mov edi,dword[ebp+4]
   add edi,dword[ebp+8]
   imul edi,dword[ebp+12]
   mov esi,dword[ebp+8]
   mov edx,dword[ebp+0]
   add edx,dword[ebp+4]
   imul esi,edx
   add edi,esi
   imul edi,2
   mov dword[ebp+16],edi
   if_28_8:
   cmp_28_9:
   cmp dword[ebp+0],1
   jne cmp_28_34
   cmp_28_18:
   cmp dword[ebp+4],2
   je if_28_8_code
   cmp_28_25:
   cmp dword[ebp+8],3
   je if_28_8_code  ; opt2
   cmp_28_34:
   cmp dword[ebp+12],4
   jne if_28_5_end
   if_28_8_code:  ; opt1
       mov ebx,0
       mov eax,1
       int 0x80
     exit_29_9_end:
   if_28_5_end:
   mov dword[ebp+24],10
   loop_31_5:
     if_32_10:
     cmp_32_10:
     cmp dword[ebp+24],0
     jne if_32_7_end
     if_32_10_code:  ; opt1
       jmp loop_31_5_end
     if_32_7_end:
     sub dword[ebp+24],1
     jmp loop_31_5
   jmp loop_31_5
   loop_31_5_end:
     mov ebx,1
     mov eax,1
     int 0x80
   exit_37_5_end:
