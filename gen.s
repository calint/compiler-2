section .data
; -- field prompt  [1:6]
prompt db '  hello    enter name: '
prompt.len equ $-prompt

; -- field name  [2:9]
name db '............................................................'
name.len equ $-name

; -- field prompt2  [3:7]
prompt2 db '  not a name: '
prompt2.len equ $-prompt2

; --- table name_to_prompt [5:7]
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
;   print(prompt.len prompt)  [33:2]
        mov ecx,prompt  ;  [11:2]
        mov edx,prompt.len  ;  [12:2]
        mov ebx,1  ;  [13:2]
        mov eax,4  ;  [14:2]
        int 0x80  ;  [15:2]
    _end_print_529:
    _loop_555:  ; [34:2]
;       var [35:7]
;         len=    [35:7]
;           read(name.len,name)-1      [35:11]
;           read(name.len name):edi  [35:11]
                mov esi,name  ;  [19:2]
                mov edx,name.len  ;  [20:2]
                xor eax,eax  ;  [21:2]
                xor edi,edi  ;  [22:2]
                syscall  ;  [23:2]
                mov edi,eax  ;  [24:2]
            _end_read_571:
            sub edi,1  ;  [35:31]
            mov dword[ebp+0],edi  ;  [35:7]
;       if [596]
          _if_bgn_598:
          cmp dword[ebp+0],0  ;  [1:0]
          jne _if_bgn_644
;             print(prompt.len prompt)  [37:4]
                  mov ecx,prompt  ;  [11:2]
                  mov edx,prompt.len  ;  [12:2]
                  mov ebx,1  ;  [13:2]
                  mov eax,4  ;  [14:2]
                  int 0x80  ;  [15:2]
              _end_print_610:
            jmp _if_end_596
          _if_end_598:
          _if_bgn_644:
          cmp dword[ebp+0],4  ;  [1:0]
          jg _if_else_596
;             print(prompt2.len prompt2)  [39:4]
                  mov ecx,prompt2  ;  [11:2]
                  mov edx,prompt2.len  ;  [12:2]
                  mov ebx,1  ;  [13:2]
                  mov eax,4  ;  [14:2]
                  int 0x80  ;  [15:2]
              _end_print_656:
            jmp _if_end_596
          _if_end_644:
        _if_else_596:
;           if [694]
              _if_bgn_696:
;                 read(name.len,name)      [41:7]
;                 read(name.len name):edi  [41:7]
                      mov esi,name  ;  [19:2]
                      mov edx,name.len  ;  [20:2]
                      xor eax,eax  ;  [21:2]
                      xor edi,edi  ;  [22:2]
                      syscall  ;  [23:2]
                      mov edi,eax  ;  [24:2]
                  _end_read_697:
              cmp edi,1  ;  [1:0]
              jne _if_else_694
                  jmp _end_loop_555
                jmp _if_end_694
              _if_end_696:
            _if_else_694:
            _if_end_694:
;           print(len name)  [43:4]
                mov ecx,name  ;  [11:2]
                mov edx,dword[ebp+0]  ;  [12:2]
                mov ebx,1  ;  [13:2]
                mov eax,4  ;  [14:2]
                int 0x80  ;  [15:2]
            _end_print_734:
        _if_end_596:
      jmp _loop_555
    _end_loop_555:  ; [34:2]
;   exit()  [46:2]
        mov eax,1  ;  [28:2]
        int 0x80  ;  [29:2]
    _end_exit_758:

;           max regs in use: 1
;         max frames in use: 4
;          max stack in use: 1
