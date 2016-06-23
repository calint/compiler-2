section .text
global _start
_start:
  mov ebp,stk
; main(){  [0]
   _loop_353:
;    print(prompt.len prompt){  [363]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_363:
;    var ln=
;     read(name.len name):ln{  [397]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_397:
;    print(hello.len hello){  [419]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_419:
;    print(ln name){  [444]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_444:
   jmp _loop_353
   _loop_353_end:
;  exit(){  [463]
     mov eax,1
     int 0x80
   _end_exit_463:
; }


section .data
prompt     db 'enter_name_'
prompt.len equ $-prompt
name     db '......................'
name.len equ $-name
hello     db 'hello_'
hello.len equ $-hello

section .bss
stk resd 256
