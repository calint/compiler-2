; generated by baz

true equ 1
false equ 0

section .data
align 4
;[1:1] field hello = "hello world from baz\n" 
hello: db 'hello world from baz',10,''
hello.len equ $-hello
;[2:1] field input = "............................................................" 
input: db '............................................................'
input.len equ $-input
;[3:1] field prompt1 = "enter name:\n" 
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
;[4:1] field prompt2 = "that is not a name.\n" 
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
;[5:1] field prompt3 = "hello " 
prompt3: db 'hello '
prompt3.len equ $-prompt3
;[6:1] field dot = "." 
dot: db '.'
dot.len equ $-dot
;[7:1] field nl = "\n" 
nl: db '',10,''
nl.len equ $-nl

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

assert:
;  assert(expr : bool) 
   push rbp
   mov rbp, rsp
;  expr: byte[rbp+16]
   if_33_8:
;  [33:8] ? not expr 
;  [33:8] ? not expr 
   cmp_33_8:
   cmp byte[rbp+16], 0
   jne if_33_5_end
   if_33_8_code:  ; opt1
;    [33:17] exit(1) 
;    exit(v : reg_rdi) 
;      inline: 33_17
;      alloc rdi
;      alias v -> rdi
       mov rdi, 1
;      [10:5] mov(rax, 60) 
       mov rax, 60
;      [10:19] # exit system call 
;      [11:5] mov(rdi, v) 
;      [11:19] # return code 
;      [12:5] syscall 
       syscall
;      free rdi
     exit_33_17_end:
   if_33_5_end:
   pop rbp
   ret

main:
;  x: qword[rbp-8]
;  [41:5] var x = 1 
;  [41:9] x = 1 
;  [41:13] 1 
;  [41:13] 1 
;  [41:13] x=1 
   mov qword[rbp-8], 1
;  y: qword[rbp-16]
;  [42:5] var y = 2 
;  [42:9] y = 2 
;  [42:13] 2 
;  [42:13] 2 
;  [42:13] y=2 
   mov qword[rbp-16], 2
;  o1: qword[rbp-36]
;  [44:5] var o1 : object = {{x * 10, y}, 0xff0000}
;  [44:9] o1 : object = {{x * 10, y}, 0xff0000}
;  [44:23] {{x * 10, y}, 0xff0000}
;    [44:23] {{x * 10, y}, 0xff0000}
;      [44:24] {x * 10, y}
;      [44:25] x * 10
;      [44:25] x * 10
;      [44:25] o1.pos.x=x 
;      alloc r15
       mov r15, qword[rbp-8]
       mov qword[rbp-36], r15
;      free r15
;      [44:29] o1.pos.x* 10
;      alloc r15
       mov r15, qword[rbp-36]
       imul r15, 10
       mov qword[rbp-36], r15
;      free r15
;      [44:33] y
;      [44:33] y
;      [44:33] o1.pos.y=y
;      alloc r15
       mov r15, qword[rbp-16]
       mov qword[rbp-28], r15
;      free r15
;    [44:37] 0xff0000
;    [44:37] 0xff0000
;    [44:37] o1.color=0xff0000
     mov dword[rbp-20], 0xff0000
;  [45:5] assert(o1.pos.x == 10) 
   sub rsp, 36
;  alloc r15
;    [45:12] o1.pos.x == 10
;    [45:12] ? o1.pos.x == 10
;    [45:12] ? o1.pos.x == 10
     cmp_45_12:
     cmp qword[rbp-36], 10
     jne false_45_12
     true_45_12:  ; opt1
     mov r15, 1
     jmp end_45_12
     false_45_12:
     mov r15, 0
     end_45_12:
   push r15
;  free r15
   call assert
   add rsp, 44
;  [46:5] assert(o1.pos.y == 2) 
   sub rsp, 36
;  alloc r15
;    [46:12] o1.pos.y == 2
;    [46:12] ? o1.pos.y == 2
;    [46:12] ? o1.pos.y == 2
     cmp_46_12:
     cmp qword[rbp-28], 2
     jne false_46_12
     true_46_12:  ; opt1
     mov r15, 1
     jmp end_46_12
     false_46_12:
     mov r15, 0
     end_46_12:
   push r15
;  free r15
   call assert
   add rsp, 44
;  [47:5] assert(o1.color == 0xff0000) 
   sub rsp, 36
;  alloc r15
;    [47:12] o1.color == 0xff0000
;    [47:12] ? o1.color == 0xff0000
;    [47:12] ? o1.color == 0xff0000
     cmp_47_12:
     cmp dword[rbp-20], 0xff0000
     jne false_47_12
     true_47_12:  ; opt1
     mov r15, 1
     jmp end_47_12
     false_47_12:
     mov r15, 0
     end_47_12:
   push r15
;  free r15
   call assert
   add rsp, 44
;  p1: qword[rbp-52]
;  [49:5] var p1 : point = {-1, -2}
;  [49:9] p1 : point = {-1, -2}
;  [49:22] {-1, -2}
;    [49:22] {-1, -2}
;    [49:23] -1
;    [49:23] -1
;    [49:24] p1.x=-1
     mov qword[rbp-52], -1
;    [49:27] -2
;    [49:27] -2
;    [49:28] p1.y=-2
     mov qword[rbp-44], -2
;  [50:5] o1.pos = p1 
;  [50:14] p1 
;    [50:14] p1 
;    alloc r15
     mov r15, qword[rbp-52]
     mov qword[rbp-36], r15
;    free r15
;    alloc r15
     mov r15, qword[rbp-44]
     mov qword[rbp-28], r15
;    free r15
;  [51:5] assert(o1.pos.x == -1) 
   sub rsp, 52
;  alloc r15
;    [51:12] o1.pos.x == -1
;    [51:12] ? o1.pos.x == -1
;    [51:12] ? o1.pos.x == -1
     cmp_51_12:
     cmp qword[rbp-36], -1
     jne false_51_12
     true_51_12:  ; opt1
     mov r15, 1
     jmp end_51_12
     false_51_12:
     mov r15, 0
     end_51_12:
   push r15
;  free r15
   call assert
   add rsp, 60
;  [52:5] assert(o1.pos.y == -2) 
   sub rsp, 52
;  alloc r15
;    [52:12] o1.pos.y == -2
;    [52:12] ? o1.pos.y == -2
;    [52:12] ? o1.pos.y == -2
     cmp_52_12:
     cmp qword[rbp-28], -2
     jne false_52_12
     true_52_12:  ; opt1
     mov r15, 1
     jmp end_52_12
     false_52_12:
     mov r15, 0
     end_52_12:
   push r15
;  free r15
   call assert
   add rsp, 60
;  o2: qword[rbp-72]
;  [54:5] var o2 : object = o1 
;  [54:9] o2 : object = o1 
;  [54:23] o1 
;    [54:23] o1 
;      [54:23] o1 
;      alloc r15
       mov r15, qword[rbp-36]
       mov qword[rbp-72], r15
;      free r15
;      alloc r15
       mov r15, qword[rbp-28]
       mov qword[rbp-64], r15
;      free r15
;    alloc r15
     mov r15d, dword[rbp-20]
     mov dword[rbp-56], r15d
;    free r15
;  [55:5] assert(o2.pos.x == -1) 
   sub rsp, 72
;  alloc r15
;    [55:12] o2.pos.x == -1
;    [55:12] ? o2.pos.x == -1
;    [55:12] ? o2.pos.x == -1
     cmp_55_12:
     cmp qword[rbp-72], -1
     jne false_55_12
     true_55_12:  ; opt1
     mov r15, 1
     jmp end_55_12
     false_55_12:
     mov r15, 0
     end_55_12:
   push r15
;  free r15
   call assert
   add rsp, 80
;  [56:5] assert(o2.pos.y == -2) 
   sub rsp, 72
;  alloc r15
;    [56:12] o2.pos.y == -2
;    [56:12] ? o2.pos.y == -2
;    [56:12] ? o2.pos.y == -2
     cmp_56_12:
     cmp qword[rbp-64], -2
     jne false_56_12
     true_56_12:  ; opt1
     mov r15, 1
     jmp end_56_12
     false_56_12:
     mov r15, 0
     end_56_12:
   push r15
;  free r15
   call assert
   add rsp, 80
;  [57:5] assert(o2.color == 0xff0000) 
   sub rsp, 72
;  alloc r15
;    [57:12] o2.color == 0xff0000
;    [57:12] ? o2.color == 0xff0000
;    [57:12] ? o2.color == 0xff0000
     cmp_57_12:
     cmp dword[rbp-56], 0xff0000
     jne false_57_12
     true_57_12:  ; opt1
     mov r15, 1
     jmp end_57_12
     false_57_12:
     mov r15, 0
     end_57_12:
   push r15
;  free r15
   call assert
   add rsp, 80
;  [59:5] print(hello.len, hello) 
;  print(len : reg_rdx, ptr : reg_rsi) 
;    inline: 59_5
;    alloc rdx
;    alias len -> rdx
     mov rdx, hello.len
;    alloc rsi
;    alias ptr -> rsi
     mov rsi, hello
;    [16:5] mov(rax, 1) 
     mov rax, 1
;    [16:19] # write system call 
;    [17:5] mov(rdi, 1) 
     mov rdi, 1
;    [17:19] # file descriptor for standard out 
;    [18:5] mov(rsi, ptr) 
;    [18:19] # buffer address 
;    [19:5] mov(rdx, len) 
;    [19:19] # buffer size 
;    [20:5] syscall 
     syscall
;    free rsi
;    free rdx
   print_59_5_end:
;  [60:5] loop
   loop_60_5:
;    [61:9] print(prompt1.len, prompt1) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 61_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, prompt1.len
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, prompt1
;      [16:5] mov(rax, 1) 
       mov rax, 1
;      [16:19] # write system call 
;      [17:5] mov(rdi, 1) 
       mov rdi, 1
;      [17:19] # file descriptor for standard out 
;      [18:5] mov(rsi, ptr) 
;      [18:19] # buffer address 
;      [19:5] mov(rdx, len) 
;      [19:19] # buffer size 
;      [20:5] syscall 
       syscall
;      free rsi
;      free rdx
     print_61_9_end:
;    len: qword[rbp-80]
;    [62:9] var len = read(input.len, input) - 1 
;    [62:13] len = read(input.len, input) - 1 
;    [62:19] read(input.len, input) - 1 
;    [62:19] read(input.len, input) - 1 
;    [62:19] len=read(input.len, input) 
;    [62:19] read(input.len, input) 
;    read(len : reg_rdx, ptr : reg_rsi) : nbytes 
;      inline: 62_19
;      alias nbytes -> len
;      alloc rdx
;      alias len -> rdx
       mov rdx, input.len
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, input
;      [24:5] mov(rax, 0) 
       mov rax, 0
;      [24:19] # read system call 
;      [25:5] mov(rdi, 0) 
       mov rdi, 0
;      [25:19] # file descriptor for standard input 
;      [26:5] mov(rsi, ptr) 
;      [26:19] # buffer address 
;      [27:5] mov(rdx, len) 
;      [27:19] # buffer size 
;      [28:5] syscall 
       syscall
;      [29:5] mov(nbytes, rax) 
       mov qword[rbp-80], rax
;      [29:22] # return value 
;      free rsi
;      free rdx
     read_62_19_end:
;    [62:44] len- 1 
     sub qword[rbp-80], 1
;    [62:49] # -1 don't include the '\n' 
     if_63_12:
;    [63:12] ? len == 0 
;    [63:12] ? len == 0 
     cmp_63_12:
     cmp qword[rbp-80], 0
     jne if_63_9_end
     if_63_12_code:  ; opt1
;      [64:13] break 
       jmp loop_60_5_end
     if_63_9_end:
     if_66_12:
;    [66:12] ? len <= 4 
;    [66:12] ? len <= 4 
     cmp_66_12:
     cmp qword[rbp-80], 4
     jg if_66_9_end
     if_66_12_code:  ; opt1
;      [67:13] print(prompt2.len, prompt2) 
;      print(len : reg_rdx, ptr : reg_rsi) 
;        inline: 67_13
;        alloc rdx
;        alias len -> rdx
         mov rdx, prompt2.len
;        alloc rsi
;        alias ptr -> rsi
         mov rsi, prompt2
;        [16:5] mov(rax, 1) 
         mov rax, 1
;        [16:19] # write system call 
;        [17:5] mov(rdi, 1) 
         mov rdi, 1
;        [17:19] # file descriptor for standard out 
;        [18:5] mov(rsi, ptr) 
;        [18:19] # buffer address 
;        [19:5] mov(rdx, len) 
;        [19:19] # buffer size 
;        [20:5] syscall 
         syscall
;        free rsi
;        free rdx
       print_67_13_end:
;      [68:13] continue 
       jmp loop_60_5
     if_66_9_end:
;    [70:9] print(prompt3.len, prompt3) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 70_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, prompt3.len
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, prompt3
;      [16:5] mov(rax, 1) 
       mov rax, 1
;      [16:19] # write system call 
;      [17:5] mov(rdi, 1) 
       mov rdi, 1
;      [17:19] # file descriptor for standard out 
;      [18:5] mov(rsi, ptr) 
;      [18:19] # buffer address 
;      [19:5] mov(rdx, len) 
;      [19:19] # buffer size 
;      [20:5] syscall 
       syscall
;      free rsi
;      free rdx
     print_70_9_end:
;    [71:9] print(len, input) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 71_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, qword[rbp-80]
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, input
;      [16:5] mov(rax, 1) 
       mov rax, 1
;      [16:19] # write system call 
;      [17:5] mov(rdi, 1) 
       mov rdi, 1
;      [17:19] # file descriptor for standard out 
;      [18:5] mov(rsi, ptr) 
;      [18:19] # buffer address 
;      [19:5] mov(rdx, len) 
;      [19:19] # buffer size 
;      [20:5] syscall 
       syscall
;      free rsi
;      free rdx
     print_71_9_end:
;    [72:9] print(dot.len, dot) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 72_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, dot.len
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, dot
;      [16:5] mov(rax, 1) 
       mov rax, 1
;      [16:19] # write system call 
;      [17:5] mov(rdi, 1) 
       mov rdi, 1
;      [17:19] # file descriptor for standard out 
;      [18:5] mov(rsi, ptr) 
;      [18:19] # buffer address 
;      [19:5] mov(rdx, len) 
;      [19:19] # buffer size 
;      [20:5] syscall 
       syscall
;      free rsi
;      free rdx
     print_72_9_end:
;    [73:9] print(nl.len, nl) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 73_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, nl.len
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, nl
;      [16:5] mov(rax, 1) 
       mov rax, 1
;      [16:19] # write system call 
;      [17:5] mov(rdi, 1) 
       mov rdi, 1
;      [17:19] # file descriptor for standard out 
;      [18:5] mov(rsi, ptr) 
;      [18:19] # buffer address 
;      [19:5] mov(rdx, len) 
;      [19:19] # buffer size 
;      [20:5] syscall 
       syscall
;      free rsi
;      free rdx
     print_73_9_end:
   jmp loop_60_5
   loop_60_5_end:
; main end

; system call: exit 0
mov rax, 60
mov rdi, 0
syscall

; max scratch registers in use: 1
;            max frames in use: 7
