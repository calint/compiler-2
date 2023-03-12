; generated by baz

section .data
align 4
;[1:1] field hello="hello\n" 
hello db 'hello',10,''
hello.len equ $-hello

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
print:
;  len: rdx
;  ptr: rcx
   push rbp
   mov rbp,rsp
;  [4:5] mov(rbx,1)
   mov rbx,1
;  [5:5] mov(rax,4)
   mov rax,4
;  [6:5] int(0x80)
   int 0x80
   pop rbp
   ret

bar:
   push rbp
   mov rbp,rsp
;  [16:5] print(hello.len,hello)
   mov rcx,hello
   mov rdx,hello.len
   call print
   pop rbp
   ret

main:
;  [20:5] var a=3 
;  [20:9] a=3 
;  [20:11] 3 
;  [20:11] a=3 
   mov qword[rbp-8],3
;  [21:5] var b=2 
;  [21:9] b=2 
;  [21:11] 2 
;  [21:11] b=2 
   mov qword[rbp-16],2
;  [22:5] loop
   loop_22_5:
     if_23_12:
;    [23:12] ? a=0 
;    [23:12] ? a=0 
     cmp_23_12:
     cmp qword[rbp-8],0
     jne if_23_9_end
     if_23_12_code:  ; opt1
;      [23:16] break 
       jmp loop_22_5_end
     if_23_9_end:
;    [24:9] bar()
     sub rsp,16
     call bar
     add rsp,16
;    [25:9] a=a-1 
;    [25:11] a-1 
;    [25:11] a=a
;    [25:13] a-1 
     sub qword[rbp-8],1
   jmp loop_22_5
   loop_22_5_end:
;  [27:5] exit(0)
;    inline: 27_5
;    [10:5] mov(rbx,v)
     mov rbx,0
;    [11:5] mov(rax,1)
     mov rax,1
;    [12:5] int(0x80)
     int 0x80
   exit_27_5_end:

; max scratch registers in use: 1
;            max frames in use: 5

