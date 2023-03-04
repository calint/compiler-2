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
;   var [9:9]
;     b=    [9:9]
;       2 [9:11]
        mov dword[ebp+4],2  ;  [9:11]
;   var [10:9]
;     c=    [10:9]
;       3 [10:11]
        mov dword[ebp+8],3  ;  [10:11]
;   var [11:9]
;     d=    [11:9]
;       4 [11:11]
        mov dword[ebp+12],4  ;  [11:11]
;   var [12:9]
;     r=    [12:9]
;       ((b+c)*d+c*(a+b))*2 [12:11]
;         ((b+c)*d+c*(a+b)) [12:12]
;           (b+c) [12:13]
            mov edi,dword[ebp+4]  ;  [12:13]
            add edi,dword[ebp+8]  ;  [12:15]
          imul edi,dword[ebp+12]  ;  [12:18]
;           c*(a+b) [12:21]
            mov esi,dword[ebp+8]  ;  [12:20]
;             (a+b) [12:23]
              mov edx,dword[ebp+0]  ;  [12:23]
              add edx,dword[ebp+4]  ;  [12:25]
            imul esi,edx  ;  [12:23]
          add edi,esi  ;  [12:21]
        imul edi,2  ;  [12:29]
        mov dword[ebp+16],edi  ;  [12:9]
;   if [13:5]:[155]
;   if r=58 or r=56 [13:8]
    _if_13_8:
    cmp dword[ebp+16],58  ;  [13:8]
    je _if_13_8_code
    cmp dword[ebp+16],56  ;  [13:16]
    jne _if_15_13
    _if_13_8_code:
;     exit(0)  [14:9]
          mov ebx,0  ;  [2:5]
          mov eax,1  ;  [3:5]
          int 0x80  ;  [4:5]
      _end_exit_14_9:
    jmp _if_end_13_5
;   if r=57 [15:13]
    _if_15_13:
    cmp dword[ebp+16],57  ;  [15:13]
    jne _if_else_13_5
    _if_15_13_code:
;     exit(1)  [16:9]
          mov ebx,1  ;  [2:5]
          mov eax,1  ;  [3:5]
          int 0x80  ;  [4:5]
      _end_exit_16_9:
    jmp _if_end_13_5
    _if_else_13_5:
;       exit(r)  [18:9]
            mov ebx,dword[ebp+16]  ;  [2:5]
            mov eax,1  ;  [3:5]
            int 0x80  ;  [4:5]
        _end_exit_18_9:
    _if_end_13_5:

;           max regs in use: 3
;         max frames in use: 3
;          max stack in use: 5
