section .text
global _start
_start:
  mov ebp,stk
  mov esp,stkend
; main(){  [0]
;  var b=
;  var c=
   mov dword[ebp+4],2
;  b=
   mov dword[ebp+0],3
;  c=
   mov dword[ebp+4],4
   _loop_674:
     _loop_684:
;      print(prompt.len prompt){  [695]
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_695:
;      var ln=
;       read(name.len name):ln{  [730]
          mov esi,name
          mov edx,name.len
          xor eax,eax
          xor edi,edi
          syscall
          mov dword[ebp+8],eax
        _end_read_730:
;      if [753]
;      orl(eq eq):eax{  [756]
;      eq(ln 1):ebx{  [760]
         cmp dword[ebp+8],1
         cmove ebx,[mem1]
         cmovne ebx,[mem0]
       _end_eq_760:
;      eq(ln 2):ecx{  [769]
         cmp dword[ebp+8],2
         cmove ecx,[mem1]
         cmovne ecx,[mem0]
       _end_eq_769:
         mov eax,ebx
         or eax,ecx
       _end_orl_756:
       cmp eax,1
       jne _end__if_753
         jmp _loop_684
       _end__if_753:
;      print(hello.len hello){  [804]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_804:
;      print(ln name){  [830]
         mov ecx,name
         mov edx,dword[ebp+8]
         mov ebx,1
         mov eax,4
         int 0x80
       _end_print_830:
     jmp _loop_684
     _end_loop_684:
   jmp _loop_674
   _end_loop_674:
;  exit(){  [853]
     mov eax,1
     int 0x80
   _end_exit_853:
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
