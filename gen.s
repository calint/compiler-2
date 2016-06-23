section .text
global _start
_start:
  mov ebp,stk
; main(){  [0]
   _loop_353:
     readname:
;    print(prompt.len prompt){  [379]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_379:
;    var ln=
;     read(name.len name):ln{  [413]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_413:
     cmp dword[ebp+0],1
     je readname
;    print(hello.len hello){  [462]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_462:
;    print(ln name){  [487]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_487:
   jmp _loop_353
   _end_loop_353:
;  exit(){  [506]
     mov eax,1
     int 0x80
   _end_exit_506:
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
