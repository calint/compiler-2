section .data
; -- field prompt  [2:7]
prompt db '  hello    enter name: '
prompt.len equ $-prompt

; -- field name  [3:9]
name db '............................................................'
name.len equ $-name

; --- table name_to_prompt [5:7]
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
;   print(prompt.len prompt)  [65:2]
        mov ecx,prompt  ;  [11:2]
        mov edx,prompt.len  ;  [12:2]
        mov ebx,1  ;  [13:2]
        mov eax,4  ;  [14:2]
        int 0x80  ;  [15:2]
    _end_print_1019:
    _loop_1063:  ; [67:2]
;       var aaa=1  [76:7]
        mov dword[ebp+0],1
;       var a=3  [77:7]
        mov dword[ebp+4],3
;       var b=2  [78:7]
        mov dword[ebp+8],2
;       print(plus name)  [82:3]
;         plus(read plus):edi  [82:9]
;           read(name.len name):esi  [82:14]
                mov esi,name  ;  [19:2]
                mov edx,name.len  ;  [20:2]
                xor eax,eax  ;  [21:2]
                xor edi,edi  ;  [22:2]
                syscall  ;  [23:2]
                mov esi,eax  ;  [24:2]
            _end_read_1377:
;           plus(plus plus):edx  [82:34]
;             plus(plus plus):ecx  [82:39]
;               plus(b a):ebx  [82:44]
                    mov ebx,dword[ebp+8]  ;  [57:2]
                    add ebx,dword[ebp+4]  ;  [58:2]
                _end_plus_1407:
;               plus(b a):eax  [82:54]
                    mov eax,dword[ebp+8]  ;  [57:2]
                    add eax,dword[ebp+4]  ;  [58:2]
                _end_plus_1417:
                  mov ecx,ebx  ;  [57:2]
                  add ecx,eax  ;  [58:2]
              _end_plus_1402:
;             plus(1 2):eax  [82:65]
                  mov eax,1  ;  [57:2]
                  add eax,2  ;  [58:2]
              _end_plus_1428:
                mov edx,ecx  ;  [57:2]
                add edx,eax  ;  [58:2]
            _end_plus_1397:
              mov edi,esi  ;  [57:2]
              add edi,edx  ;  [58:2]
          _end_plus_1372:
            mov ecx,name  ;  [11:2]
            mov edx,edi  ;  [12:2]
            mov ebx,1  ;  [13:2]
            mov eax,4  ;  [14:2]
            int 0x80  ;  [15:2]
        _end_print_1366:
;       exit()  [84:3]
            mov eax,1  ;  [30:2]
            int 0x80  ;  [31:2]
        _end_exit_1451:
      jmp _loop_1063
    _end_loop_1063:  ; [67:2]
;   exit()  [86:2]
        mov eax,1  ;  [30:2]
        int 0x80  ;  [31:2]
    _end_exit_1460:

;           max regs in use: 6
;         max frames in use: 3
;          max stack in use: 3
