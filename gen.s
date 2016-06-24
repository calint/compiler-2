section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
   _loop_608:
;    print(read name){  [616]
;    read(name.len name):eax{  [622]
       mov esi,name
       mov edx,name.len
       xor eax,eax
       xor edi,edi
       syscall
     _end_read_622:
       mov ecx,name
       mov edx,eax
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_616:
   jmp _loop_608
   _end_loop_608:
;  exit(){  [652]
     mov eax,1
     int 0x80
   _end_exit_652:
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
