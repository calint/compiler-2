# compiler-2: baz

Experimental compiler for a minimalistic, specialized language that targets NASM
x86_64 assembly on Linux.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by NASM for x86_64
* super-loop program with non-reentrant inlined functions

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
C/C++ Header                    41           1358            730           6469
C++                              1             84            104            557
-------------------------------------------------------------------------------
SUM:                            42           1442            834           7026
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
    # note: `equal` is built-in function to compare user types for equality
    #       or same size arrays

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
    sete r15b
    bool_end_109_12:
    assert_109_5:
        if_20_29_109_5:
        cmp_20_29_109_5:
        cmp r15b, false
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
    sete r15b
    bool_end_110_12:
    assert_110_5:
        if_20_29_110_5:
        cmp_20_29_110_5:
        cmp r15b, false
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
    sete r15b
    bool_end_114_12:
    assert_114_5:
        if_20_29_114_5:
        cmp_20_29_114_5:
        cmp r15b, false
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
    cmp r14, false
    setne r15b
    bool_end_118_12:
    assert_118_5:
        if_20_29_118_5:
        cmp_20_29_118_5:
        cmp r15b, false
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
    cmp r14, false
    sete r15b
    bool_end_122_12:
    assert_122_5:
        if_20_29_122_5:
        cmp_20_29_122_5:
        cmp r15b, false
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
    sete r15b
    bool_end_126_12:
    assert_126_5:
        if_20_29_126_5:
        cmp_20_29_126_5:
        cmp r15b, false
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
    sete r15b
    bool_end_129_12:
    assert_129_5:
        if_20_29_129_5:
        cmp_20_29_129_5:
        cmp r15b, false
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
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_20_29_133_5:
        cmp_20_29_133_5:
        cmp r15b, false
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
    sete r15b
    bool_end_134_12:
    assert_134_5:
        if_20_29_134_5:
        cmp_20_29_134_5:
        cmp r15b, false
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
    cmp r14, false
    setne r15b
    bool_end_137_12:
    assert_137_5:
        if_20_29_137_5:
        cmp_20_29_137_5:
        cmp r15b, false
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
    cmp_142_12:
        lea rsi, [rsp - 72]
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
        je cmps_eq_142_16
        mov r14, false
        jmp cmps_end_142_16
        cmps_eq_142_16:
        mov r14, true
        cmps_end_142_16:
    cmp r14, false
    sete r15b
    bool_end_142_12:
    assert_142_5:
        if_20_29_142_5:
        cmp_20_29_142_5:
        cmp r15b, false
        jne if_20_26_142_5_end
        if_20_29_142_5_code:
            mov rdi, 1
            exit_20_38_142_5:
                    mov rax, 60
                syscall
            exit_20_38_142_5_end:
        if_20_26_142_5_end:
    assert_142_5_end:
    mov qword [rsp - 96], 0
    bar_145_5:
        if_59_8_145_5:
        cmp_59_8_145_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_145_5_end
        if_59_8_145_5_code:
            jmp bar_145_5_end
        if_59_5_145_5_end:
        mov qword [rsp - 96], 0xff
    bar_145_5_end:
    cmp_146_12:
    cmp qword [rsp - 96], 0
    sete r15b
    bool_end_146_12:
    assert_146_5:
        if_20_29_146_5:
        cmp_20_29_146_5:
        cmp r15b, false
        jne if_20_26_146_5_end
        if_20_29_146_5_code:
            mov rdi, 1
            exit_20_38_146_5:
                    mov rax, 60
                syscall
            exit_20_38_146_5_end:
        if_20_26_146_5_end:
    assert_146_5_end:
    mov qword [rsp - 96], 1
    bar_149_5:
        if_59_8_149_5:
        cmp_59_8_149_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_149_5_end
        if_59_8_149_5_code:
            jmp bar_149_5_end
        if_59_5_149_5_end:
        mov qword [rsp - 96], 0xff
    bar_149_5_end:
    cmp_150_12:
    cmp qword [rsp - 96], 0xff
    sete r15b
    bool_end_150_12:
    assert_150_5:
        if_20_29_150_5:
        cmp_20_29_150_5:
        cmp r15b, false
        jne if_20_26_150_5_end
        if_20_29_150_5_code:
            mov rdi, 1
            exit_20_38_150_5:
                    mov rax, 60
                syscall
            exit_20_38_150_5_end:
        if_20_26_150_5_end:
    assert_150_5_end:
    mov qword [rsp - 104], 1
    baz_153_13:
        mov r15, qword [rsp - 104]
        imul r15, 2
        mov qword [rsp - 112], r15
    baz_153_13_end:
    cmp_154_12:
    cmp qword [rsp - 112], 2
    sete r15b
    bool_end_154_12:
    assert_154_5:
        if_20_29_154_5:
        cmp_20_29_154_5:
        cmp r15b, false
        jne if_20_26_154_5_end
        if_20_29_154_5_code:
            mov rdi, 1
            exit_20_38_154_5:
                    mov rax, 60
                syscall
            exit_20_38_154_5_end:
        if_20_26_154_5_end:
    assert_154_5_end:
    baz_156_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 112], r15
    baz_156_9_end:
    cmp_157_12:
    cmp qword [rsp - 112], 2
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_20_29_157_5:
        cmp_20_29_157_5:
        cmp r15b, false
        jne if_20_26_157_5_end
        if_20_29_157_5_code:
            mov rdi, 1
            exit_20_38_157_5:
                    mov rax, 60
                syscall
            exit_20_38_157_5_end:
        if_20_26_157_5_end:
    assert_157_5_end:
    baz_159_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 128], r15
    baz_159_23_end:
    mov qword [rsp - 120], 0
    cmp_160_12:
    cmp qword [rsp - 128], 4
    sete r15b
    bool_end_160_12:
    assert_160_5:
        if_20_29_160_5:
        cmp_20_29_160_5:
        cmp r15b, false
        jne if_20_26_160_5_end
        if_20_29_160_5_code:
            mov rdi, 1
            exit_20_38_160_5:
                    mov rax, 60
                syscall
            exit_20_38_160_5_end:
        if_20_26_160_5_end:
    assert_160_5_end:
    mov qword [rsp - 136], 1
    mov qword [rsp - 144], 2
    mov r15, qword [rsp - 136]
    imul r15, 10
    mov qword [rsp - 164], r15
    mov r15, qword [rsp - 144]
    mov qword [rsp - 156], r15
    mov dword [rsp - 148], 0xff0000
    cmp_166_12:
    cmp qword [rsp - 164], 10
    sete r15b
    bool_end_166_12:
    assert_166_5:
        if_20_29_166_5:
        cmp_20_29_166_5:
        cmp r15b, false
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
    cmp qword [rsp - 156], 2
    sete r15b
    bool_end_167_12:
    assert_167_5:
        if_20_29_167_5:
        cmp_20_29_167_5:
        cmp r15b, false
        jne if_20_26_167_5_end
        if_20_29_167_5_code:
            mov rdi, 1
            exit_20_38_167_5:
                    mov rax, 60
                syscall
            exit_20_38_167_5_end:
        if_20_26_167_5_end:
    assert_167_5_end:
    cmp_168_12:
    cmp dword [rsp - 148], 0xff0000
    sete r15b
    bool_end_168_12:
    assert_168_5:
        if_20_29_168_5:
        cmp_20_29_168_5:
        cmp r15b, false
        jne if_20_26_168_5_end
        if_20_29_168_5_code:
            mov rdi, 1
            exit_20_38_168_5:
                    mov rax, 60
                syscall
            exit_20_38_168_5_end:
        if_20_26_168_5_end:
    assert_168_5_end:
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
    cmp_172_12:
    cmp qword [rsp - 164], -1
    sete r15b
    bool_end_172_12:
    assert_172_5:
        if_20_29_172_5:
        cmp_20_29_172_5:
        cmp r15b, false
        jne if_20_26_172_5_end
        if_20_29_172_5_code:
            mov rdi, 1
            exit_20_38_172_5:
                    mov rax, 60
                syscall
            exit_20_38_172_5_end:
        if_20_26_172_5_end:
    assert_172_5_end:
    cmp_173_12:
    cmp qword [rsp - 156], -2
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_20_29_173_5:
        cmp_20_29_173_5:
        cmp r15b, false
        jne if_20_26_173_5_end
        if_20_29_173_5_code:
            mov rdi, 1
            exit_20_38_173_5:
                    mov rax, 60
                syscall
            exit_20_38_173_5_end:
        if_20_26_173_5_end:
    assert_173_5_end:
    lea rsi, [rsp - 164]
    lea rdi, [rsp - 200]
    mov rcx, 20
    rep movsb
    cmp_176_12:
    cmp qword [rsp - 200], -1
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_20_29_176_5:
        cmp_20_29_176_5:
        cmp r15b, false
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
    cmp qword [rsp - 192], -2
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_20_29_177_5:
        cmp_20_29_177_5:
        cmp r15b, false
        jne if_20_26_177_5_end
        if_20_29_177_5_code:
            mov rdi, 1
            exit_20_38_177_5:
                    mov rax, 60
                syscall
            exit_20_38_177_5_end:
        if_20_26_177_5_end:
    assert_177_5_end:
    cmp_178_12:
    cmp dword [rsp - 184], 0xff0000
    sete r15b
    bool_end_178_12:
    assert_178_5:
        if_20_29_178_5:
        cmp_20_29_178_5:
        cmp r15b, false
        jne if_20_26_178_5_end
        if_20_29_178_5_code:
            mov rdi, 1
            exit_20_38_178_5:
                    mov rax, 60
                syscall
            exit_20_38_178_5_end:
        if_20_26_178_5_end:
    assert_178_5_end:
    mov qword [rsp - 220], 0
    mov qword [rsp - 212], 0
    mov dword [rsp - 204], 0
    mov qword [rsp - 212], 73
    cmp_183_12:
    lea r14, [rsp - 220]
    mov r13, 0
    mov r12, 183
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_20_29_183_5:
        cmp_20_29_183_5:
        cmp r15b, false
        jne if_20_26_183_5_end
        if_20_29_183_5_code:
            mov rdi, 1
            exit_20_38_183_5:
                    mov rax, 60
                syscall
            exit_20_38_183_5_end:
        if_20_26_183_5_end:
    assert_183_5_end:
    mov rcx, 64
    lea rdi, [rsp - 732]
    xor rax, rax
    rep stosq
    lea r15, [rsp - 732]
    mov r14, 1
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 0xffee
    cmp_187_12:
    lea r14, [rsp - 732]
    mov r13, 1
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 0xffee
    sete r15b
    bool_end_187_12:
    assert_187_5:
        if_20_29_187_5:
        cmp_20_29_187_5:
        cmp r15b, false
        jne if_20_26_187_5_end
        if_20_29_187_5_code:
            mov rdi, 1
            exit_20_38_187_5:
                    mov rax, 60
                syscall
            exit_20_38_187_5_end:
        if_20_26_187_5_end:
    assert_187_5_end:
    mov rcx, 8
    lea r15, [rsp - 732]
    mov r14, 1
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
    lea rsi, [r15]
    lea r15, [rsp - 732]
    mov r14, 0
    mov r13, 191
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 191
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_196_12:
    lea r14, [rsp - 732]
    mov r13, 0
    mov r12, 196
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 196
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 0xffee
    sete r15b
    bool_end_196_12:
    assert_196_5:
        if_20_29_196_5:
        cmp_20_29_196_5:
        cmp r15b, false
        jne if_20_26_196_5_end
        if_20_29_196_5_code:
            mov rdi, 1
            exit_20_38_196_5:
                    mov rax, 60
                syscall
            exit_20_38_196_5_end:
        if_20_26_196_5_end:
    assert_196_5_end:
    cmp_197_12:
        mov rcx, 8
        lea r13, [rsp - 732]
        mov r12, 0
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
        lea rsi, [r13]
        lea r13, [rsp - 732]
        mov r12, 1
        mov r11, 199
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 199
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        je cmps_eq_197_12
        mov r14, false
        jmp cmps_end_197_12
        cmps_eq_197_12:
        mov r14, true
        cmps_end_197_12:
    cmp r14, false
    setne r15b
    bool_end_197_12:
    assert_197_5:
        if_20_29_197_5:
        cmp_20_29_197_5:
        cmp r15b, false
        jne if_20_26_197_5_end
        if_20_29_197_5_code:
            mov rdi, 1
            exit_20_38_197_5:
                    mov rax, 60
                syscall
            exit_20_38_197_5_end:
        if_20_26_197_5_end:
    assert_197_5_end:
    mov rcx, 16
    lea rdi, [rsp - 860]
    xor rax, rax
    rep stosq
    mov rdx, hello.len
    mov rsi, hello
    print_204_5:
            mov rax, 1
            mov rdi, 0
        syscall
    print_204_5_end:
    loop_205_5:
        mov rdx, prompt1.len
        mov rsi, prompt1
        print_206_9:
                mov rax, 1
                mov rdi, 0
            syscall
        print_206_9_end:
        str_in_207_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 859]
                mov rdx, 127
            syscall
                mov byte [rsp - 860], al
                sub byte [rsp - 860], 1
        str_in_207_9_end:
        if_208_12:
        cmp_208_12:
        cmp byte [rsp - 860], 0
        jne if_210_19
        if_208_12_code:
            jmp loop_205_5_end
        jmp if_208_9_end
        if_210_19:
        cmp_210_19:
        cmp byte [rsp - 860], 4
        jg if_else_208_9
        if_210_19_code:
            mov rdx, prompt2.len
            mov rsi, prompt2
            print_211_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_211_13_end:
            jmp loop_205_5
        jmp if_208_9_end
        if_else_208_9:
            mov rdx, prompt3.len
            mov rsi, prompt3
            print_214_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_214_13_end:
            str_out_215_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 859]
                    movsx rdx, byte [rsp - 860]
                syscall
            str_out_215_13_end:
            mov rdx, dot.len
            mov rsi, dot
            print_216_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_216_13_end:
            mov rdx, nl.len
            mov rsi, nl
            print_217_13:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_217_13_end:
        if_208_9_end:
    jmp loop_205_5
    loop_205_5_end:
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
    sete r15b
    bool_end_109_12:
;   [20:6] assert(expr : bool) 
    assert_109_5:
;       [109:5] alias expr -> r15b  (lea: )
        if_20_29_109_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_109_5:
        cmp r15b, false
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
    sete r15b
    bool_end_110_12:
;   [20:6] assert(expr : bool) 
    assert_110_5:
;       [110:5] alias expr -> r15b  (lea: )
        if_20_29_110_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_110_5:
        cmp r15b, false
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
    sete r15b
    bool_end_114_12:
;   [20:6] assert(expr : bool) 
    assert_114_5:
;       [114:5] alias expr -> r15b  (lea: )
        if_20_29_114_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_114_5:
        cmp r15b, false
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
    cmp r14, false
;   [118:12] free scratch register 'r14'
    setne r15b
    bool_end_118_12:
;   [20:6] assert(expr : bool) 
    assert_118_5:
;       [118:5] alias expr -> r15b  (lea: )
        if_20_29_118_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_118_5:
        cmp r15b, false
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
    cmp r14, false
;   [122:12] free scratch register 'r14'
    sete r15b
    bool_end_122_12:
;   [20:6] assert(expr : bool) 
    assert_122_5:
;       [122:5] alias expr -> r15b  (lea: )
        if_20_29_122_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_122_5:
        cmp r15b, false
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
;   [125:16] arr = ~inv(arr[ix - 1])
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
    sete r15b
    bool_end_126_12:
;   [20:6] assert(expr : bool) 
    assert_126_5:
;       [126:5] alias expr -> r15b  (lea: )
        if_20_29_126_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_126_5:
        cmp r15b, false
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
    sete r15b
    bool_end_129_12:
;   [20:6] assert(expr : bool) 
    assert_129_5:
;       [129:5] alias expr -> r15b  (lea: )
        if_20_29_129_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_129_5:
        cmp r15b, false
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
    sete r15b
    bool_end_133_12:
;   [20:6] assert(expr : bool) 
    assert_133_5:
;       [133:5] alias expr -> r15b  (lea: )
        if_20_29_133_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_133_5:
        cmp r15b, false
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
    sete r15b
    bool_end_134_12:
;   [20:6] assert(expr : bool) 
    assert_134_5:
;       [134:5] alias expr -> r15b  (lea: )
        if_20_29_134_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_134_5:
        cmp r15b, false
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
    cmp r14, false
;   [137:12] free scratch register 'r14'
    setne r15b
    bool_end_137_12:
;   [20:6] assert(expr : bool) 
    assert_137_5:
;       [137:5] alias expr -> r15b  (lea: )
        if_20_29_137_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_137_5:
        cmp r15b, false
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
;   [138:5] # note: `equal` is built-in function to compare user types for equality
;   [139:5] # or same size arrays
;   [141:5] q.x = 3
;   [141:11] 3
    mov qword [rsp - 88], 3
;   [142:5] assert(not equal(p, q))
;   [142:12] allocate scratch register -> r15
;   [142:12] ? not equal(p, q)
;   [142:12] ? not equal(p, q)
    cmp_142_12:
;   [142:16] allocate scratch register -> r14
;       [142:16] r14 = equal(p, q)
;       [142:16] = expression
;       [142:16] equal(p, q)
;       [142:16] allocate named register 'rsi'
;       [142:16] allocate named register 'rdi'
;       [142:16] allocate named register 'rcx'
;       [142:22] p
        lea rsi, [rsp - 72]
;       [142:25] q
        lea rdi, [rsp - 88]
        mov rcx, 2
        repe cmpsq
;       [142:16] free named register 'rcx'
;       [142:16] free named register 'rdi'
;       [142:16] free named register 'rsi'
        je cmps_eq_142_16
        mov r14, false
        jmp cmps_end_142_16
        cmps_eq_142_16:
        mov r14, true
        cmps_end_142_16:
    cmp r14, false
;   [142:12] free scratch register 'r14'
    sete r15b
    bool_end_142_12:
;   [20:6] assert(expr : bool) 
    assert_142_5:
;       [142:5] alias expr -> r15b  (lea: )
        if_20_29_142_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_142_5:
        cmp r15b, false
        jne if_20_26_142_5_end
        if_20_29_142_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_142_5:
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
            exit_20_38_142_5_end:
        if_20_26_142_5_end:
;       [142:5] free scratch register 'r15'
    assert_142_5_end:
;   [144:5] var i = 0
;   [144:9] i: i64 (8B @ qword [rsp - 96])
;   [144:9] i = 0
;   [144:13] 0
    mov qword [rsp - 96], 0
;   [145:5] bar(i)
;   [58:6] bar(arg) 
    bar_145_5:
;       [145:5] alias arg -> i  (lea: )
        if_59_8_145_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_145_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_145_5_end
        if_59_8_145_5_code:
;           [59:17] return
            jmp bar_145_5_end
        if_59_5_145_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
        mov qword [rsp - 96], 0xff
    bar_145_5_end:
;   [146:5] assert(i == 0)
;   [146:12] allocate scratch register -> r15
;   [146:12] ? i == 0
;   [146:12] ? i == 0
    cmp_146_12:
    cmp qword [rsp - 96], 0
    sete r15b
    bool_end_146_12:
;   [20:6] assert(expr : bool) 
    assert_146_5:
;       [146:5] alias expr -> r15b  (lea: )
        if_20_29_146_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_146_5:
        cmp r15b, false
        jne if_20_26_146_5_end
        if_20_29_146_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_146_5:
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
            exit_20_38_146_5_end:
        if_20_26_146_5_end:
;       [146:5] free scratch register 'r15'
    assert_146_5_end:
;   [148:5] i = 1
;   [148:9] 1
    mov qword [rsp - 96], 1
;   [149:5] bar(i)
;   [58:6] bar(arg) 
    bar_149_5:
;       [149:5] alias arg -> i  (lea: )
        if_59_8_149_5:
;       [59:8] ? arg == 0
;       [59:8] ? arg == 0
        cmp_59_8_149_5:
        cmp qword [rsp - 96], 0
        jne if_59_5_149_5_end
        if_59_8_149_5_code:
;           [59:17] return
            jmp bar_149_5_end
        if_59_5_149_5_end:
;       [60:5] arg = 0xff
;       [60:11] 0xff
        mov qword [rsp - 96], 0xff
    bar_149_5_end:
;   [150:5] assert(i == 0xff)
;   [150:12] allocate scratch register -> r15
;   [150:12] ? i == 0xff
;   [150:12] ? i == 0xff
    cmp_150_12:
    cmp qword [rsp - 96], 0xff
    sete r15b
    bool_end_150_12:
;   [20:6] assert(expr : bool) 
    assert_150_5:
;       [150:5] alias expr -> r15b  (lea: )
        if_20_29_150_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_150_5:
        cmp r15b, false
        jne if_20_26_150_5_end
        if_20_29_150_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_150_5:
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
            exit_20_38_150_5_end:
        if_20_26_150_5_end:
;       [150:5] free scratch register 'r15'
    assert_150_5_end:
;   [152:5] var j = 1
;   [152:9] j: i64 (8B @ qword [rsp - 104])
;   [152:9] j = 1
;   [152:13] 1
    mov qword [rsp - 104], 1
;   [153:5] var k = baz(j)
;   [153:9] k: i64 (8B @ qword [rsp - 112])
;   [153:9] k = baz(j)
;   [153:13] k = baz(j)
;   [153:13] = expression
;   [153:13] baz(j)
;   [69:6] baz(arg) : i64 res 
    baz_153_13:
;       [153:13] alias res -> qword [rsp - 112]  (lea: )
;       [153:13] alias arg -> j  (lea: )
;       [70:5] res = arg * 2
;       [70:11] allocate scratch register -> r15
;       [70:11] arg
        mov r15, qword [rsp - 104]
;       [70:17] r15 * 2
;       [70:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 112], r15
;       [70:11] free scratch register 'r15'
    baz_153_13_end:
;   [154:5] assert(k == 2)
;   [154:12] allocate scratch register -> r15
;   [154:12] ? k == 2
;   [154:12] ? k == 2
    cmp_154_12:
    cmp qword [rsp - 112], 2
    sete r15b
    bool_end_154_12:
;   [20:6] assert(expr : bool) 
    assert_154_5:
;       [154:5] alias expr -> r15b  (lea: )
        if_20_29_154_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_154_5:
        cmp r15b, false
        jne if_20_26_154_5_end
        if_20_29_154_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_154_5:
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
            exit_20_38_154_5_end:
        if_20_26_154_5_end:
;       [154:5] free scratch register 'r15'
    assert_154_5_end:
;   [156:5] k = baz(1)
;   [156:9] k = baz(1)
;   [156:9] = expression
;   [156:9] baz(1)
;   [69:6] baz(arg) : i64 res 
    baz_156_9:
;       [156:9] alias res -> qword [rsp - 112]  (lea: )
;       [156:9] alias arg -> 1  (lea: )
;       [70:5] res = arg * 2
;       [70:11] allocate scratch register -> r15
;       [70:11] arg
        mov r15, 1
;       [70:17] r15 * 2
;       [70:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 112], r15
;       [70:11] free scratch register 'r15'
    baz_156_9_end:
;   [157:5] assert(k == 2)
;   [157:12] allocate scratch register -> r15
;   [157:12] ? k == 2
;   [157:12] ? k == 2
    cmp_157_12:
    cmp qword [rsp - 112], 2
    sete r15b
    bool_end_157_12:
;   [20:6] assert(expr : bool) 
    assert_157_5:
;       [157:5] alias expr -> r15b  (lea: )
        if_20_29_157_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_157_5:
        cmp r15b, false
        jne if_20_26_157_5_end
        if_20_29_157_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_157_5:
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
            exit_20_38_157_5_end:
        if_20_26_157_5_end:
;       [157:5] free scratch register 'r15'
    assert_157_5_end:
;   [159:5] var p0 : point = {baz(2), 0}
;   [159:9] p0: point (16B @ qword [rsp - 128])
;   [159:9] p0 = {baz(2), 0}
;   [159:22] copy field 'x'
;   [159:23] qword [rsp - 128] = baz(2)
;   [159:23] = expression
;   [159:23] baz(2)
;   [69:6] baz(arg) : i64 res 
    baz_159_23:
;       [159:23] alias res -> qword [rsp - 128]  (lea: )
;       [159:23] alias arg -> 2  (lea: )
;       [70:5] res = arg * 2
;       [70:11] allocate scratch register -> r15
;       [70:11] arg
        mov r15, 2
;       [70:17] r15 * 2
;       [70:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 128], r15
;       [70:11] free scratch register 'r15'
    baz_159_23_end:
;   [159:22] copy field 'y'
    mov qword [rsp - 120], 0
;   [160:5] assert(p0.x == 4)
;   [160:12] allocate scratch register -> r15
;   [160:12] ? p0.x == 4
;   [160:12] ? p0.x == 4
    cmp_160_12:
    cmp qword [rsp - 128], 4
    sete r15b
    bool_end_160_12:
;   [20:6] assert(expr : bool) 
    assert_160_5:
;       [160:5] alias expr -> r15b  (lea: )
        if_20_29_160_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_160_5:
        cmp r15b, false
        jne if_20_26_160_5_end
        if_20_29_160_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_160_5:
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
            exit_20_38_160_5_end:
        if_20_26_160_5_end:
;       [160:5] free scratch register 'r15'
    assert_160_5_end:
;   [162:5] var x = 1
;   [162:9] x: i64 (8B @ qword [rsp - 136])
;   [162:9] x = 1
;   [162:13] 1
    mov qword [rsp - 136], 1
;   [163:5] var y = 2
;   [163:9] y: i64 (8B @ qword [rsp - 144])
;   [163:9] y = 2
;   [163:13] 2
    mov qword [rsp - 144], 2
;   [165:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [165:9] o1: object (20B @ qword [rsp - 164])
;   [165:9] o1 = {{x * 10, y}, 0xff0000}
;   [165:23] copy field 'pos'
;   [165:24] copy field 'x'
;   [165:25] allocate scratch register -> r15
;   [165:25] x
    mov r15, qword [rsp - 136]
;   [165:29] r15 * 10
;   [165:29] dst is reg, src is const
    imul r15, 10
    mov qword [rsp - 164], r15
;   [165:25] free scratch register 'r15'
;   [165:24] copy field 'y'
;   [165:33] allocate scratch register -> r15
    mov r15, qword [rsp - 144]
    mov qword [rsp - 156], r15
;   [165:33] free scratch register 'r15'
;   [165:23] copy field 'color'
    mov dword [rsp - 148], 0xff0000
;   [166:5] assert(o1.pos.x == 10)
;   [166:12] allocate scratch register -> r15
;   [166:12] ? o1.pos.x == 10
;   [166:12] ? o1.pos.x == 10
    cmp_166_12:
    cmp qword [rsp - 164], 10
    sete r15b
    bool_end_166_12:
;   [20:6] assert(expr : bool) 
    assert_166_5:
;       [166:5] alias expr -> r15b  (lea: )
        if_20_29_166_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_166_5:
        cmp r15b, false
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
;   [167:5] assert(o1.pos.y == 2)
;   [167:12] allocate scratch register -> r15
;   [167:12] ? o1.pos.y == 2
;   [167:12] ? o1.pos.y == 2
    cmp_167_12:
    cmp qword [rsp - 156], 2
    sete r15b
    bool_end_167_12:
;   [20:6] assert(expr : bool) 
    assert_167_5:
;       [167:5] alias expr -> r15b  (lea: )
        if_20_29_167_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_167_5:
        cmp r15b, false
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
;   [168:5] assert(o1.color == 0xff0000)
;   [168:12] allocate scratch register -> r15
;   [168:12] ? o1.color == 0xff0000
;   [168:12] ? o1.color == 0xff0000
    cmp_168_12:
    cmp dword [rsp - 148], 0xff0000
    sete r15b
    bool_end_168_12:
;   [20:6] assert(expr : bool) 
    assert_168_5:
;       [168:5] alias expr -> r15b  (lea: )
        if_20_29_168_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_168_5:
        cmp r15b, false
        jne if_20_26_168_5_end
        if_20_29_168_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_168_5:
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
            exit_20_38_168_5_end:
        if_20_26_168_5_end:
;       [168:5] free scratch register 'r15'
    assert_168_5_end:
;   [170:5] var p1 : point = {-x, -y}
;   [170:9] p1: point (16B @ qword [rsp - 180])
;   [170:9] p1 = {-x, -y}
;   [170:22] copy field 'x'
;   [170:23] allocate scratch register -> r15
    mov r15, qword [rsp - 136]
    mov qword [rsp - 180], r15
;   [170:23] free scratch register 'r15'
    neg qword [rsp - 180]
;   [170:22] copy field 'y'
;   [170:27] allocate scratch register -> r15
    mov r15, qword [rsp - 144]
    mov qword [rsp - 172], r15
;   [170:27] free scratch register 'r15'
    neg qword [rsp - 172]
;   [171:5] o1.pos = p1
;   [171:14] allocate named register 'rsi'
;   [171:14] allocate named register 'rdi'
;   [171:14] allocate named register 'rcx'
    lea rsi, [rsp - 180]
    lea rdi, [rsp - 164]
    mov rcx, 2
    rep movsq
;   [171:14] free named register 'rcx'
;   [171:14] free named register 'rdi'
;   [171:14] free named register 'rsi'
;   [172:5] assert(o1.pos.x == -1)
;   [172:12] allocate scratch register -> r15
;   [172:12] ? o1.pos.x == -1
;   [172:12] ? o1.pos.x == -1
    cmp_172_12:
    cmp qword [rsp - 164], -1
    sete r15b
    bool_end_172_12:
;   [20:6] assert(expr : bool) 
    assert_172_5:
;       [172:5] alias expr -> r15b  (lea: )
        if_20_29_172_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_172_5:
        cmp r15b, false
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
;   [173:5] assert(o1.pos.y == -2)
;   [173:12] allocate scratch register -> r15
;   [173:12] ? o1.pos.y == -2
;   [173:12] ? o1.pos.y == -2
    cmp_173_12:
    cmp qword [rsp - 156], -2
    sete r15b
    bool_end_173_12:
;   [20:6] assert(expr : bool) 
    assert_173_5:
;       [173:5] alias expr -> r15b  (lea: )
        if_20_29_173_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_173_5:
        cmp r15b, false
        jne if_20_26_173_5_end
        if_20_29_173_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_173_5:
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
            exit_20_38_173_5_end:
        if_20_26_173_5_end:
;       [173:5] free scratch register 'r15'
    assert_173_5_end:
;   [175:5] var o2 : object = o1
;   [175:9] o2: object (20B @ qword [rsp - 200])
;   [175:9] o2 = o1
;   [175:23] allocate named register 'rsi'
;   [175:23] allocate named register 'rdi'
;   [175:23] allocate named register 'rcx'
    lea rsi, [rsp - 164]
    lea rdi, [rsp - 200]
    mov rcx, 20
    rep movsb
;   [175:23] free named register 'rcx'
;   [175:23] free named register 'rdi'
;   [175:23] free named register 'rsi'
;   [176:5] assert(o2.pos.x == -1)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? o2.pos.x == -1
;   [176:12] ? o2.pos.x == -1
    cmp_176_12:
    cmp qword [rsp - 200], -1
    sete r15b
    bool_end_176_12:
;   [20:6] assert(expr : bool) 
    assert_176_5:
;       [176:5] alias expr -> r15b  (lea: )
        if_20_29_176_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_176_5:
        cmp r15b, false
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
;   [177:5] assert(o2.pos.y == -2)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? o2.pos.y == -2
;   [177:12] ? o2.pos.y == -2
    cmp_177_12:
    cmp qword [rsp - 192], -2
    sete r15b
    bool_end_177_12:
;   [20:6] assert(expr : bool) 
    assert_177_5:
;       [177:5] alias expr -> r15b  (lea: )
        if_20_29_177_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_177_5:
        cmp r15b, false
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
;   [178:5] assert(o2.color == 0xff0000)
;   [178:12] allocate scratch register -> r15
;   [178:12] ? o2.color == 0xff0000
;   [178:12] ? o2.color == 0xff0000
    cmp_178_12:
    cmp dword [rsp - 184], 0xff0000
    sete r15b
    bool_end_178_12:
;   [20:6] assert(expr : bool) 
    assert_178_5:
;       [178:5] alias expr -> r15b  (lea: )
        if_20_29_178_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_178_5:
        cmp r15b, false
        jne if_20_26_178_5_end
        if_20_29_178_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_178_5:
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
            exit_20_38_178_5_end:
        if_20_26_178_5_end:
;       [178:5] free scratch register 'r15'
    assert_178_5_end:
;   [180:5] var o3 : object[1]
;   [180:9] o3: object[1] (20B @ qword [rsp - 220])
;   [180:9] clear 1 * 20B = 20B
;   [180:5] size <= 32B, use mov
    mov qword [rsp - 220], 0
    mov qword [rsp - 212], 0
    mov dword [rsp - 204], 0
;   [181:5] # index 0 in an array can be accessed without array index
;   [182:5] o3.pos.y = 73
;   [182:16] 73
    mov qword [rsp - 212], 73
;   [183:5] assert(o3[0].pos.y == 73)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? o3[0].pos.y == 73
;   [183:12] ? o3[0].pos.y == 73
    cmp_183_12:
;   [183:12] allocate scratch register -> r14
    lea r14, [rsp - 220]
;   [183:12] allocate scratch register -> r13
;   [183:15] set array index
;   [183:15] 0
    mov r13, 0
;   [183:15] bounds check
;   [183:15] allocate scratch register -> r12
;   [183:15] line number
    mov r12, 183
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
;   [183:15] free scratch register 'r12'
    imul r13, 20
    add r14, r13
;   [183:12] free scratch register 'r13'
    cmp qword [r14 + 8], 73
;   [183:12] free scratch register 'r14'
    sete r15b
    bool_end_183_12:
;   [20:6] assert(expr : bool) 
    assert_183_5:
;       [183:5] alias expr -> r15b  (lea: )
        if_20_29_183_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_183_5:
        cmp r15b, false
        jne if_20_26_183_5_end
        if_20_29_183_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_183_5:
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
            exit_20_38_183_5_end:
        if_20_26_183_5_end:
;       [183:5] free scratch register 'r15'
    assert_183_5_end:
;   [185:5] var worlds : world[8]
;   [185:9] worlds: world[8] (64B @ qword [rsp - 732])
;   [185:9] clear 8 * 64B = 512B
;   [185:5] allocate named register 'rcx'
;   [185:5] allocate named register 'rdi'
;   [185:5] allocate named register 'rax'
    mov rcx, 64
    lea rdi, [rsp - 732]
    xor rax, rax
    rep stosq
;   [185:5] free named register 'rax'
;   [185:5] free named register 'rdi'
;   [185:5] free named register 'rcx'
;   [186:5] worlds[1].locations[1] = 0xffee
;   [186:5] allocate scratch register -> r15
    lea r15, [rsp - 732]
;   [186:5] allocate scratch register -> r14
;   [186:12] set array index
;   [186:12] 1
    mov r14, 1
;   [186:12] bounds check
;   [186:12] allocate scratch register -> r13
;   [186:12] line number
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [186:12] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [186:5] free scratch register 'r14'
;   [186:5] allocate scratch register -> r14
;   [186:25] set array index
;   [186:25] 1
    mov r14, 1
;   [186:25] bounds check
;   [186:25] allocate scratch register -> r13
;   [186:25] line number
    mov r13, 186
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [186:25] free scratch register 'r13'
;   [186:30] 0xffee
    mov qword [r15 + r14 * 8], 0xffee
;   [186:5] free scratch register 'r14'
;   [186:5] free scratch register 'r15'
;   [187:5] assert(worlds[1].locations[1] == 0xffee)
;   [187:12] allocate scratch register -> r15
;   [187:12] ? worlds[1].locations[1] == 0xffee
;   [187:12] ? worlds[1].locations[1] == 0xffee
    cmp_187_12:
;   [187:12] allocate scratch register -> r14
    lea r14, [rsp - 732]
;   [187:12] allocate scratch register -> r13
;   [187:19] set array index
;   [187:19] 1
    mov r13, 1
;   [187:19] bounds check
;   [187:19] allocate scratch register -> r12
;   [187:19] line number
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [187:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [187:12] free scratch register 'r13'
;   [187:12] allocate scratch register -> r13
;   [187:32] set array index
;   [187:32] 1
    mov r13, 1
;   [187:32] bounds check
;   [187:32] allocate scratch register -> r12
;   [187:32] line number
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [187:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 0xffee
;   [187:12] free scratch register 'r13'
;   [187:12] free scratch register 'r14'
    sete r15b
    bool_end_187_12:
;   [20:6] assert(expr : bool) 
    assert_187_5:
;       [187:5] alias expr -> r15b  (lea: )
        if_20_29_187_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_187_5:
        cmp r15b, false
        jne if_20_26_187_5_end
        if_20_29_187_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_187_5:
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
            exit_20_38_187_5_end:
        if_20_26_187_5_end:
;       [187:5] free scratch register 'r15'
    assert_187_5_end:
;   [189:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [189:5] allocate named register 'rsi'
;   [189:5] allocate named register 'rdi'
;   [189:5] allocate named register 'rcx'
;   [191:9] array_size_of(worlds.locations)
;   [192:9] rcx = array_size_of(worlds.locations)
;   [192:9] = expression
;   [192:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [190:9] worlds[1].locations
;   [190:9] allocate scratch register -> r15
    lea r15, [rsp - 732]
;   [190:9] allocate scratch register -> r14
;   [190:16] set array index
;   [190:16] 1
    mov r14, 1
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
    lea rsi, [r15]
;   [189:5] free scratch register 'r15'
;   [191:9] worlds[0].locations
;   [191:9] allocate scratch register -> r15
    lea r15, [rsp - 732]
;   [191:9] allocate scratch register -> r14
;   [191:16] set array index
;   [191:16] 0
    mov r14, 0
;   [191:16] bounds check
;   [191:16] allocate scratch register -> r13
;   [191:16] line number
    mov r13, 191
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [191:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [191:9] free scratch register 'r14'
;   [191:9] bounds check
;   [191:9] allocate scratch register -> r14
;   [191:9] line number
    mov r14, 191
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [191:9] free scratch register 'r14'
    lea rdi, [r15]
;   [189:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [189:5] free named register 'rcx'
;   [189:5] free named register 'rdi'
;   [189:5] free named register 'rsi'
;   [194:5] # note: `array_copy` is built-in and can use indexed positions
;   [195:5] # `array_size_of` is built-in
;   [196:5] assert(worlds[0].locations[1] == 0xffee)
;   [196:12] allocate scratch register -> r15
;   [196:12] ? worlds[0].locations[1] == 0xffee
;   [196:12] ? worlds[0].locations[1] == 0xffee
    cmp_196_12:
;   [196:12] allocate scratch register -> r14
    lea r14, [rsp - 732]
;   [196:12] allocate scratch register -> r13
;   [196:19] set array index
;   [196:19] 0
    mov r13, 0
;   [196:19] bounds check
;   [196:19] allocate scratch register -> r12
;   [196:19] line number
    mov r12, 196
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [196:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [196:12] free scratch register 'r13'
;   [196:12] allocate scratch register -> r13
;   [196:32] set array index
;   [196:32] 1
    mov r13, 1
;   [196:32] bounds check
;   [196:32] allocate scratch register -> r12
;   [196:32] line number
    mov r12, 196
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [196:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 0xffee
;   [196:12] free scratch register 'r13'
;   [196:12] free scratch register 'r14'
    sete r15b
    bool_end_196_12:
;   [20:6] assert(expr : bool) 
    assert_196_5:
;       [196:5] alias expr -> r15b  (lea: )
        if_20_29_196_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_196_5:
        cmp r15b, false
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
;   [197:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [197:12] allocate scratch register -> r15
;   [197:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [197:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_197_12:
;   [197:12] allocate scratch register -> r14
;       [197:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [197:12] = expression
;       [197:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [197:12] allocate named register 'rsi'
;       [197:12] allocate named register 'rdi'
;       [197:12] allocate named register 'rcx'
;       [199:14] array_size_of(worlds.locations)
;       [200:14] rcx = array_size_of(worlds.locations)
;       [200:14] = expression
;       [200:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [198:14] worlds[0].locations
;       [198:14] allocate scratch register -> r13
        lea r13, [rsp - 732]
;       [198:14] allocate scratch register -> r12
;       [198:21] set array index
;       [198:21] 0
        mov r12, 0
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
        lea rsi, [r13]
;       [197:12] free scratch register 'r13'
;       [199:14] worlds[1].locations
;       [199:14] allocate scratch register -> r13
        lea r13, [rsp - 732]
;       [199:14] allocate scratch register -> r12
;       [199:21] set array index
;       [199:21] 1
        mov r12, 1
;       [199:21] bounds check
;       [199:21] allocate scratch register -> r11
;       [199:21] line number
        mov r11, 199
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [199:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [199:14] free scratch register 'r12'
;       [199:14] bounds check
;       [199:14] allocate scratch register -> r12
;       [199:14] line number
        mov r12, 199
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [199:14] free scratch register 'r12'
        lea rdi, [r13]
;       [197:12] free scratch register 'r13'
        shl rcx, 3
        repe cmpsb
;       [197:12] free named register 'rcx'
;       [197:12] free named register 'rdi'
;       [197:12] free named register 'rsi'
        je cmps_eq_197_12
        mov r14, false
        jmp cmps_end_197_12
        cmps_eq_197_12:
        mov r14, true
        cmps_end_197_12:
    cmp r14, false
;   [197:12] free scratch register 'r14'
    setne r15b
    bool_end_197_12:
;   [20:6] assert(expr : bool) 
    assert_197_5:
;       [197:5] alias expr -> r15b  (lea: )
        if_20_29_197_5:
;       [20:29] ? not expr
;       [20:29] ? not expr
        cmp_20_29_197_5:
        cmp r15b, false
        jne if_20_26_197_5_end
        if_20_29_197_5_code:
;           [20:38] exit(1)
;           [20:43] allocate named register 'rdi'
            mov rdi, 1
;           [12:6] exit(v : reg_rdi) 
            exit_20_38_197_5:
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
            exit_20_38_197_5_end:
        if_20_26_197_5_end:
;       [197:5] free scratch register 'r15'
    assert_197_5_end:
;   [203:5] var nm : str
;   [203:9] nm: str (128B @ byte [rsp - 860])
;   [203:9] clear 1 * 128B = 128B
;   [203:5] allocate named register 'rcx'
;   [203:5] allocate named register 'rdi'
;   [203:5] allocate named register 'rax'
    mov rcx, 16
    lea rdi, [rsp - 860]
    xor rax, rax
    rep stosq
;   [203:5] free named register 'rax'
;   [203:5] free named register 'rdi'
;   [203:5] free named register 'rcx'
;   [204:5] print(hello.len, hello)
;   [204:11] allocate named register 'rdx'
    mov rdx, hello.len
;   [204:22] allocate named register 'rsi'
    mov rsi, hello
;   [22:6] print(len : reg_rdx, ptr : reg_rsi) 
    print_204_5:
;       [204:5] alias len -> rdx  (lea: )
;       [204:5] alias ptr -> rsi  (lea: )
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
;       [204:5] free named register 'rsi'
;       [204:5] free named register 'rdx'
    print_204_5_end:
;   [205:5] loop
    loop_205_5:
;       [206:9] print(prompt1.len, prompt1)
;       [206:15] allocate named register 'rdx'
        mov rdx, prompt1.len
;       [206:28] allocate named register 'rsi'
        mov rsi, prompt1
;       [22:6] print(len : reg_rdx, ptr : reg_rsi) 
        print_206_9:
;           [206:9] alias len -> rdx  (lea: )
;           [206:9] alias ptr -> rsi  (lea: )
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
;           [206:9] free named register 'rsi'
;           [206:9] free named register 'rdx'
        print_206_9_end:
;       [207:9] str_in(nm)
;       [84:6] str_in(s : str) 
        str_in_207_9:
;           [207:9] alias s -> nm  (lea: )
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
        str_in_207_9_end:
        if_208_12:
;       [208:12] ? nm.len == 0
;       [208:12] ? nm.len == 0
        cmp_208_12:
        cmp byte [rsp - 860], 0
        jne if_210_19
        if_208_12_code:
;           [209:13] break
            jmp loop_205_5_end
        jmp if_208_9_end
        if_210_19:
;       [210:19] ? nm.len <= 4
;       [210:19] ? nm.len <= 4
        cmp_210_19:
        cmp byte [rsp - 860], 4
        jg if_else_208_9
        if_210_19_code:
;           [211:13] print(prompt2.len, prompt2)
;           [211:19] allocate named register 'rdx'
            mov rdx, prompt2.len
;           [211:32] allocate named register 'rsi'
            mov rsi, prompt2
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_211_13:
;               [211:13] alias len -> rdx  (lea: )
;               [211:13] alias ptr -> rsi  (lea: )
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
;               [211:13] free named register 'rsi'
;               [211:13] free named register 'rdx'
            print_211_13_end:
;           [212:13] continue
            jmp loop_205_5
        jmp if_208_9_end
        if_else_208_9:
;           [214:13] print(prompt3.len, prompt3)
;           [214:19] allocate named register 'rdx'
            mov rdx, prompt3.len
;           [214:32] allocate named register 'rsi'
            mov rsi, prompt3
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_214_13:
;               [214:13] alias len -> rdx  (lea: )
;               [214:13] alias ptr -> rsi  (lea: )
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
;               [214:13] free named register 'rsi'
;               [214:13] free named register 'rdx'
            print_214_13_end:
;           [215:13] str_out(nm)
;           [93:6] str_out(s : str) 
            str_out_215_13:
;               [215:13] alias s -> nm  (lea: )
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
            str_out_215_13_end:
;           [216:13] print(dot.len, dot)
;           [216:19] allocate named register 'rdx'
            mov rdx, dot.len
;           [216:28] allocate named register 'rsi'
            mov rsi, dot
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
;           [217:13] print(nl.len, nl)
;           [217:19] allocate named register 'rdx'
            mov rdx, nl.len
;           [217:27] allocate named register 'rsi'
            mov rsi, nl
;           [22:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_217_13:
;               [217:13] alias len -> rdx  (lea: )
;               [217:13] alias ptr -> rsi  (lea: )
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
;               [217:13] free named register 'rsi'
;               [217:13] free named register 'rdx'
            print_217_13_end:
        if_208_9_end:
    jmp loop_205_5
    loop_205_5_end:
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
;          optimization pass 1: 88
;          optimization pass 2: 0
```
