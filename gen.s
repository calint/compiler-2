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
;[3:1] field prompt1 = "enter name: " 
prompt1: db 'enter name: '
prompt1.len equ $-prompt1
;[4:1] field prompt2 = "that is not a name.\n" 
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
;[5:1] field prompt3 = "hello " 
prompt3: db 'hello '
prompt3.len equ $-prompt3
;[6:1] field nl = "\n" 
nl: db '',10,''
nl.len equ $-nl
;[7:1] field dot = "." 
dot: db '.'
dot.len equ $-dot

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

;[32:1] # define a type 'vector' 
;[33:1] # member 'x' being default type (64 bit integer) 
;[34:1] # member 'y' being 32 bit 
;[35:1] # member 'z' being 16 bit 
;[36:1] # member 'w' being 8 bit 
;[37:1] # member 'valid' being a boolean 
main:
;  [42:5] # declare 'v' on the stack 
;  v: qword[rbp-15]
;  [43:5] var v : vector = 3 
;  [43:9] v : vector = 3 
;  [43:22] 3 
;  [43:22] 3 
;  [43:22] v=3 
   mov qword[rbp-15], 3
;  [43:27] # sets v.x is set to 3 
;  [44:5] v.y = 4 
;  [44:11] 4 
;  [44:11] 4 
;  [44:11] v.y=4 
   mov dword[rbp-7], 4
;  [45:5] v.z = 5 
;  [45:11] 5 
;  [45:11] 5 
;  [45:11] v.z=5 
   mov word[rbp-3], 5
;  [46:5] v.w = 6 
;  [46:11] 6 
;  [46:11] 6 
;  [46:11] v.w=6 
   mov byte[rbp-1], 6
;  itm: qword[rbp-46]
;  [48:5] var itm : item = 1 
;  [48:9] itm : item = 1 
;  [48:22] 1 
;  [48:22] 1 
;  [48:22] itm=1 
   mov qword[rbp-46], 1
;  [48:27] # sets itm.pos.x = 1 
;  [49:5] itm.vel.x = 4 
;  [49:17] 4 
;  [49:17] 4 
;  [49:17] itm.vel.x=4 
   mov qword[rbp-31], 4
;  [50:5] itm.valid = true 
;  [50:17] true 
;  [50:17] true 
;  [50:17] itm.valid=true 
   mov byte[rbp-16], true
;  [52:5] # check if valid 
   if_53_8:
;  [53:8] ? not itm.valid 
;  [53:8] ? not itm.valid 
   cmp_53_8:
   cmp byte[rbp-16], 0
   jne if_53_5_end
   if_53_8_code:  ; opt1
;    [53:22] exit(1) 
;    exit(v : reg_rdi) 
;      inline: 53_22
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
     exit_53_22_end:
   if_53_5_end:
;  [55:5] print(hello.len, hello) 
;  print(len : reg_rdx, ptr : reg_rsi) 
;    inline: 55_5
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
   print_55_5_end:
;  [56:5] loop
   loop_56_5:
;    [57:9] print(prompt1.len, prompt1) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 57_9
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
     print_57_9_end:
;    len: qword[rbp-54]
;    [58:9] var len = read(input.len, input) - 1 
;    [58:13] len = read(input.len, input) - 1 
;    [58:19] read(input.len, input) - 1 
;    [58:19] read(input.len, input) - 1 
;    [58:19] len=read(input.len, input) 
;    [58:19] read(input.len, input) 
;    read(len : reg_rdx, ptr : reg_rsi) : nbytes 
;      inline: 58_19
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
       mov qword[rbp-54], rax
;      [29:22] # return value 
;      free rsi
;      free rdx
     read_58_19_end:
;    [58:44] len- 1 
     sub qword[rbp-54], 1
;    [58:49] # -1 don't include the '\n' 
     if_59_12:
;    [59:12] ? len == 0 
;    [59:12] ? len == 0 
     cmp_59_12:
     cmp qword[rbp-54], 0
     jne if_59_9_end
     if_59_12_code:  ; opt1
;      [60:13] break 
       jmp loop_56_5_end
     if_59_9_end:
     if_62_12:
;    [62:12] ? len <= itm.vel.x 
;    [62:12] ? len <= itm.vel.x 
     cmp_62_12:
;    alloc r15
     mov r15, qword[rbp-31]
     cmp qword[rbp-54], r15
;    free r15
     jg if_62_9_end
     if_62_12_code:  ; opt1
;      [63:13] print(prompt2.len, prompt2) 
;      print(len : reg_rdx, ptr : reg_rsi) 
;        inline: 63_13
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
       print_63_13_end:
;      [64:13] continue 
       jmp loop_56_5
     if_62_9_end:
;    [66:9] print(prompt3.len, prompt3) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 66_9
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
     print_66_9_end:
;    [67:9] print(len, input) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 67_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, qword[rbp-54]
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
     print_67_9_end:
;    [68:9] print(dot.len, dot) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 68_9
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
     print_68_9_end:
;    [69:9] print(nl.len, nl) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 69_9
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
     print_69_9_end:
   jmp loop_56_5
   loop_56_5_end:
; main end

; system call: exit 0
mov rax, 60
mov rdi, 0
syscall

; max scratch registers in use: 1
;            max frames in use: 7
