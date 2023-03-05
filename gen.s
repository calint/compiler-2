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
;   [8:5] var a=1 
;     [8:9] a=1 
;     [8:11] 1 
        mov dword[ebp+0],1
;   [9:5] var b=2 
;     [9:9] b=2 
;     [9:11] 2 
        mov dword[ebp+4],2
;   [10:5] var c=3 
;     [10:9] c=3 
;     [10:11] 3 
        mov dword[ebp+8],3
;   [11:5] var d=4 
;     [11:9] d=4 
;     [11:11] 4 
        mov dword[ebp+12],4
;   [12:5] var r=((b+c)*d+c*(a+b))*2 
;     [12:9] r=((b+c)*d+c*(a+b))*2 
;     [12:11] ((b+c)*d+c*(a+b))*2 
;       [12:12] ((b+c)*d+c*(a+b))
;         [12:13] (b+c)
            mov edi,dword[ebp+4]
            add edi,dword[ebp+8]
          imul edi,dword[ebp+12]
;         [12:21] c*(a+b)
            mov esi,dword[ebp+8]
;           [12:23] (a+b)
              mov edx,dword[ebp+0]
              add edx,dword[ebp+4]
            imul esi,edx
          add edi,esi
        imul edi,2
        mov dword[ebp+16],edi
    if_14_8:
;   [14:8] (a=1 and b=2) or (c=3 and d=4)
;   [14:9] (a=1 and b=2)
;   [14:9] a=1 
    cmp_14_9:
    cmp dword[ebp+0],1  ;  [14:9]
    jne cmp_14_26
;   [14:17] b=2
    cmp_14_17:
    cmp dword[ebp+4],2  ;  [14:17]
    jne cmp_14_26
    jmp if_14_8_code
;   [14:26] (c=3 and d=4)
;   [14:26] c=3 
    cmp_14_26:
    cmp dword[ebp+8],3  ;  [14:26]
    jne if_end_14_5
;   [14:34] d=4
    cmp_14_34:
    cmp dword[ebp+12],4  ;  [14:34]
    jne if_end_14_5
    if_14_8_code:
;     [15:9] exit(1)
;         [2:5] mov(ebx,v)
          mov ebx,1
;         [3:5] mov(eax,1)
          mov eax,1
;         [4:5] int(0x80)
          int 0x80
      exit_end_15_9:
    if_end_14_5:
;   [16:5] exit(r)
;       [2:5] mov(ebx,v)
        mov ebx,dword[ebp+16]
;       [3:5] mov(eax,1)
        mov eax,1
;       [4:5] int(0x80)
        int 0x80
    exit_end_16_5:

;           max regs in use: 3
;         max frames in use: 2
;          max stack in use: 5
