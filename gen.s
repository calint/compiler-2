section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
;  print(prompt.len prompt){  [652]
     mov ecx,prompt
     mov edx,prompt.len
     mov ebx,1
     mov eax,4
     int 0x80
   _end_print_652:
   _loop_678:
;    print(read name){  [686]
;    read(name.len name):eax{  [692]
       mov esi,name
       mov edx,name.len
       xor eax,eax
       xor edi,edi
       syscall
     _end_read_692:
       mov ecx,name
       mov edx,eax
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_686:
   jmp _loop_678
   _end_loop_678:
;  exit(){  [722]
     mov eax,1
     int 0x80
   _end_exit_722:
; }


section .data
prompt     db 'echo__type_new_line__ctrl_c__to_break____'
prompt.len equ $-prompt
name     db '............................................................'
name.len equ $-name
mem1 dd 1
mem0 dd 0

section .bss
stk resd 256
stkend:
