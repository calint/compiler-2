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
* arrays
* optional bounds checking at runtime
  * optional line number
* inlined functions
* keywords: `func`, `type`, `field`, `var`, `loop`, `if`, `else`, `continue`,
`break`, `return`
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`, `address_of`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
run `prog.baz`
  * optional parameters: _stack size_, _bounds check_, with _line number information_ e.g:
    * `./run-baz.sh myprogram.baz --stack=262144`: stack size, no runtime checks
    * `./run-baz.sh myprogram.baz --stack=262144 --check=bounds`: checks bounds without
    line number information (faster)
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=bounds,line`: checks
    bounds with line number information
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    40           1225            686           5840
C++                              1             77             83            542
-------------------------------------------------------------------------------
SUM:                            41           1302            769           6382
-------------------------------------------------------------------------------
```

## Sample

```text
field   hello = "hello world from baz\n"
field prompt1 = "enter name:\n"
field prompt2 = "that is not a name.\n"
field prompt3 = "hello "
field     dot = "."
field      nl = "\n"

# all functions are inlined

# arguments can be placed in specified register using `reg_...` syntax

func exit(v : reg_rdi) {
    mov(rax, 60)  # exit system call
    mov(rdi, v)   # return code
    syscall()
}

func assert(expr : bool) if not expr exit(1)

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

# user types are defined using keyword `type`

# default type is `i64` and does not need to be specified

type point {x, y}

type object {pos : point, color : i32}

type world { locations : i64[8] }

# function arguments are equivalent to mutable references

func foo(pt : point) {
    pt.x = 0b10    # binary value 2
    pt.y = 0xb     # hex value 11
}

# default argument type is `i64`

func bar(arg) {
    if arg == 0 return
    arg = 0xff
}

# return target is specified as a variable, in this case `res`

func inv(i : i32) : i32 res {
    res = ~i
}

func baz(arg) : i64 res {
    res = arg * 2
}

# array arguments are declared with type and []

func faz(arg : i32[]) {
    arg[1] = 0xfe
}

func main() {
    var arr : i32[4]
    # arrays are initialized to 0

    var ix = 1

    arr[ix] = 2
    arr[ix + 1] = arr[ix]
    assert(arr[1] == 2)
    assert(arr[2] == 2)

    array_copy(arr[2], arr, 2)
    # copy from, to, number of elements
    assert(arr[0] == 2)

    var arr1 : i32[8]
    array_copy(arr, arr1, 4)
    assert(arrays_equal(arr, arr1, 4))

    arr1[2] = -1
    assert(not arrays_equal(arr, arr1, 4))

    ix = 3
    arr[ix] = ~inv(arr[ix - 1])
    assert(arr[ix] == 2)

    faz(arr)
    assert(arr[1] == 0xfe)

    var p : point = {0, 0}
    foo(p)
    assert(p.x == 2)
    assert(p.y == 0xb)

    var i = 0
    bar(i)
    assert(i == 0)

    i = 1
    bar(i)
    assert(i == 0xff)

    var j = 1
    var k = baz(j)
    assert(k == 2)

    k = baz(1)
    assert(k == 2)

    var p0 : point = {baz(2), 0}
    assert(p0.x == 4)

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

    var o3 : object[1]
    # index 0 in an array can be accessed without array index
    o3.pos.y = 73
    assert(o3[0].pos.y == 73)

    var input_buffer : i8[80]

    var worlds : world[8]
    worlds[1].locations[1] = 0xffee
    assert(worlds[1].locations[1] == 0xffee)

    array_copy(
        worlds[1].locations,
        worlds[0].locations,
        array_size_of(worlds.locations)
    )
    assert(worlds[0].locations[1] == 0xffee)
    assert(arrays_equal(
             worlds[0].locations,
             worlds[1].locations,
             array_size_of(worlds.locations)
          ))

    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        
        var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
        # note: `array_size_of` and `address_of` are built-in functions
        #       -1 to not include the trailing '\n'

        if len == 0 {
            break
        } else if len <= 4 {
            print(prompt2.len, prompt2)
            continue
        } else {
            print(prompt3.len, prompt3)
            print(len, address_of(input_buffer))
            print(dot.len, dot)
            print(nl.len, nl)
        }
    }
}
```

## Generates

```nasm
DEFAULT REL
section .bss
stk resd 131072
stk.end:
true equ 1
false equ 0
section .data
hello: db 'hello world from baz', 10,''
hello.len equ $ - hello
prompt1: db 'enter name:', 10,''
prompt1.len equ $ - prompt1
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $ - prompt2
prompt3: db 'hello '
prompt3.len equ $ - prompt3
dot: db '.'
dot.len equ $ - dot
nl: db '', 10,''
nl.len equ $ - nl
section .text
bits 64
global _start
_start:
mov rsp,stk.end
main:
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    mov r14, 83
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, 84
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    mov r13, 84
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_85_12:
        mov r13, 1
        mov r12, 85
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_85_12
    jmp bool_true_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
    assert_85_5:
        if_18_29_85_5:
        cmp_18_29_85_5:
        test r15, r15
        jne if_18_26_85_5_end
        jmp if_18_29_85_5_code
        if_18_29_85_5_code:
            mov rdi, 1
            exit_18_38_85_5:
                    mov rax, 60
                syscall
            exit_18_38_85_5_end:
        if_18_26_85_5_end:
    assert_85_5_end:
    cmp_86_12:
        mov r13, 2
        mov r12, 86
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_86_12
    jmp bool_true_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
    assert_86_5:
        if_18_29_86_5:
        cmp_18_29_86_5:
        test r15, r15
        jne if_18_26_86_5_end
        jmp if_18_29_86_5_code
        if_18_29_86_5_code:
            mov rdi, 1
            exit_18_38_86_5:
                    mov rax, 60
                syscall
            exit_18_38_86_5_end:
        if_18_26_86_5_end:
    assert_86_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 88
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    mov r15, 88
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_90_12:
        mov r13, 0
        mov r12, 90
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_90_12
    jmp bool_true_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
    assert_90_5:
        if_18_29_90_5:
        cmp_18_29_90_5:
        test r15, r15
        jne if_18_26_90_5_end
        jmp if_18_29_90_5_code
        if_18_29_90_5_code:
            mov rdi, 1
            exit_18_38_90_5:
                    mov rax, 60
                syscall
            exit_18_38_90_5_end:
        if_18_26_90_5_end:
    assert_90_5_end:
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
    mov rcx, 4
    mov r15, 93
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    mov r15, 93
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_94_12:
        mov rcx, 4
        mov r13, 94
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 94
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_94_12
        mov r14, false
        jmp cmps_end_94_12
        cmps_eq_94_12:
        mov r14, true
        cmps_end_94_12:
    test r14, r14
    je bool_false_94_12
    jmp bool_true_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
    assert_94_5:
        if_18_29_94_5:
        cmp_18_29_94_5:
        test r15, r15
        jne if_18_26_94_5_end
        jmp if_18_29_94_5_code
        if_18_29_94_5_code:
            mov rdi, 1
            exit_18_38_94_5:
                    mov rax, 60
                syscall
            exit_18_38_94_5_end:
        if_18_26_94_5_end:
    assert_94_5_end:
    mov r15, 2
    mov r14, 96
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_97_12:
        mov rcx, 4
        mov r13, 97
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 97
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_97_16
        mov r14, false
        jmp cmps_end_97_16
        cmps_eq_97_16:
        mov r14, true
        cmps_end_97_16:
    test r14, r14
    jne bool_false_97_12
    jmp bool_true_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
    assert_97_5:
        if_18_29_97_5:
        cmp_18_29_97_5:
        test r15, r15
        jne if_18_26_97_5_end
        jmp if_18_29_97_5_code
        if_18_29_97_5_code:
            mov rdi, 1
            exit_18_38_97_5:
                    mov rax, 60
                syscall
            exit_18_38_97_5_end:
        if_18_26_97_5_end:
    assert_97_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r14, 100
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    mov r13, 100
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_100_16:
        lea r13, [rsp + r14 * 4 - 16]
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
        not dword [rsp + r15 * 4 - 16]
    inv_100_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_101_12:
        mov r13, qword [rsp - 24]
        mov r12, 101
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 2
    jne bool_false_101_12
    jmp bool_true_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
    assert_101_5:
        if_18_29_101_5:
        cmp_18_29_101_5:
        test r15, r15
        jne if_18_26_101_5_end
        jmp if_18_29_101_5_code
        if_18_29_101_5_code:
            mov rdi, 1
            exit_18_38_101_5:
                    mov rax, 60
                syscall
            exit_18_38_101_5_end:
        if_18_26_101_5_end:
    assert_101_5_end:
    faz_103_5:
        mov r15, 1
        mov r14, 74
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_103_5_end:
    cmp_104_12:
        mov r13, 1
        mov r12, 104
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
        movsx r14, dword [rsp + r13 * 4 - 16]
    cmp r14, 0xfe
    jne bool_false_104_12
    jmp bool_true_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
    assert_104_5:
        if_18_29_104_5:
        cmp_18_29_104_5:
        test r15, r15
        jne if_18_26_104_5_end
        jmp if_18_29_104_5_code
        if_18_29_104_5_code:
            mov rdi, 1
            exit_18_38_104_5:
                    mov rax, 60
                syscall
            exit_18_38_104_5_end:
        if_18_26_104_5_end:
    assert_104_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_107_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_107_5_end:
    cmp_108_12:
    cmp qword [rsp - 72], 2
    jne bool_false_108_12
    jmp bool_true_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
    assert_108_5:
        if_18_29_108_5:
        cmp_18_29_108_5:
        test r15, r15
        jne if_18_26_108_5_end
        jmp if_18_29_108_5_code
        if_18_29_108_5_code:
            mov rdi, 1
            exit_18_38_108_5:
                    mov rax, 60
                syscall
            exit_18_38_108_5_end:
        if_18_26_108_5_end:
    assert_108_5_end:
    cmp_109_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_109_12
    jmp bool_true_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
    assert_109_5:
        if_18_29_109_5:
        cmp_18_29_109_5:
        test r15, r15
        jne if_18_26_109_5_end
        jmp if_18_29_109_5_code
        if_18_29_109_5_code:
            mov rdi, 1
            exit_18_38_109_5:
                    mov rax, 60
                syscall
            exit_18_38_109_5_end:
        if_18_26_109_5_end:
    assert_109_5_end:
    mov qword [rsp - 80], 0
    bar_112_5:
        if_57_8_112_5:
        cmp_57_8_112_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_112_5_end
        jmp if_57_8_112_5_code
        if_57_8_112_5_code:
            jmp bar_112_5_end
        if_57_5_112_5_end:
        mov qword [rsp - 80], 0xff
    bar_112_5_end:
    cmp_113_12:
    cmp qword [rsp - 80], 0
    jne bool_false_113_12
    jmp bool_true_113_12
    bool_true_113_12:
    mov r15, true
    jmp bool_end_113_12
    bool_false_113_12:
    mov r15, false
    bool_end_113_12:
    assert_113_5:
        if_18_29_113_5:
        cmp_18_29_113_5:
        test r15, r15
        jne if_18_26_113_5_end
        jmp if_18_29_113_5_code
        if_18_29_113_5_code:
            mov rdi, 1
            exit_18_38_113_5:
                    mov rax, 60
                syscall
            exit_18_38_113_5_end:
        if_18_26_113_5_end:
    assert_113_5_end:
    mov qword [rsp - 80], 1
    bar_116_5:
        if_57_8_116_5:
        cmp_57_8_116_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_116_5_end
        jmp if_57_8_116_5_code
        if_57_8_116_5_code:
            jmp bar_116_5_end
        if_57_5_116_5_end:
        mov qword [rsp - 80], 0xff
    bar_116_5_end:
    cmp_117_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_117_12
    jmp bool_true_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
    assert_117_5:
        if_18_29_117_5:
        cmp_18_29_117_5:
        test r15, r15
        jne if_18_26_117_5_end
        jmp if_18_29_117_5_code
        if_18_29_117_5_code:
            mov rdi, 1
            exit_18_38_117_5:
                    mov rax, 60
                syscall
            exit_18_38_117_5_end:
        if_18_26_117_5_end:
    assert_117_5_end:
    mov qword [rsp - 88], 1
    baz_120_13:
        mov r15, qword [rsp - 88]
        imul r15, 2
    baz_120_13_end:
    mov qword [rsp - 96], r15
    cmp_121_12:
    cmp qword [rsp - 96], 2
    jne bool_false_121_12
    jmp bool_true_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
    assert_121_5:
        if_18_29_121_5:
        cmp_18_29_121_5:
        test r15, r15
        jne if_18_26_121_5_end
        jmp if_18_29_121_5_code
        if_18_29_121_5_code:
            mov rdi, 1
            exit_18_38_121_5:
                    mov rax, 60
                syscall
            exit_18_38_121_5_end:
        if_18_26_121_5_end:
    assert_121_5_end:
    baz_123_9:
        mov r15, 1
        imul r15, 2
    baz_123_9_end:
    mov qword [rsp - 96], r15
    cmp_124_12:
    cmp qword [rsp - 96], 2
    jne bool_false_124_12
    jmp bool_true_124_12
    bool_true_124_12:
    mov r15, true
    jmp bool_end_124_12
    bool_false_124_12:
    mov r15, false
    bool_end_124_12:
    assert_124_5:
        if_18_29_124_5:
        cmp_18_29_124_5:
        test r15, r15
        jne if_18_26_124_5_end
        jmp if_18_29_124_5_code
        if_18_29_124_5_code:
            mov rdi, 1
            exit_18_38_124_5:
                    mov rax, 60
                syscall
            exit_18_38_124_5_end:
        if_18_26_124_5_end:
    assert_124_5_end:
    baz_126_23:
        mov r15, 2
        imul r15, 2
    baz_126_23_end:
    mov qword [rsp - 112], r15
    mov qword [rsp - 104], 0
    cmp_127_12:
    cmp qword [rsp - 112], 4
    jne bool_false_127_12
    jmp bool_true_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
    assert_127_5:
        if_18_29_127_5:
        cmp_18_29_127_5:
        test r15, r15
        jne if_18_26_127_5_end
        jmp if_18_29_127_5_code
        if_18_29_127_5_code:
            mov rdi, 1
            exit_18_38_127_5:
                    mov rax, 60
                syscall
            exit_18_38_127_5_end:
        if_18_26_127_5_end:
    assert_127_5_end:
    mov qword [rsp - 120], 1
    mov qword [rsp - 128], 2
        mov r15, qword [rsp - 120]
        imul r15, 10
        mov qword [rsp - 148], r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
    mov dword [rsp - 132], 0xff0000
    cmp_133_12:
    cmp qword [rsp - 148], 10
    jne bool_false_133_12
    jmp bool_true_133_12
    bool_true_133_12:
    mov r15, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15, false
    bool_end_133_12:
    assert_133_5:
        if_18_29_133_5:
        cmp_18_29_133_5:
        test r15, r15
        jne if_18_26_133_5_end
        jmp if_18_29_133_5_code
        if_18_29_133_5_code:
            mov rdi, 1
            exit_18_38_133_5:
                    mov rax, 60
                syscall
            exit_18_38_133_5_end:
        if_18_26_133_5_end:
    assert_133_5_end:
    cmp_134_12:
    cmp qword [rsp - 140], 2
    jne bool_false_134_12
    jmp bool_true_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
    assert_134_5:
        if_18_29_134_5:
        cmp_18_29_134_5:
        test r15, r15
        jne if_18_26_134_5_end
        jmp if_18_29_134_5_code
        if_18_29_134_5_code:
            mov rdi, 1
            exit_18_38_134_5:
                    mov rax, 60
                syscall
            exit_18_38_134_5_end:
        if_18_26_134_5_end:
    assert_134_5_end:
    cmp_135_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_135_12
    jmp bool_true_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
    assert_135_5:
        if_18_29_135_5:
        cmp_18_29_135_5:
        test r15, r15
        jne if_18_26_135_5_end
        jmp if_18_29_135_5_code
        if_18_29_135_5_code:
            mov rdi, 1
            exit_18_38_135_5:
                    mov rax, 60
                syscall
            exit_18_38_135_5_end:
        if_18_26_135_5_end:
    assert_135_5_end:
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
    neg qword [rsp - 164]
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
    neg qword [rsp - 156]
    lea rdi, [rsp - 148]
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
    cmp_139_12:
    cmp qword [rsp - 148], -1
    jne bool_false_139_12
    jmp bool_true_139_12
    bool_true_139_12:
    mov r15, true
    jmp bool_end_139_12
    bool_false_139_12:
    mov r15, false
    bool_end_139_12:
    assert_139_5:
        if_18_29_139_5:
        cmp_18_29_139_5:
        test r15, r15
        jne if_18_26_139_5_end
        jmp if_18_29_139_5_code
        if_18_29_139_5_code:
            mov rdi, 1
            exit_18_38_139_5:
                    mov rax, 60
                syscall
            exit_18_38_139_5_end:
        if_18_26_139_5_end:
    assert_139_5_end:
    cmp_140_12:
    cmp qword [rsp - 140], -2
    jne bool_false_140_12
    jmp bool_true_140_12
    bool_true_140_12:
    mov r15, true
    jmp bool_end_140_12
    bool_false_140_12:
    mov r15, false
    bool_end_140_12:
    assert_140_5:
        if_18_29_140_5:
        cmp_18_29_140_5:
        test r15, r15
        jne if_18_26_140_5_end
        jmp if_18_29_140_5_code
        if_18_29_140_5_code:
            mov rdi, 1
            exit_18_38_140_5:
                    mov rax, 60
                syscall
            exit_18_38_140_5_end:
        if_18_26_140_5_end:
    assert_140_5_end:
    lea rdi, [rsp - 184]
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
    cmp_143_12:
    cmp qword [rsp - 184], -1
    jne bool_false_143_12
    jmp bool_true_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
    assert_143_5:
        if_18_29_143_5:
        cmp_18_29_143_5:
        test r15, r15
        jne if_18_26_143_5_end
        jmp if_18_29_143_5_code
        if_18_29_143_5_code:
            mov rdi, 1
            exit_18_38_143_5:
                    mov rax, 60
                syscall
            exit_18_38_143_5_end:
        if_18_26_143_5_end:
    assert_143_5_end:
    cmp_144_12:
    cmp qword [rsp - 176], -2
    jne bool_false_144_12
    jmp bool_true_144_12
    bool_true_144_12:
    mov r15, true
    jmp bool_end_144_12
    bool_false_144_12:
    mov r15, false
    bool_end_144_12:
    assert_144_5:
        if_18_29_144_5:
        cmp_18_29_144_5:
        test r15, r15
        jne if_18_26_144_5_end
        jmp if_18_29_144_5_code
        if_18_29_144_5_code:
            mov rdi, 1
            exit_18_38_144_5:
                    mov rax, 60
                syscall
            exit_18_38_144_5_end:
        if_18_26_144_5_end:
    assert_144_5_end:
    cmp_145_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_145_12
    jmp bool_true_145_12
    bool_true_145_12:
    mov r15, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15, false
    bool_end_145_12:
    assert_145_5:
        if_18_29_145_5:
        cmp_18_29_145_5:
        test r15, r15
        jne if_18_26_145_5_end
        jmp if_18_29_145_5_code
        if_18_29_145_5_code:
            mov rdi, 1
            exit_18_38_145_5:
                    mov rax, 60
                syscall
            exit_18_38_145_5_end:
        if_18_26_145_5_end:
    assert_145_5_end:
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
    mov qword [rsp - 196], 73
    cmp_150_12:
        lea r13, [rsp - 204]
        mov r12, 0
        mov r11, 150
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 1
        cmovge rbp, r11
        jge panic_bounds
        imul r12, 20
        add r13, r12
        mov r14, qword [r13 + 8]
    cmp r14, 73
    jne bool_false_150_12
    jmp bool_true_150_12
    bool_true_150_12:
    mov r15, true
    jmp bool_end_150_12
    bool_false_150_12:
    mov r15, false
    bool_end_150_12:
    assert_150_5:
        if_18_29_150_5:
        cmp_18_29_150_5:
        test r15, r15
        jne if_18_26_150_5_end
        jmp if_18_29_150_5_code
        if_18_29_150_5_code:
            mov rdi, 1
            exit_18_38_150_5:
                    mov rax, 60
                syscall
            exit_18_38_150_5_end:
        if_18_26_150_5_end:
    assert_150_5_end:
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
    lea r15, [rsp - 796]
    mov r14, 1
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 0xffee
    cmp_156_12:
        lea r13, [rsp - 796]
        mov r12, 1
        mov r11, 156
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        mov r11, 156
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8]
    cmp r14, 0xffee
    jne bool_false_156_12
    jmp bool_true_156_12
    bool_true_156_12:
    mov r15, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15, false
    bool_end_156_12:
    assert_156_5:
        if_18_29_156_5:
        cmp_18_29_156_5:
        test r15, r15
        jne if_18_26_156_5_end
        jmp if_18_29_156_5_code
        if_18_29_156_5_code:
            mov rdi, 1
            exit_18_38_156_5:
                    mov rax, 60
                syscall
            exit_18_38_156_5_end:
        if_18_26_156_5_end:
    assert_156_5_end:
    mov rcx, 8
    lea r15, [rsp - 796]
    mov r14, 1
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 159
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 796]
    mov r14, 0
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 160
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_163_12:
        lea r13, [rsp - 796]
        mov r12, 0
        mov r11, 163
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 1
        mov r11, 163
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        mov r14, qword [r13 + r12 * 8]
    cmp r14, 0xffee
    jne bool_false_163_12
    jmp bool_true_163_12
    bool_true_163_12:
    mov r15, true
    jmp bool_end_163_12
    bool_false_163_12:
    mov r15, false
    bool_end_163_12:
    assert_163_5:
        if_18_29_163_5:
        cmp_18_29_163_5:
        test r15, r15
        jne if_18_26_163_5_end
        jmp if_18_29_163_5_code
        if_18_29_163_5_code:
            mov rdi, 1
            exit_18_38_163_5:
                    mov rax, 60
                syscall
            exit_18_38_163_5_end:
        if_18_26_163_5_end:
    assert_163_5_end:
    cmp_164_12:
        mov rcx, 8
        lea r13, [rsp - 796]
        mov r12, 0
        mov r11, 165
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 165
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 796]
        mov r12, 1
        mov r11, 166
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 166
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        je cmps_eq_164_12
        mov r14, false
        jmp cmps_end_164_12
        cmps_eq_164_12:
        mov r14, true
        cmps_end_164_12:
    test r14, r14
    je bool_false_164_12
    jmp bool_true_164_12
    bool_true_164_12:
    mov r15, true
    jmp bool_end_164_12
    bool_false_164_12:
    mov r15, false
    bool_end_164_12:
    assert_164_5:
        if_18_29_164_5:
        cmp_18_29_164_5:
        test r15, r15
        jne if_18_26_164_5_end
        jmp if_18_29_164_5_code
        if_18_29_164_5_code:
            mov rdi, 1
            exit_18_38_164_5:
                    mov rax, 60
                syscall
            exit_18_38_164_5_end:
        if_18_26_164_5_end:
    assert_164_5_end:
    mov rdx, hello.len
    mov rsi, hello
    print_170_5:
            mov rax, 1
            mov rdi, 1
        syscall
    print_170_5_end:
    loop_171_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_172_9:
                mov rax, 1
                mov rdi, 1
            syscall
        print_172_9_end:
        mov rdx, 80
        lea rsi, [rsp - 284]
        read_174_19:
                mov rax, 0
                mov rdi, 0
            syscall
                mov qword [rsp - 804], rax
        read_174_19_end:
        sub qword [rsp - 804], 1
        if_178_12:
        cmp_178_12:
        cmp qword [rsp - 804], 0
        jne if_180_19
        jmp if_178_12_code
        if_178_12_code:
            jmp loop_171_5_end
        jmp if_178_9_end
        if_180_19:
        cmp_180_19:
        cmp qword [rsp - 804], 4
        jg if_else_178_9
        jmp if_180_19_code
        if_180_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_181_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_181_13_end:
            jmp loop_171_5
        jmp if_178_9_end
        if_else_178_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_184_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_184_13_end:
            mov rdx, qword [rsp - 804]
            lea rsi, [rsp - 284]
            print_185_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_185_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_186_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_186_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_187_13:
                    mov rax, 1
                    mov rdi, 1
                syscall
            print_187_13_end:
        if_178_9_end:
    jmp loop_171_5
    loop_171_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
panic_bounds:
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
    mov rax, rbp
    lea rdi, [num_buffer + 19]
    mov byte [rdi], 10
    dec rdi
    mov rcx, 10
.convert_loop:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jnz .convert_loop
    inc rdi
    mov rax, 1
    mov rsi, rdi
    lea rdx, [num_buffer + 20]
    sub rdx, rdi
    mov rdi, 2
    syscall
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
    msg_panic: db 'panic: bounds at line '
    msg_panic_len equ $ - msg_panic
section .bss
    num_buffer: resb 21
```

## With comments

```nasm

;
; generated by baz
;

DEFAULT REL

section .bss
stk resd 131072
stk.end:

true equ 1
false equ 0

section .data
;[1:1] field hello = "hello world from baz\n"
hello: db 'hello world from baz', 10,''
hello.len equ $ - hello
;[2:1] field prompt1 = "enter name:\n"
prompt1: db 'enter name:', 10,''
prompt1.len equ $ - prompt1
;[3:1] field prompt2 = "that is not a name.\n"
prompt2: db 'that is not a name.', 10,''
prompt2.len equ $ - prompt2
;[4:1] field prompt3 = "hello "
prompt3: db 'hello '
prompt3.len equ $ - prompt3
;[5:1] field dot = "."
dot: db '.'
dot.len equ $ - dot
;[6:1] field nl = "\n"
nl: db '', 10,''
nl.len equ $ - nl

section .text
bits 64
global _start
_start:
mov rsp,stk.end
;
; program
;
;[8:1] # all functions are inlined
;[10:1] # arguments can be placed in specified register using `reg_...` syntax
;[37:1] # user types are defined using keyword `type`
;[39:1] # default type is `i64` and does not need to be specified
;[41:1] point : 16 B    fields: 
;[41:1]       name :  offset :    size :  array? : array size
;[41:1]          x :       0 :       8 :      no :           
;[41:1]          y :       8 :       8 :      no :           

;[43:1] object : 20 B    fields: 
;[43:1]       name :  offset :    size :  array? : array size
;[43:1]        pos :       0 :      16 :      no :           
;[43:1]      color :      16 :       4 :      no :           

;[45:1] world : 64 B    fields: 
;[45:1]       name :  offset :    size :  array? : array size
;[45:1]  locations :       0 :      64 :     yes :          8

;[47:1] # function arguments are equivalent to mutable references
;[54:1] # default argument type is `i64`
;[61:1] # return target is specified as a variable, in this case `res`
;[71:1] # array arguments are declared with type and []
main:
;   [78:5] var arr : i32[4]
;   [78:9] arr: i32[4] @ dword [rsp - 16]
;   [78:9] clear array 4 * 4 B = 16 B
;   [78:5] allocate named register 'rdi'
;   [78:5] allocate named register 'rcx'
;   [78:5] allocate named register 'rax'
    lea rdi, [rsp - 16]
    mov rcx, 16
    xor rax, rax
    rep stosb
;   [78:5] free named register 'rax'
;   [78:5] free named register 'rcx'
;   [78:5] free named register 'rdi'
;   [79:5] # arrays are initialized to 0
;   [81:5] var ix = 1
;   [81:9] ix: i64 @ qword [rsp - 24]
;   [81:9] ix =1
;   [81:14] 1
;   [81:14] 1
;   [81:14] 1
    mov qword [rsp - 24], 1
;   [83:5] arr[ix] = 2
;   [83:5] allocate scratch register -> r15
;   [83:9] set array index
;   [83:9] ix
;   [83:9] ix
;   [83:9] ix
    mov r15, qword [rsp - 24]
;   [83:9] bounds check
;   [83:9] allocate scratch register -> r14
;   [83:9] line number
    mov r14, 83
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [83:9] free scratch register 'r14'
;   [83:15] 2
;   [83:15] 2
;   [83:15] 2
    mov dword [rsp + r15 * 4 - 16], 2
;   [83:5] free scratch register 'r15'
;   [84:5] arr[ix + 1] = arr[ix]
;   [84:5] allocate scratch register -> r15
;   [84:9] set array index
;   [84:9] ix + 1
;   [84:9] ix + 1
;   [84:9] ix
    mov r15, qword [rsp - 24]
;   [84:14] r15 + 1
    add r15, 1
;   [84:9] bounds check
;   [84:9] allocate scratch register -> r14
;   [84:9] line number
    mov r14, 84
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [84:9] free scratch register 'r14'
;   [84:19] arr[ix]
;   [84:19] arr[ix]
;   [84:19] arr[ix]
;   [84:19] allocate scratch register -> r14
;   [84:23] set array index
;   [84:23] ix
;   [84:23] ix
;   [84:23] ix
    mov r14, qword [rsp - 24]
;   [84:23] bounds check
;   [84:23] allocate scratch register -> r13
;   [84:23] line number
    mov r13, 84
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [84:23] free scratch register 'r13'
;   [84:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
;   [84:19] free scratch register 'r13'
;   [84:19] free scratch register 'r14'
;   [84:5] free scratch register 'r15'
;   [85:5] assert(arr[1] == 2)
;   [85:12] allocate scratch register -> r15
;   [85:12] arr[1] == 2
;   [85:12] ? arr[1] == 2
;   [85:12] ? arr[1] == 2
    cmp_85_12:
;   [85:12] allocate scratch register -> r14
;       [85:12] arr[1]
;       [85:12] arr[1]
;       [85:12] allocate scratch register -> r13
;       [85:16] set array index
;       [85:16] 1
;       [85:16] 1
;       [85:16] 1
        mov r13, 1
;       [85:16] bounds check
;       [85:16] allocate scratch register -> r12
;       [85:16] line number
        mov r12, 85
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
;       [85:16] free scratch register 'r12'
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [85:12] free scratch register 'r13'
    cmp r14, 2
;   [85:12] free scratch register 'r14'
    jne bool_false_85_12
    jmp bool_true_85_12
    bool_true_85_12:
    mov r15, true
    jmp bool_end_85_12
    bool_false_85_12:
    mov r15, false
    bool_end_85_12:
;   [18:6] assert(expr : bool) 
    assert_85_5:
;       [85:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_85_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_85_5:
        test r15, r15
        jne if_18_26_85_5_end
        jmp if_18_29_85_5_code
        if_18_29_85_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_85_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_85_5_end:
        if_18_26_85_5_end:
;       [85:5] free scratch register 'r15'
    assert_85_5_end:
;   [86:5] assert(arr[2] == 2)
;   [86:12] allocate scratch register -> r15
;   [86:12] arr[2] == 2
;   [86:12] ? arr[2] == 2
;   [86:12] ? arr[2] == 2
    cmp_86_12:
;   [86:12] allocate scratch register -> r14
;       [86:12] arr[2]
;       [86:12] arr[2]
;       [86:12] allocate scratch register -> r13
;       [86:16] set array index
;       [86:16] 2
;       [86:16] 2
;       [86:16] 2
        mov r13, 2
;       [86:16] bounds check
;       [86:16] allocate scratch register -> r12
;       [86:16] line number
        mov r12, 86
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
;       [86:16] free scratch register 'r12'
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [86:12] free scratch register 'r13'
    cmp r14, 2
;   [86:12] free scratch register 'r14'
    jne bool_false_86_12
    jmp bool_true_86_12
    bool_true_86_12:
    mov r15, true
    jmp bool_end_86_12
    bool_false_86_12:
    mov r15, false
    bool_end_86_12:
;   [18:6] assert(expr : bool) 
    assert_86_5:
;       [86:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_86_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_86_5:
        test r15, r15
        jne if_18_26_86_5_end
        jmp if_18_29_86_5_code
        if_18_29_86_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_86_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_86_5_end:
        if_18_26_86_5_end:
;       [86:5] free scratch register 'r15'
    assert_86_5_end:
;   [88:5] array_copy(arr[2], arr, 2)
;   [88:5] allocate named register 'rsi'
;   [88:5] allocate named register 'rdi'
;   [88:5] allocate named register 'rcx'
;   [88:29] 2
;   [88:29] 2
;   [88:29] 2
;   [88:29] 2
    mov rcx, 2
;   [88:16] arr[2]
;   [88:16] allocate scratch register -> r15
;   [88:20] set array index
;   [88:20] 2
;   [88:20] 2
;   [88:20] 2
    mov r15, 2
;   [88:20] bounds check
;   [88:20] allocate scratch register -> r14
;   [88:20] line number
    mov r14, 88
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [88:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [88:20] free scratch register 'r13'
    cmovg rbp, r14
    jg panic_bounds
;   [88:20] free scratch register 'r14'
    lea rsi, [rsp + r15 * 4 - 16]
;   [88:5] free scratch register 'r15'
;   [88:24] arr
;   [88:24] bounds check
;   [88:24] allocate scratch register -> r15
;   [88:24] line number
    mov r15, 88
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [88:24] free scratch register 'r15'
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
;   [88:5] free named register 'rcx'
;   [88:5] free named register 'rdi'
;   [88:5] free named register 'rsi'
;   [89:5] # copy from, to, number of elements
;   [90:5] assert(arr[0] == 2)
;   [90:12] allocate scratch register -> r15
;   [90:12] arr[0] == 2
;   [90:12] ? arr[0] == 2
;   [90:12] ? arr[0] == 2
    cmp_90_12:
;   [90:12] allocate scratch register -> r14
;       [90:12] arr[0]
;       [90:12] arr[0]
;       [90:12] allocate scratch register -> r13
;       [90:16] set array index
;       [90:16] 0
;       [90:16] 0
;       [90:16] 0
        mov r13, 0
;       [90:16] bounds check
;       [90:16] allocate scratch register -> r12
;       [90:16] line number
        mov r12, 90
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
;       [90:16] free scratch register 'r12'
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [90:12] free scratch register 'r13'
    cmp r14, 2
;   [90:12] free scratch register 'r14'
    jne bool_false_90_12
    jmp bool_true_90_12
    bool_true_90_12:
    mov r15, true
    jmp bool_end_90_12
    bool_false_90_12:
    mov r15, false
    bool_end_90_12:
;   [18:6] assert(expr : bool) 
    assert_90_5:
;       [90:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_90_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_90_5:
        test r15, r15
        jne if_18_26_90_5_end
        jmp if_18_29_90_5_code
        if_18_29_90_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_90_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_90_5_end:
        if_18_26_90_5_end:
;       [90:5] free scratch register 'r15'
    assert_90_5_end:
;   [92:5] var arr1 : i32[8]
;   [92:9] arr1: i32[8] @ dword [rsp - 56]
;   [92:9] clear array 8 * 4 B = 32 B
;   [92:5] allocate named register 'rdi'
;   [92:5] allocate named register 'rcx'
;   [92:5] allocate named register 'rax'
    lea rdi, [rsp - 56]
    mov rcx, 32
    xor rax, rax
    rep stosb
;   [92:5] free named register 'rax'
;   [92:5] free named register 'rcx'
;   [92:5] free named register 'rdi'
;   [93:5] array_copy(arr, arr1, 4)
;   [93:5] allocate named register 'rsi'
;   [93:5] allocate named register 'rdi'
;   [93:5] allocate named register 'rcx'
;   [93:27] 4
;   [93:27] 4
;   [93:27] 4
;   [93:27] 4
    mov rcx, 4
;   [93:16] arr
;   [93:16] bounds check
;   [93:16] allocate scratch register -> r15
;   [93:16] line number
    mov r15, 93
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [93:16] free scratch register 'r15'
    lea rsi, [rsp - 16]
;   [93:21] arr1
;   [93:21] bounds check
;   [93:21] allocate scratch register -> r15
;   [93:21] line number
    mov r15, 93
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [93:21] free scratch register 'r15'
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
;   [93:5] free named register 'rcx'
;   [93:5] free named register 'rdi'
;   [93:5] free named register 'rsi'
;   [94:5] assert(arrays_equal(arr, arr1, 4))
;   [94:12] allocate scratch register -> r15
;   [94:12] arrays_equal(arr, arr1, 4)
;   [94:12] ? arrays_equal(arr, arr1, 4)
;   [94:12] ? arrays_equal(arr, arr1, 4)
    cmp_94_12:
;   [94:12] allocate scratch register -> r14
;       [94:12] arrays_equal(arr, arr1, 4)
;       [94:12] r14 = arrays_equal(arr, arr1, 4)
;       [94:12] arrays_equal(arr, arr1, 4)
;       [94:12] allocate named register 'rsi'
;       [94:12] allocate named register 'rdi'
;       [94:12] allocate named register 'rcx'
;       [94:36] 4
;       [94:36] 4
;       [94:36] 4
;       [94:36] 4
        mov rcx, 4
;       [94:25] arr
;       [94:25] bounds check
;       [94:25] allocate scratch register -> r13
;       [94:25] line number
        mov r13, 94
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [94:25] free scratch register 'r13'
        lea rsi, [rsp - 16]
;       [94:30] arr1
;       [94:30] bounds check
;       [94:30] allocate scratch register -> r13
;       [94:30] line number
        mov r13, 94
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [94:30] free scratch register 'r13'
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [94:12] free named register 'rcx'
;       [94:12] free named register 'rdi'
;       [94:12] free named register 'rsi'
        je cmps_eq_94_12
        mov r14, false
        jmp cmps_end_94_12
        cmps_eq_94_12:
        mov r14, true
        cmps_end_94_12:
    test r14, r14
;   [94:12] free scratch register 'r14'
    je bool_false_94_12
    jmp bool_true_94_12
    bool_true_94_12:
    mov r15, true
    jmp bool_end_94_12
    bool_false_94_12:
    mov r15, false
    bool_end_94_12:
;   [18:6] assert(expr : bool) 
    assert_94_5:
;       [94:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_94_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_94_5:
        test r15, r15
        jne if_18_26_94_5_end
        jmp if_18_29_94_5_code
        if_18_29_94_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_94_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_94_5_end:
        if_18_26_94_5_end:
;       [94:5] free scratch register 'r15'
    assert_94_5_end:
;   [96:5] arr1[2] = -1
;   [96:5] allocate scratch register -> r15
;   [96:10] set array index
;   [96:10] 2
;   [96:10] 2
;   [96:10] 2
    mov r15, 2
;   [96:10] bounds check
;   [96:10] allocate scratch register -> r14
;   [96:10] line number
    mov r14, 96
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [96:10] free scratch register 'r14'
;   [96:15] -1
;   [96:15] -1
;   [96:16] -1
    mov dword [rsp + r15 * 4 - 56], -1
;   [96:5] free scratch register 'r15'
;   [97:5] assert(not arrays_equal(arr, arr1, 4))
;   [97:12] allocate scratch register -> r15
;   [97:12] not arrays_equal(arr, arr1, 4)
;   [97:12] ? not arrays_equal(arr, arr1, 4)
;   [97:12] ? not arrays_equal(arr, arr1, 4)
    cmp_97_12:
;   [97:16] allocate scratch register -> r14
;       [97:16] arrays_equal(arr, arr1, 4)
;       [97:16] r14 = arrays_equal(arr, arr1, 4)
;       [97:16] arrays_equal(arr, arr1, 4)
;       [97:16] allocate named register 'rsi'
;       [97:16] allocate named register 'rdi'
;       [97:16] allocate named register 'rcx'
;       [97:40] 4
;       [97:40] 4
;       [97:40] 4
;       [97:40] 4
        mov rcx, 4
;       [97:29] arr
;       [97:29] bounds check
;       [97:29] allocate scratch register -> r13
;       [97:29] line number
        mov r13, 97
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [97:29] free scratch register 'r13'
        lea rsi, [rsp - 16]
;       [97:34] arr1
;       [97:34] bounds check
;       [97:34] allocate scratch register -> r13
;       [97:34] line number
        mov r13, 97
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [97:34] free scratch register 'r13'
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [97:16] free named register 'rcx'
;       [97:16] free named register 'rdi'
;       [97:16] free named register 'rsi'
        je cmps_eq_97_16
        mov r14, false
        jmp cmps_end_97_16
        cmps_eq_97_16:
        mov r14, true
        cmps_end_97_16:
    test r14, r14
;   [97:12] free scratch register 'r14'
    jne bool_false_97_12
    jmp bool_true_97_12
    bool_true_97_12:
    mov r15, true
    jmp bool_end_97_12
    bool_false_97_12:
    mov r15, false
    bool_end_97_12:
;   [18:6] assert(expr : bool) 
    assert_97_5:
;       [97:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_97_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_97_5:
        test r15, r15
        jne if_18_26_97_5_end
        jmp if_18_29_97_5_code
        if_18_29_97_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_97_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_97_5_end:
        if_18_26_97_5_end:
;       [97:5] free scratch register 'r15'
    assert_97_5_end:
;   [99:5] ix = 3
;   [99:10] 3
;   [99:10] 3
;   [99:10] 3
    mov qword [rsp - 24], 3
;   [100:5] arr[ix] = ~inv(arr[ix - 1])
;   [100:5] allocate scratch register -> r15
;   [100:9] set array index
;   [100:9] ix
;   [100:9] ix
;   [100:9] ix
    mov r15, qword [rsp - 24]
;   [100:9] bounds check
;   [100:9] allocate scratch register -> r14
;   [100:9] line number
    mov r14, 100
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [100:9] free scratch register 'r14'
;   [100:15] ~inv(arr[ix - 1])
;   [100:15] ~inv(arr[ix - 1])
;   [100:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;   [100:16] ~inv(arr[ix - 1])
;   [100:20] allocate scratch register -> r14
;   [100:24] set array index
;   [100:24] ix - 1
;   [100:24] ix - 1
;   [100:24] ix
    mov r14, qword [rsp - 24]
;   [100:29] r14 - 1
    sub r14, 1
;   [100:24] bounds check
;   [100:24] allocate scratch register -> r13
;   [100:24] line number
    mov r13, 100
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [100:24] free scratch register 'r13'
;   [63:6] inv(i : i32) : i32 res 
    inv_100_16:
;       [100:16] alias res -> dword [rsp + r15 * 4 - 16]  (lea: , len: 0)
;       [100:16] alias i -> arr  (lea: rsp + r14 * 4 - 16, len: 4)
;       [64:5] res = ~i
;       [64:11] ~i
;       [64:11] ~i
;       [64:12] ~i
;       [64:12] allocate scratch register -> r13
        lea r13, [rsp + r14 * 4 - 16]
;       [64:12] allocate scratch register -> r12
        mov r12d, dword [r13]
        mov dword [rsp + r15 * 4 - 16], r12d
;       [64:12] free scratch register 'r12'
        not dword [rsp + r15 * 4 - 16]
;       [64:12] free scratch register 'r13'
;       [100:16] free scratch register 'r14'
    inv_100_16_end:
    not dword [rsp + r15 * 4 - 16]
;   [100:5] free scratch register 'r15'
;   [101:5] assert(arr[ix] == 2)
;   [101:12] allocate scratch register -> r15
;   [101:12] arr[ix] == 2
;   [101:12] ? arr[ix] == 2
;   [101:12] ? arr[ix] == 2
    cmp_101_12:
;   [101:12] allocate scratch register -> r14
;       [101:12] arr[ix]
;       [101:12] arr[ix]
;       [101:12] allocate scratch register -> r13
;       [101:16] set array index
;       [101:16] ix
;       [101:16] ix
;       [101:16] ix
        mov r13, qword [rsp - 24]
;       [101:16] bounds check
;       [101:16] allocate scratch register -> r12
;       [101:16] line number
        mov r12, 101
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
;       [101:16] free scratch register 'r12'
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [101:12] free scratch register 'r13'
    cmp r14, 2
;   [101:12] free scratch register 'r14'
    jne bool_false_101_12
    jmp bool_true_101_12
    bool_true_101_12:
    mov r15, true
    jmp bool_end_101_12
    bool_false_101_12:
    mov r15, false
    bool_end_101_12:
;   [18:6] assert(expr : bool) 
    assert_101_5:
;       [101:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_101_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_101_5:
        test r15, r15
        jne if_18_26_101_5_end
        jmp if_18_29_101_5_code
        if_18_29_101_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_101_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_101_5_end:
        if_18_26_101_5_end:
;       [101:5] free scratch register 'r15'
    assert_101_5_end:
;   [103:5] faz(arr)
;   [73:6] faz(arg : i32[]) 
    faz_103_5:
;       [103:5] alias arg -> arr  (lea: , len: 0)
;       [74:5] arg[1] = 0xfe
;       [74:5] allocate scratch register -> r15
;       [74:9] set array index
;       [74:9] 1
;       [74:9] 1
;       [74:9] 1
        mov r15, 1
;       [74:9] bounds check
;       [74:9] allocate scratch register -> r14
;       [74:9] line number
        mov r14, 74
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
;       [74:9] free scratch register 'r14'
;       [74:14] 0xfe
;       [74:14] 0xfe
;       [74:14] 0xfe
        mov dword [rsp + r15 * 4 - 16], 0xfe
;       [74:5] free scratch register 'r15'
    faz_103_5_end:
;   [104:5] assert(arr[1] == 0xfe)
;   [104:12] allocate scratch register -> r15
;   [104:12] arr[1] == 0xfe
;   [104:12] ? arr[1] == 0xfe
;   [104:12] ? arr[1] == 0xfe
    cmp_104_12:
;   [104:12] allocate scratch register -> r14
;       [104:12] arr[1]
;       [104:12] arr[1]
;       [104:12] allocate scratch register -> r13
;       [104:16] set array index
;       [104:16] 1
;       [104:16] 1
;       [104:16] 1
        mov r13, 1
;       [104:16] bounds check
;       [104:16] allocate scratch register -> r12
;       [104:16] line number
        mov r12, 104
        test r13, r13
        cmovs rbp, r12
        js panic_bounds
        cmp r13, 4
        cmovge rbp, r12
        jge panic_bounds
;       [104:16] free scratch register 'r12'
        movsx r14, dword [rsp + r13 * 4 - 16]
;       [104:12] free scratch register 'r13'
    cmp r14, 0xfe
;   [104:12] free scratch register 'r14'
    jne bool_false_104_12
    jmp bool_true_104_12
    bool_true_104_12:
    mov r15, true
    jmp bool_end_104_12
    bool_false_104_12:
    mov r15, false
    bool_end_104_12:
;   [18:6] assert(expr : bool) 
    assert_104_5:
;       [104:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_104_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_104_5:
        test r15, r15
        jne if_18_26_104_5_end
        jmp if_18_29_104_5_code
        if_18_29_104_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_104_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_104_5_end:
        if_18_26_104_5_end:
;       [104:5] free scratch register 'r15'
    assert_104_5_end:
;   [106:5] var p : point = {0, 0}
;   [106:9] p: point @ qword [rsp - 72]
;   [106:9] p ={0, 0}
;   [106:21] {0, 0}
;   [106:21] {0, 0}
;   [106:22] 0
;   [106:22] 0
;   [106:22] 0
    mov qword [rsp - 72], 0
;   [106:25] 0
;   [106:25] 0
;   [106:25] 0
    mov qword [rsp - 64], 0
;   [107:5] foo(p)
;   [49:6] foo(pt : point) 
    foo_107_5:
;       [107:5] alias pt -> p  (lea: , len: 0)
;       [50:5] pt.x = 0b10
;       [50:12] 0b10
;       [50:12] 0b10
;       [50:12] 0b10
        mov qword [rsp - 72], 0b10
;       [50:20] # binary value 2
;       [51:5] pt.y = 0xb
;       [51:12] 0xb
;       [51:12] 0xb
;       [51:12] 0xb
        mov qword [rsp - 64], 0xb
;       [51:20] # hex value 11
    foo_107_5_end:
;   [108:5] assert(p.x == 2)
;   [108:12] allocate scratch register -> r15
;   [108:12] p.x == 2
;   [108:12] ? p.x == 2
;   [108:12] ? p.x == 2
    cmp_108_12:
    cmp qword [rsp - 72], 2
    jne bool_false_108_12
    jmp bool_true_108_12
    bool_true_108_12:
    mov r15, true
    jmp bool_end_108_12
    bool_false_108_12:
    mov r15, false
    bool_end_108_12:
;   [18:6] assert(expr : bool) 
    assert_108_5:
;       [108:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_108_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_108_5:
        test r15, r15
        jne if_18_26_108_5_end
        jmp if_18_29_108_5_code
        if_18_29_108_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_108_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_108_5_end:
        if_18_26_108_5_end:
;       [108:5] free scratch register 'r15'
    assert_108_5_end:
;   [109:5] assert(p.y == 0xb)
;   [109:12] allocate scratch register -> r15
;   [109:12] p.y == 0xb
;   [109:12] ? p.y == 0xb
;   [109:12] ? p.y == 0xb
    cmp_109_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_109_12
    jmp bool_true_109_12
    bool_true_109_12:
    mov r15, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15, false
    bool_end_109_12:
;   [18:6] assert(expr : bool) 
    assert_109_5:
;       [109:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_109_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_109_5:
        test r15, r15
        jne if_18_26_109_5_end
        jmp if_18_29_109_5_code
        if_18_29_109_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_109_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_109_5_end:
        if_18_26_109_5_end:
;       [109:5] free scratch register 'r15'
    assert_109_5_end:
;   [111:5] var i = 0
;   [111:9] i: i64 @ qword [rsp - 80]
;   [111:9] i =0
;   [111:13] 0
;   [111:13] 0
;   [111:13] 0
    mov qword [rsp - 80], 0
;   [112:5] bar(i)
;   [56:6] bar(arg) 
    bar_112_5:
;       [112:5] alias arg -> i  (lea: , len: 0)
        if_57_8_112_5:
;       [57:8] ? arg == 0
;       [57:8] ? arg == 0
        cmp_57_8_112_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_112_5_end
        jmp if_57_8_112_5_code
        if_57_8_112_5_code:
;           [57:17] return
            jmp bar_112_5_end
        if_57_5_112_5_end:
;       [58:5] arg = 0xff
;       [58:11] 0xff
;       [58:11] 0xff
;       [58:11] 0xff
        mov qword [rsp - 80], 0xff
    bar_112_5_end:
;   [113:5] assert(i == 0)
;   [113:12] allocate scratch register -> r15
;   [113:12] i == 0
;   [113:12] ? i == 0
;   [113:12] ? i == 0
    cmp_113_12:
    cmp qword [rsp - 80], 0
    jne bool_false_113_12
    jmp bool_true_113_12
    bool_true_113_12:
    mov r15, true
    jmp bool_end_113_12
    bool_false_113_12:
    mov r15, false
    bool_end_113_12:
;   [18:6] assert(expr : bool) 
    assert_113_5:
;       [113:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_113_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_113_5:
        test r15, r15
        jne if_18_26_113_5_end
        jmp if_18_29_113_5_code
        if_18_29_113_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_113_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_113_5_end:
        if_18_26_113_5_end:
;       [113:5] free scratch register 'r15'
    assert_113_5_end:
;   [115:5] i = 1
;   [115:9] 1
;   [115:9] 1
;   [115:9] 1
    mov qword [rsp - 80], 1
;   [116:5] bar(i)
;   [56:6] bar(arg) 
    bar_116_5:
;       [116:5] alias arg -> i  (lea: , len: 0)
        if_57_8_116_5:
;       [57:8] ? arg == 0
;       [57:8] ? arg == 0
        cmp_57_8_116_5:
        cmp qword [rsp - 80], 0
        jne if_57_5_116_5_end
        jmp if_57_8_116_5_code
        if_57_8_116_5_code:
;           [57:17] return
            jmp bar_116_5_end
        if_57_5_116_5_end:
;       [58:5] arg = 0xff
;       [58:11] 0xff
;       [58:11] 0xff
;       [58:11] 0xff
        mov qword [rsp - 80], 0xff
    bar_116_5_end:
;   [117:5] assert(i == 0xff)
;   [117:12] allocate scratch register -> r15
;   [117:12] i == 0xff
;   [117:12] ? i == 0xff
;   [117:12] ? i == 0xff
    cmp_117_12:
    cmp qword [rsp - 80], 0xff
    jne bool_false_117_12
    jmp bool_true_117_12
    bool_true_117_12:
    mov r15, true
    jmp bool_end_117_12
    bool_false_117_12:
    mov r15, false
    bool_end_117_12:
;   [18:6] assert(expr : bool) 
    assert_117_5:
;       [117:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_117_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_117_5:
        test r15, r15
        jne if_18_26_117_5_end
        jmp if_18_29_117_5_code
        if_18_29_117_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_117_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_117_5_end:
        if_18_26_117_5_end:
;       [117:5] free scratch register 'r15'
    assert_117_5_end:
;   [119:5] var j = 1
;   [119:9] j: i64 @ qword [rsp - 88]
;   [119:9] j =1
;   [119:13] 1
;   [119:13] 1
;   [119:13] 1
    mov qword [rsp - 88], 1
;   [120:5] var k = baz(j)
;   [120:9] k: i64 @ qword [rsp - 96]
;   [120:9] k =baz(j)
;   [120:13] baz(j)
;   [120:13] allocate scratch register -> r15
;   [120:13] baz(j)
;   [120:13] r15 = baz(j)
;   [120:13] baz(j)
;   [67:6] baz(arg) : i64 res 
    baz_120_13:
;       [120:13] alias res -> r15  (lea: , len: 0)
;       [120:13] alias arg -> j  (lea: , len: 0)
;       [68:5] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:11] arg
        mov r15, qword [rsp - 88]
;       [68:17] res * 2
        imul r15, 2
    baz_120_13_end:
    mov qword [rsp - 96], r15
;   [120:13] free scratch register 'r15'
;   [121:5] assert(k == 2)
;   [121:12] allocate scratch register -> r15
;   [121:12] k == 2
;   [121:12] ? k == 2
;   [121:12] ? k == 2
    cmp_121_12:
    cmp qword [rsp - 96], 2
    jne bool_false_121_12
    jmp bool_true_121_12
    bool_true_121_12:
    mov r15, true
    jmp bool_end_121_12
    bool_false_121_12:
    mov r15, false
    bool_end_121_12:
;   [18:6] assert(expr : bool) 
    assert_121_5:
;       [121:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_121_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_121_5:
        test r15, r15
        jne if_18_26_121_5_end
        jmp if_18_29_121_5_code
        if_18_29_121_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_121_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_121_5_end:
        if_18_26_121_5_end:
;       [121:5] free scratch register 'r15'
    assert_121_5_end:
;   [123:5] k = baz(1)
;   [123:9] baz(1)
;   [123:9] allocate scratch register -> r15
;   [123:9] baz(1)
;   [123:9] r15 = baz(1)
;   [123:9] baz(1)
;   [67:6] baz(arg) : i64 res 
    baz_123_9:
;       [123:9] alias res -> r15  (lea: , len: 0)
;       [123:9] alias arg -> 1  (lea: , len: 0)
;       [68:5] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:11] arg
        mov r15, 1
;       [68:17] res * 2
        imul r15, 2
    baz_123_9_end:
    mov qword [rsp - 96], r15
;   [123:9] free scratch register 'r15'
;   [124:5] assert(k == 2)
;   [124:12] allocate scratch register -> r15
;   [124:12] k == 2
;   [124:12] ? k == 2
;   [124:12] ? k == 2
    cmp_124_12:
    cmp qword [rsp - 96], 2
    jne bool_false_124_12
    jmp bool_true_124_12
    bool_true_124_12:
    mov r15, true
    jmp bool_end_124_12
    bool_false_124_12:
    mov r15, false
    bool_end_124_12:
;   [18:6] assert(expr : bool) 
    assert_124_5:
;       [124:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_124_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_124_5:
        test r15, r15
        jne if_18_26_124_5_end
        jmp if_18_29_124_5_code
        if_18_29_124_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_124_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_124_5_end:
        if_18_26_124_5_end:
;       [124:5] free scratch register 'r15'
    assert_124_5_end:
;   [126:5] var p0 : point = {baz(2), 0}
;   [126:9] p0: point @ qword [rsp - 112]
;   [126:9] p0 ={baz(2), 0}
;   [126:22] {baz(2), 0}
;   [126:22] {baz(2), 0}
;   [126:23] baz(2)
;   [126:23] allocate scratch register -> r15
;   [126:23] baz(2)
;   [126:23] r15 = baz(2)
;   [126:23] baz(2)
;   [67:6] baz(arg) : i64 res 
    baz_126_23:
;       [126:23] alias res -> r15  (lea: , len: 0)
;       [126:23] alias arg -> 2  (lea: , len: 0)
;       [68:5] res = arg * 2
;       [68:11] arg * 2
;       [68:11] arg * 2
;       [68:11] arg
        mov r15, 2
;       [68:17] res * 2
        imul r15, 2
    baz_126_23_end:
    mov qword [rsp - 112], r15
;   [126:23] free scratch register 'r15'
;   [126:31] 0
;   [126:31] 0
;   [126:31] 0
    mov qword [rsp - 104], 0
;   [127:5] assert(p0.x == 4)
;   [127:12] allocate scratch register -> r15
;   [127:12] p0.x == 4
;   [127:12] ? p0.x == 4
;   [127:12] ? p0.x == 4
    cmp_127_12:
    cmp qword [rsp - 112], 4
    jne bool_false_127_12
    jmp bool_true_127_12
    bool_true_127_12:
    mov r15, true
    jmp bool_end_127_12
    bool_false_127_12:
    mov r15, false
    bool_end_127_12:
;   [18:6] assert(expr : bool) 
    assert_127_5:
;       [127:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_127_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_127_5:
        test r15, r15
        jne if_18_26_127_5_end
        jmp if_18_29_127_5_code
        if_18_29_127_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_127_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_127_5_end:
        if_18_26_127_5_end:
;       [127:5] free scratch register 'r15'
    assert_127_5_end:
;   [129:5] var x = 1
;   [129:9] x: i64 @ qword [rsp - 120]
;   [129:9] x =1
;   [129:13] 1
;   [129:13] 1
;   [129:13] 1
    mov qword [rsp - 120], 1
;   [130:5] var y = 2
;   [130:9] y: i64 @ qword [rsp - 128]
;   [130:9] y =2
;   [130:13] 2
;   [130:13] 2
;   [130:13] 2
    mov qword [rsp - 128], 2
;   [132:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [132:9] o1: object @ qword [rsp - 148]
;   [132:9] o1 ={{x * 10, y}, 0xff0000}
;   [132:23] {{x * 10, y}, 0xff0000}
;   [132:23] {{x * 10, y}, 0xff0000}
;       [132:24] {x * 10, y}
;       [132:25] x * 10
;       [132:25] allocate scratch register -> r15
;       [132:25] x * 10
;       [132:25] x
        mov r15, qword [rsp - 120]
;       [132:29] r15 * 10
        imul r15, 10
        mov qword [rsp - 148], r15
;       [132:25] free scratch register 'r15'
;       [132:33] y
;       [132:33] y
;       [132:33] y
;       [132:33] allocate scratch register -> r15
        mov r15, qword [rsp - 128]
        mov qword [rsp - 140], r15
;       [132:33] free scratch register 'r15'
;   [132:37] 0xff0000
;   [132:37] 0xff0000
;   [132:37] 0xff0000
    mov dword [rsp - 132], 0xff0000
;   [133:5] assert(o1.pos.x == 10)
;   [133:12] allocate scratch register -> r15
;   [133:12] o1.pos.x == 10
;   [133:12] ? o1.pos.x == 10
;   [133:12] ? o1.pos.x == 10
    cmp_133_12:
    cmp qword [rsp - 148], 10
    jne bool_false_133_12
    jmp bool_true_133_12
    bool_true_133_12:
    mov r15, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15, false
    bool_end_133_12:
;   [18:6] assert(expr : bool) 
    assert_133_5:
;       [133:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_133_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_133_5:
        test r15, r15
        jne if_18_26_133_5_end
        jmp if_18_29_133_5_code
        if_18_29_133_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_133_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_133_5_end:
        if_18_26_133_5_end:
;       [133:5] free scratch register 'r15'
    assert_133_5_end:
;   [134:5] assert(o1.pos.y == 2)
;   [134:12] allocate scratch register -> r15
;   [134:12] o1.pos.y == 2
;   [134:12] ? o1.pos.y == 2
;   [134:12] ? o1.pos.y == 2
    cmp_134_12:
    cmp qword [rsp - 140], 2
    jne bool_false_134_12
    jmp bool_true_134_12
    bool_true_134_12:
    mov r15, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15, false
    bool_end_134_12:
;   [18:6] assert(expr : bool) 
    assert_134_5:
;       [134:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_134_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_134_5:
        test r15, r15
        jne if_18_26_134_5_end
        jmp if_18_29_134_5_code
        if_18_29_134_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_134_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_134_5_end:
        if_18_26_134_5_end:
;       [134:5] free scratch register 'r15'
    assert_134_5_end:
;   [135:5] assert(o1.color == 0xff0000)
;   [135:12] allocate scratch register -> r15
;   [135:12] o1.color == 0xff0000
;   [135:12] ? o1.color == 0xff0000
;   [135:12] ? o1.color == 0xff0000
    cmp_135_12:
    cmp dword [rsp - 132], 0xff0000
    jne bool_false_135_12
    jmp bool_true_135_12
    bool_true_135_12:
    mov r15, true
    jmp bool_end_135_12
    bool_false_135_12:
    mov r15, false
    bool_end_135_12:
;   [18:6] assert(expr : bool) 
    assert_135_5:
;       [135:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_135_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_135_5:
        test r15, r15
        jne if_18_26_135_5_end
        jmp if_18_29_135_5_code
        if_18_29_135_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_135_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_135_5_end:
        if_18_26_135_5_end:
;       [135:5] free scratch register 'r15'
    assert_135_5_end:
;   [137:5] var p1 : point = {-x, -y}
;   [137:9] p1: point @ qword [rsp - 164]
;   [137:9] p1 ={-x, -y}
;   [137:22] {-x, -y}
;   [137:22] {-x, -y}
;   [137:23] -x
;   [137:23] -x
;   [137:24] -x
;   [137:24] allocate scratch register -> r15
    mov r15, qword [rsp - 120]
    mov qword [rsp - 164], r15
;   [137:24] free scratch register 'r15'
    neg qword [rsp - 164]
;   [137:27] -y
;   [137:27] -y
;   [137:28] -y
;   [137:28] allocate scratch register -> r15
    mov r15, qword [rsp - 128]
    mov qword [rsp - 156], r15
;   [137:28] free scratch register 'r15'
    neg qword [rsp - 156]
;   [138:5] o1.pos = p1
;   [138:5] allocate named register 'rsi'
;   [138:5] allocate named register 'rdi'
;   [138:5] allocate named register 'rcx'
    lea rdi, [rsp - 148]
;   [138:14] p1
    lea rsi, [rsp - 164]
    mov rcx, 16
    rep movsb
;   [138:5] free named register 'rcx'
;   [138:5] free named register 'rdi'
;   [138:5] free named register 'rsi'
;   [139:5] assert(o1.pos.x == -1)
;   [139:12] allocate scratch register -> r15
;   [139:12] o1.pos.x == -1
;   [139:12] ? o1.pos.x == -1
;   [139:12] ? o1.pos.x == -1
    cmp_139_12:
    cmp qword [rsp - 148], -1
    jne bool_false_139_12
    jmp bool_true_139_12
    bool_true_139_12:
    mov r15, true
    jmp bool_end_139_12
    bool_false_139_12:
    mov r15, false
    bool_end_139_12:
;   [18:6] assert(expr : bool) 
    assert_139_5:
;       [139:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_139_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_139_5:
        test r15, r15
        jne if_18_26_139_5_end
        jmp if_18_29_139_5_code
        if_18_29_139_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_139_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_139_5_end:
        if_18_26_139_5_end:
;       [139:5] free scratch register 'r15'
    assert_139_5_end:
;   [140:5] assert(o1.pos.y == -2)
;   [140:12] allocate scratch register -> r15
;   [140:12] o1.pos.y == -2
;   [140:12] ? o1.pos.y == -2
;   [140:12] ? o1.pos.y == -2
    cmp_140_12:
    cmp qword [rsp - 140], -2
    jne bool_false_140_12
    jmp bool_true_140_12
    bool_true_140_12:
    mov r15, true
    jmp bool_end_140_12
    bool_false_140_12:
    mov r15, false
    bool_end_140_12:
;   [18:6] assert(expr : bool) 
    assert_140_5:
;       [140:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_140_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_140_5:
        test r15, r15
        jne if_18_26_140_5_end
        jmp if_18_29_140_5_code
        if_18_29_140_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_140_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_140_5_end:
        if_18_26_140_5_end:
;       [140:5] free scratch register 'r15'
    assert_140_5_end:
;   [142:5] var o2 : object = o1
;   [142:9] o2: object @ qword [rsp - 184]
;   [142:9] o2 =o1
;   [142:9] allocate named register 'rsi'
;   [142:9] allocate named register 'rdi'
;   [142:9] allocate named register 'rcx'
    lea rdi, [rsp - 184]
;   [142:23] o1
    lea rsi, [rsp - 148]
    mov rcx, 20
    rep movsb
;   [142:9] free named register 'rcx'
;   [142:9] free named register 'rdi'
;   [142:9] free named register 'rsi'
;   [143:5] assert(o2.pos.x == -1)
;   [143:12] allocate scratch register -> r15
;   [143:12] o2.pos.x == -1
;   [143:12] ? o2.pos.x == -1
;   [143:12] ? o2.pos.x == -1
    cmp_143_12:
    cmp qword [rsp - 184], -1
    jne bool_false_143_12
    jmp bool_true_143_12
    bool_true_143_12:
    mov r15, true
    jmp bool_end_143_12
    bool_false_143_12:
    mov r15, false
    bool_end_143_12:
;   [18:6] assert(expr : bool) 
    assert_143_5:
;       [143:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_143_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_143_5:
        test r15, r15
        jne if_18_26_143_5_end
        jmp if_18_29_143_5_code
        if_18_29_143_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_143_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_143_5_end:
        if_18_26_143_5_end:
;       [143:5] free scratch register 'r15'
    assert_143_5_end:
;   [144:5] assert(o2.pos.y == -2)
;   [144:12] allocate scratch register -> r15
;   [144:12] o2.pos.y == -2
;   [144:12] ? o2.pos.y == -2
;   [144:12] ? o2.pos.y == -2
    cmp_144_12:
    cmp qword [rsp - 176], -2
    jne bool_false_144_12
    jmp bool_true_144_12
    bool_true_144_12:
    mov r15, true
    jmp bool_end_144_12
    bool_false_144_12:
    mov r15, false
    bool_end_144_12:
;   [18:6] assert(expr : bool) 
    assert_144_5:
;       [144:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_144_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_144_5:
        test r15, r15
        jne if_18_26_144_5_end
        jmp if_18_29_144_5_code
        if_18_29_144_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_144_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_144_5_end:
        if_18_26_144_5_end:
;       [144:5] free scratch register 'r15'
    assert_144_5_end:
;   [145:5] assert(o2.color == 0xff0000)
;   [145:12] allocate scratch register -> r15
;   [145:12] o2.color == 0xff0000
;   [145:12] ? o2.color == 0xff0000
;   [145:12] ? o2.color == 0xff0000
    cmp_145_12:
    cmp dword [rsp - 168], 0xff0000
    jne bool_false_145_12
    jmp bool_true_145_12
    bool_true_145_12:
    mov r15, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15, false
    bool_end_145_12:
;   [18:6] assert(expr : bool) 
    assert_145_5:
;       [145:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_145_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_145_5:
        test r15, r15
        jne if_18_26_145_5_end
        jmp if_18_29_145_5_code
        if_18_29_145_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_145_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_145_5_end:
        if_18_26_145_5_end:
;       [145:5] free scratch register 'r15'
    assert_145_5_end:
;   [147:5] var o3 : object[1]
;   [147:9] o3: object[1] @ qword [rsp - 204]
;   [147:9] clear array 1 * 20 B = 20 B
;   [147:5] allocate named register 'rdi'
;   [147:5] allocate named register 'rcx'
;   [147:5] allocate named register 'rax'
    lea rdi, [rsp - 204]
    mov rcx, 20
    xor rax, rax
    rep stosb
;   [147:5] free named register 'rax'
;   [147:5] free named register 'rcx'
;   [147:5] free named register 'rdi'
;   [148:5] # index 0 in an array can be accessed without array index
;   [149:5] o3.pos.y = 73
;   [149:16] 73
;   [149:16] 73
;   [149:16] 73
    mov qword [rsp - 196], 73
;   [150:5] assert(o3[0].pos.y == 73)
;   [150:12] allocate scratch register -> r15
;   [150:12] o3[0].pos.y == 73
;   [150:12] ? o3[0].pos.y == 73
;   [150:12] ? o3[0].pos.y == 73
    cmp_150_12:
;   [150:12] allocate scratch register -> r14
;       [150:12] o3[0].pos.y
;       [150:12] o3[0].pos.y
;       [150:12] allocate scratch register -> r13
        lea r13, [rsp - 204]
;       [150:12] allocate scratch register -> r12
;       [150:15] set array index
;       [150:15] 0
;       [150:15] 0
;       [150:15] 0
        mov r12, 0
;       [150:15] bounds check
;       [150:15] allocate scratch register -> r11
;       [150:15] line number
        mov r11, 150
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 1
        cmovge rbp, r11
        jge panic_bounds
;       [150:15] free scratch register 'r11'
        imul r12, 20
        add r13, r12
;       [150:12] free scratch register 'r12'
        mov r14, qword [r13 + 8]
;       [150:12] free scratch register 'r13'
    cmp r14, 73
;   [150:12] free scratch register 'r14'
    jne bool_false_150_12
    jmp bool_true_150_12
    bool_true_150_12:
    mov r15, true
    jmp bool_end_150_12
    bool_false_150_12:
    mov r15, false
    bool_end_150_12:
;   [18:6] assert(expr : bool) 
    assert_150_5:
;       [150:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_150_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_150_5:
        test r15, r15
        jne if_18_26_150_5_end
        jmp if_18_29_150_5_code
        if_18_29_150_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_150_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_150_5_end:
        if_18_26_150_5_end:
;       [150:5] free scratch register 'r15'
    assert_150_5_end:
;   [152:5] var input_buffer : i8[80]
;   [152:9] input_buffer: i8[80] @ byte [rsp - 284]
;   [152:9] clear array 80 * 1 B = 80 B
;   [152:5] allocate named register 'rdi'
;   [152:5] allocate named register 'rcx'
;   [152:5] allocate named register 'rax'
    lea rdi, [rsp - 284]
    mov rcx, 80
    xor rax, rax
    rep stosb
;   [152:5] free named register 'rax'
;   [152:5] free named register 'rcx'
;   [152:5] free named register 'rdi'
;   [154:5] var worlds : world[8]
;   [154:9] worlds: world[8] @ qword [rsp - 796]
;   [154:9] clear array 8 * 64 B = 512 B
;   [154:5] allocate named register 'rdi'
;   [154:5] allocate named register 'rcx'
;   [154:5] allocate named register 'rax'
    lea rdi, [rsp - 796]
    mov rcx, 512
    xor rax, rax
    rep stosb
;   [154:5] free named register 'rax'
;   [154:5] free named register 'rcx'
;   [154:5] free named register 'rdi'
;   [155:5] worlds[1].locations[1] = 0xffee
;   [155:5] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [155:5] allocate scratch register -> r14
;   [155:12] set array index
;   [155:12] 1
;   [155:12] 1
;   [155:12] 1
    mov r14, 1
;   [155:12] bounds check
;   [155:12] allocate scratch register -> r13
;   [155:12] line number
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [155:12] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [155:5] free scratch register 'r14'
;   [155:5] allocate scratch register -> r14
;   [155:25] set array index
;   [155:25] 1
;   [155:25] 1
;   [155:25] 1
    mov r14, 1
;   [155:25] bounds check
;   [155:25] allocate scratch register -> r13
;   [155:25] line number
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [155:25] free scratch register 'r13'
;   [155:30] 0xffee
;   [155:30] 0xffee
;   [155:30] 0xffee
    mov qword [r15 + r14 * 8], 0xffee
;   [155:5] free scratch register 'r14'
;   [155:5] free scratch register 'r15'
;   [156:5] assert(worlds[1].locations[1] == 0xffee)
;   [156:12] allocate scratch register -> r15
;   [156:12] worlds[1].locations[1] == 0xffee
;   [156:12] ? worlds[1].locations[1] == 0xffee
;   [156:12] ? worlds[1].locations[1] == 0xffee
    cmp_156_12:
;   [156:12] allocate scratch register -> r14
;       [156:12] worlds[1].locations[1]
;       [156:12] worlds[1].locations[1]
;       [156:12] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [156:12] allocate scratch register -> r12
;       [156:19] set array index
;       [156:19] 1
;       [156:19] 1
;       [156:19] 1
        mov r12, 1
;       [156:19] bounds check
;       [156:19] allocate scratch register -> r11
;       [156:19] line number
        mov r11, 156
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [156:19] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [156:12] free scratch register 'r12'
;       [156:12] allocate scratch register -> r12
;       [156:32] set array index
;       [156:32] 1
;       [156:32] 1
;       [156:32] 1
        mov r12, 1
;       [156:32] bounds check
;       [156:32] allocate scratch register -> r11
;       [156:32] line number
        mov r11, 156
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [156:32] free scratch register 'r11'
        mov r14, qword [r13 + r12 * 8]
;       [156:12] free scratch register 'r12'
;       [156:12] free scratch register 'r13'
    cmp r14, 0xffee
;   [156:12] free scratch register 'r14'
    jne bool_false_156_12
    jmp bool_true_156_12
    bool_true_156_12:
    mov r15, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15, false
    bool_end_156_12:
;   [18:6] assert(expr : bool) 
    assert_156_5:
;       [156:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_156_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_156_5:
        test r15, r15
        jne if_18_26_156_5_end
        jmp if_18_29_156_5_code
        if_18_29_156_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_156_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_156_5_end:
        if_18_26_156_5_end:
;       [156:5] free scratch register 'r15'
    assert_156_5_end:
;   [158:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [158:5] allocate named register 'rsi'
;   [158:5] allocate named register 'rdi'
;   [158:5] allocate named register 'rcx'
;   [160:9] array_size_of(worlds.locations)
;   [160:9] array_size_of(worlds.locations)
;   [161:9] array_size_of(worlds.locations)
;   [161:9] rcx = array_size_of(worlds.locations)
;   [161:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [159:9] worlds[1].locations
;   [159:9] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [159:9] allocate scratch register -> r14
;   [159:16] set array index
;   [159:16] 1
;   [159:16] 1
;   [159:16] 1
    mov r14, 1
;   [159:16] bounds check
;   [159:16] allocate scratch register -> r13
;   [159:16] line number
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [159:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [159:9] free scratch register 'r14'
;   [159:9] bounds check
;   [159:9] allocate scratch register -> r14
;   [159:9] line number
    mov r14, 159
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [159:9] free scratch register 'r14'
    lea rsi, [r15]
;   [158:5] free scratch register 'r15'
;   [160:9] worlds[0].locations
;   [160:9] allocate scratch register -> r15
    lea r15, [rsp - 796]
;   [160:9] allocate scratch register -> r14
;   [160:16] set array index
;   [160:16] 0
;   [160:16] 0
;   [160:16] 0
    mov r14, 0
;   [160:16] bounds check
;   [160:16] allocate scratch register -> r13
;   [160:16] line number
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [160:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [160:9] free scratch register 'r14'
;   [160:9] bounds check
;   [160:9] allocate scratch register -> r14
;   [160:9] line number
    mov r14, 160
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [160:9] free scratch register 'r14'
    lea rdi, [r15]
;   [158:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [158:5] free named register 'rcx'
;   [158:5] free named register 'rdi'
;   [158:5] free named register 'rsi'
;   [163:5] assert(worlds[0].locations[1] == 0xffee)
;   [163:12] allocate scratch register -> r15
;   [163:12] worlds[0].locations[1] == 0xffee
;   [163:12] ? worlds[0].locations[1] == 0xffee
;   [163:12] ? worlds[0].locations[1] == 0xffee
    cmp_163_12:
;   [163:12] allocate scratch register -> r14
;       [163:12] worlds[0].locations[1]
;       [163:12] worlds[0].locations[1]
;       [163:12] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [163:12] allocate scratch register -> r12
;       [163:19] set array index
;       [163:19] 0
;       [163:19] 0
;       [163:19] 0
        mov r12, 0
;       [163:19] bounds check
;       [163:19] allocate scratch register -> r11
;       [163:19] line number
        mov r11, 163
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [163:19] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [163:12] free scratch register 'r12'
;       [163:12] allocate scratch register -> r12
;       [163:32] set array index
;       [163:32] 1
;       [163:32] 1
;       [163:32] 1
        mov r12, 1
;       [163:32] bounds check
;       [163:32] allocate scratch register -> r11
;       [163:32] line number
        mov r11, 163
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [163:32] free scratch register 'r11'
        mov r14, qword [r13 + r12 * 8]
;       [163:12] free scratch register 'r12'
;       [163:12] free scratch register 'r13'
    cmp r14, 0xffee
;   [163:12] free scratch register 'r14'
    jne bool_false_163_12
    jmp bool_true_163_12
    bool_true_163_12:
    mov r15, true
    jmp bool_end_163_12
    bool_false_163_12:
    mov r15, false
    bool_end_163_12:
;   [18:6] assert(expr : bool) 
    assert_163_5:
;       [163:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_163_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_163_5:
        test r15, r15
        jne if_18_26_163_5_end
        jmp if_18_29_163_5_code
        if_18_29_163_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_163_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_163_5_end:
        if_18_26_163_5_end:
;       [163:5] free scratch register 'r15'
    assert_163_5_end:
;   [164:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [164:12] allocate scratch register -> r15
;   [164:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [164:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [164:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_164_12:
;   [164:12] allocate scratch register -> r14
;       [164:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [164:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [164:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [164:12] allocate named register 'rsi'
;       [164:12] allocate named register 'rdi'
;       [164:12] allocate named register 'rcx'
;       [166:14] array_size_of(worlds.locations)
;       [166:14] array_size_of(worlds.locations)
;       [167:14] array_size_of(worlds.locations)
;       [167:14] rcx = array_size_of(worlds.locations)
;       [167:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [165:14] worlds[0].locations
;       [165:14] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [165:14] allocate scratch register -> r12
;       [165:21] set array index
;       [165:21] 0
;       [165:21] 0
;       [165:21] 0
        mov r12, 0
;       [165:21] bounds check
;       [165:21] allocate scratch register -> r11
;       [165:21] line number
        mov r11, 165
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [165:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [165:14] free scratch register 'r12'
;       [165:14] bounds check
;       [165:14] allocate scratch register -> r12
;       [165:14] line number
        mov r12, 165
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [165:14] free scratch register 'r12'
        lea rsi, [r13]
;       [164:12] free scratch register 'r13'
;       [166:14] worlds[1].locations
;       [166:14] allocate scratch register -> r13
        lea r13, [rsp - 796]
;       [166:14] allocate scratch register -> r12
;       [166:21] set array index
;       [166:21] 1
;       [166:21] 1
;       [166:21] 1
        mov r12, 1
;       [166:21] bounds check
;       [166:21] allocate scratch register -> r11
;       [166:21] line number
        mov r11, 166
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [166:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [166:14] free scratch register 'r12'
;       [166:14] bounds check
;       [166:14] allocate scratch register -> r12
;       [166:14] line number
        mov r12, 166
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [166:14] free scratch register 'r12'
        lea rdi, [r13]
;       [164:12] free scratch register 'r13'
        shl rcx, 3
        repe cmpsb
;       [164:12] free named register 'rcx'
;       [164:12] free named register 'rdi'
;       [164:12] free named register 'rsi'
        je cmps_eq_164_12
        mov r14, false
        jmp cmps_end_164_12
        cmps_eq_164_12:
        mov r14, true
        cmps_end_164_12:
    test r14, r14
;   [164:12] free scratch register 'r14'
    je bool_false_164_12
    jmp bool_true_164_12
    bool_true_164_12:
    mov r15, true
    jmp bool_end_164_12
    bool_false_164_12:
    mov r15, false
    bool_end_164_12:
;   [18:6] assert(expr : bool) 
    assert_164_5:
;       [164:5] alias expr -> r15  (lea: , len: 0)
        if_18_29_164_5:
;       [18:29] ? not expr
;       [18:29] ? not expr
        cmp_18_29_164_5:
        test r15, r15
        jne if_18_26_164_5_end
        jmp if_18_29_164_5_code
        if_18_29_164_5_code:
;           [18:38] exit(1)
;           [18:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_18_38_164_5:
;               [18:38] alias v -> rdi  (lea: , len: 0)
;               [13:5] mov(rax, 60)
;                   [13:14] 60
;                   [13:14] 60
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;                   [14:14] v
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [18:38] free named register 'rdi'
            exit_18_38_164_5_end:
        if_18_26_164_5_end:
;       [164:5] free scratch register 'r15'
    assert_164_5_end:
;   [170:5] print(hello.len, hello)
;   [170:11] allocate named register 'rdx'
    mov rdx, hello.len
;   [170:22] allocate named register 'rsi'
    mov rsi, hello
;   [20:6] print(len : reg_rdx, ptr : reg_rsi) 
    print_170_5:
;       [170:5] alias len -> rdx  (lea: , len: 0)
;       [170:5] alias ptr -> rsi  (lea: , len: 0)
;       [21:5] mov(rax, 1)
;           [21:14] 1
;           [21:14] 1
;           [21:14] 1
            mov rax, 1
;       [21:19] # write system call
;       [22:5] mov(rdi, 1)
;           [22:14] 1
;           [22:14] 1
;           [22:14] 1
            mov rdi, 1
;       [22:19] # file descriptor for standard out
;       [23:5] mov(rsi, ptr)
;           [23:14] ptr
;           [23:14] ptr
;           [23:14] ptr
;       [23:19] # buffer address
;       [24:5] mov(rdx, len)
;           [24:14] len
;           [24:14] len
;           [24:14] len
;       [24:19] # buffer size
;       [25:5] syscall()
        syscall
;       [170:5] free named register 'rsi'
;       [170:5] free named register 'rdx'
    print_170_5_end:
;   [171:5] loop
    loop_171_5:
;       [172:9] print(prompt1.len, prompt1)
;       [172:15] allocate named register 'rdx'
        mov rdx, prompt1.len
;       [172:28] allocate named register 'rsi'
        mov rsi, prompt1
;       [20:6] print(len : reg_rdx, ptr : reg_rsi) 
        print_172_9:
;           [172:9] alias len -> rdx  (lea: , len: 0)
;           [172:9] alias ptr -> rsi  (lea: , len: 0)
;           [21:5] mov(rax, 1)
;               [21:14] 1
;               [21:14] 1
;               [21:14] 1
                mov rax, 1
;           [21:19] # write system call
;           [22:5] mov(rdi, 1)
;               [22:14] 1
;               [22:14] 1
;               [22:14] 1
                mov rdi, 1
;           [22:19] # file descriptor for standard out
;           [23:5] mov(rsi, ptr)
;               [23:14] ptr
;               [23:14] ptr
;               [23:14] ptr
;           [23:19] # buffer address
;           [24:5] mov(rdx, len)
;               [24:14] len
;               [24:14] len
;               [24:14] len
;           [24:19] # buffer size
;           [25:5] syscall()
            syscall
;           [172:9] free named register 'rsi'
;           [172:9] free named register 'rdx'
        print_172_9_end:
;       [174:9] var len = read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [174:13] len: i64 @ qword [rsp - 804]
;       [174:13] len =read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [174:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [174:19] read(array_size_of(input_buffer), address_of(input_buffer)) - 1
;       [174:19] len = read(array_size_of(input_buffer), address_of(input_buffer))
;       [174:19] read(array_size_of(input_buffer), address_of(input_buffer))
;       [174:24] allocate named register 'rdx'
;       [174:24] array_size_of(input_buffer)
;       [174:24] array_size_of(input_buffer)
;       [174:24] rdx = array_size_of(input_buffer)
;       [174:24] array_size_of(input_buffer)
        mov rdx, 80
;       [174:53] allocate named register 'rsi'
;       [174:53] address_of(input_buffer)
;       [174:53] address_of(input_buffer)
;       [174:53] rsi = address_of(input_buffer)
;       [174:53] address_of(input_buffer)
        lea rsi, [rsp - 284]
;       [28:6] read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes 
        read_174_19:
;           [174:19] alias nbytes -> qword [rsp - 804]  (lea: , len: 0)
;           [174:19] alias len -> rdx  (lea: , len: 0)
;           [174:19] alias ptr -> rsi  (lea: , len: 0)
;           [29:5] mov(rax, 0)
;               [29:14] 0
;               [29:14] 0
;               [29:14] 0
                mov rax, 0
;           [29:19] # read system call
;           [30:5] mov(rdi, 0)
;               [30:14] 0
;               [30:14] 0
;               [30:14] 0
                mov rdi, 0
;           [30:19] # file descriptor for standard input
;           [31:5] mov(rsi, ptr)
;               [31:14] ptr
;               [31:14] ptr
;               [31:14] ptr
;           [31:19] # buffer address
;           [32:5] mov(rdx, len)
;               [32:14] len
;               [32:14] len
;               [32:14] len
;           [32:19] # buffer size
;           [33:5] syscall()
            syscall
;           [34:5] mov(nbytes, rax)
;               [34:17] rax
;               [34:17] rax
;               [34:17] rax
                mov qword [rsp - 804], rax
;           [34:22] # return value
;           [174:19] free named register 'rsi'
;           [174:19] free named register 'rdx'
        read_174_19_end:
;       [174:81] len - 1
        sub qword [rsp - 804], 1
;       [175:9] # note: `array_size_of` and `address_of` are built-in functions
;       [176:9] # -1 to not include the trailing '\n'
        if_178_12:
;       [178:12] ? len == 0
;       [178:12] ? len == 0
        cmp_178_12:
        cmp qword [rsp - 804], 0
        jne if_180_19
        jmp if_178_12_code
        if_178_12_code:
;           [179:13] break
            jmp loop_171_5_end
        jmp if_178_9_end
        if_180_19:
;       [180:19] ? len <= 4
;       [180:19] ? len <= 4
        cmp_180_19:
        cmp qword [rsp - 804], 4
        jg if_else_178_9
        jmp if_180_19_code
        if_180_19_code:
;           [181:13] print(prompt2.len, prompt2)
;           [181:19] allocate named register 'rdx'
            mov rdx, prompt2.len
;           [181:32] allocate named register 'rsi'
            mov rsi, prompt2
;           [20:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_181_13:
;               [181:13] alias len -> rdx  (lea: , len: 0)
;               [181:13] alias ptr -> rsi  (lea: , len: 0)
;               [21:5] mov(rax, 1)
;                   [21:14] 1
;                   [21:14] 1
;                   [21:14] 1
                    mov rax, 1
;               [21:19] # write system call
;               [22:5] mov(rdi, 1)
;                   [22:14] 1
;                   [22:14] 1
;                   [22:14] 1
                    mov rdi, 1
;               [22:19] # file descriptor for standard out
;               [23:5] mov(rsi, ptr)
;                   [23:14] ptr
;                   [23:14] ptr
;                   [23:14] ptr
;               [23:19] # buffer address
;               [24:5] mov(rdx, len)
;                   [24:14] len
;                   [24:14] len
;                   [24:14] len
;               [24:19] # buffer size
;               [25:5] syscall()
                syscall
;               [181:13] free named register 'rsi'
;               [181:13] free named register 'rdx'
            print_181_13_end:
;           [182:13] continue
            jmp loop_171_5
        jmp if_178_9_end
        if_else_178_9:
;           [184:13] print(prompt3.len, prompt3)
;           [184:19] allocate named register 'rdx'
            mov rdx, prompt3.len
;           [184:32] allocate named register 'rsi'
            mov rsi, prompt3
;           [20:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_184_13:
;               [184:13] alias len -> rdx  (lea: , len: 0)
;               [184:13] alias ptr -> rsi  (lea: , len: 0)
;               [21:5] mov(rax, 1)
;                   [21:14] 1
;                   [21:14] 1
;                   [21:14] 1
                    mov rax, 1
;               [21:19] # write system call
;               [22:5] mov(rdi, 1)
;                   [22:14] 1
;                   [22:14] 1
;                   [22:14] 1
                    mov rdi, 1
;               [22:19] # file descriptor for standard out
;               [23:5] mov(rsi, ptr)
;                   [23:14] ptr
;                   [23:14] ptr
;                   [23:14] ptr
;               [23:19] # buffer address
;               [24:5] mov(rdx, len)
;                   [24:14] len
;                   [24:14] len
;                   [24:14] len
;               [24:19] # buffer size
;               [25:5] syscall()
                syscall
;               [184:13] free named register 'rsi'
;               [184:13] free named register 'rdx'
            print_184_13_end:
;           [185:13] print(len, address_of(input_buffer))
;           [185:19] allocate named register 'rdx'
            mov rdx, qword [rsp - 804]
;           [185:24] allocate named register 'rsi'
;           [185:24] address_of(input_buffer)
;           [185:24] address_of(input_buffer)
;           [185:24] rsi = address_of(input_buffer)
;           [185:24] address_of(input_buffer)
            lea rsi, [rsp - 284]
;           [20:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_185_13:
;               [185:13] alias len -> rdx  (lea: , len: 0)
;               [185:13] alias ptr -> rsi  (lea: , len: 0)
;               [21:5] mov(rax, 1)
;                   [21:14] 1
;                   [21:14] 1
;                   [21:14] 1
                    mov rax, 1
;               [21:19] # write system call
;               [22:5] mov(rdi, 1)
;                   [22:14] 1
;                   [22:14] 1
;                   [22:14] 1
                    mov rdi, 1
;               [22:19] # file descriptor for standard out
;               [23:5] mov(rsi, ptr)
;                   [23:14] ptr
;                   [23:14] ptr
;                   [23:14] ptr
;               [23:19] # buffer address
;               [24:5] mov(rdx, len)
;                   [24:14] len
;                   [24:14] len
;                   [24:14] len
;               [24:19] # buffer size
;               [25:5] syscall()
                syscall
;               [185:13] free named register 'rsi'
;               [185:13] free named register 'rdx'
            print_185_13_end:
;           [186:13] print(dot.len, dot)
;           [186:19] allocate named register 'rdx'
            mov rdx, dot.len
;           [186:28] allocate named register 'rsi'
            mov rsi, dot
;           [20:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_186_13:
;               [186:13] alias len -> rdx  (lea: , len: 0)
;               [186:13] alias ptr -> rsi  (lea: , len: 0)
;               [21:5] mov(rax, 1)
;                   [21:14] 1
;                   [21:14] 1
;                   [21:14] 1
                    mov rax, 1
;               [21:19] # write system call
;               [22:5] mov(rdi, 1)
;                   [22:14] 1
;                   [22:14] 1
;                   [22:14] 1
                    mov rdi, 1
;               [22:19] # file descriptor for standard out
;               [23:5] mov(rsi, ptr)
;                   [23:14] ptr
;                   [23:14] ptr
;                   [23:14] ptr
;               [23:19] # buffer address
;               [24:5] mov(rdx, len)
;                   [24:14] len
;                   [24:14] len
;                   [24:14] len
;               [24:19] # buffer size
;               [25:5] syscall()
                syscall
;               [186:13] free named register 'rsi'
;               [186:13] free named register 'rdx'
            print_186_13_end:
;           [187:13] print(nl.len, nl)
;           [187:19] allocate named register 'rdx'
            mov rdx, nl.len
;           [187:27] allocate named register 'rsi'
            mov rsi, nl
;           [20:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_187_13:
;               [187:13] alias len -> rdx  (lea: , len: 0)
;               [187:13] alias ptr -> rsi  (lea: , len: 0)
;               [21:5] mov(rax, 1)
;                   [21:14] 1
;                   [21:14] 1
;                   [21:14] 1
                    mov rax, 1
;               [21:19] # write system call
;               [22:5] mov(rdi, 1)
;                   [22:14] 1
;                   [22:14] 1
;                   [22:14] 1
                    mov rdi, 1
;               [22:19] # file descriptor for standard out
;               [23:5] mov(rsi, ptr)
;                   [23:14] ptr
;                   [23:14] ptr
;                   [23:14] ptr
;               [23:19] # buffer address
;               [24:5] mov(rdx, len)
;                   [24:14] len
;                   [24:14] len
;                   [24:14] len
;               [24:19] # buffer size
;               [25:5] syscall()
                syscall
;               [187:13] free named register 'rsi'
;               [187:13] free named register 'rdx'
            print_187_13_end:
        if_178_9_end:
    jmp loop_171_5
    loop_171_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

panic_bounds:
;   print message to stderr
    mov rax, 1
    mov rdi, 2
    lea rsi, [msg_panic]
    mov rdx, msg_panic_len
    syscall
;   line number is in `rbp`
    mov rax, rbp
;   convert to string
    lea rdi, [num_buffer + 19]
    mov byte [rdi], 10
    dec rdi
    mov rcx, 10
.convert_loop:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    test rax, rax
    jnz .convert_loop
    inc rdi
;   print line number to stderr
    mov rax, 1
    mov rsi, rdi
    lea rdx, [num_buffer + 20]
    sub rdx, rdi
    mov rdi, 2
    syscall
;   exit with error code 255
    mov rax, 60
    mov rdi, 255
    syscall
section .rodata
    msg_panic: db 'panic: bounds at line '
    msg_panic_len equ $ - msg_panic
section .bss
    num_buffer: resb 21

; max scratch registers in use: 5
;            max frames in use: 7
;               max stack size: 804 B
```
