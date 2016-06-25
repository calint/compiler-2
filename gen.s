section .text
global _start
_start:
  mov ebp,stk
  mov esp,stk.end
;   print(prompt.len prompt){  [58:3]
        mov ecx,prompt  ;  [7:30]
        mov edx,prompt.len  ;  [10:9]
        mov ebx,1  ;  [11:2]
        mov eax,4  ;  [11:14]
        int 0x80  ;  [12:12]
    _end_print_771:
    _loop_797:  ; [62:3]
;       print(plus name){  [63:1]
;         plus(read -2):eax{  [63:7]
;           read(name.len name):ebx{  [63:12]
                mov esi,name  ;  [16:1]
                mov edx,name.len  ;  [18:12]
                xor eax,eax  ;  [19:2]
                xor edi,edi  ;  [19:12]
                syscall  ;  [20:8]
                mov ebx,eax  ;  [21:3]
            _end_read_824:
              mov eax,ebx  ;  [53:10]
              add eax,-2  ;  [56:6]
          _end_plus_819:
            mov ecx,name  ;  [7:30]
            mov edx,eax  ;  [10:9]
            mov ebx,1  ;  [11:2]
            mov eax,4  ;  [11:14]
            int 0x80  ;  [12:12]
        _end_print_813:
      jmp _loop_797
    _end_loop_797:  ; [62:3]
;   exit(){  [65:13]
        mov eax,1  ;  [27:22]
        int 0x80  ;  [27:34]
    _end_exit_858:

section .data
dd1 dd 1
dd0 dd 0

; --- field prompt  [1:9]
prompt db '___echo__type_new_line__ctrl_c__to_break____'
prompt.len equ $-prompt

; --- field name  [2:55]
name db '............................................................'
name.len equ $-name

; --- table name_to_prompt [3:70]
name_to_prompt:
name_to_prompt.end:
name_to_prompt.len equ $-name_to_prompt


section .bss
stk resd 256
stk.end
