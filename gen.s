section .text
global _start
_start:
  mov ebp,stk
; main(){  [0]
   _loop_553:
;    test2(){  [563]
;      print(hello.len hello){  [458]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
;      }
;      test(){  [482]
;        print(prompt.len prompt){  [347]
           mov ecx,prompt
           mov edx,prompt.len
           mov ebx,1
           mov eax,4
           int 0x80
;        }
;        var ln=
;         read(name.len name):ln{  [380]
            mov esi,name
            mov edx,name.len
            xor eax,eax
            xor edi,edi
            syscall
            mov dword[ebp+0],eax
;         }
;        print(hello.len hello){  [401]
           mov ecx,hello
           mov edx,hello.len
           mov ebx,1
           mov eax,4
           int 0x80
;        }
;        print(ln name){  [425]
           mov ecx,name
           mov edx,dword[ebp+0]
           mov ebx,1
           mov eax,4
           int 0x80
;        }
;      }
;    }
   jmp _loop_553
;  var c=
;   plus(plus 2):c{  [581]
;   plus(2 4):eax{  [586]
      mov eax,2
      add eax,4
;   }
      mov dword[ebp+4],eax
      add dword[ebp+4],2
;   }
   ;  plus(plus 3):eax{  [606]
;  plus(2 4):ebx{  [611]
     mov ebx,2
     add ebx,4
;  }
     mov eax,ebx
     add eax,3
;  }
mov dword[ebp+4],eax
;  exit(){  [626]
     mov eax,1
     int 0x80
;  }
; }


section .data
prompt     db 'enter_name_'
prompt.len equ $-prompt
name     db '......................'
name.len equ $-name
hello     db 'hello_'
hello.len equ $-hello

section .bss
stk resd 256
