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
;   var d=4  [62:6]
    mov dword[ebp+12],4
;   a=    [63:2]
;     (b+c)*d      [63:4]
;       (b+c)      [63:5]
        mov edi,dword[ebp+4]  ;  [63:5]
        add edi,dword[ebp+8]  ;  [63:7]
      imul edi,dword[ebp+12]  ;  [63:10]
      mov dword[ebp+0],edi  ;  [63:2]
;   a=    [64:2]
;     b      [64:4]
      mov edi,dword[ebp+4]  ;  [64:4]
      mov dword[ebp+0],edi  ;  [64:4]
;   a=    [65:2]
;     2      [65:4]
      mov dword[ebp+0],2  ;  [65:4]
;   a=    [66:2]
;     name.len      [66:4]
      mov dword[ebp+0],name.len  ;  [66:4]
;   a=    [67:2]
;     name.len-1      [67:4]
      mov edi,name.len  ;  [67:4]
      sub edi,1  ;  [67:13]
      mov dword[ebp+0],edi  ;  [67:2]
;   a=    [68:2]
;     b*c+d-5      [68:4]
      mov edi,dword[ebp+4]  ;  [68:4]
      imul edi,dword[ebp+8]  ;  [68:6]
      add edi,dword[ebp+12]  ;  [68:8]
      sub edi,5  ;  [68:10]
      mov dword[ebp+0],edi  ;  [68:2]
;   print(prompt.len prompt)  [69:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_892:
    _loop_918:  ; [70:2]
;       print(read name)  [71:3]
;           read(name.len,name)-2      [71:9]
;           read(name.len name):edx  [71:9]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov edx,eax  ;  [23:2]
            _end_read_932:
            sub edx,2  ;  [71:29]
;           name+1*b      [71:31]
            mov ecx,name  ;  [71:31]
;             1*b      [71:37]
              mov edi,1  ;  [71:36]
              imul edi,dword[ebp+4]  ;  [71:38]
            add ecx,edi  ;  [71:37]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_926:
      jmp _loop_918
    _end_loop_918:  ; [70:2]
;   exit()  [73:2]
        mov eax,1  ;  [27:2]
        int 0x80  ;  [28:2]
    _end_exit_968:

;           max regs in use: 3
;         max frames in use: 3
;          max stack in use: 4
