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
;      orl(eq eq):eax{  [539]
;      eq(ln 1):ebx{  [543]
         cmp dword[ebp+0],1
         cmove ebx,[mem1]
         cmovne ebx,[mem0]
       _end_eq_543:
;      eq(ln 2):ecx{  [552]
         cmp dword[ebp+0],2
         cmove ecx,[mem1]
         cmovne ecx,[mem0]
       _end_eq_552:
         mov eax,ebx
         or eax,ecx
       _end_orl_539:
       cmp eax,1
       jne _end__if_536
         jmp _loop_467
       _end__if_536:
;      print(hello.len hello){  [587]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_587:
;      print(ln name){  [613]
         mov ecx,name
         mov edx,dword[ebp+0]
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_613:
     jmp _loop_467
     _end_loop_467:
   jmp _loop_457
   _end_loop_457:
;  exit(){  [636]
     mov eax,1
     int 0x80
   _end_exit_636:
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
