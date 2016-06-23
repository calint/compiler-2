section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
; main(){  [0]
   _loop_608:
;    print(prompt.len prompt){  [616]
       mov ecx,prompt
       mov edx,prompt.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_616:
;    var ln=
;     read(name.len name):dword[ebp+0]{  [650]
        mov esi,name
        mov edx,name.len
        xor eax,eax
        xor edi,edi
        syscall
        mov dword[ebp+0],eax
      _end_read_650:
;    if [672]
;    orl(eq eq):eax{  [675]
;    eq(ln 1):ebx{  [679]
       cmp dword[ebp+0],1
       cmove ebx,[mem1]
       cmovne ebx,[mem0]
     _end_eq_679:
;    eq(ln 2):ecx{  [688]
       cmp dword[ebp+0],2
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_688:
       mov eax,ebx
       or eax,ecx
     _end_orl_675:
     cmp eax,1
     jne _end_if_672
       jmp _loop_608
     _end_if_672:
;    if [716]
;    eq(ln 3):ecx{  [719]
       cmp dword[ebp+0],3
       cmove ecx,[mem1]
       cmovne ecx,[mem0]
     _end_eq_719:
     cmp ecx,1
     jne _end_if_716
       jmp _end_loop_608
     _end_if_716:
;    print(hello.len hello){  [739]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_739:
;    print(ln name){  [764]
       mov ecx,name
       mov edx,dword[ebp+0]
       mov ebx,1
       mov eax,4
       int 0x80
     _end_print_764:
   jmp _loop_608
   _end_loop_608:
;  exit(){  [783]
     mov eax,1
     int 0x80
   _end_exit_783:
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
stk.end:
