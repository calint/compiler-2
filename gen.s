; generated by baz

section .data
;[1:1] field prompt=" hello enter name: " 
prompt db '  hello    enter name: '
prompt.len equ $-prompt
;[2:1] field name="............................................................" 
name db '............................................................'
name.len equ $-name
;[3:1] field prompt2=" not a name: " 
prompt2 db '  not a name: '
prompt2.len equ $-prompt2
;[4:1] field len=0 
len dq 0
;[5:1] field hello="hello\n" 
hello db 'hello',10,''
hello.len equ $-hello

section .bss
stk resd 256
stk.end:

section .text
align 4
bits 64
global _start
_start:
mov rsp,stk.end
;  [43:5] var a=1 
;  [43:9] a=1 
;  [43:11] 1 
;  [43:11] a=1 
   mov qword[stk+0],1
;  [44:5] var b=2 
;  [44:9] b=2 
;  [44:11] 2 
;  [44:11] b=2 
   mov qword[stk+8],2
;  [45:5] var c=3 
;  [45:9] c=3 
;  [45:11] 3 
;  [45:11] c=3 
   mov qword[stk+16],3
;  [46:5] var d=4 
;  [46:9] d=4 
;  [46:11] 4 
;  [46:11] d=4 
   mov qword[stk+24],4
;  [47:5] var r=a+b+c+d 
;  [47:9] r=a+b+c+d 
;  [47:11] a+b+c+d 
;  [47:11] r15=a
   mov r15,qword[stk+0]
;  [47:13] r15+b
   add r15,qword[stk+8]
;  [47:15] r15+c
   add r15,qword[stk+16]
;  [47:17] r15+d 
   add r15,qword[stk+24]
   mov qword[stk+32],r15
;  [48:5] var x=2 
;  [48:9] x=2 
;  [48:11] 2 
;  [48:11] x=2 
   mov qword[stk+40],2
;  [49:5] foo(x)
;    inline: 49_5
;    [35:5] loop
     loop_35_5_49_5:
       if_36_12_49_5:
;      [36:12] ? i=0 
;      [36:12] ? i=0 
       cmp_36_12_49_5:
       cmp qword[stk+40],0
       jne if_36_9_49_5_end
       if_36_12_49_5_code:  ; opt1
;        [36:16] return 
         jmp foo_49_5_end
       if_36_9_49_5_end:
;      [37:9] bar()
;        inline: 37_9_49_5
;        [31:5] print(hello.len,hello)
;          inline: 31_5_37_9_49_5
;          [8:5] mov(rcx,ptr)
           mov rcx,hello
;          [9:5] mov(rdx,len)
           mov rdx,hello.len
;          [10:5] mov(rbx,1)
           mov rbx,1
;          [11:5] mov(rax,4)
           mov rax,4
;          [12:5] int(0x80)
           int 0x80
         print_31_5_37_9_49_5_end:
       bar_37_9_49_5_end:
;      [38:9] i=i-1 
;      [38:11] i-1 
;      [38:11] i=i
;      [38:13] i-1 
       sub qword[stk+40],1
     jmp loop_35_5_49_5
     loop_35_5_49_5_end:
   foo_49_5_end:
;  [50:5] x=1 
;  [50:7] 1 
;  [50:7] x=1 
   mov qword[stk+40],1
;  [51:5] foo(x)
;    inline: 51_5
;    [35:5] loop
     loop_35_5_51_5:
       if_36_12_51_5:
;      [36:12] ? i=0 
;      [36:12] ? i=0 
       cmp_36_12_51_5:
       cmp qword[stk+40],0
       jne if_36_9_51_5_end
       if_36_12_51_5_code:  ; opt1
;        [36:16] return 
         jmp foo_51_5_end
       if_36_9_51_5_end:
;      [37:9] bar()
;        inline: 37_9_51_5
;        [31:5] print(hello.len,hello)
;          inline: 31_5_37_9_51_5
;          [8:5] mov(rcx,ptr)
           mov rcx,hello
;          [9:5] mov(rdx,len)
           mov rdx,hello.len
;          [10:5] mov(rbx,1)
           mov rbx,1
;          [11:5] mov(rax,4)
           mov rax,4
;          [12:5] int(0x80)
           int 0x80
         print_31_5_37_9_51_5_end:
       bar_37_9_51_5_end:
;      [38:9] i=i-1 
;      [38:11] i-1 
;      [38:11] i=i
;      [38:13] i-1 
       sub qword[stk+40],1
     jmp loop_35_5_51_5
     loop_35_5_51_5_end:
   foo_51_5_end:
;  [52:5] exit(0)
;    inline: 52_5
;    [25:5] mov(rbx,v)
     mov rbx,0
;    [26:5] mov(rax,1)
     mov rax,1
;    [27:5] int(0x80)
     int 0x80
   exit_52_5_end:

;      max registers in use: 1
;         max frames in use: 5
;          max stack in use: 6

