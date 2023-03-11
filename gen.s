; generated by baz

section .data

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
main:
;  [8:5] var a=1 
;  [8:9] a=1 
;  [8:11] 1 
;  [8:11] a=1 
   mov qword[rbp-8],1
;  [9:5] var b=2 
;  [9:9] b=2 
;  [9:11] 2 
;  [9:11] b=2 
   mov qword[rbp-16],2
;  [10:5] var c=3 
;  [10:9] c=3 
;  [10:11] 3 
;  [10:11] c=3 
   mov qword[rbp-24],3
;  [11:5] var d=4 
;  [11:9] d=4 
;  [11:11] 4 
;  [11:11] d=4 
   mov qword[rbp-32],4
;  [12:5] var r=((b+c)*d+c*(a+b))*2 
;  [12:9] r=((b+c)*d+c*(a+b))*2 
;  [12:11] ((b+c)*d+c*(a+b))*2 
;  [12:12] r15=((b+c)*d+c*(a+b))
;  [12:12] ((b+c)*d+c*(a+b))
;  [12:13] r15=(b+c)
;  [12:13] (b+c)
;  [12:13] r15=b
   mov r15,qword[rbp-16]
;  [12:15] r15+c
   add r15,qword[rbp-24]
;  [12:18] r15*d
   imul r15,qword[rbp-32]
;  [12:21] r15+c*(a+b)
;  [12:21] c*(a+b)
;  [12:20] r14=c
   mov r14,qword[rbp-24]
;  [12:23] r14*(a+b)
;  [12:23] (a+b)
;  [12:23] r13=a
   mov r13,qword[rbp-8]
;  [12:25] r13+b
   add r13,qword[rbp-16]
   imul r14,r13
   add r15,r14
;  [12:29] r15*2 
   imul r15,2
   mov qword[rbp-40],r15
;  [13:5] exit(r)
;    inline: 13_5
;    [2:5] mov(rbx,v)
     mov rbx,qword[rbp-40]
;    [3:5] mov(rax,1)
     mov rax,1
;    [4:5] int(0x80)
     int 0x80
   exit_13_5_end:

;      max registers in use: 3
;         max frames in use: 2
