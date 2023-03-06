section .data
; [1:0] field prompt{" hello enter name: "}
prompt db '  hello    enter name: '
prompt.len equ $-prompt

; [2:1] field name{............................................................}
name db '............................................................'
name.len equ $-name

; [3:1] field prompt2{" not a name: "}
prompt2 db '  not a name: '
prompt2.len equ $-prompt2


section .bss
stk resd 256
stk.end:

section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
;   [29:5] print(prompt.len,prompt)
;       [6:2] mov(ecx,ptr)
        mov ecx,prompt
;       [7:2] mov(edx,len)
        mov edx,prompt.len
;       [8:2] mov(ebx,1)
        mov ebx,1
;       [9:2] mov(eax,4)
        mov eax,4
;       [10:2] int(0x80)
        int 0x80
    print_end_29_5:
;   [30:5] loop{ var len=read(name.len,name)-1 # remove the \n if len=0 print(prompt.len,prompt) else if len<=4 print(prompt2.len,prompt2) else{ if read(name.len,name)=1 # is only \n break print(len,name) } }
    _loop_496:
;       [31:9] var len=read(name.len,name)-1 
;         [31:13] len=read(name.len,name)-1 
;         [31:17] read(name.len,name)-1 
;           [31:17] read(name.len,name)
;               [14:2] mov(esi,ptr)
                mov esi,name
;               [15:2] mov(edx,len)
                mov edx,name.len
;               [16:2] xor(eax)
                xor eax,eax
;               [17:2] xor(edi)
                xor edi,edi
;               [18:2] syscall 
                syscall
;               [19:2] mov(int,eax)
                mov edi,eax
            read_end_31_17:
            sub edi,1
            mov dword[ebp+0],edi
        if_32_12:
;       [32:12] len=0 
;       [32:12] len=0 
        cmp_32_12:
        cmp dword[ebp+0],0
        jne if_34_17
        if_32_12_code:
;         [33:13] print(prompt.len,prompt)
;             [6:2] mov(ecx,ptr)
              mov ecx,prompt
;             [7:2] mov(edx,len)
              mov edx,prompt.len
;             [8:2] mov(ebx,1)
              mov ebx,1
;             [9:2] mov(eax,4)
              mov eax,4
;             [10:2] int(0x80)
              int 0x80
          print_end_33_13:
          jmp if_end_32_9
        if_34_17:
;       [34:17] len<=4 
;       [34:17] len<=4 
        cmp_34_17:
        cmp dword[ebp+0],4
        jg if_else_32_9
        if_34_17_code:
;         [35:13] print(prompt2.len,prompt2)
;             [6:2] mov(ecx,ptr)
              mov ecx,prompt2
;             [7:2] mov(edx,len)
              mov edx,prompt2.len
;             [8:2] mov(ebx,1)
              mov ebx,1
;             [9:2] mov(eax,4)
              mov eax,4
;             [10:2] int(0x80)
              int 0x80
          print_end_35_13:
          jmp if_end_32_9
        if_else_32_9:
            if_37_16:
;           [37:16] read(name.len,name)=1 
;           [37:16] read(name.len,name)=1 
            cmp_37_16:
;             [37:16] read(name.len,name)
;               [37:16] read(name.len,name)
;                   [14:2] mov(esi,ptr)
                    mov esi,name
;                   [15:2] mov(edx,len)
                    mov edx,name.len
;                   [16:2] xor(eax)
                    xor eax,eax
;                   [17:2] xor(edi)
                    xor edi,edi
;                   [18:2] syscall 
                    syscall
;                   [19:2] mov(int,eax)
                    mov edi,eax
                read_end_37_16:
            cmp edi,1
            jne if_end_37_13
            if_37_16_code:
              jmp _end_loop_496
            if_end_37_13:
;           [39:13] print(len,name)
;               [6:2] mov(ecx,ptr)
                mov ecx,name
;               [7:2] mov(edx,len)
                mov edx,dword[ebp+0]
;               [8:2] mov(ebx,1)
                mov ebx,1
;               [9:2] mov(eax,4)
                mov eax,4
;               [10:2] int(0x80)
                int 0x80
            print_end_39_13:
        if_end_32_9:
      jmp _loop_496
    _end_loop_496:
;   [43:5] var a=1 
;     [43:9] a=1 
;     [43:11] 1 
        mov dword[ebp+0],1
;   [44:5] var b=2 
;     [44:9] b=2 
;     [44:11] 2 
        mov dword[ebp+4],2
;   [45:5] var c=3 
;     [45:9] c=3 
;     [45:11] 3 
        mov dword[ebp+8],3
;   [46:5] var d=4 
;     [46:9] d=4 
;     [46:11] 4 
        mov dword[ebp+12],4
;   [47:5] var r=((b+c)*d+c*(a+b))*2 
;     [47:9] r=((b+c)*d+c*(a+b))*2 
;     [47:11] ((b+c)*d+c*(a+b))*2 
;       [47:12] ((b+c)*d+c*(a+b))
;         [47:13] (b+c)
            mov edi,dword[ebp+4]
            add edi,dword[ebp+8]
          imul edi,dword[ebp+12]
;         [47:21] c*(a+b)
            mov esi,dword[ebp+8]
;           [47:23] (a+b)
              mov edx,dword[ebp+0]
              add edx,dword[ebp+4]
            imul esi,edx
          add edi,esi
        imul edi,2
        mov dword[ebp+16],edi
    if_50_8:
;   [50:8] (a=1 and (b=2 or c=3)) or d=4 
;   [50:9] (a=1 and (b=2 or c=3))
;   [50:9] a=1 
    cmp_50_9:
    cmp dword[ebp+0],1
    jne cmp_50_34
;   [50:18] (b=2 or c=3)
;   [50:18] b=2 
    cmp_50_18:
    cmp dword[ebp+4],2
    je if_50_8_code
;   [50:25] c=3
    cmp_50_25:
    cmp dword[ebp+8],3
    jne cmp_50_34
;   [50:34] d=4 
    cmp_50_34:
    cmp dword[ebp+12],4
    jne if_end_50_5
    if_50_8_code:
;     [51:9] exit(1)
;         [23:5] mov(ebx,v)
          mov ebx,1
;         [24:5] mov(eax,1)
          mov eax,1
;         [25:5] int(0x80)
          int 0x80
      exit_end_51_9:
    if_end_50_5:
;   [52:5] exit(r)
;       [23:5] mov(ebx,v)
        mov ebx,dword[ebp+16]
;       [24:5] mov(eax,1)
        mov eax,1
;       [25:5] int(0x80)
        int 0x80
    exit_end_52_5:

;           max regs in use: 3
;         max frames in use: 4
;          max stack in use: 5
