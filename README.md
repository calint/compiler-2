# compiler-2
experimental compiler to nasm x86_64 for linux

intention:
* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler for x86_64 nasm
* generating inlined functions for a super-loop, no reentrance type of program

```
field   hello = "hello world from baz\n"
field   input = "............................................................"
field prompt1 = "enter name: "
field prompt2 = "that is not a name.\n"
field prompt3 = "hello "
field      nl = "\n"
field     dot = "."

func inline exit(v : reg_rdi) {
    mov(rax, 60) # exit system call
    mov(rdi, v)  # return code
    syscall
}

func inline print(len : reg_rdx, ptr : reg_rsi) {
    mov(rax, 1)   # write system call
    mov(rdi, 1)   # file descriptor for standard out
    mov(rsi, ptr) # buffer address 
    mov(rdx, len) # buffer size
    syscall
}

func inline read(len : reg_rdx, ptr : reg_rsi) : nbytes_read {
	mov(rax, 0)    # read system call
	mov(rdi, 0)    # file descriptor for standard input
	mov(rsi, ptr)  # buffer address
	mov(rdx, len)  # buffer size
	syscall
    mov(nbytes_read, rax) # return value
}

func inline main {
    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        var len = read(input.len, input) - 1    # -1 don't include the '\n'
        if len == 0 {
            break
        }
        if len <= 4 {
            print(prompt2.len, prompt2)
            continue
        }
        print(prompt3.len, prompt3)
        print(len, input)
        print(dot.len, dot)
        print(nl.len, nl)
    }
    exit(0)
}
```

generates:
```
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

main:
;  [33:5] print(hello.len, hello) 
;  print(len : reg_rdx, ptr : reg_rsi) 
;    inline: 33_5
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
   print_33_5_end:
;  [34:5] loop
   loop_34_5:
;    [35:9] print(prompt1.len, prompt1) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 35_9
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
     print_35_9_end:
;    len: qword[rbp-8]
;    [36:9] var len = read(input.len, input) - 1 
;    [36:13] len = read(input.len, input) - 1 
;    [36:19] read(input.len, input) - 1 
;    [36:19] read(input.len, input) - 1 
;    [36:19] len=read(input.len, input) 
;    [36:19] read(input.len, input) 
;    read(len : reg_rdx, ptr : reg_rsi) : nbytes_read 
;      inline: 36_19
;      alias nbytes_read -> len
;      alloc rdx
;      alias len -> rdx
       mov rdx, input.len
;      alloc rsi
;      alias ptr -> rsi
       mov rsi, input
;      [24:2] mov(rax, 0) 
       mov rax, 0
;      [24:17] # read system call 
;      [25:2] mov(rdi, 0) 
       mov rdi, 0
;      [25:17] # file descriptor for standard input 
;      [26:2] mov(rsi, ptr) 
;      [26:17] # buffer address 
;      [27:2] mov(rdx, len) 
;      [27:17] # buffer size 
;      [28:2] syscall 
       syscall
;      [29:5] mov(nbytes_read, rax) 
       mov qword[rbp-8], rax
;      [29:27] # return value 
;      free rsi
;      free rdx
     read_36_19_end:
;    [36:44] len- 1 
     sub qword[rbp-8], 1
;    [36:49] # -1 don't include the '\n' 
     if_37_12:
;    [37:12] ? len == 0 
;    [37:12] ? len == 0 
     cmp_37_12:
     cmp qword[rbp-8], 0
     jne if_37_9_end
     if_37_12_code:  ; opt1
;      [38:13] break 
       jmp loop_34_5_end
     if_37_9_end:
     if_40_12:
;    [40:12] ? len <= 4 
;    [40:12] ? len <= 4 
     cmp_40_12:
     cmp qword[rbp-8], 4
     jg if_40_9_end
     if_40_12_code:  ; opt1
;      [41:13] print(prompt2.len, prompt2) 
;      print(len : reg_rdx, ptr : reg_rsi) 
;        inline: 41_13
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
       print_41_13_end:
;      [42:13] continue 
       jmp loop_34_5
     if_40_9_end:
;    [44:9] print(prompt3.len, prompt3) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 44_9
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
     print_44_9_end:
;    [45:9] print(len, input) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 45_9
;      alloc rdx
;      alias len -> rdx
       mov rdx, qword[rbp-8]
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
     print_45_9_end:
;    [46:9] print(dot.len, dot) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 46_9
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
     print_46_9_end:
;    [47:9] print(nl.len, nl) 
;    print(len : reg_rdx, ptr : reg_rsi) 
;      inline: 47_9
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
     print_47_9_end:
   jmp loop_34_5
   loop_34_5_end:
;  [49:5] exit(0) 
;  exit(v : reg_rdi) 
;    inline: 49_5
;    alloc rdi
;    alias v -> rdi
     mov rdi, 0
;    [10:5] mov(rax, 60) 
     mov rax, 60
;    [10:18] # exit system call 
;    [11:5] mov(rdi, v) 
;    [11:18] # return code 
;    [12:5] syscall 
     syscall
;    free rdi
   exit_49_5_end:

; max scratch registers in use: 1
;            max frames in use: 7
```