section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
   _loop_648:
     _loop_658:
;      print(prompt.len prompt){  [669]
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_669:
;      var ln=
;       read(name.len name):ln{  [704]
          mov esi,name
          mov edx,name.len
          xor eax,eax
          xor edi,edi
          syscall
          mov dword[ebp+0],eax
        _end_read_704:
;      if [727]
;      orl(eq eq):eax{  [730]
;      eq(ln 1):ebx{  [734]
         cmp dword[ebp+0],1
         cmove ebx,[mem1]
         cmovne ebx,[mem0]
       _end_eq_734:
;      eq(ln 2):ecx{  [743]
         cmp dword[ebp+0],2
         cmove ecx,[mem1]
         cmovne ecx,[mem0]
       _end_eq_743:
         mov eax,ebx
         or eax,ecx
       _end_orl_730:
       cmp eax,1
       jne _end__if_727
         jmp _loop_658
       _end__if_727:
;      print(hello.len hello){  [778]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_778:
;      print(ln name){  [804]
         mov ecx,name
         mov edx,dword[ebp+0]
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_804:
     jmp _loop_658
     _end_loop_658:
   jmp _loop_648
   _end_loop_648:
;  exit(){  [827]
     mov eax,1
     int 0x80
   _end_exit_827:
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
