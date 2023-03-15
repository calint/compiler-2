; generated by baz

section .data
align 4
;[1:1] field hello="hello\n" 
hello db 'hello',10,''
hello.len equ $-hello
;[2:1] field world="world\n" 
world db 'world',10,''
world.len equ $-world

section .bss
align 4
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
;  res: qword[rbp-8]
;  a: qword[rbp+16]
;  b: qword[rbp+24]
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
;      [20:16] break 
       jmp loop_19_5_end
     if_20_9_end:
;    [21:9] print(hello.len,hello)
;      inline: 21_9
;      alloc rdx
;      len -> rdx
       mov rdx,hello.len
;      alloc rcx
;      ptr -> rcx
       mov rcx,hello
;      [5:5] mov(rcx,ptr)
;      [6:5] mov(rdx,len)
;      [7:5] mov(rbx,1)
       mov rbx,1
;      [8:5] mov(rax,4)
       mov rax,4
;      [9:5] int(0x80)
       int 0x80
;      free rcx
;      free rdx
     print_21_9_end:
;    [22:9] b=b-1 
;    [22:11] b-1 
;    [22:11] b=b
;    [22:13] b-1 
     sub qword[rbp+24],1
   jmp loop_19_5
   loop_19_5_end:
;  [24:5] res=a 
;  alloc r15
;  [24:9] a 
;  [24:9] r15=a 
   mov r15,qword[rbp+16]
   mov qword[rbp-8],r15
;  free r15
   mov rax,qword[rbp-8]
   pop rbp
   ret

foo:
   push rbp
   mov rbp,rsp
;  [28:5] print(world.len,world)
;    inline: 28_5
;    alloc rdx
;    len -> rdx
     mov rdx,world.len
;    alloc rcx
;    ptr -> rcx
     mov rcx,world
;    [5:5] mov(rcx,ptr)
;    [6:5] mov(rdx,len)
;    [7:5] mov(rbx,1)
     mov rbx,1
;    [8:5] mov(rax,4)
     mov rax,4
;    [9:5] int(0x80)
     int 0x80
;    free rcx
;    free rdx
   print_28_5_end:
   pop rbp
   ret

main:
;  [37:5] var a=1 
;  a: dword[rbp-8]
;  [37:9] a=1 
;  [37:11] 1 
;  [37:11] a=1 
   mov dword[rbp-8],1
;  [38:5] var b=2 
;  b: dword[rbp-12]
;  [38:9] b=2 
;  [38:11] 2 
;  [38:11] b=2 
   mov dword[rbp-12],2
;  [39:5] var c=3 
;  c: dword[rbp-16]
;  [39:9] c=3 
;  [39:11] 3 
;  [39:11] c=3 
   mov dword[rbp-16],3
;  [40:5] bar(a,b)
   sub rsp,12
   push dword[rbp-12]
   push dword[rbp-8]
   call bar
   add rsp,28
;  [41:5] foo()
   sub rsp,12
   call foo
   add rsp,12
;  [42:5] exit(0)
;    inline: 42_5
;    v -> 0
;    [13:5] mov(rbx,v)
     mov rbx,0
;    [14:5] mov(rax,1)
     mov rax,1
;    [15:5] int(0x80)
     int 0x80
   exit_42_5_end:

; max scratch registers in use: 1
;            max frames in use: 6
