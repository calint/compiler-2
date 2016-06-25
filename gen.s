section .data
; -- field prompt  [1:6]
prompt db '___echo__type_new_line__ctrl_c__to_break____'
prompt.len equ $-prompt

; -- field name  [2:9]
name db '............................................................'
name.len equ $-name

; --- table name_to_prompt [4:7]
name_to_prompt:
name_to_prompt.end:
name_to_prompt.len equ $-name_to_prompt


dd1 dd 1
dd0 dd 0

section .bss
stk resd 256
stk.end


section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
;   var a=
    mov dword[ebp+0],7
;   print(a prompt){  [63:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+0]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_871:
;   a=
    mov dword[ebp+0],10
;   print(a prompt){  [65:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+0]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_894:
;   var b=
;   b=
    mov dword[ebp+4],15
;   print(b prompt){  [68:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+4]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_924:
    _loop_969:  ; [70:2]
;       print(plus name){  [73:3]
;         plus(read -2):edi{  [73:9]
;           read(name.len name):esi{  [73:14]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov esi,eax  ;  [23:2]
            _end_read_1051:
              mov edi,esi  ;  [56:2]
              add edi,-2  ;  [57:2]
          _end_plus_1046:
            mov ecx,name  ;  [10:2]
            mov edx,edi  ;  [11:2]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_1040:
;       exit(){  [74:3]
            mov eax,1  ;  [29:2]
            int 0x80  ;  [30:2]
        _end_exit_1083:
      jmp _loop_969
    _end_loop_969:  ; [70:2]
;   exit(){  [76:2]
        mov eax,1  ;  [29:2]
        int 0x80  ;  [30:2]
    _end_exit_1092:
