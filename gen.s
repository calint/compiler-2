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

assert:
;  assert(expr : bool) 
   push rbp
   mov rbp, rsp
;  expr: byte[rbp+16]
   if_8_8:
;  [8:8] ? not expr 
;  [8:8] ? not expr 
   cmp_8_8:
   cmp byte[rbp+16], 0
   jne if_8_5_end
   if_8_8_code:  ; opt1
;    [8:17] exit(1) 
;    exit(v : reg_rdi) 
;      inline: 8_17
;      alloc rdi
;      alias v -> rdi
       mov rdi, 1
;      [2:5] mov(rax, 60) 
       mov rax, 60
;      [2:19] # exit system call 
;      [3:5] mov(rdi, v) 
;      [3:19] # return code 
;      [4:5] syscall 
       syscall
;      free rdi
     exit_8_17_end:
   if_8_5_end:
   pop rbp
   ret

main:
;  a: dword[rbp-4]
;  [16:5] var a : i32 = 10 
;  [16:9] a : i32 = 10 
;  [16:19] 10 
;  [16:19] 10 
;  [16:19] a=10 
   mov dword[rbp-4], 10
;  pos: qword[rbp-19]
;  [17:5] var pos : vector = {a, a + 1, 12, 13}
;  [17:9] pos : vector = {a, a + 1, 12, 13}
;  [17:24] {a, a + 1, 12, 13}
;    [17:24] {a, a + 1, 12, 13}
;    [17:25] a
;    [17:25] a
;    [17:25] pos.x=a
;    alloc r15
     movsx r15, dword[rbp-4]
     mov qword[rbp-19], r15
;    free r15
;    [17:28] a + 1
;    [17:28] a + 1
;    [17:28] pos.y=a 
;    alloc r15
     mov r15d, dword[rbp-4]
     mov dword[rbp-11], r15d
;    free r15
;    [17:32] pos.y+ 1
     add dword[rbp-11], 1
;    [17:35] 12
;    [17:35] 12
;    [17:35] pos.z=12
     mov word[rbp-7], 12
;    [17:39] 13
;    [17:39] 13
;    [17:39] pos.w=13
     mov byte[rbp-5], 13
;  [18:5] assert(pos.x == 10) 
   sub rsp, 19
;  alloc r15
;    [18:12] pos.x == 10
;    [18:12] ? pos.x == 10
;    [18:12] ? pos.x == 10
     cmp_18_12:
     cmp qword[rbp-19], 10
     jne false_18_12
     true_18_12:  ; opt1
     mov r15, 1
     jmp end_18_12
     false_18_12:
     mov r15, 0
     end_18_12:
   push r15
;  free r15
   call assert
   add rsp, 27
;  [19:5] assert(pos.y == 11) 
   sub rsp, 19
;  alloc r15
;    [19:12] pos.y == 11
;    [19:12] ? pos.y == 11
;    [19:12] ? pos.y == 11
     cmp_19_12:
     cmp dword[rbp-11], 11
     jne false_19_12
     true_19_12:  ; opt1
     mov r15, 1
     jmp end_19_12
     false_19_12:
     mov r15, 0
     end_19_12:
   push r15
;  free r15
   call assert
   add rsp, 27
;  [20:5] assert(pos.z == 12) 
   sub rsp, 19
;  alloc r15
;    [20:12] pos.z == 12
;    [20:12] ? pos.z == 12
;    [20:12] ? pos.z == 12
     cmp_20_12:
     cmp word[rbp-7], 12
     jne false_20_12
     true_20_12:  ; opt1
     mov r15, 1
     jmp end_20_12
     false_20_12:
     mov r15, 0
     end_20_12:
   push r15
;  free r15
   call assert
   add rsp, 27
;  [21:5] assert(pos.w == 13) 
   sub rsp, 19
;  alloc r15
;    [21:12] pos.w == 13
;    [21:12] ? pos.w == 13
;    [21:12] ? pos.w == 13
     cmp_21_12:
     cmp byte[rbp-5], 13
     jne false_21_12
     true_21_12:  ; opt1
     mov r15, 1
     jmp end_21_12
     false_21_12:
     mov r15, 0
     end_21_12:
   push r15
;  free r15
   call assert
   add rsp, 27
;  obj: qword[rbp-50]
;  [23:5] var obj : object = {{a + a, 2, 3, 4}, {5, 6, 7, 8}, true}
;  [23:9] obj : object = {{a + a, 2, 3, 4}, {5, 6, 7, 8}, true}
;  [23:24] {{a + a, 2, 3, 4}, {5, 6, 7, 8}, true}
;    [23:24] {{a + a, 2, 3, 4}, {5, 6, 7, 8}, true}
;      [23:25] {a + a, 2, 3, 4}
;      [23:26] a + a
;      [23:26] a + a
;      [23:26] obj.pos.x=a 
;      alloc r15
       movsx r15, dword[rbp-4]
       mov qword[rbp-50], r15
;      free r15
;      [23:30] obj.pos.x+ a
;      alloc r15
       movsx r15, dword[rbp-4]
       add qword[rbp-50], r15
;      free r15
;      [23:33] 2
;      [23:33] 2
;      [23:33] obj.pos.y=2
       mov dword[rbp-42], 2
;      [23:36] 3
;      [23:36] 3
;      [23:36] obj.pos.z=3
       mov word[rbp-38], 3
;      [23:39] 4
;      [23:39] 4
;      [23:39] obj.pos.w=4
       mov byte[rbp-36], 4
;      [23:43] {5, 6, 7, 8}
;      [23:44] 5
;      [23:44] 5
;      [23:44] obj.vel.x=5
       mov qword[rbp-35], 5
;      [23:47] 6
;      [23:47] 6
;      [23:47] obj.vel.y=6
       mov dword[rbp-27], 6
;      [23:50] 7
;      [23:50] 7
;      [23:50] obj.vel.z=7
       mov word[rbp-23], 7
;      [23:53] 8
;      [23:53] 8
;      [23:53] obj.vel.w=8
       mov byte[rbp-21], 8
;    [23:57] true
     mov byte[rbp-20], true
;  [24:5] assert(obj.pos.x == 20) 
   sub rsp, 50
;  alloc r15
;    [24:12] obj.pos.x == 20
;    [24:12] ? obj.pos.x == 20
;    [24:12] ? obj.pos.x == 20
     cmp_24_12:
     cmp qword[rbp-50], 20
     jne false_24_12
     true_24_12:  ; opt1
     mov r15, 1
     jmp end_24_12
     false_24_12:
     mov r15, 0
     end_24_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [25:5] assert(obj.pos.y == 2) 
   sub rsp, 50
;  alloc r15
;    [25:12] obj.pos.y == 2
;    [25:12] ? obj.pos.y == 2
;    [25:12] ? obj.pos.y == 2
     cmp_25_12:
     cmp dword[rbp-42], 2
     jne false_25_12
     true_25_12:  ; opt1
     mov r15, 1
     jmp end_25_12
     false_25_12:
     mov r15, 0
     end_25_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [26:5] assert(obj.pos.z == 3) 
   sub rsp, 50
;  alloc r15
;    [26:12] obj.pos.z == 3
;    [26:12] ? obj.pos.z == 3
;    [26:12] ? obj.pos.z == 3
     cmp_26_12:
     cmp word[rbp-38], 3
     jne false_26_12
     true_26_12:  ; opt1
     mov r15, 1
     jmp end_26_12
     false_26_12:
     mov r15, 0
     end_26_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [27:5] assert(obj.pos.w == 4) 
   sub rsp, 50
;  alloc r15
;    [27:12] obj.pos.w == 4
;    [27:12] ? obj.pos.w == 4
;    [27:12] ? obj.pos.w == 4
     cmp_27_12:
     cmp byte[rbp-36], 4
     jne false_27_12
     true_27_12:  ; opt1
     mov r15, 1
     jmp end_27_12
     false_27_12:
     mov r15, 0
     end_27_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [28:5] assert(obj.vel.x == 5) 
   sub rsp, 50
;  alloc r15
;    [28:12] obj.vel.x == 5
;    [28:12] ? obj.vel.x == 5
;    [28:12] ? obj.vel.x == 5
     cmp_28_12:
     cmp qword[rbp-35], 5
     jne false_28_12
     true_28_12:  ; opt1
     mov r15, 1
     jmp end_28_12
     false_28_12:
     mov r15, 0
     end_28_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [29:5] assert(obj.vel.y == 6) 
   sub rsp, 50
;  alloc r15
;    [29:12] obj.vel.y == 6
;    [29:12] ? obj.vel.y == 6
;    [29:12] ? obj.vel.y == 6
     cmp_29_12:
     cmp dword[rbp-27], 6
     jne false_29_12
     true_29_12:  ; opt1
     mov r15, 1
     jmp end_29_12
     false_29_12:
     mov r15, 0
     end_29_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [30:5] assert(obj.vel.z == 7) 
   sub rsp, 50
;  alloc r15
;    [30:12] obj.vel.z == 7
;    [30:12] ? obj.vel.z == 7
;    [30:12] ? obj.vel.z == 7
     cmp_30_12:
     cmp word[rbp-23], 7
     jne false_30_12
     true_30_12:  ; opt1
     mov r15, 1
     jmp end_30_12
     false_30_12:
     mov r15, 0
     end_30_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [31:5] assert(obj.vel.w == 8) 
   sub rsp, 50
;  alloc r15
;    [31:12] obj.vel.w == 8
;    [31:12] ? obj.vel.w == 8
;    [31:12] ? obj.vel.w == 8
     cmp_31_12:
     cmp byte[rbp-21], 8
     jne false_31_12
     true_31_12:  ; opt1
     mov r15, 1
     jmp end_31_12
     false_31_12:
     mov r15, 0
     end_31_12:
   push r15
;  free r15
   call assert
   add rsp, 58
;  [32:5] assert(obj.valid == true) 
   sub rsp, 50
;  alloc r15
;    [32:12] obj.valid == true
;    [32:12] ? obj.valid == true
;    [32:12] ? obj.valid == true
     cmp_32_12:
     cmp byte[rbp-20], true
     jne false_32_12
     true_32_12:  ; opt1
     mov r15, 1
     jmp end_32_12
     false_32_12:
     mov r15, 0
     end_32_12:
   push r15
;  free r15
   call assert
   add rsp, 58
; main end

; system call: exit 0
mov rax, 60
mov rdi, 0
syscall

; max scratch registers in use: 1
;            max frames in use: 5
