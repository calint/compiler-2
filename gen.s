; generated by baz

section .data
align 4

section .bss
align 4
stk resd 1024
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
;  a: qword[rbp-8]
;  [8:9] a=1 
;  [8:11] 1 
;  [8:11] a=1 
   mov qword[rbp-8],1
;  [9:5] var b=2 
;  b: qword[rbp-16]
;  [9:9] b=2 
;  [9:11] 2 
;  [9:11] b=2 
   mov qword[rbp-16],2
;  [10:5] var c=3 
;  c: qword[rbp-24]
;  [10:9] c=3 
;  [10:11] 3 
;  [10:11] c=3 
   mov qword[rbp-24],3
;  [11:5] var d=4 
;  d: qword[rbp-32]
;  [11:9] d=4 
;  [11:11] 4 
;  [11:11] d=4 
   mov qword[rbp-32],4
   if_12_8:
;  [12:8] ? a=0 and b=2 or c=0 and d=4 
;  [12:8] ? a=0 and b=2 
;  [12:8] ? a=0 
   cmp_12_8:
   cmp qword[rbp-8],0
   jne cmp_12_23
;  [12:16] ? b=2 
   cmp_12_16:
   cmp qword[rbp-16],2
   je if_12_8_code  ; opt2
;  [12:23] ? c=0 and d=4 
;  [12:23] ? c=0 
   cmp_12_23:
   cmp qword[rbp-24],0
   jne if_12_5_end
;  [12:31] ? d=4 
   cmp_12_31:
   cmp qword[rbp-32],4
   jne if_12_5_end
   if_12_8_code:  ; opt1
;    [12:35] exit(1)
;    exit(v:reg_rdi) 
;      inline: 12_35
;      alloc rdi
;      alias v -> rdi
       mov rdi,1
;      [2:5] mov(rax,60)
       mov rax,60
;      [2:17] # exit system call 
;      [3:5] mov(rdi,v)
;      [3:17] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_12_35_end:
   if_12_5_end:
;  [12:43] # bug: wrong token at construct recursive stmt_if_bool_ops 
;  [13:5] exit(0)
;  exit(v:reg_rdi) 
;    inline: 13_5
;    alloc rdi
;    alias v -> rdi
     mov rdi,0
;    [2:5] mov(rax,60)
     mov rax,60
;    [2:17] # exit system call 
;    [3:5] mov(rdi,v)
;    [3:17] # return code 
;    [4:5] syscall 
     syscall
;    free rdi
   exit_13_5_end:

; max scratch registers in use: 1
;            max frames in use: 5

