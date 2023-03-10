; generated by baz

section .data
;[1:1] field hello="hello\n" 
hello db 'hello',10,''
hello.len equ $-hello

section .bss
stk resd 256
stk.end:

section .text
align 4
bits 64
global _start
_start:
mov rsp,stk.end
jmp main

print:
;  len rsp+8
;  ptr rsp+16
;  [4:5] mov(rcx,ptr)
   mov rcx,qword[rsp+16]
;  [5:5] mov(rdx,len)
   mov rdx,qword[rsp+8]
;  [6:5] mov(rbx,1)
   mov rbx,1
;  [7:5] mov(rax,4)
   mov rax,4
;  [8:5] int(0x80)
   int 0x80
   ret


bar:
;  [18:5] print(hello.len,hello)
   push hello
   push hello.len
   call print
   add rsp,16
   ret

main:
;  [22:5] var a=3 
;  [22:9] a=3 
;  [22:11] 3 
;  [22:11] a=3 
   mov qword[rsp-8],3
;  [23:5] var b=2 
;  [23:9] b=2 
;  [23:11] 2 
;  [23:11] b=2 
   mov qword[rsp-16],2
;  [24:5] loop
   loop_24_5:
     if_25_12:
;    [25:12] ? a=0 
;    [25:12] ? a=0 
     cmp_25_12:
     cmp qword[rsp-8],0
     jne if_25_9_end
     jmp if_25_12_code
     if_25_12_code:
;      [25:16] break 
       jmp loop_24_5_end
     if_25_9_end:
;    [26:9] bar()
     mov r15,rsp
     sub rsp,16
     push r15
     call bar
     pop rsp
;    [27:9] a=a-1 
;    [27:11] a-1 
;    [27:11] a=a
;    [27:13] a-1 
     sub qword[rsp-8],1
   jmp loop_24_5
   loop_24_5_end:
;  [29:5] exit(0)
;    inline: 29_5
;    [12:5] mov(rbx,v)
     mov rbx,0
;    [13:5] mov(rax,1)
     mov rax,1
;    [14:5] int(0x80)
     int 0x80
   exit_29_5_end:

;      max registers in use: 1
;         max frames in use: 2
;          max stack in use: 0
