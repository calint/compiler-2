section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
   _loop_614:
;    print(prompt.len prompt){  [622]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_622:
;    var ln=
;     read(name.len name):ln{  [656]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_656:
;    if [678]
;    orl(eq eq):eax{  [681]
;    eq(ln 1):ebx{  [685]
       cmp dword[ebp+0],1
       cmove ebx,[mem1]
       cmovne ebx,[mem0]
     _end_eq_685:
;    eq(ln 2):ecx{  [694]
       cmp dword[ebp+0],2
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_694:
       mov eax,ebx
       or eax,ecx
     _end_orl_681:
     cmp eax,1
     jne _end__if_678
       jmp _loop_614
     _end__if_678:
;    if [722]
;    eq(ln 3):ecx{  [725]
       cmp dword[ebp+0],3
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_725:
     cmp ecx,1
     jne _end__if_722
       jmp _end_loop_614
     _end__if_722:
;    print(hello.len hello){  [754]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_754:
;    print(ln name){  [779]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_779:
   jmp _loop_614
   _end_loop_614:
;  exit(){  [798]
     mov eax,1
     int 0x80
   _end_exit_798:
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
