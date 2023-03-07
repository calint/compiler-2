section .data

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
;  [9:5] a=a+1 
;  [9:7] a+1 
;  [9:7] a=a
;  [9:9] a+1 
   add dword[ebp+0],1
;  [10:5] var b=2 
;  [10:9] b=2 
;  [10:11] 2 
;  [10:11] b=2 
   mov dword[ebp+4],2
;  [11:5] var c=3 
;  [11:9] c=3 
;  [11:11] 3 
;  [11:11] c=3 
   mov dword[ebp+8],3
;  [12:5] var d=4 
;  [12:9] d=4 
;  [12:11] 4 
;  [12:11] d=4 
   mov dword[ebp+12],4
;  [13:5] var r=((b+c)*d+c*(a+b))*2 
;  [13:9] r=((b+c)*d+c*(a+b))*2 
;  [13:11] ((b+c)*d+c*(a+b))*2 
;  [13:12] r=((b+c)*d+c*(a+b))
;  [13:12] ((b+c)*d+c*(a+b))
;  [13:13] r=(b+c)
;  [13:13] (b+c)
;  [13:13] r=b
   mov edi,dword[ebp+4]
   mov dword[ebp+16],edi
;  [13:15] r+c
   mov edi,dword[ebp+8]
   add dword[ebp+16],edi
;  [13:18] r*d
   mov edi,dword[ebp+16]
   imul edi,dword[ebp+12]
   mov dword[ebp+16],edi
;  [13:21] r+c*(a+b)
;  [13:21] c*(a+b)
;  [13:20] edi=c
   mov edi,dword[ebp+8]
;  [13:23] edi*(a+b)
;  [13:23] (a+b)
;  [13:23] esi=a
   mov esi,dword[ebp+0]
;  [13:25] esi+b
   add esi,dword[ebp+4]
   imul edi,esi
   add dword[ebp+16],edi
;  [13:29] r*2 
   mov edi,dword[ebp+16]
   imul edi,2
   mov dword[ebp+16],edi
;  [14:1] # if (a=1 or b=2) and (c=3 or d=4) 
;  [15:1] # if (a=1 and b=2) or (c=3 and d=4) 
   if_16_8:
;  [16:8] (a=1 and (b=2 or c=3)) or d=4 
;  [16:9] (a=1 and (b=2 or c=3))
;  [16:9] a=1 
   cmp_16_9:
   cmp dword[ebp+0],1
   jne cmp_16_34
;  [16:18] (b=2 or c=3)
;  [16:18] b=2 
   cmp_16_18:
   cmp dword[ebp+4],2
   je if_16_8_code
;  [16:25] c=3
   cmp_16_25:
   cmp dword[ebp+8],3
   je if_16_8_code  ; opt2
;  [16:34] d=4 
   cmp_16_34:
   cmp dword[ebp+12],4
   jne if_16_5_end
   if_16_8_code:  ; opt1
;    [17:9] exit(0)
;      [2:5] mov(ebx,v)
       mov ebx,0
;      [3:5] mov(eax,1)
       mov eax,1
;      [4:5] int(0x80)
       int 0x80
     exit_17_9_end:
   if_16_5_end:
;  [18:5] var n=10 
;  [18:9] n=10 
;  [18:11] 10 
;  [18:11] n=10 
   mov dword[ebp+20],10
;  [19:5] loop
   loop_19_5:
     if_20_10:
;    [20:10] n=0 
;    [20:10] n=0 
     cmp_20_10:
     cmp dword[ebp+20],0
     jne if_20_7_end
     if_20_10_code:  ; opt1
;      [20:14] break 
       jmp loop_19_5_end
     if_20_7_end:
;    [21:7] var r=((a+b)*c+d)*2 
;    [21:11] r=((a+b)*c+d)*2 
;    [21:13] ((a+b)*c+d)*2 
;    [21:14] r=((a+b)*c+d)
;    [21:14] ((a+b)*c+d)
;    [21:15] r=(a+b)
;    [21:15] (a+b)
;    [21:15] r=a
     mov edi,dword[ebp+0]
     mov dword[ebp+24],edi
;    [21:17] r+b
     mov edi,dword[ebp+4]
     add dword[ebp+24],edi
;    [21:20] r*c
     mov edi,dword[ebp+24]
     imul edi,dword[ebp+8]
     mov dword[ebp+24],edi
;    [21:22] r+d
     mov edi,dword[ebp+12]
     add dword[ebp+24],edi
;    [21:25] r*2 
     mov edi,dword[ebp+24]
     imul edi,2
     mov dword[ebp+24],edi
;    [22:7] n=n-1 
;    [22:9] n-1 
;    [22:9] n=n
;    [22:11] n-1 
     sub dword[ebp+20],1
;    [23:7] continue 
     jmp loop_19_5
   jmp loop_19_5
   loop_19_5_end:
;  [25:5] exit(1)
;    [2:5] mov(ebx,v)
     mov ebx,1
;    [3:5] mov(eax,1)
     mov eax,1
;    [4:5] int(0x80)
     int 0x80
   exit_25_5_end:

;           max regs in use: 2
;         max frames in use: 2
;          max stack in use: 6

