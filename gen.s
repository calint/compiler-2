section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
; main(){  [0]
   _loop_668:
;    print(prompt.len prompt){  [677]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_677:
;    var ln=
;     read(name.len name):dword[ebp+0]{  [712]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_712:
;    if [735]
;    orl(eq eq):eax{  [738]
;    eq(ln 1):ebx{  [742]
       cmp dword[ebp+0],1
       cmove ebx,[mem1]
       cmovne ebx,[mem0]
     _end_eq_742:
;    eq(ln 2):ecx{  [751]
       cmp dword[ebp+0],2
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_751:
       mov eax,ebx
       or eax,ecx
     _end_orl_738:
     cmp eax,1
     jne _end_if_735
       jmp _loop_668
     _end_if_735:
;    if [782]
;    eq(ln 3):ecx{  [785]
       cmp dword[ebp+0],3
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_785:
     cmp ecx,1
     jne _end_if_782
       jmp _end_loop_668
     _end_if_782:
;    print(hello.len hello){  [807]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_807:
;    print(ln name){  [833]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_833:
   jmp _loop_668
   _end_loop_668:
;  exit(){  [854]
     mov eax,1
     int 0x80
   _end_exit_854:
; }


section .data
prompt     db 'enter_name_'
prompt.len equ $-prompt
name     db '......................'
name.len equ $-name
hello     db 'hello_'
hello.len equ $-hello

section .data
mem1 dd 1
mem0 dd 0

section .bss
stk resd 256
stk.end:
