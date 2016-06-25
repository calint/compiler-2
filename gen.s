section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
;   print(prompt.len prompt){  [56:2]
        mov ecx,prompt
        mov edx,prompt.len
        mov ebx,1
        mov eax,4
        int 0x80
    _end_print_666:
    _loop_692:  ; [57:2]
;       print(plus name){  [58:3]
;         plus(read -2):eax{  [58:9]
;           read(name.len name):ebx{  [58:14]
                mov esi,name
                mov edx,name.len
                xor eax,eax
                xor edi,edi
                syscall
                mov ebx,eax
            _end_read_711:
              mov eax,ebx
              add eax,-2
          _end_plus_706:
            mov ecx,name
            mov edx,eax
            mov ebx,1
            mov eax,4
            int 0x80
        _end_print_700:
      jmp _loop_692
    _end_loop_692:  ; [57:2]
;   exit(){  [60:2]
        mov eax,1
        int 0x80
    _end_exit_745:

section .data
dd1 dd 1
dd0 dd 0

; --- field prompt  [1:6]
prompt:    db '___echo__type_new_line__ctrl_c__to_break____'
prompt.len equ $-prompt

; --- field name  [2:9]
name:    db '............................................................'
name.len equ $-name

; --- table names [4:7]
names:
names_end:


section .bss
stk resd 256
stk.end
