; generated by baz

section .data
align 4

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

main:
;  [8:5] var a=-1 
;  a: qword[rbp-8]
;  [8:9] a=-1 
;  [8:11] -1 
;  [8:12] a=-1 
   mov qword[rbp-8],-1
;  [9:5] var b=-(-a*-a)
;  b: qword[rbp-16]
;  [9:9] b=-(-a*-a)
;  alloc r15
;  [9:11] -(-a*-a)
;  [9:13] r15=(-a*-a)
;  [9:13] (-a*-a)
;  [9:14] r15=-a
   mov r15,qword[rbp-8]
   neg r15
;  [9:17] r15*-a
   imul r15,qword[rbp-8]
   neg r15
   neg r15
   mov qword[rbp-16],r15
;  free r15
   if_10_8:
;  [10:8] ? b=-1 
;  [10:8] ? b=-1 
   cmp_10_8:
   cmp qword[rbp-16],-1
   jne if_10_5_end
   jmp if_10_8_code
   if_10_8_code:
;    [11:9] exit(0)
;    exit(v) 
;      inline: 11_9
;      alias v -> 0
;      [2:5] mov(rbx,v)
       mov rbx,0
;      [3:5] mov(rax,1)
       mov rax,1
;      [4:5] int(0x80)
       int 0x80
     exit_11_9_end:
   if_10_5_end:
;  [12:5] exit(1)
;  exit(v) 
;    inline: 12_5
;    alias v -> 1
;    [2:5] mov(rbx,v)
     mov rbx,1
;    [3:5] mov(rax,1)
     mov rax,1
;    [4:5] int(0x80)
     int 0x80
   exit_12_5_end:

; max scratch registers in use: 1
;            max frames in use: 5
