section .data
; generated by compiler baz

section .bss
stk resd 256
stk.end:

section .text
bits 32
global _start
_start:
mov ebp,stk
mov esp,stk.end
;  [8:5] var a=1 
;  [8:9] a=1 
;  [8:11] 1 
;  [8:11] a=1 
   mov dword[ebp+0],1
;  [9:5] var b=2 
;  [9:9] b=2 
;  [9:11] 2 
;  [9:11] b=2 
   mov dword[ebp+4],2
;  [10:5] var c=3 
;  [10:9] c=3 
;  [10:11] 3 
;  [10:11] c=3 
   mov dword[ebp+8],3
;  [11:5] var d=4 
;  [11:9] d=4 
;  [11:11] 4 
;  [11:11] d=4 
   mov dword[ebp+12],4
;  [12:5] var r=a+b+c+d 
;  [12:9] r=a+b+c+d 
;  [12:11] a+b+c+d 
;  [12:11] edi=a
   mov edi,dword[ebp+0]
;  [12:13] edi+b
   add edi,dword[ebp+4]
;  [12:15] edi+c
   add edi,dword[ebp+8]
;  [12:17] edi+d 
   add edi,dword[ebp+12]
   mov dword[ebp+16],edi
;  [13:1] # var r=a*b+c*d 
;  [14:5] var r=((b+c)*d+c*(a+b))*2 
;  [14:9] r=((b+c)*d+c*(a+b))*2 
;  [14:11] ((b+c)*d+c*(a+b))*2 
;  [14:12] edi=((b+c)*d+c*(a+b))
;  [14:12] ((b+c)*d+c*(a+b))
;  [14:13] edi=(b+c)
;  [14:13] (b+c)
;  [14:13] edi=b
   mov edi,dword[ebp+4]
;  [14:15] edi+c
   add edi,dword[ebp+8]
;  [14:18] edi*d
   imul edi,dword[ebp+12]
;  [14:21] edi+c*(a+b)
;  [14:21] c*(a+b)
;  [14:20] esi=c
   mov esi,dword[ebp+8]
;  [14:23] esi*(a+b)
;  [14:23] (a+b)
;  [14:23] edx=a
   mov edx,dword[ebp+0]
;  [14:25] edx+b
   add edx,dword[ebp+4]
   imul esi,edx
   add edi,esi
;  [14:29] edi*2 
   imul edi,2
   mov dword[ebp+16],edi
;  [15:1] # if (a=1 or b=2) and (c=3 or d=4) 
;  [16:1] # if (a=1 and b=2) or (c=3 and d=4) 
   if_17_8:
;  [17:8] ? (a=1 and (b=2 or c=3)) or d=4 
;  [17:9] ? (a=1 and (b=2 or c=3))
;  [17:9] ? a=1 
   cmp_17_9:
   cmp dword[ebp+0],1
   jne cmp_17_34
;  [17:18] ? (b=2 or c=3)
;  [17:18] ? b=2 
   cmp_17_18:
   cmp dword[ebp+4],2
   je if_17_8_code
;  [17:25] ? c=3
   cmp_17_25:
   cmp dword[ebp+8],3
   je if_17_8_code  ; opt2
;  [17:34] ? d=4 
   cmp_17_34:
   cmp dword[ebp+12],4
   jne if_17_5_end
   if_17_8_code:  ; opt1
;    [18:9] exit(0)
;      [2:5] mov(ebx,v)
       mov ebx,0
;      [3:5] mov(eax,1)
       mov eax,1
;      [4:5] int(0x80)
       int 0x80
     exit_18_9_end:
   if_17_5_end:
;  [19:5] var n=10 
;  [19:9] n=10 
;  [19:11] 10 
;  [19:11] n=10 
   mov dword[ebp+24],10
;  [20:5] loop
   loop_20_5:
     if_21_10:
;    [21:10] ? n=0 
;    [21:10] ? n=0 
     cmp_21_10:
     cmp dword[ebp+24],0
     jne if_21_7_end
     if_21_10_code:  ; opt1
;      [21:14] break 
       jmp loop_20_5_end
     if_21_7_end:
;    [22:1] # var r=((a+b)*c+d)*2 
;    [23:7] n=n-1 
;    [23:9] n-1 
;    [23:9] n=n
;    [23:11] n-1 
     sub dword[ebp+24],1
;    [24:7] continue 
     jmp loop_20_5
   jmp loop_20_5
   loop_20_5_end:
;  [26:5] exit(1)
;    [2:5] mov(ebx,v)
     mov ebx,1
;    [3:5] mov(eax,1)
     mov eax,1
;    [4:5] int(0x80)
     int 0x80
   exit_26_5_end:

;           max regs in use: 3
;         max frames in use: 2
;          max stack in use: 7

