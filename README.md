# compiler-2: baz

Experimental compiler for a minimalistic, specialized language that targets NASM
x86_64 assembly on Linux.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by NASM for x86_64
* super loop program with non-reentrant inlined functions

## Supports

* built-in integer types (64, 32, 16, 8 bit)
* built-in boolean type
* user defined types
* inlined functions
* keywords: `func`, `field`, `var`, `loop`, `if`, `else`, `continue`, `break`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
run `prog.baz`
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`

## Sample

```text
field   hello = "hello world from baz\n"
field   input = "............................................................"
field prompt1 = "enter name:\n"
field prompt2 = "that is not a name.\n"
field prompt3 = "hello "
field     dot = "."
field      nl = "\n"

func exit(v : reg_rdi) {
    mov(rax, 60)  # exit system call
    mov(rdi, v)   # return code
    syscall()
}

func print(len : reg_rdx, ptr : reg_rsi) {
    mov(rax, 1)   # write system call
    mov(rdi, 1)   # file descriptor for standard out
    mov(rsi, ptr) # buffer address 
    mov(rdx, len) # buffer size
    syscall()
}

func read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes {
    mov(rax, 0)   # read system call
    mov(rdi, 0)   # file descriptor for standard input
    mov(rsi, ptr) # buffer address
    mov(rdx, len) # buffer size
    syscall()
    mov(nbytes, rax) # return value
}

func assert(expr : bool) {
    if not expr exit(1)
}

type point {x, y}

type object {pos : point, color : i32}

# inline function arguments are equivalent to mutable references
func foo(p : point) {
    p.x = 0b10    # binary value 2
    p.y = 0xb     # hex value 11
}

# default argument type is i64
func bar(arg) {
    arg = 0xff
}

func baz(arg) : i64 res {
    res = arg * 2
}

func main() {
    var p : point = {0, 0}
    foo(p)
    assert(p.x == 2)
    assert(p.y == 0xb)

    var i = 0
    bar(i)
    assert(i == 0xff)

    var j = 1
    var k = baz(j)
    assert(k == 2)

    var x = 1
    var y = 2

    var o1 : object = {{x * 10, y}, 0xff0000}
    assert(o1.pos.x == 10)
    assert(o1.pos.y == 2)
    assert(o1.color == 0xff0000)

    var p1 : point = {-x, -y}
    o1.pos = p1
    assert(o1.pos.x == -1)
    assert(o1.pos.y == -2)

    var o2 : object = o1
    assert(o2.pos.x == -1)
    assert(o2.pos.y == -2)
    assert(o2.color == 0xff0000)

    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        var len = read(input.len, input) - 1    # -1 don't include the '\n'
        if len == 0 {
            break
        } else if len <= 4 {
            print(prompt2.len, prompt2)
            continue
        } else {
            print(prompt3.len, prompt3)
            print(len, input)
            print(dot.len, dot)
            print(nl.len, nl)
        }
    }
}
```

## Generates

```text
true equ 1
false equ 0
section .data
align 4
hello: db 'hello world from baz',10,''
hello.len equ $-hello
input: db '............................................................'
input.len equ $-input
prompt1: db 'enter name:',10,''
prompt1.len equ $-prompt1
prompt2: db 'that is not a name.',10,''
prompt2.len equ $-prompt2
prompt3: db 'hello '
prompt3.len equ $-prompt3
dot: db '.'
dot.len equ $-dot
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
main:
     mov qword[rbp-16], 0
     mov qword[rbp-8], 0
     mov qword[rbp-16], 0b10
     mov qword[rbp-8], 0xb
   foo_57_5_end:
     cmp_58_12:
     cmp qword[rbp-16], 2
     jne bool_false_58_12
     bool_true_58_12:  ; opt1
     mov r15, true
     jmp bool_end_58_12
     bool_false_58_12:
     mov r15, false
     bool_end_58_12:
     if_33_8_58_5:
     cmp_33_8_58_5:
     cmp r15, 0
     jne if_33_5_58_5_end
     if_33_8_58_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_58_5_end:
     if_33_5_58_5_end:
   assert_58_5_end:
     cmp_59_12:
     cmp qword[rbp-8], 0xb
     jne bool_false_59_12
     bool_true_59_12:  ; opt1
     mov r15, true
     jmp bool_end_59_12
     bool_false_59_12:
     mov r15, false
     bool_end_59_12:
     if_33_8_59_5:
     cmp_33_8_59_5:
     cmp r15, 0
     jne if_33_5_59_5_end
     if_33_8_59_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_59_5_end:
     if_33_5_59_5_end:
   assert_59_5_end:
   mov qword[rbp-24], 0
     mov qword[rbp-24], 0xff
   bar_62_5_end:
     cmp_63_12:
     cmp qword[rbp-24], 0xff
     jne bool_false_63_12
     bool_true_63_12:  ; opt1
     mov r15, true
     jmp bool_end_63_12
     bool_false_63_12:
     mov r15, false
     bool_end_63_12:
     if_33_8_63_5:
     cmp_33_8_63_5:
     cmp r15, 0
     jne if_33_5_63_5_end
     if_33_8_63_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_63_5_end:
     if_33_5_63_5_end:
   assert_63_5_end:
   mov qword[rbp-32], 1
     mov r15, qword[rbp-32]
     imul r15, 2
     mov qword[rbp-40], r15
   baz_66_13_end:
     cmp_67_12:
     cmp qword[rbp-40], 2
     jne bool_false_67_12
     bool_true_67_12:  ; opt1
     mov r15, true
     jmp bool_end_67_12
     bool_false_67_12:
     mov r15, false
     bool_end_67_12:
     if_33_8_67_5:
     cmp_33_8_67_5:
     cmp r15, 0
     jne if_33_5_67_5_end
     if_33_8_67_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_67_5_end:
     if_33_5_67_5_end:
   assert_67_5_end:
   mov qword[rbp-48], 1
   mov qword[rbp-56], 2
       mov r15, qword[rbp-48]
       imul r15, 10
       mov qword[rbp-76], r15
       mov r15, qword[rbp-56]
       mov qword[rbp-68], r15
     mov dword[rbp-60], 0xff0000
     cmp_73_12:
     cmp qword[rbp-76], 10
     jne bool_false_73_12
     bool_true_73_12:  ; opt1
     mov r15, true
     jmp bool_end_73_12
     bool_false_73_12:
     mov r15, false
     bool_end_73_12:
     if_33_8_73_5:
     cmp_33_8_73_5:
     cmp r15, 0
     jne if_33_5_73_5_end
     if_33_8_73_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_73_5_end:
     if_33_5_73_5_end:
   assert_73_5_end:
     cmp_74_12:
     cmp qword[rbp-68], 2
     jne bool_false_74_12
     bool_true_74_12:  ; opt1
     mov r15, true
     jmp bool_end_74_12
     bool_false_74_12:
     mov r15, false
     bool_end_74_12:
     if_33_8_74_5:
     cmp_33_8_74_5:
     cmp r15, 0
     jne if_33_5_74_5_end
     if_33_8_74_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_74_5_end:
     if_33_5_74_5_end:
   assert_74_5_end:
     cmp_75_12:
     cmp dword[rbp-60], 0xff0000
     jne bool_false_75_12
     bool_true_75_12:  ; opt1
     mov r15, true
     jmp bool_end_75_12
     bool_false_75_12:
     mov r15, false
     bool_end_75_12:
     if_33_8_75_5:
     cmp_33_8_75_5:
     cmp r15, 0
     jne if_33_5_75_5_end
     if_33_8_75_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_75_5_end:
     if_33_5_75_5_end:
   assert_75_5_end:
     mov r15, qword[rbp-48]
     mov qword[rbp-92], r15
     neg qword[rbp-92]
     mov r15, qword[rbp-56]
     mov qword[rbp-84], r15
     neg qword[rbp-84]
     mov r15, qword[rbp-92]
     mov qword[rbp-76], r15
     mov r15, qword[rbp-84]
     mov qword[rbp-68], r15
     cmp_79_12:
     cmp qword[rbp-76], -1
     jne bool_false_79_12
     bool_true_79_12:  ; opt1
     mov r15, true
     jmp bool_end_79_12
     bool_false_79_12:
     mov r15, false
     bool_end_79_12:
     if_33_8_79_5:
     cmp_33_8_79_5:
     cmp r15, 0
     jne if_33_5_79_5_end
     if_33_8_79_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_79_5_end:
     if_33_5_79_5_end:
   assert_79_5_end:
     cmp_80_12:
     cmp qword[rbp-68], -2
     jne bool_false_80_12
     bool_true_80_12:  ; opt1
     mov r15, true
     jmp bool_end_80_12
     bool_false_80_12:
     mov r15, false
     bool_end_80_12:
     if_33_8_80_5:
     cmp_33_8_80_5:
     cmp r15, 0
     jne if_33_5_80_5_end
     if_33_8_80_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_80_5_end:
     if_33_5_80_5_end:
   assert_80_5_end:
       mov r15, qword[rbp-76]
       mov qword[rbp-112], r15
       mov r15, qword[rbp-68]
       mov qword[rbp-104], r15
     mov r15d, dword[rbp-60]
     mov dword[rbp-96], r15d
     cmp_83_12:
     cmp qword[rbp-112], -1
     jne bool_false_83_12
     bool_true_83_12:  ; opt1
     mov r15, true
     jmp bool_end_83_12
     bool_false_83_12:
     mov r15, false
     bool_end_83_12:
     if_33_8_83_5:
     cmp_33_8_83_5:
     cmp r15, 0
     jne if_33_5_83_5_end
     if_33_8_83_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_83_5_end:
     if_33_5_83_5_end:
   assert_83_5_end:
     cmp_84_12:
     cmp qword[rbp-104], -2
     jne bool_false_84_12
     bool_true_84_12:  ; opt1
     mov r15, true
     jmp bool_end_84_12
     bool_false_84_12:
     mov r15, false
     bool_end_84_12:
     if_33_8_84_5:
     cmp_33_8_84_5:
     cmp r15, 0
     jne if_33_5_84_5_end
     if_33_8_84_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_84_5_end:
     if_33_5_84_5_end:
   assert_84_5_end:
     cmp_85_12:
     cmp dword[rbp-96], 0xff0000
     jne bool_false_85_12
     bool_true_85_12:  ; opt1
     mov r15, true
     jmp bool_end_85_12
     bool_false_85_12:
     mov r15, false
     bool_end_85_12:
     if_33_8_85_5:
     cmp_33_8_85_5:
     cmp r15, 0
     jne if_33_5_85_5_end
     if_33_8_85_5_code:  ; opt1
         mov rdi, 1
         mov rax, 60
         syscall
       exit_33_17_85_5_end:
     if_33_5_85_5_end:
   assert_85_5_end:
     mov rdx, hello.len
     mov rsi, hello
     mov rax, 1
     mov rdi, 1
     syscall
   print_87_5_end:
   loop_88_5:
       mov rdx, prompt1.len
       mov rsi, prompt1
       mov rax, 1
       mov rdi, 1
       syscall
     print_89_9_end:
       mov rdx, input.len
       mov rsi, input
       mov rax, 0
       mov rdi, 0
       syscall
       mov qword[rbp-120], rax
     read_90_19_end:
     sub qword[rbp-120], 1
     if_91_12:
     cmp_91_12:
     cmp qword[rbp-120], 0
     jne if_93_19
     if_91_12_code:  ; opt1
       jmp loop_88_5_end
     jmp if_91_9_end
     if_93_19:
     cmp_93_19:
     cmp qword[rbp-120], 4
     jg if_else_91_9
     if_93_19_code:  ; opt1
         mov rdx, prompt2.len
         mov rsi, prompt2
         mov rax, 1
         mov rdi, 1
         syscall
       print_94_13_end:
       jmp loop_88_5
     jmp if_91_9_end
     if_else_91_9:
           mov rdx, prompt3.len
           mov rsi, prompt3
           mov rax, 1
           mov rdi, 1
           syscall
         print_97_13_end:
           mov rdx, qword[rbp-120]
           mov rsi, input
           mov rax, 1
           mov rdi, 1
           syscall
         print_98_13_end:
           mov rdx, dot.len
           mov rsi, dot
           mov rax, 1
           mov rdi, 1
           syscall
         print_99_13_end:
           mov rdx, nl.len
           mov rsi, nl
           mov rax, 1
           mov rdi, 1
           syscall
         print_100_13_end:
     if_91_9_end:
   jmp loop_88_5
   loop_88_5_end:
mov rax, 60
mov rdi, 0
syscall
```

