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
    * `./run-baz.sh myprogram.baz --stack=262144 --checks=upper`: checks upper
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
C/C++ Header                    43           1619            803           6999
C++                              1             99            120            608
-------------------------------------------------------------------------------
SUM:                            44           1718            923           7607
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

# function arguments and return are equivalent to mutable references

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

# return variable is a mutable reference to destination

func point_init() : point res {
    res.x = -1
    res.y = -2
}

func object_init() : object res {
    res.pos.y = 74
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

    var pt : point = point_init()
    assert(pt.x == -1)
    assert(pt.y == -2)

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

    var o3 : object[2]
    o3.pos.y = 73
    # index 0 in an array can be accessed without array index

    assert(o3[0].pos.y == 73)

    o3[1] = object_init()
    assert(o3[1].pos.y == 74)

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
    cmp_132_12:
    cmp qword [rsp - 213], 0
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
    mov qword [rsp - 213], -1
    cmp_136_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_136_12:
    assert_136_5:
        if_13_29_136_5:
        cmp_13_29_136_5:
        cmp r15b, 0
        jne if_13_26_136_5_end
        if_13_29_136_5_code:
            mov rdi, 1
            exit_13_38_136_5:
                    mov rax, 60
                syscall
            exit_13_38_136_5_end:
        if_13_26_136_5_end:
    assert_136_5_end:
    mov qword [rsp - 221], 1
    mov r15, qword [rsp - 221]
    mov r14, 141
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 205], 2
    mov r15, qword [rsp - 221]
    add r15, 1
    mov r14, 142
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    mov r13, 142
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
    cmp_143_12:
    mov r14, 1
    mov r13, 143
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_143_12:
    assert_143_5:
        if_13_29_143_5:
        cmp_13_29_143_5:
        cmp r15b, 0
        jne if_13_26_143_5_end
        if_13_29_143_5_code:
            mov rdi, 1
            exit_13_38_143_5:
                    mov rax, 60
                syscall
            exit_13_38_143_5_end:
        if_13_26_143_5_end:
    assert_143_5_end:
    cmp_144_12:
    mov r14, 2
    mov r13, 144
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_144_12:
    assert_144_5:
        if_13_29_144_5:
        cmp_13_29_144_5:
        cmp r15b, 0
        jne if_13_26_144_5_end
        if_13_29_144_5_code:
            mov rdi, 1
            exit_13_38_144_5:
                    mov rax, 60
                syscall
            exit_13_38_144_5_end:
        if_13_26_144_5_end:
    assert_144_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 146
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 205]
    mov r15, 146
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
    cmp_148_12:
    mov r14, 0
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
    bool_end_148_12:
    assert_148_5:
        if_13_29_148_5:
        cmp_13_29_148_5:
        cmp r15b, 0
        jne if_13_26_148_5_end
        if_13_29_148_5_code:
            mov rdi, 1
            exit_13_38_148_5:
                    mov rax, 60
                syscall
            exit_13_38_148_5_end:
        if_13_26_148_5_end:
    assert_148_5_end:
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov rcx, 4
    mov r15, 151
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 205]
    mov r15, 151
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
    cmp_152_12:
        mov rcx, 4
        mov r13, 152
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 152
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
    mov r15, 2
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 253], -1
    cmp_156_12:
        mov rcx, 4
        mov r13, 156
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 205]
        mov r13, 156
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
    mov qword [rsp - 221], 3
    mov r15, qword [rsp - 221]
    mov r14, 159
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 221]
    sub r14, 1
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_159_16:
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
        not dword [rsp + r15 * 4 - 205]
    inv_159_16_end:
    not dword [rsp + r15 * 4 - 205]
    cmp_160_12:
    mov r14, qword [rsp - 221]
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 2
    sete r15b
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
    faz_162_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 205], 254
    faz_162_5_end:
    cmp_163_12:
    mov r14, 1
    mov r13, 163
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 205], 254
    sete r15b
    bool_end_163_12:
    assert_163_5:
        if_13_29_163_5:
        cmp_13_29_163_5:
        cmp r15b, 0
        jne if_13_26_163_5_end
        if_13_29_163_5_code:
            mov rdi, 1
            exit_13_38_163_5:
                    mov rax, 60
                syscall
            exit_13_38_163_5_end:
        if_13_26_163_5_end:
    assert_163_5_end:
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    foo_166_5:
        mov qword [rsp - 269], 2
        mov qword [rsp - 261], 11
    foo_166_5_end:
    cmp_167_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_167_12:
    assert_167_5:
        if_13_29_167_5:
        cmp_13_29_167_5:
        cmp r15b, 0
        jne if_13_26_167_5_end
        if_13_29_167_5_code:
            mov rdi, 1
            exit_13_38_167_5:
                    mov rax, 60
                syscall
            exit_13_38_167_5_end:
        if_13_26_167_5_end:
    assert_167_5_end:
    cmp_168_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_168_12:
    assert_168_5:
        if_13_29_168_5:
        cmp_13_29_168_5:
        cmp r15b, 0
        jne if_13_26_168_5_end
        if_13_29_168_5_code:
            mov rdi, 1
            exit_13_38_168_5:
                    mov rax, 60
                syscall
            exit_13_38_168_5_end:
        if_13_26_168_5_end:
    assert_168_5_end:
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
    cmp_171_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov qword [rsp - 285], 3
    cmp_176_12:
        lea rsi, [rsp - 269]
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
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
    mov qword [rsp - 293], 0
    bar_179_5:
        if_56_8_179_5:
        cmp_56_8_179_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_179_5_end
        if_56_8_179_5_code:
            jmp bar_179_5_end
        if_56_5_179_5_end:
        mov qword [rsp - 293], 255
    bar_179_5_end:
    cmp_180_12:
    cmp qword [rsp - 293], 0
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
    mov qword [rsp - 293], 1
    bar_183_5:
        if_56_8_183_5:
        cmp_56_8_183_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_183_5_end
        if_56_8_183_5_code:
            jmp bar_183_5_end
        if_56_5_183_5_end:
        mov qword [rsp - 293], 255
    bar_183_5_end:
    cmp_184_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_184_12:
    assert_184_5:
        if_13_29_184_5:
        cmp_13_29_184_5:
        cmp r15b, 0
        jne if_13_26_184_5_end
        if_13_29_184_5_code:
            mov rdi, 1
            exit_13_38_184_5:
                    mov rax, 60
                syscall
            exit_13_38_184_5_end:
        if_13_26_184_5_end:
    assert_184_5_end:
    mov qword [rsp - 301], 1
    baz_187_13:
        mov r15, qword [rsp - 301]
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_187_13_end:
    cmp_188_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_188_12:
    assert_188_5:
        if_13_29_188_5:
        cmp_13_29_188_5:
        cmp r15b, 0
        jne if_13_26_188_5_end
        if_13_29_188_5_code:
            mov rdi, 1
            exit_13_38_188_5:
                    mov rax, 60
                syscall
            exit_13_38_188_5_end:
        if_13_26_188_5_end:
    assert_188_5_end:
    baz_190_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 309], r15
    baz_190_9_end:
    cmp_191_12:
    cmp qword [rsp - 309], 2
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
    baz_193_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 325], r15
    baz_193_23_end:
    mov qword [rsp - 317], 0
    cmp_194_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_194_12:
    assert_194_5:
        if_13_29_194_5:
        cmp_13_29_194_5:
        cmp r15b, 0
        jne if_13_26_194_5_end
        if_13_29_194_5_code:
            mov rdi, 1
            exit_13_38_194_5:
                    mov rax, 60
                syscall
            exit_13_38_194_5_end:
        if_13_26_194_5_end:
    assert_194_5_end:
    point_init_196_22:
        mov qword [rsp - 341], -1
        mov qword [rsp - 333], -2
    point_init_196_22_end:
    cmp_197_12:
    cmp qword [rsp - 341], -1
    sete r15b
    bool_end_197_12:
    assert_197_5:
        if_13_29_197_5:
        cmp_13_29_197_5:
        cmp r15b, 0
        jne if_13_26_197_5_end
        if_13_29_197_5_code:
            mov rdi, 1
            exit_13_38_197_5:
                    mov rax, 60
                syscall
            exit_13_38_197_5_end:
        if_13_26_197_5_end:
    assert_197_5_end:
    cmp_198_12:
    cmp qword [rsp - 333], -2
    sete r15b
    bool_end_198_12:
    assert_198_5:
        if_13_29_198_5:
        cmp_13_29_198_5:
        cmp r15b, 0
        jne if_13_26_198_5_end
        if_13_29_198_5_code:
            mov rdi, 1
            exit_13_38_198_5:
                    mov rax, 60
                syscall
            exit_13_38_198_5_end:
        if_13_26_198_5_end:
    assert_198_5_end:
    mov qword [rsp - 349], 1
    mov qword [rsp - 357], 2
    mov r15, qword [rsp - 349]
    imul r15, 10
    mov qword [rsp - 377], r15
    mov r15, qword [rsp - 357]
    mov qword [rsp - 369], r15
    mov dword [rsp - 361], 16711680
    cmp_204_12:
    cmp qword [rsp - 377], 10
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_13_29_204_5:
        cmp_13_29_204_5:
        cmp r15b, 0
        jne if_13_26_204_5_end
        if_13_29_204_5_code:
            mov rdi, 1
            exit_13_38_204_5:
                    mov rax, 60
                syscall
            exit_13_38_204_5_end:
        if_13_26_204_5_end:
    assert_204_5_end:
    cmp_205_12:
    cmp qword [rsp - 369], 2
    sete r15b
    bool_end_205_12:
    assert_205_5:
        if_13_29_205_5:
        cmp_13_29_205_5:
        cmp r15b, 0
        jne if_13_26_205_5_end
        if_13_29_205_5_code:
            mov rdi, 1
            exit_13_38_205_5:
                    mov rax, 60
                syscall
            exit_13_38_205_5_end:
        if_13_26_205_5_end:
    assert_205_5_end:
    cmp_206_12:
    cmp dword [rsp - 361], 16711680
    sete r15b
    bool_end_206_12:
    assert_206_5:
        if_13_29_206_5:
        cmp_13_29_206_5:
        cmp r15b, 0
        jne if_13_26_206_5_end
        if_13_29_206_5_code:
            mov rdi, 1
            exit_13_38_206_5:
                    mov rax, 60
                syscall
            exit_13_38_206_5_end:
        if_13_26_206_5_end:
    assert_206_5_end:
    mov r15, qword [rsp - 349]
    mov qword [rsp - 393], r15
    neg qword [rsp - 393]
    mov r15, qword [rsp - 357]
    mov qword [rsp - 385], r15
    neg qword [rsp - 385]
    mov rax, qword [rsp - 393]
    mov qword [rsp - 377], rax
    mov rax, qword [rsp - 385]
    mov qword [rsp - 369], rax
    cmp_210_12:
    cmp qword [rsp - 377], -1
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_13_29_210_5:
        cmp_13_29_210_5:
        cmp r15b, 0
        jne if_13_26_210_5_end
        if_13_29_210_5_code:
            mov rdi, 1
            exit_13_38_210_5:
                    mov rax, 60
                syscall
            exit_13_38_210_5_end:
        if_13_26_210_5_end:
    assert_210_5_end:
    cmp_211_12:
    cmp qword [rsp - 369], -2
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
    lea rsi, [rsp - 377]
    lea rdi, [rsp - 413]
    mov rcx, 20
    rep movsb
    cmp_214_12:
    cmp qword [rsp - 413], -1
    sete r15b
    bool_end_214_12:
    assert_214_5:
        if_13_29_214_5:
        cmp_13_29_214_5:
        cmp r15b, 0
        jne if_13_26_214_5_end
        if_13_29_214_5_code:
            mov rdi, 1
            exit_13_38_214_5:
                    mov rax, 60
                syscall
            exit_13_38_214_5_end:
        if_13_26_214_5_end:
    assert_214_5_end:
    cmp_215_12:
    cmp qword [rsp - 405], -2
    sete r15b
    bool_end_215_12:
    assert_215_5:
        if_13_29_215_5:
        cmp_13_29_215_5:
        cmp r15b, 0
        jne if_13_26_215_5_end
        if_13_29_215_5_code:
            mov rdi, 1
            exit_13_38_215_5:
                    mov rax, 60
                syscall
            exit_13_38_215_5_end:
        if_13_26_215_5_end:
    assert_215_5_end:
    cmp_216_12:
    cmp dword [rsp - 397], 16711680
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_13_29_216_5:
        cmp_13_29_216_5:
        cmp r15b, 0
        jne if_13_26_216_5_end
        if_13_29_216_5_code:
            mov rdi, 1
            exit_13_38_216_5:
                    mov rax, 60
                syscall
            exit_13_38_216_5_end:
        if_13_26_216_5_end:
    assert_216_5_end:
    xor al, al
    lea rdi, [rsp - 453]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 445], 73
    cmp_222_12:
    lea r14, [rsp - 453]
    mov r13, 0
    mov r12, 222
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
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
    lea r15, [rsp - 453]
    mov r14, 1
    mov r13, 224
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_224_13:
        mov qword [r15 + 8], 74
    object_init_224_13_end:
    cmp_225_12:
    lea r14, [rsp - 453]
    mov r13, 1
    mov r12, 225
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 74
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_13_29_225_5:
        cmp_13_29_225_5:
        cmp r15b, 0
        jne if_13_26_225_5_end
        if_13_29_225_5_code:
            mov rdi, 1
            exit_13_38_225_5:
                    mov rax, 60
                syscall
            exit_13_38_225_5_end:
        if_13_26_225_5_end:
    assert_225_5_end:
    xor al, al
    lea rdi, [rsp - 965]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 228
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 228
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_229_12:
    lea r14, [rsp - 965]
    mov r13, 1
    mov r12, 229
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 229
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_13_29_229_5:
        cmp_13_29_229_5:
        cmp r15b, 0
        jne if_13_26_229_5_end
        if_13_29_229_5_code:
            mov rdi, 1
            exit_13_38_229_5:
                    mov rax, 60
                syscall
            exit_13_38_229_5_end:
        if_13_26_229_5_end:
    assert_229_5_end:
    mov rcx, 8
    lea r15, [rsp - 965]
    mov r14, 1
    mov r13, 232
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 232
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 965]
    mov r14, 0
    mov r13, 233
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 233
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_239_12:
    lea r14, [rsp - 965]
    mov r13, 0
    mov r12, 239
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 239
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_13_29_239_5:
        cmp_13_29_239_5:
        cmp r15b, 0
        jne if_13_26_239_5_end
        if_13_29_239_5_code:
            mov rdi, 1
            exit_13_38_239_5:
                    mov rax, 60
                syscall
            exit_13_38_239_5_end:
        if_13_26_239_5_end:
    assert_239_5_end:
    cmp_240_12:
        mov rcx, 8
        lea r13, [rsp - 965]
        mov r12, 0
        mov r11, 241
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 241
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 965]
        mov r12, 1
        mov r11, 242
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 242
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
    bool_end_240_12:
    assert_240_5:
        if_13_29_240_5:
        cmp_13_29_240_5:
        cmp r15b, 0
        jne if_13_26_240_5_end
        if_13_29_240_5_code:
            mov rdi, 1
            exit_13_38_240_5:
                    mov rax, 60
                syscall
            exit_13_38_240_5_end:
        if_13_26_240_5_end:
    assert_240_5_end:
    xor al, al
    lea rdi, [rsp - 1093]
    mov rcx, 128
    rep stosb
    print_247_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_24_4_247_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_24_4_247_5_end:
    print_247_5_end:
    loop_248_5:
        print_249_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_24_4_249_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_24_4_249_9_end:
        print_249_9_end:
        str_in_250_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1092]
                mov rdx, 127
            syscall
                mov byte [rsp - 1093], al
                sub byte [rsp - 1093], 1
        str_in_250_9_end:
        if_251_12:
        cmp_251_12:
        cmp byte [rsp - 1093], 0
        jne if_253_19
        if_251_12_code:
            jmp loop_248_5_end
        jmp if_251_9_end
        if_253_19:
        cmp_253_19:
        cmp byte [rsp - 1093], 4
        jg if_else_251_9
        if_253_19_code:
            print_254_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_24_4_254_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_254_13_end:
            print_254_13_end:
            jmp loop_248_5
        jmp if_251_9_end
        if_else_251_9:
            print_257_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_24_4_257_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_257_13_end:
            print_257_13_end:
            str_out_258_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1092]
                    movsx rdx, byte [rsp - 1093]
                syscall
            str_out_258_13_end:
            print_259_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_24_4_259_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_259_13_end:
            print_259_13_end:
            print_260_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_24_4_260_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_24_4_260_13_end:
            print_260_13_end:
        if_251_9_end:
    jmp loop_248_5
    loop_248_5_end:
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

;[46:1] # function arguments and return are equivalent to mutable references
;[53:1] # default argument type is `i64`
;[60:1] # return target is specified as a variable, in this case `res`
;[70:1] # array arguments are declared with type and []
;[76:1] str : 128 B    fields:
;[76:1]       name :  offset :    size :  array? : array size
;[76:1]        len :       0 :       1 :      no :           
;[76:1]       data :       1 :     127 :     yes :        127

;[98:1] # return variable is a mutable reference to destination
;[109:1] # constants are declared in global scope
main:
;   [116:5] dat hello : i8[] = "hello world from baz\n"
;   [116:11] hello: i8[21] (21 B @ [rsp - 21])
;   [117:5] dat prompt1 : i8[] = "enter name:\n"
;   [117:9] prompt1: i8[12] (12 B @ [rsp - 33])
;   [118:5] dat prompt2 : i8[] = "that is not a name.\n"
;   [118:9] prompt2: i8[20] (20 B @ [rsp - 53])
;   [119:5] dat prompt3 : i8[] = "hello "
;   [119:9] prompt3: i8[6] (6 B @ [rsp - 59])
;   [120:5] dat dot : i8[] = "."
;   [120:13] dot: i8[1] (1 B @ [rsp - 60])
;   [121:5] dat nl : i8[] = "\n"
;   [121:14] nl: i8[1] (1 B @ [rsp - 61])
;   [123:5] dat s1 : str = { 3 }
;   [123:14] s1: str (128 B @ [rsp - 189])
;   [124:5] # remaining uninitialized fields are zeroed
;   [126:5] # initial data declared before any variables and initialized with constants
;   [128:5] var arr : i32[4]
;   [128:9] arr: i32[4] (16 B @ [rsp - 205])
;   [128:9] zero 4 * 4 B = 16 B
;   [128:5] size <= 32 B, use mov
    mov qword [rsp - 205], 0
    mov qword [rsp - 197], 0
;   [129:5] # arrays are initialized to 0
;   [131:5] var answer
;   [131:9] answer: i64 (8 B @ [rsp - 213])
;   [131:9] zero 1 * 8 B = 8 B
;   [131:5] size <= 32 B, use mov
    mov qword [rsp - 213], 0
;   [132:5] assert(answer == 0)
;   [132:12] allocate scratch register -> r15
;   [132:12] ? answer == 0
;   [132:12] ? answer == 0
    cmp_132_12:
    cmp qword [rsp - 213], 0
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
;   [133:5] # variables without initializer are zeroed
;   [135:5] answer = maybe
;   [135:14] maybe
    mov qword [rsp - 213], -1
;   [136:5] assert(answer == -1)
;   [136:12] allocate scratch register -> r15
;   [136:12] ? answer == -1
;   [136:12] ? answer == -1
    cmp_136_12:
    cmp qword [rsp - 213], -1
    sete r15b
    bool_end_136_12:
;   [13:6] assert(expr : bool) 
    assert_136_5:
;       [136:5] alias expr -> r15b  (lea: )
        if_13_29_136_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_136_5:
        cmp r15b, 0
        jne if_13_26_136_5_end
        if_13_29_136_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_136_5:
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
            exit_13_38_136_5_end:
        if_13_26_136_5_end:
;       [136:5] free scratch register 'r15'
    assert_136_5_end:
;   [138:5] var ix = 1
;   [138:9] ix: i64 (8 B @ [rsp - 221])
;   [138:9] ix = 1
;   [138:14] 1
    mov qword [rsp - 221], 1
;   [139:5] # variables can have an initial value that can be an expression
;   [141:5] arr[ix] = 2
;   [141:5] allocate scratch register -> r15
;   [141:9] set array index
;   [141:9] ix
    mov r15, qword [rsp - 221]
;   [141:9] bounds check
;   [141:9] allocate scratch register -> r14
;   [141:9] line number
    mov r14, 141
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [141:9] free scratch register 'r14'
;   [141:15] 2
    mov dword [rsp + r15 * 4 - 205], 2
;   [141:5] free scratch register 'r15'
;   [142:5] arr[ix + 1] = arr[ix]
;   [142:5] allocate scratch register -> r15
;   [142:9] set array index
;   [142:9] ix
    mov r15, qword [rsp - 221]
;   [142:14] r15 + 1
    add r15, 1
;   [142:9] bounds check
;   [142:9] allocate scratch register -> r14
;   [142:9] line number
    mov r14, 142
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [142:9] free scratch register 'r14'
;   [142:19] arr[ix]
;   [142:19] allocate scratch register -> r14
;   [142:23] set array index
;   [142:23] ix
    mov r14, qword [rsp - 221]
;   [142:23] bounds check
;   [142:23] allocate scratch register -> r13
;   [142:23] line number
    mov r13, 142
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [142:23] free scratch register 'r13'
;   [142:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 205]
    mov dword [rsp + r15 * 4 - 205], r13d
;   [142:19] free scratch register 'r13'
;   [142:19] free scratch register 'r14'
;   [142:5] free scratch register 'r15'
;   [143:5] assert(arr[1] == 2)
;   [143:12] allocate scratch register -> r15
;   [143:12] ? arr[1] == 2
;   [143:12] ? arr[1] == 2
    cmp_143_12:
;   [143:12] allocate scratch register -> r14
;   [143:16] set array index
;   [143:16] 1
    mov r14, 1
;   [143:16] bounds check
;   [143:16] allocate scratch register -> r13
;   [143:16] line number
    mov r13, 143
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [143:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 2
;   [143:12] free scratch register 'r14'
    sete r15b
    bool_end_143_12:
;   [13:6] assert(expr : bool) 
    assert_143_5:
;       [143:5] alias expr -> r15b  (lea: )
        if_13_29_143_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_143_5:
        cmp r15b, 0
        jne if_13_26_143_5_end
        if_13_29_143_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_143_5:
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
            exit_13_38_143_5_end:
        if_13_26_143_5_end:
;       [143:5] free scratch register 'r15'
    assert_143_5_end:
;   [144:5] assert(arr[2] == 2)
;   [144:12] allocate scratch register -> r15
;   [144:12] ? arr[2] == 2
;   [144:12] ? arr[2] == 2
    cmp_144_12:
;   [144:12] allocate scratch register -> r14
;   [144:16] set array index
;   [144:16] 2
    mov r14, 2
;   [144:16] bounds check
;   [144:16] allocate scratch register -> r13
;   [144:16] line number
    mov r13, 144
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [144:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 2
;   [144:12] free scratch register 'r14'
    sete r15b
    bool_end_144_12:
;   [13:6] assert(expr : bool) 
    assert_144_5:
;       [144:5] alias expr -> r15b  (lea: )
        if_13_29_144_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_144_5:
        cmp r15b, 0
        jne if_13_26_144_5_end
        if_13_29_144_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_144_5:
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
            exit_13_38_144_5_end:
        if_13_26_144_5_end:
;       [144:5] free scratch register 'r15'
    assert_144_5_end:
;   [146:5] array_copy(arr[2], arr, 2)
;   [146:5] allocate named register 'rsi'
;   [146:5] allocate named register 'rdi'
;   [146:5] allocate named register 'rcx'
;   [146:29] 2
;   [146:29] 2
    mov rcx, 2
;   [146:16] arr[2]
;   [146:16] allocate scratch register -> r15
;   [146:20] set array index
;   [146:20] 2
    mov r15, 2
;   [146:20] bounds check
;   [146:20] allocate scratch register -> r14
;   [146:20] line number
    mov r14, 146
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [146:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [146:20] free scratch register 'r13'
    cmovg rbp, r14
    jg panic_bounds
;   [146:20] free scratch register 'r14'
    lea rsi, [rsp + r15 * 4 - 205]
;   [146:5] free scratch register 'r15'
;   [146:24] arr
;   [146:24] bounds check
;   [146:24] allocate scratch register -> r15
;   [146:24] line number
    mov r15, 146
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [146:24] free scratch register 'r15'
    lea rdi, [rsp - 205]
    shl rcx, 2
    rep movsb
;   [146:5] free named register 'rcx'
;   [146:5] free named register 'rdi'
;   [146:5] free named register 'rsi'
;   [147:5] # copy from, to, number of elements
;   [148:5] assert(arr[0] == 2)
;   [148:12] allocate scratch register -> r15
;   [148:12] ? arr[0] == 2
;   [148:12] ? arr[0] == 2
    cmp_148_12:
;   [148:12] allocate scratch register -> r14
;   [148:16] set array index
;   [148:16] 0
    mov r14, 0
;   [148:16] bounds check
;   [148:16] allocate scratch register -> r13
;   [148:16] line number
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [148:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 2
;   [148:12] free scratch register 'r14'
    sete r15b
    bool_end_148_12:
;   [13:6] assert(expr : bool) 
    assert_148_5:
;       [148:5] alias expr -> r15b  (lea: )
        if_13_29_148_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_148_5:
        cmp r15b, 0
        jne if_13_26_148_5_end
        if_13_29_148_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_148_5:
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
            exit_13_38_148_5_end:
        if_13_26_148_5_end:
;       [148:5] free scratch register 'r15'
    assert_148_5_end:
;   [150:5] var arr1 : i32[8]
;   [150:9] arr1: i32[8] (32 B @ [rsp - 253])
;   [150:9] zero 8 * 4 B = 32 B
;   [150:5] size <= 32 B, use mov
    mov qword [rsp - 253], 0
    mov qword [rsp - 245], 0
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
;   [151:5] array_copy(arr, arr1, 4)
;   [151:5] allocate named register 'rsi'
;   [151:5] allocate named register 'rdi'
;   [151:5] allocate named register 'rcx'
;   [151:27] 4
;   [151:27] 4
    mov rcx, 4
;   [151:16] arr
;   [151:16] bounds check
;   [151:16] allocate scratch register -> r15
;   [151:16] line number
    mov r15, 151
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [151:16] free scratch register 'r15'
    lea rsi, [rsp - 205]
;   [151:21] arr1
;   [151:21] bounds check
;   [151:21] allocate scratch register -> r15
;   [151:21] line number
    mov r15, 151
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [151:21] free scratch register 'r15'
    lea rdi, [rsp - 253]
    shl rcx, 2
    rep movsb
;   [151:5] free named register 'rcx'
;   [151:5] free named register 'rdi'
;   [151:5] free named register 'rsi'
;   [152:5] assert(arrays_equal(arr, arr1, 4))
;   [152:12] allocate scratch register -> r15
;   [152:12] ? arrays_equal(arr, arr1, 4)
;   [152:12] ? arrays_equal(arr, arr1, 4)
    cmp_152_12:
;   [152:12] allocate scratch register -> r14
;       [152:12] r14 = arrays_equal(arr, arr1, 4)
;       [152:12] = expression
;       [152:12] arrays_equal(arr, arr1, 4)
;       [152:12] allocate named register 'rsi'
;       [152:12] allocate named register 'rdi'
;       [152:12] allocate named register 'rcx'
;       [152:36] 4
;       [152:36] 4
        mov rcx, 4
;       [152:25] arr
;       [152:25] bounds check
;       [152:25] allocate scratch register -> r13
;       [152:25] line number
        mov r13, 152
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [152:25] free scratch register 'r13'
        lea rsi, [rsp - 205]
;       [152:30] arr1
;       [152:30] bounds check
;       [152:30] allocate scratch register -> r13
;       [152:30] line number
        mov r13, 152
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [152:30] free scratch register 'r13'
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
;       [152:12] free named register 'rcx'
;       [152:12] free named register 'rdi'
;       [152:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [152:12] free scratch register 'r14'
    setne r15b
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
;   [153:5] # `arrays_equal` is built-in function
;   [155:5] arr1[2] = -1
;   [155:5] allocate scratch register -> r15
;   [155:10] set array index
;   [155:10] 2
    mov r15, 2
;   [155:10] bounds check
;   [155:10] allocate scratch register -> r14
;   [155:10] line number
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [155:10] free scratch register 'r14'
;   [155:16] -1
    mov dword [rsp + r15 * 4 - 253], -1
;   [155:5] free scratch register 'r15'
;   [156:5] assert(not arrays_equal(arr, arr1, 4))
;   [156:12] allocate scratch register -> r15
;   [156:12] ? not arrays_equal(arr, arr1, 4)
;   [156:12] ? not arrays_equal(arr, arr1, 4)
    cmp_156_12:
;   [156:16] allocate scratch register -> r14
;       [156:16] r14 = arrays_equal(arr, arr1, 4)
;       [156:16] = expression
;       [156:16] arrays_equal(arr, arr1, 4)
;       [156:16] allocate named register 'rsi'
;       [156:16] allocate named register 'rdi'
;       [156:16] allocate named register 'rcx'
;       [156:40] 4
;       [156:40] 4
        mov rcx, 4
;       [156:29] arr
;       [156:29] bounds check
;       [156:29] allocate scratch register -> r13
;       [156:29] line number
        mov r13, 156
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [156:29] free scratch register 'r13'
        lea rsi, [rsp - 205]
;       [156:34] arr1
;       [156:34] bounds check
;       [156:34] allocate scratch register -> r13
;       [156:34] line number
        mov r13, 156
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [156:34] free scratch register 'r13'
        lea rdi, [rsp - 253]
        shl rcx, 2
        repe cmpsb
;       [156:16] free named register 'rcx'
;       [156:16] free named register 'rdi'
;       [156:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [156:12] free scratch register 'r14'
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
;   [158:5] ix = 3
;   [158:10] 3
    mov qword [rsp - 221], 3
;   [159:5] arr[ix] = ~inv(arr[ix - 1])
;   [159:5] allocate scratch register -> r15
;   [159:9] set array index
;   [159:9] ix
    mov r15, qword [rsp - 221]
;   [159:9] bounds check
;   [159:9] allocate scratch register -> r14
;   [159:9] line number
    mov r14, 159
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [159:9] free scratch register 'r14'
;   [159:16] arr = ~inv(arr[ix - 1])
;   [159:16] = expression
;   [159:16] ~inv(arr[ix - 1])
;   [159:20] allocate scratch register -> r14
;   [159:24] set array index
;   [159:24] ix
    mov r14, qword [rsp - 221]
;   [159:29] r14 - 1
    sub r14, 1
;   [159:24] bounds check
;   [159:24] allocate scratch register -> r13
;   [159:24] line number
    mov r13, 159
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [159:24] free scratch register 'r13'
;   [62:6] inv(i : i32) : i32 res 
    inv_159_16:
;       [159:16] alias res -> arr  (lea: rsp + r15 * 4 - 205)
;       [159:16] alias i -> arr  (lea: rsp + r14 * 4 - 205)
;       [63:5] res = ~i
;       [63:12] ~i
;       [63:12] allocate scratch register -> r13
        mov r13d, dword [rsp + r14 * 4 - 205]
        mov dword [rsp + r15 * 4 - 205], r13d
;       [63:12] free scratch register 'r13'
        not dword [rsp + r15 * 4 - 205]
;       [159:16] free scratch register 'r14'
    inv_159_16_end:
    not dword [rsp + r15 * 4 - 205]
;   [159:5] free scratch register 'r15'
;   [160:5] assert(arr[ix] == 2)
;   [160:12] allocate scratch register -> r15
;   [160:12] ? arr[ix] == 2
;   [160:12] ? arr[ix] == 2
    cmp_160_12:
;   [160:12] allocate scratch register -> r14
;   [160:16] set array index
;   [160:16] ix
    mov r14, qword [rsp - 221]
;   [160:16] bounds check
;   [160:16] allocate scratch register -> r13
;   [160:16] line number
    mov r13, 160
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [160:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 2
;   [160:12] free scratch register 'r14'
    sete r15b
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
;   [162:5] faz(arr)
;   [72:6] faz(arg : i32[]) 
    faz_162_5:
;       [162:5] alias arg -> arr  (lea: )
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
    faz_162_5_end:
;   [163:5] assert(arr[1] == 0xfe)
;   [163:12] allocate scratch register -> r15
;   [163:12] ? arr[1] == 0xfe
;   [163:12] ? arr[1] == 0xfe
    cmp_163_12:
;   [163:12] allocate scratch register -> r14
;   [163:16] set array index
;   [163:16] 1
    mov r14, 1
;   [163:16] bounds check
;   [163:16] allocate scratch register -> r13
;   [163:16] line number
    mov r13, 163
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [163:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 205], 254
;   [163:12] free scratch register 'r14'
    sete r15b
    bool_end_163_12:
;   [13:6] assert(expr : bool) 
    assert_163_5:
;       [163:5] alias expr -> r15b  (lea: )
        if_13_29_163_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_163_5:
        cmp r15b, 0
        jne if_13_26_163_5_end
        if_13_29_163_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_163_5:
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
            exit_13_38_163_5_end:
        if_13_26_163_5_end:
;       [163:5] free scratch register 'r15'
    assert_163_5_end:
;   [165:5] var p : point = {0, 0}
;   [165:9] p: point (16 B @ [rsp - 269])
;   [165:9] p = {0, 0}
;   [165:21] copy field 'x'
    mov qword [rsp - 269], 0
;   [165:21] copy field 'y'
    mov qword [rsp - 261], 0
;   [166:5] foo(p)
;   [48:6] foo(pt : point) 
    foo_166_5:
;       [166:5] alias pt -> p  (lea: )
;       [49:5] pt.x = 0b10
;       [49:12] 0b10
        mov qword [rsp - 269], 2
;       [49:20] # binary value 2
;       [50:5] pt.y = 0xb
;       [50:12] 0xb
        mov qword [rsp - 261], 11
;       [50:20] # hex value 11
    foo_166_5_end:
;   [167:5] assert(p.x == 2)
;   [167:12] allocate scratch register -> r15
;   [167:12] ? p.x == 2
;   [167:12] ? p.x == 2
    cmp_167_12:
    cmp qword [rsp - 269], 2
    sete r15b
    bool_end_167_12:
;   [13:6] assert(expr : bool) 
    assert_167_5:
;       [167:5] alias expr -> r15b  (lea: )
        if_13_29_167_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_167_5:
        cmp r15b, 0
        jne if_13_26_167_5_end
        if_13_29_167_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_167_5:
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
            exit_13_38_167_5_end:
        if_13_26_167_5_end:
;       [167:5] free scratch register 'r15'
    assert_167_5_end:
;   [168:5] assert(p.y == 0xb)
;   [168:12] allocate scratch register -> r15
;   [168:12] ? p.y == 0xb
;   [168:12] ? p.y == 0xb
    cmp_168_12:
    cmp qword [rsp - 261], 11
    sete r15b
    bool_end_168_12:
;   [13:6] assert(expr : bool) 
    assert_168_5:
;       [168:5] alias expr -> r15b  (lea: )
        if_13_29_168_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_168_5:
        cmp r15b, 0
        jne if_13_26_168_5_end
        if_13_29_168_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_168_5:
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
            exit_13_38_168_5_end:
        if_13_26_168_5_end:
;       [168:5] free scratch register 'r15'
    assert_168_5_end:
;   [170:5] var q : point = p
;   [170:9] q: point (16 B @ [rsp - 285])
;   [170:9] q = p
;   [170:21] size <= 16 B, use mov
;   [170:21] allocate named register 'rax'
    mov rax, qword [rsp - 269]
    mov qword [rsp - 285], rax
    mov rax, qword [rsp - 261]
    mov qword [rsp - 277], rax
;   [170:21] free named register 'rax'
;   [171:5] assert(equal(p, q))
;   [171:12] allocate scratch register -> r15
;   [171:12] ? equal(p, q)
;   [171:12] ? equal(p, q)
    cmp_171_12:
;   [171:12] allocate scratch register -> r14
;       [171:12] r14 = equal(p, q)
;       [171:12] = expression
;       [171:12] equal(p, q)
;       [171:12] allocate named register 'rsi'
;       [171:12] allocate named register 'rdi'
;       [171:12] allocate named register 'rcx'
;       [171:18] p
        lea rsi, [rsp - 269]
;       [171:21] q
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
;       [171:12] free named register 'rcx'
;       [171:12] free named register 'rdi'
;       [171:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [171:12] free scratch register 'r14'
    setne r15b
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
;   [172:5] # `equal` is built-in function to compare user types for equality or same
;   [173:5] # size arrays
;   [175:5] q.x = 3
;   [175:11] 3
    mov qword [rsp - 285], 3
;   [176:5] assert(not equal(p, q))
;   [176:12] allocate scratch register -> r15
;   [176:12] ? not equal(p, q)
;   [176:12] ? not equal(p, q)
    cmp_176_12:
;   [176:16] allocate scratch register -> r14
;       [176:16] r14 = equal(p, q)
;       [176:16] = expression
;       [176:16] equal(p, q)
;       [176:16] allocate named register 'rsi'
;       [176:16] allocate named register 'rdi'
;       [176:16] allocate named register 'rcx'
;       [176:22] p
        lea rsi, [rsp - 269]
;       [176:25] q
        lea rdi, [rsp - 285]
        mov rcx, 2
        repe cmpsq
;       [176:16] free named register 'rcx'
;       [176:16] free named register 'rdi'
;       [176:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [176:12] free scratch register 'r14'
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
;   [178:5] var i = 0
;   [178:9] i: i64 (8 B @ [rsp - 293])
;   [178:9] i = 0
;   [178:13] 0
    mov qword [rsp - 293], 0
;   [179:5] bar(i)
;   [55:6] bar(arg) 
    bar_179_5:
;       [179:5] alias arg -> i  (lea: )
        if_56_8_179_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_179_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_179_5_end
        if_56_8_179_5_code:
;           [56:17] return
            jmp bar_179_5_end
        if_56_5_179_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 293], 255
    bar_179_5_end:
;   [180:5] assert(i == 0)
;   [180:12] allocate scratch register -> r15
;   [180:12] ? i == 0
;   [180:12] ? i == 0
    cmp_180_12:
    cmp qword [rsp - 293], 0
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
;   [182:5] i = 1
;   [182:9] 1
    mov qword [rsp - 293], 1
;   [183:5] bar(i)
;   [55:6] bar(arg) 
    bar_183_5:
;       [183:5] alias arg -> i  (lea: )
        if_56_8_183_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_183_5:
        cmp qword [rsp - 293], 0
        jne if_56_5_183_5_end
        if_56_8_183_5_code:
;           [56:17] return
            jmp bar_183_5_end
        if_56_5_183_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 293], 255
    bar_183_5_end:
;   [184:5] assert(i == 0xff)
;   [184:12] allocate scratch register -> r15
;   [184:12] ? i == 0xff
;   [184:12] ? i == 0xff
    cmp_184_12:
    cmp qword [rsp - 293], 255
    sete r15b
    bool_end_184_12:
;   [13:6] assert(expr : bool) 
    assert_184_5:
;       [184:5] alias expr -> r15b  (lea: )
        if_13_29_184_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_184_5:
        cmp r15b, 0
        jne if_13_26_184_5_end
        if_13_29_184_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_184_5:
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
            exit_13_38_184_5_end:
        if_13_26_184_5_end:
;       [184:5] free scratch register 'r15'
    assert_184_5_end:
;   [186:5] var j = 1
;   [186:9] j: i64 (8 B @ [rsp - 301])
;   [186:9] j = 1
;   [186:13] 1
    mov qword [rsp - 301], 1
;   [187:5] var k = baz(j)
;   [187:9] k: i64 (8 B @ [rsp - 309])
;   [187:9] k = baz(j)
;   [187:13] k = baz(j)
;   [187:13] = expression
;   [187:13] baz(j)
;   [66:6] baz(arg) : i64 res 
    baz_187_13:
;       [187:13] alias res -> k  (lea: rsp - 309)
;       [187:13] alias arg -> j  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, qword [rsp - 301]
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 309], r15
;       [67:11] free scratch register 'r15'
    baz_187_13_end:
;   [188:5] assert(k == 2)
;   [188:12] allocate scratch register -> r15
;   [188:12] ? k == 2
;   [188:12] ? k == 2
    cmp_188_12:
    cmp qword [rsp - 309], 2
    sete r15b
    bool_end_188_12:
;   [13:6] assert(expr : bool) 
    assert_188_5:
;       [188:5] alias expr -> r15b  (lea: )
        if_13_29_188_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_188_5:
        cmp r15b, 0
        jne if_13_26_188_5_end
        if_13_29_188_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_188_5:
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
            exit_13_38_188_5_end:
        if_13_26_188_5_end:
;       [188:5] free scratch register 'r15'
    assert_188_5_end:
;   [190:5] k = baz(1)
;   [190:9] k = baz(1)
;   [190:9] = expression
;   [190:9] baz(1)
;   [66:6] baz(arg) : i64 res 
    baz_190_9:
;       [190:9] alias res -> k  (lea: rsp - 309)
;       [190:9] alias arg -> 1  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 1
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 309], r15
;       [67:11] free scratch register 'r15'
    baz_190_9_end:
;   [191:5] assert(k == 2)
;   [191:12] allocate scratch register -> r15
;   [191:12] ? k == 2
;   [191:12] ? k == 2
    cmp_191_12:
    cmp qword [rsp - 309], 2
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
;   [193:5] var p0 : point = {baz(2), 0}
;   [193:9] p0: point (16 B @ [rsp - 325])
;   [193:9] p0 = {baz(2), 0}
;   [193:22] copy field 'x'
;   [193:23] qword [rsp - 325] = baz(2)
;   [193:23] = expression
;   [193:23] baz(2)
;   [66:6] baz(arg) : i64 res 
    baz_193_23:
;       [193:23] alias res -> qword [rsp - 325]  (lea: rsp - 325)
;       [193:23] alias arg -> 2  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 2
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 325], r15
;       [67:11] free scratch register 'r15'
    baz_193_23_end:
;   [193:22] copy field 'y'
    mov qword [rsp - 317], 0
;   [194:5] assert(p0.x == 4)
;   [194:12] allocate scratch register -> r15
;   [194:12] ? p0.x == 4
;   [194:12] ? p0.x == 4
    cmp_194_12:
    cmp qword [rsp - 325], 4
    sete r15b
    bool_end_194_12:
;   [13:6] assert(expr : bool) 
    assert_194_5:
;       [194:5] alias expr -> r15b  (lea: )
        if_13_29_194_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_194_5:
        cmp r15b, 0
        jne if_13_26_194_5_end
        if_13_29_194_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_194_5:
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
            exit_13_38_194_5_end:
        if_13_26_194_5_end:
;       [194:5] free scratch register 'r15'
    assert_194_5_end:
;   [196:5] var pt : point = point_init()
;   [196:9] pt: point (16 B @ [rsp - 341])
;   [196:9] pt = point_init()
;   [196:22] point_init()
;   [100:6] point_init() : point res 
    point_init_196_22:
;       [196:22] alias res -> pt  (lea: rsp - 341)
;       [101:5] res.x = -1
;       [101:14] -1
        mov qword [rsp - 341], -1
;       [102:5] res.y = -2
;       [102:14] -2
        mov qword [rsp - 333], -2
    point_init_196_22_end:
;   [197:5] assert(pt.x == -1)
;   [197:12] allocate scratch register -> r15
;   [197:12] ? pt.x == -1
;   [197:12] ? pt.x == -1
    cmp_197_12:
    cmp qword [rsp - 341], -1
    sete r15b
    bool_end_197_12:
;   [13:6] assert(expr : bool) 
    assert_197_5:
;       [197:5] alias expr -> r15b  (lea: )
        if_13_29_197_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_197_5:
        cmp r15b, 0
        jne if_13_26_197_5_end
        if_13_29_197_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_197_5:
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
            exit_13_38_197_5_end:
        if_13_26_197_5_end:
;       [197:5] free scratch register 'r15'
    assert_197_5_end:
;   [198:5] assert(pt.y == -2)
;   [198:12] allocate scratch register -> r15
;   [198:12] ? pt.y == -2
;   [198:12] ? pt.y == -2
    cmp_198_12:
    cmp qword [rsp - 333], -2
    sete r15b
    bool_end_198_12:
;   [13:6] assert(expr : bool) 
    assert_198_5:
;       [198:5] alias expr -> r15b  (lea: )
        if_13_29_198_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_198_5:
        cmp r15b, 0
        jne if_13_26_198_5_end
        if_13_29_198_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_198_5:
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
            exit_13_38_198_5_end:
        if_13_26_198_5_end:
;       [198:5] free scratch register 'r15'
    assert_198_5_end:
;   [200:5] var x = 1
;   [200:9] x: i64 (8 B @ [rsp - 349])
;   [200:9] x = 1
;   [200:13] 1
    mov qword [rsp - 349], 1
;   [201:5] var y = 2
;   [201:9] y: i64 (8 B @ [rsp - 357])
;   [201:9] y = 2
;   [201:13] 2
    mov qword [rsp - 357], 2
;   [203:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [203:9] o1: object (20 B @ [rsp - 377])
;   [203:9] o1 = {{x * 10, y}, 0xff0000}
;   [203:23] copy field 'pos'
;   [203:24] copy field 'x'
;   [203:25] allocate scratch register -> r15
;   [203:25] x
    mov r15, qword [rsp - 349]
;   [203:29] r15 * 10
;   [203:29] dst is reg, src is const
    imul r15, 10
    mov qword [rsp - 377], r15
;   [203:25] free scratch register 'r15'
;   [203:24] copy field 'y'
;   [203:33] allocate scratch register -> r15
    mov r15, qword [rsp - 357]
    mov qword [rsp - 369], r15
;   [203:33] free scratch register 'r15'
;   [203:23] copy field 'color'
    mov dword [rsp - 361], 16711680
;   [204:5] assert(o1.pos.x == 10)
;   [204:12] allocate scratch register -> r15
;   [204:12] ? o1.pos.x == 10
;   [204:12] ? o1.pos.x == 10
    cmp_204_12:
    cmp qword [rsp - 377], 10
    sete r15b
    bool_end_204_12:
;   [13:6] assert(expr : bool) 
    assert_204_5:
;       [204:5] alias expr -> r15b  (lea: )
        if_13_29_204_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_204_5:
        cmp r15b, 0
        jne if_13_26_204_5_end
        if_13_29_204_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_204_5:
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
            exit_13_38_204_5_end:
        if_13_26_204_5_end:
;       [204:5] free scratch register 'r15'
    assert_204_5_end:
;   [205:5] assert(o1.pos.y == 2)
;   [205:12] allocate scratch register -> r15
;   [205:12] ? o1.pos.y == 2
;   [205:12] ? o1.pos.y == 2
    cmp_205_12:
    cmp qword [rsp - 369], 2
    sete r15b
    bool_end_205_12:
;   [13:6] assert(expr : bool) 
    assert_205_5:
;       [205:5] alias expr -> r15b  (lea: )
        if_13_29_205_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_205_5:
        cmp r15b, 0
        jne if_13_26_205_5_end
        if_13_29_205_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_205_5:
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
            exit_13_38_205_5_end:
        if_13_26_205_5_end:
;       [205:5] free scratch register 'r15'
    assert_205_5_end:
;   [206:5] assert(o1.color == 0xff0000)
;   [206:12] allocate scratch register -> r15
;   [206:12] ? o1.color == 0xff0000
;   [206:12] ? o1.color == 0xff0000
    cmp_206_12:
    cmp dword [rsp - 361], 16711680
    sete r15b
    bool_end_206_12:
;   [13:6] assert(expr : bool) 
    assert_206_5:
;       [206:5] alias expr -> r15b  (lea: )
        if_13_29_206_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_206_5:
        cmp r15b, 0
        jne if_13_26_206_5_end
        if_13_29_206_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_206_5:
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
            exit_13_38_206_5_end:
        if_13_26_206_5_end:
;       [206:5] free scratch register 'r15'
    assert_206_5_end:
;   [208:5] var p1 : point = {-x, -y}
;   [208:9] p1: point (16 B @ [rsp - 393])
;   [208:9] p1 = {-x, -y}
;   [208:22] copy field 'x'
;   [208:23] allocate scratch register -> r15
    mov r15, qword [rsp - 349]
    mov qword [rsp - 393], r15
;   [208:23] free scratch register 'r15'
    neg qword [rsp - 393]
;   [208:22] copy field 'y'
;   [208:27] allocate scratch register -> r15
    mov r15, qword [rsp - 357]
    mov qword [rsp - 385], r15
;   [208:27] free scratch register 'r15'
    neg qword [rsp - 385]
;   [209:5] o1.pos = p1
;   [209:14] size <= 16 B, use mov
;   [209:14] allocate named register 'rax'
    mov rax, qword [rsp - 393]
    mov qword [rsp - 377], rax
    mov rax, qword [rsp - 385]
    mov qword [rsp - 369], rax
;   [209:14] free named register 'rax'
;   [210:5] assert(o1.pos.x == -1)
;   [210:12] allocate scratch register -> r15
;   [210:12] ? o1.pos.x == -1
;   [210:12] ? o1.pos.x == -1
    cmp_210_12:
    cmp qword [rsp - 377], -1
    sete r15b
    bool_end_210_12:
;   [13:6] assert(expr : bool) 
    assert_210_5:
;       [210:5] alias expr -> r15b  (lea: )
        if_13_29_210_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_210_5:
        cmp r15b, 0
        jne if_13_26_210_5_end
        if_13_29_210_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_210_5:
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
            exit_13_38_210_5_end:
        if_13_26_210_5_end:
;       [210:5] free scratch register 'r15'
    assert_210_5_end:
;   [211:5] assert(o1.pos.y == -2)
;   [211:12] allocate scratch register -> r15
;   [211:12] ? o1.pos.y == -2
;   [211:12] ? o1.pos.y == -2
    cmp_211_12:
    cmp qword [rsp - 369], -2
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
;   [213:5] var o2 : object = o1
;   [213:9] o2: object (20 B @ [rsp - 413])
;   [213:9] o2 = o1
;   [213:23] allocate named register 'rsi'
;   [213:23] allocate named register 'rdi'
;   [213:23] allocate named register 'rcx'
    lea rsi, [rsp - 377]
    lea rdi, [rsp - 413]
    mov rcx, 20
    rep movsb
;   [213:23] free named register 'rcx'
;   [213:23] free named register 'rdi'
;   [213:23] free named register 'rsi'
;   [214:5] assert(o2.pos.x == -1)
;   [214:12] allocate scratch register -> r15
;   [214:12] ? o2.pos.x == -1
;   [214:12] ? o2.pos.x == -1
    cmp_214_12:
    cmp qword [rsp - 413], -1
    sete r15b
    bool_end_214_12:
;   [13:6] assert(expr : bool) 
    assert_214_5:
;       [214:5] alias expr -> r15b  (lea: )
        if_13_29_214_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_214_5:
        cmp r15b, 0
        jne if_13_26_214_5_end
        if_13_29_214_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_214_5:
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
            exit_13_38_214_5_end:
        if_13_26_214_5_end:
;       [214:5] free scratch register 'r15'
    assert_214_5_end:
;   [215:5] assert(o2.pos.y == -2)
;   [215:12] allocate scratch register -> r15
;   [215:12] ? o2.pos.y == -2
;   [215:12] ? o2.pos.y == -2
    cmp_215_12:
    cmp qword [rsp - 405], -2
    sete r15b
    bool_end_215_12:
;   [13:6] assert(expr : bool) 
    assert_215_5:
;       [215:5] alias expr -> r15b  (lea: )
        if_13_29_215_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_215_5:
        cmp r15b, 0
        jne if_13_26_215_5_end
        if_13_29_215_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_215_5:
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
            exit_13_38_215_5_end:
        if_13_26_215_5_end:
;       [215:5] free scratch register 'r15'
    assert_215_5_end:
;   [216:5] assert(o2.color == 0xff0000)
;   [216:12] allocate scratch register -> r15
;   [216:12] ? o2.color == 0xff0000
;   [216:12] ? o2.color == 0xff0000
    cmp_216_12:
    cmp dword [rsp - 397], 16711680
    sete r15b
    bool_end_216_12:
;   [13:6] assert(expr : bool) 
    assert_216_5:
;       [216:5] alias expr -> r15b  (lea: )
        if_13_29_216_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_216_5:
        cmp r15b, 0
        jne if_13_26_216_5_end
        if_13_29_216_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_216_5:
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
            exit_13_38_216_5_end:
        if_13_26_216_5_end:
;       [216:5] free scratch register 'r15'
    assert_216_5_end:
;   [218:5] var o3 : object[2]
;   [218:9] o3: object[2] (40 B @ [rsp - 453])
;   [218:9] zero 2 * 20 B = 40 B
;   [218:5] allocate named register 'rax'
;   [218:5] allocate named register 'rdi'
;   [218:5] allocate named register 'rcx'
    xor al, al
    lea rdi, [rsp - 453]
    mov rcx, 40
    rep stosb
;   [218:5] free named register 'rcx'
;   [218:5] free named register 'rdi'
;   [218:5] free named register 'rax'
;   [219:5] o3.pos.y = 73
;   [219:16] 73
    mov qword [rsp - 445], 73
;   [220:5] # index 0 in an array can be accessed without array index
;   [222:5] assert(o3[0].pos.y == 73)
;   [222:12] allocate scratch register -> r15
;   [222:12] ? o3[0].pos.y == 73
;   [222:12] ? o3[0].pos.y == 73
    cmp_222_12:
;   [222:12] allocate scratch register -> r14
    lea r14, [rsp - 453]
;   [222:12] allocate scratch register -> r13
;   [222:15] set array index
;   [222:15] 0
    mov r13, 0
;   [222:15] bounds check
;   [222:15] allocate scratch register -> r12
;   [222:15] line number
    mov r12, 222
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
;   [222:15] free scratch register 'r12'
    imul r13, 20
    add r14, r13
;   [222:12] free scratch register 'r13'
    cmp qword [r14 + 8], 73
;   [222:12] free scratch register 'r14'
    sete r15b
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
;   [224:5] o3[1] = object_init()
;   [224:5] allocate scratch register -> r15
    lea r15, [rsp - 453]
;   [224:5] allocate scratch register -> r14
;   [224:8] set array index
;   [224:8] 1
    mov r14, 1
;   [224:8] bounds check
;   [224:8] allocate scratch register -> r13
;   [224:8] line number
    mov r13, 224
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [224:8] free scratch register 'r13'
    imul r14, 20
    add r15, r14
;   [224:5] free scratch register 'r14'
;   [224:13] object_init()
;   [105:6] object_init() : object res 
    object_init_224_13:
;       [224:13] alias res -> o3  (lea: r15)
;       [106:5] res.pos.y = 74
;       [106:17] 74
        mov qword [r15 + 8], 74
    object_init_224_13_end:
;   [224:5] free scratch register 'r15'
;   [225:5] assert(o3[1].pos.y == 74)
;   [225:12] allocate scratch register -> r15
;   [225:12] ? o3[1].pos.y == 74
;   [225:12] ? o3[1].pos.y == 74
    cmp_225_12:
;   [225:12] allocate scratch register -> r14
    lea r14, [rsp - 453]
;   [225:12] allocate scratch register -> r13
;   [225:15] set array index
;   [225:15] 1
    mov r13, 1
;   [225:15] bounds check
;   [225:15] allocate scratch register -> r12
;   [225:15] line number
    mov r12, 225
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
;   [225:15] free scratch register 'r12'
    imul r13, 20
    add r14, r13
;   [225:12] free scratch register 'r13'
    cmp qword [r14 + 8], 74
;   [225:12] free scratch register 'r14'
    sete r15b
    bool_end_225_12:
;   [13:6] assert(expr : bool) 
    assert_225_5:
;       [225:5] alias expr -> r15b  (lea: )
        if_13_29_225_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_225_5:
        cmp r15b, 0
        jne if_13_26_225_5_end
        if_13_29_225_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_225_5:
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
            exit_13_38_225_5_end:
        if_13_26_225_5_end:
;       [225:5] free scratch register 'r15'
    assert_225_5_end:
;   [227:5] var worlds : world[8]
;   [227:9] worlds: world[8] (512 B @ [rsp - 965])
;   [227:9] zero 8 * 64 B = 512 B
;   [227:5] allocate named register 'rax'
;   [227:5] allocate named register 'rdi'
;   [227:5] allocate named register 'rcx'
    xor al, al
    lea rdi, [rsp - 965]
    mov rcx, 512
    rep stosb
;   [227:5] free named register 'rcx'
;   [227:5] free named register 'rdi'
;   [227:5] free named register 'rax'
;   [228:5] worlds[1].locations[1] = 0xffee
;   [228:5] allocate scratch register -> r15
    lea r15, [rsp - 965]
;   [228:5] allocate scratch register -> r14
;   [228:12] set array index
;   [228:12] 1
    mov r14, 1
;   [228:12] bounds check
;   [228:12] allocate scratch register -> r13
;   [228:12] line number
    mov r13, 228
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [228:12] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [228:5] free scratch register 'r14'
;   [228:5] allocate scratch register -> r14
;   [228:25] set array index
;   [228:25] 1
    mov r14, 1
;   [228:25] bounds check
;   [228:25] allocate scratch register -> r13
;   [228:25] line number
    mov r13, 228
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [228:25] free scratch register 'r13'
;   [228:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [228:5] free scratch register 'r14'
;   [228:5] free scratch register 'r15'
;   [229:5] assert(worlds[1].locations[1] == 0xffee)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? worlds[1].locations[1] == 0xffee
;   [229:12] ? worlds[1].locations[1] == 0xffee
    cmp_229_12:
;   [229:12] allocate scratch register -> r14
    lea r14, [rsp - 965]
;   [229:12] allocate scratch register -> r13
;   [229:19] set array index
;   [229:19] 1
    mov r13, 1
;   [229:19] bounds check
;   [229:19] allocate scratch register -> r12
;   [229:19] line number
    mov r12, 229
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [229:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [229:12] free scratch register 'r13'
;   [229:12] allocate scratch register -> r13
;   [229:32] set array index
;   [229:32] 1
    mov r13, 1
;   [229:32] bounds check
;   [229:32] allocate scratch register -> r12
;   [229:32] line number
    mov r12, 229
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [229:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [229:12] free scratch register 'r13'
;   [229:12] free scratch register 'r14'
    sete r15b
    bool_end_229_12:
;   [13:6] assert(expr : bool) 
    assert_229_5:
;       [229:5] alias expr -> r15b  (lea: )
        if_13_29_229_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_229_5:
        cmp r15b, 0
        jne if_13_26_229_5_end
        if_13_29_229_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_229_5:
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
            exit_13_38_229_5_end:
        if_13_26_229_5_end:
;       [229:5] free scratch register 'r15'
    assert_229_5_end:
;   [231:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [231:5] allocate named register 'rsi'
;   [231:5] allocate named register 'rdi'
;   [231:5] allocate named register 'rcx'
;   [234:9] array_size_of(worlds.locations)
;   [234:9] rcx = array_size_of(worlds.locations)
;   [234:9] = expression
;   [234:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [232:9] worlds[1].locations
;   [232:9] allocate scratch register -> r15
    lea r15, [rsp - 965]
;   [232:9] allocate scratch register -> r14
;   [232:16] set array index
;   [232:16] 1
    mov r14, 1
;   [232:16] bounds check
;   [232:16] allocate scratch register -> r13
;   [232:16] line number
    mov r13, 232
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [232:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [232:9] free scratch register 'r14'
;   [232:9] bounds check
;   [232:9] allocate scratch register -> r14
;   [232:9] line number
    mov r14, 232
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [232:9] free scratch register 'r14'
    lea rsi, [r15]
;   [231:5] free scratch register 'r15'
;   [233:9] worlds[0].locations
;   [233:9] allocate scratch register -> r15
    lea r15, [rsp - 965]
;   [233:9] allocate scratch register -> r14
;   [233:16] set array index
;   [233:16] 0
    mov r14, 0
;   [233:16] bounds check
;   [233:16] allocate scratch register -> r13
;   [233:16] line number
    mov r13, 233
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [233:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [233:9] free scratch register 'r14'
;   [233:9] bounds check
;   [233:9] allocate scratch register -> r14
;   [233:9] line number
    mov r14, 233
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [233:9] free scratch register 'r14'
    lea rdi, [r15]
;   [231:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [231:5] free named register 'rcx'
;   [231:5] free named register 'rdi'
;   [231:5] free named register 'rsi'
;   [236:5] # `array_copy` is built-in and can use indexed positions
;   [237:5] # `array_size_of` is built-in
;   [239:5] assert(worlds[0].locations[1] == 0xffee)
;   [239:12] allocate scratch register -> r15
;   [239:12] ? worlds[0].locations[1] == 0xffee
;   [239:12] ? worlds[0].locations[1] == 0xffee
    cmp_239_12:
;   [239:12] allocate scratch register -> r14
    lea r14, [rsp - 965]
;   [239:12] allocate scratch register -> r13
;   [239:19] set array index
;   [239:19] 0
    mov r13, 0
;   [239:19] bounds check
;   [239:19] allocate scratch register -> r12
;   [239:19] line number
    mov r12, 239
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [239:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [239:12] free scratch register 'r13'
;   [239:12] allocate scratch register -> r13
;   [239:32] set array index
;   [239:32] 1
    mov r13, 1
;   [239:32] bounds check
;   [239:32] allocate scratch register -> r12
;   [239:32] line number
    mov r12, 239
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [239:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [239:12] free scratch register 'r13'
;   [239:12] free scratch register 'r14'
    sete r15b
    bool_end_239_12:
;   [13:6] assert(expr : bool) 
    assert_239_5:
;       [239:5] alias expr -> r15b  (lea: )
        if_13_29_239_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_239_5:
        cmp r15b, 0
        jne if_13_26_239_5_end
        if_13_29_239_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_239_5:
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
            exit_13_38_239_5_end:
        if_13_26_239_5_end:
;       [239:5] free scratch register 'r15'
    assert_239_5_end:
;   [240:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [240:12] allocate scratch register -> r15
;   [240:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [240:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_240_12:
;   [240:12] allocate scratch register -> r14
;       [240:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [240:12] = expression
;       [240:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [240:12] allocate named register 'rsi'
;       [240:12] allocate named register 'rdi'
;       [240:12] allocate named register 'rcx'
;       [243:14] array_size_of(worlds.locations)
;       [243:14] rcx = array_size_of(worlds.locations)
;       [243:14] = expression
;       [243:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [241:14] worlds[0].locations
;       [241:14] allocate scratch register -> r13
        lea r13, [rsp - 965]
;       [241:14] allocate scratch register -> r12
;       [241:21] set array index
;       [241:21] 0
        mov r12, 0
;       [241:21] bounds check
;       [241:21] allocate scratch register -> r11
;       [241:21] line number
        mov r11, 241
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [241:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [241:14] free scratch register 'r12'
;       [241:14] bounds check
;       [241:14] allocate scratch register -> r12
;       [241:14] line number
        mov r12, 241
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [241:14] free scratch register 'r12'
        lea rsi, [r13]
;       [240:12] free scratch register 'r13'
;       [242:14] worlds[1].locations
;       [242:14] allocate scratch register -> r13
        lea r13, [rsp - 965]
;       [242:14] allocate scratch register -> r12
;       [242:21] set array index
;       [242:21] 1
        mov r12, 1
;       [242:21] bounds check
;       [242:21] allocate scratch register -> r11
;       [242:21] line number
        mov r11, 242
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [242:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [242:14] free scratch register 'r12'
;       [242:14] bounds check
;       [242:14] allocate scratch register -> r12
;       [242:14] line number
        mov r12, 242
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [242:14] free scratch register 'r12'
        lea rdi, [r13]
;       [240:12] free scratch register 'r13'
        shl rcx, 3
        repe cmpsb
;       [240:12] free named register 'rcx'
;       [240:12] free named register 'rdi'
;       [240:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [240:12] free scratch register 'r14'
    setne r15b
    bool_end_240_12:
;   [13:6] assert(expr : bool) 
    assert_240_5:
;       [240:5] alias expr -> r15b  (lea: )
        if_13_29_240_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_240_5:
        cmp r15b, 0
        jne if_13_26_240_5_end
        if_13_29_240_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_240_5:
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
            exit_13_38_240_5_end:
        if_13_26_240_5_end:
;       [240:5] free scratch register 'r15'
    assert_240_5_end:
;   [246:5] var nm : str
;   [246:9] nm: str (128 B @ [rsp - 1093])
;   [246:9] zero 1 * 128 B = 128 B
;   [246:5] allocate named register 'rax'
;   [246:5] allocate named register 'rdi'
;   [246:5] allocate named register 'rcx'
    xor al, al
    lea rdi, [rsp - 1093]
    mov rcx, 128
    rep stosb
;   [246:5] free named register 'rcx'
;   [246:5] free named register 'rdi'
;   [246:5] free named register 'rax'
;   [247:5] print(hello)
;   [23:6] print(str : i8[]) 
    print_247_5:
;       [247:5] alias str -> hello  (lea: )
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
        sys_print_24_4_247_5:
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
        sys_print_24_4_247_5_end:
    print_247_5_end:
;   [248:5] loop
    loop_248_5:
;       [249:9] print(prompt1)
;       [23:6] print(str : i8[]) 
        print_249_9:
;           [249:9] alias str -> prompt1  (lea: )
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
            sys_print_24_4_249_9:
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
            sys_print_24_4_249_9_end:
        print_249_9_end:
;       [250:9] str_in(nm)
;       [81:6] str_in(s : str) 
        str_in_250_9:
;           [250:9] alias s -> nm  (lea: )
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
                lea rsi, [rsp - 1092]
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
                mov byte [rsp - 1093], al
;               [87:22] s.len - 1
                sub byte [rsp - 1093], 1
;           [87:25] # return value
        str_in_250_9_end:
        if_251_12:
;       [251:12] ? nm.len == 0
;       [251:12] ? nm.len == 0
        cmp_251_12:
        cmp byte [rsp - 1093], 0
        jne if_253_19
        if_251_12_code:
;           [252:13] break
            jmp loop_248_5_end
        jmp if_251_9_end
        if_253_19:
;       [253:19] ? nm.len <= 4
;       [253:19] ? nm.len <= 4
        cmp_253_19:
        cmp byte [rsp - 1093], 4
        jg if_else_251_9
        if_253_19_code:
;           [254:13] print(prompt2)
;           [23:6] print(str : i8[]) 
            print_254_13:
;               [254:13] alias str -> prompt2  (lea: )
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
                sys_print_24_4_254_13:
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
                sys_print_24_4_254_13_end:
            print_254_13_end:
;           [255:13] continue
            jmp loop_248_5
        jmp if_251_9_end
        if_else_251_9:
;           [257:13] print(prompt3)
;           [23:6] print(str : i8[]) 
            print_257_13:
;               [257:13] alias str -> prompt3  (lea: )
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
                sys_print_24_4_257_13:
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
                sys_print_24_4_257_13_end:
            print_257_13_end:
;           [258:13] str_out(nm)
;           [90:6] str_out(s : str) 
            str_out_258_13:
;               [258:13] alias s -> nm  (lea: )
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
                    lea rsi, [rsp - 1092]
;               [93:34] # buffer address
;               [94:5] mov(rdx, s.len)
;                   [94:14] s.len
                    movsx rdx, byte [rsp - 1093]
;               [94:21] # buffer size
;               [95:5] syscall()
                syscall
            str_out_258_13_end:
;           [259:13] print(dot)
;           [23:6] print(str : i8[]) 
            print_259_13:
;               [259:13] alias str -> dot  (lea: )
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
                sys_print_24_4_259_13:
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
                sys_print_24_4_259_13_end:
            print_259_13_end:
;           [260:13] print(nl)
;           [23:6] print(str : i8[]) 
            print_260_13:
;               [260:13] alias str -> nl  (lea: )
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
                sys_print_24_4_260_13:
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
                sys_print_24_4_260_13_end:
            print_260_13_end:
        if_251_9_end:
    jmp loop_248_5
    loop_248_5_end:
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
;               max stack size: 1093 B
;          optimization pass 1: 103
;          optimization pass 2: 0
```
