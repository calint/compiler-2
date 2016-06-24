section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
;  print(prompt.len prompt){  [630]
     mov ecx,prompt
     mov edx,prompt.len
     mov ebx,1
     mov eax,4
     int 0x80
   _end_print_630:
   _loop_656:
;    print(read name){  [664]
;    read(name.len name):eax{  [670]
       mov esi,name
       mov edx,name.len
       xor eax,eax
       xor edi,edi
       syscall
     _end_read_670:
       mov ecx,name
       mov edx,eax
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_664:
   jmp _loop_656
   _end_loop_656:
;  exit(){  [700]
     mov eax,1
     int 0x80
   _end_exit_700:
; }


section .data
prompt     db 'echo__type_new_line'
prompt.len equ $-prompt
name     db '............................................................'
name.len equ $-name
mem1 dd 1
mem0 dd 0

section .bss
stk resd 256
stkend:
