section .text
global _start
_start:
  mov ebp,stk
; main(){  [0]
   _loop_409:
;    print(prompt.len prompt){  [419]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_419:
;    var ln=
;     read(name.len name):ln{  [453]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_453:
;    if [475]
;    eq(ln 1):eax{  [478]
       cmp dword[ebp+0],1
       cmove eax,[mem1]
       cmovne eax,[mem0]
     _end_eq_478:
     cmp eax,1
     jne _end__if_475
       jmp _loop_409
     _end__if_475:
;    print(hello.len hello){  [509]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_509:
;    print(ln name){  [534]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_534:
   jmp _loop_409
   _end_loop_409:
;  exit(){  [553]
     mov eax,1
     int 0x80
   _end_exit_553:
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
