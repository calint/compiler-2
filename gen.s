; generated by baz

true equ 1
false equ 0

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
;  r: byte[rbp-1]
;  [12:5] var r:bool=f(true)
;  [12:9] r:bool=f(true)
;  [12:16] ? f(true)
;  [12:16] ? f(true)
   cmp_12_16:
;  alloc r15
;    [12:16] f(true)
;    [12:16] r15=f(true)
;    [12:16] f(true)
;    f(b:bool):res:bool 
;      inline: 12_16
;      alias res -> r15
;      alias b -> true
       if_8_8_12_16:
;      [8:8] ? b 
;      [8:8] ? b 
       cmp_8_8_12_16:
;      const eval to true
;        [8:10] res=false 
         mov r15,false
       if_8_5_12_16_end:
     f_12_16_end:
   cmp r15,0
;  free r15
   je false_12_9
   jmp true_12_9
   true_12_9:
   mov byte[rbp-1],1
   jmp end_12_9
   false_12_9:
   mov byte[rbp-1],0
   end_12_9:
   if_13_8:
;  [13:8] ? r=false 
;  [13:8] ? r=false 
   cmp_13_8:
   cmp byte[rbp-1],false
   jne if_13_5_end
   jmp if_13_8_code
   if_13_8_code:
;    [14:9] exit(4)
;    exit(v:reg_rdi) 
;      inline: 14_9
;      alloc rdi
;      alias v -> rdi
       mov rdi,4
;      [2:5] mov(rax,60)
       mov rax,60
;      [2:17] # exit system call 
;      [3:5] mov(rdi,v)
;      [3:17] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_14_9_end:
   if_13_5_end:
;  [16:5] exit(0)
;  exit(v:reg_rdi) 
;    inline: 16_5
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
   exit_16_5_end:

; max scratch registers in use: 1
;            max frames in use: 5
