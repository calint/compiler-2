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
C/C++ Header                    42           1411            751           6849
C++                              1             84            104            558
-------------------------------------------------------------------------------
SUM:                            43           1495            855           7407
-------------------------------------------------------------------------------
```

## Sample

```text
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

func print_str(str : i8[]) {
   print(array_size_of(str), address_of(str)) 
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
    dat   hello : i8[] = "hello world from baz\n"
    dat prompt1 : i8[] = "enter name:\n"
    dat prompt2 : i8[] = "that is not a name.\n"
    dat prompt3 : i8[] = "hello "
    dat     dot : i8[] = "."
    dat      nl : i8[] = "\n"

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
    print_str(hello)
    loop {
        print_str(prompt1)
        str_in(nm)
        if nm.len == 0 {
            break
        } else if nm.len <= 4 {
            print_str(prompt2)
            continue
        } else {
            print_str(prompt3)
            str_out(nm)
            print_str(dot)
            print_str(nl)
        }
    }
}
```

## Generates

```nasm
default rel
section .bss
stk resd 131072
stk.end:
section .text
bits 64
global _start
_start:
lea rsi, [dat]
lea rdi, [stk.end]
sub rdi, dat.len
mov rcx, dat.len
cld
rep movsb
mov rsp, stk.end
main:
    mov qword [rsp - 77], 0
    mov qword [rsp - 69], 0
    mov qword [rsp - 85], 1
    mov r15, qword [rsp - 85]
    mov r14, 111
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 77], 2
    mov r15, qword [rsp - 85]
    add r15, 1
    mov r14, 112
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 85]
    mov r13, 112
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 77]
    mov dword [rsp + r15 * 4 - 77], r13d
    cmp_113_12:
    mov r14, 1
    mov r13, 113
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_113_12:
    assert_113_5:
        if_13_29_113_5:
        cmp_13_29_113_5:
        cmp r15b, 0
        jne if_13_26_113_5_end
        if_13_29_113_5_code:
            mov rdi, 1
            exit_13_38_113_5:
                    mov rax, 60
                syscall
            exit_13_38_113_5_end:
        if_13_26_113_5_end:
    assert_113_5_end:
    cmp_114_12:
    mov r14, 2
    mov r13, 114
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_114_12:
    assert_114_5:
        if_13_29_114_5:
        cmp_13_29_114_5:
        cmp r15b, 0
        jne if_13_26_114_5_end
        if_13_29_114_5_code:
            mov rdi, 1
            exit_13_38_114_5:
                    mov rax, 60
                syscall
            exit_13_38_114_5_end:
        if_13_26_114_5_end:
    assert_114_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 116
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 77]
    mov r15, 116
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 77]
    shl rcx, 2
    rep movsb
    cmp_118_12:
    mov r14, 0
    mov r13, 118
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_118_12:
    assert_118_5:
        if_13_29_118_5:
        cmp_13_29_118_5:
        cmp r15b, 0
        jne if_13_26_118_5_end
        if_13_29_118_5_code:
            mov rdi, 1
            exit_13_38_118_5:
                    mov rax, 60
                syscall
            exit_13_38_118_5_end:
        if_13_26_118_5_end:
    assert_118_5_end:
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
    mov qword [rsp - 93], 0
    mov rcx, 4
    mov r15, 121
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 77]
    mov r15, 121
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 117]
    shl rcx, 2
    rep movsb
    cmp_122_12:
        mov rcx, 4
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 117]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_122_12:
    assert_122_5:
        if_13_29_122_5:
        cmp_13_29_122_5:
        cmp r15b, 0
        jne if_13_26_122_5_end
        if_13_29_122_5_code:
            mov rdi, 1
            exit_13_38_122_5:
                    mov rax, 60
                syscall
            exit_13_38_122_5_end:
        if_13_26_122_5_end:
    assert_122_5_end:
    mov r15, 2
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 117], -1
    cmp_126_12:
        mov rcx, 4
        mov r13, 126
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 126
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 117]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_126_12:
    assert_126_5:
        if_13_29_126_5:
        cmp_13_29_126_5:
        cmp r15b, 0
        jne if_13_26_126_5_end
        if_13_29_126_5_code:
            mov rdi, 1
            exit_13_38_126_5:
                    mov rax, 60
                syscall
            exit_13_38_126_5_end:
        if_13_26_126_5_end:
    assert_126_5_end:
    mov qword [rsp - 85], 3
    mov r15, qword [rsp - 85]
    mov r14, 129
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 85]
    sub r14, 1
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_129_16:
        mov r13d, dword [rsp + r14 * 4 - 77]
        mov dword [rsp + r15 * 4 - 77], r13d
        not dword [rsp + r15 * 4 - 77]
    inv_129_16_end:
    not dword [rsp + r15 * 4 - 77]
    cmp_130_12:
    mov r14, qword [rsp - 85]
    mov r13, 130
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_130_12:
    assert_130_5:
        if_13_29_130_5:
        cmp_13_29_130_5:
        cmp r15b, 0
        jne if_13_26_130_5_end
        if_13_29_130_5_code:
            mov rdi, 1
            exit_13_38_130_5:
                    mov rax, 60
                syscall
            exit_13_38_130_5_end:
        if_13_26_130_5_end:
    assert_130_5_end:
    faz_132_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 77], 254
    faz_132_5_end:
    cmp_133_12:
    mov r14, 1
    mov r13, 133
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 254
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_13_29_133_5:
        cmp_13_29_133_5:
        cmp r15b, 0
        jne if_13_26_133_5_end
        if_13_29_133_5_code:
            mov rdi, 1
            exit_13_38_133_5:
                    mov rax, 60
                syscall
            exit_13_38_133_5_end:
        if_13_26_133_5_end:
    assert_133_5_end:
    mov qword [rsp - 133], 0
    mov qword [rsp - 125], 0
    foo_136_5:
        mov qword [rsp - 133], 2
        mov qword [rsp - 125], 11
    foo_136_5_end:
    cmp_137_12:
    cmp qword [rsp - 133], 2
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_13_29_137_5:
        cmp_13_29_137_5:
        cmp r15b, 0
        jne if_13_26_137_5_end
        if_13_29_137_5_code:
            mov rdi, 1
            exit_13_38_137_5:
                    mov rax, 60
                syscall
            exit_13_38_137_5_end:
        if_13_26_137_5_end:
    assert_137_5_end:
    cmp_138_12:
    cmp qword [rsp - 125], 11
    sete r15b
    bool_end_138_12:
    assert_138_5:
        if_13_29_138_5:
        cmp_13_29_138_5:
        cmp r15b, 0
        jne if_13_26_138_5_end
        if_13_29_138_5_code:
            mov rdi, 1
            exit_13_38_138_5:
                    mov rax, 60
                syscall
            exit_13_38_138_5_end:
        if_13_26_138_5_end:
    assert_138_5_end:
    lea rsi, [rsp - 133]
    lea rdi, [rsp - 149]
    mov rcx, 2
    rep movsq
    cmp_141_12:
        lea rsi, [rsp - 133]
        lea rdi, [rsp - 149]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_141_12:
    assert_141_5:
        if_13_29_141_5:
        cmp_13_29_141_5:
        cmp r15b, 0
        jne if_13_26_141_5_end
        if_13_29_141_5_code:
            mov rdi, 1
            exit_13_38_141_5:
                    mov rax, 60
                syscall
            exit_13_38_141_5_end:
        if_13_26_141_5_end:
    assert_141_5_end:
    mov qword [rsp - 149], 3
    cmp_146_12:
        lea rsi, [rsp - 133]
        lea rdi, [rsp - 149]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_146_12:
    assert_146_5:
        if_13_29_146_5:
        cmp_13_29_146_5:
        cmp r15b, 0
        jne if_13_26_146_5_end
        if_13_29_146_5_code:
            mov rdi, 1
            exit_13_38_146_5:
                    mov rax, 60
                syscall
            exit_13_38_146_5_end:
        if_13_26_146_5_end:
    assert_146_5_end:
    mov qword [rsp - 157], 0
    bar_149_5:
        if_56_8_149_5:
        cmp_56_8_149_5:
        cmp qword [rsp - 157], 0
        jne if_56_5_149_5_end
        if_56_8_149_5_code:
            jmp bar_149_5_end
        if_56_5_149_5_end:
        mov qword [rsp - 157], 255
    bar_149_5_end:
    cmp_150_12:
    cmp qword [rsp - 157], 0
    sete r15b
    bool_end_150_12:
    assert_150_5:
        if_13_29_150_5:
        cmp_13_29_150_5:
        cmp r15b, 0
        jne if_13_26_150_5_end
        if_13_29_150_5_code:
            mov rdi, 1
            exit_13_38_150_5:
                    mov rax, 60
                syscall
            exit_13_38_150_5_end:
        if_13_26_150_5_end:
    assert_150_5_end:
    mov qword [rsp - 157], 1
    bar_153_5:
        if_56_8_153_5:
        cmp_56_8_153_5:
        cmp qword [rsp - 157], 0
        jne if_56_5_153_5_end
        if_56_8_153_5_code:
            jmp bar_153_5_end
        if_56_5_153_5_end:
        mov qword [rsp - 157], 255
    bar_153_5_end:
    cmp_154_12:
    cmp qword [rsp - 157], 255
    sete r15b
    bool_end_154_12:
    assert_154_5:
        if_13_29_154_5:
        cmp_13_29_154_5:
        cmp r15b, 0
        jne if_13_26_154_5_end
        if_13_29_154_5_code:
            mov rdi, 1
            exit_13_38_154_5:
                    mov rax, 60
                syscall
            exit_13_38_154_5_end:
        if_13_26_154_5_end:
    assert_154_5_end:
    mov qword [rsp - 165], 1
    baz_157_13:
        mov r15, qword [rsp - 165]
        imul r15, 2
        mov qword [rsp - 173], r15
    baz_157_13_end:
    cmp_158_12:
    cmp qword [rsp - 173], 2
    sete r15b
    bool_end_158_12:
    assert_158_5:
        if_13_29_158_5:
        cmp_13_29_158_5:
        cmp r15b, 0
        jne if_13_26_158_5_end
        if_13_29_158_5_code:
            mov rdi, 1
            exit_13_38_158_5:
                    mov rax, 60
                syscall
            exit_13_38_158_5_end:
        if_13_26_158_5_end:
    assert_158_5_end:
    baz_160_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 173], r15
    baz_160_9_end:
    cmp_161_12:
    cmp qword [rsp - 173], 2
    sete r15b
    bool_end_161_12:
    assert_161_5:
        if_13_29_161_5:
        cmp_13_29_161_5:
        cmp r15b, 0
        jne if_13_26_161_5_end
        if_13_29_161_5_code:
            mov rdi, 1
            exit_13_38_161_5:
                    mov rax, 60
                syscall
            exit_13_38_161_5_end:
        if_13_26_161_5_end:
    assert_161_5_end:
    baz_163_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 189], r15
    baz_163_23_end:
    mov qword [rsp - 181], 0
    cmp_164_12:
    cmp qword [rsp - 189], 4
    sete r15b
    bool_end_164_12:
    assert_164_5:
        if_13_29_164_5:
        cmp_13_29_164_5:
        cmp r15b, 0
        jne if_13_26_164_5_end
        if_13_29_164_5_code:
            mov rdi, 1
            exit_13_38_164_5:
                    mov rax, 60
                syscall
            exit_13_38_164_5_end:
        if_13_26_164_5_end:
    assert_164_5_end:
    mov qword [rsp - 197], 1
    mov qword [rsp - 205], 2
    mov r15, qword [rsp - 197]
    imul r15, 10
    mov qword [rsp - 225], r15
    mov r15, qword [rsp - 205]
    mov qword [rsp - 217], r15
    mov dword [rsp - 209], 16711680
    cmp_170_12:
    cmp qword [rsp - 225], 10
    sete r15b
    bool_end_170_12:
    assert_170_5:
        if_13_29_170_5:
        cmp_13_29_170_5:
        cmp r15b, 0
        jne if_13_26_170_5_end
        if_13_29_170_5_code:
            mov rdi, 1
            exit_13_38_170_5:
                    mov rax, 60
                syscall
            exit_13_38_170_5_end:
        if_13_26_170_5_end:
    assert_170_5_end:
    cmp_171_12:
    cmp qword [rsp - 217], 2
    sete r15b
    bool_end_171_12:
    assert_171_5:
        if_13_29_171_5:
        cmp_13_29_171_5:
        cmp r15b, 0
        jne if_13_26_171_5_end
        if_13_29_171_5_code:
            mov rdi, 1
            exit_13_38_171_5:
                    mov rax, 60
                syscall
            exit_13_38_171_5_end:
        if_13_26_171_5_end:
    assert_171_5_end:
    cmp_172_12:
    cmp dword [rsp - 209], 16711680
    sete r15b
    bool_end_172_12:
    assert_172_5:
        if_13_29_172_5:
        cmp_13_29_172_5:
        cmp r15b, 0
        jne if_13_26_172_5_end
        if_13_29_172_5_code:
            mov rdi, 1
            exit_13_38_172_5:
                    mov rax, 60
                syscall
            exit_13_38_172_5_end:
        if_13_26_172_5_end:
    assert_172_5_end:
    mov r15, qword [rsp - 197]
    mov qword [rsp - 241], r15
    neg qword [rsp - 241]
    mov r15, qword [rsp - 205]
    mov qword [rsp - 233], r15
    neg qword [rsp - 233]
    lea rsi, [rsp - 241]
    lea rdi, [rsp - 225]
    mov rcx, 2
    rep movsq
    cmp_176_12:
    cmp qword [rsp - 225], -1
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_13_29_176_5:
        cmp_13_29_176_5:
        cmp r15b, 0
        jne if_13_26_176_5_end
        if_13_29_176_5_code:
            mov rdi, 1
            exit_13_38_176_5:
                    mov rax, 60
                syscall
            exit_13_38_176_5_end:
        if_13_26_176_5_end:
    assert_176_5_end:
    cmp_177_12:
    cmp qword [rsp - 217], -2
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_13_29_177_5:
        cmp_13_29_177_5:
        cmp r15b, 0
        jne if_13_26_177_5_end
        if_13_29_177_5_code:
            mov rdi, 1
            exit_13_38_177_5:
                    mov rax, 60
                syscall
            exit_13_38_177_5_end:
        if_13_26_177_5_end:
    assert_177_5_end:
    lea rsi, [rsp - 225]
    lea rdi, [rsp - 261]
    mov rcx, 20
    rep movsb
    cmp_180_12:
    cmp qword [rsp - 261], -1
    sete r15b
    bool_end_180_12:
    assert_180_5:
        if_13_29_180_5:
        cmp_13_29_180_5:
        cmp r15b, 0
        jne if_13_26_180_5_end
        if_13_29_180_5_code:
            mov rdi, 1
            exit_13_38_180_5:
                    mov rax, 60
                syscall
            exit_13_38_180_5_end:
        if_13_26_180_5_end:
    assert_180_5_end:
    cmp_181_12:
    cmp qword [rsp - 253], -2
    sete r15b
    bool_end_181_12:
    assert_181_5:
        if_13_29_181_5:
        cmp_13_29_181_5:
        cmp r15b, 0
        jne if_13_26_181_5_end
        if_13_29_181_5_code:
            mov rdi, 1
            exit_13_38_181_5:
                    mov rax, 60
                syscall
            exit_13_38_181_5_end:
        if_13_26_181_5_end:
    assert_181_5_end:
    cmp_182_12:
    cmp dword [rsp - 245], 16711680
    sete r15b
    bool_end_182_12:
    assert_182_5:
        if_13_29_182_5:
        cmp_13_29_182_5:
        cmp r15b, 0
        jne if_13_26_182_5_end
        if_13_29_182_5_code:
            mov rdi, 1
            exit_13_38_182_5:
                    mov rax, 60
                syscall
            exit_13_38_182_5_end:
        if_13_26_182_5_end:
    assert_182_5_end:
    mov qword [rsp - 281], 0
    mov qword [rsp - 273], 0
    mov dword [rsp - 265], 0
    mov qword [rsp - 273], 73
    cmp_187_12:
    lea r14, [rsp - 281]
    mov r13, 0
    mov r12, 187
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
    bool_end_187_12:
    assert_187_5:
        if_13_29_187_5:
        cmp_13_29_187_5:
        cmp r15b, 0
        jne if_13_26_187_5_end
        if_13_29_187_5_code:
            mov rdi, 1
            exit_13_38_187_5:
                    mov rax, 60
                syscall
            exit_13_38_187_5_end:
        if_13_26_187_5_end:
    assert_187_5_end:
    mov rcx, 512
    lea rdi, [rsp - 793]
    xor rax, rax
    rep stosb
    lea r15, [rsp - 793]
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
    mov r14, 1
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_191_12:
    lea r14, [rsp - 793]
    mov r13, 1
    mov r12, 191
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 191
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_13_29_191_5:
        cmp_13_29_191_5:
        cmp r15b, 0
        jne if_13_26_191_5_end
        if_13_29_191_5_code:
            mov rdi, 1
            exit_13_38_191_5:
                    mov rax, 60
                syscall
            exit_13_38_191_5_end:
        if_13_26_191_5_end:
    assert_191_5_end:
    mov rcx, 8
    lea r15, [rsp - 793]
    mov r14, 1
    mov r13, 194
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 194
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 793]
    mov r14, 0
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 195
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_200_12:
    lea r14, [rsp - 793]
    mov r13, 0
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_200_12:
    assert_200_5:
        if_13_29_200_5:
        cmp_13_29_200_5:
        cmp r15b, 0
        jne if_13_26_200_5_end
        if_13_29_200_5_code:
            mov rdi, 1
            exit_13_38_200_5:
                    mov rax, 60
                syscall
            exit_13_38_200_5_end:
        if_13_26_200_5_end:
    assert_200_5_end:
    cmp_201_12:
        mov rcx, 8
        lea r13, [rsp - 793]
        mov r12, 0
        mov r11, 202
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 202
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 793]
        mov r12, 1
        mov r11, 203
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 203
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_201_12:
    assert_201_5:
        if_13_29_201_5:
        cmp_13_29_201_5:
        cmp r15b, 0
        jne if_13_26_201_5_end
        if_13_29_201_5_code:
            mov rdi, 1
            exit_13_38_201_5:
                    mov rax, 60
                syscall
            exit_13_38_201_5_end:
        if_13_26_201_5_end:
    assert_201_5_end:
    mov rcx, 128
    lea rdi, [rsp - 921]
    xor rax, rax
    rep stosb
    print_str_208_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        print_24_4_208_5:
                mov rax, 1
                mov rdi, 0
            syscall
        print_24_4_208_5_end:
    print_str_208_5_end:
    loop_209_5:
        print_str_210_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            print_24_4_210_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_24_4_210_9_end:
        print_str_210_9_end:
        str_in_211_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 920]
                mov rdx, 127
            syscall
                mov byte [rsp - 921], al
                sub byte [rsp - 921], 1
        str_in_211_9_end:
        if_212_12:
        cmp_212_12:
        cmp byte [rsp - 921], 0
        jne if_214_19
        if_212_12_code:
            jmp loop_209_5_end
        jmp if_212_9_end
        if_214_19:
        cmp_214_19:
        cmp byte [rsp - 921], 4
        jg if_else_212_9
        if_214_19_code:
            print_str_215_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                print_24_4_215_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_215_13_end:
            print_str_215_13_end:
            jmp loop_209_5
        jmp if_212_9_end
        if_else_212_9:
            print_str_218_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                print_24_4_218_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_218_13_end:
            print_str_218_13_end:
            str_out_219_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 920]
                    movsx rdx, byte [rsp - 921]
                syscall
            str_out_219_13_end:
            print_str_220_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                print_24_4_220_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_220_13_end:
            print_str_220_13_end:
            print_str_221_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                print_24_4_221_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_221_13_end:
            print_str_221_13_end:
        if_212_9_end:
    jmp loop_209_5
    loop_209_5_end:
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
section .rodata
dat:
db '', 10, ''
db '.'
db 'hello '
db 'that is not a name.', 10, ''
db 'enter name:', 10, ''
db 'hello world from baz', 10, ''
dat.len equ $ - dat
```

## With comments

```nasm

;
; generated by baz
;

default rel

section .bss
stk resd 131072
stk.end:

section .text
bits 64
global _start
_start:

; copy data to stack
lea rsi, [dat]
lea rdi, [stk.end]
sub rdi, dat.len
mov rcx, dat.len
cld
rep movsb

; initialize stack pointer
mov rsp, stk.end

;
; program
;

;[1:1] # all functions are inlined
;[3:1] # arguments can be placed in specified register using `reg_...` syntax
;[11:1] # single statement blocks can ommit { ... }
;[36:1] # user types are defined using keyword `type`
;[38:1] # default type is `i64` and does not need to be specified
;[40:1] point : 16B    fields: 
;[40:1]       name :  offset :    size :  array? : array size
;[40:1]          x :       0 :       8 :      no :           
;[40:1]          y :       8 :       8 :      no :           

;[42:1] object : 20B    fields: 
;[42:1]       name :  offset :    size :  array? : array size
;[42:1]        pos :       0 :      16 :      no :           
;[42:1]      color :      16 :       4 :      no :           

;[44:1] world : 64B    fields: 
;[44:1]       name :  offset :    size :  array? : array size
;[44:1]  locations :       0 :      64 :     yes :          8

;[46:1] # function arguments are equivalent to mutable references
;[53:1] # default argument type is `i64`
;[60:1] # return target is specified as a variable, in this case `res`
;[70:1] # array arguments are declared with type and []
;[76:1] str : 128B    fields: 
;[76:1]       name :  offset :    size :  array? : array size
;[76:1]        len :       0 :       1 :      no :           
;[76:1]       data :       1 :     127 :     yes :        127

main:
;   [99:5] dat hello : i8[] = "hello world from baz\n"
;   [99:11] hello: i8[21] (21B @ [rsp - 21])
;   [100:5] dat prompt1 : i8[] = "enter name:\n"
;   [100:9] prompt1: i8[12] (12B @ [rsp - 33])
;   [101:5] dat prompt2 : i8[] = "that is not a name.\n"
;   [101:9] prompt2: i8[20] (20B @ [rsp - 53])
;   [102:5] dat prompt3 : i8[] = "hello "
;   [102:9] prompt3: i8[6] (6B @ [rsp - 59])
;   [103:5] dat dot : i8[] = "."
;   [103:13] dot: i8[1] (1B @ [rsp - 60])
;   [104:5] dat nl : i8[] = "\n"
;   [104:14] nl: i8[1] (1B @ [rsp - 61])
;   [106:5] var arr : i32[4]
;   [106:9] arr: i32[4] (16B @ [rsp - 77])
;   [106:9] clear 4 * 4B = 16B
;   [106:5] size <= 32B, use mov
    mov qword [rsp - 77], 0
    mov qword [rsp - 69], 0
;   [107:5] # arrays are initialized to 0
;   [109:5] var ix = 1
;   [109:9] ix: i64 (8B @ [rsp - 85])
;   [109:9] ix = 1
;   [109:14] 1
    mov qword [rsp - 85], 1
;   [111:5] arr[ix] = 2
;   [111:5] allocate scratch register -> r15
;   [111:9] set array index
;   [111:9] ix
    mov r15, qword [rsp - 85]
;   [111:9] bounds check
;   [111:9] allocate scratch register -> r14
;   [111:9] line number
    mov r14, 111
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [111:9] free scratch register 'r14'
;   [111:15] 2
    mov dword [rsp + r15 * 4 - 77], 2
;   [111:5] free scratch register 'r15'
;   [112:5] arr[ix + 1] = arr[ix]
;   [112:5] allocate scratch register -> r15
;   [112:9] set array index
;   [112:9] ix
    mov r15, qword [rsp - 85]
;   [112:14] r15 + 1
    add r15, 1
;   [112:9] bounds check
;   [112:9] allocate scratch register -> r14
;   [112:9] line number
    mov r14, 112
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [112:9] free scratch register 'r14'
;   [112:19] arr[ix]
;   [112:19] allocate scratch register -> r14
;   [112:23] set array index
;   [112:23] ix
    mov r14, qword [rsp - 85]
;   [112:23] bounds check
;   [112:23] allocate scratch register -> r13
;   [112:23] line number
    mov r13, 112
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [112:23] free scratch register 'r13'
;   [112:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 77]
    mov dword [rsp + r15 * 4 - 77], r13d
;   [112:19] free scratch register 'r13'
;   [112:19] free scratch register 'r14'
;   [112:5] free scratch register 'r15'
;   [113:5] assert(arr[1] == 2)
;   [113:12] allocate scratch register -> r15
;   [113:12] ? arr[1] == 2
;   [113:12] ? arr[1] == 2
    cmp_113_12:
;   [113:12] allocate scratch register -> r14
;   [113:16] set array index
;   [113:16] 1
    mov r14, 1
;   [113:16] bounds check
;   [113:16] allocate scratch register -> r13
;   [113:16] line number
    mov r13, 113
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [113:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 77], 2
;   [113:12] free scratch register 'r14'
    sete r15b
    bool_end_113_12:
;   [13:6] assert(expr : bool) 
    assert_113_5:
;       [113:5] alias expr -> r15b  (lea: )
        if_13_29_113_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_113_5:
        cmp r15b, 0
        jne if_13_26_113_5_end
        if_13_29_113_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_113_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_113_5_end:
        if_13_26_113_5_end:
;       [113:5] free scratch register 'r15'
    assert_113_5_end:
;   [114:5] assert(arr[2] == 2)
;   [114:12] allocate scratch register -> r15
;   [114:12] ? arr[2] == 2
;   [114:12] ? arr[2] == 2
    cmp_114_12:
;   [114:12] allocate scratch register -> r14
;   [114:16] set array index
;   [114:16] 2
    mov r14, 2
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
    cmp dword [rsp + r14 * 4 - 77], 2
;   [114:12] free scratch register 'r14'
    sete r15b
    bool_end_114_12:
;   [13:6] assert(expr : bool) 
    assert_114_5:
;       [114:5] alias expr -> r15b  (lea: )
        if_13_29_114_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_114_5:
        cmp r15b, 0
        jne if_13_26_114_5_end
        if_13_29_114_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_114_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_114_5_end:
        if_13_26_114_5_end:
;       [114:5] free scratch register 'r15'
    assert_114_5_end:
;   [116:5] array_copy(arr[2], arr, 2)
;   [116:5] allocate named register 'rsi'
;   [116:5] allocate named register 'rdi'
;   [116:5] allocate named register 'rcx'
;   [116:29] 2
;   [116:29] 2
    mov rcx, 2
;   [116:16] arr[2]
;   [116:16] allocate scratch register -> r15
;   [116:20] set array index
;   [116:20] 2
    mov r15, 2
;   [116:20] bounds check
;   [116:20] allocate scratch register -> r14
;   [116:20] line number
    mov r14, 116
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [116:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [116:20] free scratch register 'r13'
    cmovg rbp, r14
    jg panic_bounds
;   [116:20] free scratch register 'r14'
    lea rsi, [rsp + r15 * 4 - 77]
;   [116:5] free scratch register 'r15'
;   [116:24] arr
;   [116:24] bounds check
;   [116:24] allocate scratch register -> r15
;   [116:24] line number
    mov r15, 116
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [116:24] free scratch register 'r15'
    lea rdi, [rsp - 77]
    shl rcx, 2
    rep movsb
;   [116:5] free named register 'rcx'
;   [116:5] free named register 'rdi'
;   [116:5] free named register 'rsi'
;   [117:5] # copy from, to, number of elements
;   [118:5] assert(arr[0] == 2)
;   [118:12] allocate scratch register -> r15
;   [118:12] ? arr[0] == 2
;   [118:12] ? arr[0] == 2
    cmp_118_12:
;   [118:12] allocate scratch register -> r14
;   [118:16] set array index
;   [118:16] 0
    mov r14, 0
;   [118:16] bounds check
;   [118:16] allocate scratch register -> r13
;   [118:16] line number
    mov r13, 118
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [118:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 77], 2
;   [118:12] free scratch register 'r14'
    sete r15b
    bool_end_118_12:
;   [13:6] assert(expr : bool) 
    assert_118_5:
;       [118:5] alias expr -> r15b  (lea: )
        if_13_29_118_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_118_5:
        cmp r15b, 0
        jne if_13_26_118_5_end
        if_13_29_118_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_118_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_118_5_end:
        if_13_26_118_5_end:
;       [118:5] free scratch register 'r15'
    assert_118_5_end:
;   [120:5] var arr1 : i32[8]
;   [120:9] arr1: i32[8] (32B @ [rsp - 117])
;   [120:9] clear 8 * 4B = 32B
;   [120:5] size <= 32B, use mov
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
    mov qword [rsp - 93], 0
;   [121:5] array_copy(arr, arr1, 4)
;   [121:5] allocate named register 'rsi'
;   [121:5] allocate named register 'rdi'
;   [121:5] allocate named register 'rcx'
;   [121:27] 4
;   [121:27] 4
    mov rcx, 4
;   [121:16] arr
;   [121:16] bounds check
;   [121:16] allocate scratch register -> r15
;   [121:16] line number
    mov r15, 121
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [121:16] free scratch register 'r15'
    lea rsi, [rsp - 77]
;   [121:21] arr1
;   [121:21] bounds check
;   [121:21] allocate scratch register -> r15
;   [121:21] line number
    mov r15, 121
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [121:21] free scratch register 'r15'
    lea rdi, [rsp - 117]
    shl rcx, 2
    rep movsb
;   [121:5] free named register 'rcx'
;   [121:5] free named register 'rdi'
;   [121:5] free named register 'rsi'
;   [122:5] assert(arrays_equal(arr, arr1, 4))
;   [122:12] allocate scratch register -> r15
;   [122:12] ? arrays_equal(arr, arr1, 4)
;   [122:12] ? arrays_equal(arr, arr1, 4)
    cmp_122_12:
;   [122:12] allocate scratch register -> r14
;       [122:12] r14 = arrays_equal(arr, arr1, 4)
;       [122:12] = expression
;       [122:12] arrays_equal(arr, arr1, 4)
;       [122:12] allocate named register 'rsi'
;       [122:12] allocate named register 'rdi'
;       [122:12] allocate named register 'rcx'
;       [122:36] 4
;       [122:36] 4
        mov rcx, 4
;       [122:25] arr
;       [122:25] bounds check
;       [122:25] allocate scratch register -> r13
;       [122:25] line number
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [122:25] free scratch register 'r13'
        lea rsi, [rsp - 77]
;       [122:30] arr1
;       [122:30] bounds check
;       [122:30] allocate scratch register -> r13
;       [122:30] line number
        mov r13, 122
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [122:30] free scratch register 'r13'
        lea rdi, [rsp - 117]
        shl rcx, 2
        repe cmpsb
;       [122:12] free named register 'rcx'
;       [122:12] free named register 'rdi'
;       [122:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [122:12] free scratch register 'r14'
    setne r15b
    bool_end_122_12:
;   [13:6] assert(expr : bool) 
    assert_122_5:
;       [122:5] alias expr -> r15b  (lea: )
        if_13_29_122_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_122_5:
        cmp r15b, 0
        jne if_13_26_122_5_end
        if_13_29_122_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_122_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_122_5_end:
        if_13_26_122_5_end:
;       [122:5] free scratch register 'r15'
    assert_122_5_end:
;   [123:5] # note: `arrays_equal` is built-in function
;   [125:5] arr1[2] = -1
;   [125:5] allocate scratch register -> r15
;   [125:10] set array index
;   [125:10] 2
    mov r15, 2
;   [125:10] bounds check
;   [125:10] allocate scratch register -> r14
;   [125:10] line number
    mov r14, 125
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [125:10] free scratch register 'r14'
;   [125:16] -1
    mov dword [rsp + r15 * 4 - 117], -1
;   [125:5] free scratch register 'r15'
;   [126:5] assert(not arrays_equal(arr, arr1, 4))
;   [126:12] allocate scratch register -> r15
;   [126:12] ? not arrays_equal(arr, arr1, 4)
;   [126:12] ? not arrays_equal(arr, arr1, 4)
    cmp_126_12:
;   [126:16] allocate scratch register -> r14
;       [126:16] r14 = arrays_equal(arr, arr1, 4)
;       [126:16] = expression
;       [126:16] arrays_equal(arr, arr1, 4)
;       [126:16] allocate named register 'rsi'
;       [126:16] allocate named register 'rdi'
;       [126:16] allocate named register 'rcx'
;       [126:40] 4
;       [126:40] 4
        mov rcx, 4
;       [126:29] arr
;       [126:29] bounds check
;       [126:29] allocate scratch register -> r13
;       [126:29] line number
        mov r13, 126
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [126:29] free scratch register 'r13'
        lea rsi, [rsp - 77]
;       [126:34] arr1
;       [126:34] bounds check
;       [126:34] allocate scratch register -> r13
;       [126:34] line number
        mov r13, 126
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [126:34] free scratch register 'r13'
        lea rdi, [rsp - 117]
        shl rcx, 2
        repe cmpsb
;       [126:16] free named register 'rcx'
;       [126:16] free named register 'rdi'
;       [126:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [126:12] free scratch register 'r14'
    sete r15b
    bool_end_126_12:
;   [13:6] assert(expr : bool) 
    assert_126_5:
;       [126:5] alias expr -> r15b  (lea: )
        if_13_29_126_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_126_5:
        cmp r15b, 0
        jne if_13_26_126_5_end
        if_13_29_126_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_126_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_126_5_end:
        if_13_26_126_5_end:
;       [126:5] free scratch register 'r15'
    assert_126_5_end:
;   [128:5] ix = 3
;   [128:10] 3
    mov qword [rsp - 85], 3
;   [129:5] arr[ix] = ~inv(arr[ix - 1])
;   [129:5] allocate scratch register -> r15
;   [129:9] set array index
;   [129:9] ix
    mov r15, qword [rsp - 85]
;   [129:9] bounds check
;   [129:9] allocate scratch register -> r14
;   [129:9] line number
    mov r14, 129
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [129:9] free scratch register 'r14'
;   [129:16] arr = ~inv(arr[ix - 1])
;   [129:16] = expression
;   [129:16] ~inv(arr[ix - 1])
;   [129:20] allocate scratch register -> r14
;   [129:24] set array index
;   [129:24] ix
    mov r14, qword [rsp - 85]
;   [129:29] r14 - 1
    sub r14, 1
;   [129:24] bounds check
;   [129:24] allocate scratch register -> r13
;   [129:24] line number
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [129:24] free scratch register 'r13'
;   [62:6] inv(i : i32) : i32 res 
    inv_129_16:
;       [129:16] alias res -> dword [rsp + r15 * 4 - 77]  (lea: )
;       [129:16] alias i -> arr  (lea: rsp + r14 * 4 - 77)
;       [63:5] res = ~i
;       [63:12] ~i
;       [63:12] allocate scratch register -> r13
        mov r13d, dword [rsp + r14 * 4 - 77]
        mov dword [rsp + r15 * 4 - 77], r13d
;       [63:12] free scratch register 'r13'
        not dword [rsp + r15 * 4 - 77]
;       [129:16] free scratch register 'r14'
    inv_129_16_end:
    not dword [rsp + r15 * 4 - 77]
;   [129:5] free scratch register 'r15'
;   [130:5] assert(arr[ix] == 2)
;   [130:12] allocate scratch register -> r15
;   [130:12] ? arr[ix] == 2
;   [130:12] ? arr[ix] == 2
    cmp_130_12:
;   [130:12] allocate scratch register -> r14
;   [130:16] set array index
;   [130:16] ix
    mov r14, qword [rsp - 85]
;   [130:16] bounds check
;   [130:16] allocate scratch register -> r13
;   [130:16] line number
    mov r13, 130
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [130:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 77], 2
;   [130:12] free scratch register 'r14'
    sete r15b
    bool_end_130_12:
;   [13:6] assert(expr : bool) 
    assert_130_5:
;       [130:5] alias expr -> r15b  (lea: )
        if_13_29_130_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_130_5:
        cmp r15b, 0
        jne if_13_26_130_5_end
        if_13_29_130_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_130_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_130_5_end:
        if_13_26_130_5_end:
;       [130:5] free scratch register 'r15'
    assert_130_5_end:
;   [132:5] faz(arr)
;   [72:6] faz(arg : i32[]) 
    faz_132_5:
;       [132:5] alias arg -> arr  (lea: )
;       [73:5] arg[1] = 0xfe
;       [73:5] allocate scratch register -> r15
;       [73:9] set array index
;       [73:9] 1
        mov r15, 1
;       [73:9] bounds check
;       [73:9] allocate scratch register -> r14
;       [73:9] line number
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
;       [73:9] free scratch register 'r14'
;       [73:14] 0xfe
        mov dword [rsp + r15 * 4 - 77], 254
;       [73:5] free scratch register 'r15'
    faz_132_5_end:
;   [133:5] assert(arr[1] == 0xfe)
;   [133:12] allocate scratch register -> r15
;   [133:12] ? arr[1] == 0xfe
;   [133:12] ? arr[1] == 0xfe
    cmp_133_12:
;   [133:12] allocate scratch register -> r14
;   [133:16] set array index
;   [133:16] 1
    mov r14, 1
;   [133:16] bounds check
;   [133:16] allocate scratch register -> r13
;   [133:16] line number
    mov r13, 133
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [133:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 77], 254
;   [133:12] free scratch register 'r14'
    sete r15b
    bool_end_133_12:
;   [13:6] assert(expr : bool) 
    assert_133_5:
;       [133:5] alias expr -> r15b  (lea: )
        if_13_29_133_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_133_5:
        cmp r15b, 0
        jne if_13_26_133_5_end
        if_13_29_133_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_133_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_133_5_end:
        if_13_26_133_5_end:
;       [133:5] free scratch register 'r15'
    assert_133_5_end:
;   [135:5] var p : point = {0, 0}
;   [135:9] p: point (16B @ [rsp - 133])
;   [135:9] p = {0, 0}
;   [135:21] copy field 'x'
    mov qword [rsp - 133], 0
;   [135:21] copy field 'y'
    mov qword [rsp - 125], 0
;   [136:5] foo(p)
;   [48:6] foo(pt : point) 
    foo_136_5:
;       [136:5] alias pt -> p  (lea: )
;       [49:5] pt.x = 0b10
;       [49:12] 0b10
        mov qword [rsp - 133], 2
;       [49:20] # binary value 2
;       [50:5] pt.y = 0xb
;       [50:12] 0xb
        mov qword [rsp - 125], 11
;       [50:20] # hex value 11
    foo_136_5_end:
;   [137:5] assert(p.x == 2)
;   [137:12] allocate scratch register -> r15
;   [137:12] ? p.x == 2
;   [137:12] ? p.x == 2
    cmp_137_12:
    cmp qword [rsp - 133], 2
    sete r15b
    bool_end_137_12:
;   [13:6] assert(expr : bool) 
    assert_137_5:
;       [137:5] alias expr -> r15b  (lea: )
        if_13_29_137_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_137_5:
        cmp r15b, 0
        jne if_13_26_137_5_end
        if_13_29_137_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_137_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_137_5_end:
        if_13_26_137_5_end:
;       [137:5] free scratch register 'r15'
    assert_137_5_end:
;   [138:5] assert(p.y == 0xb)
;   [138:12] allocate scratch register -> r15
;   [138:12] ? p.y == 0xb
;   [138:12] ? p.y == 0xb
    cmp_138_12:
    cmp qword [rsp - 125], 11
    sete r15b
    bool_end_138_12:
;   [13:6] assert(expr : bool) 
    assert_138_5:
;       [138:5] alias expr -> r15b  (lea: )
        if_13_29_138_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_138_5:
        cmp r15b, 0
        jne if_13_26_138_5_end
        if_13_29_138_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_138_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_138_5_end:
        if_13_26_138_5_end:
;       [138:5] free scratch register 'r15'
    assert_138_5_end:
;   [140:5] var q : point = p
;   [140:9] q: point (16B @ [rsp - 149])
;   [140:9] q = p
;   [140:21] allocate named register 'rsi'
;   [140:21] allocate named register 'rdi'
;   [140:21] allocate named register 'rcx'
    lea rsi, [rsp - 133]
    lea rdi, [rsp - 149]
    mov rcx, 2
    rep movsq
;   [140:21] free named register 'rcx'
;   [140:21] free named register 'rdi'
;   [140:21] free named register 'rsi'
;   [141:5] assert(equal(p, q))
;   [141:12] allocate scratch register -> r15
;   [141:12] ? equal(p, q)
;   [141:12] ? equal(p, q)
    cmp_141_12:
;   [141:12] allocate scratch register -> r14
;       [141:12] r14 = equal(p, q)
;       [141:12] = expression
;       [141:12] equal(p, q)
;       [141:12] allocate named register 'rsi'
;       [141:12] allocate named register 'rdi'
;       [141:12] allocate named register 'rcx'
;       [141:18] p
        lea rsi, [rsp - 133]
;       [141:21] q
        lea rdi, [rsp - 149]
        mov rcx, 2
        repe cmpsq
;       [141:12] free named register 'rcx'
;       [141:12] free named register 'rdi'
;       [141:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [141:12] free scratch register 'r14'
    setne r15b
    bool_end_141_12:
;   [13:6] assert(expr : bool) 
    assert_141_5:
;       [141:5] alias expr -> r15b  (lea: )
        if_13_29_141_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_141_5:
        cmp r15b, 0
        jne if_13_26_141_5_end
        if_13_29_141_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_141_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_141_5_end:
        if_13_26_141_5_end:
;       [141:5] free scratch register 'r15'
    assert_141_5_end:
;   [142:5] # note: `equal` is built-in function to compare user types for equality
;   [143:5] # or same size arrays
;   [145:5] q.x = 3
;   [145:11] 3
    mov qword [rsp - 149], 3
;   [146:5] assert(not equal(p, q))
;   [146:12] allocate scratch register -> r15
;   [146:12] ? not equal(p, q)
;   [146:12] ? not equal(p, q)
    cmp_146_12:
;   [146:16] allocate scratch register -> r14
;       [146:16] r14 = equal(p, q)
;       [146:16] = expression
;       [146:16] equal(p, q)
;       [146:16] allocate named register 'rsi'
;       [146:16] allocate named register 'rdi'
;       [146:16] allocate named register 'rcx'
;       [146:22] p
        lea rsi, [rsp - 133]
;       [146:25] q
        lea rdi, [rsp - 149]
        mov rcx, 2
        repe cmpsq
;       [146:16] free named register 'rcx'
;       [146:16] free named register 'rdi'
;       [146:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [146:12] free scratch register 'r14'
    sete r15b
    bool_end_146_12:
;   [13:6] assert(expr : bool) 
    assert_146_5:
;       [146:5] alias expr -> r15b  (lea: )
        if_13_29_146_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_146_5:
        cmp r15b, 0
        jne if_13_26_146_5_end
        if_13_29_146_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_146_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_146_5_end:
        if_13_26_146_5_end:
;       [146:5] free scratch register 'r15'
    assert_146_5_end:
;   [148:5] var i = 0
;   [148:9] i: i64 (8B @ [rsp - 157])
;   [148:9] i = 0
;   [148:13] 0
    mov qword [rsp - 157], 0
;   [149:5] bar(i)
;   [55:6] bar(arg) 
    bar_149_5:
;       [149:5] alias arg -> i  (lea: )
        if_56_8_149_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_149_5:
        cmp qword [rsp - 157], 0
        jne if_56_5_149_5_end
        if_56_8_149_5_code:
;           [56:17] return
            jmp bar_149_5_end
        if_56_5_149_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 157], 255
    bar_149_5_end:
;   [150:5] assert(i == 0)
;   [150:12] allocate scratch register -> r15
;   [150:12] ? i == 0
;   [150:12] ? i == 0
    cmp_150_12:
    cmp qword [rsp - 157], 0
    sete r15b
    bool_end_150_12:
;   [13:6] assert(expr : bool) 
    assert_150_5:
;       [150:5] alias expr -> r15b  (lea: )
        if_13_29_150_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_150_5:
        cmp r15b, 0
        jne if_13_26_150_5_end
        if_13_29_150_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_150_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_150_5_end:
        if_13_26_150_5_end:
;       [150:5] free scratch register 'r15'
    assert_150_5_end:
;   [152:5] i = 1
;   [152:9] 1
    mov qword [rsp - 157], 1
;   [153:5] bar(i)
;   [55:6] bar(arg) 
    bar_153_5:
;       [153:5] alias arg -> i  (lea: )
        if_56_8_153_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_153_5:
        cmp qword [rsp - 157], 0
        jne if_56_5_153_5_end
        if_56_8_153_5_code:
;           [56:17] return
            jmp bar_153_5_end
        if_56_5_153_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 157], 255
    bar_153_5_end:
;   [154:5] assert(i == 0xff)
;   [154:12] allocate scratch register -> r15
;   [154:12] ? i == 0xff
;   [154:12] ? i == 0xff
    cmp_154_12:
    cmp qword [rsp - 157], 255
    sete r15b
    bool_end_154_12:
;   [13:6] assert(expr : bool) 
    assert_154_5:
;       [154:5] alias expr -> r15b  (lea: )
        if_13_29_154_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_154_5:
        cmp r15b, 0
        jne if_13_26_154_5_end
        if_13_29_154_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_154_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_154_5_end:
        if_13_26_154_5_end:
;       [154:5] free scratch register 'r15'
    assert_154_5_end:
;   [156:5] var j = 1
;   [156:9] j: i64 (8B @ [rsp - 165])
;   [156:9] j = 1
;   [156:13] 1
    mov qword [rsp - 165], 1
;   [157:5] var k = baz(j)
;   [157:9] k: i64 (8B @ [rsp - 173])
;   [157:9] k = baz(j)
;   [157:13] k = baz(j)
;   [157:13] = expression
;   [157:13] baz(j)
;   [66:6] baz(arg) : i64 res 
    baz_157_13:
;       [157:13] alias res -> qword [rsp - 173]  (lea: )
;       [157:13] alias arg -> j  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, qword [rsp - 165]
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 173], r15
;       [67:11] free scratch register 'r15'
    baz_157_13_end:
;   [158:5] assert(k == 2)
;   [158:12] allocate scratch register -> r15
;   [158:12] ? k == 2
;   [158:12] ? k == 2
    cmp_158_12:
    cmp qword [rsp - 173], 2
    sete r15b
    bool_end_158_12:
;   [13:6] assert(expr : bool) 
    assert_158_5:
;       [158:5] alias expr -> r15b  (lea: )
        if_13_29_158_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_158_5:
        cmp r15b, 0
        jne if_13_26_158_5_end
        if_13_29_158_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_158_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_158_5_end:
        if_13_26_158_5_end:
;       [158:5] free scratch register 'r15'
    assert_158_5_end:
;   [160:5] k = baz(1)
;   [160:9] k = baz(1)
;   [160:9] = expression
;   [160:9] baz(1)
;   [66:6] baz(arg) : i64 res 
    baz_160_9:
;       [160:9] alias res -> qword [rsp - 173]  (lea: )
;       [160:9] alias arg -> 1  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 1
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 173], r15
;       [67:11] free scratch register 'r15'
    baz_160_9_end:
;   [161:5] assert(k == 2)
;   [161:12] allocate scratch register -> r15
;   [161:12] ? k == 2
;   [161:12] ? k == 2
    cmp_161_12:
    cmp qword [rsp - 173], 2
    sete r15b
    bool_end_161_12:
;   [13:6] assert(expr : bool) 
    assert_161_5:
;       [161:5] alias expr -> r15b  (lea: )
        if_13_29_161_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_161_5:
        cmp r15b, 0
        jne if_13_26_161_5_end
        if_13_29_161_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_161_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_161_5_end:
        if_13_26_161_5_end:
;       [161:5] free scratch register 'r15'
    assert_161_5_end:
;   [163:5] var p0 : point = {baz(2), 0}
;   [163:9] p0: point (16B @ [rsp - 189])
;   [163:9] p0 = {baz(2), 0}
;   [163:22] copy field 'x'
;   [163:23] qword [rsp - 189] = baz(2)
;   [163:23] = expression
;   [163:23] baz(2)
;   [66:6] baz(arg) : i64 res 
    baz_163_23:
;       [163:23] alias res -> qword [rsp - 189]  (lea: )
;       [163:23] alias arg -> 2  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 2
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 189], r15
;       [67:11] free scratch register 'r15'
    baz_163_23_end:
;   [163:22] copy field 'y'
    mov qword [rsp - 181], 0
;   [164:5] assert(p0.x == 4)
;   [164:12] allocate scratch register -> r15
;   [164:12] ? p0.x == 4
;   [164:12] ? p0.x == 4
    cmp_164_12:
    cmp qword [rsp - 189], 4
    sete r15b
    bool_end_164_12:
;   [13:6] assert(expr : bool) 
    assert_164_5:
;       [164:5] alias expr -> r15b  (lea: )
        if_13_29_164_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_164_5:
        cmp r15b, 0
        jne if_13_26_164_5_end
        if_13_29_164_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_164_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_164_5_end:
        if_13_26_164_5_end:
;       [164:5] free scratch register 'r15'
    assert_164_5_end:
;   [166:5] var x = 1
;   [166:9] x: i64 (8B @ [rsp - 197])
;   [166:9] x = 1
;   [166:13] 1
    mov qword [rsp - 197], 1
;   [167:5] var y = 2
;   [167:9] y: i64 (8B @ [rsp - 205])
;   [167:9] y = 2
;   [167:13] 2
    mov qword [rsp - 205], 2
;   [169:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [169:9] o1: object (20B @ [rsp - 225])
;   [169:9] o1 = {{x * 10, y}, 0xff0000}
;   [169:23] copy field 'pos'
;   [169:24] copy field 'x'
;   [169:25] allocate scratch register -> r15
;   [169:25] x
    mov r15, qword [rsp - 197]
;   [169:29] r15 * 10
;   [169:29] dst is reg, src is const
    imul r15, 10
    mov qword [rsp - 225], r15
;   [169:25] free scratch register 'r15'
;   [169:24] copy field 'y'
;   [169:33] allocate scratch register -> r15
    mov r15, qword [rsp - 205]
    mov qword [rsp - 217], r15
;   [169:33] free scratch register 'r15'
;   [169:23] copy field 'color'
    mov dword [rsp - 209], 16711680
;   [170:5] assert(o1.pos.x == 10)
;   [170:12] allocate scratch register -> r15
;   [170:12] ? o1.pos.x == 10
;   [170:12] ? o1.pos.x == 10
    cmp_170_12:
    cmp qword [rsp - 225], 10
    sete r15b
    bool_end_170_12:
;   [13:6] assert(expr : bool) 
    assert_170_5:
;       [170:5] alias expr -> r15b  (lea: )
        if_13_29_170_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_170_5:
        cmp r15b, 0
        jne if_13_26_170_5_end
        if_13_29_170_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_170_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_170_5_end:
        if_13_26_170_5_end:
;       [170:5] free scratch register 'r15'
    assert_170_5_end:
;   [171:5] assert(o1.pos.y == 2)
;   [171:12] allocate scratch register -> r15
;   [171:12] ? o1.pos.y == 2
;   [171:12] ? o1.pos.y == 2
    cmp_171_12:
    cmp qword [rsp - 217], 2
    sete r15b
    bool_end_171_12:
;   [13:6] assert(expr : bool) 
    assert_171_5:
;       [171:5] alias expr -> r15b  (lea: )
        if_13_29_171_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_171_5:
        cmp r15b, 0
        jne if_13_26_171_5_end
        if_13_29_171_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_171_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_171_5_end:
        if_13_26_171_5_end:
;       [171:5] free scratch register 'r15'
    assert_171_5_end:
;   [172:5] assert(o1.color == 0xff0000)
;   [172:12] allocate scratch register -> r15
;   [172:12] ? o1.color == 0xff0000
;   [172:12] ? o1.color == 0xff0000
    cmp_172_12:
    cmp dword [rsp - 209], 16711680
    sete r15b
    bool_end_172_12:
;   [13:6] assert(expr : bool) 
    assert_172_5:
;       [172:5] alias expr -> r15b  (lea: )
        if_13_29_172_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_172_5:
        cmp r15b, 0
        jne if_13_26_172_5_end
        if_13_29_172_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_172_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_172_5_end:
        if_13_26_172_5_end:
;       [172:5] free scratch register 'r15'
    assert_172_5_end:
;   [174:5] var p1 : point = {-x, -y}
;   [174:9] p1: point (16B @ [rsp - 241])
;   [174:9] p1 = {-x, -y}
;   [174:22] copy field 'x'
;   [174:23] allocate scratch register -> r15
    mov r15, qword [rsp - 197]
    mov qword [rsp - 241], r15
;   [174:23] free scratch register 'r15'
    neg qword [rsp - 241]
;   [174:22] copy field 'y'
;   [174:27] allocate scratch register -> r15
    mov r15, qword [rsp - 205]
    mov qword [rsp - 233], r15
;   [174:27] free scratch register 'r15'
    neg qword [rsp - 233]
;   [175:5] o1.pos = p1
;   [175:14] allocate named register 'rsi'
;   [175:14] allocate named register 'rdi'
;   [175:14] allocate named register 'rcx'
    lea rsi, [rsp - 241]
    lea rdi, [rsp - 225]
    mov rcx, 2
    rep movsq
;   [175:14] free named register 'rcx'
;   [175:14] free named register 'rdi'
;   [175:14] free named register 'rsi'
;   [176:5] assert(o1.pos.x == -1)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? o1.pos.x == -1
;   [176:12] ? o1.pos.x == -1
    cmp_176_12:
    cmp qword [rsp - 225], -1
    sete r15b
    bool_end_176_12:
;   [13:6] assert(expr : bool) 
    assert_176_5:
;       [176:5] alias expr -> r15b  (lea: )
        if_13_29_176_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_176_5:
        cmp r15b, 0
        jne if_13_26_176_5_end
        if_13_29_176_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_176_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_176_5_end:
        if_13_26_176_5_end:
;       [176:5] free scratch register 'r15'
    assert_176_5_end:
;   [177:5] assert(o1.pos.y == -2)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? o1.pos.y == -2
;   [177:12] ? o1.pos.y == -2
    cmp_177_12:
    cmp qword [rsp - 217], -2
    sete r15b
    bool_end_177_12:
;   [13:6] assert(expr : bool) 
    assert_177_5:
;       [177:5] alias expr -> r15b  (lea: )
        if_13_29_177_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_177_5:
        cmp r15b, 0
        jne if_13_26_177_5_end
        if_13_29_177_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_177_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_177_5_end:
        if_13_26_177_5_end:
;       [177:5] free scratch register 'r15'
    assert_177_5_end:
;   [179:5] var o2 : object = o1
;   [179:9] o2: object (20B @ [rsp - 261])
;   [179:9] o2 = o1
;   [179:23] allocate named register 'rsi'
;   [179:23] allocate named register 'rdi'
;   [179:23] allocate named register 'rcx'
    lea rsi, [rsp - 225]
    lea rdi, [rsp - 261]
    mov rcx, 20
    rep movsb
;   [179:23] free named register 'rcx'
;   [179:23] free named register 'rdi'
;   [179:23] free named register 'rsi'
;   [180:5] assert(o2.pos.x == -1)
;   [180:12] allocate scratch register -> r15
;   [180:12] ? o2.pos.x == -1
;   [180:12] ? o2.pos.x == -1
    cmp_180_12:
    cmp qword [rsp - 261], -1
    sete r15b
    bool_end_180_12:
;   [13:6] assert(expr : bool) 
    assert_180_5:
;       [180:5] alias expr -> r15b  (lea: )
        if_13_29_180_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_180_5:
        cmp r15b, 0
        jne if_13_26_180_5_end
        if_13_29_180_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_180_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_180_5_end:
        if_13_26_180_5_end:
;       [180:5] free scratch register 'r15'
    assert_180_5_end:
;   [181:5] assert(o2.pos.y == -2)
;   [181:12] allocate scratch register -> r15
;   [181:12] ? o2.pos.y == -2
;   [181:12] ? o2.pos.y == -2
    cmp_181_12:
    cmp qword [rsp - 253], -2
    sete r15b
    bool_end_181_12:
;   [13:6] assert(expr : bool) 
    assert_181_5:
;       [181:5] alias expr -> r15b  (lea: )
        if_13_29_181_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_181_5:
        cmp r15b, 0
        jne if_13_26_181_5_end
        if_13_29_181_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_181_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_181_5_end:
        if_13_26_181_5_end:
;       [181:5] free scratch register 'r15'
    assert_181_5_end:
;   [182:5] assert(o2.color == 0xff0000)
;   [182:12] allocate scratch register -> r15
;   [182:12] ? o2.color == 0xff0000
;   [182:12] ? o2.color == 0xff0000
    cmp_182_12:
    cmp dword [rsp - 245], 16711680
    sete r15b
    bool_end_182_12:
;   [13:6] assert(expr : bool) 
    assert_182_5:
;       [182:5] alias expr -> r15b  (lea: )
        if_13_29_182_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_182_5:
        cmp r15b, 0
        jne if_13_26_182_5_end
        if_13_29_182_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_182_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_182_5_end:
        if_13_26_182_5_end:
;       [182:5] free scratch register 'r15'
    assert_182_5_end:
;   [184:5] var o3 : object[1]
;   [184:9] o3: object[1] (20B @ [rsp - 281])
;   [184:9] clear 1 * 20B = 20B
;   [184:5] size <= 32B, use mov
    mov qword [rsp - 281], 0
    mov qword [rsp - 273], 0
    mov dword [rsp - 265], 0
;   [185:5] # index 0 in an array can be accessed without array index
;   [186:5] o3.pos.y = 73
;   [186:16] 73
    mov qword [rsp - 273], 73
;   [187:5] assert(o3[0].pos.y == 73)
;   [187:12] allocate scratch register -> r15
;   [187:12] ? o3[0].pos.y == 73
;   [187:12] ? o3[0].pos.y == 73
    cmp_187_12:
;   [187:12] allocate scratch register -> r14
    lea r14, [rsp - 281]
;   [187:12] allocate scratch register -> r13
;   [187:15] set array index
;   [187:15] 0
    mov r13, 0
;   [187:15] bounds check
;   [187:15] allocate scratch register -> r12
;   [187:15] line number
    mov r12, 187
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
;   [187:15] free scratch register 'r12'
    imul r13, 20
    add r14, r13
;   [187:12] free scratch register 'r13'
    cmp qword [r14 + 8], 73
;   [187:12] free scratch register 'r14'
    sete r15b
    bool_end_187_12:
;   [13:6] assert(expr : bool) 
    assert_187_5:
;       [187:5] alias expr -> r15b  (lea: )
        if_13_29_187_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_187_5:
        cmp r15b, 0
        jne if_13_26_187_5_end
        if_13_29_187_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_187_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_187_5_end:
        if_13_26_187_5_end:
;       [187:5] free scratch register 'r15'
    assert_187_5_end:
;   [189:5] var worlds : world[8]
;   [189:9] worlds: world[8] (512B @ [rsp - 793])
;   [189:9] clear 8 * 64B = 512B
;   [189:5] allocate named register 'rcx'
;   [189:5] allocate named register 'rdi'
;   [189:5] allocate named register 'rax'
    mov rcx, 512
    lea rdi, [rsp - 793]
    xor rax, rax
    rep stosb
;   [189:5] free named register 'rax'
;   [189:5] free named register 'rdi'
;   [189:5] free named register 'rcx'
;   [190:5] worlds[1].locations[1] = 0xffee
;   [190:5] allocate scratch register -> r15
    lea r15, [rsp - 793]
;   [190:5] allocate scratch register -> r14
;   [190:12] set array index
;   [190:12] 1
    mov r14, 1
;   [190:12] bounds check
;   [190:12] allocate scratch register -> r13
;   [190:12] line number
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [190:12] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [190:5] free scratch register 'r14'
;   [190:5] allocate scratch register -> r14
;   [190:25] set array index
;   [190:25] 1
    mov r14, 1
;   [190:25] bounds check
;   [190:25] allocate scratch register -> r13
;   [190:25] line number
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [190:25] free scratch register 'r13'
;   [190:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [190:5] free scratch register 'r14'
;   [190:5] free scratch register 'r15'
;   [191:5] assert(worlds[1].locations[1] == 0xffee)
;   [191:12] allocate scratch register -> r15
;   [191:12] ? worlds[1].locations[1] == 0xffee
;   [191:12] ? worlds[1].locations[1] == 0xffee
    cmp_191_12:
;   [191:12] allocate scratch register -> r14
    lea r14, [rsp - 793]
;   [191:12] allocate scratch register -> r13
;   [191:19] set array index
;   [191:19] 1
    mov r13, 1
;   [191:19] bounds check
;   [191:19] allocate scratch register -> r12
;   [191:19] line number
    mov r12, 191
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [191:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [191:12] free scratch register 'r13'
;   [191:12] allocate scratch register -> r13
;   [191:32] set array index
;   [191:32] 1
    mov r13, 1
;   [191:32] bounds check
;   [191:32] allocate scratch register -> r12
;   [191:32] line number
    mov r12, 191
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [191:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [191:12] free scratch register 'r13'
;   [191:12] free scratch register 'r14'
    sete r15b
    bool_end_191_12:
;   [13:6] assert(expr : bool) 
    assert_191_5:
;       [191:5] alias expr -> r15b  (lea: )
        if_13_29_191_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_191_5:
        cmp r15b, 0
        jne if_13_26_191_5_end
        if_13_29_191_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_191_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_191_5_end:
        if_13_26_191_5_end:
;       [191:5] free scratch register 'r15'
    assert_191_5_end:
;   [193:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [193:5] allocate named register 'rsi'
;   [193:5] allocate named register 'rdi'
;   [193:5] allocate named register 'rcx'
;   [195:9] array_size_of(worlds.locations)
;   [196:9] rcx = array_size_of(worlds.locations)
;   [196:9] = expression
;   [196:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [194:9] worlds[1].locations
;   [194:9] allocate scratch register -> r15
    lea r15, [rsp - 793]
;   [194:9] allocate scratch register -> r14
;   [194:16] set array index
;   [194:16] 1
    mov r14, 1
;   [194:16] bounds check
;   [194:16] allocate scratch register -> r13
;   [194:16] line number
    mov r13, 194
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [194:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [194:9] free scratch register 'r14'
;   [194:9] bounds check
;   [194:9] allocate scratch register -> r14
;   [194:9] line number
    mov r14, 194
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [194:9] free scratch register 'r14'
    lea rsi, [r15]
;   [193:5] free scratch register 'r15'
;   [195:9] worlds[0].locations
;   [195:9] allocate scratch register -> r15
    lea r15, [rsp - 793]
;   [195:9] allocate scratch register -> r14
;   [195:16] set array index
;   [195:16] 0
    mov r14, 0
;   [195:16] bounds check
;   [195:16] allocate scratch register -> r13
;   [195:16] line number
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [195:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [195:9] free scratch register 'r14'
;   [195:9] bounds check
;   [195:9] allocate scratch register -> r14
;   [195:9] line number
    mov r14, 195
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [195:9] free scratch register 'r14'
    lea rdi, [r15]
;   [193:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [193:5] free named register 'rcx'
;   [193:5] free named register 'rdi'
;   [193:5] free named register 'rsi'
;   [198:5] # note: `array_copy` is built-in and can use indexed positions
;   [199:5] # `array_size_of` is built-in
;   [200:5] assert(worlds[0].locations[1] == 0xffee)
;   [200:12] allocate scratch register -> r15
;   [200:12] ? worlds[0].locations[1] == 0xffee
;   [200:12] ? worlds[0].locations[1] == 0xffee
    cmp_200_12:
;   [200:12] allocate scratch register -> r14
    lea r14, [rsp - 793]
;   [200:12] allocate scratch register -> r13
;   [200:19] set array index
;   [200:19] 0
    mov r13, 0
;   [200:19] bounds check
;   [200:19] allocate scratch register -> r12
;   [200:19] line number
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [200:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [200:12] free scratch register 'r13'
;   [200:12] allocate scratch register -> r13
;   [200:32] set array index
;   [200:32] 1
    mov r13, 1
;   [200:32] bounds check
;   [200:32] allocate scratch register -> r12
;   [200:32] line number
    mov r12, 200
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [200:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [200:12] free scratch register 'r13'
;   [200:12] free scratch register 'r14'
    sete r15b
    bool_end_200_12:
;   [13:6] assert(expr : bool) 
    assert_200_5:
;       [200:5] alias expr -> r15b  (lea: )
        if_13_29_200_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_200_5:
        cmp r15b, 0
        jne if_13_26_200_5_end
        if_13_29_200_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_200_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_200_5_end:
        if_13_26_200_5_end:
;       [200:5] free scratch register 'r15'
    assert_200_5_end:
;   [201:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [201:12] allocate scratch register -> r15
;   [201:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [201:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_201_12:
;   [201:12] allocate scratch register -> r14
;       [201:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [201:12] = expression
;       [201:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [201:12] allocate named register 'rsi'
;       [201:12] allocate named register 'rdi'
;       [201:12] allocate named register 'rcx'
;       [203:14] array_size_of(worlds.locations)
;       [204:14] rcx = array_size_of(worlds.locations)
;       [204:14] = expression
;       [204:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [202:14] worlds[0].locations
;       [202:14] allocate scratch register -> r13
        lea r13, [rsp - 793]
;       [202:14] allocate scratch register -> r12
;       [202:21] set array index
;       [202:21] 0
        mov r12, 0
;       [202:21] bounds check
;       [202:21] allocate scratch register -> r11
;       [202:21] line number
        mov r11, 202
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [202:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [202:14] free scratch register 'r12'
;       [202:14] bounds check
;       [202:14] allocate scratch register -> r12
;       [202:14] line number
        mov r12, 202
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [202:14] free scratch register 'r12'
        lea rsi, [r13]
;       [201:12] free scratch register 'r13'
;       [203:14] worlds[1].locations
;       [203:14] allocate scratch register -> r13
        lea r13, [rsp - 793]
;       [203:14] allocate scratch register -> r12
;       [203:21] set array index
;       [203:21] 1
        mov r12, 1
;       [203:21] bounds check
;       [203:21] allocate scratch register -> r11
;       [203:21] line number
        mov r11, 203
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [203:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [203:14] free scratch register 'r12'
;       [203:14] bounds check
;       [203:14] allocate scratch register -> r12
;       [203:14] line number
        mov r12, 203
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [203:14] free scratch register 'r12'
        lea rdi, [r13]
;       [201:12] free scratch register 'r13'
        shl rcx, 3
        repe cmpsb
;       [201:12] free named register 'rcx'
;       [201:12] free named register 'rdi'
;       [201:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [201:12] free scratch register 'r14'
    setne r15b
    bool_end_201_12:
;   [13:6] assert(expr : bool) 
    assert_201_5:
;       [201:5] alias expr -> r15b  (lea: )
        if_13_29_201_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_201_5:
        cmp r15b, 0
        jne if_13_26_201_5_end
        if_13_29_201_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_201_5:
;               [13:38] alias v -> rdi  (lea: )
;               [6:5] mov(rax, 60)
;                   [6:14] 60
                    mov rax, 60
;               [6:19] # exit system call
;               [7:5] mov(rdi, v)
;                   [7:14] v
;               [7:19] # return code
;               [8:5] syscall()
                syscall
;               [13:38] free named register 'rdi'
            exit_13_38_201_5_end:
        if_13_26_201_5_end:
;       [201:5] free scratch register 'r15'
    assert_201_5_end:
;   [207:5] var nm : str
;   [207:9] nm: str (128B @ [rsp - 921])
;   [207:9] clear 1 * 128B = 128B
;   [207:5] allocate named register 'rcx'
;   [207:5] allocate named register 'rdi'
;   [207:5] allocate named register 'rax'
    mov rcx, 128
    lea rdi, [rsp - 921]
    xor rax, rax
    rep stosb
;   [207:5] free named register 'rax'
;   [207:5] free named register 'rdi'
;   [207:5] free named register 'rcx'
;   [208:5] print_str(hello)
;   [23:6] print_str(str : i8[]) 
    print_str_208_5:
;       [208:5] alias str -> hello  (lea: )
;       [24:4] print(array_size_of(str), address_of(str))
;       [24:10] allocate named register 'rdx'
;       [24:10] rdx = array_size_of(str)
;       [24:10] = expression
;       [24:10] array_size_of(str)
        mov rdx, 21
;       [24:30] allocate named register 'rsi'
;       [24:30] rsi = address_of(str)
;       [24:30] = expression
;       [24:30] address_of(str)
        lea rsi, [rsp - 21]
;       [15:6] print(len : reg_rdx, ptr : reg_rsi) 
        print_24_4_208_5:
;           [24:4] alias len -> rdx  (lea: )
;           [24:4] alias ptr -> rsi  (lea: )
;           [16:5] mov(rax, 1)
;               [16:14] 1
                mov rax, 1
;           [16:19] # write system call
;           [17:5] mov(rdi, 0)
;               [17:14] 0
                mov rdi, 0
;           [17:19] # file descriptor for standard out
;           [18:5] mov(rsi, ptr)
;               [18:14] ptr
;           [18:19] # buffer address
;           [19:5] mov(rdx, len)
;               [19:14] len
;           [19:19] # buffer size
;           [20:5] syscall()
            syscall
;           [24:4] free named register 'rsi'
;           [24:4] free named register 'rdx'
        print_24_4_208_5_end:
    print_str_208_5_end:
;   [209:5] loop
    loop_209_5:
;       [210:9] print_str(prompt1)
;       [23:6] print_str(str : i8[]) 
        print_str_210_9:
;           [210:9] alias str -> prompt1  (lea: )
;           [24:4] print(array_size_of(str), address_of(str))
;           [24:10] allocate named register 'rdx'
;           [24:10] rdx = array_size_of(str)
;           [24:10] = expression
;           [24:10] array_size_of(str)
            mov rdx, 12
;           [24:30] allocate named register 'rsi'
;           [24:30] rsi = address_of(str)
;           [24:30] = expression
;           [24:30] address_of(str)
            lea rsi, [rsp - 33]
;           [15:6] print(len : reg_rdx, ptr : reg_rsi) 
            print_24_4_210_9:
;               [24:4] alias len -> rdx  (lea: )
;               [24:4] alias ptr -> rsi  (lea: )
;               [16:5] mov(rax, 1)
;                   [16:14] 1
                    mov rax, 1
;               [16:19] # write system call
;               [17:5] mov(rdi, 0)
;                   [17:14] 0
                    mov rdi, 0
;               [17:19] # file descriptor for standard out
;               [18:5] mov(rsi, ptr)
;                   [18:14] ptr
;               [18:19] # buffer address
;               [19:5] mov(rdx, len)
;                   [19:14] len
;               [19:19] # buffer size
;               [20:5] syscall()
                syscall
;               [24:4] free named register 'rsi'
;               [24:4] free named register 'rdx'
            print_24_4_210_9_end:
        print_str_210_9_end:
;       [211:9] str_in(nm)
;       [81:6] str_in(s : str) 
        str_in_211_9:
;           [211:9] alias s -> nm  (lea: )
;           [82:5] mov(rax, 0)
;               [82:14] 0
                mov rax, 0
;           [82:19] # read system call
;           [83:5] mov(rdi, 0)
;               [83:14] 0
                mov rdi, 0
;           [83:19] # file descriptor for standard input
;           [84:5] mov(rsi, address_of(s.data))
;               [84:14] rsi = address_of(s.data)
;               [84:14] = expression
;               [84:14] address_of(s.data)
                lea rsi, [rsp - 920]
;           [84:34] # buffer address
;           [85:5] mov(rdx, array_size_of(s.data))
;               [85:14] rdx = array_size_of(s.data)
;               [85:14] = expression
;               [85:14] array_size_of(s.data)
                mov rdx, 127
;           [85:37] # buffer size
;           [86:5] syscall()
            syscall
;           [87:5] mov(s.len, rax - 1)
;               [87:16] rax
                mov byte [rsp - 921], al
;               [87:22] s.len - 1
                sub byte [rsp - 921], 1
;           [87:25] # return value
        str_in_211_9_end:
        if_212_12:
;       [212:12] ? nm.len == 0
;       [212:12] ? nm.len == 0
        cmp_212_12:
        cmp byte [rsp - 921], 0
        jne if_214_19
        if_212_12_code:
;           [213:13] break
            jmp loop_209_5_end
        jmp if_212_9_end
        if_214_19:
;       [214:19] ? nm.len <= 4
;       [214:19] ? nm.len <= 4
        cmp_214_19:
        cmp byte [rsp - 921], 4
        jg if_else_212_9
        if_214_19_code:
;           [215:13] print_str(prompt2)
;           [23:6] print_str(str : i8[]) 
            print_str_215_13:
;               [215:13] alias str -> prompt2  (lea: )
;               [24:4] print(array_size_of(str), address_of(str))
;               [24:10] allocate named register 'rdx'
;               [24:10] rdx = array_size_of(str)
;               [24:10] = expression
;               [24:10] array_size_of(str)
                mov rdx, 20
;               [24:30] allocate named register 'rsi'
;               [24:30] rsi = address_of(str)
;               [24:30] = expression
;               [24:30] address_of(str)
                lea rsi, [rsp - 53]
;               [15:6] print(len : reg_rdx, ptr : reg_rsi) 
                print_24_4_215_13:
;                   [24:4] alias len -> rdx  (lea: )
;                   [24:4] alias ptr -> rsi  (lea: )
;                   [16:5] mov(rax, 1)
;                       [16:14] 1
                        mov rax, 1
;                   [16:19] # write system call
;                   [17:5] mov(rdi, 0)
;                       [17:14] 0
                        mov rdi, 0
;                   [17:19] # file descriptor for standard out
;                   [18:5] mov(rsi, ptr)
;                       [18:14] ptr
;                   [18:19] # buffer address
;                   [19:5] mov(rdx, len)
;                       [19:14] len
;                   [19:19] # buffer size
;                   [20:5] syscall()
                    syscall
;                   [24:4] free named register 'rsi'
;                   [24:4] free named register 'rdx'
                print_24_4_215_13_end:
            print_str_215_13_end:
;           [216:13] continue
            jmp loop_209_5
        jmp if_212_9_end
        if_else_212_9:
;           [218:13] print_str(prompt3)
;           [23:6] print_str(str : i8[]) 
            print_str_218_13:
;               [218:13] alias str -> prompt3  (lea: )
;               [24:4] print(array_size_of(str), address_of(str))
;               [24:10] allocate named register 'rdx'
;               [24:10] rdx = array_size_of(str)
;               [24:10] = expression
;               [24:10] array_size_of(str)
                mov rdx, 6
;               [24:30] allocate named register 'rsi'
;               [24:30] rsi = address_of(str)
;               [24:30] = expression
;               [24:30] address_of(str)
                lea rsi, [rsp - 59]
;               [15:6] print(len : reg_rdx, ptr : reg_rsi) 
                print_24_4_218_13:
;                   [24:4] alias len -> rdx  (lea: )
;                   [24:4] alias ptr -> rsi  (lea: )
;                   [16:5] mov(rax, 1)
;                       [16:14] 1
                        mov rax, 1
;                   [16:19] # write system call
;                   [17:5] mov(rdi, 0)
;                       [17:14] 0
                        mov rdi, 0
;                   [17:19] # file descriptor for standard out
;                   [18:5] mov(rsi, ptr)
;                       [18:14] ptr
;                   [18:19] # buffer address
;                   [19:5] mov(rdx, len)
;                       [19:14] len
;                   [19:19] # buffer size
;                   [20:5] syscall()
                    syscall
;                   [24:4] free named register 'rsi'
;                   [24:4] free named register 'rdx'
                print_24_4_218_13_end:
            print_str_218_13_end:
;           [219:13] str_out(nm)
;           [90:6] str_out(s : str) 
            str_out_219_13:
;               [219:13] alias s -> nm  (lea: )
;               [91:5] mov(rax, 1)
;                   [91:14] 1
                    mov rax, 1
;               [91:19] # write system call
;               [92:5] mov(rdi, 0)
;                   [92:14] 0
                    mov rdi, 0
;               [92:19] # file descriptor for standard out
;               [93:5] mov(rsi, address_of(s.data))
;                   [93:14] rsi = address_of(s.data)
;                   [93:14] = expression
;                   [93:14] address_of(s.data)
                    lea rsi, [rsp - 920]
;               [93:34] # buffer address
;               [94:5] mov(rdx, s.len)
;                   [94:14] s.len
                    movsx rdx, byte [rsp - 921]
;               [94:21] # buffer size
;               [95:5] syscall()
                syscall
            str_out_219_13_end:
;           [220:13] print_str(dot)
;           [23:6] print_str(str : i8[]) 
            print_str_220_13:
;               [220:13] alias str -> dot  (lea: )
;               [24:4] print(array_size_of(str), address_of(str))
;               [24:10] allocate named register 'rdx'
;               [24:10] rdx = array_size_of(str)
;               [24:10] = expression
;               [24:10] array_size_of(str)
                mov rdx, 1
;               [24:30] allocate named register 'rsi'
;               [24:30] rsi = address_of(str)
;               [24:30] = expression
;               [24:30] address_of(str)
                lea rsi, [rsp - 60]
;               [15:6] print(len : reg_rdx, ptr : reg_rsi) 
                print_24_4_220_13:
;                   [24:4] alias len -> rdx  (lea: )
;                   [24:4] alias ptr -> rsi  (lea: )
;                   [16:5] mov(rax, 1)
;                       [16:14] 1
                        mov rax, 1
;                   [16:19] # write system call
;                   [17:5] mov(rdi, 0)
;                       [17:14] 0
                        mov rdi, 0
;                   [17:19] # file descriptor for standard out
;                   [18:5] mov(rsi, ptr)
;                       [18:14] ptr
;                   [18:19] # buffer address
;                   [19:5] mov(rdx, len)
;                       [19:14] len
;                   [19:19] # buffer size
;                   [20:5] syscall()
                    syscall
;                   [24:4] free named register 'rsi'
;                   [24:4] free named register 'rdx'
                print_24_4_220_13_end:
            print_str_220_13_end:
;           [221:13] print_str(nl)
;           [23:6] print_str(str : i8[]) 
            print_str_221_13:
;               [221:13] alias str -> nl  (lea: )
;               [24:4] print(array_size_of(str), address_of(str))
;               [24:10] allocate named register 'rdx'
;               [24:10] rdx = array_size_of(str)
;               [24:10] = expression
;               [24:10] array_size_of(str)
                mov rdx, 1
;               [24:30] allocate named register 'rsi'
;               [24:30] rsi = address_of(str)
;               [24:30] = expression
;               [24:30] address_of(str)
                lea rsi, [rsp - 61]
;               [15:6] print(len : reg_rdx, ptr : reg_rsi) 
                print_24_4_221_13:
;                   [24:4] alias len -> rdx  (lea: )
;                   [24:4] alias ptr -> rsi  (lea: )
;                   [16:5] mov(rax, 1)
;                       [16:14] 1
                        mov rax, 1
;                   [16:19] # write system call
;                   [17:5] mov(rdi, 0)
;                       [17:14] 0
                        mov rdi, 0
;                   [17:19] # file descriptor for standard out
;                   [18:5] mov(rsi, ptr)
;                       [18:14] ptr
;                   [18:19] # buffer address
;                   [19:5] mov(rdx, len)
;                       [19:14] len
;                   [19:19] # buffer size
;                   [20:5] syscall()
                    syscall
;                   [24:4] free named register 'rsi'
;                   [24:4] free named register 'rdx'
                print_24_4_221_13_end:
            print_str_221_13_end:
        if_212_9_end:
    jmp loop_209_5
    loop_209_5_end:
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

section .rodata
dat:
; nl: i8[1]
db '', 10, ''
; dot: i8[1]
db '.'
; prompt3: i8[6]
db 'hello '
; prompt2: i8[20]
db 'that is not a name.', 10, ''
; prompt1: i8[12]
db 'enter name:', 10, ''
; hello: i8[21]
db 'hello world from baz', 10, ''
dat.len equ $ - dat

; max scratch registers in use: 5
;            max frames in use: 9
;               max stack size: 921B
;          optimization pass 1: 88
;          optimization pass 2: 0
```
