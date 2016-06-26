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
;   print(prompt.len prompt)  [64:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_976:
    _loop_1020:  ; [66:2]
;       var aa=
        mov dword[ebp+0],1
;       var bb=
        mov dword[ebp+4],2
;       print(plus name)  [73:3]
;         plus(read plus):edi  [73:9]
;           read(name.len name):esi  [73:14]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov esi,eax  ;  [23:2]
            _end_read_1172:
;           plus(plus plus):edx  [73:34]
;             plus(plus plus):ecx  [73:39]
;               plus(bb aa):ebx  [73:44]
                    mov ebx,dword[ebp+4]  ;  [56:2]
                    add ebx,dword[ebp+0]  ;  [57:2]
                _end_plus_1202:
;               plus(bb aa):eax  [73:56]
                    mov eax,dword[ebp+4]  ;  [56:2]
                    add eax,dword[ebp+0]  ;  [57:2]
                _end_plus_1214:
                  mov ecx,ebx  ;  [56:2]
                  add ecx,eax  ;  [57:2]
              _end_plus_1197:
;             plus(1 2):eax  [73:69]
                  mov eax,1  ;  [56:2]
                  add eax,2  ;  [57:2]
              _end_plus_1227:
                mov edx,ecx  ;  [56:2]
                add edx,eax  ;  [57:2]
            _end_plus_1192:
              mov edi,esi  ;  [56:2]
              add edi,edx  ;  [57:2]
          _end_plus_1167:
            mov ecx,name  ;  [10:2]
            mov edx,edi  ;  [11:2]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_1161:
;       exit()  [75:3]
            mov eax,1  ;  [29:2]
            int 0x80  ;  [30:2]
        _end_exit_1250:
      jmp _loop_1020
    _end_loop_1020:  ; [66:2]
;   exit()  [77:2]
        mov eax,1  ;  [29:2]
        int 0x80  ;  [30:2]
    _end_exit_1259:

;   max scratch regs in use: 6
;         max frames in use: 3
;          max stack in use: 2
