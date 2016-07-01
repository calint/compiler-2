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
;   var [62:6]
;     a=    [62:6]
;       1      [62:8]
        mov dword[ebp+0],1  ;  [62:8]
;   var [63:6]
;     b=    [63:6]
;       2      [63:8]
        mov dword[ebp+4],2  ;  [63:8]
;   var [64:6]
;     c=    [64:6]
;       3      [64:8]
        mov dword[ebp+8],3  ;  [64:8]
;   var [65:6]
;     d=    [65:6]
;       4      [65:8]
        mov dword[ebp+12],4  ;  [65:8]
;   a=    [66:2]
;     ((b+c)*d+c*(a+b))*2      [66:4]
;       ((b+c)*d+c*(a+b))      [66:5]
;         (b+c)      [66:6]
          mov edi,dword[ebp+4]  ;  [66:6]
          add edi,dword[ebp+8]  ;  [66:8]
        imul edi,dword[ebp+12]  ;  [66:11]
;         c*(a+b)      [66:14]
          mov esi,dword[ebp+8]  ;  [66:13]
;           (a+b)      [66:16]
            mov edx,dword[ebp+0]  ;  [66:16]
            add edx,dword[ebp+4]  ;  [66:18]
          imul esi,edx  ;  [66:16]
        add edi,esi  ;  [66:14]
      imul edi,2  ;  [66:22]
      mov dword[ebp+0],edi  ;  [66:2]
;   print(prompt.len prompt)  [67:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_878:
    _loop_904:  ; [68:2]
;       print(read name)  [69:3]
;           read(name.len,name)-2      [69:9]
;           read(name.len name):edx  [69:9]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov edx,eax  ;  [23:2]
            _end_read_918:
            sub edx,2  ;  [69:29]
;           name+1*b      [69:31]
            mov ecx,name  ;  [69:31]
;             1*b      [69:37]
              mov edi,1  ;  [69:36]
              imul edi,dword[ebp+4]  ;  [69:38]
            add ecx,edi  ;  [69:37]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_912:
      jmp _loop_904
    _end_loop_904:  ; [68:2]
;   exit()  [71:2]
        mov eax,1  ;  [27:2]
        int 0x80  ;  [28:2]
    _end_exit_954:

;           max regs in use: 3
;         max frames in use: 3
;          max stack in use: 4
