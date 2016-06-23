section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
   _loop_645:
     _loop_655:
;      print(prompt.len prompt){  [666]
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_666:
;      var ln=
;       read(name.len name):ln{  [701]
          mov esi,name
          mov edx,name.len
          xor eax,eax
          xor edi,edi
          syscall
          mov dword[ebp+0],eax
        _end_read_701:
;      ln=
;       plus(ln 1):ln{  [727]
          add dword[ebp+0],1
        _end_plus_727:
;      if [741]
;      orl(eq eq):eax{  [744]
;      eq(ln 1):ebx{  [748]
         cmp dword[ebp+0],1
         cmove ebx,[mem1]
         cmovne ebx,[mem0]
       _end_eq_748:
;      eq(ln 2):ecx{  [757]
         cmp dword[ebp+0],2
         cmove ecx,[mem1]
         cmovne ecx,[mem0]
       _end_eq_757:
         mov eax,ebx
         or eax,ecx
       _end_orl_744:
       cmp eax,1
       jne _end__if_741
         jmp _loop_655
       _end__if_741:
;      print(hello.len hello){  [792]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_792:
;      print(ln name){  [818]
         mov ecx,name
         mov edx,dword[ebp+0]
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_818:
     jmp _loop_655
     _end_loop_655:
   jmp _loop_645
   _end_loop_645:
;  exit(){  [841]
     mov eax,1
     int 0x80
   _end_exit_841:
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
