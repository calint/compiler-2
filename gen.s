section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
   _loop_648:
;    print(prompt.len prompt){  [658]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_658:
;    var ln=
;     read(name.len name):ln{  [692]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_692:
;    if [714]
;    orl(eq eq):eax{  [717]
;    eq(ln 1):ebx{  [721]
       cmp dword[ebp+0],1
       cmove ebx,[mem1]
       cmovne ebx,[mem0]
     _end_eq_721:
;    eq(ln 2):ecx{  [730]
       cmp dword[ebp+0],2
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_730:
       mov eax,ebx
       or eax,ecx
     _end_orl_717:
     cmp eax,1
     jne _end__if_714
       jmp _loop_648
     _end__if_714:
;    if [762]
;    eq(ln 3):ecx{  [765]
       cmp dword[ebp+0],3
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_765:
     cmp ecx,1
     jne _end__if_762
       jmp _end_loop_648
     _end__if_762:
;    print(hello.len hello){  [793]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_793:
;    print(ln name){  [818]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_818:
   jmp _loop_648
   _end_loop_648:
;  exit(){  [837]
     mov eax,1
     int 0x80
   _end_exit_837:
; }


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
