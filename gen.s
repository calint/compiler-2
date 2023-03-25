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

f1:
;  f1(i):res 
;  res: qword[rbp-8]
;  i: qword[rbp+16]
   push rbp
   mov rbp,rsp
;  [8:5] res=-i 
;  [8:9] -i 
;  [8:10] res=-i 
;  alloc r15
   mov r15,qword[rbp+16]
   mov qword[rbp-8],r15
;  free r15
   neg qword[rbp-8]
   mov rax,qword[rbp-8]
   pop rbp
   ret

f2:
;  f2(i):res:qword 
;  res: qword[rbp-8]
;  i: qword[rbp+16]
   push rbp
   mov rbp,rsp
;  [12:5] res=-i*2 
;  alloc r15
;  [12:9] -i*2 
;  [12:10] r15=-i
   mov r15,qword[rbp+16]
   neg r15
;  [12:12] r15*2 
   imul r15,2
   mov qword[rbp-8],r15
;  free r15
   mov rax,qword[rbp-8]
   pop rbp
   ret

f3:
;  f3(i:dword):res:dword 
;  res: dword[rbp-4]
;  i: dword[rbp+16]
   push rbp
   mov rbp,rsp
;  [16:5] res=-i*3 
;  alloc r15
;  [16:9] -i*3 
;  [16:10] r15=-i
   movsx r15,dword[rbp+16]
   neg r15
;  [16:12] r15*3 
   imul r15,3
   mov dword[rbp-4],r15d
;  free r15
   movsx rax,dword[rbp-4]
   pop rbp
   ret

f4:
;  f4(i:word):res:word 
;  res: word[rbp-2]
;  i: word[rbp+16]
   push rbp
   mov rbp,rsp
;  [20:5] res=-i*4 
;  alloc r15
;  [20:9] -i*4 
;  [20:10] r15=-i
   movsx r15,word[rbp+16]
   neg r15
;  [20:12] r15*4 
   imul r15,4
   mov word[rbp-2],r15w
;  free r15
   movsx rax,word[rbp-2]
   pop rbp
   ret

f5:
;  f5(i:byte):res:byte 
;  res: byte[rbp-1]
;  i: byte[rbp+16]
   push rbp
   mov rbp,rsp
;  [24:5] res=-i*4 
;  alloc r15
;  [24:9] -i*4 
;  [24:10] r15=-i
   movsx r15,byte[rbp+16]
   neg r15
;  [24:12] r15*4 
   imul r15,4
   mov byte[rbp-1],r15b
;  free r15
   movsx rax,byte[rbp-1]
   pop rbp
   ret

main:
;  [32:5] var q=f1(1)
;  q: qword[rbp-8]
;  [32:9] q=f1(1)
;  [32:11] f1(1)
;  [32:11] q=f1(1)
;  [32:11] f1(1)
   sub rsp,8
   push 1
   call f1
   add rsp,16
   mov qword[rbp-8],rax
   if_33_8:
;  [33:8] ? not q=-1 
;  [33:8] ? not q=-1 
   cmp_33_8:
   cmp qword[rbp-8],-1
   je if_33_5_end
   jmp if_33_8_code
   if_33_8_code:
;    [33:17] exit(1)
;    exit(v:reg_rdi) 
;      inline: 33_17
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
     exit_33_17_end:
   if_33_5_end:
;  [35:5] var bt:byte=1 
;  bt: byte[rbp-9]
;  [35:9] bt=1 
;  [35:17] 1 
;  [35:17] bt=1 
   mov byte[rbp-9],1
;  [36:5] var wd:word=2 
;  wd: word[rbp-11]
;  [36:9] wd=2 
;  [36:17] 2 
;  [36:17] wd=2 
   mov word[rbp-11],2
;  [37:5] var dd:dword=3 
;  dd: dword[rbp-15]
;  [37:9] dd=3 
;  [37:18] 3 
;  [37:18] dd=3 
   mov dword[rbp-15],3
;  [38:5] var qd:qword=4 
;  qd: qword[rbp-23]
;  [38:9] qd=4 
;  [38:18] 4 
;  [38:18] qd=4 
   mov qword[rbp-23],4
;  [40:5] var b:byte=f5(bt)
;  b: byte[rbp-24]
;  [40:9] b=f5(bt)
;  [40:16] f5(bt)
;  [40:16] b=f5(bt)
;  [40:16] f5(bt)
   sub rsp,24
;  alloc r15
   movsx r15,byte[rbp-9]
   push r15
;  free r15
   call f5
   add rsp,32
   mov byte[rbp-24],al
   if_41_8:
;  [41:8] ? not b=-4 
;  [41:8] ? not b=-4 
   cmp_41_8:
   cmp byte[rbp-24],-4
   je if_41_5_end
   jmp if_41_8_code
   if_41_8_code:
;    [41:17] exit(2)
;    exit(v:reg_rdi) 
;      inline: 41_17
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
     exit_41_17_end:
   if_41_5_end:
;  [43:5] var w:word=f4(wd)
;  w: word[rbp-26]
;  [43:9] w=f4(wd)
;  [43:16] f4(wd)
;  [43:16] w=f4(wd)
;  [43:16] f4(wd)
   sub rsp,26
;  alloc r15
   movsx r15,word[rbp-11]
   push r15
;  free r15
   call f4
   add rsp,34
   mov word[rbp-26],ax
   if_44_8:
;  [44:8] ? not w=-8 
;  [44:8] ? not w=-8 
   cmp_44_8:
   cmp word[rbp-26],-8
   je if_44_5_end
   jmp if_44_8_code
   if_44_8_code:
;    [44:17] exit(3)
;    exit(v:reg_rdi) 
;      inline: 44_17
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
     exit_44_17_end:
   if_44_5_end:
;  [46:5] var d:dword=f3(dd)
;  d: dword[rbp-30]
;  [46:9] d=f3(dd)
;  [46:17] f3(dd)
;  [46:17] d=f3(dd)
;  [46:17] f3(dd)
   sub rsp,30
;  alloc r15
   movsx r15,dword[rbp-15]
   push r15
;  free r15
   call f3
   add rsp,38
   mov dword[rbp-30],eax
   if_47_8:
;  [47:8] ? not d=-9 
;  [47:8] ? not d=-9 
   cmp_47_8:
   cmp dword[rbp-30],-9
   je if_47_5_end
   jmp if_47_8_code
   if_47_8_code:
;    [47:17] exit(4)
;    exit(v:reg_rdi) 
;      inline: 47_17
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
     exit_47_17_end:
   if_47_5_end:
;  [49:5] q=f2(qd)
;  [49:7] f2(qd)
;  [49:7] q=f2(qd)
;  [49:7] f2(qd)
   sub rsp,30
   push qword[rbp-23]
   call f2
   add rsp,38
   mov qword[rbp-8],rax
   if_50_8:
;  [50:8] ? not q=-8 
;  [50:8] ? not q=-8 
   cmp_50_8:
   cmp qword[rbp-8],-8
   je if_50_5_end
   jmp if_50_8_code
   if_50_8_code:
;    [50:17] exit(5)
;    exit(v:reg_rdi) 
;      inline: 50_17
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
     exit_50_17_end:
   if_50_5_end:
;  [52:5] q=f6(b)
;  [52:7] f6(b)
;  [52:7] q=f6(b)
;  [52:7] f6(b)
;  f6(i:byte):res:qword 
;    inline: 52_7
;    alias res -> q
;    alias i -> b
;    [28:5] res=-i 
;    [28:9] -i 
;    [28:10] res=-i 
;    alloc r15
     movsx r15,byte[rbp-24]
     mov qword[rbp-8],r15
;    free r15
     neg qword[rbp-8]
   f6_52_7_end:
   if_53_8:
;  [53:8] ? not q=4 
;  [53:8] ? not q=4 
   cmp_53_8:
   cmp qword[rbp-8],4
   je if_53_5_end
   jmp if_53_8_code
   if_53_8_code:
;    [53:16] exit(8)
;    exit(v:reg_rdi) 
;      inline: 53_16
;      alloc rdi
;      alias v -> rdi
       mov rdi,8
;      [2:5] mov(rax,60)
       mov rax,60
;      [2:17] # exit system call 
;      [3:5] mov(rdi,v)
;      [3:17] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_53_16_end:
   if_53_5_end:
;  [55:1] # b=f6(b) # truncation error 
;  [57:5] exit(0)
;  exit(v:reg_rdi) 
;    inline: 57_5
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
   exit_57_5_end:

; max scratch registers in use: 2
;            max frames in use: 5
