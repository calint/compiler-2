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
;[33:1] # member 'x' being default size (64 bit) 
;[34:1] # member 'y' being 32 bit 
;[35:1] # member 'z' being 16 bit 
;[36:1] # member 'w' being 8 bit 
;[37:1] # member 'valid' being a boolean 
main:
;  [41:5] # declare 'v' on the stack 
;  v: qword[rbp-16]
;  [42:5] var v : vector = 3 
;  [42:9] v : vector = 3 
;  [42:22] 3 
;  [42:22] 3 
;  [42:22] v=3 
   mov qword[rbp-16], 3
;  [42:27] # v.x is set to 3 
;  [43:5] v.y = 4 
;  [43:11] 4 
;  [43:11] 4 
;  [43:11] v.y=4 
   mov dword[rbp-8], 4
;  [44:5] v.z = 5 
;  [44:11] 5 
;  [44:11] 5 
;  [44:11] v.z=5 
   mov word[rbp-4], 5
;  [45:5] v.w = 6 
;  [45:11] 6 
;  [45:11] 6 
;  [45:11] v.w=6 
   mov byte[rbp-2], 6
;  [46:5] v.valid = true 
;  [46:15] true 
;  [46:15] true 
;  [46:15] v.valid=true 
   mov byte[rbp-1], true
;  [48:5] # check if valid 
   if_49_8:
;  [49:8] ? not v.valid 
;  [49:8] ? not v.valid 
   cmp_49_8:
   cmp byte[rbp-1], 0
   jne if_49_5_end
   if_49_8_code:  ; opt1
;    [49:20] exit(1) 
;    exit(v : reg_rdi) 
;      inline: 49_20
;      alloc rdi
;      alias v -> rdi
       mov rdi, 1
;      [10:5] mov(rax, 60) 
       mov rax, 60
;      [10:18] # exit system call 
;      [11:5] mov(rdi, v) 
;      [11:18] # return code 
;      [12:5] syscall 
       syscall
;      free rdi
     exit_49_20_end:
   if_49_5_end:
;  [51:5] print(hello.len, hello) 
;  print(len : reg_rdx, ptr : reg_rsi) 
;    inline: 51_5
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
   print_51_5_end:
;  [52:5] loop
   loop_52_5:
;    [53:9] print(prompt1.len, prompt1) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 53_9
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
     print_53_9_end:
;    len: qword[rbp-24]
;    [54:9] var len = read(input.len, input) - 1 
;    [54:13] len = read(input.len, input) - 1 
;    [54:19] read(input.len, input) - 1 
;    [54:19] read(input.len, input) - 1 
;    [54:19] len=read(input.len, input) 
;    [54:19] read(input.len, input) 
;    read(len : reg_rdx, ptr : reg_rsi) : nbytes_read 
;      inline: 54_19
;      alias nbytes_read -> len
;      alloc rdx
;      alias len -> rdx
       mov rdx, input.len
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, input
;      [24:5] mov(rax, 0) 
       mov rax, 0
;      [24:20] # read system call 
;      [25:5] mov(rdi, 0) 
       mov rdi, 0
;      [25:20] # file descriptor for standard input 
;      [26:5] mov(rsi, ptr) 
;      [26:20] # buffer address 
;      [27:5] mov(rdx, len) 
;      [27:20] # buffer size 
;      [28:5] syscall 
       syscall
;      [29:5] mov(nbytes_read, rax) 
       mov qword[rbp-24], rax
;      [29:27] # return value 
;      free rsi
;      free rdx
     read_54_19_end:
;    [54:44] len- 1 
     sub qword[rbp-24], 1
;    [54:49] # -1 don't include the '\n' 
     if_55_12:
;    [55:12] ? len == 0 
;    [55:12] ? len == 0 
     cmp_55_12:
     cmp qword[rbp-24], 0
     jne if_55_9_end
     if_55_12_code:  ; opt1
;      [56:13] break 
       jmp loop_52_5_end
     if_55_9_end:
     if_58_12:
;    [58:12] ? len <= v.x 
;    [58:12] ? len <= v.x 
     cmp_58_12:
;    alloc r15
     mov r15, qword[rbp-16]
     cmp qword[rbp-24], r15
;    free r15
     jg if_58_9_end
     if_58_12_code:  ; opt1
;      [59:13] print(prompt2.len, prompt2) 
;      print(len : reg_rdx, ptr : reg_rsi) 
;        inline: 59_13
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
       print_59_13_end:
;      [60:13] continue 
       jmp loop_52_5
     if_58_9_end:
;    [62:9] print(prompt3.len, prompt3) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 62_9
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
     print_62_9_end:
;    [63:9] print(len, input) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 63_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, qword[rbp-24]
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
     print_63_9_end:
;    [64:9] print(dot.len, dot) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 64_9
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
     print_64_9_end:
;    [65:9] print(nl.len, nl) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 65_9
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
     print_65_9_end:
   jmp loop_52_5
   loop_52_5_end:
; main end

; system call: exit 0
mov rax, 60
mov rdi, 0
syscall

; max scratch registers in use: 1
;            max frames in use: 7
