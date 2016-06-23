section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
   _loop_620:
;    print(prompt.len prompt){  [630]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_630:
;    var ln=
;     read(name.len name):ln{  [664]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_664:
;    if [686]
;    orl(eq eq):eax{  [689]
;    eq(ln 1):ebx{  [693]
       cmp dword[ebp+0],1
       cmove ebx,[mem1]
       cmovne ebx,[mem0]
     _end_eq_693:
;    eq(ln 2):ecx{  [702]
       cmp dword[ebp+0],2
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_702:
       mov eax,ebx
       or eax,ecx
     _end_orl_689:
     cmp eax,1
     jne _end__if_686
       jmp _loop_620
     _end__if_686:
;    if [734]
;    eq(ln 3):ecx{  [737]
       cmp dword[ebp+0],3
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_737:
     cmp ecx,1
     jne _end__if_734
       jmp _end_loop_620
     _end__if_734:
;    print(hello.len hello){  [765]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_765:
;    print(ln name){  [790]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_790:
   jmp _loop_620
   _end_loop_620:
;  exit(){  [809]
     mov eax,1
     int 0x80
   _end_exit_809:
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
