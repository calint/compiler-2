section .data
section .bss
stk resd 256
stk.end:
section .text
bits 32
global _start
_start:
mov ebp,stk
mov esp,stk.end
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
   if_17_8:
   cmp_17_9:
   cmp dword[ebp+0],1
   jne cmp_17_34
   cmp_17_18:
   cmp dword[ebp+4],2
   je if_17_8_code
   cmp_17_25:
   cmp dword[ebp+8],3
   je if_17_8_code  ; opt2
   cmp_17_34:
   cmp dword[ebp+12],4
   jne if_17_5_end
   if_17_8_code:  ; opt1
       mov ebx,0
       mov eax,1
       int 0x80
     exit_18_9_end:
   if_17_5_end:
   mov dword[ebp+24],10
   loop_20_5:
     if_21_10:
     cmp_21_10:
     cmp dword[ebp+24],0
     jne if_21_7_end
     if_21_10_code:  ; opt1
       jmp loop_20_5_end
     if_21_7_end:
     sub dword[ebp+24],1
     jmp loop_20_5
   jmp loop_20_5
   loop_20_5_end:
     mov ebx,1
     mov eax,1
     int 0x80
   exit_26_5_end:
