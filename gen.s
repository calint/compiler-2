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
;  [8:5] var a=0xfffffffffffffffe 
;  a: qword[rbp-8]
;  [8:9] a=0xfffffffffffffffe 
;  [8:11] 0xfffffffffffffffe 
;  [8:11] a=0xfffffffffffffffe 
   mov qword[rbp-8],0xfffffffffffffffe
;  [8:30] # -1 
;  [9:5] var b=2*~a 
;  b: qword[rbp-16]
;  [9:9] b=2*~a 
;  [9:11] 2*~a 
;  [9:11] b=2
   mov qword[rbp-16],2
;  [9:14] b*~a 
;  alloc r15
   mov r15,qword[rbp-8]
   not r15
   imul r15,qword[rbp-16]
   mov qword[rbp-16],r15
;  free r15
   if_10_8:
;  [10:8] ? b=2 
   cmp_10_8:
   cmp qword[rbp-16],2
   jne if_10_5_end
   if_10_8_code:  ; opt1
;    [11:9] exit(0)
;    exit(v:reg_rdi) 
;      inline: 11_9
;      alloc rdi
;      alias v -> rdi
       mov rdi,0
;      [2:5] mov(rax,60)
       mov rax,60
;      [2:17] # exit system call 
;      [3:5] mov(rdi,v)
;      [3:17] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_11_9_end:
   if_10_5_end:
;  [12:5] exit(1)
;  exit(v:reg_rdi) 
;    inline: 12_5
;    alloc rdi
;    alias v -> rdi
     mov rdi,1
;    [2:5] mov(rax,60)
     mov rax,60
;    [2:17] # exit system call 
;    [3:5] mov(rdi,v)
;    [3:17] # return code 
;    [4:5] syscall 
     syscall
;    free rdi
   exit_12_5_end:

; max scratch registers in use: 2
;            max frames in use: 5

