section .text
global _start
_start:
  mov ebp,stk
; main(){  [0]
;  test2(){  [556]
;    print(hello.len hello){  [458]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
;    }
;    test(){  [482]
;      print(prompt.len prompt){  [347]
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
;      }
;      var ln=
;       read(name.len name):ln{  [380]
          mov esi,name
          mov edx,name.len
          xor eax,eax
          xor edi,edi
          syscall
          mov dword[ebp+0],eax
;       }
;      print(hello.len hello){  [401]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
;      }
;      print(ln name){  [425]
         mov ecx,name
         mov edx,dword[ebp+0]
         mov ebx,1
         mov eax,4
         int 0x80
;      }
;    }
;  }
;  var c=
;   op_plus(op_plus 2):c{  [571]
;   op_plus(2 4):eax{  [579]
      mov eax,2
      add eax,4
;   }
      mov dword[ebp+4],eax
      add dword[ebp+4],2
;   }
   ;  op_plus(op_plus 3):eax{  [602]
;  op_plus(2 4):ebx{  [610]
     mov ebx,2
     add ebx,4
;  }
     mov eax,ebx
     add eax,3
;  }
mov dword[ebp+4],eax
;  exit(){  [628]
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
