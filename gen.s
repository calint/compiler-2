section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
; main(){  [0]
;  print(prompt.len prompt){  [652]
     mov ecx,prompt
     mov edx,prompt.len
     mov ebx,1
     mov eax,4
     int 0x80
   _end_print_652:
   _loop_678:
;    print(plus name){  [686]
;    plus(read -2):eax{  [692]
;    read(name.len name):ebx{  [697]
       mov esi,name
       mov edx,name.len
       xor eax,eax
       xor edi,edi
       syscall
       mov ebx,eax
     _end_read_697:
       mov eax,ebx
       add eax,-2
     _end_plus_692:
       mov ecx,name
       mov edx,eax
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_686:
   jmp _loop_678
   _end_loop_678:
;  exit(){  [731]
     mov eax,1
     int 0x80
   _end_exit_731:
; }


section .data
prompt     db 'echo__type_new_line__ctrl_c__to_break____'
prompt.len equ $-prompt
name     db '............................................................'
name.len equ $-name
dd1 dd 1
dd0 dd 0

section .bss
stk resd 256
stk.end
