section .text
global _start
_start:
  mov ebp,stk
; main(){  [0]
   _loop_506:
     _loop_516:
;      print(prompt.len prompt){  [527]
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_527:
;      var ln=
;       read(name.len name):ln{  [562]
          mov esi,name
          mov edx,name.len
          xor eax,eax
          xor edi,edi
          syscall
          mov dword[ebp+0],eax
        _end_read_562:
;      if [585]
;      orl(eq eq):eax{  [588]
;      eq(ln 1):ebx{  [592]
         cmp dword[ebp+0],1
         cmove ebx,[mem1]
         cmovne ebx,[mem0]
       _end_eq_592:
;      eq(ln 2):ecx{  [601]
         cmp dword[ebp+0],2
         cmove ecx,[mem1]
         cmovne ecx,[mem0]
       _end_eq_601:
         mov eax,ebx
         or eax,ecx
       _end_orl_588:
       cmp eax,1
       jne _end__if_585
         jmp _loop_516
       _end__if_585:
;      print(hello.len hello){  [636]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_636:
;      print(ln name){  [662]
         mov ecx,name
         mov edx,dword[ebp+0]
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_662:
     jmp _loop_516
     _end_loop_516:
   jmp _loop_506
   _end_loop_506:
;  exit(){  [685]
     mov eax,1
     int 0x80
   _end_exit_685:
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
