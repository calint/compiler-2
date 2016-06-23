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
;      if [724]
;      orl(eq eq):eax{  [727]
;      eq(ln 1):ebx{  [731]
         cmp dword[ebp+0],1
         cmove ebx,[mem1]
         cmovne ebx,[mem0]
       _end_eq_731:
;      eq(ln 2):ecx{  [740]
         cmp dword[ebp+0],2
         cmove ecx,[mem1]
         cmovne ecx,[mem0]
       _end_eq_740:
         mov eax,ebx
         or eax,ecx
       _end_orl_727:
       cmp eax,1
       jne _end__if_724
         jmp _loop_655
       _end__if_724:
;      print(hello.len hello){  [775]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_775:
;      print(ln name){  [801]
         mov ecx,name
         mov edx,dword[ebp+0]
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_801:
     jmp _loop_655
     _end_loop_655:
   jmp _loop_645
   _end_loop_645:
;  exit(){  [824]
     mov eax,1
     int 0x80
   _end_exit_824:
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
