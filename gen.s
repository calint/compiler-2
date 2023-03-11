; generated by baz

section .data
;[1:1] field hello="hello\n" 
hello db 'hello',10,''
hello.len equ $-hello
;[2:1] field world="world\n" 
world db 'world',10,''
world.len equ $-world

section .bss
stk resd 256
stk.end:

section .text
align 4
bits 64
global _start
_start:
mov rsp,stk.end
mov rbp,rsp
jmp main
bar:
;  a: rsp+16
;  b: rsp+24
   push rbp
   mov rbp,rsp
;  [19:5] loop
   loop_19_5:
     if_20_12:
;    [20:12] ? b=0 
;    [20:12] ? b=0 
     cmp_20_12:
     cmp qword[rbp+24],0
     jne if_20_9_end
     jmp if_20_12_code
     if_20_12_code:
;      [20:16] return 
       pop rbp
       ret
     if_20_9_end:
;    [21:9] print(hello.len,hello)
       mov rdx,hello.len
       mov rcx,hello
;      inline: 21_9
;      [5:5] mov(rcx,ptr)
;      [6:5] mov(rdx,len)
;      [7:5] mov(rbx,1)
       mov rbx,1
;      [8:5] mov(rax,4)
       mov rax,4
;      [9:5] int(0x80)
       int 0x80
     print_21_9_end:
;    [22:9] b=b-1 
;    [22:11] b-1 
;    [22:11] b=b
;    [22:13] b-1 
     sub qword[rbp+24],1
   jmp loop_19_5
   loop_19_5_end:
   pop rbp
   ret

foo:
   push rbp
   mov rbp,rsp
;  [27:5] print(world.len,world)
     mov rdx,world.len
     mov rcx,world
;    inline: 27_5
;    [5:5] mov(rcx,ptr)
;    [6:5] mov(rdx,len)
;    [7:5] mov(rbx,1)
     mov rbx,1
;    [8:5] mov(rax,4)
     mov rax,4
;    [9:5] int(0x80)
     int 0x80
   print_27_5_end:
   pop rbp
   ret

main:
;  [31:5] var a=1 
;  [31:9] a=1 
;  [31:11] 1 
;  [31:11] a=1 
   mov qword[rbp-8],1
;  [32:5] var b=2 
;  [32:9] b=2 
;  [32:11] 2 
;  [32:11] b=2 
   mov qword[rbp-16],2
;  [33:5] var c=3 
;  [33:9] c=3 
;  [33:11] 3 
;  [33:11] c=3 
   mov qword[rbp-24],3
;  [34:5] bar(a,b)
   sub rsp,24
   push qword[rbp-16]
   push qword[rbp-8]
   call bar
   add rsp,40
;  [35:5] foo()
   sub rsp,24
   call foo
   add rsp,24
;  [36:5] exit(0)
;    inline: 36_5
;    [13:5] mov(rbx,v)
     mov rbx,0
;    [14:5] mov(rax,1)
     mov rax,1
;    [15:5] int(0x80)
     int 0x80
   exit_36_5_end:

;      max registers in use: 1
;         max frames in use: 3
