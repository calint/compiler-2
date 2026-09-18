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
* data
* variables
* constants
* arrays
* array iteration
* optional bounds checking at runtime
  * optional line number
* inlined functions
* keywords: `func`, `type`, `dat`, `var`, `const`, `foo`, `loop`, `if`, `else`,
  `continue`, `break`, `return`
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
C/C++ Header                    49           1857            881           7977
C++                              1             15              7            170
-------------------------------------------------------------------------------
SUM:                            50           1872            888           8147
-------------------------------------------------------------------------------
```

## Sample

```text
# user types are defined using keyword `type`

# built-in types are `i63`, `i32`, `i16`, `i8` and `bool`

# default type is `i64` and does not need to be specified

type point {x, y}

type object {pos : point, color : i32}

type world { locations : i64[8] }

type str {
    len : i8,
    data : i8[127]
}

# initial data is initialized before variables

dat   hello : i8[] = "hello world from baz\n"
dat prompt1 : i8[] = "enter name:\n"
dat prompt2 : i8[] = "that is not a name.\n"
dat prompt3 : i8[] = "hello "
dat     dot : i8[] = "."
dat      nl : i8[] = "\n"
dat    nums : i64[4] = { 1 } # remaining elements are zeroed
dat      s1 : str = { 3 } # remaining fields are zeroed

# all functions are inlined

# arguments can be placed in specified register using `reg_...` syntax

func exit(v : reg_rdi) {
    mov(rax, 60)  # exit system call
    mov(rdi, v)   # return code
    syscall()
}

# single statement blocks can ommit { ... }

func assert(x : bool) if not x exit(1)

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

# function arguments and return are equivalent to mutable references

func fooz(pt : point) {
    pt.x = 0b10    # binary value 2
    pt.y = 0xb     # hex value 11
}

# default argument type is `i64`

func bar(arg) {
    if arg == 0 return
    arg = 0xff
}

# return target is specified as a variable, in this case `res`

# return variable is a mutable reference to destination

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

func str_in(s : str) {
    mov(rax, 0)                     # read system call
    mov(rdi, 0)                     # file descriptor for standard input
    mov(rsi, address_of(s.data))    # buffer address
    mov(rdx, array_size_of(s.data)) # buffer size
    syscall()
    mov(s.len, rax - 1) # return value
} 

func str_out(s : str) {
    mov(rax, 1)                  # write system call
    mov(rdi, 0)                  # file descriptor for standard out
    mov(rsi, address_of(s.data)) # buffer address 
    mov(rdx, s.len)              # buffer size
    syscall()
} 

func point_init() : point res {
    res.x = -1
    res.y = -2
}

func object_init() : object res {
    res.pos.y = 74
}

const yes = 1
const no = 0
const maybe = -1

# constants can be declared in any scope and shadow outer declarations

func main() {
    var arr : i32[4]
    # arrays are initialized to 0

    var answer
    assert(answer == 0)
    # variables without initializer are zeroed

    answer = maybe
    assert(answer == -1)

    {
        const maybe = 33
        assert(maybe == 33)
    }

    assert(maybe == -1)

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

    var arr3 : i64[] = { 3, 5 }
    foo arr3 {
        e = e + i + n
    }
    assert(arr3[0] == 3 + 0 + 2)
    assert(arr3[1] == 5 + 1 + 2)
    # `foo` is a language construct that iterates over an array injecting:
    #   `e`: current element
    #   `i`: index starting at 0
    #   `n`: constant array size

    var p : point = {0, 0}
    fooz(p)
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

    var p0 : point = {baz(3), 0}
    assert(p0.x == 6)

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

    var arr2 : i64[] = { -1, 2 }
    assert(array_size_of(arr2) == 2)
    assert(arr2[0] == -1)
    assert(arr2[1] == 2)

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
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
    mov qword [rsp - 245], 0
    cmp_133_12:
    cmp qword [rsp - 245], 0
    sete r15b
    bool_end_133_12:
    assert_133_5:
        if_41_26_133_5:
        cmp_41_26_133_5:
        cmp r15b, 0
        jne if_41_23_133_5_end
        if_41_26_133_5_code:
            mov rdi, 1
            exit_41_32_133_5:
                    mov rax, 60
                syscall
            exit_41_32_133_5_end:
        if_41_23_133_5_end:
    assert_133_5_end:
    mov qword [rsp - 245], -1
    cmp_137_12:
    cmp qword [rsp - 245], -1
    sete r15b
    bool_end_137_12:
    assert_137_5:
        if_41_26_137_5:
        cmp_41_26_137_5:
        cmp r15b, 0
        jne if_41_23_137_5_end
        if_41_26_137_5_code:
            mov rdi, 1
            exit_41_32_137_5:
                    mov rax, 60
                syscall
            exit_41_32_137_5_end:
        if_41_23_137_5_end:
    assert_137_5_end:
        cmp_141_16:
        bool_end_141_16:
        mov r15b, 1
        assert_141_9:
            if_41_26_141_9:
            cmp_41_26_141_9:
            cmp r15b, 0
            jne if_41_23_141_9_end
            if_41_26_141_9_code:
                mov rdi, 1
                exit_41_32_141_9:
                        mov rax, 60
                    syscall
                exit_41_32_141_9_end:
            if_41_23_141_9_end:
        assert_141_9_end:
    cmp_144_12:
    bool_end_144_12:
    mov r15b, 1
    assert_144_5:
        if_41_26_144_5:
        cmp_41_26_144_5:
        cmp r15b, 0
        jne if_41_23_144_5_end
        if_41_26_144_5_code:
            mov rdi, 1
            exit_41_32_144_5:
                    mov rax, 60
                syscall
            exit_41_32_144_5_end:
        if_41_23_144_5_end:
    assert_144_5_end:
    mov qword [rsp - 253], 1
    mov r15, qword [rsp - 253]
    mov r14, 149
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 237], 2
    mov r15, qword [rsp - 253]
    add r15, 1
    mov r14, 150
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 253]
    mov r13, 150
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 237]
    mov dword [rsp + r15 * 4 - 237], r13d
    cmp_151_12:
    mov r14, 1
    mov r13, 151
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_151_12:
    assert_151_5:
        if_41_26_151_5:
        cmp_41_26_151_5:
        cmp r15b, 0
        jne if_41_23_151_5_end
        if_41_26_151_5_code:
            mov rdi, 1
            exit_41_32_151_5:
                    mov rax, 60
                syscall
            exit_41_32_151_5_end:
        if_41_23_151_5_end:
    assert_151_5_end:
    cmp_152_12:
    mov r14, 2
    mov r13, 152
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_152_12:
    assert_152_5:
        if_41_26_152_5:
        cmp_41_26_152_5:
        cmp r15b, 0
        jne if_41_23_152_5_end
        if_41_26_152_5_code:
            mov rdi, 1
            exit_41_32_152_5:
                    mov rax, 60
                syscall
            exit_41_32_152_5_end:
        if_41_23_152_5_end:
    assert_152_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 237]
    mov r15, 154
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 237]
    shl rcx, 2
    rep movsb
    cmp_156_12:
    mov r14, 0
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_41_26_156_5:
        cmp_41_26_156_5:
        cmp r15b, 0
        jne if_41_23_156_5_end
        if_41_26_156_5_code:
            mov rdi, 1
            exit_41_32_156_5:
                    mov rax, 60
                syscall
            exit_41_32_156_5_end:
        if_41_23_156_5_end:
    assert_156_5_end:
    mov qword [rsp - 285], 0
    mov qword [rsp - 277], 0
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
    mov rcx, 4
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 237]
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 285]
    shl rcx, 2
    rep movsb
    cmp_160_12:
        mov rcx, 4
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 237]
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 285]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_160_12:
    assert_160_5:
        if_41_26_160_5:
        cmp_41_26_160_5:
        cmp r15b, 0
        jne if_41_23_160_5_end
        if_41_26_160_5_code:
            mov rdi, 1
            exit_41_32_160_5:
                    mov rax, 60
                syscall
            exit_41_32_160_5_end:
        if_41_23_160_5_end:
    assert_160_5_end:
    mov r15, 2
    mov r14, 163
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 285], -1
    cmp_164_12:
        mov rcx, 4
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 237]
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 285]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_164_12:
    assert_164_5:
        if_41_26_164_5:
        cmp_41_26_164_5:
        cmp r15b, 0
        jne if_41_23_164_5_end
        if_41_26_164_5_code:
            mov rdi, 1
            exit_41_32_164_5:
                    mov rax, 60
                syscall
            exit_41_32_164_5_end:
        if_41_23_164_5_end:
    assert_164_5_end:
    mov qword [rsp - 253], 3
    mov r15, qword [rsp - 253]
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 253]
    sub r14, 1
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_167_16:
        mov r13d, dword [rsp + r14 * 4 - 237]
        mov dword [rsp + r15 * 4 - 237], r13d
        not dword [rsp + r15 * 4 - 237]
    inv_167_16_end:
    not dword [rsp + r15 * 4 - 237]
    cmp_168_12:
    mov r14, qword [rsp - 253]
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 2
    sete r15b
    bool_end_168_12:
    assert_168_5:
        if_41_26_168_5:
        cmp_41_26_168_5:
        cmp r15b, 0
        jne if_41_23_168_5_end
        if_41_26_168_5_code:
            mov rdi, 1
            exit_41_32_168_5:
                    mov rax, 60
                syscall
            exit_41_32_168_5_end:
        if_41_23_168_5_end:
    assert_168_5_end:
    faz_170_5:
        mov r15, 1
        mov r14, 93
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 237], 254
    faz_170_5_end:
    cmp_171_12:
    mov r14, 1
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 237], 254
    sete r15b
    bool_end_171_12:
    assert_171_5:
        if_41_26_171_5:
        cmp_41_26_171_5:
        cmp r15b, 0
        jne if_41_23_171_5_end
        if_41_26_171_5_code:
            mov rdi, 1
            exit_41_32_171_5:
                    mov rax, 60
                syscall
            exit_41_32_171_5_end:
        if_41_23_171_5_end:
    assert_171_5_end:
    mov qword [rsp - 301], 3
    mov qword [rsp - 293], 5
    lea r15, [rsp - 301]
    mov qword [rsp - 317], 0
    foo_174_5:
        mov r14, qword [rsp - 317]
        add qword [r15], r14
        add qword [r15], 2
        foo_174_5_continue:
            add r15, 8
            inc qword [rsp - 317]
            cmp qword [rsp - 317], 2
            jne foo_174_5
    foo_174_5_end:
    cmp_177_12:
    mov r14, 0
    mov r13, 177
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rsp + r14 * 8 - 301], r13
    sete r15b
    bool_end_177_12:
    assert_177_5:
        if_41_26_177_5:
        cmp_41_26_177_5:
        cmp r15b, 0
        jne if_41_23_177_5_end
        if_41_26_177_5_code:
            mov rdi, 1
            exit_41_32_177_5:
                    mov rax, 60
                syscall
            exit_41_32_177_5_end:
        if_41_23_177_5_end:
    assert_177_5_end:
    cmp_178_12:
    mov r14, 1
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rsp + r14 * 8 - 301], r13
    sete r15b
    bool_end_178_12:
    assert_178_5:
        if_41_26_178_5:
        cmp_41_26_178_5:
        cmp r15b, 0
        jne if_41_23_178_5_end
        if_41_26_178_5_code:
            mov rdi, 1
            exit_41_32_178_5:
                    mov rax, 60
                syscall
            exit_41_32_178_5_end:
        if_41_23_178_5_end:
    assert_178_5_end:
    mov qword [rsp - 317], 0
    mov qword [rsp - 309], 0
    fooz_185_5:
        mov qword [rsp - 317], 2
        mov qword [rsp - 309], 11
    fooz_185_5_end:
    cmp_186_12:
    cmp qword [rsp - 317], 2
    sete r15b
    bool_end_186_12:
    assert_186_5:
        if_41_26_186_5:
        cmp_41_26_186_5:
        cmp r15b, 0
        jne if_41_23_186_5_end
        if_41_26_186_5_code:
            mov rdi, 1
            exit_41_32_186_5:
                    mov rax, 60
                syscall
            exit_41_32_186_5_end:
        if_41_23_186_5_end:
    assert_186_5_end:
    cmp_187_12:
    cmp qword [rsp - 309], 11
    sete r15b
    bool_end_187_12:
    assert_187_5:
        if_41_26_187_5:
        cmp_41_26_187_5:
        cmp r15b, 0
        jne if_41_23_187_5_end
        if_41_26_187_5_code:
            mov rdi, 1
            exit_41_32_187_5:
                    mov rax, 60
                syscall
            exit_41_32_187_5_end:
        if_41_23_187_5_end:
    assert_187_5_end:
    mov rax, qword [rsp - 317]
    mov qword [rsp - 333], rax
    mov rax, qword [rsp - 309]
    mov qword [rsp - 325], rax
    cmp_190_12:
        lea rsi, [rsp - 317]
        lea rdi, [rsp - 333]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_190_12:
    assert_190_5:
        if_41_26_190_5:
        cmp_41_26_190_5:
        cmp r15b, 0
        jne if_41_23_190_5_end
        if_41_26_190_5_code:
            mov rdi, 1
            exit_41_32_190_5:
                    mov rax, 60
                syscall
            exit_41_32_190_5_end:
        if_41_23_190_5_end:
    assert_190_5_end:
    mov qword [rsp - 333], 3
    cmp_195_12:
        lea rsi, [rsp - 317]
        lea rdi, [rsp - 333]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_41_26_195_5:
        cmp_41_26_195_5:
        cmp r15b, 0
        jne if_41_23_195_5_end
        if_41_26_195_5_code:
            mov rdi, 1
            exit_41_32_195_5:
                    mov rax, 60
                syscall
            exit_41_32_195_5_end:
        if_41_23_195_5_end:
    assert_195_5_end:
    mov qword [rsp - 341], 0
    bar_198_5:
        if_74_8_198_5:
        cmp_74_8_198_5:
        cmp qword [rsp - 341], 0
        jne if_74_5_198_5_end
        if_74_8_198_5_code:
            jmp bar_198_5_end
        if_74_5_198_5_end:
        mov qword [rsp - 341], 255
    bar_198_5_end:
    cmp_199_12:
    cmp qword [rsp - 341], 0
    sete r15b
    bool_end_199_12:
    assert_199_5:
        if_41_26_199_5:
        cmp_41_26_199_5:
        cmp r15b, 0
        jne if_41_23_199_5_end
        if_41_26_199_5_code:
            mov rdi, 1
            exit_41_32_199_5:
                    mov rax, 60
                syscall
            exit_41_32_199_5_end:
        if_41_23_199_5_end:
    assert_199_5_end:
    mov qword [rsp - 341], 1
    bar_202_5:
        if_74_8_202_5:
        cmp_74_8_202_5:
        cmp qword [rsp - 341], 0
        jne if_74_5_202_5_end
        if_74_8_202_5_code:
            jmp bar_202_5_end
        if_74_5_202_5_end:
        mov qword [rsp - 341], 255
    bar_202_5_end:
    cmp_203_12:
    cmp qword [rsp - 341], 255
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_41_26_203_5:
        cmp_41_26_203_5:
        cmp r15b, 0
        jne if_41_23_203_5_end
        if_41_26_203_5_code:
            mov rdi, 1
            exit_41_32_203_5:
                    mov rax, 60
                syscall
            exit_41_32_203_5_end:
        if_41_23_203_5_end:
    assert_203_5_end:
    mov qword [rsp - 349], 1
    baz_206_13:
        mov r15, qword [rsp - 349]
        imul r15, 2
        mov qword [rsp - 357], r15
    baz_206_13_end:
    cmp_207_12:
    cmp qword [rsp - 357], 2
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_41_26_207_5:
        cmp_41_26_207_5:
        cmp r15b, 0
        jne if_41_23_207_5_end
        if_41_26_207_5_code:
            mov rdi, 1
            exit_41_32_207_5:
                    mov rax, 60
                syscall
            exit_41_32_207_5_end:
        if_41_23_207_5_end:
    assert_207_5_end:
    baz_209_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 357], r15
    baz_209_9_end:
    cmp_210_12:
    cmp qword [rsp - 357], 2
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_41_26_210_5:
        cmp_41_26_210_5:
        cmp r15b, 0
        jne if_41_23_210_5_end
        if_41_26_210_5_code:
            mov rdi, 1
            exit_41_32_210_5:
                    mov rax, 60
                syscall
            exit_41_32_210_5_end:
        if_41_23_210_5_end:
    assert_210_5_end:
    baz_212_23:
        mov r15, 3
        imul r15, 2
        mov qword [rsp - 373], r15
    baz_212_23_end:
    mov qword [rsp - 365], 0
    cmp_213_12:
    cmp qword [rsp - 373], 6
    sete r15b
    bool_end_213_12:
    assert_213_5:
        if_41_26_213_5:
        cmp_41_26_213_5:
        cmp r15b, 0
        jne if_41_23_213_5_end
        if_41_26_213_5_code:
            mov rdi, 1
            exit_41_32_213_5:
                    mov rax, 60
                syscall
            exit_41_32_213_5_end:
        if_41_23_213_5_end:
    assert_213_5_end:
    point_init_215_22:
        mov qword [rsp - 389], -1
        mov qword [rsp - 381], -2
    point_init_215_22_end:
    cmp_216_12:
    cmp qword [rsp - 389], -1
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_41_26_216_5:
        cmp_41_26_216_5:
        cmp r15b, 0
        jne if_41_23_216_5_end
        if_41_26_216_5_code:
            mov rdi, 1
            exit_41_32_216_5:
                    mov rax, 60
                syscall
            exit_41_32_216_5_end:
        if_41_23_216_5_end:
    assert_216_5_end:
    cmp_217_12:
    cmp qword [rsp - 381], -2
    sete r15b
    bool_end_217_12:
    assert_217_5:
        if_41_26_217_5:
        cmp_41_26_217_5:
        cmp r15b, 0
        jne if_41_23_217_5_end
        if_41_26_217_5_code:
            mov rdi, 1
            exit_41_32_217_5:
                    mov rax, 60
                syscall
            exit_41_32_217_5_end:
        if_41_23_217_5_end:
    assert_217_5_end:
    mov qword [rsp - 397], 1
    mov qword [rsp - 405], 2
    mov r15, qword [rsp - 397]
    imul r15, 10
    mov qword [rsp - 425], r15
    mov r15, qword [rsp - 405]
    mov qword [rsp - 417], r15
    mov dword [rsp - 409], 16711680
    cmp_223_12:
    cmp qword [rsp - 425], 10
    sete r15b
    bool_end_223_12:
    assert_223_5:
        if_41_26_223_5:
        cmp_41_26_223_5:
        cmp r15b, 0
        jne if_41_23_223_5_end
        if_41_26_223_5_code:
            mov rdi, 1
            exit_41_32_223_5:
                    mov rax, 60
                syscall
            exit_41_32_223_5_end:
        if_41_23_223_5_end:
    assert_223_5_end:
    cmp_224_12:
    cmp qword [rsp - 417], 2
    sete r15b
    bool_end_224_12:
    assert_224_5:
        if_41_26_224_5:
        cmp_41_26_224_5:
        cmp r15b, 0
        jne if_41_23_224_5_end
        if_41_26_224_5_code:
            mov rdi, 1
            exit_41_32_224_5:
                    mov rax, 60
                syscall
            exit_41_32_224_5_end:
        if_41_23_224_5_end:
    assert_224_5_end:
    cmp_225_12:
    cmp dword [rsp - 409], 16711680
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_41_26_225_5:
        cmp_41_26_225_5:
        cmp r15b, 0
        jne if_41_23_225_5_end
        if_41_26_225_5_code:
            mov rdi, 1
            exit_41_32_225_5:
                    mov rax, 60
                syscall
            exit_41_32_225_5_end:
        if_41_23_225_5_end:
    assert_225_5_end:
    mov r15, qword [rsp - 397]
    mov qword [rsp - 441], r15
    neg qword [rsp - 441]
    mov r15, qword [rsp - 405]
    mov qword [rsp - 433], r15
    neg qword [rsp - 433]
    mov rax, qword [rsp - 441]
    mov qword [rsp - 425], rax
    mov rax, qword [rsp - 433]
    mov qword [rsp - 417], rax
    cmp_229_12:
    cmp qword [rsp - 425], -1
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_41_26_229_5:
        cmp_41_26_229_5:
        cmp r15b, 0
        jne if_41_23_229_5_end
        if_41_26_229_5_code:
            mov rdi, 1
            exit_41_32_229_5:
                    mov rax, 60
                syscall
            exit_41_32_229_5_end:
        if_41_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    cmp qword [rsp - 417], -2
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_41_26_230_5:
        cmp_41_26_230_5:
        cmp r15b, 0
        jne if_41_23_230_5_end
        if_41_26_230_5_code:
            mov rdi, 1
            exit_41_32_230_5:
                    mov rax, 60
                syscall
            exit_41_32_230_5_end:
        if_41_23_230_5_end:
    assert_230_5_end:
    lea rsi, [rsp - 425]
    lea rdi, [rsp - 461]
    mov rcx, 20
    rep movsb
    cmp_233_12:
    cmp qword [rsp - 461], -1
    sete r15b
    bool_end_233_12:
    assert_233_5:
        if_41_26_233_5:
        cmp_41_26_233_5:
        cmp r15b, 0
        jne if_41_23_233_5_end
        if_41_26_233_5_code:
            mov rdi, 1
            exit_41_32_233_5:
                    mov rax, 60
                syscall
            exit_41_32_233_5_end:
        if_41_23_233_5_end:
    assert_233_5_end:
    cmp_234_12:
    cmp qword [rsp - 453], -2
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_41_26_234_5:
        cmp_41_26_234_5:
        cmp r15b, 0
        jne if_41_23_234_5_end
        if_41_26_234_5_code:
            mov rdi, 1
            exit_41_32_234_5:
                    mov rax, 60
                syscall
            exit_41_32_234_5_end:
        if_41_23_234_5_end:
    assert_234_5_end:
    cmp_235_12:
    cmp dword [rsp - 445], 16711680
    sete r15b
    bool_end_235_12:
    assert_235_5:
        if_41_26_235_5:
        cmp_41_26_235_5:
        cmp r15b, 0
        jne if_41_23_235_5_end
        if_41_26_235_5_code:
            mov rdi, 1
            exit_41_32_235_5:
                    mov rax, 60
                syscall
            exit_41_32_235_5_end:
        if_41_23_235_5_end:
    assert_235_5_end:
    xor al, al
    lea rdi, [rsp - 501]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 493], 73
    cmp_241_12:
    lea r14, [rsp - 501]
    mov r13, 0
    mov r12, 241
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
    bool_end_241_12:
    assert_241_5:
        if_41_26_241_5:
        cmp_41_26_241_5:
        cmp r15b, 0
        jne if_41_23_241_5_end
        if_41_26_241_5_code:
            mov rdi, 1
            exit_41_32_241_5:
                    mov rax, 60
                syscall
            exit_41_32_241_5_end:
        if_41_23_241_5_end:
    assert_241_5_end:
    lea r15, [rsp - 501]
    mov r14, 1
    mov r13, 243
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_243_13:
        mov qword [r15 + 8], 74
    object_init_243_13_end:
    cmp_244_12:
    lea r14, [rsp - 501]
    mov r13, 1
    mov r12, 244
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
    bool_end_244_12:
    assert_244_5:
        if_41_26_244_5:
        cmp_41_26_244_5:
        cmp r15b, 0
        jne if_41_23_244_5_end
        if_41_26_244_5_code:
            mov rdi, 1
            exit_41_32_244_5:
                    mov rax, 60
                syscall
            exit_41_32_244_5_end:
        if_41_23_244_5_end:
    assert_244_5_end:
    xor al, al
    lea rdi, [rsp - 1013]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 1013]
    mov r14, 1
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_248_12:
    lea r14, [rsp - 1013]
    mov r13, 1
    mov r12, 248
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 248
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_248_12:
    assert_248_5:
        if_41_26_248_5:
        cmp_41_26_248_5:
        cmp r15b, 0
        jne if_41_23_248_5_end
        if_41_26_248_5_code:
            mov rdi, 1
            exit_41_32_248_5:
                    mov rax, 60
                syscall
            exit_41_32_248_5_end:
        if_41_23_248_5_end:
    assert_248_5_end:
    mov rcx, 8
    lea r15, [rsp - 1013]
    mov r14, 1
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 251
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 1013]
    mov r14, 0
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 252
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_258_12:
    lea r14, [rsp - 1013]
    mov r13, 0
    mov r12, 258
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 258
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_258_12:
    assert_258_5:
        if_41_26_258_5:
        cmp_41_26_258_5:
        cmp r15b, 0
        jne if_41_23_258_5_end
        if_41_26_258_5_code:
            mov rdi, 1
            exit_41_32_258_5:
                    mov rax, 60
                syscall
            exit_41_32_258_5_end:
        if_41_23_258_5_end:
    assert_258_5_end:
    cmp_259_12:
        mov rcx, 8
        lea r13, [rsp - 1013]
        mov r12, 0
        mov r11, 260
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 260
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 1013]
        mov r12, 1
        mov r11, 261
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 261
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
    cmp r14b, 0
    setne r15b
    bool_end_259_12:
    assert_259_5:
        if_41_26_259_5:
        cmp_41_26_259_5:
        cmp r15b, 0
        jne if_41_23_259_5_end
        if_41_26_259_5_code:
            mov rdi, 1
            exit_41_32_259_5:
                    mov rax, 60
                syscall
            exit_41_32_259_5_end:
        if_41_23_259_5_end:
    assert_259_5_end:
    mov qword [rsp - 1029], -1
    mov qword [rsp - 1021], 2
    cmp_266_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_266_12:
    assert_266_5:
        if_41_26_266_5:
        cmp_41_26_266_5:
        cmp r15b, 0
        jne if_41_23_266_5_end
        if_41_26_266_5_code:
            mov rdi, 1
            exit_41_32_266_5:
                    mov rax, 60
                syscall
            exit_41_32_266_5_end:
        if_41_23_266_5_end:
    assert_266_5_end:
    cmp_267_12:
    mov r14, 0
    mov r13, 267
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1029], -1
    sete r15b
    bool_end_267_12:
    assert_267_5:
        if_41_26_267_5:
        cmp_41_26_267_5:
        cmp r15b, 0
        jne if_41_23_267_5_end
        if_41_26_267_5_code:
            mov rdi, 1
            exit_41_32_267_5:
                    mov rax, 60
                syscall
            exit_41_32_267_5_end:
        if_41_23_267_5_end:
    assert_267_5_end:
    cmp_268_12:
    mov r14, 1
    mov r13, 268
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1029], 2
    sete r15b
    bool_end_268_12:
    assert_268_5:
        if_41_26_268_5:
        cmp_41_26_268_5:
        cmp r15b, 0
        jne if_41_23_268_5_end
        if_41_26_268_5_code:
            mov rdi, 1
            exit_41_32_268_5:
                    mov rax, 60
                syscall
            exit_41_32_268_5_end:
        if_41_23_268_5_end:
    assert_268_5_end:
    xor al, al
    lea rdi, [rsp - 1157]
    mov rcx, 128
    rep stosb
    print_271_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_52_4_271_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_52_4_271_5_end:
    print_271_5_end:
    loop_272_5:
        print_273_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_52_4_273_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_52_4_273_9_end:
        print_273_9_end:
        str_in_274_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1156]
                mov rdx, 127
            syscall
                mov byte [rsp - 1157], al
                sub byte [rsp - 1157], 1
        str_in_274_9_end:
        if_275_12:
        cmp_275_12:
        cmp byte [rsp - 1157], 0
        jne if_277_19
        if_275_12_code:
            jmp loop_272_5_end
        jmp if_275_9_end
        if_277_19:
        cmp_277_19:
        cmp byte [rsp - 1157], 4
        jg if_else_275_9
        if_277_19_code:
            print_278_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_52_4_278_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_278_13_end:
            print_278_13_end:
            jmp loop_272_5
        jmp if_275_9_end
        if_else_275_9:
            print_281_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_52_4_281_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_281_13_end:
            print_281_13_end:
            str_out_282_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1156]
                    movsx rdx, byte [rsp - 1157]
                syscall
            str_out_282_13_end:
            print_283_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_52_4_283_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_283_13_end:
            print_283_13_end:
            print_284_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_52_4_284_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_52_4_284_13_end:
            print_284_13_end:
        if_275_9_end:
    jmp loop_272_5
    loop_272_5_end:
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
dq 1
times 24 db 0
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

;[1:1] # user types are defined using keyword `type`
;[3:1] # built-in types are `i63`, `i32`, `i16`, `i8` and `bool`
;[5:1] # default type is `i64` and does not need to be specified
;[7:1] point : 16 B    fields:
;[7:1]       name :  offset :    size :  array? : array size
;[7:1]          x :       0 :       8 :      no :           
;[7:1]          y :       8 :       8 :      no :           

;[9:1] object : 20 B    fields:
;[9:1]       name :  offset :    size :  array? : array size
;[9:1]        pos :       0 :      16 :      no :           
;[9:1]      color :      16 :       4 :      no :           

;[11:1] world : 64 B    fields:
;[11:1]       name :  offset :    size :  array? : array size
;[11:1]  locations :       0 :      64 :     yes :          8

;[13:1] str : 128 B    fields:
;[13:1]       name :  offset :    size :  array? : array size
;[13:1]        len :       0 :       1 :      no :           
;[13:1]       data :       1 :     127 :     yes :        127

;[18:1] # initial data is initialized before variables
;[20:1] dat hello : i8[] = "hello world from baz\n"
;[20:7] hello: i8[21] (21 B @ [rsp - 21])
;[21:1] dat prompt1 : i8[] = "enter name:\n"
;[21:5] prompt1: i8[12] (12 B @ [rsp - 33])
;[22:1] dat prompt2 : i8[] = "that is not a name.\n"
;[22:5] prompt2: i8[20] (20 B @ [rsp - 53])
;[23:1] dat prompt3 : i8[] = "hello "
;[23:5] prompt3: i8[6] (6 B @ [rsp - 59])
;[24:1] dat dot : i8[] = "."
;[24:9] dot: i8[1] (1 B @ [rsp - 60])
;[25:1] dat nl : i8[] = "\n"
;[25:10] nl: i8[1] (1 B @ [rsp - 61])
;[26:1] dat nums : i64[4] = { 1 }
;[26:8] nums: i64[4] (32 B @ [rsp - 93])
;[26:30] # remaining elements are zeroed
;[27:1] dat s1 : str = { 3 }
;[27:10] s1: str (128 B @ [rsp - 221])
;[27:27] # remaining fields are zeroed
;[29:1] # all functions are inlined
;[31:1] # arguments can be placed in specified register using `reg_...` syntax
;[39:1] # single statement blocks can ommit { ... }
;[64:1] # function arguments and return are equivalent to mutable references
;[71:1] # default argument type is `i64`
;[78:1] # return target is specified as a variable, in this case `res`
;[80:1] # return variable is a mutable reference to destination
;[90:1] # array arguments are declared with type and []
;[122:7] const yes = 1
;[123:7] const no = 0
;[124:7] const maybe = -1
;[126:1] # constants can be declared in any scope and shadow outer declarations

main:
;   [129:5] var arr : i32[4]
;   [129:9] arr: i32[4] (16 B @ [rsp - 237])
;   [129:9] zero 4 * 4 B = 16 B
;   [129:5] size <= 32 B, use mov
    mov qword [rsp - 237], 0
    mov qword [rsp - 229], 0
;   [130:5] # arrays are initialized to 0
;   [132:5] var answer
;   [132:9] answer: i64 (8 B @ [rsp - 245])
;   [132:9] zero 1 * 8 B = 8 B
;   [132:5] size <= 32 B, use mov
    mov qword [rsp - 245], 0
;   [133:5] assert(answer == 0)
;   [133:12] allocate scratch register -> r15
;   [133:12] ? answer == 0
;   [133:12] ? answer == 0
    cmp_133_12:
    cmp qword [rsp - 245], 0
    sete r15b
    bool_end_133_12:
;   [41:6] assert(x : bool)
    assert_133_5:
;       [133:5] alias x -> r15b
        if_41_26_133_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_133_5:
        cmp r15b, 0
        jne if_41_23_133_5_end
        if_41_26_133_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_133_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_133_5_end:
        if_41_23_133_5_end:
;       [133:5] free scratch register r15
    assert_133_5_end:
;   [134:5] # variables without initializer are zeroed
;   [136:5] answer = maybe
;   [136:14] instructions without scratch register 1, with 2
;   [136:14] maybe
    mov qword [rsp - 245], -1
;   [137:5] assert(answer == -1)
;   [137:12] allocate scratch register -> r15
;   [137:12] ? answer == -1
;   [137:12] ? answer == -1
    cmp_137_12:
    cmp qword [rsp - 245], -1
    sete r15b
    bool_end_137_12:
;   [41:6] assert(x : bool)
    assert_137_5:
;       [137:5] alias x -> r15b
        if_41_26_137_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_137_5:
        cmp r15b, 0
        jne if_41_23_137_5_end
        if_41_26_137_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_137_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_137_5_end:
        if_41_23_137_5_end:
;       [137:5] free scratch register r15
    assert_137_5_end:
;       [140:15] const maybe = 33
;       [141:9] assert(maybe == 33)
;       [141:16] allocate scratch register -> r15
;       [141:16] ? maybe == 33
;       [141:16] ? maybe == 33
        cmp_141_16:
;       [141:16] const eval to true
        bool_end_141_16:
        mov r15b, 1
;       [41:6] assert(x : bool)
        assert_141_9:
;           [141:9] alias x -> r15b
            if_41_26_141_9:
;           [41:26] ? not x
;           [41:26] ? not x
            cmp_41_26_141_9:
            cmp r15b, 0
            jne if_41_23_141_9_end
            if_41_26_141_9_code:
;               [41:32] exit(1)
;               [41:37] allocate named register rdi
                mov rdi, 1
;               [33:6] exit(v : reg_rdi)
                exit_41_32_141_9:
;                   [41:32] alias v -> rdi
;                   [34:5] mov(rax, 60)
;                       [34:14] 60
                        mov rax, 60
;                   [34:19] # exit system call
;                   [35:5] mov(rdi, v)
;                       [35:14] v
;                   [35:19] # return code
;                   [36:5] syscall()
                    syscall
;                   [41:32] free named register rdi
                exit_41_32_141_9_end:
            if_41_23_141_9_end:
;           [141:9] free scratch register r15
        assert_141_9_end:
;   [144:5] assert(maybe == -1)
;   [144:12] allocate scratch register -> r15
;   [144:12] ? maybe == -1
;   [144:12] ? maybe == -1
    cmp_144_12:
;   [144:12] const eval to true
    bool_end_144_12:
    mov r15b, 1
;   [41:6] assert(x : bool)
    assert_144_5:
;       [144:5] alias x -> r15b
        if_41_26_144_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_144_5:
        cmp r15b, 0
        jne if_41_23_144_5_end
        if_41_26_144_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_144_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_144_5_end:
        if_41_23_144_5_end:
;       [144:5] free scratch register r15
    assert_144_5_end:
;   [146:5] var ix = 1
;   [146:9] ix: i64 (8 B @ [rsp - 253])
;   [146:9] ix = 1
;   [146:14] instructions without scratch register 1, with 2
;   [146:14] 1
    mov qword [rsp - 253], 1
;   [147:5] # variables can have an initial value that can be an expression
;   [149:5] arr[ix] = 2
;   [149:5] allocate scratch register -> r15
;   [149:9] set array index
;   [149:9] ix
    mov r15, qword [rsp - 253]
;   [149:9] bounds check
;   [149:9] allocate scratch register -> r14
;   [149:9] line number
    mov r14, 149
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [149:9] free scratch register r14
;   [149:15] instructions without scratch register 1, with 2
;   [149:15] 2
    mov dword [rsp + r15 * 4 - 237], 2
;   [149:5] free scratch register r15
;   [150:5] arr[ix + 1] = arr[ix]
;   [150:5] allocate scratch register -> r15
;   [150:9] set array index
;   [150:9] ix
    mov r15, qword [rsp - 253]
;   [150:14] r15 + 1
    add r15, 1
;   [150:9] bounds check
;   [150:9] allocate scratch register -> r14
;   [150:9] line number
    mov r14, 150
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [150:9] free scratch register r14
;   [150:19] instructions without scratch register 10, with 10
;   [150:19] arr[ix]
;   [150:19] allocate scratch register -> r14
;   [150:23] set array index
;   [150:23] ix
    mov r14, qword [rsp - 253]
;   [150:23] bounds check
;   [150:23] allocate scratch register -> r13
;   [150:23] line number
    mov r13, 150
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [150:23] free scratch register r13
;   [150:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 237]
    mov dword [rsp + r15 * 4 - 237], r13d
;   [150:19] free scratch register r13
;   [150:19] free scratch register r14
;   [150:5] free scratch register r15
;   [151:5] assert(arr[1] == 2)
;   [151:12] allocate scratch register -> r15
;   [151:12] ? arr[1] == 2
;   [151:12] ? arr[1] == 2
    cmp_151_12:
;   [151:12] allocate scratch register -> r14
;   [151:16] set array index
;   [151:16] 1
    mov r14, 1
;   [151:16] bounds check
;   [151:16] allocate scratch register -> r13
;   [151:16] line number
    mov r13, 151
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [151:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 237], 2
;   [151:12] free scratch register r14
    sete r15b
    bool_end_151_12:
;   [41:6] assert(x : bool)
    assert_151_5:
;       [151:5] alias x -> r15b
        if_41_26_151_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_151_5:
        cmp r15b, 0
        jne if_41_23_151_5_end
        if_41_26_151_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_151_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_151_5_end:
        if_41_23_151_5_end:
;       [151:5] free scratch register r15
    assert_151_5_end:
;   [152:5] assert(arr[2] == 2)
;   [152:12] allocate scratch register -> r15
;   [152:12] ? arr[2] == 2
;   [152:12] ? arr[2] == 2
    cmp_152_12:
;   [152:12] allocate scratch register -> r14
;   [152:16] set array index
;   [152:16] 2
    mov r14, 2
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
;   [152:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 237], 2
;   [152:12] free scratch register r14
    sete r15b
    bool_end_152_12:
;   [41:6] assert(x : bool)
    assert_152_5:
;       [152:5] alias x -> r15b
        if_41_26_152_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_152_5:
        cmp r15b, 0
        jne if_41_23_152_5_end
        if_41_26_152_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_152_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_152_5_end:
        if_41_23_152_5_end:
;       [152:5] free scratch register r15
    assert_152_5_end:
;   [154:5] array_copy(arr[2], arr, 2)
;   [154:5] allocate named register rsi
;   [154:5] allocate named register rdi
;   [154:5] allocate named register rcx
;   [154:29] 2
;   [154:29] 2
    mov rcx, 2
;   [154:16] arr[2]
;   [154:16] allocate scratch register -> r15
;   [154:20] set array index
;   [154:20] 2
    mov r15, 2
;   [154:20] bounds check
;   [154:20] allocate scratch register -> r14
;   [154:20] line number
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [154:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [154:20] free scratch register r13
    cmovg rbp, r14
    jg panic_bounds
;   [154:20] free scratch register r14
    lea rsi, [rsp + r15 * 4 - 237]
;   [154:5] free scratch register r15
;   [154:24] arr
;   [154:24] bounds check
;   [154:24] allocate scratch register -> r15
;   [154:24] line number
    mov r15, 154
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [154:24] free scratch register r15
    lea rdi, [rsp - 237]
    shl rcx, 2
    rep movsb
;   [154:5] free named register rcx
;   [154:5] free named register rdi
;   [154:5] free named register rsi
;   [155:5] # copy from, to, number of elements
;   [156:5] assert(arr[0] == 2)
;   [156:12] allocate scratch register -> r15
;   [156:12] ? arr[0] == 2
;   [156:12] ? arr[0] == 2
    cmp_156_12:
;   [156:12] allocate scratch register -> r14
;   [156:16] set array index
;   [156:16] 0
    mov r14, 0
;   [156:16] bounds check
;   [156:16] allocate scratch register -> r13
;   [156:16] line number
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [156:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 237], 2
;   [156:12] free scratch register r14
    sete r15b
    bool_end_156_12:
;   [41:6] assert(x : bool)
    assert_156_5:
;       [156:5] alias x -> r15b
        if_41_26_156_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_156_5:
        cmp r15b, 0
        jne if_41_23_156_5_end
        if_41_26_156_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_156_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_156_5_end:
        if_41_23_156_5_end:
;       [156:5] free scratch register r15
    assert_156_5_end:
;   [158:5] var arr1 : i32[8]
;   [158:9] arr1: i32[8] (32 B @ [rsp - 285])
;   [158:9] zero 8 * 4 B = 32 B
;   [158:5] size <= 32 B, use mov
    mov qword [rsp - 285], 0
    mov qword [rsp - 277], 0
    mov qword [rsp - 269], 0
    mov qword [rsp - 261], 0
;   [159:5] array_copy(arr, arr1, 4)
;   [159:5] allocate named register rsi
;   [159:5] allocate named register rdi
;   [159:5] allocate named register rcx
;   [159:27] 4
;   [159:27] 4
    mov rcx, 4
;   [159:16] arr
;   [159:16] bounds check
;   [159:16] allocate scratch register -> r15
;   [159:16] line number
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [159:16] free scratch register r15
    lea rsi, [rsp - 237]
;   [159:21] arr1
;   [159:21] bounds check
;   [159:21] allocate scratch register -> r15
;   [159:21] line number
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [159:21] free scratch register r15
    lea rdi, [rsp - 285]
    shl rcx, 2
    rep movsb
;   [159:5] free named register rcx
;   [159:5] free named register rdi
;   [159:5] free named register rsi
;   [160:5] assert(arrays_equal(arr, arr1, 4))
;   [160:12] allocate scratch register -> r15
;   [160:12] ? arrays_equal(arr, arr1, 4)
;   [160:12] ? arrays_equal(arr, arr1, 4)
    cmp_160_12:
;   [160:12] allocate scratch register -> r14
;       [160:12] r14 = arrays_equal(arr, arr1, 4)
;       [160:12] = expression
;       [160:12] arrays_equal(arr, arr1, 4)
;       [160:12] allocate named register rsi
;       [160:12] allocate named register rdi
;       [160:12] allocate named register rcx
;       [160:36] 4
;       [160:36] 4
        mov rcx, 4
;       [160:25] arr
;       [160:25] bounds check
;       [160:25] allocate scratch register -> r13
;       [160:25] line number
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [160:25] free scratch register r13
        lea rsi, [rsp - 237]
;       [160:30] arr1
;       [160:30] bounds check
;       [160:30] allocate scratch register -> r13
;       [160:30] line number
        mov r13, 160
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [160:30] free scratch register r13
        lea rdi, [rsp - 285]
        shl rcx, 2
        repe cmpsb
;       [160:12] free named register rcx
;       [160:12] free named register rdi
;       [160:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [160:12] free scratch register r14
    setne r15b
    bool_end_160_12:
;   [41:6] assert(x : bool)
    assert_160_5:
;       [160:5] alias x -> r15b
        if_41_26_160_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_160_5:
        cmp r15b, 0
        jne if_41_23_160_5_end
        if_41_26_160_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_160_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_160_5_end:
        if_41_23_160_5_end:
;       [160:5] free scratch register r15
    assert_160_5_end:
;   [161:5] # `arrays_equal` is built-in function
;   [163:5] arr1[2] = -1
;   [163:5] allocate scratch register -> r15
;   [163:10] set array index
;   [163:10] 2
    mov r15, 2
;   [163:10] bounds check
;   [163:10] allocate scratch register -> r14
;   [163:10] line number
    mov r14, 163
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [163:10] free scratch register r14
;   [163:15] instructions without scratch register 1, with 2
;   [163:16] -1
    mov dword [rsp + r15 * 4 - 285], -1
;   [163:5] free scratch register r15
;   [164:5] assert(not arrays_equal(arr, arr1, 4))
;   [164:12] allocate scratch register -> r15
;   [164:12] ? not arrays_equal(arr, arr1, 4)
;   [164:12] ? not arrays_equal(arr, arr1, 4)
    cmp_164_12:
;   [164:16] allocate scratch register -> r14
;       [164:16] r14 = arrays_equal(arr, arr1, 4)
;       [164:16] = expression
;       [164:16] arrays_equal(arr, arr1, 4)
;       [164:16] allocate named register rsi
;       [164:16] allocate named register rdi
;       [164:16] allocate named register rcx
;       [164:40] 4
;       [164:40] 4
        mov rcx, 4
;       [164:29] arr
;       [164:29] bounds check
;       [164:29] allocate scratch register -> r13
;       [164:29] line number
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [164:29] free scratch register r13
        lea rsi, [rsp - 237]
;       [164:34] arr1
;       [164:34] bounds check
;       [164:34] allocate scratch register -> r13
;       [164:34] line number
        mov r13, 164
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [164:34] free scratch register r13
        lea rdi, [rsp - 285]
        shl rcx, 2
        repe cmpsb
;       [164:16] free named register rcx
;       [164:16] free named register rdi
;       [164:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [164:12] free scratch register r14
    sete r15b
    bool_end_164_12:
;   [41:6] assert(x : bool)
    assert_164_5:
;       [164:5] alias x -> r15b
        if_41_26_164_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_164_5:
        cmp r15b, 0
        jne if_41_23_164_5_end
        if_41_26_164_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_164_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_164_5_end:
        if_41_23_164_5_end:
;       [164:5] free scratch register r15
    assert_164_5_end:
;   [166:5] ix = 3
;   [166:10] instructions without scratch register 1, with 2
;   [166:10] 3
    mov qword [rsp - 253], 3
;   [167:5] arr[ix] = ~inv(arr[ix - 1])
;   [167:5] allocate scratch register -> r15
;   [167:9] set array index
;   [167:9] ix
    mov r15, qword [rsp - 253]
;   [167:9] bounds check
;   [167:9] allocate scratch register -> r14
;   [167:9] line number
    mov r14, 167
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [167:9] free scratch register r14
;   [167:15] instructions without scratch register 15, with 15
;   [167:16] arr = ~inv(arr[ix - 1])
;   [167:16] = expression
;   [167:16] ~inv(arr[ix - 1])
;   [167:20] allocate scratch register -> r14
;   [167:24] set array index
;   [167:24] ix
    mov r14, qword [rsp - 253]
;   [167:29] r14 - 1
    sub r14, 1
;   [167:24] bounds check
;   [167:24] allocate scratch register -> r13
;   [167:24] line number
    mov r13, 167
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [167:24] free scratch register r13
;   [82:6] inv(i : i32) : i32 res
    inv_167_16:
;       [167:16] alias res -> arr (lea: rsp + r15 * 4 - 237)
;       [167:16] alias i -> arr (lea: rsp + r14 * 4 - 237)
;       [83:5] res = ~i
;       [83:11] instructions without scratch register 3, with 3
;       [83:12] ~i
;       [83:12] allocate scratch register -> r13
        mov r13d, dword [rsp + r14 * 4 - 237]
        mov dword [rsp + r15 * 4 - 237], r13d
;       [83:12] free scratch register r13
        not dword [rsp + r15 * 4 - 237]
;       [167:16] free scratch register r14
    inv_167_16_end:
    not dword [rsp + r15 * 4 - 237]
;   [167:5] free scratch register r15
;   [168:5] assert(arr[ix] == 2)
;   [168:12] allocate scratch register -> r15
;   [168:12] ? arr[ix] == 2
;   [168:12] ? arr[ix] == 2
    cmp_168_12:
;   [168:12] allocate scratch register -> r14
;   [168:16] set array index
;   [168:16] ix
    mov r14, qword [rsp - 253]
;   [168:16] bounds check
;   [168:16] allocate scratch register -> r13
;   [168:16] line number
    mov r13, 168
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [168:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 237], 2
;   [168:12] free scratch register r14
    sete r15b
    bool_end_168_12:
;   [41:6] assert(x : bool)
    assert_168_5:
;       [168:5] alias x -> r15b
        if_41_26_168_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_168_5:
        cmp r15b, 0
        jne if_41_23_168_5_end
        if_41_26_168_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_168_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_168_5_end:
        if_41_23_168_5_end:
;       [168:5] free scratch register r15
    assert_168_5_end:
;   [170:5] faz(arr)
;   [92:6] faz(arg : i32[])
    faz_170_5:
;       [170:5] alias arg -> arr
;       [93:5] arg[1] = 0xfe
;       [93:5] allocate scratch register -> r15
;       [93:9] set array index
;       [93:9] 1
        mov r15, 1
;       [93:9] bounds check
;       [93:9] allocate scratch register -> r14
;       [93:9] line number
        mov r14, 93
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
;       [93:9] free scratch register r14
;       [93:14] instructions without scratch register 1, with 2
;       [93:14] 0xfe
        mov dword [rsp + r15 * 4 - 237], 254
;       [93:5] free scratch register r15
    faz_170_5_end:
;   [171:5] assert(arr[1] == 0xfe)
;   [171:12] allocate scratch register -> r15
;   [171:12] ? arr[1] == 0xfe
;   [171:12] ? arr[1] == 0xfe
    cmp_171_12:
;   [171:12] allocate scratch register -> r14
;   [171:16] set array index
;   [171:16] 1
    mov r14, 1
;   [171:16] bounds check
;   [171:16] allocate scratch register -> r13
;   [171:16] line number
    mov r13, 171
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [171:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 237], 254
;   [171:12] free scratch register r14
    sete r15b
    bool_end_171_12:
;   [41:6] assert(x : bool)
    assert_171_5:
;       [171:5] alias x -> r15b
        if_41_26_171_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_171_5:
        cmp r15b, 0
        jne if_41_23_171_5_end
        if_41_26_171_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_171_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_171_5_end:
        if_41_23_171_5_end:
;       [171:5] free scratch register r15
    assert_171_5_end:
;   [173:5] var arr3 : i64[] = { 3, 5 }
;   [173:9] arr3: i64[2] (16 B @ [rsp - 301])
;   [173:9] arr3 = { 3, 5 }
;   [173:26] [0]
;   [173:26] instructions without scratch register 1, with 2
;   [173:26] 3
    mov qword [rsp - 301], 3
;   [173:26] [1]
;   [173:29] instructions without scratch register 1, with 2
;   [173:29] 5
    mov qword [rsp - 293], 5
;   [174:5] foo arr3
;   [174:5] allocate scratch register -> r15
;   [174:9] e: i64 (r15);   [174:9] i: i64 (8 B @ [rsp - 317])
    lea r15, [rsp - 301]
;   [174:9] const n = 2
;   [174:14] initiate counter i to 0
    mov qword [rsp - 317], 0
    foo_174_5:
;       [175:9] e = e + i + n
;       [175:13] instructions without scratch register 3, with 4
;       [175:13] e
;       [175:17] e + i
;       [175:17] allocate scratch register -> r14
        mov r14, qword [rsp - 317]
        add qword [r15], r14
;       [175:17] free scratch register r14
;       [175:21] e + n
        add qword [r15], 2
        foo_174_5_continue:
            add r15, 8
            inc qword [rsp - 317]
            cmp qword [rsp - 317], 2
            jne foo_174_5
    foo_174_5_end:
;   [174:5] free scratch register r15
;   [177:5] assert(arr3[0] == 3 + 0 + 2)
;   [177:12] allocate scratch register -> r15
;   [177:12] ? arr3[0] == 3 + 0 + 2
;   [177:12] ? arr3[0] == 3 + 0 + 2
    cmp_177_12:
;   [177:12] allocate scratch register -> r14
;   [177:17] set array index
;   [177:17] 0
    mov r14, 0
;   [177:17] bounds check
;   [177:17] allocate scratch register -> r13
;   [177:17] line number
    mov r13, 177
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [177:17] free scratch register r13
;   [177:23] allocate scratch register -> r13
;       [177:23] 3
        mov r13, 3
;       [177:27] r13 + 0
        add r13, 0
;       [177:31] r13 + 2
        add r13, 2
    cmp qword [rsp + r14 * 8 - 301], r13
;   [177:12] free scratch register r13
;   [177:12] free scratch register r14
    sete r15b
    bool_end_177_12:
;   [41:6] assert(x : bool)
    assert_177_5:
;       [177:5] alias x -> r15b
        if_41_26_177_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_177_5:
        cmp r15b, 0
        jne if_41_23_177_5_end
        if_41_26_177_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_177_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_177_5_end:
        if_41_23_177_5_end:
;       [177:5] free scratch register r15
    assert_177_5_end:
;   [178:5] assert(arr3[1] == 5 + 1 + 2)
;   [178:12] allocate scratch register -> r15
;   [178:12] ? arr3[1] == 5 + 1 + 2
;   [178:12] ? arr3[1] == 5 + 1 + 2
    cmp_178_12:
;   [178:12] allocate scratch register -> r14
;   [178:17] set array index
;   [178:17] 1
    mov r14, 1
;   [178:17] bounds check
;   [178:17] allocate scratch register -> r13
;   [178:17] line number
    mov r13, 178
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [178:17] free scratch register r13
;   [178:23] allocate scratch register -> r13
;       [178:23] 5
        mov r13, 5
;       [178:27] r13 + 1
        add r13, 1
;       [178:31] r13 + 2
        add r13, 2
    cmp qword [rsp + r14 * 8 - 301], r13
;   [178:12] free scratch register r13
;   [178:12] free scratch register r14
    sete r15b
    bool_end_178_12:
;   [41:6] assert(x : bool)
    assert_178_5:
;       [178:5] alias x -> r15b
        if_41_26_178_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_178_5:
        cmp r15b, 0
        jne if_41_23_178_5_end
        if_41_26_178_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_178_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_178_5_end:
        if_41_23_178_5_end:
;       [178:5] free scratch register r15
    assert_178_5_end:
;   [179:5] # `foo` is a language construct that iterates over an array injecting:
;   [180:5] # `e`: current element
;   [181:5] # `i`: index starting at 0
;   [182:5] # `n`: constant array size
;   [184:5] var p : point = {0, 0}
;   [184:9] p: point (16 B @ [rsp - 317])
;   [184:9] p = {0, 0}
;   [184:22] copy field 'x'
    mov qword [rsp - 317], 0
;   [184:25] copy field 'y'
    mov qword [rsp - 309], 0
;   [185:5] fooz(p)
;   [66:6] fooz(pt : point)
    fooz_185_5:
;       [185:5] alias pt -> p
;       [67:5] pt.x = 0b10
;       [67:12] instructions without scratch register 1, with 2
;       [67:12] 0b10
        mov qword [rsp - 317], 2
;       [67:20] # binary value 2
;       [68:5] pt.y = 0xb
;       [68:12] instructions without scratch register 1, with 2
;       [68:12] 0xb
        mov qword [rsp - 309], 11
;       [68:20] # hex value 11
    fooz_185_5_end:
;   [186:5] assert(p.x == 2)
;   [186:12] allocate scratch register -> r15
;   [186:12] ? p.x == 2
;   [186:12] ? p.x == 2
    cmp_186_12:
    cmp qword [rsp - 317], 2
    sete r15b
    bool_end_186_12:
;   [41:6] assert(x : bool)
    assert_186_5:
;       [186:5] alias x -> r15b
        if_41_26_186_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_186_5:
        cmp r15b, 0
        jne if_41_23_186_5_end
        if_41_26_186_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_186_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_186_5_end:
        if_41_23_186_5_end:
;       [186:5] free scratch register r15
    assert_186_5_end:
;   [187:5] assert(p.y == 0xb)
;   [187:12] allocate scratch register -> r15
;   [187:12] ? p.y == 0xb
;   [187:12] ? p.y == 0xb
    cmp_187_12:
    cmp qword [rsp - 309], 11
    sete r15b
    bool_end_187_12:
;   [41:6] assert(x : bool)
    assert_187_5:
;       [187:5] alias x -> r15b
        if_41_26_187_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_187_5:
        cmp r15b, 0
        jne if_41_23_187_5_end
        if_41_26_187_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_187_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_187_5_end:
        if_41_23_187_5_end:
;       [187:5] free scratch register r15
    assert_187_5_end:
;   [189:5] var q : point = p
;   [189:9] q: point (16 B @ [rsp - 333])
;   [189:9] q = p
;   [189:21] size <= 16 B, use mov
;   [189:21] allocate named register rax
    mov rax, qword [rsp - 317]
    mov qword [rsp - 333], rax
    mov rax, qword [rsp - 309]
    mov qword [rsp - 325], rax
;   [189:21] free named register rax
;   [190:5] assert(equal(p, q))
;   [190:12] allocate scratch register -> r15
;   [190:12] ? equal(p, q)
;   [190:12] ? equal(p, q)
    cmp_190_12:
;   [190:12] allocate scratch register -> r14
;       [190:12] r14 = equal(p, q)
;       [190:12] = expression
;       [190:12] equal(p, q)
;       [190:12] allocate named register rsi
;       [190:12] allocate named register rdi
;       [190:12] allocate named register rcx
;       [190:18] p
        lea rsi, [rsp - 317]
;       [190:21] q
        lea rdi, [rsp - 333]
        mov rcx, 2
        repe cmpsq
;       [190:12] free named register rcx
;       [190:12] free named register rdi
;       [190:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [190:12] free scratch register r14
    setne r15b
    bool_end_190_12:
;   [41:6] assert(x : bool)
    assert_190_5:
;       [190:5] alias x -> r15b
        if_41_26_190_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_190_5:
        cmp r15b, 0
        jne if_41_23_190_5_end
        if_41_26_190_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_190_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_190_5_end:
        if_41_23_190_5_end:
;       [190:5] free scratch register r15
    assert_190_5_end:
;   [191:5] # `equal` is built-in function to compare user types for equality or same
;   [192:5] # size arrays
;   [194:5] q.x = 3
;   [194:11] instructions without scratch register 1, with 2
;   [194:11] 3
    mov qword [rsp - 333], 3
;   [195:5] assert(not equal(p, q))
;   [195:12] allocate scratch register -> r15
;   [195:12] ? not equal(p, q)
;   [195:12] ? not equal(p, q)
    cmp_195_12:
;   [195:16] allocate scratch register -> r14
;       [195:16] r14 = equal(p, q)
;       [195:16] = expression
;       [195:16] equal(p, q)
;       [195:16] allocate named register rsi
;       [195:16] allocate named register rdi
;       [195:16] allocate named register rcx
;       [195:22] p
        lea rsi, [rsp - 317]
;       [195:25] q
        lea rdi, [rsp - 333]
        mov rcx, 2
        repe cmpsq
;       [195:16] free named register rcx
;       [195:16] free named register rdi
;       [195:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [195:12] free scratch register r14
    sete r15b
    bool_end_195_12:
;   [41:6] assert(x : bool)
    assert_195_5:
;       [195:5] alias x -> r15b
        if_41_26_195_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_195_5:
        cmp r15b, 0
        jne if_41_23_195_5_end
        if_41_26_195_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_195_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_195_5_end:
        if_41_23_195_5_end:
;       [195:5] free scratch register r15
    assert_195_5_end:
;   [197:5] var i = 0
;   [197:9] i: i64 (8 B @ [rsp - 341])
;   [197:9] i = 0
;   [197:13] instructions without scratch register 1, with 2
;   [197:13] 0
    mov qword [rsp - 341], 0
;   [198:5] bar(i)
;   [73:6] bar(arg)
    bar_198_5:
;       [198:5] alias arg -> i
        if_74_8_198_5:
;       [74:8] ? arg == 0
;       [74:8] ? arg == 0
        cmp_74_8_198_5:
        cmp qword [rsp - 341], 0
        jne if_74_5_198_5_end
        if_74_8_198_5_code:
;           [74:17] return
            jmp bar_198_5_end
        if_74_5_198_5_end:
;       [75:5] arg = 0xff
;       [75:11] instructions without scratch register 1, with 2
;       [75:11] 0xff
        mov qword [rsp - 341], 255
    bar_198_5_end:
;   [199:5] assert(i == 0)
;   [199:12] allocate scratch register -> r15
;   [199:12] ? i == 0
;   [199:12] ? i == 0
    cmp_199_12:
    cmp qword [rsp - 341], 0
    sete r15b
    bool_end_199_12:
;   [41:6] assert(x : bool)
    assert_199_5:
;       [199:5] alias x -> r15b
        if_41_26_199_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_199_5:
        cmp r15b, 0
        jne if_41_23_199_5_end
        if_41_26_199_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_199_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_199_5_end:
        if_41_23_199_5_end:
;       [199:5] free scratch register r15
    assert_199_5_end:
;   [201:5] i = 1
;   [201:9] instructions without scratch register 1, with 2
;   [201:9] 1
    mov qword [rsp - 341], 1
;   [202:5] bar(i)
;   [73:6] bar(arg)
    bar_202_5:
;       [202:5] alias arg -> i
        if_74_8_202_5:
;       [74:8] ? arg == 0
;       [74:8] ? arg == 0
        cmp_74_8_202_5:
        cmp qword [rsp - 341], 0
        jne if_74_5_202_5_end
        if_74_8_202_5_code:
;           [74:17] return
            jmp bar_202_5_end
        if_74_5_202_5_end:
;       [75:5] arg = 0xff
;       [75:11] instructions without scratch register 1, with 2
;       [75:11] 0xff
        mov qword [rsp - 341], 255
    bar_202_5_end:
;   [203:5] assert(i == 0xff)
;   [203:12] allocate scratch register -> r15
;   [203:12] ? i == 0xff
;   [203:12] ? i == 0xff
    cmp_203_12:
    cmp qword [rsp - 341], 255
    sete r15b
    bool_end_203_12:
;   [41:6] assert(x : bool)
    assert_203_5:
;       [203:5] alias x -> r15b
        if_41_26_203_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_203_5:
        cmp r15b, 0
        jne if_41_23_203_5_end
        if_41_26_203_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_203_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_203_5_end:
        if_41_23_203_5_end:
;       [203:5] free scratch register r15
    assert_203_5_end:
;   [205:5] var j = 1
;   [205:9] j: i64 (8 B @ [rsp - 349])
;   [205:9] j = 1
;   [205:13] instructions without scratch register 1, with 2
;   [205:13] 1
    mov qword [rsp - 349], 1
;   [206:5] var k = baz(j)
;   [206:9] k: i64 (8 B @ [rsp - 357])
;   [206:9] k = baz(j)
;   [206:13] instructions without scratch register 5, with 5
;   [206:13] k = baz(j)
;   [206:13] = expression
;   [206:13] baz(j)
;   [86:6] baz(arg) : i64 res
    baz_206_13:
;       [206:13] alias res -> k
;       [206:13] alias arg -> j
;       [87:5] res = arg * 2
;       [87:11] instructions without scratch register 5, with 3
;       [87:11] allocate scratch register -> r15
;       [87:11] arg
        mov r15, qword [rsp - 349]
;       [87:17] r15 * 2
;       [87:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 357], r15
;       [87:11] free scratch register r15
    baz_206_13_end:
;   [207:5] assert(k == 2)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? k == 2
;   [207:12] ? k == 2
    cmp_207_12:
    cmp qword [rsp - 357], 2
    sete r15b
    bool_end_207_12:
;   [41:6] assert(x : bool)
    assert_207_5:
;       [207:5] alias x -> r15b
        if_41_26_207_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_207_5:
        cmp r15b, 0
        jne if_41_23_207_5_end
        if_41_26_207_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_207_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_207_5_end:
        if_41_23_207_5_end:
;       [207:5] free scratch register r15
    assert_207_5_end:
;   [209:5] k = baz(1)
;   [209:9] instructions without scratch register 5, with 5
;   [209:9] k = baz(1)
;   [209:9] = expression
;   [209:9] baz(1)
;   [86:6] baz(arg) : i64 res
    baz_209_9:
;       [209:9] alias res -> k
;       [209:9] alias arg -> 1
;       [87:5] res = arg * 2
;       [87:11] instructions without scratch register 4, with 3
;       [87:11] allocate scratch register -> r15
;       [87:11] arg
        mov r15, 1
;       [87:17] r15 * 2
;       [87:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 357], r15
;       [87:11] free scratch register r15
    baz_209_9_end:
;   [210:5] assert(k == 2)
;   [210:12] allocate scratch register -> r15
;   [210:12] ? k == 2
;   [210:12] ? k == 2
    cmp_210_12:
    cmp qword [rsp - 357], 2
    sete r15b
    bool_end_210_12:
;   [41:6] assert(x : bool)
    assert_210_5:
;       [210:5] alias x -> r15b
        if_41_26_210_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_210_5:
        cmp r15b, 0
        jne if_41_23_210_5_end
        if_41_26_210_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_210_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_210_5_end:
        if_41_23_210_5_end:
;       [210:5] free scratch register r15
    assert_210_5_end:
;   [212:5] var p0 : point = {baz(3), 0}
;   [212:9] p0: point (16 B @ [rsp - 373])
;   [212:9] p0 = {baz(3), 0}
;   [212:23] copy field 'x'
;   [212:23] instructions without scratch register 5, with 5
;   [212:23] p0.x = baz(3)
;   [212:23] = expression
;   [212:23] baz(3)
;   [86:6] baz(arg) : i64 res
    baz_212_23:
;       [212:23] alias res -> p0.x (lea: rsp - 373)
;       [212:23] alias arg -> 3
;       [87:5] res = arg * 2
;       [87:11] instructions without scratch register 4, with 3
;       [87:11] allocate scratch register -> r15
;       [87:11] arg
        mov r15, 3
;       [87:17] r15 * 2
;       [87:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 373], r15
;       [87:11] free scratch register r15
    baz_212_23_end:
;   [212:31] copy field 'y'
    mov qword [rsp - 365], 0
;   [213:5] assert(p0.x == 6)
;   [213:12] allocate scratch register -> r15
;   [213:12] ? p0.x == 6
;   [213:12] ? p0.x == 6
    cmp_213_12:
    cmp qword [rsp - 373], 6
    sete r15b
    bool_end_213_12:
;   [41:6] assert(x : bool)
    assert_213_5:
;       [213:5] alias x -> r15b
        if_41_26_213_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_213_5:
        cmp r15b, 0
        jne if_41_23_213_5_end
        if_41_26_213_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_213_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_213_5_end:
        if_41_23_213_5_end:
;       [213:5] free scratch register r15
    assert_213_5_end:
;   [215:5] var pt : point = point_init()
;   [215:9] pt: point (16 B @ [rsp - 389])
;   [215:9] pt = point_init()
;   [215:22] point_init()
;   [113:6] point_init() : point res
    point_init_215_22:
;       [215:22] alias res -> pt
;       [114:5] res.x = -1
;       [114:13] instructions without scratch register 1, with 2
;       [114:14] -1
        mov qword [rsp - 389], -1
;       [115:5] res.y = -2
;       [115:13] instructions without scratch register 1, with 2
;       [115:14] -2
        mov qword [rsp - 381], -2
    point_init_215_22_end:
;   [216:5] assert(pt.x == -1)
;   [216:12] allocate scratch register -> r15
;   [216:12] ? pt.x == -1
;   [216:12] ? pt.x == -1
    cmp_216_12:
    cmp qword [rsp - 389], -1
    sete r15b
    bool_end_216_12:
;   [41:6] assert(x : bool)
    assert_216_5:
;       [216:5] alias x -> r15b
        if_41_26_216_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_216_5:
        cmp r15b, 0
        jne if_41_23_216_5_end
        if_41_26_216_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_216_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_216_5_end:
        if_41_23_216_5_end:
;       [216:5] free scratch register r15
    assert_216_5_end:
;   [217:5] assert(pt.y == -2)
;   [217:12] allocate scratch register -> r15
;   [217:12] ? pt.y == -2
;   [217:12] ? pt.y == -2
    cmp_217_12:
    cmp qword [rsp - 381], -2
    sete r15b
    bool_end_217_12:
;   [41:6] assert(x : bool)
    assert_217_5:
;       [217:5] alias x -> r15b
        if_41_26_217_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_217_5:
        cmp r15b, 0
        jne if_41_23_217_5_end
        if_41_26_217_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_217_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_217_5_end:
        if_41_23_217_5_end:
;       [217:5] free scratch register r15
    assert_217_5_end:
;   [219:5] var x = 1
;   [219:9] x: i64 (8 B @ [rsp - 397])
;   [219:9] x = 1
;   [219:13] instructions without scratch register 1, with 2
;   [219:13] 1
    mov qword [rsp - 397], 1
;   [220:5] var y = 2
;   [220:9] y: i64 (8 B @ [rsp - 405])
;   [220:9] y = 2
;   [220:13] instructions without scratch register 1, with 2
;   [220:13] 2
    mov qword [rsp - 405], 2
;   [222:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [222:9] o1: object (20 B @ [rsp - 425])
;   [222:9] o1 = {{x * 10, y}, 0xff0000}
;   [222:24] copy field 'pos'
;   [222:25] copy field 'x'
;   [222:25] instructions without scratch register 5, with 3
;   [222:25] allocate scratch register -> r15
;   [222:25] x
    mov r15, qword [rsp - 397]
;   [222:29] r15 * 10
;   [222:29] dst is reg, src is const
    imul r15, 10
    mov qword [rsp - 425], r15
;   [222:25] free scratch register r15
;   [222:33] copy field 'y'
;   [222:33] allocate scratch register -> r15
    mov r15, qword [rsp - 405]
    mov qword [rsp - 417], r15
;   [222:33] free scratch register r15
;   [222:37] copy field 'color'
    mov dword [rsp - 409], 16711680
;   [223:5] assert(o1.pos.x == 10)
;   [223:12] allocate scratch register -> r15
;   [223:12] ? o1.pos.x == 10
;   [223:12] ? o1.pos.x == 10
    cmp_223_12:
    cmp qword [rsp - 425], 10
    sete r15b
    bool_end_223_12:
;   [41:6] assert(x : bool)
    assert_223_5:
;       [223:5] alias x -> r15b
        if_41_26_223_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_223_5:
        cmp r15b, 0
        jne if_41_23_223_5_end
        if_41_26_223_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_223_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_223_5_end:
        if_41_23_223_5_end:
;       [223:5] free scratch register r15
    assert_223_5_end:
;   [224:5] assert(o1.pos.y == 2)
;   [224:12] allocate scratch register -> r15
;   [224:12] ? o1.pos.y == 2
;   [224:12] ? o1.pos.y == 2
    cmp_224_12:
    cmp qword [rsp - 417], 2
    sete r15b
    bool_end_224_12:
;   [41:6] assert(x : bool)
    assert_224_5:
;       [224:5] alias x -> r15b
        if_41_26_224_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_224_5:
        cmp r15b, 0
        jne if_41_23_224_5_end
        if_41_26_224_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_224_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_224_5_end:
        if_41_23_224_5_end:
;       [224:5] free scratch register r15
    assert_224_5_end:
;   [225:5] assert(o1.color == 0xff0000)
;   [225:12] allocate scratch register -> r15
;   [225:12] ? o1.color == 0xff0000
;   [225:12] ? o1.color == 0xff0000
    cmp_225_12:
    cmp dword [rsp - 409], 16711680
    sete r15b
    bool_end_225_12:
;   [41:6] assert(x : bool)
    assert_225_5:
;       [225:5] alias x -> r15b
        if_41_26_225_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_225_5:
        cmp r15b, 0
        jne if_41_23_225_5_end
        if_41_26_225_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_225_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_225_5_end:
        if_41_23_225_5_end:
;       [225:5] free scratch register r15
    assert_225_5_end:
;   [227:5] var p1 : point = {-x, -y}
;   [227:9] p1: point (16 B @ [rsp - 441])
;   [227:9] p1 = {-x, -y}
;   [227:23] copy field 'x'
;   [227:23] allocate scratch register -> r15
    mov r15, qword [rsp - 397]
    mov qword [rsp - 441], r15
;   [227:23] free scratch register r15
    neg qword [rsp - 441]
;   [227:27] copy field 'y'
;   [227:27] allocate scratch register -> r15
    mov r15, qword [rsp - 405]
    mov qword [rsp - 433], r15
;   [227:27] free scratch register r15
    neg qword [rsp - 433]
;   [228:5] o1.pos = p1
;   [228:14] size <= 16 B, use mov
;   [228:14] allocate named register rax
    mov rax, qword [rsp - 441]
    mov qword [rsp - 425], rax
    mov rax, qword [rsp - 433]
    mov qword [rsp - 417], rax
;   [228:14] free named register rax
;   [229:5] assert(o1.pos.x == -1)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? o1.pos.x == -1
;   [229:12] ? o1.pos.x == -1
    cmp_229_12:
    cmp qword [rsp - 425], -1
    sete r15b
    bool_end_229_12:
;   [41:6] assert(x : bool)
    assert_229_5:
;       [229:5] alias x -> r15b
        if_41_26_229_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_229_5:
        cmp r15b, 0
        jne if_41_23_229_5_end
        if_41_26_229_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_229_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_229_5_end:
        if_41_23_229_5_end:
;       [229:5] free scratch register r15
    assert_229_5_end:
;   [230:5] assert(o1.pos.y == -2)
;   [230:12] allocate scratch register -> r15
;   [230:12] ? o1.pos.y == -2
;   [230:12] ? o1.pos.y == -2
    cmp_230_12:
    cmp qword [rsp - 417], -2
    sete r15b
    bool_end_230_12:
;   [41:6] assert(x : bool)
    assert_230_5:
;       [230:5] alias x -> r15b
        if_41_26_230_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_230_5:
        cmp r15b, 0
        jne if_41_23_230_5_end
        if_41_26_230_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_230_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_230_5_end:
        if_41_23_230_5_end:
;       [230:5] free scratch register r15
    assert_230_5_end:
;   [232:5] var o2 : object = o1
;   [232:9] o2: object (20 B @ [rsp - 461])
;   [232:9] o2 = o1
;   [232:23] allocate named register rsi
;   [232:23] allocate named register rdi
;   [232:23] allocate named register rcx
    lea rsi, [rsp - 425]
    lea rdi, [rsp - 461]
    mov rcx, 20
    rep movsb
;   [232:23] free named register rcx
;   [232:23] free named register rdi
;   [232:23] free named register rsi
;   [233:5] assert(o2.pos.x == -1)
;   [233:12] allocate scratch register -> r15
;   [233:12] ? o2.pos.x == -1
;   [233:12] ? o2.pos.x == -1
    cmp_233_12:
    cmp qword [rsp - 461], -1
    sete r15b
    bool_end_233_12:
;   [41:6] assert(x : bool)
    assert_233_5:
;       [233:5] alias x -> r15b
        if_41_26_233_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_233_5:
        cmp r15b, 0
        jne if_41_23_233_5_end
        if_41_26_233_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_233_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_233_5_end:
        if_41_23_233_5_end:
;       [233:5] free scratch register r15
    assert_233_5_end:
;   [234:5] assert(o2.pos.y == -2)
;   [234:12] allocate scratch register -> r15
;   [234:12] ? o2.pos.y == -2
;   [234:12] ? o2.pos.y == -2
    cmp_234_12:
    cmp qword [rsp - 453], -2
    sete r15b
    bool_end_234_12:
;   [41:6] assert(x : bool)
    assert_234_5:
;       [234:5] alias x -> r15b
        if_41_26_234_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_234_5:
        cmp r15b, 0
        jne if_41_23_234_5_end
        if_41_26_234_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_234_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_234_5_end:
        if_41_23_234_5_end:
;       [234:5] free scratch register r15
    assert_234_5_end:
;   [235:5] assert(o2.color == 0xff0000)
;   [235:12] allocate scratch register -> r15
;   [235:12] ? o2.color == 0xff0000
;   [235:12] ? o2.color == 0xff0000
    cmp_235_12:
    cmp dword [rsp - 445], 16711680
    sete r15b
    bool_end_235_12:
;   [41:6] assert(x : bool)
    assert_235_5:
;       [235:5] alias x -> r15b
        if_41_26_235_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_235_5:
        cmp r15b, 0
        jne if_41_23_235_5_end
        if_41_26_235_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_235_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_235_5_end:
        if_41_23_235_5_end:
;       [235:5] free scratch register r15
    assert_235_5_end:
;   [237:5] var o3 : object[2]
;   [237:9] o3: object[2] (40 B @ [rsp - 501])
;   [237:9] zero 2 * 20 B = 40 B
;   [237:5] allocate named register rax
;   [237:5] allocate named register rdi
;   [237:5] allocate named register rcx
    xor al, al
    lea rdi, [rsp - 501]
    mov rcx, 40
    rep stosb
;   [237:5] free named register rcx
;   [237:5] free named register rdi
;   [237:5] free named register rax
;   [238:5] o3.pos.y = 73
;   [238:16] instructions without scratch register 1, with 2
;   [238:16] 73
    mov qword [rsp - 493], 73
;   [239:5] # index 0 in an array can be accessed without array index
;   [241:5] assert(o3[0].pos.y == 73)
;   [241:12] allocate scratch register -> r15
;   [241:12] ? o3[0].pos.y == 73
;   [241:12] ? o3[0].pos.y == 73
    cmp_241_12:
;   [241:12] allocate scratch register -> r14
    lea r14, [rsp - 501]
;   [241:12] allocate scratch register -> r13
;   [241:15] set array index
;   [241:15] 0
    mov r13, 0
;   [241:15] bounds check
;   [241:15] allocate scratch register -> r12
;   [241:15] line number
    mov r12, 241
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
;   [241:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [241:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [241:12] free scratch register r14
    sete r15b
    bool_end_241_12:
;   [41:6] assert(x : bool)
    assert_241_5:
;       [241:5] alias x -> r15b
        if_41_26_241_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_241_5:
        cmp r15b, 0
        jne if_41_23_241_5_end
        if_41_26_241_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_241_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_241_5_end:
        if_41_23_241_5_end:
;       [241:5] free scratch register r15
    assert_241_5_end:
;   [243:5] o3[1] = object_init()
;   [243:5] allocate scratch register -> r15
    lea r15, [rsp - 501]
;   [243:5] allocate scratch register -> r14
;   [243:8] set array index
;   [243:8] 1
    mov r14, 1
;   [243:8] bounds check
;   [243:8] allocate scratch register -> r13
;   [243:8] line number
    mov r13, 243
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [243:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [243:5] free scratch register r14
;   [243:13] object_init()
;   [118:6] object_init() : object res
    object_init_243_13:
;       [243:13] alias res -> o3 (lea: r15)
;       [119:5] res.pos.y = 74
;       [119:17] instructions without scratch register 1, with 2
;       [119:17] 74
        mov qword [r15 + 8], 74
    object_init_243_13_end:
;   [243:5] free scratch register r15
;   [244:5] assert(o3[1].pos.y == 74)
;   [244:12] allocate scratch register -> r15
;   [244:12] ? o3[1].pos.y == 74
;   [244:12] ? o3[1].pos.y == 74
    cmp_244_12:
;   [244:12] allocate scratch register -> r14
    lea r14, [rsp - 501]
;   [244:12] allocate scratch register -> r13
;   [244:15] set array index
;   [244:15] 1
    mov r13, 1
;   [244:15] bounds check
;   [244:15] allocate scratch register -> r12
;   [244:15] line number
    mov r12, 244
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
;   [244:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [244:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [244:12] free scratch register r14
    sete r15b
    bool_end_244_12:
;   [41:6] assert(x : bool)
    assert_244_5:
;       [244:5] alias x -> r15b
        if_41_26_244_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_244_5:
        cmp r15b, 0
        jne if_41_23_244_5_end
        if_41_26_244_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_244_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_244_5_end:
        if_41_23_244_5_end:
;       [244:5] free scratch register r15
    assert_244_5_end:
;   [246:5] var worlds : world[8]
;   [246:9] worlds: world[8] (512 B @ [rsp - 1013])
;   [246:9] zero 8 * 64 B = 512 B
;   [246:5] allocate named register rax
;   [246:5] allocate named register rdi
;   [246:5] allocate named register rcx
    xor al, al
    lea rdi, [rsp - 1013]
    mov rcx, 512
    rep stosb
;   [246:5] free named register rcx
;   [246:5] free named register rdi
;   [246:5] free named register rax
;   [247:5] worlds[1].locations[1] = 0xffee
;   [247:5] allocate scratch register -> r15
    lea r15, [rsp - 1013]
;   [247:5] allocate scratch register -> r14
;   [247:12] set array index
;   [247:12] 1
    mov r14, 1
;   [247:12] bounds check
;   [247:12] allocate scratch register -> r13
;   [247:12] line number
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [247:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [247:5] free scratch register r14
;   [247:5] allocate scratch register -> r14
;   [247:25] set array index
;   [247:25] 1
    mov r14, 1
;   [247:25] bounds check
;   [247:25] allocate scratch register -> r13
;   [247:25] line number
    mov r13, 247
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [247:25] free scratch register r13
;   [247:30] instructions without scratch register 1, with 2
;   [247:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [247:5] free scratch register r14
;   [247:5] free scratch register r15
;   [248:5] assert(worlds[1].locations[1] == 0xffee)
;   [248:12] allocate scratch register -> r15
;   [248:12] ? worlds[1].locations[1] == 0xffee
;   [248:12] ? worlds[1].locations[1] == 0xffee
    cmp_248_12:
;   [248:12] allocate scratch register -> r14
    lea r14, [rsp - 1013]
;   [248:12] allocate scratch register -> r13
;   [248:19] set array index
;   [248:19] 1
    mov r13, 1
;   [248:19] bounds check
;   [248:19] allocate scratch register -> r12
;   [248:19] line number
    mov r12, 248
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [248:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [248:12] free scratch register r13
;   [248:12] allocate scratch register -> r13
;   [248:32] set array index
;   [248:32] 1
    mov r13, 1
;   [248:32] bounds check
;   [248:32] allocate scratch register -> r12
;   [248:32] line number
    mov r12, 248
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [248:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [248:12] free scratch register r13
;   [248:12] free scratch register r14
    sete r15b
    bool_end_248_12:
;   [41:6] assert(x : bool)
    assert_248_5:
;       [248:5] alias x -> r15b
        if_41_26_248_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_248_5:
        cmp r15b, 0
        jne if_41_23_248_5_end
        if_41_26_248_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_248_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_248_5_end:
        if_41_23_248_5_end:
;       [248:5] free scratch register r15
    assert_248_5_end:
;   [250:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [250:5] allocate named register rsi
;   [250:5] allocate named register rdi
;   [250:5] allocate named register rcx
;   [253:9] array_size_of(worlds.locations)
;   [253:9] rcx = array_size_of(worlds.locations)
;   [253:9] = expression
;   [253:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [251:9] worlds[1].locations
;   [251:9] allocate scratch register -> r15
    lea r15, [rsp - 1013]
;   [251:9] allocate scratch register -> r14
;   [251:16] set array index
;   [251:16] 1
    mov r14, 1
;   [251:16] bounds check
;   [251:16] allocate scratch register -> r13
;   [251:16] line number
    mov r13, 251
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [251:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [251:9] free scratch register r14
;   [251:9] bounds check
;   [251:9] allocate scratch register -> r14
;   [251:9] line number
    mov r14, 251
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [251:9] free scratch register r14
    lea rsi, [r15]
;   [250:5] free scratch register r15
;   [252:9] worlds[0].locations
;   [252:9] allocate scratch register -> r15
    lea r15, [rsp - 1013]
;   [252:9] allocate scratch register -> r14
;   [252:16] set array index
;   [252:16] 0
    mov r14, 0
;   [252:16] bounds check
;   [252:16] allocate scratch register -> r13
;   [252:16] line number
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [252:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [252:9] free scratch register r14
;   [252:9] bounds check
;   [252:9] allocate scratch register -> r14
;   [252:9] line number
    mov r14, 252
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [252:9] free scratch register r14
    lea rdi, [r15]
;   [250:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [250:5] free named register rcx
;   [250:5] free named register rdi
;   [250:5] free named register rsi
;   [255:5] # `array_copy` is built-in and can use indexed positions
;   [256:5] # `array_size_of` is built-in
;   [258:5] assert(worlds[0].locations[1] == 0xffee)
;   [258:12] allocate scratch register -> r15
;   [258:12] ? worlds[0].locations[1] == 0xffee
;   [258:12] ? worlds[0].locations[1] == 0xffee
    cmp_258_12:
;   [258:12] allocate scratch register -> r14
    lea r14, [rsp - 1013]
;   [258:12] allocate scratch register -> r13
;   [258:19] set array index
;   [258:19] 0
    mov r13, 0
;   [258:19] bounds check
;   [258:19] allocate scratch register -> r12
;   [258:19] line number
    mov r12, 258
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [258:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [258:12] free scratch register r13
;   [258:12] allocate scratch register -> r13
;   [258:32] set array index
;   [258:32] 1
    mov r13, 1
;   [258:32] bounds check
;   [258:32] allocate scratch register -> r12
;   [258:32] line number
    mov r12, 258
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [258:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [258:12] free scratch register r13
;   [258:12] free scratch register r14
    sete r15b
    bool_end_258_12:
;   [41:6] assert(x : bool)
    assert_258_5:
;       [258:5] alias x -> r15b
        if_41_26_258_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_258_5:
        cmp r15b, 0
        jne if_41_23_258_5_end
        if_41_26_258_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_258_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_258_5_end:
        if_41_23_258_5_end:
;       [258:5] free scratch register r15
    assert_258_5_end:
;   [259:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [259:12] allocate scratch register -> r15
;   [259:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [259:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_259_12:
;   [259:12] allocate scratch register -> r14
;       [259:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [259:12] = expression
;       [259:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [259:12] allocate named register rsi
;       [259:12] allocate named register rdi
;       [259:12] allocate named register rcx
;       [262:14] array_size_of(worlds.locations)
;       [262:14] rcx = array_size_of(worlds.locations)
;       [262:14] = expression
;       [262:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [260:14] worlds[0].locations
;       [260:14] allocate scratch register -> r13
        lea r13, [rsp - 1013]
;       [260:14] allocate scratch register -> r12
;       [260:21] set array index
;       [260:21] 0
        mov r12, 0
;       [260:21] bounds check
;       [260:21] allocate scratch register -> r11
;       [260:21] line number
        mov r11, 260
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [260:21] free scratch register r11
        shl r12, 6
        add r13, r12
;       [260:14] free scratch register r12
;       [260:14] bounds check
;       [260:14] allocate scratch register -> r12
;       [260:14] line number
        mov r12, 260
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [260:14] free scratch register r12
        lea rsi, [r13]
;       [259:12] free scratch register r13
;       [261:14] worlds[1].locations
;       [261:14] allocate scratch register -> r13
        lea r13, [rsp - 1013]
;       [261:14] allocate scratch register -> r12
;       [261:21] set array index
;       [261:21] 1
        mov r12, 1
;       [261:21] bounds check
;       [261:21] allocate scratch register -> r11
;       [261:21] line number
        mov r11, 261
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [261:21] free scratch register r11
        shl r12, 6
        add r13, r12
;       [261:14] free scratch register r12
;       [261:14] bounds check
;       [261:14] allocate scratch register -> r12
;       [261:14] line number
        mov r12, 261
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [261:14] free scratch register r12
        lea rdi, [r13]
;       [259:12] free scratch register r13
        shl rcx, 3
        repe cmpsb
;       [259:12] free named register rcx
;       [259:12] free named register rdi
;       [259:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [259:12] free scratch register r14
    setne r15b
    bool_end_259_12:
;   [41:6] assert(x : bool)
    assert_259_5:
;       [259:5] alias x -> r15b
        if_41_26_259_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_259_5:
        cmp r15b, 0
        jne if_41_23_259_5_end
        if_41_26_259_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_259_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_259_5_end:
        if_41_23_259_5_end:
;       [259:5] free scratch register r15
    assert_259_5_end:
;   [265:5] var arr2 : i64[] = { -1, 2 }
;   [265:9] arr2: i64[2] (16 B @ [rsp - 1029])
;   [265:9] arr2 = { -1, 2 }
;   [265:26] [0]
;   [265:26] instructions without scratch register 1, with 2
;   [265:27] -1
    mov qword [rsp - 1029], -1
;   [265:26] [1]
;   [265:30] instructions without scratch register 1, with 2
;   [265:30] 2
    mov qword [rsp - 1021], 2
;   [266:5] assert(array_size_of(arr2) == 2)
;   [266:12] allocate scratch register -> r15
;   [266:12] ? array_size_of(arr2) == 2
;   [266:12] ? array_size_of(arr2) == 2
    cmp_266_12:
;   [266:12] allocate scratch register -> r14
;       [266:12] r14 = array_size_of(arr2)
;       [266:12] = expression
;       [266:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
;   [266:12] free scratch register r14
    sete r15b
    bool_end_266_12:
;   [41:6] assert(x : bool)
    assert_266_5:
;       [266:5] alias x -> r15b
        if_41_26_266_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_266_5:
        cmp r15b, 0
        jne if_41_23_266_5_end
        if_41_26_266_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_266_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_266_5_end:
        if_41_23_266_5_end:
;       [266:5] free scratch register r15
    assert_266_5_end:
;   [267:5] assert(arr2[0] == -1)
;   [267:12] allocate scratch register -> r15
;   [267:12] ? arr2[0] == -1
;   [267:12] ? arr2[0] == -1
    cmp_267_12:
;   [267:12] allocate scratch register -> r14
;   [267:17] set array index
;   [267:17] 0
    mov r14, 0
;   [267:17] bounds check
;   [267:17] allocate scratch register -> r13
;   [267:17] line number
    mov r13, 267
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [267:17] free scratch register r13
    cmp qword [rsp + r14 * 8 - 1029], -1
;   [267:12] free scratch register r14
    sete r15b
    bool_end_267_12:
;   [41:6] assert(x : bool)
    assert_267_5:
;       [267:5] alias x -> r15b
        if_41_26_267_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_267_5:
        cmp r15b, 0
        jne if_41_23_267_5_end
        if_41_26_267_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_267_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_267_5_end:
        if_41_23_267_5_end:
;       [267:5] free scratch register r15
    assert_267_5_end:
;   [268:5] assert(arr2[1] == 2)
;   [268:12] allocate scratch register -> r15
;   [268:12] ? arr2[1] == 2
;   [268:12] ? arr2[1] == 2
    cmp_268_12:
;   [268:12] allocate scratch register -> r14
;   [268:17] set array index
;   [268:17] 1
    mov r14, 1
;   [268:17] bounds check
;   [268:17] allocate scratch register -> r13
;   [268:17] line number
    mov r13, 268
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [268:17] free scratch register r13
    cmp qword [rsp + r14 * 8 - 1029], 2
;   [268:12] free scratch register r14
    sete r15b
    bool_end_268_12:
;   [41:6] assert(x : bool)
    assert_268_5:
;       [268:5] alias x -> r15b
        if_41_26_268_5:
;       [41:26] ? not x
;       [41:26] ? not x
        cmp_41_26_268_5:
        cmp r15b, 0
        jne if_41_23_268_5_end
        if_41_26_268_5_code:
;           [41:32] exit(1)
;           [41:37] allocate named register rdi
            mov rdi, 1
;           [33:6] exit(v : reg_rdi)
            exit_41_32_268_5:
;               [41:32] alias v -> rdi
;               [34:5] mov(rax, 60)
;                   [34:14] 60
                    mov rax, 60
;               [34:19] # exit system call
;               [35:5] mov(rdi, v)
;                   [35:14] v
;               [35:19] # return code
;               [36:5] syscall()
                syscall
;               [41:32] free named register rdi
            exit_41_32_268_5_end:
        if_41_23_268_5_end:
;       [268:5] free scratch register r15
    assert_268_5_end:
;   [270:5] var nm : str
;   [270:9] nm: str (128 B @ [rsp - 1157])
;   [270:9] zero 1 * 128 B = 128 B
;   [270:5] allocate named register rax
;   [270:5] allocate named register rdi
;   [270:5] allocate named register rcx
    xor al, al
    lea rdi, [rsp - 1157]
    mov rcx, 128
    rep stosb
;   [270:5] free named register rcx
;   [270:5] free named register rdi
;   [270:5] free named register rax
;   [271:5] print(hello)
;   [51:6] print(str : i8[])
    print_271_5:
;       [271:5] alias str -> hello
;       [52:4] sys_print(array_size_of(str), address_of(str))
;       [52:14] allocate named register rdx
;       [52:14] rdx = array_size_of(str)
;       [52:14] = expression
;       [52:14] array_size_of(str)
        mov rdx, 21
;       [52:34] allocate named register rsi
;       [52:34] rsi = address_of(str)
;       [52:34] = expression
;       [52:34] address_of(str)
        lea rsi, [rsp - 21]
;       [43:6] sys_print(len : reg_rdx, ptr : reg_rsi)
        sys_print_52_4_271_5:
;           [52:4] alias len -> rdx
;           [52:4] alias ptr -> rsi
;           [44:5] mov(rax, 1)
;               [44:14] 1
                mov rax, 1
;           [44:19] # write system call
;           [45:5] mov(rdi, 0)
;               [45:14] 0
                mov rdi, 0
;           [45:19] # file descriptor for standard out
;           [46:5] mov(rsi, ptr)
;               [46:14] ptr
;           [46:19] # buffer address
;           [47:5] mov(rdx, len)
;               [47:14] len
;           [47:19] # buffer size
;           [48:5] syscall()
            syscall
;           [52:4] free named register rsi
;           [52:4] free named register rdx
        sys_print_52_4_271_5_end:
    print_271_5_end:
;   [272:5] label
    loop_272_5:
;       [273:9] print(prompt1)
;       [51:6] print(str : i8[])
        print_273_9:
;           [273:9] alias str -> prompt1
;           [52:4] sys_print(array_size_of(str), address_of(str))
;           [52:14] allocate named register rdx
;           [52:14] rdx = array_size_of(str)
;           [52:14] = expression
;           [52:14] array_size_of(str)
            mov rdx, 12
;           [52:34] allocate named register rsi
;           [52:34] rsi = address_of(str)
;           [52:34] = expression
;           [52:34] address_of(str)
            lea rsi, [rsp - 33]
;           [43:6] sys_print(len : reg_rdx, ptr : reg_rsi)
            sys_print_52_4_273_9:
;               [52:4] alias len -> rdx
;               [52:4] alias ptr -> rsi
;               [44:5] mov(rax, 1)
;                   [44:14] 1
                    mov rax, 1
;               [44:19] # write system call
;               [45:5] mov(rdi, 0)
;                   [45:14] 0
                    mov rdi, 0
;               [45:19] # file descriptor for standard out
;               [46:5] mov(rsi, ptr)
;                   [46:14] ptr
;               [46:19] # buffer address
;               [47:5] mov(rdx, len)
;                   [47:14] len
;               [47:19] # buffer size
;               [48:5] syscall()
                syscall
;               [52:4] free named register rsi
;               [52:4] free named register rdx
            sys_print_52_4_273_9_end:
        print_273_9_end:
;       [274:9] str_in(nm)
;       [96:6] str_in(s : str)
        str_in_274_9:
;           [274:9] alias s -> nm
;           [97:5] mov(rax, 0)
;               [97:14] 0
                mov rax, 0
;           [97:37] # read system call
;           [98:5] mov(rdi, 0)
;               [98:14] 0
                mov rdi, 0
;           [98:37] # file descriptor for standard input
;           [99:5] mov(rsi, address_of(s.data))
;               [99:14] rsi = address_of(s.data)
;               [99:14] = expression
;               [99:14] address_of(s.data)
                lea rsi, [rsp - 1156]
;           [99:37] # buffer address
;           [100:5] mov(rdx, array_size_of(s.data))
;               [100:14] rdx = array_size_of(s.data)
;               [100:14] = expression
;               [100:14] array_size_of(s.data)
                mov rdx, 127
;           [100:37] # buffer size
;           [101:5] syscall()
            syscall
;           [102:5] mov(s.len, rax - 1)
;               [102:16] instructions without scratch register 2, with 3
;               [102:16] rax
                mov byte [rsp - 1157], al
;               [102:22] s.len - 1
                sub byte [rsp - 1157], 1
;           [102:25] # return value
        str_in_274_9_end:
        if_275_12:
;       [275:12] ? nm.len == 0
;       [275:12] ? nm.len == 0
        cmp_275_12:
        cmp byte [rsp - 1157], 0
        jne if_277_19
        if_275_12_code:
;           [276:13] break
            jmp loop_272_5_end
        jmp if_275_9_end
        if_277_19:
;       [277:19] ? nm.len <= 4
;       [277:19] ? nm.len <= 4
        cmp_277_19:
        cmp byte [rsp - 1157], 4
        jg if_else_275_9
        if_277_19_code:
;           [278:13] print(prompt2)
;           [51:6] print(str : i8[])
            print_278_13:
;               [278:13] alias str -> prompt2
;               [52:4] sys_print(array_size_of(str), address_of(str))
;               [52:14] allocate named register rdx
;               [52:14] rdx = array_size_of(str)
;               [52:14] = expression
;               [52:14] array_size_of(str)
                mov rdx, 20
;               [52:34] allocate named register rsi
;               [52:34] rsi = address_of(str)
;               [52:34] = expression
;               [52:34] address_of(str)
                lea rsi, [rsp - 53]
;               [43:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_52_4_278_13:
;                   [52:4] alias len -> rdx
;                   [52:4] alias ptr -> rsi
;                   [44:5] mov(rax, 1)
;                       [44:14] 1
                        mov rax, 1
;                   [44:19] # write system call
;                   [45:5] mov(rdi, 0)
;                       [45:14] 0
                        mov rdi, 0
;                   [45:19] # file descriptor for standard out
;                   [46:5] mov(rsi, ptr)
;                       [46:14] ptr
;                   [46:19] # buffer address
;                   [47:5] mov(rdx, len)
;                       [47:14] len
;                   [47:19] # buffer size
;                   [48:5] syscall()
                    syscall
;                   [52:4] free named register rsi
;                   [52:4] free named register rdx
                sys_print_52_4_278_13_end:
            print_278_13_end:
;           [279:13] continue
            jmp loop_272_5
        jmp if_275_9_end
        if_else_275_9:
;           [281:13] print(prompt3)
;           [51:6] print(str : i8[])
            print_281_13:
;               [281:13] alias str -> prompt3
;               [52:4] sys_print(array_size_of(str), address_of(str))
;               [52:14] allocate named register rdx
;               [52:14] rdx = array_size_of(str)
;               [52:14] = expression
;               [52:14] array_size_of(str)
                mov rdx, 6
;               [52:34] allocate named register rsi
;               [52:34] rsi = address_of(str)
;               [52:34] = expression
;               [52:34] address_of(str)
                lea rsi, [rsp - 59]
;               [43:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_52_4_281_13:
;                   [52:4] alias len -> rdx
;                   [52:4] alias ptr -> rsi
;                   [44:5] mov(rax, 1)
;                       [44:14] 1
                        mov rax, 1
;                   [44:19] # write system call
;                   [45:5] mov(rdi, 0)
;                       [45:14] 0
                        mov rdi, 0
;                   [45:19] # file descriptor for standard out
;                   [46:5] mov(rsi, ptr)
;                       [46:14] ptr
;                   [46:19] # buffer address
;                   [47:5] mov(rdx, len)
;                       [47:14] len
;                   [47:19] # buffer size
;                   [48:5] syscall()
                    syscall
;                   [52:4] free named register rsi
;                   [52:4] free named register rdx
                sys_print_52_4_281_13_end:
            print_281_13_end:
;           [282:13] str_out(nm)
;           [105:6] str_out(s : str)
            str_out_282_13:
;               [282:13] alias s -> nm
;               [106:5] mov(rax, 1)
;                   [106:14] 1
                    mov rax, 1
;               [106:34] # write system call
;               [107:5] mov(rdi, 0)
;                   [107:14] 0
                    mov rdi, 0
;               [107:34] # file descriptor for standard out
;               [108:5] mov(rsi, address_of(s.data))
;                   [108:14] rsi = address_of(s.data)
;                   [108:14] = expression
;                   [108:14] address_of(s.data)
                    lea rsi, [rsp - 1156]
;               [108:34] # buffer address
;               [109:5] mov(rdx, s.len)
;                   [109:14] s.len
                    movsx rdx, byte [rsp - 1157]
;               [109:34] # buffer size
;               [110:5] syscall()
                syscall
            str_out_282_13_end:
;           [283:13] print(dot)
;           [51:6] print(str : i8[])
            print_283_13:
;               [283:13] alias str -> dot
;               [52:4] sys_print(array_size_of(str), address_of(str))
;               [52:14] allocate named register rdx
;               [52:14] rdx = array_size_of(str)
;               [52:14] = expression
;               [52:14] array_size_of(str)
                mov rdx, 1
;               [52:34] allocate named register rsi
;               [52:34] rsi = address_of(str)
;               [52:34] = expression
;               [52:34] address_of(str)
                lea rsi, [rsp - 60]
;               [43:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_52_4_283_13:
;                   [52:4] alias len -> rdx
;                   [52:4] alias ptr -> rsi
;                   [44:5] mov(rax, 1)
;                       [44:14] 1
                        mov rax, 1
;                   [44:19] # write system call
;                   [45:5] mov(rdi, 0)
;                       [45:14] 0
                        mov rdi, 0
;                   [45:19] # file descriptor for standard out
;                   [46:5] mov(rsi, ptr)
;                       [46:14] ptr
;                   [46:19] # buffer address
;                   [47:5] mov(rdx, len)
;                       [47:14] len
;                   [47:19] # buffer size
;                   [48:5] syscall()
                    syscall
;                   [52:4] free named register rsi
;                   [52:4] free named register rdx
                sys_print_52_4_283_13_end:
            print_283_13_end:
;           [284:13] print(nl)
;           [51:6] print(str : i8[])
            print_284_13:
;               [284:13] alias str -> nl
;               [52:4] sys_print(array_size_of(str), address_of(str))
;               [52:14] allocate named register rdx
;               [52:14] rdx = array_size_of(str)
;               [52:14] = expression
;               [52:14] array_size_of(str)
                mov rdx, 1
;               [52:34] allocate named register rsi
;               [52:34] rsi = address_of(str)
;               [52:34] = expression
;               [52:34] address_of(str)
                lea rsi, [rsp - 61]
;               [43:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_52_4_284_13:
;                   [52:4] alias len -> rdx
;                   [52:4] alias ptr -> rsi
;                   [44:5] mov(rax, 1)
;                       [44:14] 1
                        mov rax, 1
;                   [44:19] # write system call
;                   [45:5] mov(rdi, 0)
;                       [45:14] 0
                        mov rdi, 0
;                   [45:19] # file descriptor for standard out
;                   [46:5] mov(rsi, ptr)
;                       [46:14] ptr
;                   [46:19] # buffer address
;                   [47:5] mov(rdx, len)
;                       [47:14] len
;                   [47:19] # buffer size
;                   [48:5] syscall()
                    syscall
;                   [52:4] free named register rsi
;                   [52:4] free named register rdx
                sys_print_52_4_284_13_end:
            print_284_13_end:
        if_275_9_end:
    jmp loop_272_5
    loop_272_5_end:
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
;[27:10] s1
;[27:23] i8
db 3
;[27:21] zero remaining fields
times 127 db 0
;[26:8] nums
;[26:15] i64[4]
;[26:26] [0]
;[26:26] i64
dq 1
;[26:15] pad 3 'i64' of size 8
times 24 db 0
;[25:10] nl
;[25:22] i8[1]
db `\n`
;[24:9] dot
;[24:22] i8[1]
db `.`
;[23:5] prompt3
;[23:22] i8[6]
db `hello `
;[22:5] prompt2
;[22:22] i8[20]
db `that is not a name.\n`
;[21:5] prompt1
;[21:22] i8[12]
db `enter name:\n`
;[20:7] hello
;[20:22] i8[21]
db `hello world from baz\n`
dat.len equ $ - dat

; max scratch registers in use: 5
;            max frames in use: 10
;               max stack size: 1157 B
;          optimization pass 1: 120
;          optimization pass 2: 0
```
