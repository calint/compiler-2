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

f:
;  f(b:bool):res:bool 
;  res: byte[rbp-1]
;  b: byte[rbp+16]
   push rbp
   mov rbp,rsp
   if_8_8:
;  [8:8] ? b 
;  [8:8] ? b 
   cmp_8_8:
   cmp byte[rbp+16],0
   je if_else_8_5
   jmp if_8_8_code
   if_8_8_code:
;    [8:10] res=false 
     mov byte[rbp-1],false
   jmp if_8_5_end
   if_else_8_5:
;      [8:25] res=true 
       mov byte[rbp-1],true
   if_8_5_end:
   movsx rax,byte[rbp-1]
   pop rbp
   ret

main:
;  b: byte[rbp-1]
;  [12:5] var b:bool=true 
;  [12:9] b:bool=true 
   mov byte[rbp-1],true
;  q: qword[rbp-9]
;  [13:5] var q=1 
;  [13:9] q=1 
;  [13:11] 1 
;  [13:11] q=1 
   mov qword[rbp-9],1
;  r: byte[rbp-10]
;  [15:5] var r:bool=f(not b)
;  [15:9] r:bool=f(not b)
;  [15:16] ? f(not b)
;  [15:16] ? f(not b)
   cmp_15_16:
;  alloc r15
;    [15:16] f(not b)
;    [15:16] r15=f(not b)
;    [15:16] f(not b)
     sub rsp,10
;    alloc r14
;      [15:18] not b
;      [15:18] ? not b
;      [15:18] ? not b
       cmp_15_18:
       cmp byte[rbp-1],0
       jne false_15_18
       jmp true_15_18
       true_15_18:
       mov r14,1
       jmp end_15_18
       false_15_18:
       mov r14,0
       end_15_18:
     push r14
;    free r14
     call f
     add rsp,18
     mov r15,rax
   cmp r15,0
;  free r15
   je false_15_9
   jmp true_15_9
   true_15_9:
   mov byte[rbp-10],1
   jmp end_15_9
   false_15_9:
   mov byte[rbp-10],0
   end_15_9:
   if_16_8:
;  [16:8] ? not r 
;  [16:8] ? not r 
   cmp_16_8:
   cmp byte[rbp-10],0
   jne if_16_5_end
   jmp if_16_8_code
   if_16_8_code:
;    [16:14] exit(1)
;    exit(v:reg_rdi) 
;      inline: 16_14
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
     exit_16_14_end:
   if_16_5_end:
;  [18:5] r=f(b)
;  [18:7] ? f(b)
;  [18:7] ? f(b)
   cmp_18_7:
;  alloc r15
;    [18:7] f(b)
;    [18:7] r15=f(b)
;    [18:7] f(b)
     sub rsp,10
;    alloc r14
;      [18:9] b
;      [18:9] ? b
;      [18:9] ? b
       cmp_18_9:
       cmp byte[rbp-1],0
       je false_18_9
       jmp true_18_9
       true_18_9:
       mov r14,1
       jmp end_18_9
       false_18_9:
       mov r14,0
       end_18_9:
     push r14
;    free r14
     call f
     add rsp,18
     mov r15,rax
   cmp r15,0
;  free r15
   je false_18_5
   jmp true_18_5
   true_18_5:
   mov byte[rbp-10],1
   jmp end_18_5
   false_18_5:
   mov byte[rbp-10],0
   end_18_5:
   if_19_8:
;  [19:8] ? r 
;  [19:8] ? r 
   cmp_19_8:
   cmp byte[rbp-10],0
   je if_19_5_end
   jmp if_19_8_code
   if_19_8_code:
;    [19:10] exit(2)
;    exit(v:reg_rdi) 
;      inline: 19_10
;      alloc rdi
;      alias v -> rdi
       mov rdi,2
;      [2:5] mov(rax,60)
       mov rax,60
;      [2:17] # exit system call 
;      [3:5] mov(rdi,v)
;      [3:17] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_19_10_end:
   if_19_5_end:
;  [21:5] r=f(q)
;  [21:7] ? f(q)
;  [21:7] ? f(q)
   cmp_21_7:
;  alloc r15
;    [21:7] f(q)
;    [21:7] r15=f(q)
;    [21:7] f(q)
     sub rsp,10
;    alloc r14
;      [21:9] q
;      [21:9] ? q
;      [21:9] ? q
       cmp_21_9:
       cmp qword[rbp-9],0
       je false_21_9
       jmp true_21_9
       true_21_9:
       mov r14,1
       jmp end_21_9
       false_21_9:
       mov r14,0
       end_21_9:
     push r14
;    free r14
     call f
     add rsp,18
     mov r15,rax
   cmp r15,0
;  free r15
   je false_21_5
   jmp true_21_5
   true_21_5:
   mov byte[rbp-10],1
   jmp end_21_5
   false_21_5:
   mov byte[rbp-10],0
   end_21_5:
   if_22_8:
;  [22:8] ? r 
;  [22:8] ? r 
   cmp_22_8:
   cmp byte[rbp-10],0
   je if_22_5_end
   jmp if_22_8_code
   if_22_8_code:
;    [22:10] exit(3)
;    exit(v:reg_rdi) 
;      inline: 22_10
;      alloc rdi
;      alias v -> rdi
       mov rdi,3
;      [2:5] mov(rax,60)
       mov rax,60
;      [2:17] # exit system call 
;      [3:5] mov(rdi,v)
;      [3:17] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_22_10_end:
   if_22_5_end:
;  [24:5] r=f(true)
;  [24:7] ? f(true)
;  [24:7] ? f(true)
   cmp_24_7:
;  alloc r15
;    [24:7] f(true)
;    [24:7] r15=f(true)
;    [24:7] f(true)
     sub rsp,10
     push true
     call f
     add rsp,18
     mov r15,rax
   cmp r15,0
;  free r15
   je false_24_5
   jmp true_24_5
   true_24_5:
   mov byte[rbp-10],1
   jmp end_24_5
   false_24_5:
   mov byte[rbp-10],0
   end_24_5:
   if_25_8:
;  [25:8] ? r 
;  [25:8] ? r 
   cmp_25_8:
   cmp byte[rbp-10],0
   je if_25_5_end
   jmp if_25_8_code
   if_25_8_code:
;    [25:10] exit(4)
;    exit(v:reg_rdi) 
;      inline: 25_10
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
     exit_25_10_end:
   if_25_5_end:
;  [27:5] r=f(false)
;  [27:7] ? f(false)
;  [27:7] ? f(false)
   cmp_27_7:
;  alloc r15
;    [27:7] f(false)
;    [27:7] r15=f(false)
;    [27:7] f(false)
     sub rsp,10
     push false
     call f
     add rsp,18
     mov r15,rax
   cmp r15,0
;  free r15
   je false_27_5
   jmp true_27_5
   true_27_5:
   mov byte[rbp-10],1
   jmp end_27_5
   false_27_5:
   mov byte[rbp-10],0
   end_27_5:
   if_28_8:
;  [28:8] ? not r 
;  [28:8] ? not r 
   cmp_28_8:
   cmp byte[rbp-10],0
   jne if_28_5_end
   jmp if_28_8_code
   if_28_8_code:
;    [28:14] exit(5)
;    exit(v:reg_rdi) 
;      inline: 28_14
;      alloc rdi
;      alias v -> rdi
       mov rdi,5
;      [2:5] mov(rax,60)
       mov rax,60
;      [2:17] # exit system call 
;      [3:5] mov(rdi,v)
;      [3:17] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_28_14_end:
   if_28_5_end:
;  [30:5] exit(0)
;  exit(v:reg_rdi) 
;    inline: 30_5
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
   exit_30_5_end:

; max scratch registers in use: 2
;            max frames in use: 5
