section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
   _loop_608:
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
       cmp dword[ebp+0],1
       cmove ebx,[mem1]
       cmovne ebx,[mem0]
       cmp dword[ebp+0],2
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
       mov eax,ebx
       or eax,ecx
     cmp eax,1
     jne _end__if672
       jmp _loop_608
       cmp dword[ebp+0],3
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     cmp ecx,1
     jne _end__if716
       jmp _end_loop_608
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
   jmp _loop_608
     mov eax,1
     int 0x80


section .data
prompt     db 'enter_name_'
prompt.len equ $-prompt
name     db '......................'
name.len equ $-name
hello     db 'hello_'
hello.len equ $-hello
mem1 dd 1
mem0 dd 0

section .bss
stk resd 256
stkend:
