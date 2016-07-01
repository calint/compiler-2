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
;   var [59:6]
;     a=    [59:6]
;       1      [59:8]
        mov dword[ebp+0],1  ;  [59:8]
;   var [60:6]
;     b=    [60:6]
;       2      [60:8]
        mov dword[ebp+4],2  ;  [60:8]
;   var [61:6]
;     c=    [61:6]
;       3      [61:8]
        mov dword[ebp+8],3  ;  [61:8]
;   var [62:6]
;     d=    [62:6]
;       4      [62:8]
        mov dword[ebp+12],4  ;  [62:8]
;   a=    [63:2]
;     ((b+c)*d+c*(a+b))*2      [63:4]
;       ((b+c)*d+c*(a+b))      [63:5]
;         (b+c)      [63:6]
          mov edi,dword[ebp+4]  ;  [63:6]
          add edi,dword[ebp+8]  ;  [63:8]
        imul edi,dword[ebp+12]  ;  [63:11]
;         c*(a+b)      [63:14]
          mov esi,dword[ebp+8]  ;  [63:13]
;           (a+b)      [63:16]
            mov edx,dword[ebp+0]  ;  [63:16]
            add edx,dword[ebp+4]  ;  [63:18]
          imul esi,edx  ;  [63:16]
        add edi,esi  ;  [63:14]
      imul edi,2  ;  [63:22]
      mov dword[ebp+0],edi  ;  [63:2]
;   print(prompt.len prompt)  [64:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_856:
    _loop_882:  ; [65:2]
;       print(read name)  [66:3]
;           read(name.len,name)-2      [66:9]
;           read(name.len name):edx  [66:9]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov edx,eax  ;  [23:2]
            _end_read_896:
            sub edx,2  ;  [66:29]
;           name+1*b      [66:31]
            mov ecx,name  ;  [66:31]
;             1*b      [66:37]
              mov edi,1  ;  [66:36]
              imul edi,dword[ebp+4]  ;  [66:38]
            add ecx,edi  ;  [66:37]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_890:
      jmp _loop_882
    _end_loop_882:  ; [65:2]
;   exit()  [68:2]
        mov eax,1  ;  [27:2]
        int 0x80  ;  [28:2]
    _end_exit_932:

;           max regs in use: 3
;         max frames in use: 3
;          max stack in use: 4
