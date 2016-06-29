section .data
; -- field prompt  [1:6]
prompt db '  hello    enter name: '
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
stk.end:


section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
;   var a=1  [59:6]
    mov dword[ebp+0],1
;   var b=2  [60:6]
    mov dword[ebp+4],2
;   var c=3  [61:6]
    mov dword[ebp+8],3
;   a=    [62:2]
;     =1      [62:5]
    mov dword[ebp+0],1  ;  [62:4]
;   print(prompt.len prompt)  [63:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_793:
    _loop_819:  ; [64:2]
;       print(read name)  [65:3]
;         read(name.len name):edi  [65:9]
              mov esi,name  ;  [18:2]
              mov edx,name.len  ;  [19:2]
              xor eax,eax  ;  [20:2]
              xor edi,edi  ;  [21:2]
              syscall  ;  [22:2]
              mov edi,eax  ;  [23:2]
          _end_read_833:
            mov ecx,name  ;  [10:2]
            mov edx,edi  ;  [11:2]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_827:
      jmp _loop_819
    _end_loop_819:  ; [64:2]
;   exit()  [67:2]
        mov eax,1  ;  [27:2]
        int 0x80  ;  [28:2]
    _end_exit_863:

;           max regs in use: 1
;         max frames in use: 3
;          max stack in use: 3
