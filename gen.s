section .data

section .bss
stk resd 256
stk.end:

section .text
[bits 32]
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
;   [12:1] # if (a=1 or b=2) and (c=3 or d=4) 
    if_13_8:
;   [13:8] (a=1 and b=2) or (c=3 and d=4)
;   [13:9] (a=1 and b=2)
;   [13:9] a=1 
    cmp_13_9:
    cmp dword[ebp+0],1
    jne cmp_13_26
;   [13:17] b=2
    cmp_13_17:
    cmp dword[ebp+4],2
    je if_13_8_code
;   [13:26] (c=3 and d=4)
;   [13:26] c=3 
    cmp_13_26:
    cmp dword[ebp+8],3
    jne if_13_5_end
;   [13:34] d=4
    cmp_13_34:
    cmp dword[ebp+12],4
    jne if_13_5_end
    if_13_8_code:
;     [14:1] # if (a=1 and (b=2 or c=3)) or d=4 
;     [15:9] exit(0)
;       [2:5] mov(ebx,v)
        mov ebx,0
;       [3:5] mov(eax,1)
        mov eax,1
;       [4:5] int(0x80)
        int 0x80
      exit_15_9_end:
    if_13_5_end:
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

