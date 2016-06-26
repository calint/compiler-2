section .data
; -- field prompt  [2:7]
prompt db '  hello    enter name: '
prompt.len equ $-prompt

; -- field name  [3:9]
name db '............................................................'
name.len equ $-name

; --- table name_to_prompt [5:7]
name_to_prompt:
name_to_prompt.end:
name_to_prompt.len equ $-name_to_prompt


dd1 dd 1
dd0 dd 0

section .bss
stk resd 256
stk.end


section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
;   var b=2  [62:6]
    mov dword[ebp+0],2
;   var a=1  [63:6]
    mov dword[ebp+4],1
;   var c=3  [64:6]
    mov dword[ebp+8],3
;   a=
;   =(1+b-c)
    mov edi,1  ;  [65:5]
    add edi,dword[ebp+0]  ;  [65:7]
    sub edi,dword[ebp+8]  ;  [65:9]
    mov dword[ebp+4],edi  ;  [65:2]
;   print(prompt.len prompt)  [67:2]
        mov ecx,prompt  ;  [11:2]
        mov edx,prompt.len  ;  [12:2]
        mov ebx,1  ;  [13:2]
        mov eax,4  ;  [14:2]
        int 0x80  ;  [15:2]
    _end_print_970:
    _loop_1014:  ; [69:2]
;       print(read name)  [70:3]
;         read(name.len name):edi  [70:9]
              mov esi,name  ;  [19:2]
              mov edx,name.len  ;  [20:2]
              xor eax,eax  ;  [21:2]
              xor edi,edi  ;  [22:2]
              syscall  ;  [23:2]
              mov edi,eax  ;  [24:2]
          _end_read_1028:
            mov ecx,name  ;  [11:2]
            mov edx,edi  ;  [12:2]
            mov ebx,1  ;  [13:2]
            mov eax,4  ;  [14:2]
            int 0x80  ;  [15:2]
        _end_print_1022:
      jmp _loop_1014
    _end_loop_1014:  ; [69:2]
;   exit()  [72:2]
        mov eax,1  ;  [30:2]
        int 0x80  ;  [31:2]
    _end_exit_1058:

;           max regs in use: 1
;         max frames in use: 3
;          max stack in use: 3
