section .text
global _start
_start:
  mov ebp,stk
; main(){  [0]
   _loop_457:
     _loop_467:
;      print(prompt.len prompt){  [478]
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_478:
;      var ln=
;       read(name.len name):ln{  [513]
          mov esi,name
          mov edx,name.len
          xor eax,eax
          xor edi,edi
          syscall
          mov dword[ebp+0],eax
        _end_read_513:
;      if [536]
;      eq(ln 1):eax{  [539]
         cmp dword[ebp+0],1
         jne else
         mov eax,1
         jmp endif
         else:
         xor eax,eax
         endif:
       _end_eq_539:
       cmp eax,1
       jne _end__if_536
         jmp _loop_467
       _end__if_536:
;      print(hello.len hello){  [573]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_573:
;      print(ln name){  [599]
         mov ecx,name
         mov edx,dword[ebp+0]
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_599:
     jmp _loop_467
     _end_loop_467:
   jmp _loop_457
   _end_loop_457:
;  exit(){  [622]
     mov eax,1
     int 0x80
   _end_exit_622:
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
