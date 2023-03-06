section .data

section .bss
stk resd 256
stk.end:

section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
; [23:1] # comment 
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
;   [12:1] # if (a=1 or b=2) and (c=3 or d=4) 
;   [13:1] # if (a=1 and b=2) or (c=3 and d=4) 
    if_14_8:
;   [14:8] (a=1 and (b=2 or c=3)) or d=4 
;   [14:9] (a=1 and (b=2 or c=3))
;   [14:9] a=1 
    cmp_14_9:
    cmp dword[ebp+0],1
    jne cmp_14_34
;   [14:18] (b=2 or c=3)
;   [14:18] b=2 
    cmp_14_18:
    cmp dword[ebp+4],2
    je if_14_8_code
;   [14:25] c=3
    cmp_14_25:
    cmp dword[ebp+8],3
    jne cmp_14_34
;   [14:34] d=4 
    cmp_14_34:
    cmp dword[ebp+12],4
    jne if_14_5_end
    if_14_8_code:
;     [15:9] exit(0)
;       [2:5] mov(ebx,v)
        mov ebx,0
;       [3:5] mov(eax,1)
        mov eax,1
;       [4:5] int(0x80)
        int 0x80
      exit_15_9_end:
    if_14_5_end:
;   [16:5] exit(1)
;     [2:5] mov(ebx,v)
      mov ebx,1
;     [3:5] mov(eax,1)
      mov eax,1
;     [4:5] int(0x80)
      int 0x80
    exit_16_5_end:
;   [17:5] loop
    loop_17_5:
;     [18:7] break 
      jmp loop_17_5_end
;     [19:7] continue 
      jmp loop_17_5
    jmp loop_17_5
    loop_17_5_end:

;           max regs in use: 0
;         max frames in use: 2
;          max stack in use: 4
