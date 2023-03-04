section .data

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
;   var [8:9]
;     a=    [8:9]
;       1 [8:11]
        mov dword[ebp+0],1  ;  [8:11]
;   exit(a)  [9:5]
        mov ebx,dword[ebp+0]  ;  [2:2]
        mov eax,1  ;  [3:2]
        int 0x80  ;  [4:2]
    _end_exit_79:

;           max regs in use: 0
;         max frames in use: 2
;          max stack in use: 1
