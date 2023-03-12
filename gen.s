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
a:
;  i: rsp+16
   push rbp
   mov rbp,rsp
;  [8:5] res=i+1 
;  alloc r15
;  [8:9] i+1 
;  [8:9] r15=i
   mov r15,qword[rbp+16]
;  [8:11] r15+1 
   add r15,1
   mov qword[rbp-8],r15
;  free r15
   mov rax,qword[rbp-8]
   pop rbp
   ret

b:
;  i: rsp+16
   push rbp
   mov rbp,rsp
;  [12:5] res=i+2 
;  alloc r15
;  [12:9] i+2 
;  [12:9] r15=i
   mov r15,qword[rbp+16]
;  [12:11] r15+2 
   add r15,2
   mov qword[rbp-8],r15
;  free r15
   mov rax,qword[rbp-8]
   pop rbp
   ret

c:
;  i: rsp+16
   push rbp
   mov rbp,rsp
;  [16:5] res=i+3 
;  alloc r15
;  [16:9] i+3 
;  [16:9] r15=i
   mov r15,qword[rbp+16]
;  [16:11] r15+3 
   add r15,3
   mov qword[rbp-8],r15
;  free r15
   mov rax,qword[rbp-8]
   pop rbp
   ret

main:
;  [20:5] exit(a(b(c(1)+c(2))))
;  alloc rbx
;    [20:10] a(b(c(1)+c(2)))
;    [20:10] rbx=a(b(c(1)+c(2)))
;    [20:10] a(b(c(1)+c(2)))
     push rbx
;    alloc r15
;      [20:12] b(c(1)+c(2))
;      [20:12] r15=b(c(1)+c(2))
;      [20:12] b(c(1)+c(2))
;      alloc r14
;        [20:14] c(1)+c(2)
;        [20:14] r14=c(1)
;        [20:14] c(1)
         push 1
         call c
         add rsp,8
         mov r14,rax
;        [20:19] r14+c(2)
;        alloc r13
;        [20:19] c(2)
         push 2
         call c
         add rsp,8
         mov r13,rax
         add r14,r13
;        free r13
       push r14
;      free r14
       call b
       add rsp,8
       mov r15,rax
     push r15
;    free r15
     call a
     add rsp,8
     pop rbx
     mov rbx,rax
;    inline: 20_5
;    [2:5] mov(rbx,v)
;    [3:5] mov(rax,1)
     mov rax,1
;    [4:5] int(0x80)
     int 0x80
   exit_20_5_end:
;  free rbx

; max scratch registers in use: 3
;            max frames in use: 4

