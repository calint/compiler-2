section .text
global _start:
; main(){  [0]
;  test2(){  [586]
;    print(hello.len hello){  [488]
       mov ecx,hello
       mov edx,hello.len
       mov ebx,1
       mov eax,4
       int 0x80
;    }
;    test(){  [512]
;      print(prompt.len prompt){  [377]
         mov ecx,prompt
         mov edx,prompt.len
         mov ebx,1
         mov eax,4
         int 0x80
;      }
;      var ln=
;       read(name.len name):ln{  [410]
          mov esi,name
          mov edx,name.len
          xor eax,eax
          xor edi,edi
          syscall
          mov dword[esp-4],eax
;       }
;      print(hello.len hello){  [431]
         mov ecx,hello
         mov edx,hello.len
         mov ebx,1
         mov eax,4
         int 0x80
;      }
;      print(ln name){  [455]
         mov ecx,name
         mov edx,dword[esp-4]
         mov ebx,1
         mov eax,4
         int 0x80
;      }
;    }
;  }
;  var c=
;   op_plus(1 2):c{  [601]
      mov dword[esp-8],1
      add dword[esp-8],2
;   }
   ;  op_plus(1 3):eax{  [621]
     mov eax,1
     add eax,3
;  }
mov dword[esp-8],eax
;  exit(){  [636]
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
