section .data
prompt db '  hello    enter name: '
prompt.len equ $-prompt
name db '............................................................'
name.len equ $-name
prompt2 db '  not a name: '
prompt2.len equ $-prompt2
section .bss
stk resd 256
stk.end:
section .text
align 4
global _start
_start:
mov ebp,stk
mov esp,stk.end
     mov ecx,prompt
     mov edx,prompt.len
     mov ebx,1
     mov eax,4
     int 0x80
   print_29_5_end:
   loop_30_5:
       mov esi,name
       mov edx,name.len
       xor eax,eax
       xor edi,edi
       syscall
       mov dword[ebp+0],eax
     read_31_17_end:
     sub dword[ebp+0],1
     if_32_12:
     cmp_32_12:
     cmp dword[ebp+0],0
     jne if_34_17
     if_32_12_code:  ; opt1
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
       print_33_13_end:
       jmp if_32_9_end
     if_34_17:
     cmp_34_17:
     cmp dword[ebp+0],4
     jg if_else_32_9
     if_34_17_code:  ; opt1
         mov ecx,prompt2
         mov edx,prompt2.len
         mov ebx,1
         mov eax,4
         int 0x80
       print_35_13_end:
       jmp if_32_9_end
     if_else_32_9:
           mov edx,dword[ebp+0]
           add edx,1
           mov ecx,name
           mov ebx,1
           mov eax,4
           int 0x80
         print_37_13_end:
         if_38_16:
         cmp_38_16:
             mov esi,name
             mov edx,name.len
             xor eax,eax
             xor edi,edi
             syscall
             mov edx,eax
           read_38_16_end:
         cmp edx,1
         jne if_38_13_end
         if_38_16_code:  ; opt1
           jmp loop_30_5_end
         if_38_13_end:
     if_32_9_end:
   jmp loop_30_5
   loop_30_5_end:
     mov ebx,0
     mov eax,1
     int 0x80
   exit_42_5_end:
