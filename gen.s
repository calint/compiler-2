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
;   print(prompt.len prompt)  [61:2]
        mov ecx,prompt  ;  [10:2]
        mov edx,prompt.len  ;  [11:2]
        mov ebx,1  ;  [12:2]
        mov eax,4  ;  [13:2]
        int 0x80  ;  [14:2]
    _end_print_803:
    _loop_829:  ; [62:2]
;       var [63:7]
;         len=    [63:7]
;           read(name.len,name)      [63:11]
;           read(name.len name):edi  [63:11]
                mov esi,name  ;  [18:2]
                mov edx,name.len  ;  [19:2]
                xor eax,eax  ;  [20:2]
                xor edi,edi  ;  [21:2]
                syscall  ;  [22:2]
                mov edi,eax  ;  [23:2]
            _end_read_845:
            mov dword[ebp+0],edi  ;  [63:7]
;       if [868]
;         (len=3)      [64:6]
        cmp dword[ebp+0],3  ;  [1:0]
        jne _end_if_868
            jmp _end_loop_829
        _end_if_868:
;       print(len name)  [66:3]
            mov ecx,name  ;  [10:2]
            mov edx,dword[ebp+0]  ;  [11:2]
            mov ebx,1  ;  [12:2]
            mov eax,4  ;  [13:2]
            int 0x80  ;  [14:2]
        _end_print_889:
      jmp _loop_829
    _end_loop_829:  ; [62:2]
;   exit()  [68:2]
        mov eax,1  ;  [27:2]
        int 0x80  ;  [28:2]
    _end_exit_909:

;           max regs in use: 1
;         max frames in use: 3
;          max stack in use: 1
