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
;   print(prompt.len prompt)  [32:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_510:
    _loop_536:  ; [33:2]
;       if [576]
;         (read(name.len,name)=1)      [35:6]
;         read(name.len name):edi  [35:6]
              mov esi,name  ;  [18:2]
              mov edx,name.len  ;  [19:2]
              xor eax,eax  ;  [20:2]
              xor edi,edi  ;  [21:2]
              syscall  ;  [22:2]
              mov edi,eax  ;  [23:2]
          _end_read_579:
        cmp edi,1  ;  [1:0]
        jne _end_if_576
            jmp _end_loop_536
        _end_if_576:
      jmp _loop_536
    _end_loop_536:  ; [33:2]
;   exit()  [39:2]
        mov eax,1  ;  [27:2]
        int 0x80  ;  [28:2]
    _end_exit_634:

;           max regs in use: 1
;         max frames in use: 3
;          max stack in use: 0
