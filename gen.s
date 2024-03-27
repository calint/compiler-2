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
;      [12:5] syscall() 
       syscall
;      free rdi
     exit_33_17_end:
   if_33_5_end:
   pop rbp
   ret

main:
;  p: qword[rbp-16]
;  [46:5] var p : point = {0, 0}
;  [46:9] p : point = {0, 0}
;  [46:21] {0, 0}
;    [46:21] {0, 0}
;    [46:22] 0
;    [46:22] 0
;    [46:22] p.x = 0
     mov qword[rbp-16], 0
;    [46:25] 0
;    [46:25] 0
;    [46:25] p.y = 0
     mov qword[rbp-8], 0
;  [47:5] foo(p) 
;  foo(p : point) 
;    inline: 47_5
;    alias p -> p
;    [41:5] p.x = 0b10 
;    [41:11] 0b10 
;    [41:11] 0b10 
;    [41:11] p.x = 0b10 
     mov qword[rbp-16], 0b10
;    [41:19] # binary value 2 
;    [42:5] p.y = 0xb 
;    [42:11] 0xb 
;    [42:11] 0xb 
;    [42:11] p.y = 0xb 
     mov qword[rbp-8], 0xb
;    [42:19] # hex value 11 
   foo_47_5_end:
;  [48:5] # inlined functions can write to the arguments 
;  [49:5] assert(p.x == 2) 
   sub rsp, 16
;  alloc r15
;    [49:12] p.x == 2
;    [49:12] ? p.x == 2
;    [49:12] ? p.x == 2
     cmp_49_12:
     cmp qword[rbp-16], 2
     jne bool_false_49_12
     bool_true_49_12:  ; opt1
     mov r15, true
     jmp bool_end_49_12
     bool_false_49_12:
     mov r15, false
     bool_end_49_12:
   push r15
;  free r15
   call assert
   add rsp, 24
;  [50:5] assert(p.y == 0xb) 
   sub rsp, 16
;  alloc r15
;    [50:12] p.y == 0xb
;    [50:12] ? p.y == 0xb
;    [50:12] ? p.y == 0xb
     cmp_50_12:
     cmp qword[rbp-8], 0xb
     jne bool_false_50_12
     bool_true_50_12:  ; opt1
     mov r15, true
     jmp bool_end_50_12
     bool_false_50_12:
     mov r15, false
     bool_end_50_12:
   push r15
;  free r15
   call assert
   add rsp, 24
;  x: qword[rbp-24]
;  [52:5] var x = 1 
;  [52:9] x = 1 
;  [52:13] 1 
;  [52:13] 1 
;  [52:13] x = 1 
   mov qword[rbp-24], 1
;  y: qword[rbp-32]
;  [53:5] var y = 2 
;  [53:9] y = 2 
;  [53:13] 2 
;  [53:13] 2 
;  [53:13] y = 2 
   mov qword[rbp-32], 2
;  o1: qword[rbp-52]
;  [55:5] var o1 : object = {{x * 10, y}, 0xff0000}
;  [55:9] o1 : object = {{x * 10, y}, 0xff0000}
;  [55:23] {{x * 10, y}, 0xff0000}
;    [55:23] {{x * 10, y}, 0xff0000}
;      [55:24] {x * 10, y}
;      [55:25] x * 10
;      alloc r15
;      [55:25] x * 10
;      [55:25] r15 = x 
       mov r15, qword[rbp-24]
;      [55:29] r15 * 10
       imul r15, 10
       mov qword[rbp-52], r15
;      free r15
;      [55:33] y
;      [55:33] y
;      [55:33] o1.pos.y = y
;      alloc r15
       mov r15, qword[rbp-32]
       mov qword[rbp-44], r15
;      free r15
;    [55:37] 0xff0000
;    [55:37] 0xff0000
;    [55:37] o1.color = 0xff0000
     mov dword[rbp-36], 0xff0000
;  [56:5] assert(o1.pos.x == 10) 
   sub rsp, 52
;  alloc r15
;    [56:12] o1.pos.x == 10
;    [56:12] ? o1.pos.x == 10
;    [56:12] ? o1.pos.x == 10
     cmp_56_12:
     cmp qword[rbp-52], 10
     jne bool_false_56_12
     bool_true_56_12:  ; opt1
     mov r15, true
     jmp bool_end_56_12
     bool_false_56_12:
     mov r15, false
     bool_end_56_12:
   push r15
;  free r15
   call assert
   add rsp, 60
;  [57:5] assert(o1.pos.y == 2) 
   sub rsp, 52
;  alloc r15
;    [57:12] o1.pos.y == 2
;    [57:12] ? o1.pos.y == 2
;    [57:12] ? o1.pos.y == 2
     cmp_57_12:
     cmp qword[rbp-44], 2
     jne bool_false_57_12
     bool_true_57_12:  ; opt1
     mov r15, true
     jmp bool_end_57_12
     bool_false_57_12:
     mov r15, false
     bool_end_57_12:
   push r15
;  free r15
   call assert
   add rsp, 60
;  [58:5] assert(o1.color == 0xff0000) 
   sub rsp, 52
;  alloc r15
;    [58:12] o1.color == 0xff0000
;    [58:12] ? o1.color == 0xff0000
;    [58:12] ? o1.color == 0xff0000
     cmp_58_12:
     cmp dword[rbp-36], 0xff0000
     jne bool_false_58_12
     bool_true_58_12:  ; opt1
     mov r15, true
     jmp bool_end_58_12
     bool_false_58_12:
     mov r15, false
     bool_end_58_12:
   push r15
;  free r15
   call assert
   add rsp, 60
;  p1: qword[rbp-68]
;  [60:5] var p1 : point = {-x, -y}
;  [60:9] p1 : point = {-x, -y}
;  [60:22] {-x, -y}
;    [60:22] {-x, -y}
;    [60:23] -x
;    [60:23] -x
;    [60:24] p1.x = -x
;    alloc r15
     mov r15, qword[rbp-24]
     mov qword[rbp-68], r15
;    free r15
     neg qword[rbp-68]
;    [60:27] -y
;    [60:27] -y
;    [60:28] p1.y = -y
;    alloc r15
     mov r15, qword[rbp-32]
     mov qword[rbp-60], r15
;    free r15
     neg qword[rbp-60]
;  [61:5] o1.pos = p1 
;  [61:14] p1 
;    [61:14] p1 
;    alloc r15
     mov r15, qword[rbp-68]
     mov qword[rbp-52], r15
;    free r15
;    alloc r15
     mov r15, qword[rbp-60]
     mov qword[rbp-44], r15
;    free r15
;  [62:5] assert(o1.pos.x == -1) 
   sub rsp, 68
;  alloc r15
;    [62:12] o1.pos.x == -1
;    [62:12] ? o1.pos.x == -1
;    [62:12] ? o1.pos.x == -1
     cmp_62_12:
     cmp qword[rbp-52], -1
     jne bool_false_62_12
     bool_true_62_12:  ; opt1
     mov r15, true
     jmp bool_end_62_12
     bool_false_62_12:
     mov r15, false
     bool_end_62_12:
   push r15
;  free r15
   call assert
   add rsp, 76
;  [63:5] assert(o1.pos.y == -2) 
   sub rsp, 68
;  alloc r15
;    [63:12] o1.pos.y == -2
;    [63:12] ? o1.pos.y == -2
;    [63:12] ? o1.pos.y == -2
     cmp_63_12:
     cmp qword[rbp-44], -2
     jne bool_false_63_12
     bool_true_63_12:  ; opt1
     mov r15, true
     jmp bool_end_63_12
     bool_false_63_12:
     mov r15, false
     bool_end_63_12:
   push r15
;  free r15
   call assert
   add rsp, 76
;  o2: qword[rbp-88]
;  [65:5] var o2 : object = o1 
;  [65:9] o2 : object = o1 
;  [65:23] o1 
;    [65:23] o1 
;      [65:23] o1 
;      alloc r15
       mov r15, qword[rbp-52]
       mov qword[rbp-88], r15
;      free r15
;      alloc r15
       mov r15, qword[rbp-44]
       mov qword[rbp-80], r15
;      free r15
;    alloc r15
     mov r15d, dword[rbp-36]
     mov dword[rbp-72], r15d
;    free r15
;  [66:5] assert(o2.pos.x == -1) 
   sub rsp, 88
;  alloc r15
;    [66:12] o2.pos.x == -1
;    [66:12] ? o2.pos.x == -1
;    [66:12] ? o2.pos.x == -1
     cmp_66_12:
     cmp qword[rbp-88], -1
     jne bool_false_66_12
     bool_true_66_12:  ; opt1
     mov r15, true
     jmp bool_end_66_12
     bool_false_66_12:
     mov r15, false
     bool_end_66_12:
   push r15
;  free r15
   call assert
   add rsp, 96
;  [67:5] assert(o2.pos.y == -2) 
   sub rsp, 88
;  alloc r15
;    [67:12] o2.pos.y == -2
;    [67:12] ? o2.pos.y == -2
;    [67:12] ? o2.pos.y == -2
     cmp_67_12:
     cmp qword[rbp-80], -2
     jne bool_false_67_12
     bool_true_67_12:  ; opt1
     mov r15, true
     jmp bool_end_67_12
     bool_false_67_12:
     mov r15, false
     bool_end_67_12:
   push r15
;  free r15
   call assert
   add rsp, 96
;  [68:5] assert(o2.color == 0xff0000) 
   sub rsp, 88
;  alloc r15
;    [68:12] o2.color == 0xff0000
;    [68:12] ? o2.color == 0xff0000
;    [68:12] ? o2.color == 0xff0000
     cmp_68_12:
     cmp dword[rbp-72], 0xff0000
     jne bool_false_68_12
     bool_true_68_12:  ; opt1
     mov r15, true
     jmp bool_end_68_12
     bool_false_68_12:
     mov r15, false
     bool_end_68_12:
   push r15
;  free r15
   call assert
   add rsp, 96
;  [70:5] print(hello.len, hello) 
;  print(len : reg_rdx, ptr : reg_rsi) 
;    inline: 70_5
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
;    [20:5] syscall() 
     syscall
;    free rsi
;    free rdx
   print_70_5_end:
;  [71:5] loop
   loop_71_5:
;    [72:9] print(prompt1.len, prompt1) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 72_9
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
;      [20:5] syscall() 
       syscall
;      free rsi
;      free rdx
     print_72_9_end:
;    len: qword[rbp-96]
;    [73:9] var len = read(input.len, input) - 1 
;    [73:13] len = read(input.len, input) - 1 
;    [73:19] read(input.len, input) - 1 
;    [73:19] read(input.len, input) - 1 
;    [73:19] len = read(input.len, input) 
;    [73:19] read(input.len, input) 
;    read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
;      inline: 73_19
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
;      [28:5] syscall() 
       syscall
;      [29:5] mov(nbytes, rax) 
       mov qword[rbp-96], rax
;      [29:22] # return value 
;      free rsi
;      free rdx
     read_73_19_end:
;    [73:44] len - 1 
     sub qword[rbp-96], 1
;    [73:49] # -1 don't include the '\n' 
     if_74_12:
;    [74:12] ? len == 0 
;    [74:12] ? len == 0 
     cmp_74_12:
     cmp qword[rbp-96], 0
     jne if_74_9_end
     if_74_12_code:  ; opt1
;      [75:13] break 
       jmp loop_71_5_end
     if_74_9_end:
     if_77_12:
;    [77:12] ? len <= 4 
;    [77:12] ? len <= 4 
     cmp_77_12:
     cmp qword[rbp-96], 4
     jg if_77_9_end
     if_77_12_code:  ; opt1
;      [78:13] print(prompt2.len, prompt2) 
;      print(len : reg_rdx, ptr : reg_rsi) 
;        inline: 78_13
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
;        [20:5] syscall() 
         syscall
;        free rsi
;        free rdx
       print_78_13_end:
;      [79:13] continue 
       jmp loop_71_5
     if_77_9_end:
;    [81:9] print(prompt3.len, prompt3) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 81_9
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
;      [20:5] syscall() 
       syscall
;      free rsi
;      free rdx
     print_81_9_end:
;    [82:9] print(len, input) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 82_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, qword[rbp-96]
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
;      [20:5] syscall() 
       syscall
;      free rsi
;      free rdx
     print_82_9_end:
;    [83:9] print(dot.len, dot) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 83_9
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
;      [20:5] syscall() 
       syscall
;      free rsi
;      free rdx
     print_83_9_end:
;    [84:9] print(nl.len, nl) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 84_9
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
;      [20:5] syscall() 
       syscall
;      free rsi
;      free rdx
     print_84_9_end:
   jmp loop_71_5
   loop_71_5_end:
; main end

; system call: exit 0
mov rax, 60
mov rdi, 0
syscall

; max scratch registers in use: 1
;            max frames in use: 7
;               max stack size: 96 B
