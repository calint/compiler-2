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
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`, `address_of`, `equal`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
run `prog.baz`
  * optional parameters: _stack size_, _bounds check_, with _line number
    information_ e.g:
    * `./run-baz.sh myprogram.baz --stack=262144`: stack size, no runtime checks
    * `./run-baz.sh myprogram.baz --stack=262144 --check=upper`: checks upper
    bounds without line number information and is often enough to ensure catching
    negative values (faster)
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper,line`: checks
    upper bounds with line number information
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper,lower,line`: checks
    bounds with line number information
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    41           1344            737           6487
C++                              1             81             84            594
-------------------------------------------------------------------------------
SUM:                            42           1425            821           7081
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

# single statement blocks can ommit { ... }

func assert(expr : bool) if not expr exit(1)

func print(len : reg_rdx, ptr : reg_rsi) {
    mov(rax, 1)   # write system call
    mov(rdi, 0)   # file descriptor for standard out
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

type str {
    len : i8,
    data : i8[127]
}

func str_in(s : str) {
    mov(rax, 0)   # read system call
    mov(rdi, 0)   # file descriptor for standard input
    mov(rsi, address_of(s.data)) # buffer address
    mov(rdx, array_size_of(s.data)) # buffer size
    syscall()
    mov(s.len, rax - 1) # return value
} 

func str_out(s : str) {
    mov(rax, 1)   # write system call
    mov(rdi, 0)   # file descriptor for standard out
    mov(rsi, address_of(s.data)) # buffer address 
    mov(rdx, s.len) # buffer size
    syscall()
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
    # note: `arrays_equal` is built-in function

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

    var q : point = p
    assert(equal(p, q))
    # use `equal` to compare for equality

    q.x = 3
    assert(not equal(p, q))

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

    var worlds : world[8]
    worlds[1].locations[1] = 0xffee
    assert(worlds[1].locations[1] == 0xffee)

    array_copy(
        worlds[1].locations,
        worlds[0].locations,
        array_size_of(worlds.locations)
    )
    # note: `array_copy` is built-in and can use indexed positions
    #       `array_size_of` is built-in
    assert(worlds[0].locations[1] == 0xffee)
    assert(arrays_equal(
             worlds[0].locations,
             worlds[1].locations,
             array_size_of(worlds.locations)
          ))

    var nm : str
    print(hello.len, hello)
    loop {
        print(prompt1.len, prompt1)
        str_in(nm)
        if nm.len == 0 {
            break
        } else if nm.len <= 4 {
            print(prompt2.len, prompt2)
            continue
        } else {
            print(prompt3.len, prompt3)
            str_out(nm)
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
    mov qword [rsp - 16], 0
    mov qword [rsp - 8], 0
    mov qword [rsp - 24], 1
    mov r15, qword [rsp - 24]
    mov r14, 107
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 16], 2
    mov r15, qword [rsp - 24]
    add r15, 1
    mov r14, 108
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    mov r13, 108
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
    cmp_109_12:
    mov r14, 1
    mov r13, 109
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_109_12
    bool_true_109_12:
    mov r15b, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15b, false
    bool_end_109_12:
    assert_109_5:
        if_20_29_109_5:
        cmp_20_29_109_5:
        test r15b, r15b
        jne if_20_26_109_5_end
        if_20_29_109_5_code:
            mov rdi, 1
            exit_20_38_109_5:
                    mov rax, 60
                syscall
            exit_20_38_109_5_end:
        if_20_26_109_5_end:
    assert_109_5_end:
    cmp_110_12:
    mov r14, 2
    mov r13, 110
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_110_12
    bool_true_110_12:
    mov r15b, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15b, false
    bool_end_110_12:
    assert_110_5:
        if_20_29_110_5:
        cmp_20_29_110_5:
        test r15b, r15b
        jne if_20_26_110_5_end
        if_20_29_110_5_code:
            mov rdi, 1
            exit_20_38_110_5:
                    mov rax, 60
                syscall
            exit_20_38_110_5_end:
        if_20_26_110_5_end:
    assert_110_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 112
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 16]
    mov r15, 112
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
    cmp_114_12:
    mov r14, 0
    mov r13, 114
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_114_12
    bool_true_114_12:
    mov r15b, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15b, false
    bool_end_114_12:
    assert_114_5:
        if_20_29_114_5:
        cmp_20_29_114_5:
        test r15b, r15b
        jne if_20_26_114_5_end
        if_20_29_114_5_code:
            mov rdi, 1
            exit_20_38_114_5:
                    mov rax, 60
                syscall
            exit_20_38_114_5_end:
        if_20_26_114_5_end:
    assert_114_5_end:
    mov qword [rsp - 56], 0
    mov qword [rsp - 48], 0
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
    mov rcx, 4
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 16]
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
    cmp_118_12:
        mov rcx, 4
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_118_12
        mov r14, false
        jmp cmps_end_118_12
        cmps_eq_118_12:
        mov r14, true
        cmps_end_118_12:
    test r14, r14
    je bool_false_118_12
    bool_true_118_12:
    mov r15b, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15b, false
    bool_end_118_12:
    assert_118_5:
        if_20_29_118_5:
        cmp_20_29_118_5:
        test r15b, r15b
        jne if_20_26_118_5_end
        if_20_29_118_5_code:
            mov rdi, 1
            exit_20_38_118_5:
                    mov rax, 60
                syscall
            exit_20_38_118_5_end:
        if_20_26_118_5_end:
    assert_118_5_end:
    mov r15, 2
    mov r14, 121
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 56], -1
    cmp_122_12:
        mov rcx, 4
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 16]
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
        je cmps_eq_122_16
        mov r14, false
        jmp cmps_end_122_16
        cmps_eq_122_16:
        mov r14, true
        cmps_end_122_16:
    test r14, r14
    jne bool_false_122_12
    bool_true_122_12:
    mov r15b, true
    jmp bool_end_122_12
    bool_false_122_12:
    mov r15b, false
    bool_end_122_12:
    assert_122_5:
        if_20_29_122_5:
        cmp_20_29_122_5:
        test r15b, r15b
        jne if_20_26_122_5_end
        if_20_29_122_5_code:
            mov rdi, 1
            exit_20_38_122_5:
                    mov rax, 60
                syscall
            exit_20_38_122_5_end:
        if_20_26_122_5_end:
    assert_122_5_end:
    mov qword [rsp - 24], 3
    mov r15, qword [rsp - 24]
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 24]
    sub r14, 1
    mov r13, 125
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_125_16:
        mov r13d, dword [rsp + r14 * 4 - 16]
        mov dword [rsp + r15 * 4 - 16], r13d
        not dword [rsp + r15 * 4 - 16]
    inv_125_16_end:
    not dword [rsp + r15 * 4 - 16]
    cmp_126_12:
    mov r14, qword [rsp - 24]
    mov r13, 126
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 2
    jne bool_false_126_12
    bool_true_126_12:
    mov r15b, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15b, false
    bool_end_126_12:
    assert_126_5:
        if_20_29_126_5:
        cmp_20_29_126_5:
        test r15b, r15b
        jne if_20_26_126_5_end
        if_20_29_126_5_code:
            mov rdi, 1
            exit_20_38_126_5:
                    mov rax, 60
                syscall
            exit_20_38_126_5_end:
        if_20_26_126_5_end:
    assert_126_5_end:
    faz_128_5:
        mov r15, 1
        mov r14, 76
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 16], 0xfe
    faz_128_5_end:
    cmp_129_12:
    mov r14, 1
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 16], 0xfe
    jne bool_false_129_12
    bool_true_129_12:
    mov r15b, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15b, false
    bool_end_129_12:
    assert_129_5:
        if_20_29_129_5:
        cmp_20_29_129_5:
        test r15b, r15b
        jne if_20_26_129_5_end
        if_20_29_129_5_code:
            mov rdi, 1
            exit_20_38_129_5:
                    mov rax, 60
                syscall
            exit_20_38_129_5_end:
        if_20_26_129_5_end:
    assert_129_5_end:
    mov qword [rsp - 72], 0
    mov qword [rsp - 64], 0
    foo_132_5:
        mov qword [rsp - 72], 0b10
        mov qword [rsp - 64], 0xb
    foo_132_5_end:
    cmp_133_12:
    cmp qword [rsp - 72], 2
    jne bool_false_133_12
    bool_true_133_12:
    mov r15b, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15b, false
    bool_end_133_12:
    assert_133_5:
        if_20_29_133_5:
        cmp_20_29_133_5:
        test r15b, r15b
        jne if_20_26_133_5_end
        if_20_29_133_5_code:
            mov rdi, 1
            exit_20_38_133_5:
                    mov rax, 60
                syscall
            exit_20_38_133_5_end:
        if_20_26_133_5_end:
    assert_133_5_end:
    cmp_134_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_134_12
    bool_true_134_12:
    mov r15b, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15b, false
    bool_end_134_12:
    assert_134_5:
        if_20_29_134_5:
        cmp_20_29_134_5:
        test r15b, r15b
        jne if_20_26_134_5_end
        if_20_29_134_5_code:
            mov rdi, 1
            exit_20_38_134_5:
                    mov rax, 60
                syscall
            exit_20_38_134_5_end:
        if_20_26_134_5_end:
    assert_134_5_end:
    lea rsi, [rsp - 72]
    lea rdi, [rsp - 88]
    mov rcx, 2
    rep movsq
    cmp_137_12:
        lea rsi, [rsp - 72]
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
        je cmps_eq_137_12
        mov r14, false
        jmp cmps_end_137_12
        cmps_eq_137_12:
        mov r14, true
        cmps_end_137_12:
    test r14, r14
    je bool_false_137_12
    bool_true_137_12:
    mov r15b, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15b, false
    bool_end_137_12:
    assert_137_5:
        if_20_29_137_5:
        cmp_20_29_137_5:
        test r15b, r15b
        jne if_20_26_137_5_end
        if_20_29_137_5_code:
            mov rdi, 1
            exit_20_38_137_5:
                    mov rax, 60
                syscall
            exit_20_38_137_5_end:
        if_20_26_137_5_end:
    assert_137_5_end:
    mov qword [rsp - 88], 3
    cmp_141_12:
        lea rsi, [rsp - 72]
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
        je cmps_eq_141_16
        mov r14, false
        jmp cmps_end_141_16
        cmps_eq_141_16:
        mov r14, true
        cmps_end_141_16:
    test r14, r14
    jne bool_false_141_12
    bool_true_141_12:
    mov r15b, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15b, false
    bool_end_141_12:
    assert_141_5:
        if_20_29_141_5:
        cmp_20_29_141_5:
        test r15b, r15b
        jne if_20_26_141_5_end
        if_20_29_141_5_code:
            mov rdi, 1
            exit_20_38_141_5:
                    mov rax, 60
                syscall
            exit_20_38_141_5_end:
        if_20_26_141_5_end:
    assert_141_5_end:
    mov qword [rsp - 96], 0
    bar_144_5:
        if_59_8_144_5:
        cmp_59_8_144_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_144_5_end
        if_59_8_144_5_code:
            jmp bar_144_5_end
        if_59_5_144_5_end:
        mov qword [rsp - 96], 0xff
    bar_144_5_end:
    cmp_145_12:
    cmp qword [rsp - 96], 0
    jne bool_false_145_12
    bool_true_145_12:
    mov r15b, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15b, false
    bool_end_145_12:
    assert_145_5:
        if_20_29_145_5:
        cmp_20_29_145_5:
        test r15b, r15b
        jne if_20_26_145_5_end
        if_20_29_145_5_code:
            mov rdi, 1
            exit_20_38_145_5:
                    mov rax, 60
                syscall
            exit_20_38_145_5_end:
        if_20_26_145_5_end:
    assert_145_5_end:
    mov qword [rsp - 96], 1
    bar_148_5:
        if_59_8_148_5:
        cmp_59_8_148_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_148_5_end
        if_59_8_148_5_code:
            jmp bar_148_5_end
        if_59_5_148_5_end:
        mov qword [rsp - 96], 0xff
    bar_148_5_end:
    cmp_149_12:
    cmp qword [rsp - 96], 0xff
    jne bool_false_149_12
    bool_true_149_12:
    mov r15b, true
    jmp bool_end_149_12
    bool_false_149_12:
    mov r15b, false
    bool_end_149_12:
    assert_149_5:
        if_20_29_149_5:
        cmp_20_29_149_5:
        test r15b, r15b
        jne if_20_26_149_5_end
        if_20_29_149_5_code:
            mov rdi, 1
            exit_20_38_149_5:
                    mov rax, 60
                syscall
            exit_20_38_149_5_end:
        if_20_26_149_5_end:
    assert_149_5_end:
    mov qword [rsp - 104], 1
    baz_152_13:
        mov r15, qword [rsp - 104]
        imul r15, 2
    baz_152_13_end:
    mov qword [rsp - 112], r15
    cmp_153_12:
    cmp qword [rsp - 112], 2
    jne bool_false_153_12
    bool_true_153_12:
    mov r15b, true
    jmp bool_end_153_12
    bool_false_153_12:
    mov r15b, false
    bool_end_153_12:
    assert_153_5:
        if_20_29_153_5:
        cmp_20_29_153_5:
        test r15b, r15b
        jne if_20_26_153_5_end
        if_20_29_153_5_code:
            mov rdi, 1
            exit_20_38_153_5:
                    mov rax, 60
                syscall
            exit_20_38_153_5_end:
        if_20_26_153_5_end:
    assert_153_5_end:
    baz_155_9:
        mov r15, 1
        imul r15, 2
    baz_155_9_end:
    mov qword [rsp - 112], r15
    cmp_156_12:
    cmp qword [rsp - 112], 2
    jne bool_false_156_12
    bool_true_156_12:
    mov r15b, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15b, false
    bool_end_156_12:
    assert_156_5:
        if_20_29_156_5:
        cmp_20_29_156_5:
        test r15b, r15b
        jne if_20_26_156_5_end
        if_20_29_156_5_code:
            mov rdi, 1
            exit_20_38_156_5:
                    mov rax, 60
                syscall
            exit_20_38_156_5_end:
        if_20_26_156_5_end:
    assert_156_5_end:
    baz_158_23:
        mov qword [rsp - 128], 2
        mov r15, qword [rsp - 128]
        imul r15, 2
        mov qword [rsp - 128], r15
    baz_158_23_end:
    mov qword [rsp - 120], 0
    cmp_159_12:
    cmp qword [rsp - 128], 4
    jne bool_false_159_12
    bool_true_159_12:
    mov r15b, true
    jmp bool_end_159_12
    bool_false_159_12:
    mov r15b, false
    bool_end_159_12:
    assert_159_5:
        if_20_29_159_5:
        cmp_20_29_159_5:
        test r15b, r15b
        jne if_20_26_159_5_end
        if_20_29_159_5_code:
            mov rdi, 1
            exit_20_38_159_5:
                    mov rax, 60
                syscall
            exit_20_38_159_5_end:
        if_20_26_159_5_end:
    assert_159_5_end:
    mov qword [rsp - 136], 1
    mov qword [rsp - 144], 2
    mov r15, qword [rsp - 136]
    mov qword [rsp - 164], r15
    mov r15, qword [rsp - 164]
    imul r15, 10
    mov qword [rsp - 164], r15
    mov r15, qword [rsp - 144]
    mov qword [rsp - 156], r15
    mov dword [rsp - 148], 0xff0000
    cmp_165_12:
    cmp qword [rsp - 164], 10
    jne bool_false_165_12
    bool_true_165_12:
    mov r15b, true
    jmp bool_end_165_12
    bool_false_165_12:
    mov r15b, false
    bool_end_165_12:
    assert_165_5:
        if_20_29_165_5:
        cmp_20_29_165_5:
        test r15b, r15b
        jne if_20_26_165_5_end
        if_20_29_165_5_code:
            mov rdi, 1
            exit_20_38_165_5:
                    mov rax, 60
                syscall
            exit_20_38_165_5_end:
        if_20_26_165_5_end:
    assert_165_5_end:
    cmp_166_12:
    cmp qword [rsp - 156], 2
    jne bool_false_166_12
    bool_true_166_12:
    mov r15b, true
    jmp bool_end_166_12
    bool_false_166_12:
    mov r15b, false
    bool_end_166_12:
    assert_166_5:
        if_20_29_166_5:
        cmp_20_29_166_5:
        test r15b, r15b
        jne if_20_26_166_5_end
        if_20_29_166_5_code:
            mov rdi, 1
            exit_20_38_166_5:
                    mov rax, 60
                syscall
            exit_20_38_166_5_end:
        if_20_26_166_5_end:
    assert_166_5_end:
    cmp_167_12:
    cmp dword [rsp - 148], 0xff0000
    jne bool_false_167_12
    bool_true_167_12:
    mov r15b, true
    jmp bool_end_167_12
    bool_false_167_12:
    mov r15b, false
    bool_end_167_12:
    assert_167_5:
        if_20_29_167_5:
        cmp_20_29_167_5:
        test r15b, r15b
        jne if_20_26_167_5_end
        if_20_29_167_5_code:
            mov rdi, 1
            exit_20_38_167_5:
                    mov rax, 60
                syscall
            exit_20_38_167_5_end:
        if_20_26_167_5_end:
    assert_167_5_end:
    mov r15, qword [rsp - 136]
    mov qword [rsp - 180], r15
    neg qword [rsp - 180]
    mov r15, qword [rsp - 144]
    mov qword [rsp - 172], r15
    neg qword [rsp - 172]
    lea rsi, [rsp - 180]
    lea rdi, [rsp - 164]
    mov rcx, 2
    rep movsq
    cmp_171_12:
    cmp qword [rsp - 164], -1
    jne bool_false_171_12
    bool_true_171_12:
    mov r15b, true
    jmp bool_end_171_12
    bool_false_171_12:
    mov r15b, false
    bool_end_171_12:
    assert_171_5:
        if_20_29_171_5:
        cmp_20_29_171_5:
        test r15b, r15b
        jne if_20_26_171_5_end
        if_20_29_171_5_code:
            mov rdi, 1
            exit_20_38_171_5:
                    mov rax, 60
                syscall
            exit_20_38_171_5_end:
        if_20_26_171_5_end:
    assert_171_5_end:
    cmp_172_12:
    cmp qword [rsp - 156], -2
    jne bool_false_172_12
    bool_true_172_12:
    mov r15b, true
    jmp bool_end_172_12
    bool_false_172_12:
    mov r15b, false
    bool_end_172_12:
    assert_172_5:
        if_20_29_172_5:
        cmp_20_29_172_5:
        test r15b, r15b
        jne if_20_26_172_5_end
        if_20_29_172_5_code:
            mov rdi, 1
            exit_20_38_172_5:
                    mov rax, 60
                syscall
            exit_20_38_172_5_end:
        if_20_26_172_5_end:
    assert_172_5_end:
    lea rsi, [rsp - 164]
    lea rdi, [rsp - 200]
    mov rcx, 20
    rep movsb
    cmp_175_12:
    cmp qword [rsp - 200], -1
    jne bool_false_175_12
    bool_true_175_12:
    mov r15b, true
    jmp bool_end_175_12
    bool_false_175_12:
    mov r15b, false
    bool_end_175_12:
    assert_175_5:
        if_20_29_175_5:
        cmp_20_29_175_5:
        test r15b, r15b
        jne if_20_26_175_5_end
        if_20_29_175_5_code:
            mov rdi, 1
            exit_20_38_175_5:
                    mov rax, 60
                syscall
            exit_20_38_175_5_end:
        if_20_26_175_5_end:
    assert_175_5_end:
    cmp_176_12:
    cmp qword [rsp - 192], -2
    jne bool_false_176_12
    bool_true_176_12:
    mov r15b, true
    jmp bool_end_176_12
    bool_false_176_12:
    mov r15b, false
    bool_end_176_12:
    assert_176_5:
        if_20_29_176_5:
        cmp_20_29_176_5:
        test r15b, r15b
        jne if_20_26_176_5_end
        if_20_29_176_5_code:
            mov rdi, 1
            exit_20_38_176_5:
                    mov rax, 60
                syscall
            exit_20_38_176_5_end:
        if_20_26_176_5_end:
    assert_176_5_end:
    cmp_177_12:
    cmp dword [rsp - 184], 0xff0000
    jne bool_false_177_12
    bool_true_177_12:
    mov r15b, true
    jmp bool_end_177_12
    bool_false_177_12:
    mov r15b, false
    bool_end_177_12:
    assert_177_5:
        if_20_29_177_5:
        cmp_20_29_177_5:
        test r15b, r15b
        jne if_20_26_177_5_end
        if_20_29_177_5_code:
            mov rdi, 1
            exit_20_38_177_5:
                    mov rax, 60
                syscall
            exit_20_38_177_5_end:
        if_20_26_177_5_end:
    assert_177_5_end:
    mov qword [rsp - 220], 0
    mov qword [rsp - 212], 0
    mov dword [rsp - 204], 0
    mov qword [rsp - 212], 73
    cmp_182_12:
    lea r14, [rsp - 220]
    mov r13, 0
    mov r12, 182
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    jne bool_false_182_12
    bool_true_182_12:
    mov r15b, true
    jmp bool_end_182_12
    bool_false_182_12:
    mov r15b, false
    bool_end_182_12:
    assert_182_5:
        if_20_29_182_5:
        cmp_20_29_182_5:
        test r15b, r15b
        jne if_20_26_182_5_end
        if_20_29_182_5_code:
            mov rdi, 1
            exit_20_38_182_5:
                    mov rax, 60
                syscall
            exit_20_38_182_5_end:
        if_20_26_182_5_end:
    assert_182_5_end:
    mov rcx, 64
    lea rdi, [rsp - 732]
    xor rax, rax
    rep stosq
    lea r15, [rsp - 732]
    mov r14, 1
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 0xffee
    cmp_186_12:
    lea r14, [rsp - 732]
    mov r13, 1
    mov r12, 186
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 186
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 0xffee
    jne bool_false_186_12
    bool_true_186_12:
    mov r15b, true
    jmp bool_end_186_12
    bool_false_186_12:
    mov r15b, false
    bool_end_186_12:
    assert_186_5:
        if_20_29_186_5:
        cmp_20_29_186_5:
        test r15b, r15b
        jne if_20_26_186_5_end
        if_20_29_186_5_code:
            mov rdi, 1
            exit_20_38_186_5:
                    mov rax, 60
                syscall
            exit_20_38_186_5_end:
        if_20_26_186_5_end:
    assert_186_5_end:
    mov rcx, 8
    lea r15, [rsp - 732]
    mov r14, 1
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 189
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 732]
    mov r14, 0
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 190
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_195_12:
    lea r14, [rsp - 732]
    mov r13, 0
    mov r12, 195
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 195
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 0xffee
    jne bool_false_195_12
    bool_true_195_12:
    mov r15b, true
    jmp bool_end_195_12
    bool_false_195_12:
    mov r15b, false
    bool_end_195_12:
    assert_195_5:
        if_20_29_195_5:
        cmp_20_29_195_5:
        test r15b, r15b
        jne if_20_26_195_5_end
        if_20_29_195_5_code:
            mov rdi, 1
            exit_20_38_195_5:
                    mov rax, 60
                syscall
            exit_20_38_195_5_end:
        if_20_26_195_5_end:
    assert_195_5_end:
    cmp_196_12:
        mov rcx, 8
        lea r13, [rsp - 732]
        mov r12, 0
        mov r11, 197
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 197
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 732]
        mov r12, 1
        mov r11, 198
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 198
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        je cmps_eq_196_12
        mov r14, false
        jmp cmps_end_196_12
        cmps_eq_196_12:
        mov r14, true
        cmps_end_196_12:
    test r14, r14
    je bool_false_196_12
    bool_true_196_12:
    mov r15b, true
    jmp bool_end_196_12
    bool_false_196_12:
    mov r15b, false
    bool_end_196_12:
    assert_196_5:
        if_20_29_196_5:
        cmp_20_29_196_5:
        test r15b, r15b
        jne if_20_26_196_5_end
        if_20_29_196_5_code:
            mov rdi, 1
            exit_20_38_196_5:
                    mov rax, 60
                syscall
            exit_20_38_196_5_end:
        if_20_26_196_5_end:
    assert_196_5_end:
    mov rcx, 16
    lea rdi, [rsp - 860]
    xor rax, rax
    rep stosq
    mov rdx, hello.len
    mov rsi, hello
    print_203_5:
            mov rax, 1
            mov rdi, 0
        syscall
    print_203_5_end:
    loop_204_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_205_9:
                mov rax, 1
                mov rdi, 0
            syscall
        print_205_9_end:
        str_in_206_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 859]
                mov rdx, 127
            syscall
                mov byte [rsp - 860], al
                sub byte [rsp - 860], 1
        str_in_206_9_end:
        if_207_12:
        cmp_207_12:
        cmp byte [rsp - 860], 0
        jne if_209_19
        if_207_12_code:
            jmp loop_204_5_end
        jmp if_207_9_end
        if_209_19:
        cmp_209_19:
        cmp byte [rsp - 860], 4
        jg if_else_207_9
        if_209_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_210_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_210_13_end:
            jmp loop_204_5
        jmp if_207_9_end
        if_else_207_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_213_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_213_13_end:
            str_out_214_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 859]
                    movsx rdx, byte [rsp - 860]
                syscall
            str_out_214_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_215_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_215_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_216_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_216_13_end:
        if_207_9_end:
    jmp loop_204_5
    loop_204_5_end:
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
;[18:1] # single statement blocks can ommit { ... }
;[39:1] # user types are defined using keyword `type`
;[41:1] # default type is `i64` and does not need to be specified
;[43:1] point : 16B    fields: 
;[43:1]       name :  offset :    size :  array? : array size
;[43:1]          x :       0 :       8 :      no :           
;[43:1]          y :       8 :       8 :      no :           

;[45:1] object : 20B    fields: 
;[45:1]       name :  offset :    size :  array? : array size
;[45:1]        pos :       0 :      16 :      no :           
;[45:1]      color :      16 :       4 :      no :           

;[47:1] world : 64B    fields: 
;[47:1]       name :  offset :    size :  array? : array size
;[47:1]  locations :       0 :      64 :     yes :          8

;[49:1] # function arguments are equivalent to mutable references
;[56:1] # default argument type is `i64`
;[63:1] # return target is specified as a variable, in this case `res`
;[73:1] # array arguments are declared with type and []
;[79:1] str : 128B    fields: 
;[79:1]       name :  offset :    size :  array? : array size
;[79:1]        len :       0 :       1 :      no :           
;[79:1]       data :       1 :     127 :     yes :        127

main:
;   [102:5] var arr : i32[4]
;   [102:9] arr: i32[4] (4B @ dword [rsp - 16])
;   [102:9] clear 4 * 4B = 16B
;   [102:5] size <= 32B, use mov
    mov qword [rsp - 16], 0
    mov qword [rsp - 8], 0
;   [103:5] # arrays are initialized to 0
;   [105:5] var ix = 1
;   [105:9] ix: i64 (8B @ qword [rsp - 24])
;   [105:9] ix = 1
;   [105:14] 1
    mov qword [rsp - 24], 1
;   [107:5] arr[ix] = 2
;   [107:5] allocate scratch register -> r15
;   [107:9] set array index
;   [107:9] ix
    mov r15, qword [rsp - 24]
;   [107:9] bounds check
;   [107:9] allocate scratch register -> r14
;   [107:9] line number
    mov r14, 107
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [107:9] free scratch register 'r14'
;   [107:15] 2
    mov dword [rsp + r15 * 4 - 16], 2
;   [107:5] free scratch register 'r15'
;   [108:5] arr[ix + 1] = arr[ix]
;   [108:5] allocate scratch register -> r15
;   [108:9] set array index
;   [108:9] ix
    mov r15, qword [rsp - 24]
;   [108:14] r15 + 1
    add r15, 1
;   [108:9] bounds check
;   [108:9] allocate scratch register -> r14
;   [108:9] line number
    mov r14, 108
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [108:9] free scratch register 'r14'
;   [108:19] arr[ix]
;   [108:19] allocate scratch register -> r14
;   [108:23] set array index
;   [108:23] ix
    mov r14, qword [rsp - 24]
;   [108:23] bounds check
;   [108:23] allocate scratch register -> r13
;   [108:23] line number
    mov r13, 108
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [108:23] free scratch register 'r13'
;   [108:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 16]
    mov dword [rsp + r15 * 4 - 16], r13d
;   [108:19] free scratch register 'r13'
;   [108:19] free scratch register 'r14'
;   [108:5] free scratch register 'r15'
;   [109:5] assert(arr[1] == 2)
;   [109:12] allocate scratch register -> r15
;   [109:12] ? arr[1] == 2
;   [109:12] ? arr[1] == 2
    cmp_109_12:
;   [109:12] allocate scratch register -> r14
;   [109:16] set array index
;   [109:16] 1
    mov r14, 1
;   [109:16] bounds check
;   [109:16] allocate scratch register -> r13
;   [109:16] line number
    mov r13, 109
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [109:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 16], 2
;   [109:12] free scratch register 'r14'
    jne bool_false_109_12
    bool_true_109_12:
    mov r15b, true
    jmp bool_end_109_12
    bool_false_109_12:
    mov r15b, false
    bool_end_109_12:
;   [20:6] assert(expr : bool) 
    assert_109_5:
;       [109:5] alias expr -> r15b  (lea: )
        if_20_29_109_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_109_5:
        test r15b, r15b
        jne if_20_26_109_5_end
        if_20_29_109_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_109_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_109_5_end:
        if_20_26_109_5_end:
;       [109:5] free scratch register 'r15'
    assert_109_5_end:
;   [110:5] assert(arr[2] == 2)
;   [110:12] allocate scratch register -> r15
;   [110:12] ? arr[2] == 2
;   [110:12] ? arr[2] == 2
    cmp_110_12:
;   [110:12] allocate scratch register -> r14
;   [110:16] set array index
;   [110:16] 2
    mov r14, 2
;   [110:16] bounds check
;   [110:16] allocate scratch register -> r13
;   [110:16] line number
    mov r13, 110
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [110:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 16], 2
;   [110:12] free scratch register 'r14'
    jne bool_false_110_12
    bool_true_110_12:
    mov r15b, true
    jmp bool_end_110_12
    bool_false_110_12:
    mov r15b, false
    bool_end_110_12:
;   [20:6] assert(expr : bool) 
    assert_110_5:
;       [110:5] alias expr -> r15b  (lea: )
        if_20_29_110_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_110_5:
        test r15b, r15b
        jne if_20_26_110_5_end
        if_20_29_110_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_110_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_110_5_end:
        if_20_26_110_5_end:
;       [110:5] free scratch register 'r15'
    assert_110_5_end:
;   [112:5] array_copy(arr[2], arr, 2)
;   [112:5] allocate named register 'rsi'
;   [112:5] allocate named register 'rdi'
;   [112:5] allocate named register 'rcx'
;   [112:29] 2
;   [112:29] 2
    mov rcx, 2
;   [112:16] arr[2]
;   [112:16] allocate scratch register -> r15
;   [112:20] set array index
;   [112:20] 2
    mov r15, 2
;   [112:20] bounds check
;   [112:20] allocate scratch register -> r14
;   [112:20] line number
    mov r14, 112
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [112:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [112:20] free scratch register 'r13'
    cmovg rbp, r14
    jg panic_bounds
;   [112:20] free scratch register 'r14'
    lea rsi, [rsp + r15 * 4 - 16]
;   [112:5] free scratch register 'r15'
;   [112:24] arr
;   [112:24] bounds check
;   [112:24] allocate scratch register -> r15
;   [112:24] line number
    mov r15, 112
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [112:24] free scratch register 'r15'
    lea rdi, [rsp - 16]
    shl rcx, 2
    rep movsb
;   [112:5] free named register 'rcx'
;   [112:5] free named register 'rdi'
;   [112:5] free named register 'rsi'
;   [113:5] # copy from, to, number of elements
;   [114:5] assert(arr[0] == 2)
;   [114:12] allocate scratch register -> r15
;   [114:12] ? arr[0] == 2
;   [114:12] ? arr[0] == 2
    cmp_114_12:
;   [114:12] allocate scratch register -> r14
;   [114:16] set array index
;   [114:16] 0
    mov r14, 0
;   [114:16] bounds check
;   [114:16] allocate scratch register -> r13
;   [114:16] line number
    mov r13, 114
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [114:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 16], 2
;   [114:12] free scratch register 'r14'
    jne bool_false_114_12
    bool_true_114_12:
    mov r15b, true
    jmp bool_end_114_12
    bool_false_114_12:
    mov r15b, false
    bool_end_114_12:
;   [20:6] assert(expr : bool) 
    assert_114_5:
;       [114:5] alias expr -> r15b  (lea: )
        if_20_29_114_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_114_5:
        test r15b, r15b
        jne if_20_26_114_5_end
        if_20_29_114_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_114_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_114_5_end:
        if_20_26_114_5_end:
;       [114:5] free scratch register 'r15'
    assert_114_5_end:
;   [116:5] var arr1 : i32[8]
;   [116:9] arr1: i32[8] (4B @ dword [rsp - 56])
;   [116:9] clear 8 * 4B = 32B
;   [116:5] size <= 32B, use mov
    mov qword [rsp - 56], 0
    mov qword [rsp - 48], 0
    mov qword [rsp - 40], 0
    mov qword [rsp - 32], 0
;   [117:5] array_copy(arr, arr1, 4)
;   [117:5] allocate named register 'rsi'
;   [117:5] allocate named register 'rdi'
;   [117:5] allocate named register 'rcx'
;   [117:27] 4
;   [117:27] 4
    mov rcx, 4
;   [117:16] arr
;   [117:16] bounds check
;   [117:16] allocate scratch register -> r15
;   [117:16] line number
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [117:16] free scratch register 'r15'
    lea rsi, [rsp - 16]
;   [117:21] arr1
;   [117:21] bounds check
;   [117:21] allocate scratch register -> r15
;   [117:21] line number
    mov r15, 117
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [117:21] free scratch register 'r15'
    lea rdi, [rsp - 56]
    shl rcx, 2
    rep movsb
;   [117:5] free named register 'rcx'
;   [117:5] free named register 'rdi'
;   [117:5] free named register 'rsi'
;   [118:5] assert(arrays_equal(arr, arr1, 4))
;   [118:12] allocate scratch register -> r15
;   [118:12] ? arrays_equal(arr, arr1, 4)
;   [118:12] ? arrays_equal(arr, arr1, 4)
    cmp_118_12:
;   [118:12] allocate scratch register -> r14
;       [118:12] r14 = arrays_equal(arr, arr1, 4)
;       [118:12] = expression
;       [118:12] arrays_equal(arr, arr1, 4)
;       [118:12] allocate named register 'rsi'
;       [118:12] allocate named register 'rdi'
;       [118:12] allocate named register 'rcx'
;       [118:36] 4
;       [118:36] 4
        mov rcx, 4
;       [118:25] arr
;       [118:25] bounds check
;       [118:25] allocate scratch register -> r13
;       [118:25] line number
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [118:25] free scratch register 'r13'
        lea rsi, [rsp - 16]
;       [118:30] arr1
;       [118:30] bounds check
;       [118:30] allocate scratch register -> r13
;       [118:30] line number
        mov r13, 118
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [118:30] free scratch register 'r13'
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [118:12] free named register 'rcx'
;       [118:12] free named register 'rdi'
;       [118:12] free named register 'rsi'
        je cmps_eq_118_12
        mov r14, false
        jmp cmps_end_118_12
        cmps_eq_118_12:
        mov r14, true
        cmps_end_118_12:
    test r14, r14
;   [118:12] free scratch register 'r14'
    je bool_false_118_12
    bool_true_118_12:
    mov r15b, true
    jmp bool_end_118_12
    bool_false_118_12:
    mov r15b, false
    bool_end_118_12:
;   [20:6] assert(expr : bool) 
    assert_118_5:
;       [118:5] alias expr -> r15b  (lea: )
        if_20_29_118_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_118_5:
        test r15b, r15b
        jne if_20_26_118_5_end
        if_20_29_118_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_118_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_118_5_end:
        if_20_26_118_5_end:
;       [118:5] free scratch register 'r15'
    assert_118_5_end:
;   [119:5] # note: `arrays_equal` is built-in function
;   [121:5] arr1[2] = -1
;   [121:5] allocate scratch register -> r15
;   [121:10] set array index
;   [121:10] 2
    mov r15, 2
;   [121:10] bounds check
;   [121:10] allocate scratch register -> r14
;   [121:10] line number
    mov r14, 121
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [121:10] free scratch register 'r14'
;   [121:16] -1
    mov dword [rsp + r15 * 4 - 56], -1
;   [121:5] free scratch register 'r15'
;   [122:5] assert(not arrays_equal(arr, arr1, 4))
;   [122:12] allocate scratch register -> r15
;   [122:12] ? not arrays_equal(arr, arr1, 4)
;   [122:12] ? not arrays_equal(arr, arr1, 4)
    cmp_122_12:
;   [122:16] allocate scratch register -> r14
;       [122:16] r14 = arrays_equal(arr, arr1, 4)
;       [122:16] = expression
;       [122:16] arrays_equal(arr, arr1, 4)
;       [122:16] allocate named register 'rsi'
;       [122:16] allocate named register 'rdi'
;       [122:16] allocate named register 'rcx'
;       [122:40] 4
;       [122:40] 4
        mov rcx, 4
;       [122:29] arr
;       [122:29] bounds check
;       [122:29] allocate scratch register -> r13
;       [122:29] line number
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [122:29] free scratch register 'r13'
        lea rsi, [rsp - 16]
;       [122:34] arr1
;       [122:34] bounds check
;       [122:34] allocate scratch register -> r13
;       [122:34] line number
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [122:34] free scratch register 'r13'
        lea rdi, [rsp - 56]
        shl rcx, 2
        repe cmpsb
;       [122:16] free named register 'rcx'
;       [122:16] free named register 'rdi'
;       [122:16] free named register 'rsi'
        je cmps_eq_122_16
        mov r14, false
        jmp cmps_end_122_16
        cmps_eq_122_16:
        mov r14, true
        cmps_end_122_16:
    test r14, r14
;   [122:12] free scratch register 'r14'
    jne bool_false_122_12
    bool_true_122_12:
    mov r15b, true
    jmp bool_end_122_12
    bool_false_122_12:
    mov r15b, false
    bool_end_122_12:
;   [20:6] assert(expr : bool) 
    assert_122_5:
;       [122:5] alias expr -> r15b  (lea: )
        if_20_29_122_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_122_5:
        test r15b, r15b
        jne if_20_26_122_5_end
        if_20_29_122_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_122_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_122_5_end:
        if_20_26_122_5_end:
;       [122:5] free scratch register 'r15'
    assert_122_5_end:
;   [124:5] ix = 3
;   [124:10] 3
    mov qword [rsp - 24], 3
;   [125:5] arr[ix] = ~inv(arr[ix - 1])
;   [125:5] allocate scratch register -> r15
;   [125:9] set array index
;   [125:9] ix
    mov r15, qword [rsp - 24]
;   [125:9] bounds check
;   [125:9] allocate scratch register -> r14
;   [125:9] line number
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [125:9] free scratch register 'r14'
;   [125:16] dword [rsp + r15 * 4 - 16] = ~inv(arr[ix - 1])
;   [125:16] = expression
;   [125:16] ~inv(arr[ix - 1])
;   [125:20] allocate scratch register -> r14
;   [125:24] set array index
;   [125:24] ix
    mov r14, qword [rsp - 24]
;   [125:29] r14 - 1
    sub r14, 1
;   [125:24] bounds check
;   [125:24] allocate scratch register -> r13
;   [125:24] line number
    mov r13, 125
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [125:24] free scratch register 'r13'
;   [65:6] inv(i : i32) : i32 res 
    inv_125_16:
;       [125:16] alias res -> dword [rsp + r15 * 4 - 16]  (lea: )
;       [125:16] alias i -> arr  (lea: rsp + r14 * 4 - 16)
;       [66:5] res = ~i
;       [66:12] ~i
;       [66:12] allocate scratch register -> r13
        mov r13d, dword [rsp + r14 * 4 - 16]
        mov dword [rsp + r15 * 4 - 16], r13d
;       [66:12] free scratch register 'r13'
        not dword [rsp + r15 * 4 - 16]
;       [125:16] free scratch register 'r14'
    inv_125_16_end:
    not dword [rsp + r15 * 4 - 16]
;   [125:5] free scratch register 'r15'
;   [126:5] assert(arr[ix] == 2)
;   [126:12] allocate scratch register -> r15
;   [126:12] ? arr[ix] == 2
;   [126:12] ? arr[ix] == 2
    cmp_126_12:
;   [126:12] allocate scratch register -> r14
;   [126:16] set array index
;   [126:16] ix
    mov r14, qword [rsp - 24]
;   [126:16] bounds check
;   [126:16] allocate scratch register -> r13
;   [126:16] line number
    mov r13, 126
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [126:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 16], 2
;   [126:12] free scratch register 'r14'
    jne bool_false_126_12
    bool_true_126_12:
    mov r15b, true
    jmp bool_end_126_12
    bool_false_126_12:
    mov r15b, false
    bool_end_126_12:
;   [20:6] assert(expr : bool) 
    assert_126_5:
;       [126:5] alias expr -> r15b  (lea: )
        if_20_29_126_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_126_5:
        test r15b, r15b
        jne if_20_26_126_5_end
        if_20_29_126_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_126_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_126_5_end:
        if_20_26_126_5_end:
;       [126:5] free scratch register 'r15'
    assert_126_5_end:
;   [128:5] faz(arr)
;   [75:6] faz(arg : i32[]) 
    faz_128_5:
;       [128:5] alias arg -> arr  (lea: )
;       [76:5] arg[1] = 0xfe
;       [76:5] allocate scratch register -> r15
;       [76:9] set array index
;       [76:9] 1
        mov r15, 1
;       [76:9] bounds check
;       [76:9] allocate scratch register -> r14
;       [76:9] line number
        mov r14, 76
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
;       [76:9] free scratch register 'r14'
;       [76:14] 0xfe
        mov dword [rsp + r15 * 4 - 16], 0xfe
;       [76:5] free scratch register 'r15'
    faz_128_5_end:
;   [129:5] assert(arr[1] == 0xfe)
;   [129:12] allocate scratch register -> r15
;   [129:12] ? arr[1] == 0xfe
;   [129:12] ? arr[1] == 0xfe
    cmp_129_12:
;   [129:12] allocate scratch register -> r14
;   [129:16] set array index
;   [129:16] 1
    mov r14, 1
;   [129:16] bounds check
;   [129:16] allocate scratch register -> r13
;   [129:16] line number
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [129:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 16], 0xfe
;   [129:12] free scratch register 'r14'
    jne bool_false_129_12
    bool_true_129_12:
    mov r15b, true
    jmp bool_end_129_12
    bool_false_129_12:
    mov r15b, false
    bool_end_129_12:
;   [20:6] assert(expr : bool) 
    assert_129_5:
;       [129:5] alias expr -> r15b  (lea: )
        if_20_29_129_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_129_5:
        test r15b, r15b
        jne if_20_26_129_5_end
        if_20_29_129_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_129_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_129_5_end:
        if_20_26_129_5_end:
;       [129:5] free scratch register 'r15'
    assert_129_5_end:
;   [131:5] var p : point = {0, 0}
;   [131:9] p: point (16B @ qword [rsp - 72])
;   [131:9] p = {0, 0}
;   [131:21] copy field 'x'
    mov qword [rsp - 72], 0
;   [131:21] copy field 'y'
    mov qword [rsp - 64], 0
;   [132:5] foo(p)
;   [51:6] foo(pt : point) 
    foo_132_5:
;       [132:5] alias pt -> p  (lea: )
;       [52:5] pt.x = 0b10
;       [52:12] 0b10
        mov qword [rsp - 72], 0b10
;       [52:20] # binary value 2
;       [53:5] pt.y = 0xb
;       [53:12] 0xb
        mov qword [rsp - 64], 0xb
;       [53:20] # hex value 11
    foo_132_5_end:
;   [133:5] assert(p.x == 2)
;   [133:12] allocate scratch register -> r15
;   [133:12] ? p.x == 2
;   [133:12] ? p.x == 2
    cmp_133_12:
    cmp qword [rsp - 72], 2
    jne bool_false_133_12
    bool_true_133_12:
    mov r15b, true
    jmp bool_end_133_12
    bool_false_133_12:
    mov r15b, false
    bool_end_133_12:
;   [20:6] assert(expr : bool) 
    assert_133_5:
;       [133:5] alias expr -> r15b  (lea: )
        if_20_29_133_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_133_5:
        test r15b, r15b
        jne if_20_26_133_5_end
        if_20_29_133_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_133_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_133_5_end:
        if_20_26_133_5_end:
;       [133:5] free scratch register 'r15'
    assert_133_5_end:
;   [134:5] assert(p.y == 0xb)
;   [134:12] allocate scratch register -> r15
;   [134:12] ? p.y == 0xb
;   [134:12] ? p.y == 0xb
    cmp_134_12:
    cmp qword [rsp - 64], 0xb
    jne bool_false_134_12
    bool_true_134_12:
    mov r15b, true
    jmp bool_end_134_12
    bool_false_134_12:
    mov r15b, false
    bool_end_134_12:
;   [20:6] assert(expr : bool) 
    assert_134_5:
;       [134:5] alias expr -> r15b  (lea: )
        if_20_29_134_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_134_5:
        test r15b, r15b
        jne if_20_26_134_5_end
        if_20_29_134_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_134_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_134_5_end:
        if_20_26_134_5_end:
;       [134:5] free scratch register 'r15'
    assert_134_5_end:
;   [136:5] var q : point = p
;   [136:9] q: point (16B @ qword [rsp - 88])
;   [136:9] q = p
;   [136:21] allocate named register 'rsi'
;   [136:21] allocate named register 'rdi'
;   [136:21] allocate named register 'rcx'
    lea rsi, [rsp - 72]
    lea rdi, [rsp - 88]
    mov rcx, 2
    rep movsq
;   [136:21] free named register 'rcx'
;   [136:21] free named register 'rdi'
;   [136:21] free named register 'rsi'
;   [137:5] assert(equal(p, q))
;   [137:12] allocate scratch register -> r15
;   [137:12] ? equal(p, q)
;   [137:12] ? equal(p, q)
    cmp_137_12:
;   [137:12] allocate scratch register -> r14
;       [137:12] r14 = equal(p, q)
;       [137:12] = expression
;       [137:12] equal(p, q)
;       [137:12] allocate named register 'rsi'
;       [137:12] allocate named register 'rdi'
;       [137:12] allocate named register 'rcx'
;       [137:18] p
        lea rsi, [rsp - 72]
;       [137:21] q
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
;       [137:12] free named register 'rcx'
;       [137:12] free named register 'rdi'
;       [137:12] free named register 'rsi'
        je cmps_eq_137_12
        mov r14, false
        jmp cmps_end_137_12
        cmps_eq_137_12:
        mov r14, true
        cmps_end_137_12:
    test r14, r14
;   [137:12] free scratch register 'r14'
    je bool_false_137_12
    bool_true_137_12:
    mov r15b, true
    jmp bool_end_137_12
    bool_false_137_12:
    mov r15b, false
    bool_end_137_12:
;   [20:6] assert(expr : bool) 
    assert_137_5:
;       [137:5] alias expr -> r15b  (lea: )
        if_20_29_137_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_137_5:
        test r15b, r15b
        jne if_20_26_137_5_end
        if_20_29_137_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_137_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_137_5_end:
        if_20_26_137_5_end:
;       [137:5] free scratch register 'r15'
    assert_137_5_end:
;   [138:5] # use `equal` to compare for equality
;   [140:5] q.x = 3
;   [140:11] 3
    mov qword [rsp - 88], 3
;   [141:5] assert(not equal(p, q))
;   [141:12] allocate scratch register -> r15
;   [141:12] ? not equal(p, q)
;   [141:12] ? not equal(p, q)
    cmp_141_12:
;   [141:16] allocate scratch register -> r14
;       [141:16] r14 = equal(p, q)
;       [141:16] = expression
;       [141:16] equal(p, q)
;       [141:16] allocate named register 'rsi'
;       [141:16] allocate named register 'rdi'
;       [141:16] allocate named register 'rcx'
;       [141:22] p
        lea rsi, [rsp - 72]
;       [141:25] q
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
;       [141:16] free named register 'rcx'
;       [141:16] free named register 'rdi'
;       [141:16] free named register 'rsi'
        je cmps_eq_141_16
        mov r14, false
        jmp cmps_end_141_16
        cmps_eq_141_16:
        mov r14, true
        cmps_end_141_16:
    test r14, r14
;   [141:12] free scratch register 'r14'
    jne bool_false_141_12
    bool_true_141_12:
    mov r15b, true
    jmp bool_end_141_12
    bool_false_141_12:
    mov r15b, false
    bool_end_141_12:
;   [20:6] assert(expr : bool) 
    assert_141_5:
;       [141:5] alias expr -> r15b  (lea: )
        if_20_29_141_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_141_5:
        test r15b, r15b
        jne if_20_26_141_5_end
        if_20_29_141_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_141_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_141_5_end:
        if_20_26_141_5_end:
;       [141:5] free scratch register 'r15'
    assert_141_5_end:
;   [143:5] var i = 0
;   [143:9] i: i64 (8B @ qword [rsp - 96])
;   [143:9] i = 0
;   [143:13] 0
    mov qword [rsp - 96], 0
;   [144:5] bar(i)
;   [58:6] bar(arg) 
    bar_144_5:
;       [144:5] alias arg -> i  (lea: )
        if_59_8_144_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_144_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_144_5_end
        if_59_8_144_5_code:
;           [59:17] return
            jmp bar_144_5_end
        if_59_5_144_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
        mov qword [rsp - 96], 0xff
    bar_144_5_end:
;   [145:5] assert(i == 0)
;   [145:12] allocate scratch register -> r15
;   [145:12] ? i == 0
;   [145:12] ? i == 0
    cmp_145_12:
    cmp qword [rsp - 96], 0
    jne bool_false_145_12
    bool_true_145_12:
    mov r15b, true
    jmp bool_end_145_12
    bool_false_145_12:
    mov r15b, false
    bool_end_145_12:
;   [20:6] assert(expr : bool) 
    assert_145_5:
;       [145:5] alias expr -> r15b  (lea: )
        if_20_29_145_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_145_5:
        test r15b, r15b
        jne if_20_26_145_5_end
        if_20_29_145_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_145_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_145_5_end:
        if_20_26_145_5_end:
;       [145:5] free scratch register 'r15'
    assert_145_5_end:
;   [147:5] i = 1
;   [147:9] 1
    mov qword [rsp - 96], 1
;   [148:5] bar(i)
;   [58:6] bar(arg) 
    bar_148_5:
;       [148:5] alias arg -> i  (lea: )
        if_59_8_148_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_148_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_148_5_end
        if_59_8_148_5_code:
;           [59:17] return
            jmp bar_148_5_end
        if_59_5_148_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
        mov qword [rsp - 96], 0xff
    bar_148_5_end:
;   [149:5] assert(i == 0xff)
;   [149:12] allocate scratch register -> r15
;   [149:12] ? i == 0xff
;   [149:12] ? i == 0xff
    cmp_149_12:
    cmp qword [rsp - 96], 0xff
    jne bool_false_149_12
    bool_true_149_12:
    mov r15b, true
    jmp bool_end_149_12
    bool_false_149_12:
    mov r15b, false
    bool_end_149_12:
;   [20:6] assert(expr : bool) 
    assert_149_5:
;       [149:5] alias expr -> r15b  (lea: )
        if_20_29_149_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_149_5:
        test r15b, r15b
        jne if_20_26_149_5_end
        if_20_29_149_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_149_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_149_5_end:
        if_20_26_149_5_end:
;       [149:5] free scratch register 'r15'
    assert_149_5_end:
;   [151:5] var j = 1
;   [151:9] j: i64 (8B @ qword [rsp - 104])
;   [151:9] j = 1
;   [151:13] 1
    mov qword [rsp - 104], 1
;   [152:5] var k = baz(j)
;   [152:9] k: i64 (8B @ qword [rsp - 112])
;   [152:9] k = baz(j)
;   [152:13] allocate scratch register -> r15
;   [152:13] r15 = baz(j)
;   [152:13] = expression
;   [152:13] baz(j)
;   [69:6] baz(arg) : i64 res 
    baz_152_13:
;       [152:13] alias res -> r15  (lea: )
;       [152:13] alias arg -> j  (lea: )
;       [70:5] res = arg * 2
;       [70:11] arg
        mov r15, qword [rsp - 104]
;       [70:17] res * 2
;       [70:17] dst is reg, src is const
        imul r15, 2
    baz_152_13_end:
    mov qword [rsp - 112], r15
;   [152:13] free scratch register 'r15'
;   [153:5] assert(k == 2)
;   [153:12] allocate scratch register -> r15
;   [153:12] ? k == 2
;   [153:12] ? k == 2
    cmp_153_12:
    cmp qword [rsp - 112], 2
    jne bool_false_153_12
    bool_true_153_12:
    mov r15b, true
    jmp bool_end_153_12
    bool_false_153_12:
    mov r15b, false
    bool_end_153_12:
;   [20:6] assert(expr : bool) 
    assert_153_5:
;       [153:5] alias expr -> r15b  (lea: )
        if_20_29_153_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_153_5:
        test r15b, r15b
        jne if_20_26_153_5_end
        if_20_29_153_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_153_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_153_5_end:
        if_20_26_153_5_end:
;       [153:5] free scratch register 'r15'
    assert_153_5_end:
;   [155:5] k = baz(1)
;   [155:9] allocate scratch register -> r15
;   [155:9] r15 = baz(1)
;   [155:9] = expression
;   [155:9] baz(1)
;   [69:6] baz(arg) : i64 res 
    baz_155_9:
;       [155:9] alias res -> r15  (lea: )
;       [155:9] alias arg -> 1  (lea: )
;       [70:5] res = arg * 2
;       [70:11] arg
        mov r15, 1
;       [70:17] res * 2
;       [70:17] dst is reg, src is const
        imul r15, 2
    baz_155_9_end:
    mov qword [rsp - 112], r15
;   [155:9] free scratch register 'r15'
;   [156:5] assert(k == 2)
;   [156:12] allocate scratch register -> r15
;   [156:12] ? k == 2
;   [156:12] ? k == 2
    cmp_156_12:
    cmp qword [rsp - 112], 2
    jne bool_false_156_12
    bool_true_156_12:
    mov r15b, true
    jmp bool_end_156_12
    bool_false_156_12:
    mov r15b, false
    bool_end_156_12:
;   [20:6] assert(expr : bool) 
    assert_156_5:
;       [156:5] alias expr -> r15b  (lea: )
        if_20_29_156_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_156_5:
        test r15b, r15b
        jne if_20_26_156_5_end
        if_20_29_156_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_156_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_156_5_end:
        if_20_26_156_5_end:
;       [156:5] free scratch register 'r15'
    assert_156_5_end:
;   [158:5] var p0 : point = {baz(2), 0}
;   [158:9] p0: point (16B @ qword [rsp - 128])
;   [158:9] p0 = {baz(2), 0}
;   [158:22] copy field 'x'
;   [158:23] qword [rsp - 128] = baz(2)
;   [158:23] = expression
;   [158:23] baz(2)
;   [69:6] baz(arg) : i64 res 
    baz_158_23:
;       [158:23] alias res -> qword [rsp - 128]  (lea: )
;       [158:23] alias arg -> 2  (lea: )
;       [70:5] res = arg * 2
;       [70:11] arg
        mov qword [rsp - 128], 2
;       [70:17] res * 2
;       [70:17] dst is not reg, src is const
;       [70:17] allocate scratch register -> r15
        mov r15, qword [rsp - 128]
        imul r15, 2
        mov qword [rsp - 128], r15
;       [70:17] free scratch register 'r15'
    baz_158_23_end:
;   [158:22] copy field 'y'
    mov qword [rsp - 120], 0
;   [159:5] assert(p0.x == 4)
;   [159:12] allocate scratch register -> r15
;   [159:12] ? p0.x == 4
;   [159:12] ? p0.x == 4
    cmp_159_12:
    cmp qword [rsp - 128], 4
    jne bool_false_159_12
    bool_true_159_12:
    mov r15b, true
    jmp bool_end_159_12
    bool_false_159_12:
    mov r15b, false
    bool_end_159_12:
;   [20:6] assert(expr : bool) 
    assert_159_5:
;       [159:5] alias expr -> r15b  (lea: )
        if_20_29_159_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_159_5:
        test r15b, r15b
        jne if_20_26_159_5_end
        if_20_29_159_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_159_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_159_5_end:
        if_20_26_159_5_end:
;       [159:5] free scratch register 'r15'
    assert_159_5_end:
;   [161:5] var x = 1
;   [161:9] x: i64 (8B @ qword [rsp - 136])
;   [161:9] x = 1
;   [161:13] 1
    mov qword [rsp - 136], 1
;   [162:5] var y = 2
;   [162:9] y: i64 (8B @ qword [rsp - 144])
;   [162:9] y = 2
;   [162:13] 2
    mov qword [rsp - 144], 2
;   [164:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [164:9] o1: object (20B @ qword [rsp - 164])
;   [164:9] o1 = {{x * 10, y}, 0xff0000}
;   [164:23] copy field 'pos'
;   [164:24] copy field 'x'
;   [164:25] x
;   [164:25] allocate scratch register -> r15
    mov r15, qword [rsp - 136]
    mov qword [rsp - 164], r15
;   [164:25] free scratch register 'r15'
;   [164:29] qword [rsp - 164] * 10
;   [164:29] dst is not reg, src is const
;   [164:29] allocate scratch register -> r15
    mov r15, qword [rsp - 164]
    imul r15, 10
    mov qword [rsp - 164], r15
;   [164:29] free scratch register 'r15'
;   [164:24] copy field 'y'
;   [164:33] allocate scratch register -> r15
    mov r15, qword [rsp - 144]
    mov qword [rsp - 156], r15
;   [164:33] free scratch register 'r15'
;   [164:23] copy field 'color'
    mov dword [rsp - 148], 0xff0000
;   [165:5] assert(o1.pos.x == 10)
;   [165:12] allocate scratch register -> r15
;   [165:12] ? o1.pos.x == 10
;   [165:12] ? o1.pos.x == 10
    cmp_165_12:
    cmp qword [rsp - 164], 10
    jne bool_false_165_12
    bool_true_165_12:
    mov r15b, true
    jmp bool_end_165_12
    bool_false_165_12:
    mov r15b, false
    bool_end_165_12:
;   [20:6] assert(expr : bool) 
    assert_165_5:
;       [165:5] alias expr -> r15b  (lea: )
        if_20_29_165_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_165_5:
        test r15b, r15b
        jne if_20_26_165_5_end
        if_20_29_165_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_165_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_165_5_end:
        if_20_26_165_5_end:
;       [165:5] free scratch register 'r15'
    assert_165_5_end:
;   [166:5] assert(o1.pos.y == 2)
;   [166:12] allocate scratch register -> r15
;   [166:12] ? o1.pos.y == 2
;   [166:12] ? o1.pos.y == 2
    cmp_166_12:
    cmp qword [rsp - 156], 2
    jne bool_false_166_12
    bool_true_166_12:
    mov r15b, true
    jmp bool_end_166_12
    bool_false_166_12:
    mov r15b, false
    bool_end_166_12:
;   [20:6] assert(expr : bool) 
    assert_166_5:
;       [166:5] alias expr -> r15b  (lea: )
        if_20_29_166_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_166_5:
        test r15b, r15b
        jne if_20_26_166_5_end
        if_20_29_166_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_166_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_166_5_end:
        if_20_26_166_5_end:
;       [166:5] free scratch register 'r15'
    assert_166_5_end:
;   [167:5] assert(o1.color == 0xff0000)
;   [167:12] allocate scratch register -> r15
;   [167:12] ? o1.color == 0xff0000
;   [167:12] ? o1.color == 0xff0000
    cmp_167_12:
    cmp dword [rsp - 148], 0xff0000
    jne bool_false_167_12
    bool_true_167_12:
    mov r15b, true
    jmp bool_end_167_12
    bool_false_167_12:
    mov r15b, false
    bool_end_167_12:
;   [20:6] assert(expr : bool) 
    assert_167_5:
;       [167:5] alias expr -> r15b  (lea: )
        if_20_29_167_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_167_5:
        test r15b, r15b
        jne if_20_26_167_5_end
        if_20_29_167_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_167_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_167_5_end:
        if_20_26_167_5_end:
;       [167:5] free scratch register 'r15'
    assert_167_5_end:
;   [169:5] var p1 : point = {-x, -y}
;   [169:9] p1: point (16B @ qword [rsp - 180])
;   [169:9] p1 = {-x, -y}
;   [169:22] copy field 'x'
;   [169:23] allocate scratch register -> r15
    mov r15, qword [rsp - 136]
    mov qword [rsp - 180], r15
;   [169:23] free scratch register 'r15'
    neg qword [rsp - 180]
;   [169:22] copy field 'y'
;   [169:27] allocate scratch register -> r15
    mov r15, qword [rsp - 144]
    mov qword [rsp - 172], r15
;   [169:27] free scratch register 'r15'
    neg qword [rsp - 172]
;   [170:5] o1.pos = p1
;   [170:14] allocate named register 'rsi'
;   [170:14] allocate named register 'rdi'
;   [170:14] allocate named register 'rcx'
    lea rsi, [rsp - 180]
    lea rdi, [rsp - 164]
    mov rcx, 2
    rep movsq
;   [170:14] free named register 'rcx'
;   [170:14] free named register 'rdi'
;   [170:14] free named register 'rsi'
;   [171:5] assert(o1.pos.x == -1)
;   [171:12] allocate scratch register -> r15
;   [171:12] ? o1.pos.x == -1
;   [171:12] ? o1.pos.x == -1
    cmp_171_12:
    cmp qword [rsp - 164], -1
    jne bool_false_171_12
    bool_true_171_12:
    mov r15b, true
    jmp bool_end_171_12
    bool_false_171_12:
    mov r15b, false
    bool_end_171_12:
;   [20:6] assert(expr : bool) 
    assert_171_5:
;       [171:5] alias expr -> r15b  (lea: )
        if_20_29_171_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_171_5:
        test r15b, r15b
        jne if_20_26_171_5_end
        if_20_29_171_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_171_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_171_5_end:
        if_20_26_171_5_end:
;       [171:5] free scratch register 'r15'
    assert_171_5_end:
;   [172:5] assert(o1.pos.y == -2)
;   [172:12] allocate scratch register -> r15
;   [172:12] ? o1.pos.y == -2
;   [172:12] ? o1.pos.y == -2
    cmp_172_12:
    cmp qword [rsp - 156], -2
    jne bool_false_172_12
    bool_true_172_12:
    mov r15b, true
    jmp bool_end_172_12
    bool_false_172_12:
    mov r15b, false
    bool_end_172_12:
;   [20:6] assert(expr : bool) 
    assert_172_5:
;       [172:5] alias expr -> r15b  (lea: )
        if_20_29_172_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_172_5:
        test r15b, r15b
        jne if_20_26_172_5_end
        if_20_29_172_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_172_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_172_5_end:
        if_20_26_172_5_end:
;       [172:5] free scratch register 'r15'
    assert_172_5_end:
;   [174:5] var o2 : object = o1
;   [174:9] o2: object (20B @ qword [rsp - 200])
;   [174:9] o2 = o1
;   [174:23] allocate named register 'rsi'
;   [174:23] allocate named register 'rdi'
;   [174:23] allocate named register 'rcx'
    lea rsi, [rsp - 164]
    lea rdi, [rsp - 200]
    mov rcx, 20
    rep movsb
;   [174:23] free named register 'rcx'
;   [174:23] free named register 'rdi'
;   [174:23] free named register 'rsi'
;   [175:5] assert(o2.pos.x == -1)
;   [175:12] allocate scratch register -> r15
;   [175:12] ? o2.pos.x == -1
;   [175:12] ? o2.pos.x == -1
    cmp_175_12:
    cmp qword [rsp - 200], -1
    jne bool_false_175_12
    bool_true_175_12:
    mov r15b, true
    jmp bool_end_175_12
    bool_false_175_12:
    mov r15b, false
    bool_end_175_12:
;   [20:6] assert(expr : bool) 
    assert_175_5:
;       [175:5] alias expr -> r15b  (lea: )
        if_20_29_175_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_175_5:
        test r15b, r15b
        jne if_20_26_175_5_end
        if_20_29_175_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_175_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_175_5_end:
        if_20_26_175_5_end:
;       [175:5] free scratch register 'r15'
    assert_175_5_end:
;   [176:5] assert(o2.pos.y == -2)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? o2.pos.y == -2
;   [176:12] ? o2.pos.y == -2
    cmp_176_12:
    cmp qword [rsp - 192], -2
    jne bool_false_176_12
    bool_true_176_12:
    mov r15b, true
    jmp bool_end_176_12
    bool_false_176_12:
    mov r15b, false
    bool_end_176_12:
;   [20:6] assert(expr : bool) 
    assert_176_5:
;       [176:5] alias expr -> r15b  (lea: )
        if_20_29_176_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_176_5:
        test r15b, r15b
        jne if_20_26_176_5_end
        if_20_29_176_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_176_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_176_5_end:
        if_20_26_176_5_end:
;       [176:5] free scratch register 'r15'
    assert_176_5_end:
;   [177:5] assert(o2.color == 0xff0000)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? o2.color == 0xff0000
;   [177:12] ? o2.color == 0xff0000
    cmp_177_12:
    cmp dword [rsp - 184], 0xff0000
    jne bool_false_177_12
    bool_true_177_12:
    mov r15b, true
    jmp bool_end_177_12
    bool_false_177_12:
    mov r15b, false
    bool_end_177_12:
;   [20:6] assert(expr : bool) 
    assert_177_5:
;       [177:5] alias expr -> r15b  (lea: )
        if_20_29_177_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_177_5:
        test r15b, r15b
        jne if_20_26_177_5_end
        if_20_29_177_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_177_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_177_5_end:
        if_20_26_177_5_end:
;       [177:5] free scratch register 'r15'
    assert_177_5_end:
;   [179:5] var o3 : object[1]
;   [179:9] o3: object[1] (20B @ qword [rsp - 220])
;   [179:9] clear 1 * 20B = 20B
;   [179:5] size <= 32B, use mov
    mov qword [rsp - 220], 0
    mov qword [rsp - 212], 0
    mov dword [rsp - 204], 0
;   [180:5] # index 0 in an array can be accessed without array index
;   [181:5] o3.pos.y = 73
;   [181:16] 73
    mov qword [rsp - 212], 73
;   [182:5] assert(o3[0].pos.y == 73)
;   [182:12] allocate scratch register -> r15
;   [182:12] ? o3[0].pos.y == 73
;   [182:12] ? o3[0].pos.y == 73
    cmp_182_12:
;   [182:12] allocate scratch register -> r14
    lea r14, [rsp - 220]
;   [182:12] allocate scratch register -> r13
;   [182:15] set array index
;   [182:15] 0
    mov r13, 0
;   [182:15] bounds check
;   [182:15] allocate scratch register -> r12
;   [182:15] line number
    mov r12, 182
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
;   [182:15] free scratch register 'r12'
    imul r13, 20
    add r14, r13
;   [182:12] free scratch register 'r13'
    cmp qword [r14 + 8], 73
;   [182:12] free scratch register 'r14'
    jne bool_false_182_12
    bool_true_182_12:
    mov r15b, true
    jmp bool_end_182_12
    bool_false_182_12:
    mov r15b, false
    bool_end_182_12:
;   [20:6] assert(expr : bool) 
    assert_182_5:
;       [182:5] alias expr -> r15b  (lea: )
        if_20_29_182_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_182_5:
        test r15b, r15b
        jne if_20_26_182_5_end
        if_20_29_182_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_182_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_182_5_end:
        if_20_26_182_5_end:
;       [182:5] free scratch register 'r15'
    assert_182_5_end:
;   [184:5] var worlds : world[8]
;   [184:9] worlds: world[8] (64B @ qword [rsp - 732])
;   [184:9] clear 8 * 64B = 512B
;   [184:5] allocate named register 'rcx'
;   [184:5] allocate named register 'rdi'
;   [184:5] allocate named register 'rax'
    mov rcx, 64
    lea rdi, [rsp - 732]
    xor rax, rax
    rep stosq
;   [184:5] free named register 'rax'
;   [184:5] free named register 'rdi'
;   [184:5] free named register 'rcx'
;   [185:5] worlds[1].locations[1] = 0xffee
;   [185:5] allocate scratch register -> r15
    lea r15, [rsp - 732]
;   [185:5] allocate scratch register -> r14
;   [185:12] set array index
;   [185:12] 1
    mov r14, 1
;   [185:12] bounds check
;   [185:12] allocate scratch register -> r13
;   [185:12] line number
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [185:12] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [185:5] free scratch register 'r14'
;   [185:5] allocate scratch register -> r14
;   [185:25] set array index
;   [185:25] 1
    mov r14, 1
;   [185:25] bounds check
;   [185:25] allocate scratch register -> r13
;   [185:25] line number
    mov r13, 185
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [185:25] free scratch register 'r13'
;   [185:30] 0xffee
    mov qword [r15 + r14 * 8], 0xffee
;   [185:5] free scratch register 'r14'
;   [185:5] free scratch register 'r15'
;   [186:5] assert(worlds[1].locations[1] == 0xffee)
;   [186:12] allocate scratch register -> r15
;   [186:12] ? worlds[1].locations[1] == 0xffee
;   [186:12] ? worlds[1].locations[1] == 0xffee
    cmp_186_12:
;   [186:12] allocate scratch register -> r14
    lea r14, [rsp - 732]
;   [186:12] allocate scratch register -> r13
;   [186:19] set array index
;   [186:19] 1
    mov r13, 1
;   [186:19] bounds check
;   [186:19] allocate scratch register -> r12
;   [186:19] line number
    mov r12, 186
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [186:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [186:12] free scratch register 'r13'
;   [186:12] allocate scratch register -> r13
;   [186:32] set array index
;   [186:32] 1
    mov r13, 1
;   [186:32] bounds check
;   [186:32] allocate scratch register -> r12
;   [186:32] line number
    mov r12, 186
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [186:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 0xffee
;   [186:12] free scratch register 'r13'
;   [186:12] free scratch register 'r14'
    jne bool_false_186_12
    bool_true_186_12:
    mov r15b, true
    jmp bool_end_186_12
    bool_false_186_12:
    mov r15b, false
    bool_end_186_12:
;   [20:6] assert(expr : bool) 
    assert_186_5:
;       [186:5] alias expr -> r15b  (lea: )
        if_20_29_186_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_186_5:
        test r15b, r15b
        jne if_20_26_186_5_end
        if_20_29_186_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_186_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_186_5_end:
        if_20_26_186_5_end:
;       [186:5] free scratch register 'r15'
    assert_186_5_end:
;   [188:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [188:5] allocate named register 'rsi'
;   [188:5] allocate named register 'rdi'
;   [188:5] allocate named register 'rcx'
;   [190:9] array_size_of(worlds.locations)
;   [191:9] rcx = array_size_of(worlds.locations)
;   [191:9] = expression
;   [191:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [189:9] worlds[1].locations
;   [189:9] allocate scratch register -> r15
    lea r15, [rsp - 732]
;   [189:9] allocate scratch register -> r14
;   [189:16] set array index
;   [189:16] 1
    mov r14, 1
;   [189:16] bounds check
;   [189:16] allocate scratch register -> r13
;   [189:16] line number
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [189:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [189:9] free scratch register 'r14'
;   [189:9] bounds check
;   [189:9] allocate scratch register -> r14
;   [189:9] line number
    mov r14, 189
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [189:9] free scratch register 'r14'
    lea rsi, [r15]
;   [188:5] free scratch register 'r15'
;   [190:9] worlds[0].locations
;   [190:9] allocate scratch register -> r15
    lea r15, [rsp - 732]
;   [190:9] allocate scratch register -> r14
;   [190:16] set array index
;   [190:16] 0
    mov r14, 0
;   [190:16] bounds check
;   [190:16] allocate scratch register -> r13
;   [190:16] line number
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [190:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [190:9] free scratch register 'r14'
;   [190:9] bounds check
;   [190:9] allocate scratch register -> r14
;   [190:9] line number
    mov r14, 190
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [190:9] free scratch register 'r14'
    lea rdi, [r15]
;   [188:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [188:5] free named register 'rcx'
;   [188:5] free named register 'rdi'
;   [188:5] free named register 'rsi'
;   [193:5] # note: `array_copy` is built-in and can use indexed positions
;   [194:5] # `array_size_of` is built-in
;   [195:5] assert(worlds[0].locations[1] == 0xffee)
;   [195:12] allocate scratch register -> r15
;   [195:12] ? worlds[0].locations[1] == 0xffee
;   [195:12] ? worlds[0].locations[1] == 0xffee
    cmp_195_12:
;   [195:12] allocate scratch register -> r14
    lea r14, [rsp - 732]
;   [195:12] allocate scratch register -> r13
;   [195:19] set array index
;   [195:19] 0
    mov r13, 0
;   [195:19] bounds check
;   [195:19] allocate scratch register -> r12
;   [195:19] line number
    mov r12, 195
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [195:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [195:12] free scratch register 'r13'
;   [195:12] allocate scratch register -> r13
;   [195:32] set array index
;   [195:32] 1
    mov r13, 1
;   [195:32] bounds check
;   [195:32] allocate scratch register -> r12
;   [195:32] line number
    mov r12, 195
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [195:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 0xffee
;   [195:12] free scratch register 'r13'
;   [195:12] free scratch register 'r14'
    jne bool_false_195_12
    bool_true_195_12:
    mov r15b, true
    jmp bool_end_195_12
    bool_false_195_12:
    mov r15b, false
    bool_end_195_12:
;   [20:6] assert(expr : bool) 
    assert_195_5:
;       [195:5] alias expr -> r15b  (lea: )
        if_20_29_195_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_195_5:
        test r15b, r15b
        jne if_20_26_195_5_end
        if_20_29_195_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_195_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_195_5_end:
        if_20_26_195_5_end:
;       [195:5] free scratch register 'r15'
    assert_195_5_end:
;   [196:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [196:12] allocate scratch register -> r15
;   [196:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [196:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_196_12:
;   [196:12] allocate scratch register -> r14
;       [196:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [196:12] = expression
;       [196:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [196:12] allocate named register 'rsi'
;       [196:12] allocate named register 'rdi'
;       [196:12] allocate named register 'rcx'
;       [198:14] array_size_of(worlds.locations)
;       [199:14] rcx = array_size_of(worlds.locations)
;       [199:14] = expression
;       [199:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [197:14] worlds[0].locations
;       [197:14] allocate scratch register -> r13
        lea r13, [rsp - 732]
;       [197:14] allocate scratch register -> r12
;       [197:21] set array index
;       [197:21] 0
        mov r12, 0
;       [197:21] bounds check
;       [197:21] allocate scratch register -> r11
;       [197:21] line number
        mov r11, 197
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [197:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [197:14] free scratch register 'r12'
;       [197:14] bounds check
;       [197:14] allocate scratch register -> r12
;       [197:14] line number
        mov r12, 197
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [197:14] free scratch register 'r12'
        lea rsi, [r13]
;       [196:12] free scratch register 'r13'
;       [198:14] worlds[1].locations
;       [198:14] allocate scratch register -> r13
        lea r13, [rsp - 732]
;       [198:14] allocate scratch register -> r12
;       [198:21] set array index
;       [198:21] 1
        mov r12, 1
;       [198:21] bounds check
;       [198:21] allocate scratch register -> r11
;       [198:21] line number
        mov r11, 198
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [198:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [198:14] free scratch register 'r12'
;       [198:14] bounds check
;       [198:14] allocate scratch register -> r12
;       [198:14] line number
        mov r12, 198
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [198:14] free scratch register 'r12'
        lea rdi, [r13]
;       [196:12] free scratch register 'r13'
        shl rcx, 3
        repe cmpsb
;       [196:12] free named register 'rcx'
;       [196:12] free named register 'rdi'
;       [196:12] free named register 'rsi'
        je cmps_eq_196_12
        mov r14, false
        jmp cmps_end_196_12
        cmps_eq_196_12:
        mov r14, true
        cmps_end_196_12:
    test r14, r14
;   [196:12] free scratch register 'r14'
    je bool_false_196_12
    bool_true_196_12:
    mov r15b, true
    jmp bool_end_196_12
    bool_false_196_12:
    mov r15b, false
    bool_end_196_12:
;   [20:6] assert(expr : bool) 
    assert_196_5:
;       [196:5] alias expr -> r15b  (lea: )
        if_20_29_196_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_196_5:
        test r15b, r15b
        jne if_20_26_196_5_end
        if_20_29_196_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_196_5:
;               [20:38] alias v -> rdi  (lea: )
;               [13:5] mov(rax, 60)
;                   [13:14] 60
                    mov rax, 60
;               [13:19] # exit system call
;               [14:5] mov(rdi, v)
;                   [14:14] v
;               [14:19] # return code
;               [15:5] syscall()
                syscall
;               [20:38] free named register 'rdi'
            exit_20_38_196_5_end:
        if_20_26_196_5_end:
;       [196:5] free scratch register 'r15'
    assert_196_5_end:
;   [202:5] var nm : str
;   [202:9] nm: str (128B @ byte [rsp - 860])
;   [202:9] clear 1 * 128B = 128B
;   [202:5] allocate named register 'rcx'
;   [202:5] allocate named register 'rdi'
;   [202:5] allocate named register 'rax'
    mov rcx, 16
    lea rdi, [rsp - 860]
    xor rax, rax
    rep stosq
;   [202:5] free named register 'rax'
;   [202:5] free named register 'rdi'
;   [202:5] free named register 'rcx'
;   [203:5] print(hello.len, hello)
;   [203:11] allocate named register 'rdx'
    mov rdx, hello.len
;   [203:22] allocate named register 'rsi'
    mov rsi, hello
;   [22:6] print(len : reg_rdx, ptr : reg_rsi) 
    print_203_5:
;       [203:5] alias len -> rdx  (lea: )
;       [203:5] alias ptr -> rsi  (lea: )
;       [23:5] mov(rax, 1)
;           [23:14] 1
            mov rax, 1
;       [23:19] # write system call
;       [24:5] mov(rdi, 0)
;           [24:14] 0
            mov rdi, 0
;       [24:19] # file descriptor for standard out
;       [25:5] mov(rsi, ptr)
;           [25:14] ptr
;       [25:19] # buffer address
;       [26:5] mov(rdx, len)
;           [26:14] len
;       [26:19] # buffer size
;       [27:5] syscall()
        syscall
;       [203:5] free named register 'rsi'
;       [203:5] free named register 'rdx'
    print_203_5_end:
;   [204:5] loop
    loop_204_5:
;       [205:9] print(prompt1.len, prompt1)
;       [205:15] allocate named register 'rdx'
        mov rdx, prompt1.len
;       [205:28] allocate named register 'rsi'
        mov rsi, prompt1
;       [22:6] print(len : reg_rdx, ptr : reg_rsi) 
        print_205_9:
;           [205:9] alias len -> rdx  (lea: )
;           [205:9] alias ptr -> rsi  (lea: )
;           [23:5] mov(rax, 1)
;               [23:14] 1
                mov rax, 1
;           [23:19] # write system call
;           [24:5] mov(rdi, 0)
;               [24:14] 0
                mov rdi, 0
;           [24:19] # file descriptor for standard out
;           [25:5] mov(rsi, ptr)
;               [25:14] ptr
;           [25:19] # buffer address
;           [26:5] mov(rdx, len)
;               [26:14] len
;           [26:19] # buffer size
;           [27:5] syscall()
            syscall
;           [205:9] free named register 'rsi'
;           [205:9] free named register 'rdx'
        print_205_9_end:
;       [206:9] str_in(nm)
;       [84:6] str_in(s : str) 
        str_in_206_9:
;           [206:9] alias s -> nm  (lea: )
;           [85:5] mov(rax, 0)
;               [85:14] 0
                mov rax, 0
;           [85:19] # read system call
;           [86:5] mov(rdi, 0)
;               [86:14] 0
                mov rdi, 0
;           [86:19] # file descriptor for standard input
;           [87:5] mov(rsi, address_of(s.data))
;               [87:14] rsi = address_of(s.data)
;               [87:14] = expression
;               [87:14] address_of(s.data)
                lea rsi, [rsp - 859]
;           [87:34] # buffer address
;           [88:5] mov(rdx, array_size_of(s.data))
;               [88:14] rdx = array_size_of(s.data)
;               [88:14] = expression
;               [88:14] array_size_of(s.data)
                mov rdx, 127
;           [88:37] # buffer size
;           [89:5] syscall()
            syscall
;           [90:5] mov(s.len, rax - 1)
;               [90:16] rax
                mov byte [rsp - 860], al
;               [90:22] s.len - 1
                sub byte [rsp - 860], 1
;           [90:25] # return value
        str_in_206_9_end:
        if_207_12:
;       [207:12] ? nm.len == 0
;       [207:12] ? nm.len == 0
        cmp_207_12:
        cmp byte [rsp - 860], 0
        jne if_209_19
        if_207_12_code:
;           [208:13] break
            jmp loop_204_5_end
        jmp if_207_9_end
        if_209_19:
;       [209:19] ? nm.len <= 4
;       [209:19] ? nm.len <= 4
        cmp_209_19:
        cmp byte [rsp - 860], 4
        jg if_else_207_9
        if_209_19_code:
;           [210:13] print(prompt2.len, prompt2)
;           [210:19] allocate named register 'rdx'
            mov rdx, prompt2.len
;           [210:32] allocate named register 'rsi'
            mov rsi, prompt2
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_210_13:
;               [210:13] alias len -> rdx  (lea: )
;               [210:13] alias ptr -> rsi  (lea: )
;               [23:5] mov(rax, 1)
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 0)
;                   [24:14] 0
                    mov rdi, 0
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [210:13] free named register 'rsi'
;               [210:13] free named register 'rdx'
            print_210_13_end:
;           [211:13] continue
            jmp loop_204_5
        jmp if_207_9_end
        if_else_207_9:
;           [213:13] print(prompt3.len, prompt3)
;           [213:19] allocate named register 'rdx'
            mov rdx, prompt3.len
;           [213:32] allocate named register 'rsi'
            mov rsi, prompt3
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_213_13:
;               [213:13] alias len -> rdx  (lea: )
;               [213:13] alias ptr -> rsi  (lea: )
;               [23:5] mov(rax, 1)
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 0)
;                   [24:14] 0
                    mov rdi, 0
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [213:13] free named register 'rsi'
;               [213:13] free named register 'rdx'
            print_213_13_end:
;           [214:13] str_out(nm)
;           [93:6] str_out(s : str) 
            str_out_214_13:
;               [214:13] alias s -> nm  (lea: )
;               [94:5] mov(rax, 1)
;                   [94:14] 1
                    mov rax, 1
;               [94:19] # write system call
;               [95:5] mov(rdi, 0)
;                   [95:14] 0
                    mov rdi, 0
;               [95:19] # file descriptor for standard out
;               [96:5] mov(rsi, address_of(s.data))
;                   [96:14] rsi = address_of(s.data)
;                   [96:14] = expression
;                   [96:14] address_of(s.data)
                    lea rsi, [rsp - 859]
;               [96:34] # buffer address
;               [97:5] mov(rdx, s.len)
;                   [97:14] s.len
                    movsx rdx, byte [rsp - 860]
;               [97:21] # buffer size
;               [98:5] syscall()
                syscall
            str_out_214_13_end:
;           [215:13] print(dot.len, dot)
;           [215:19] allocate named register 'rdx'
            mov rdx, dot.len
;           [215:28] allocate named register 'rsi'
            mov rsi, dot
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_215_13:
;               [215:13] alias len -> rdx  (lea: )
;               [215:13] alias ptr -> rsi  (lea: )
;               [23:5] mov(rax, 1)
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 0)
;                   [24:14] 0
                    mov rdi, 0
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [215:13] free named register 'rsi'
;               [215:13] free named register 'rdx'
            print_215_13_end:
;           [216:13] print(nl.len, nl)
;           [216:19] allocate named register 'rdx'
            mov rdx, nl.len
;           [216:27] allocate named register 'rsi'
            mov rsi, nl
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_216_13:
;               [216:13] alias len -> rdx  (lea: )
;               [216:13] alias ptr -> rsi  (lea: )
;               [23:5] mov(rax, 1)
;                   [23:14] 1
                    mov rax, 1
;               [23:19] # write system call
;               [24:5] mov(rdi, 0)
;                   [24:14] 0
                    mov rdi, 0
;               [24:19] # file descriptor for standard out
;               [25:5] mov(rsi, ptr)
;                   [25:14] ptr
;               [25:19] # buffer address
;               [26:5] mov(rdx, len)
;                   [26:14] len
;               [26:19] # buffer size
;               [27:5] syscall()
                syscall
;               [216:13] free named register 'rsi'
;               [216:13] free named register 'rdx'
            print_216_13_end:
        if_207_9_end:
    jmp loop_204_5
    loop_204_5_end:
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
;               max stack size: 860B
;          optimization pass 1: 60
;          optimization pass 2: 0
```
