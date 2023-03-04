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
;   var [34:9]
;     a=    [34:9]
;       1 [34:11]
        mov dword[ebp+0],1  ;  [34:11]
;   var [35:9]
;     b=    [35:9]
;       2 [35:11]
        mov dword[ebp+4],2  ;  [35:11]
;   var [36:9]
;     c=    [36:9]
;       3 [36:11]
        mov dword[ebp+8],3  ;  [36:11]
;   var [37:9]
;     d=    [37:9]
;       4 [37:11]
        mov dword[ebp+12],4  ;  [37:11]
;   var [38:9]
;     r=    [38:9]
;        ((b+c)*d+ c*(a+b))*2 [39:7]
;         ((b+c)*d+ c*(a+b)) [39:8]
;           (b+c) [39:9]
            mov edi,dword[ebp+4]  ;  [39:9]
            add edi,dword[ebp+8]  ;  [39:11]
          imul edi,dword[ebp+12]  ;  [39:14]
;            c*(a+b) [40:8]
            mov esi,dword[ebp+8]  ;  [40:7]
;             (a+b) [40:10]
              mov edx,dword[ebp+0]  ;  [40:10]
              add edx,dword[ebp+4]  ;  [40:12]
            imul esi,edx  ;  [40:10]
          add edi,esi  ;  [40:8]
        imul edi,2  ;  [40:16]
        mov dword[ebp+16],edi  ;  [38:9]
;   exit(r)  [41:5]
        mov ebx,dword[ebp+16]  ;  [28:2]
        mov eax,1  ;  [29:2]
        int 0x80  ;  [30:2]
    _end_exit_639:

;           max regs in use: 3
;         max frames in use: 2
;          max stack in use: 5
