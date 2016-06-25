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
    mov dword[ebp+0],1
;   var b=
    mov dword[ebp+4],2
;   var c=
    mov dword[ebp+8],3
;   var d=
    mov dword[ebp+12],4
;   d=plus
;     plus(d plus):d{  [65:4]
;       plus(c plus):edi{  [65:11]
;         plus(a b):esi{  [65:18]
              mov esi,dword[ebp+0]  ;  [56:2]
              add esi,esi  ;  [57:2]
          _end_plus_896:
            mov edi,dword[ebp+0]  ;  [56:2]
            add edi,esi  ;  [57:2]
        _end_plus_889:
          add dword[ebp+12],edi  ;  [57:2]
      _end_plus_882:
;   var c=
;   var a=
    mov dword[ebp+0],7
;   a=a
;   print(a prompt){  [70:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+0]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_933:
;   a=10
    mov dword[ebp+0],10
;   print(a prompt){  [72:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+0]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_956:
;   var b=
;   b=15
    mov dword[ebp+4],15
;   a=b
      mov edi,dword[ebp+4]  ;  [75:2]
      mov dword[ebp+0],edi  ;  [75:2]
;   var d=
    mov edi,dword[ebp+0]  ;  [77:2]
    mov dword[ebp+12],edi  ;  [77:2]
;   print(b prompt){  [79:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+4]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_1011:
    _loop_1074:  ; [82:2]
;       print(plus name){  [85:3]
;         plus(read -2):edi{  [85:9]
;           read(name.len name):esi{  [85:14]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov esi,eax  ;  [23:2]
            _end_read_1156:
              mov edi,esi  ;  [56:2]
              add edi,-2  ;  [57:2]
          _end_plus_1151:
            mov ecx,name  ;  [10:2]
            mov edx,edi  ;  [11:2]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_1145:
;       exit(){  [86:3]
            mov eax,1  ;  [29:2]
            int 0x80  ;  [30:2]
        _end_exit_1188:
      jmp _loop_1074
    _end_loop_1074:  ; [82:2]
;   exit(){  [88:2]
        mov eax,1  ;  [29:2]
        int 0x80  ;  [30:2]
    _end_exit_1197:

;   max scratch regs in use: 2
;         max frames in use: 5
;          max stack in use: 8
