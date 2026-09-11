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
* keywords: `func`, `type`, `dat`, `var`, `const`, `loop`, `if`, `else`, `continue`,
  `break`, `return`
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`,
  `address_of`, `equal`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
  code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
  run `prog.baz`
  * optional parameters: _stack size_, _bounds check_, with _line number
    information_ and _jump optimizations_ in boolean expression e.g:
    * `./run-baz.sh myprogram.baz --stack=262144`: stack size, no runtime checks
    * `./run-baz.sh myprogram.baz --stack=262144 --check=upper`: checks upper
      bounds without line number information and is often enough to ensure
      catching negative values (faster)
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper,line`: checks
      upper bounds with line number information
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper,lower,line`: checks
      bounds with line number information
    * option `--nopt` disables post processing jump optimizations in boolean
      expression
* to run the tests `qa/coverage/run-tests.sh` and see coverage report in `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    43           1616            817           7305
C++                              1             97            113            584
-------------------------------------------------------------------------------
SUM:                            44           1713            930           7889
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

func sys_print(len : reg_rdx, ptr : reg_rsi) {
    mov(rax, 1)   # write system call
    mov(rdi, 0)   # file descriptor for standard out
    mov(rsi, ptr) # buffer address 
    mov(rdx, len) # buffer size
    syscall()
}

func print(str : i8[]) {
   sys_print(array_size_of(str), address_of(str)) 
}

func sys_read(len : reg_rdx, ptr : reg_rsi) : i64 nbytes {
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

# constants are declared in global scope

const yes = 1
const no = 0
const maybe = -1

func main() {
    dat   hello : i8[] = "hello world from baz\n"
    dat prompt1 : i8[] = "enter name:\n"
    dat prompt2 : i8[] = "that is not a name.\n"
    dat prompt3 : i8[] = "hello "
    dat     dot : i8[] = "."
    dat      nl : i8[] = "\n"

    dat      s1 : str = { 3 }
    # remaining uninitialized fields are zeroed

    # initial data declared before any variables and initialized with constants

    var arr : i32[4]
    # arrays are initialized to 0

    var answer
    assert(answer == 0)
    # variables without initializer are zeroed

    answer = maybe
    assert(answer == -1)

    var ix = 1
    # variables can have an initial value that can be an expression

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
    # `arrays_equal` is built-in function

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
    # `equal` is built-in function to compare user types for equality or same 
    # size arrays

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
    o3.pos.y = 73
    # index 0 in an array can be accessed without array index

    assert(o3[0].pos.y == 73)

    var worlds : world[8]
    worlds[1].locations[1] = 0xffee
    assert(worlds[1].locations[1] == 0xffee)

    array_copy(
        worlds[1].locations,
        worlds[0].locations,
        array_size_of(worlds.locations)
    )
    # `array_copy` is built-in and can use indexed positions
    # `array_size_of` is built-in

    assert(worlds[0].locations[1] == 0xffee)
    assert(arrays_equal(
             worlds[0].locations,
             worlds[1].locations,
             array_size_of(worlds.locations)
          ))

    var nm : str
    print(hello)
    loop {
        print(prompt1)
        str_in(nm)
        if nm.len == 0 {
            break
        } else if nm.len <= 4 {
            print(prompt2)
            continue
        } else {
            print(prompt3)
            str_out(nm)
            print(dot)
            print(nl)
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
    mov qword [rsp - 205], 0
    mov qword [rsp - 197], 0
    mov qword [rsp - 213], 0
    cmp_121_12:
    cmp qword [rsp - 213], 0
    sete r15b
    bool_end_121_12:
    assert_121_5:
        if_13_29_121_5:
        cmp_13_29_121_5:
        cmp r15b, 0
        jne if_13_26_121_5_end
        if_13_29_121_5_code:
            mov rdi, 1
            exit_13_38_121_5:
                    mov rax, 60
                syscall
            exit_13_38_121_5_end:
        if_13_26_121_5_end:
    assert_121_5_end:
    mov qword [rsp - 213], -1
    cmp_125_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_125_12:
    assert_125_5:
        if_13_29_125_5:
        cmp_13_29_125_5:
        cmp r15b, 0
        jne if_13_26_125_5_end
        if_13_29_125_5_code:
            mov rdi, 1
            exit_13_38_125_5:
                    mov rax, 60
                syscall
            exit_13_38_125_5_end:
        if_13_26_125_5_end:
    assert_125_5_end:
    mov qword [rsp - 221], 1
    mov r15, qword [rsp - 221]
    mov r14, 130
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, 131
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    mov r13, 131
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_132_12:
    mov r14, 1
    mov r13, 132
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_132_12:
    assert_132_5:
        if_13_29_132_5:
        cmp_13_29_132_5:
        cmp r15b, 0
        jne if_13_26_132_5_end
        if_13_29_132_5_code:
            mov rdi, 1
            exit_13_38_132_5:
                    mov rax, 60
                syscall
            exit_13_38_132_5_end:
        if_13_26_132_5_end:
    assert_132_5_end:
    cmp_133_12:
    mov r14, 2
    mov r13, 133
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
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
    mov rcx, 2
    mov r15, 2
    mov r14, 135
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 205]
    mov r15, 135
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_137_12:
    mov r14, 0
    mov r13, 137
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
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
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov rcx, 4
    mov r15, 140
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 205]
    mov r15, 140
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_141_12:
        mov rcx, 4
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
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
    mov r15, 2
    mov r14, 144
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 253], -1
    cmp_145_12:
        mov rcx, 4
        mov r13, 145
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 145
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_145_12:
    assert_145_5:
        if_13_29_145_5:
        cmp_13_29_145_5:
        cmp r15b, 0
        jne if_13_26_145_5_end
        if_13_29_145_5_code:
            mov rdi, 1
            exit_13_38_145_5:
                    mov rax, 60
                syscall
            exit_13_38_145_5_end:
        if_13_26_145_5_end:
    assert_145_5_end:
    mov qword [rsp - 221], 3
    mov r15, qword [rsp - 221]
    mov r14, 148
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    sub r14, 1
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_148_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_148_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_149_12:
    mov r14, qword [rsp - 221]
    mov r13, 149
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_149_12:
    assert_149_5:
        if_13_29_149_5:
        cmp_13_29_149_5:
        cmp r15b, 0
        jne if_13_26_149_5_end
        if_13_29_149_5_code:
            mov rdi, 1
            exit_13_38_149_5:
                    mov rax, 60
                syscall
            exit_13_38_149_5_end:
        if_13_26_149_5_end:
    assert_149_5_end:
    faz_151_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 205], 254
    faz_151_5_end:
    cmp_152_12:
    mov r14, 1
    mov r13, 152
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 254
    sete r15b
    bool_end_152_12:
    assert_152_5:
        if_13_29_152_5:
        cmp_13_29_152_5:
        cmp r15b, 0
        jne if_13_26_152_5_end
        if_13_29_152_5_code:
            mov rdi, 1
            exit_13_38_152_5:
                    mov rax, 60
                syscall
            exit_13_38_152_5_end:
        if_13_26_152_5_end:
    assert_152_5_end:
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    foo_155_5:
        mov qword [rsp - 269], 2
        mov qword [rsp - 261], 11
    foo_155_5_end:
    cmp_156_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_13_29_156_5:
        cmp_13_29_156_5:
        cmp r15b, 0
        jne if_13_26_156_5_end
        if_13_29_156_5_code:
            mov rdi, 1
            exit_13_38_156_5:
                    mov rax, 60
                syscall
            exit_13_38_156_5_end:
        if_13_26_156_5_end:
    assert_156_5_end:
    cmp_157_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_13_29_157_5:
        cmp_13_29_157_5:
        cmp r15b, 0
        jne if_13_26_157_5_end
        if_13_29_157_5_code:
            mov rdi, 1
            exit_13_38_157_5:
                    mov rax, 60
                syscall
            exit_13_38_157_5_end:
        if_13_26_157_5_end:
    assert_157_5_end:
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
    cmp_160_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
    bool_end_160_12:
    assert_160_5:
        if_13_29_160_5:
        cmp_13_29_160_5:
        cmp r15b, 0
        jne if_13_26_160_5_end
        if_13_29_160_5_code:
            mov rdi, 1
            exit_13_38_160_5:
                    mov rax, 60
                syscall
            exit_13_38_160_5_end:
        if_13_26_160_5_end:
    assert_160_5_end:
    mov qword [rsp - 285], 3
    cmp_165_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    sete r15b
    bool_end_165_12:
    assert_165_5:
        if_13_29_165_5:
        cmp_13_29_165_5:
        cmp r15b, 0
        jne if_13_26_165_5_end
        if_13_29_165_5_code:
            mov rdi, 1
            exit_13_38_165_5:
                    mov rax, 60
                syscall
            exit_13_38_165_5_end:
        if_13_26_165_5_end:
    assert_165_5_end:
    mov qword [rsp - 293], 0
    bar_168_5:
        if_56_8_168_5:
        cmp_56_8_168_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_168_5_end
        if_56_8_168_5_code:
            jmp bar_168_5_end
        if_56_5_168_5_end:
        mov qword [rsp - 293], 255
    bar_168_5_end:
    cmp_169_12:
    cmp qword [rsp - 293], 0
    sete r15b
    bool_end_169_12:
    assert_169_5:
        if_13_29_169_5:
        cmp_13_29_169_5:
        cmp r15b, 0
        jne if_13_26_169_5_end
        if_13_29_169_5_code:
            mov rdi, 1
            exit_13_38_169_5:
                    mov rax, 60
                syscall
            exit_13_38_169_5_end:
        if_13_26_169_5_end:
    assert_169_5_end:
    mov qword [rsp - 293], 1
    bar_172_5:
        if_56_8_172_5:
        cmp_56_8_172_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_172_5_end
        if_56_8_172_5_code:
            jmp bar_172_5_end
        if_56_5_172_5_end:
        mov qword [rsp - 293], 255
    bar_172_5_end:
    cmp_173_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_13_29_173_5:
        cmp_13_29_173_5:
        cmp r15b, 0
        jne if_13_26_173_5_end
        if_13_29_173_5_code:
            mov rdi, 1
            exit_13_38_173_5:
                    mov rax, 60
                syscall
            exit_13_38_173_5_end:
        if_13_26_173_5_end:
    assert_173_5_end:
    mov qword [rsp - 301], 1
    baz_176_13:
        mov r15, qword [rsp - 301]
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_176_13_end:
    cmp_177_12:
    cmp qword [rsp - 309], 2
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
    baz_179_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_179_9_end:
    cmp_180_12:
    cmp qword [rsp - 309], 2
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
    baz_182_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 325], r15
    baz_182_23_end:
    mov qword [rsp - 317], 0
    cmp_183_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_13_29_183_5:
        cmp_13_29_183_5:
        cmp r15b, 0
        jne if_13_26_183_5_end
        if_13_29_183_5_code:
            mov rdi, 1
            exit_13_38_183_5:
                    mov rax, 60
                syscall
            exit_13_38_183_5_end:
        if_13_26_183_5_end:
    assert_183_5_end:
    mov qword [rsp - 333], 1
    mov qword [rsp - 341], 2
    mov r15, qword [rsp - 333]
    imul r15, 10
    mov qword [rsp - 361], r15
    mov r15, qword [rsp - 341]
    mov qword [rsp - 353], r15
    mov dword [rsp - 345], 16711680
    cmp_189_12:
    cmp qword [rsp - 361], 10
    sete r15b
    bool_end_189_12:
    assert_189_5:
        if_13_29_189_5:
        cmp_13_29_189_5:
        cmp r15b, 0
        jne if_13_26_189_5_end
        if_13_29_189_5_code:
            mov rdi, 1
            exit_13_38_189_5:
                    mov rax, 60
                syscall
            exit_13_38_189_5_end:
        if_13_26_189_5_end:
    assert_189_5_end:
    cmp_190_12:
    cmp qword [rsp - 353], 2
    sete r15b
    bool_end_190_12:
    assert_190_5:
        if_13_29_190_5:
        cmp_13_29_190_5:
        cmp r15b, 0
        jne if_13_26_190_5_end
        if_13_29_190_5_code:
            mov rdi, 1
            exit_13_38_190_5:
                    mov rax, 60
                syscall
            exit_13_38_190_5_end:
        if_13_26_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    cmp dword [rsp - 345], 16711680
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
    mov r15, qword [rsp - 333]
    mov qword [rsp - 377], r15
    neg qword [rsp - 377]
    mov r15, qword [rsp - 341]
    mov qword [rsp - 369], r15
    neg qword [rsp - 369]
    mov rax, qword [rsp - 377]
    mov qword [rsp - 361], rax
    mov rax, qword [rsp - 369]
    mov qword [rsp - 353], rax
    cmp_195_12:
    cmp qword [rsp - 361], -1
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_13_29_195_5:
        cmp_13_29_195_5:
        cmp r15b, 0
        jne if_13_26_195_5_end
        if_13_29_195_5_code:
            mov rdi, 1
            exit_13_38_195_5:
                    mov rax, 60
                syscall
            exit_13_38_195_5_end:
        if_13_26_195_5_end:
    assert_195_5_end:
    cmp_196_12:
    cmp qword [rsp - 353], -2
    sete r15b
    bool_end_196_12:
    assert_196_5:
        if_13_29_196_5:
        cmp_13_29_196_5:
        cmp r15b, 0
        jne if_13_26_196_5_end
        if_13_29_196_5_code:
            mov rdi, 1
            exit_13_38_196_5:
                    mov rax, 60
                syscall
            exit_13_38_196_5_end:
        if_13_26_196_5_end:
    assert_196_5_end:
    lea rsi, [rsp - 361]
    lea rdi, [rsp - 397]
    mov rcx, 20
    rep movsb
    cmp_199_12:
    cmp qword [rsp - 397], -1
    sete r15b
    bool_end_199_12:
    assert_199_5:
        if_13_29_199_5:
        cmp_13_29_199_5:
        cmp r15b, 0
        jne if_13_26_199_5_end
        if_13_29_199_5_code:
            mov rdi, 1
            exit_13_38_199_5:
                    mov rax, 60
                syscall
            exit_13_38_199_5_end:
        if_13_26_199_5_end:
    assert_199_5_end:
    cmp_200_12:
    cmp qword [rsp - 389], -2
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
    cmp dword [rsp - 381], 16711680
    sete r15b
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
    mov qword [rsp - 417], 0
    mov qword [rsp - 409], 0
    mov dword [rsp - 401], 0
    mov qword [rsp - 409], 73
    cmp_207_12:
    lea r14, [rsp - 417]
    mov r13, 0
    mov r12, 207
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
    bool_end_207_12:
    assert_207_5:
        if_13_29_207_5:
        cmp_13_29_207_5:
        cmp r15b, 0
        jne if_13_26_207_5_end
        if_13_29_207_5_code:
            mov rdi, 1
            exit_13_38_207_5:
                    mov rax, 60
                syscall
            exit_13_38_207_5_end:
        if_13_26_207_5_end:
    assert_207_5_end:
    xor al, al
    lea rdi, [rsp - 929]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 929]
    mov r14, 1
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_211_12:
    lea r14, [rsp - 929]
    mov r13, 1
    mov r12, 211
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 211
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_211_12:
    assert_211_5:
        if_13_29_211_5:
        cmp_13_29_211_5:
        cmp r15b, 0
        jne if_13_26_211_5_end
        if_13_29_211_5_code:
            mov rdi, 1
            exit_13_38_211_5:
                    mov rax, 60
                syscall
            exit_13_38_211_5_end:
        if_13_26_211_5_end:
    assert_211_5_end:
    mov rcx, 8
    lea r15, [rsp - 929]
    mov r14, 1
    mov r13, 214
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 214
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 929]
    mov r14, 0
    mov r13, 215
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 215
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_221_12:
    lea r14, [rsp - 929]
    mov r13, 0
    mov r12, 221
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 221
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_13_29_221_5:
        cmp_13_29_221_5:
        cmp r15b, 0
        jne if_13_26_221_5_end
        if_13_29_221_5_code:
            mov rdi, 1
            exit_13_38_221_5:
                    mov rax, 60
                syscall
            exit_13_38_221_5_end:
        if_13_26_221_5_end:
    assert_221_5_end:
    cmp_222_12:
        mov rcx, 8
        lea r13, [rsp - 929]
        mov r12, 0
        mov r11, 223
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 223
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 929]
        mov r12, 1
        mov r11, 224
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 224
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
    bool_end_222_12:
    assert_222_5:
        if_13_29_222_5:
        cmp_13_29_222_5:
        cmp r15b, 0
        jne if_13_26_222_5_end
        if_13_29_222_5_code:
            mov rdi, 1
            exit_13_38_222_5:
                    mov rax, 60
                syscall
            exit_13_38_222_5_end:
        if_13_26_222_5_end:
    assert_222_5_end:
    xor al, al
    lea rdi, [rsp - 1057]
    mov rcx, 128
    rep stosb
    print_229_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_24_4_229_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_24_4_229_5_end:
    print_229_5_end:
    loop_230_5:
        print_231_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_24_4_231_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_24_4_231_9_end:
        print_231_9_end:
        str_in_232_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1056]
                mov rdx, 127
            syscall
                mov byte [rsp - 1057], al
                sub byte [rsp - 1057], 1
        str_in_232_9_end:
        if_233_12:
        cmp_233_12:
        cmp byte [rsp - 1057], 0
        jne if_235_19
        if_233_12_code:
            jmp loop_230_5_end
        jmp if_233_9_end
        if_235_19:
        cmp_235_19:
        cmp byte [rsp - 1057], 4
        jg if_else_233_9
        if_235_19_code:
            print_236_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_24_4_236_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_236_13_end:
            print_236_13_end:
            jmp loop_230_5
        jmp if_233_9_end
        if_else_233_9:
            print_239_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_24_4_239_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_239_13_end:
            print_239_13_end:
            str_out_240_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1056]
                    movsx rdx, byte [rsp - 1057]
                syscall
            str_out_240_13_end:
            print_241_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_24_4_241_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_241_13_end:
            print_241_13_end:
            print_242_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_24_4_242_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_242_13_end:
            print_242_13_end:
        if_233_9_end:
    jmp loop_230_5
    loop_230_5_end:
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
db 3
times 127 db 0
db `\n`
db `.`
db `hello `
db `that is not a name.\n`
db `enter name:\n`
db `hello world from baz\n`
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
;[40:1] point : 16 B    fields:
;[40:1]       name :  offset :    size :  array? : array size
;[40:1]          x :       0 :       8 :      no :           
;[40:1]          y :       8 :       8 :      no :           

;[42:1] object : 20 B    fields:
;[42:1]       name :  offset :    size :  array? : array size
;[42:1]        pos :       0 :      16 :      no :           
;[42:1]      color :      16 :       4 :      no :           

;[44:1] world : 64 B    fields:
;[44:1]       name :  offset :    size :  array? : array size
;[44:1]  locations :       0 :      64 :     yes :          8

;[46:1] # function arguments are equivalent to mutable references
;[53:1] # default argument type is `i64`
;[60:1] # return target is specified as a variable, in this case `res`
;[70:1] # array arguments are declared with type and []
;[76:1] str : 128 B    fields:
;[76:1]       name :  offset :    size :  array? : array size
;[76:1]        len :       0 :       1 :      no :           
;[76:1]       data :       1 :     127 :     yes :        127

;[98:1] # constants are declared in global scope
main:
;   [105:5] dat hello : i8[] = "hello world from baz\n"
;   [105:11] hello: i8[21] (21 B @ [rsp - 21])
;   [106:5] dat prompt1 : i8[] = "enter name:\n"
;   [106:9] prompt1: i8[12] (12 B @ [rsp - 33])
;   [107:5] dat prompt2 : i8[] = "that is not a name.\n"
;   [107:9] prompt2: i8[20] (20 B @ [rsp - 53])
;   [108:5] dat prompt3 : i8[] = "hello "
;   [108:9] prompt3: i8[6] (6 B @ [rsp - 59])
;   [109:5] dat dot : i8[] = "."
;   [109:13] dot: i8[1] (1 B @ [rsp - 60])
;   [110:5] dat nl : i8[] = "\n"
;   [110:14] nl: i8[1] (1 B @ [rsp - 61])
;   [112:5] dat s1 : str = { 3 }
;   [112:14] s1: str (128 B @ [rsp - 189])
;   [113:5] # remaining uninitialized fields are zeroed
;   [115:5] # initial data declared before any variables and initialized with constants
;   [117:5] var arr : i32[4]
;   [117:9] arr: i32[4] (16 B @ [rsp - 205])
;   [117:9] zero 4 * 4 B = 16 B
;   [117:5] size <= 32 B, use mov
    mov qword [rsp - 205], 0
    mov qword [rsp - 197], 0
;   [118:5] # arrays are initialized to 0
;   [120:5] var answer
;   [120:9] answer: i64 (8 B @ [rsp - 213])
;   [120:9] zero 1 * 8 B = 8 B
;   [120:5] size <= 32 B, use mov
    mov qword [rsp - 213], 0
;   [121:5] assert(answer == 0)
;   [121:12] allocate scratch register -> r15
;   [121:12] ? answer == 0
;   [121:12] ? answer == 0
    cmp_121_12:
    cmp qword [rsp - 213], 0
    sete r15b
    bool_end_121_12:
;   [13:6] assert(expr : bool) 
    assert_121_5:
;       [121:5] alias expr -> r15b  (lea: )
        if_13_29_121_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_121_5:
        cmp r15b, 0
        jne if_13_26_121_5_end
        if_13_29_121_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_121_5:
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
            exit_13_38_121_5_end:
        if_13_26_121_5_end:
;       [121:5] free scratch register 'r15'
    assert_121_5_end:
;   [122:5] # variables without initializer are zeroed
;   [124:5] answer = maybe
;   [124:14] maybe
    mov qword [rsp - 213], -1
;   [125:5] assert(answer == -1)
;   [125:12] allocate scratch register -> r15
;   [125:12] ? answer == -1
;   [125:12] ? answer == -1
    cmp_125_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_125_12:
;   [13:6] assert(expr : bool) 
    assert_125_5:
;       [125:5] alias expr -> r15b  (lea: )
        if_13_29_125_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_125_5:
        cmp r15b, 0
        jne if_13_26_125_5_end
        if_13_29_125_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_125_5:
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
            exit_13_38_125_5_end:
        if_13_26_125_5_end:
;       [125:5] free scratch register 'r15'
    assert_125_5_end:
;   [127:5] var ix = 1
;   [127:9] ix: i64 (8 B @ [rsp - 221])
;   [127:9] ix = 1
;   [127:14] 1
    mov qword [rsp - 221], 1
;   [128:5] # variables can have an initial value that can be an expression
;   [130:5] arr[ix] = 2
;   [130:5] allocate scratch register -> r15
;   [130:9] set array index
;   [130:9] ix
    mov r15, qword [rsp - 221]
;   [130:9] bounds check
;   [130:9] allocate scratch register -> r14
;   [130:9] line number
    mov r14, 130
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [130:9] free scratch register 'r14'
;   [130:15] 2
    mov dword [rsp + r15 * 4 - 205], 2
;   [130:5] free scratch register 'r15'
;   [131:5] arr[ix + 1] = arr[ix]
;   [131:5] allocate scratch register -> r15
;   [131:9] set array index
;   [131:9] ix
    mov r15, qword [rsp - 221]
;   [131:14] r15 + 1
    add r15, 1
;   [131:9] bounds check
;   [131:9] allocate scratch register -> r14
;   [131:9] line number
    mov r14, 131
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [131:9] free scratch register 'r14'
;   [131:19] arr[ix]
;   [131:19] allocate scratch register -> r14
;   [131:23] set array index
;   [131:23] ix
    mov r14, qword [rsp - 221]
;   [131:23] bounds check
;   [131:23] allocate scratch register -> r13
;   [131:23] line number
    mov r13, 131
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [131:23] free scratch register 'r13'
;   [131:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
;   [131:19] free scratch register 'r13'
;   [131:19] free scratch register 'r14'
;   [131:5] free scratch register 'r15'
;   [132:5] assert(arr[1] == 2)
;   [132:12] allocate scratch register -> r15
;   [132:12] ? arr[1] == 2
;   [132:12] ? arr[1] == 2
    cmp_132_12:
;   [132:12] allocate scratch register -> r14
;   [132:16] set array index
;   [132:16] 1
    mov r14, 1
;   [132:16] bounds check
;   [132:16] allocate scratch register -> r13
;   [132:16] line number
    mov r13, 132
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [132:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 2
;   [132:12] free scratch register 'r14'
    sete r15b
    bool_end_132_12:
;   [13:6] assert(expr : bool) 
    assert_132_5:
;       [132:5] alias expr -> r15b  (lea: )
        if_13_29_132_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_132_5:
        cmp r15b, 0
        jne if_13_26_132_5_end
        if_13_29_132_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_132_5:
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
            exit_13_38_132_5_end:
        if_13_26_132_5_end:
;       [132:5] free scratch register 'r15'
    assert_132_5_end:
;   [133:5] assert(arr[2] == 2)
;   [133:12] allocate scratch register -> r15
;   [133:12] ? arr[2] == 2
;   [133:12] ? arr[2] == 2
    cmp_133_12:
;   [133:12] allocate scratch register -> r14
;   [133:16] set array index
;   [133:16] 2
    mov r14, 2
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
    cmp dword [rsp + r14 * 4 - 205], 2
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
;   [135:5] array_copy(arr[2], arr, 2)
;   [135:5] allocate named register 'rsi'
;   [135:5] allocate named register 'rdi'
;   [135:5] allocate named register 'rcx'
;   [135:29] 2
;   [135:29] 2
    mov rcx, 2
;   [135:16] arr[2]
;   [135:16] allocate scratch register -> r15
;   [135:20] set array index
;   [135:20] 2
    mov r15, 2
;   [135:20] bounds check
;   [135:20] allocate scratch register -> r14
;   [135:20] line number
    mov r14, 135
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [135:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [135:20] free scratch register 'r13'
    cmovg rbp, r14
    jg panic_bounds
;   [135:20] free scratch register 'r14'
    lea rsi, [rsp + r15 * 4 - 205]
;   [135:5] free scratch register 'r15'
;   [135:24] arr
;   [135:24] bounds check
;   [135:24] allocate scratch register -> r15
;   [135:24] line number
    mov r15, 135
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [135:24] free scratch register 'r15'
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
;   [135:5] free named register 'rcx'
;   [135:5] free named register 'rdi'
;   [135:5] free named register 'rsi'
;   [136:5] # copy from, to, number of elements
;   [137:5] assert(arr[0] == 2)
;   [137:12] allocate scratch register -> r15
;   [137:12] ? arr[0] == 2
;   [137:12] ? arr[0] == 2
    cmp_137_12:
;   [137:12] allocate scratch register -> r14
;   [137:16] set array index
;   [137:16] 0
    mov r14, 0
;   [137:16] bounds check
;   [137:16] allocate scratch register -> r13
;   [137:16] line number
    mov r13, 137
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [137:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 2
;   [137:12] free scratch register 'r14'
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
;   [139:5] var arr1 : i32[8]
;   [139:9] arr1: i32[8] (32 B @ [rsp - 253])
;   [139:9] zero 8 * 4 B = 32 B
;   [139:5] size <= 32 B, use mov
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
;   [140:5] array_copy(arr, arr1, 4)
;   [140:5] allocate named register 'rsi'
;   [140:5] allocate named register 'rdi'
;   [140:5] allocate named register 'rcx'
;   [140:27] 4
;   [140:27] 4
    mov rcx, 4
;   [140:16] arr
;   [140:16] bounds check
;   [140:16] allocate scratch register -> r15
;   [140:16] line number
    mov r15, 140
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [140:16] free scratch register 'r15'
    lea rsi, [rsp - 205]
;   [140:21] arr1
;   [140:21] bounds check
;   [140:21] allocate scratch register -> r15
;   [140:21] line number
    mov r15, 140
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [140:21] free scratch register 'r15'
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
;   [140:5] free named register 'rcx'
;   [140:5] free named register 'rdi'
;   [140:5] free named register 'rsi'
;   [141:5] assert(arrays_equal(arr, arr1, 4))
;   [141:12] allocate scratch register -> r15
;   [141:12] ? arrays_equal(arr, arr1, 4)
;   [141:12] ? arrays_equal(arr, arr1, 4)
    cmp_141_12:
;   [141:12] allocate scratch register -> r14
;       [141:12] r14 = arrays_equal(arr, arr1, 4)
;       [141:12] = expression
;       [141:12] arrays_equal(arr, arr1, 4)
;       [141:12] allocate named register 'rsi'
;       [141:12] allocate named register 'rdi'
;       [141:12] allocate named register 'rcx'
;       [141:36] 4
;       [141:36] 4
        mov rcx, 4
;       [141:25] arr
;       [141:25] bounds check
;       [141:25] allocate scratch register -> r13
;       [141:25] line number
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [141:25] free scratch register 'r13'
        lea rsi, [rsp - 205]
;       [141:30] arr1
;       [141:30] bounds check
;       [141:30] allocate scratch register -> r13
;       [141:30] line number
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [141:30] free scratch register 'r13'
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
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
;   [142:5] # `arrays_equal` is built-in function
;   [144:5] arr1[2] = -1
;   [144:5] allocate scratch register -> r15
;   [144:10] set array index
;   [144:10] 2
    mov r15, 2
;   [144:10] bounds check
;   [144:10] allocate scratch register -> r14
;   [144:10] line number
    mov r14, 144
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [144:10] free scratch register 'r14'
;   [144:16] -1
    mov dword [rsp + r15 * 4 - 253], -1
;   [144:5] free scratch register 'r15'
;   [145:5] assert(not arrays_equal(arr, arr1, 4))
;   [145:12] allocate scratch register -> r15
;   [145:12] ? not arrays_equal(arr, arr1, 4)
;   [145:12] ? not arrays_equal(arr, arr1, 4)
    cmp_145_12:
;   [145:16] allocate scratch register -> r14
;       [145:16] r14 = arrays_equal(arr, arr1, 4)
;       [145:16] = expression
;       [145:16] arrays_equal(arr, arr1, 4)
;       [145:16] allocate named register 'rsi'
;       [145:16] allocate named register 'rdi'
;       [145:16] allocate named register 'rcx'
;       [145:40] 4
;       [145:40] 4
        mov rcx, 4
;       [145:29] arr
;       [145:29] bounds check
;       [145:29] allocate scratch register -> r13
;       [145:29] line number
        mov r13, 145
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [145:29] free scratch register 'r13'
        lea rsi, [rsp - 205]
;       [145:34] arr1
;       [145:34] bounds check
;       [145:34] allocate scratch register -> r13
;       [145:34] line number
        mov r13, 145
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [145:34] free scratch register 'r13'
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
;       [145:16] free named register 'rcx'
;       [145:16] free named register 'rdi'
;       [145:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [145:12] free scratch register 'r14'
    sete r15b
    bool_end_145_12:
;   [13:6] assert(expr : bool) 
    assert_145_5:
;       [145:5] alias expr -> r15b  (lea: )
        if_13_29_145_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_145_5:
        cmp r15b, 0
        jne if_13_26_145_5_end
        if_13_29_145_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_145_5:
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
            exit_13_38_145_5_end:
        if_13_26_145_5_end:
;       [145:5] free scratch register 'r15'
    assert_145_5_end:
;   [147:5] ix = 3
;   [147:10] 3
    mov qword [rsp - 221], 3
;   [148:5] arr[ix] = ~inv(arr[ix - 1])
;   [148:5] allocate scratch register -> r15
;   [148:9] set array index
;   [148:9] ix
    mov r15, qword [rsp - 221]
;   [148:9] bounds check
;   [148:9] allocate scratch register -> r14
;   [148:9] line number
    mov r14, 148
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [148:9] free scratch register 'r14'
;   [148:16] arr = ~inv(arr[ix - 1])
;   [148:16] = expression
;   [148:16] ~inv(arr[ix - 1])
;   [148:20] allocate scratch register -> r14
;   [148:24] set array index
;   [148:24] ix
    mov r14, qword [rsp - 221]
;   [148:29] r14 - 1
    sub r14, 1
;   [148:24] bounds check
;   [148:24] allocate scratch register -> r13
;   [148:24] line number
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [148:24] free scratch register 'r13'
;   [62:6] inv(i : i32) : i32 res 
    inv_148_16:
;       [148:16] alias res -> dword [rsp + r15 * 4 - 205]  (lea: )
;       [148:16] alias i -> arr  (lea: rsp + r14 * 4 - 205)
;       [63:5] res = ~i
;       [63:12] ~i
;       [63:12] allocate scratch register -> r13
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
;       [63:12] free scratch register 'r13'
        not dword [rsp + r15 * 4 - 205]
;       [148:16] free scratch register 'r14'
    inv_148_16_end:
    not dword [rsp + r15 * 4 - 205]
;   [148:5] free scratch register 'r15'
;   [149:5] assert(arr[ix] == 2)
;   [149:12] allocate scratch register -> r15
;   [149:12] ? arr[ix] == 2
;   [149:12] ? arr[ix] == 2
    cmp_149_12:
;   [149:12] allocate scratch register -> r14
;   [149:16] set array index
;   [149:16] ix
    mov r14, qword [rsp - 221]
;   [149:16] bounds check
;   [149:16] allocate scratch register -> r13
;   [149:16] line number
    mov r13, 149
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [149:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 2
;   [149:12] free scratch register 'r14'
    sete r15b
    bool_end_149_12:
;   [13:6] assert(expr : bool) 
    assert_149_5:
;       [149:5] alias expr -> r15b  (lea: )
        if_13_29_149_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_149_5:
        cmp r15b, 0
        jne if_13_26_149_5_end
        if_13_29_149_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_149_5:
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
            exit_13_38_149_5_end:
        if_13_26_149_5_end:
;       [149:5] free scratch register 'r15'
    assert_149_5_end:
;   [151:5] faz(arr)
;   [72:6] faz(arg : i32[]) 
    faz_151_5:
;       [151:5] alias arg -> arr  (lea: )
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
        mov dword [rsp + r15 * 4 - 205], 254
;       [73:5] free scratch register 'r15'
    faz_151_5_end:
;   [152:5] assert(arr[1] == 0xfe)
;   [152:12] allocate scratch register -> r15
;   [152:12] ? arr[1] == 0xfe
;   [152:12] ? arr[1] == 0xfe
    cmp_152_12:
;   [152:12] allocate scratch register -> r14
;   [152:16] set array index
;   [152:16] 1
    mov r14, 1
;   [152:16] bounds check
;   [152:16] allocate scratch register -> r13
;   [152:16] line number
    mov r13, 152
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [152:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 254
;   [152:12] free scratch register 'r14'
    sete r15b
    bool_end_152_12:
;   [13:6] assert(expr : bool) 
    assert_152_5:
;       [152:5] alias expr -> r15b  (lea: )
        if_13_29_152_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_152_5:
        cmp r15b, 0
        jne if_13_26_152_5_end
        if_13_29_152_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_152_5:
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
            exit_13_38_152_5_end:
        if_13_26_152_5_end:
;       [152:5] free scratch register 'r15'
    assert_152_5_end:
;   [154:5] var p : point = {0, 0}
;   [154:9] p: point (16 B @ [rsp - 269])
;   [154:9] p = {0, 0}
;   [154:21] copy field 'x'
    mov qword [rsp - 269], 0
;   [154:21] copy field 'y'
    mov qword [rsp - 261], 0
;   [155:5] foo(p)
;   [48:6] foo(pt : point) 
    foo_155_5:
;       [155:5] alias pt -> p  (lea: )
;       [49:5] pt.x = 0b10
;       [49:12] 0b10
        mov qword [rsp - 269], 2
;       [49:20] # binary value 2
;       [50:5] pt.y = 0xb
;       [50:12] 0xb
        mov qword [rsp - 261], 11
;       [50:20] # hex value 11
    foo_155_5_end:
;   [156:5] assert(p.x == 2)
;   [156:12] allocate scratch register -> r15
;   [156:12] ? p.x == 2
;   [156:12] ? p.x == 2
    cmp_156_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_156_12:
;   [13:6] assert(expr : bool) 
    assert_156_5:
;       [156:5] alias expr -> r15b  (lea: )
        if_13_29_156_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_156_5:
        cmp r15b, 0
        jne if_13_26_156_5_end
        if_13_29_156_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_156_5:
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
            exit_13_38_156_5_end:
        if_13_26_156_5_end:
;       [156:5] free scratch register 'r15'
    assert_156_5_end:
;   [157:5] assert(p.y == 0xb)
;   [157:12] allocate scratch register -> r15
;   [157:12] ? p.y == 0xb
;   [157:12] ? p.y == 0xb
    cmp_157_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_157_12:
;   [13:6] assert(expr : bool) 
    assert_157_5:
;       [157:5] alias expr -> r15b  (lea: )
        if_13_29_157_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_157_5:
        cmp r15b, 0
        jne if_13_26_157_5_end
        if_13_29_157_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_157_5:
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
            exit_13_38_157_5_end:
        if_13_26_157_5_end:
;       [157:5] free scratch register 'r15'
    assert_157_5_end:
;   [159:5] var q : point = p
;   [159:9] q: point (16 B @ [rsp - 285])
;   [159:9] q = p
;   [159:21] size <= 16 B, use mov
;   [159:21] allocate named register 'rax'
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
;   [159:21] free named register 'rax'
;   [160:5] assert(equal(p, q))
;   [160:12] allocate scratch register -> r15
;   [160:12] ? equal(p, q)
;   [160:12] ? equal(p, q)
    cmp_160_12:
;   [160:12] allocate scratch register -> r14
;       [160:12] r14 = equal(p, q)
;       [160:12] = expression
;       [160:12] equal(p, q)
;       [160:12] allocate named register 'rsi'
;       [160:12] allocate named register 'rdi'
;       [160:12] allocate named register 'rcx'
;       [160:18] p
        lea rsi, [rsp - 269]
;       [160:21] q
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
;       [160:12] free named register 'rcx'
;       [160:12] free named register 'rdi'
;       [160:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [160:12] free scratch register 'r14'
    setne r15b
    bool_end_160_12:
;   [13:6] assert(expr : bool) 
    assert_160_5:
;       [160:5] alias expr -> r15b  (lea: )
        if_13_29_160_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_160_5:
        cmp r15b, 0
        jne if_13_26_160_5_end
        if_13_29_160_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_160_5:
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
            exit_13_38_160_5_end:
        if_13_26_160_5_end:
;       [160:5] free scratch register 'r15'
    assert_160_5_end:
;   [161:5] # `equal` is built-in function to compare user types for equality or same
;   [162:5] # size arrays
;   [164:5] q.x = 3
;   [164:11] 3
    mov qword [rsp - 285], 3
;   [165:5] assert(not equal(p, q))
;   [165:12] allocate scratch register -> r15
;   [165:12] ? not equal(p, q)
;   [165:12] ? not equal(p, q)
    cmp_165_12:
;   [165:16] allocate scratch register -> r14
;       [165:16] r14 = equal(p, q)
;       [165:16] = expression
;       [165:16] equal(p, q)
;       [165:16] allocate named register 'rsi'
;       [165:16] allocate named register 'rdi'
;       [165:16] allocate named register 'rcx'
;       [165:22] p
        lea rsi, [rsp - 269]
;       [165:25] q
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
;       [165:16] free named register 'rcx'
;       [165:16] free named register 'rdi'
;       [165:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [165:12] free scratch register 'r14'
    sete r15b
    bool_end_165_12:
;   [13:6] assert(expr : bool) 
    assert_165_5:
;       [165:5] alias expr -> r15b  (lea: )
        if_13_29_165_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_165_5:
        cmp r15b, 0
        jne if_13_26_165_5_end
        if_13_29_165_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_165_5:
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
            exit_13_38_165_5_end:
        if_13_26_165_5_end:
;       [165:5] free scratch register 'r15'
    assert_165_5_end:
;   [167:5] var i = 0
;   [167:9] i: i64 (8 B @ [rsp - 293])
;   [167:9] i = 0
;   [167:13] 0
    mov qword [rsp - 293], 0
;   [168:5] bar(i)
;   [55:6] bar(arg) 
    bar_168_5:
;       [168:5] alias arg -> i  (lea: )
        if_56_8_168_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_168_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_168_5_end
        if_56_8_168_5_code:
;           [56:17] return
            jmp bar_168_5_end
        if_56_5_168_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 293], 255
    bar_168_5_end:
;   [169:5] assert(i == 0)
;   [169:12] allocate scratch register -> r15
;   [169:12] ? i == 0
;   [169:12] ? i == 0
    cmp_169_12:
    cmp qword [rsp - 293], 0
    sete r15b
    bool_end_169_12:
;   [13:6] assert(expr : bool) 
    assert_169_5:
;       [169:5] alias expr -> r15b  (lea: )
        if_13_29_169_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_169_5:
        cmp r15b, 0
        jne if_13_26_169_5_end
        if_13_29_169_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_169_5:
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
            exit_13_38_169_5_end:
        if_13_26_169_5_end:
;       [169:5] free scratch register 'r15'
    assert_169_5_end:
;   [171:5] i = 1
;   [171:9] 1
    mov qword [rsp - 293], 1
;   [172:5] bar(i)
;   [55:6] bar(arg) 
    bar_172_5:
;       [172:5] alias arg -> i  (lea: )
        if_56_8_172_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_172_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_172_5_end
        if_56_8_172_5_code:
;           [56:17] return
            jmp bar_172_5_end
        if_56_5_172_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 293], 255
    bar_172_5_end:
;   [173:5] assert(i == 0xff)
;   [173:12] allocate scratch register -> r15
;   [173:12] ? i == 0xff
;   [173:12] ? i == 0xff
    cmp_173_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_173_12:
;   [13:6] assert(expr : bool) 
    assert_173_5:
;       [173:5] alias expr -> r15b  (lea: )
        if_13_29_173_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_173_5:
        cmp r15b, 0
        jne if_13_26_173_5_end
        if_13_29_173_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_173_5:
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
            exit_13_38_173_5_end:
        if_13_26_173_5_end:
;       [173:5] free scratch register 'r15'
    assert_173_5_end:
;   [175:5] var j = 1
;   [175:9] j: i64 (8 B @ [rsp - 301])
;   [175:9] j = 1
;   [175:13] 1
    mov qword [rsp - 301], 1
;   [176:5] var k = baz(j)
;   [176:9] k: i64 (8 B @ [rsp - 309])
;   [176:9] k = baz(j)
;   [176:13] k = baz(j)
;   [176:13] = expression
;   [176:13] baz(j)
;   [66:6] baz(arg) : i64 res 
    baz_176_13:
;       [176:13] alias res -> qword [rsp - 309]  (lea: )
;       [176:13] alias arg -> j  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, qword [rsp - 301]
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 309], r15
;       [67:11] free scratch register 'r15'
    baz_176_13_end:
;   [177:5] assert(k == 2)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? k == 2
;   [177:12] ? k == 2
    cmp_177_12:
    cmp qword [rsp - 309], 2
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
;   [179:5] k = baz(1)
;   [179:9] k = baz(1)
;   [179:9] = expression
;   [179:9] baz(1)
;   [66:6] baz(arg) : i64 res 
    baz_179_9:
;       [179:9] alias res -> qword [rsp - 309]  (lea: )
;       [179:9] alias arg -> 1  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 1
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 309], r15
;       [67:11] free scratch register 'r15'
    baz_179_9_end:
;   [180:5] assert(k == 2)
;   [180:12] allocate scratch register -> r15
;   [180:12] ? k == 2
;   [180:12] ? k == 2
    cmp_180_12:
    cmp qword [rsp - 309], 2
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
;   [182:5] var p0 : point = {baz(2), 0}
;   [182:9] p0: point (16 B @ [rsp - 325])
;   [182:9] p0 = {baz(2), 0}
;   [182:22] copy field 'x'
;   [182:23] qword [rsp - 325] = baz(2)
;   [182:23] = expression
;   [182:23] baz(2)
;   [66:6] baz(arg) : i64 res 
    baz_182_23:
;       [182:23] alias res -> qword [rsp - 325]  (lea: )
;       [182:23] alias arg -> 2  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 2
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 325], r15
;       [67:11] free scratch register 'r15'
    baz_182_23_end:
;   [182:22] copy field 'y'
    mov qword [rsp - 317], 0
;   [183:5] assert(p0.x == 4)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? p0.x == 4
;   [183:12] ? p0.x == 4
    cmp_183_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_183_12:
;   [13:6] assert(expr : bool) 
    assert_183_5:
;       [183:5] alias expr -> r15b  (lea: )
        if_13_29_183_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_183_5:
        cmp r15b, 0
        jne if_13_26_183_5_end
        if_13_29_183_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_183_5:
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
            exit_13_38_183_5_end:
        if_13_26_183_5_end:
;       [183:5] free scratch register 'r15'
    assert_183_5_end:
;   [185:5] var x = 1
;   [185:9] x: i64 (8 B @ [rsp - 333])
;   [185:9] x = 1
;   [185:13] 1
    mov qword [rsp - 333], 1
;   [186:5] var y = 2
;   [186:9] y: i64 (8 B @ [rsp - 341])
;   [186:9] y = 2
;   [186:13] 2
    mov qword [rsp - 341], 2
;   [188:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [188:9] o1: object (20 B @ [rsp - 361])
;   [188:9] o1 = {{x * 10, y}, 0xff0000}
;   [188:23] copy field 'pos'
;   [188:24] copy field 'x'
;   [188:25] allocate scratch register -> r15
;   [188:25] x
    mov r15, qword [rsp - 333]
;   [188:29] r15 * 10
;   [188:29] dst is reg, src is const
    imul r15, 10
    mov qword [rsp - 361], r15
;   [188:25] free scratch register 'r15'
;   [188:24] copy field 'y'
;   [188:33] allocate scratch register -> r15
    mov r15, qword [rsp - 341]
    mov qword [rsp - 353], r15
;   [188:33] free scratch register 'r15'
;   [188:23] copy field 'color'
    mov dword [rsp - 345], 16711680
;   [189:5] assert(o1.pos.x == 10)
;   [189:12] allocate scratch register -> r15
;   [189:12] ? o1.pos.x == 10
;   [189:12] ? o1.pos.x == 10
    cmp_189_12:
    cmp qword [rsp - 361], 10
    sete r15b
    bool_end_189_12:
;   [13:6] assert(expr : bool) 
    assert_189_5:
;       [189:5] alias expr -> r15b  (lea: )
        if_13_29_189_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_189_5:
        cmp r15b, 0
        jne if_13_26_189_5_end
        if_13_29_189_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_189_5:
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
            exit_13_38_189_5_end:
        if_13_26_189_5_end:
;       [189:5] free scratch register 'r15'
    assert_189_5_end:
;   [190:5] assert(o1.pos.y == 2)
;   [190:12] allocate scratch register -> r15
;   [190:12] ? o1.pos.y == 2
;   [190:12] ? o1.pos.y == 2
    cmp_190_12:
    cmp qword [rsp - 353], 2
    sete r15b
    bool_end_190_12:
;   [13:6] assert(expr : bool) 
    assert_190_5:
;       [190:5] alias expr -> r15b  (lea: )
        if_13_29_190_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_190_5:
        cmp r15b, 0
        jne if_13_26_190_5_end
        if_13_29_190_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_190_5:
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
            exit_13_38_190_5_end:
        if_13_26_190_5_end:
;       [190:5] free scratch register 'r15'
    assert_190_5_end:
;   [191:5] assert(o1.color == 0xff0000)
;   [191:12] allocate scratch register -> r15
;   [191:12] ? o1.color == 0xff0000
;   [191:12] ? o1.color == 0xff0000
    cmp_191_12:
    cmp dword [rsp - 345], 16711680
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
;   [193:5] var p1 : point = {-x, -y}
;   [193:9] p1: point (16 B @ [rsp - 377])
;   [193:9] p1 = {-x, -y}
;   [193:22] copy field 'x'
;   [193:23] allocate scratch register -> r15
    mov r15, qword [rsp - 333]
    mov qword [rsp - 377], r15
;   [193:23] free scratch register 'r15'
    neg qword [rsp - 377]
;   [193:22] copy field 'y'
;   [193:27] allocate scratch register -> r15
    mov r15, qword [rsp - 341]
    mov qword [rsp - 369], r15
;   [193:27] free scratch register 'r15'
    neg qword [rsp - 369]
;   [194:5] o1.pos = p1
;   [194:14] size <= 16 B, use mov
;   [194:14] allocate named register 'rax'
    mov rax, qword [rsp - 377]
    mov qword [rsp - 361], rax
    mov rax, qword [rsp - 369]
    mov qword [rsp - 353], rax
;   [194:14] free named register 'rax'
;   [195:5] assert(o1.pos.x == -1)
;   [195:12] allocate scratch register -> r15
;   [195:12] ? o1.pos.x == -1
;   [195:12] ? o1.pos.x == -1
    cmp_195_12:
    cmp qword [rsp - 361], -1
    sete r15b
    bool_end_195_12:
;   [13:6] assert(expr : bool) 
    assert_195_5:
;       [195:5] alias expr -> r15b  (lea: )
        if_13_29_195_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_195_5:
        cmp r15b, 0
        jne if_13_26_195_5_end
        if_13_29_195_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_195_5:
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
            exit_13_38_195_5_end:
        if_13_26_195_5_end:
;       [195:5] free scratch register 'r15'
    assert_195_5_end:
;   [196:5] assert(o1.pos.y == -2)
;   [196:12] allocate scratch register -> r15
;   [196:12] ? o1.pos.y == -2
;   [196:12] ? o1.pos.y == -2
    cmp_196_12:
    cmp qword [rsp - 353], -2
    sete r15b
    bool_end_196_12:
;   [13:6] assert(expr : bool) 
    assert_196_5:
;       [196:5] alias expr -> r15b  (lea: )
        if_13_29_196_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_196_5:
        cmp r15b, 0
        jne if_13_26_196_5_end
        if_13_29_196_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_196_5:
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
            exit_13_38_196_5_end:
        if_13_26_196_5_end:
;       [196:5] free scratch register 'r15'
    assert_196_5_end:
;   [198:5] var o2 : object = o1
;   [198:9] o2: object (20 B @ [rsp - 397])
;   [198:9] o2 = o1
;   [198:23] allocate named register 'rsi'
;   [198:23] allocate named register 'rdi'
;   [198:23] allocate named register 'rcx'
    lea rsi, [rsp - 361]
    lea rdi, [rsp - 397]
    mov rcx, 20
    rep movsb
;   [198:23] free named register 'rcx'
;   [198:23] free named register 'rdi'
;   [198:23] free named register 'rsi'
;   [199:5] assert(o2.pos.x == -1)
;   [199:12] allocate scratch register -> r15
;   [199:12] ? o2.pos.x == -1
;   [199:12] ? o2.pos.x == -1
    cmp_199_12:
    cmp qword [rsp - 397], -1
    sete r15b
    bool_end_199_12:
;   [13:6] assert(expr : bool) 
    assert_199_5:
;       [199:5] alias expr -> r15b  (lea: )
        if_13_29_199_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_199_5:
        cmp r15b, 0
        jne if_13_26_199_5_end
        if_13_29_199_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_199_5:
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
            exit_13_38_199_5_end:
        if_13_26_199_5_end:
;       [199:5] free scratch register 'r15'
    assert_199_5_end:
;   [200:5] assert(o2.pos.y == -2)
;   [200:12] allocate scratch register -> r15
;   [200:12] ? o2.pos.y == -2
;   [200:12] ? o2.pos.y == -2
    cmp_200_12:
    cmp qword [rsp - 389], -2
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
;   [201:5] assert(o2.color == 0xff0000)
;   [201:12] allocate scratch register -> r15
;   [201:12] ? o2.color == 0xff0000
;   [201:12] ? o2.color == 0xff0000
    cmp_201_12:
    cmp dword [rsp - 381], 16711680
    sete r15b
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
;   [203:5] var o3 : object[1]
;   [203:9] o3: object[1] (20 B @ [rsp - 417])
;   [203:9] zero 1 * 20 B = 20 B
;   [203:5] size <= 32 B, use mov
    mov qword [rsp - 417], 0
    mov qword [rsp - 409], 0
    mov dword [rsp - 401], 0
;   [204:5] o3.pos.y = 73
;   [204:16] 73
    mov qword [rsp - 409], 73
;   [205:5] # index 0 in an array can be accessed without array index
;   [207:5] assert(o3[0].pos.y == 73)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? o3[0].pos.y == 73
;   [207:12] ? o3[0].pos.y == 73
    cmp_207_12:
;   [207:12] allocate scratch register -> r14
    lea r14, [rsp - 417]
;   [207:12] allocate scratch register -> r13
;   [207:15] set array index
;   [207:15] 0
    mov r13, 0
;   [207:15] bounds check
;   [207:15] allocate scratch register -> r12
;   [207:15] line number
    mov r12, 207
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
;   [207:15] free scratch register 'r12'
    imul r13, 20
    add r14, r13
;   [207:12] free scratch register 'r13'
    cmp qword [r14 + 8], 73
;   [207:12] free scratch register 'r14'
    sete r15b
    bool_end_207_12:
;   [13:6] assert(expr : bool) 
    assert_207_5:
;       [207:5] alias expr -> r15b  (lea: )
        if_13_29_207_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_207_5:
        cmp r15b, 0
        jne if_13_26_207_5_end
        if_13_29_207_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_207_5:
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
            exit_13_38_207_5_end:
        if_13_26_207_5_end:
;       [207:5] free scratch register 'r15'
    assert_207_5_end:
;   [209:5] var worlds : world[8]
;   [209:9] worlds: world[8] (512 B @ [rsp - 929])
;   [209:9] zero 8 * 64 B = 512 B
;   [209:5] allocate named register 'rax'
;   [209:5] allocate named register 'rdi'
;   [209:5] allocate named register 'rcx'
    xor al, al
    lea rdi, [rsp - 929]
    mov rcx, 512
    rep stosb
;   [209:5] free named register 'rcx'
;   [209:5] free named register 'rdi'
;   [209:5] free named register 'rax'
;   [210:5] worlds[1].locations[1] = 0xffee
;   [210:5] allocate scratch register -> r15
    lea r15, [rsp - 929]
;   [210:5] allocate scratch register -> r14
;   [210:12] set array index
;   [210:12] 1
    mov r14, 1
;   [210:12] bounds check
;   [210:12] allocate scratch register -> r13
;   [210:12] line number
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [210:12] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [210:5] free scratch register 'r14'
;   [210:5] allocate scratch register -> r14
;   [210:25] set array index
;   [210:25] 1
    mov r14, 1
;   [210:25] bounds check
;   [210:25] allocate scratch register -> r13
;   [210:25] line number
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [210:25] free scratch register 'r13'
;   [210:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [210:5] free scratch register 'r14'
;   [210:5] free scratch register 'r15'
;   [211:5] assert(worlds[1].locations[1] == 0xffee)
;   [211:12] allocate scratch register -> r15
;   [211:12] ? worlds[1].locations[1] == 0xffee
;   [211:12] ? worlds[1].locations[1] == 0xffee
    cmp_211_12:
;   [211:12] allocate scratch register -> r14
    lea r14, [rsp - 929]
;   [211:12] allocate scratch register -> r13
;   [211:19] set array index
;   [211:19] 1
    mov r13, 1
;   [211:19] bounds check
;   [211:19] allocate scratch register -> r12
;   [211:19] line number
    mov r12, 211
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [211:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [211:12] free scratch register 'r13'
;   [211:12] allocate scratch register -> r13
;   [211:32] set array index
;   [211:32] 1
    mov r13, 1
;   [211:32] bounds check
;   [211:32] allocate scratch register -> r12
;   [211:32] line number
    mov r12, 211
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [211:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [211:12] free scratch register 'r13'
;   [211:12] free scratch register 'r14'
    sete r15b
    bool_end_211_12:
;   [13:6] assert(expr : bool) 
    assert_211_5:
;       [211:5] alias expr -> r15b  (lea: )
        if_13_29_211_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_211_5:
        cmp r15b, 0
        jne if_13_26_211_5_end
        if_13_29_211_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_211_5:
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
            exit_13_38_211_5_end:
        if_13_26_211_5_end:
;       [211:5] free scratch register 'r15'
    assert_211_5_end:
;   [213:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [213:5] allocate named register 'rsi'
;   [213:5] allocate named register 'rdi'
;   [213:5] allocate named register 'rcx'
;   [215:9] array_size_of(worlds.locations)
;   [216:9] rcx = array_size_of(worlds.locations)
;   [216:9] = expression
;   [216:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [214:9] worlds[1].locations
;   [214:9] allocate scratch register -> r15
    lea r15, [rsp - 929]
;   [214:9] allocate scratch register -> r14
;   [214:16] set array index
;   [214:16] 1
    mov r14, 1
;   [214:16] bounds check
;   [214:16] allocate scratch register -> r13
;   [214:16] line number
    mov r13, 214
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [214:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [214:9] free scratch register 'r14'
;   [214:9] bounds check
;   [214:9] allocate scratch register -> r14
;   [214:9] line number
    mov r14, 214
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [214:9] free scratch register 'r14'
    lea rsi, [r15]
;   [213:5] free scratch register 'r15'
;   [215:9] worlds[0].locations
;   [215:9] allocate scratch register -> r15
    lea r15, [rsp - 929]
;   [215:9] allocate scratch register -> r14
;   [215:16] set array index
;   [215:16] 0
    mov r14, 0
;   [215:16] bounds check
;   [215:16] allocate scratch register -> r13
;   [215:16] line number
    mov r13, 215
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [215:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [215:9] free scratch register 'r14'
;   [215:9] bounds check
;   [215:9] allocate scratch register -> r14
;   [215:9] line number
    mov r14, 215
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [215:9] free scratch register 'r14'
    lea rdi, [r15]
;   [213:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [213:5] free named register 'rcx'
;   [213:5] free named register 'rdi'
;   [213:5] free named register 'rsi'
;   [218:5] # `array_copy` is built-in and can use indexed positions
;   [219:5] # `array_size_of` is built-in
;   [221:5] assert(worlds[0].locations[1] == 0xffee)
;   [221:12] allocate scratch register -> r15
;   [221:12] ? worlds[0].locations[1] == 0xffee
;   [221:12] ? worlds[0].locations[1] == 0xffee
    cmp_221_12:
;   [221:12] allocate scratch register -> r14
    lea r14, [rsp - 929]
;   [221:12] allocate scratch register -> r13
;   [221:19] set array index
;   [221:19] 0
    mov r13, 0
;   [221:19] bounds check
;   [221:19] allocate scratch register -> r12
;   [221:19] line number
    mov r12, 221
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [221:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [221:12] free scratch register 'r13'
;   [221:12] allocate scratch register -> r13
;   [221:32] set array index
;   [221:32] 1
    mov r13, 1
;   [221:32] bounds check
;   [221:32] allocate scratch register -> r12
;   [221:32] line number
    mov r12, 221
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [221:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [221:12] free scratch register 'r13'
;   [221:12] free scratch register 'r14'
    sete r15b
    bool_end_221_12:
;   [13:6] assert(expr : bool) 
    assert_221_5:
;       [221:5] alias expr -> r15b  (lea: )
        if_13_29_221_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_221_5:
        cmp r15b, 0
        jne if_13_26_221_5_end
        if_13_29_221_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_221_5:
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
            exit_13_38_221_5_end:
        if_13_26_221_5_end:
;       [221:5] free scratch register 'r15'
    assert_221_5_end:
;   [222:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [222:12] allocate scratch register -> r15
;   [222:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [222:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_222_12:
;   [222:12] allocate scratch register -> r14
;       [222:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [222:12] = expression
;       [222:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [222:12] allocate named register 'rsi'
;       [222:12] allocate named register 'rdi'
;       [222:12] allocate named register 'rcx'
;       [224:14] array_size_of(worlds.locations)
;       [225:14] rcx = array_size_of(worlds.locations)
;       [225:14] = expression
;       [225:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [223:14] worlds[0].locations
;       [223:14] allocate scratch register -> r13
        lea r13, [rsp - 929]
;       [223:14] allocate scratch register -> r12
;       [223:21] set array index
;       [223:21] 0
        mov r12, 0
;       [223:21] bounds check
;       [223:21] allocate scratch register -> r11
;       [223:21] line number
        mov r11, 223
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [223:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [223:14] free scratch register 'r12'
;       [223:14] bounds check
;       [223:14] allocate scratch register -> r12
;       [223:14] line number
        mov r12, 223
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [223:14] free scratch register 'r12'
        lea rsi, [r13]
;       [222:12] free scratch register 'r13'
;       [224:14] worlds[1].locations
;       [224:14] allocate scratch register -> r13
        lea r13, [rsp - 929]
;       [224:14] allocate scratch register -> r12
;       [224:21] set array index
;       [224:21] 1
        mov r12, 1
;       [224:21] bounds check
;       [224:21] allocate scratch register -> r11
;       [224:21] line number
        mov r11, 224
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [224:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [224:14] free scratch register 'r12'
;       [224:14] bounds check
;       [224:14] allocate scratch register -> r12
;       [224:14] line number
        mov r12, 224
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [224:14] free scratch register 'r12'
        lea rdi, [r13]
;       [222:12] free scratch register 'r13'
        shl rcx, 3
        repe cmpsb
;       [222:12] free named register 'rcx'
;       [222:12] free named register 'rdi'
;       [222:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [222:12] free scratch register 'r14'
    setne r15b
    bool_end_222_12:
;   [13:6] assert(expr : bool) 
    assert_222_5:
;       [222:5] alias expr -> r15b  (lea: )
        if_13_29_222_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_222_5:
        cmp r15b, 0
        jne if_13_26_222_5_end
        if_13_29_222_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_222_5:
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
            exit_13_38_222_5_end:
        if_13_26_222_5_end:
;       [222:5] free scratch register 'r15'
    assert_222_5_end:
;   [228:5] var nm : str
;   [228:9] nm: str (128 B @ [rsp - 1057])
;   [228:9] zero 1 * 128 B = 128 B
;   [228:5] allocate named register 'rax'
;   [228:5] allocate named register 'rdi'
;   [228:5] allocate named register 'rcx'
    xor al, al
    lea rdi, [rsp - 1057]
    mov rcx, 128
    rep stosb
;   [228:5] free named register 'rcx'
;   [228:5] free named register 'rdi'
;   [228:5] free named register 'rax'
;   [229:5] print(hello)
;   [23:6] print(str : i8[]) 
    print_229_5:
;       [229:5] alias str -> hello  (lea: )
;       [24:4] sys_print(array_size_of(str), address_of(str))
;       [24:14] allocate named register 'rdx'
;       [24:14] rdx = array_size_of(str)
;       [24:14] = expression
;       [24:14] array_size_of(str)
        mov rdx, 21
;       [24:34] allocate named register 'rsi'
;       [24:34] rsi = address_of(str)
;       [24:34] = expression
;       [24:34] address_of(str)
        lea rsi, [rsp - 21]
;       [15:6] sys_print(len : reg_rdx, ptr : reg_rsi) 
        sys_print_24_4_229_5:
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
        sys_print_24_4_229_5_end:
    print_229_5_end:
;   [230:5] loop
    loop_230_5:
;       [231:9] print(prompt1)
;       [23:6] print(str : i8[]) 
        print_231_9:
;           [231:9] alias str -> prompt1  (lea: )
;           [24:4] sys_print(array_size_of(str), address_of(str))
;           [24:14] allocate named register 'rdx'
;           [24:14] rdx = array_size_of(str)
;           [24:14] = expression
;           [24:14] array_size_of(str)
            mov rdx, 12
;           [24:34] allocate named register 'rsi'
;           [24:34] rsi = address_of(str)
;           [24:34] = expression
;           [24:34] address_of(str)
            lea rsi, [rsp - 33]
;           [15:6] sys_print(len : reg_rdx, ptr : reg_rsi) 
            sys_print_24_4_231_9:
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
            sys_print_24_4_231_9_end:
        print_231_9_end:
;       [232:9] str_in(nm)
;       [81:6] str_in(s : str) 
        str_in_232_9:
;           [232:9] alias s -> nm  (lea: )
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
                lea rsi, [rsp - 1056]
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
                mov byte [rsp - 1057], al
;               [87:22] s.len - 1
                sub byte [rsp - 1057], 1
;           [87:25] # return value
        str_in_232_9_end:
        if_233_12:
;       [233:12] ? nm.len == 0
;       [233:12] ? nm.len == 0
        cmp_233_12:
        cmp byte [rsp - 1057], 0
        jne if_235_19
        if_233_12_code:
;           [234:13] break
            jmp loop_230_5_end
        jmp if_233_9_end
        if_235_19:
;       [235:19] ? nm.len <= 4
;       [235:19] ? nm.len <= 4
        cmp_235_19:
        cmp byte [rsp - 1057], 4
        jg if_else_233_9
        if_235_19_code:
;           [236:13] print(prompt2)
;           [23:6] print(str : i8[]) 
            print_236_13:
;               [236:13] alias str -> prompt2  (lea: )
;               [24:4] sys_print(array_size_of(str), address_of(str))
;               [24:14] allocate named register 'rdx'
;               [24:14] rdx = array_size_of(str)
;               [24:14] = expression
;               [24:14] array_size_of(str)
                mov rdx, 20
;               [24:34] allocate named register 'rsi'
;               [24:34] rsi = address_of(str)
;               [24:34] = expression
;               [24:34] address_of(str)
                lea rsi, [rsp - 53]
;               [15:6] sys_print(len : reg_rdx, ptr : reg_rsi) 
                sys_print_24_4_236_13:
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
                sys_print_24_4_236_13_end:
            print_236_13_end:
;           [237:13] continue
            jmp loop_230_5
        jmp if_233_9_end
        if_else_233_9:
;           [239:13] print(prompt3)
;           [23:6] print(str : i8[]) 
            print_239_13:
;               [239:13] alias str -> prompt3  (lea: )
;               [24:4] sys_print(array_size_of(str), address_of(str))
;               [24:14] allocate named register 'rdx'
;               [24:14] rdx = array_size_of(str)
;               [24:14] = expression
;               [24:14] array_size_of(str)
                mov rdx, 6
;               [24:34] allocate named register 'rsi'
;               [24:34] rsi = address_of(str)
;               [24:34] = expression
;               [24:34] address_of(str)
                lea rsi, [rsp - 59]
;               [15:6] sys_print(len : reg_rdx, ptr : reg_rsi) 
                sys_print_24_4_239_13:
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
                sys_print_24_4_239_13_end:
            print_239_13_end:
;           [240:13] str_out(nm)
;           [90:6] str_out(s : str) 
            str_out_240_13:
;               [240:13] alias s -> nm  (lea: )
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
                    lea rsi, [rsp - 1056]
;               [93:34] # buffer address
;               [94:5] mov(rdx, s.len)
;                   [94:14] s.len
                    movsx rdx, byte [rsp - 1057]
;               [94:21] # buffer size
;               [95:5] syscall()
                syscall
            str_out_240_13_end:
;           [241:13] print(dot)
;           [23:6] print(str : i8[]) 
            print_241_13:
;               [241:13] alias str -> dot  (lea: )
;               [24:4] sys_print(array_size_of(str), address_of(str))
;               [24:14] allocate named register 'rdx'
;               [24:14] rdx = array_size_of(str)
;               [24:14] = expression
;               [24:14] array_size_of(str)
                mov rdx, 1
;               [24:34] allocate named register 'rsi'
;               [24:34] rsi = address_of(str)
;               [24:34] = expression
;               [24:34] address_of(str)
                lea rsi, [rsp - 60]
;               [15:6] sys_print(len : reg_rdx, ptr : reg_rsi) 
                sys_print_24_4_241_13:
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
                sys_print_24_4_241_13_end:
            print_241_13_end:
;           [242:13] print(nl)
;           [23:6] print(str : i8[]) 
            print_242_13:
;               [242:13] alias str -> nl  (lea: )
;               [24:4] sys_print(array_size_of(str), address_of(str))
;               [24:14] allocate named register 'rdx'
;               [24:14] rdx = array_size_of(str)
;               [24:14] = expression
;               [24:14] array_size_of(str)
                mov rdx, 1
;               [24:34] allocate named register 'rsi'
;               [24:34] rsi = address_of(str)
;               [24:34] = expression
;               [24:34] address_of(str)
                lea rsi, [rsp - 61]
;               [15:6] sys_print(len : reg_rdx, ptr : reg_rsi) 
                sys_print_24_4_242_13:
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
                sys_print_24_4_242_13_end:
            print_242_13_end:
        if_233_9_end:
    jmp loop_230_5
    loop_230_5_end:
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
; len: i8
db 3
; zero remaining fields
times 127 db 0
; nl: i8[1]
db `\n`
; dot: i8[1]
db `.`
; prompt3: i8[6]
db `hello `
; prompt2: i8[20]
db `that is not a name.\n`
; prompt1: i8[12]
db `enter name:\n`
; hello: i8[21]
db `hello world from baz\n`
dat.len equ $ - dat

; max scratch registers in use: 5
;            max frames in use: 9
;               max stack size: 1057 B
;          optimization pass 1: 94
;          optimization pass 2: 0
```
