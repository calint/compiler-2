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
;   var c=
;   var a=
    mov dword[ebp+4],7
;   a=a
;   print(a prompt){  [65:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+4]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_885:
;   a=10
    mov dword[ebp+4],10
;   print(a prompt){  [67:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+4]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_908:
;   var b=
;   b=15
    mov dword[ebp+8],15
;   a=b
      mov edi,dword[ebp+8]  ;  [70:2]
      mov dword[ebp+4],edi  ;  [70:2]
;   var d=
    mov edi,dword[ebp+4]  ;  [72:2]
    mov dword[ebp+12],edi  ;  [72:2]
;   print(b prompt){  [73:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+8]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_962:
    _loop_1007:  ; [75:2]
;       print(plus name){  [78:3]
;         plus(read -2):edi{  [78:9]
;           read(name.len name):esi{  [78:14]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov esi,eax  ;  [23:2]
            _end_read_1089:
              mov edi,esi  ;  [56:2]
              add edi,-2  ;  [57:2]
          _end_plus_1084:
            mov ecx,name  ;  [10:2]
            mov edx,edi  ;  [11:2]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_1078:
;       exit(){  [79:3]
            mov eax,1  ;  [29:2]
            int 0x80  ;  [30:2]
        _end_exit_1121:
      jmp _loop_1007
    _end_loop_1007:  ; [75:2]
;   exit(){  [81:2]
        mov eax,1  ;  [29:2]
        int 0x80  ;  [30:2]
    _end_exit_1130:

;   max scratch regs in use: 2
;         max frames in use: 5
;          max stack in use: 4
