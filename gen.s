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
;     b*c+d-5      [63:4]
      mov edi,dword[ebp+4]  ;  [63:4]
      imul edi,dword[ebp+8]  ;  [63:6]
      add edi,dword[ebp+12]  ;  [63:8]
      sub edi,5  ;  [63:10]
      mov dword[ebp+0],edi  ;  [63:2]
;   print(prompt.len prompt)  [64:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_840:
    _loop_866:  ; [65:2]
;       print(read name)  [66:3]
;           read(name.len,name)-2      [66:9]
;           read(name.len name):edx  [66:9]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov edx,eax  ;  [23:2]
            _end_read_880:
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
        _end_print_874:
      jmp _loop_866
    _end_loop_866:  ; [65:2]
;   exit()  [68:2]
        mov eax,1  ;  [27:2]
        int 0x80  ;  [28:2]
    _end_exit_916:

;           max regs in use: 3
;         max frames in use: 3
;          max stack in use: 4
