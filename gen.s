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
;   var aa=
    mov dword[ebp+0],1
;   var ba=
    mov dword[ebp+4],2
;   var ca=
    mov dword[ebp+8],3
;   var da=
    mov dword[ebp+12],4
;   var ea=
    mov dword[ebp+16],5
;   ea=plus
;     plus(da plus):ea{  [66:5]
;       plus(ca plus):edi{  [66:13]
;         plus(aa ba):esi{  [66:21]
              mov esi,dword[ebp+0]  ;  [56:2]
              add esi,dword[ebp+4]  ;  [57:2]
          _end_plus_913:
            mov edi,dword[ebp+8]  ;  [56:2]
            add edi,esi  ;  [57:2]
        _end_plus_905:
          mov esi,dword[ebp+12]  ;  [56:2]
          mov dword[ebp+16],esi  ;  [56:2]
          add dword[ebp+16],edi  ;  [57:2]
      _end_plus_897:
;   var c=
;   var a=
    mov dword[ebp+24],7
;   a=a
;   print(a prompt){  [71:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+24]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_952:
;   a=10
    mov dword[ebp+24],10
;   print(a prompt){  [73:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+24]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_975:
;   var b=
;   b=15
    mov dword[ebp+28],15
;   a=b
      mov edi,dword[ebp+28]  ;  [76:2]
      mov dword[ebp+24],edi  ;  [76:2]
;   var d=
    mov edi,dword[ebp+24]  ;  [78:2]
    mov dword[ebp+32],edi  ;  [78:2]
;   print(b prompt){  [80:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,dword[ebp+28]  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_1030:
    _loop_1093:  ; [83:2]
;       print(plus name){  [86:3]
;         plus(read -2):edi{  [86:9]
;           read(name.len name):esi{  [86:14]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov esi,eax  ;  [23:2]
            _end_read_1175:
              mov edi,esi  ;  [56:2]
              add edi,-2  ;  [57:2]
          _end_plus_1170:
            mov ecx,name  ;  [10:2]
            mov edx,edi  ;  [11:2]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_1164:
;       exit(){  [87:3]
            mov eax,1  ;  [29:2]
            int 0x80  ;  [30:2]
        _end_exit_1207:
      jmp _loop_1093
    _end_loop_1093:  ; [83:2]
;   exit(){  [89:2]
        mov eax,1  ;  [29:2]
        int 0x80  ;  [30:2]
    _end_exit_1216:

;   max scratch regs in use: 2
;         max frames in use: 5
;          max stack in use: 9
