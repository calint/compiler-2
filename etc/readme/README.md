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
C/C++ Header                    49           1895            882           8068
C++                              1             17              7            181
-------------------------------------------------------------------------------
SUM:                            50           1912            889           8249
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

dat padding : i8[3]
# total data size must be a multiple of 16
# baz will message how much padding is missing if that is the case


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
section .bss.stack nobits
align 16
stk:
stk resb 131072
stk.end:
section .text
bits 64
global _start
_start:
mov rsp, dat.end
main:
    mov qword [rsp - 240], 0
    mov qword [rsp - 232], 0
    mov qword [rsp - 248], 0
    cmp_138_12:
    cmp qword [rsp - 248], 0
    sete r15b
    bool_end_138_12:
    assert_138_5:
        if_46_26_138_5:
        cmp_46_26_138_5:
        cmp r15b, 0
        jne if_46_23_138_5_end
        if_46_26_138_5_code:
            mov rdi, 1
            exit_46_32_138_5:
                    mov rax, 60
                syscall
            exit_46_32_138_5_end:
        if_46_23_138_5_end:
    assert_138_5_end:
    mov qword [rsp - 248], -1
    cmp_142_12:
    cmp qword [rsp - 248], -1
    sete r15b
    bool_end_142_12:
    assert_142_5:
        if_46_26_142_5:
        cmp_46_26_142_5:
        cmp r15b, 0
        jne if_46_23_142_5_end
        if_46_26_142_5_code:
            mov rdi, 1
            exit_46_32_142_5:
                    mov rax, 60
                syscall
            exit_46_32_142_5_end:
        if_46_23_142_5_end:
    assert_142_5_end:
        cmp_146_16:
        bool_end_146_16:
        mov r15b, 1
        assert_146_9:
            if_46_26_146_9:
            cmp_46_26_146_9:
            cmp r15b, 0
            jne if_46_23_146_9_end
            if_46_26_146_9_code:
                mov rdi, 1
                exit_46_32_146_9:
                        mov rax, 60
                    syscall
                exit_46_32_146_9_end:
            if_46_23_146_9_end:
        assert_146_9_end:
    cmp_149_12:
    bool_end_149_12:
    mov r15b, 1
    assert_149_5:
        if_46_26_149_5:
        cmp_46_26_149_5:
        cmp r15b, 0
        jne if_46_23_149_5_end
        if_46_26_149_5_code:
            mov rdi, 1
            exit_46_32_149_5:
                    mov rax, 60
                syscall
            exit_46_32_149_5_end:
        if_46_23_149_5_end:
    assert_149_5_end:
    mov qword [rsp - 256], 1
    mov r15, qword [rsp - 256]
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 240], 2
    mov r15, qword [rsp - 256]
    add r15, 1
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 256]
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 240]
    mov dword [rsp + r15 * 4 - 240], r13d
    cmp_156_12:
    mov r14, 1
    mov r13, 156
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 240], 2
    sete r15b
    bool_end_156_12:
    assert_156_5:
        if_46_26_156_5:
        cmp_46_26_156_5:
        cmp r15b, 0
        jne if_46_23_156_5_end
        if_46_26_156_5_code:
            mov rdi, 1
            exit_46_32_156_5:
                    mov rax, 60
                syscall
            exit_46_32_156_5_end:
        if_46_23_156_5_end:
    assert_156_5_end:
    cmp_157_12:
    mov r14, 2
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 240], 2
    sete r15b
    bool_end_157_12:
    assert_157_5:
        if_46_26_157_5:
        cmp_46_26_157_5:
        cmp r15b, 0
        jne if_46_23_157_5_end
        if_46_26_157_5_code:
            mov rdi, 1
            exit_46_32_157_5:
                    mov rax, 60
                syscall
            exit_46_32_157_5_end:
        if_46_23_157_5_end:
    assert_157_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 159
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 240]
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 240]
    shl rcx, 2
    rep movsb
    cmp_161_12:
    mov r14, 0
    mov r13, 161
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 240], 2
    sete r15b
    bool_end_161_12:
    assert_161_5:
        if_46_26_161_5:
        cmp_46_26_161_5:
        cmp r15b, 0
        jne if_46_23_161_5_end
        if_46_26_161_5_code:
            mov rdi, 1
            exit_46_32_161_5:
                    mov rax, 60
                syscall
            exit_46_32_161_5_end:
        if_46_23_161_5_end:
    assert_161_5_end:
    mov qword [rsp - 288], 0
    mov qword [rsp - 280], 0
    mov qword [rsp - 272], 0
    mov qword [rsp - 264], 0
    mov rcx, 4
    mov r15, 164
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 240]
    mov r15, 164
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 288]
    shl rcx, 2
    rep movsb
    cmp_165_12:
        mov rcx, 4
        mov r13, 165
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 240]
        mov r13, 165
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 288]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_165_12:
    assert_165_5:
        if_46_26_165_5:
        cmp_46_26_165_5:
        cmp r15b, 0
        jne if_46_23_165_5_end
        if_46_26_165_5_code:
            mov rdi, 1
            exit_46_32_165_5:
                    mov rax, 60
                syscall
            exit_46_32_165_5_end:
        if_46_23_165_5_end:
    assert_165_5_end:
    mov r15, 2
    mov r14, 168
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 288], -1
    cmp_169_12:
        mov rcx, 4
        mov r13, 169
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 240]
        mov r13, 169
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 288]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_169_12:
    assert_169_5:
        if_46_26_169_5:
        cmp_46_26_169_5:
        cmp r15b, 0
        jne if_46_23_169_5_end
        if_46_26_169_5_code:
            mov rdi, 1
            exit_46_32_169_5:
                    mov rax, 60
                syscall
            exit_46_32_169_5_end:
        if_46_23_169_5_end:
    assert_169_5_end:
    mov qword [rsp - 256], 3
    mov r15, qword [rsp - 256]
    mov r14, 172
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 256]
    sub r14, 1
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_172_16:
        mov r13d, dword [rsp + r14 * 4 - 240]
        mov dword [rsp + r15 * 4 - 240], r13d
        not dword [rsp + r15 * 4 - 240]
    inv_172_16_end:
    not dword [rsp + r15 * 4 - 240]
    cmp_173_12:
    mov r14, qword [rsp - 256]
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 240], 2
    sete r15b
    bool_end_173_12:
    assert_173_5:
        if_46_26_173_5:
        cmp_46_26_173_5:
        cmp r15b, 0
        jne if_46_23_173_5_end
        if_46_26_173_5_code:
            mov rdi, 1
            exit_46_32_173_5:
                    mov rax, 60
                syscall
            exit_46_32_173_5_end:
        if_46_23_173_5_end:
    assert_173_5_end:
    faz_175_5:
        mov r15, 1
        mov r14, 98
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 240], 254
    faz_175_5_end:
    cmp_176_12:
    mov r14, 1
    mov r13, 176
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 240], 254
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_46_26_176_5:
        cmp_46_26_176_5:
        cmp r15b, 0
        jne if_46_23_176_5_end
        if_46_26_176_5_code:
            mov rdi, 1
            exit_46_32_176_5:
                    mov rax, 60
                syscall
            exit_46_32_176_5_end:
        if_46_23_176_5_end:
    assert_176_5_end:
    mov qword [rsp - 304], 3
    mov qword [rsp - 296], 5
    lea r15, [rsp - 304]
    mov qword [rsp - 320], 0
    foo_179_5:
        mov r14, qword [rsp - 320]
        add qword [r15], r14
        add qword [r15], 2
        foo_179_5_continue:
            add r15, 8
            inc qword [rsp - 320]
            cmp qword [rsp - 320], 2
            jne foo_179_5
    foo_179_5_end:
    cmp_182_12:
    mov r14, 0
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rsp + r14 * 8 - 304], r13
    sete r15b
    bool_end_182_12:
    assert_182_5:
        if_46_26_182_5:
        cmp_46_26_182_5:
        cmp r15b, 0
        jne if_46_23_182_5_end
        if_46_26_182_5_code:
            mov rdi, 1
            exit_46_32_182_5:
                    mov rax, 60
                syscall
            exit_46_32_182_5_end:
        if_46_23_182_5_end:
    assert_182_5_end:
    cmp_183_12:
    mov r14, 1
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rsp + r14 * 8 - 304], r13
    sete r15b
    bool_end_183_12:
    assert_183_5:
        if_46_26_183_5:
        cmp_46_26_183_5:
        cmp r15b, 0
        jne if_46_23_183_5_end
        if_46_26_183_5_code:
            mov rdi, 1
            exit_46_32_183_5:
                    mov rax, 60
                syscall
            exit_46_32_183_5_end:
        if_46_23_183_5_end:
    assert_183_5_end:
    mov qword [rsp - 320], 0
    mov qword [rsp - 312], 0
    fooz_190_5:
        mov qword [rsp - 320], 2
        mov qword [rsp - 312], 11
    fooz_190_5_end:
    cmp_191_12:
    cmp qword [rsp - 320], 2
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_46_26_191_5:
        cmp_46_26_191_5:
        cmp r15b, 0
        jne if_46_23_191_5_end
        if_46_26_191_5_code:
            mov rdi, 1
            exit_46_32_191_5:
                    mov rax, 60
                syscall
            exit_46_32_191_5_end:
        if_46_23_191_5_end:
    assert_191_5_end:
    cmp_192_12:
    cmp qword [rsp - 312], 11
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_46_26_192_5:
        cmp_46_26_192_5:
        cmp r15b, 0
        jne if_46_23_192_5_end
        if_46_26_192_5_code:
            mov rdi, 1
            exit_46_32_192_5:
                    mov rax, 60
                syscall
            exit_46_32_192_5_end:
        if_46_23_192_5_end:
    assert_192_5_end:
    mov rax, qword [rsp - 320]
    mov qword [rsp - 336], rax
    mov rax, qword [rsp - 312]
    mov qword [rsp - 328], rax
    cmp_195_12:
        lea rsi, [rsp - 320]
        lea rdi, [rsp - 336]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_195_12:
    assert_195_5:
        if_46_26_195_5:
        cmp_46_26_195_5:
        cmp r15b, 0
        jne if_46_23_195_5_end
        if_46_26_195_5_code:
            mov rdi, 1
            exit_46_32_195_5:
                    mov rax, 60
                syscall
            exit_46_32_195_5_end:
        if_46_23_195_5_end:
    assert_195_5_end:
    mov qword [rsp - 336], 3
    cmp_200_12:
        lea rsi, [rsp - 320]
        lea rdi, [rsp - 336]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_200_12:
    assert_200_5:
        if_46_26_200_5:
        cmp_46_26_200_5:
        cmp r15b, 0
        jne if_46_23_200_5_end
        if_46_26_200_5_code:
            mov rdi, 1
            exit_46_32_200_5:
                    mov rax, 60
                syscall
            exit_46_32_200_5_end:
        if_46_23_200_5_end:
    assert_200_5_end:
    mov qword [rsp - 344], 0
    bar_203_5:
        if_79_8_203_5:
        cmp_79_8_203_5:
        cmp qword [rsp - 344], 0
        jne if_79_5_203_5_end
        if_79_8_203_5_code:
            jmp bar_203_5_end
        if_79_5_203_5_end:
        mov qword [rsp - 344], 255
    bar_203_5_end:
    cmp_204_12:
    cmp qword [rsp - 344], 0
    sete r15b
    bool_end_204_12:
    assert_204_5:
        if_46_26_204_5:
        cmp_46_26_204_5:
        cmp r15b, 0
        jne if_46_23_204_5_end
        if_46_26_204_5_code:
            mov rdi, 1
            exit_46_32_204_5:
                    mov rax, 60
                syscall
            exit_46_32_204_5_end:
        if_46_23_204_5_end:
    assert_204_5_end:
    mov qword [rsp - 344], 1
    bar_207_5:
        if_79_8_207_5:
        cmp_79_8_207_5:
        cmp qword [rsp - 344], 0
        jne if_79_5_207_5_end
        if_79_8_207_5_code:
            jmp bar_207_5_end
        if_79_5_207_5_end:
        mov qword [rsp - 344], 255
    bar_207_5_end:
    cmp_208_12:
    cmp qword [rsp - 344], 255
    sete r15b
    bool_end_208_12:
    assert_208_5:
        if_46_26_208_5:
        cmp_46_26_208_5:
        cmp r15b, 0
        jne if_46_23_208_5_end
        if_46_26_208_5_code:
            mov rdi, 1
            exit_46_32_208_5:
                    mov rax, 60
                syscall
            exit_46_32_208_5_end:
        if_46_23_208_5_end:
    assert_208_5_end:
    mov qword [rsp - 352], 1
    baz_211_13:
        mov r15, qword [rsp - 352]
        imul r15, 2
        mov qword [rsp - 360], r15
    baz_211_13_end:
    cmp_212_12:
    cmp qword [rsp - 360], 2
    sete r15b
    bool_end_212_12:
    assert_212_5:
        if_46_26_212_5:
        cmp_46_26_212_5:
        cmp r15b, 0
        jne if_46_23_212_5_end
        if_46_26_212_5_code:
            mov rdi, 1
            exit_46_32_212_5:
                    mov rax, 60
                syscall
            exit_46_32_212_5_end:
        if_46_23_212_5_end:
    assert_212_5_end:
    baz_214_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 360], r15
    baz_214_9_end:
    cmp_215_12:
    cmp qword [rsp - 360], 2
    sete r15b
    bool_end_215_12:
    assert_215_5:
        if_46_26_215_5:
        cmp_46_26_215_5:
        cmp r15b, 0
        jne if_46_23_215_5_end
        if_46_26_215_5_code:
            mov rdi, 1
            exit_46_32_215_5:
                    mov rax, 60
                syscall
            exit_46_32_215_5_end:
        if_46_23_215_5_end:
    assert_215_5_end:
    baz_217_23:
        mov r15, 3
        imul r15, 2
        mov qword [rsp - 376], r15
    baz_217_23_end:
    mov qword [rsp - 368], 0
    cmp_218_12:
    cmp qword [rsp - 376], 6
    sete r15b
    bool_end_218_12:
    assert_218_5:
        if_46_26_218_5:
        cmp_46_26_218_5:
        cmp r15b, 0
        jne if_46_23_218_5_end
        if_46_26_218_5_code:
            mov rdi, 1
            exit_46_32_218_5:
                    mov rax, 60
                syscall
            exit_46_32_218_5_end:
        if_46_23_218_5_end:
    assert_218_5_end:
    point_init_220_22:
        mov qword [rsp - 392], -1
        mov qword [rsp - 384], -2
    point_init_220_22_end:
    cmp_221_12:
    cmp qword [rsp - 392], -1
    sete r15b
    bool_end_221_12:
    assert_221_5:
        if_46_26_221_5:
        cmp_46_26_221_5:
        cmp r15b, 0
        jne if_46_23_221_5_end
        if_46_26_221_5_code:
            mov rdi, 1
            exit_46_32_221_5:
                    mov rax, 60
                syscall
            exit_46_32_221_5_end:
        if_46_23_221_5_end:
    assert_221_5_end:
    cmp_222_12:
    cmp qword [rsp - 384], -2
    sete r15b
    bool_end_222_12:
    assert_222_5:
        if_46_26_222_5:
        cmp_46_26_222_5:
        cmp r15b, 0
        jne if_46_23_222_5_end
        if_46_26_222_5_code:
            mov rdi, 1
            exit_46_32_222_5:
                    mov rax, 60
                syscall
            exit_46_32_222_5_end:
        if_46_23_222_5_end:
    assert_222_5_end:
    mov qword [rsp - 400], 1
    mov qword [rsp - 408], 2
    mov r15, qword [rsp - 400]
    imul r15, 10
    mov qword [rsp - 428], r15
    mov r15, qword [rsp - 408]
    mov qword [rsp - 420], r15
    mov dword [rsp - 412], 16711680
    cmp_228_12:
    cmp qword [rsp - 428], 10
    sete r15b
    bool_end_228_12:
    assert_228_5:
        if_46_26_228_5:
        cmp_46_26_228_5:
        cmp r15b, 0
        jne if_46_23_228_5_end
        if_46_26_228_5_code:
            mov rdi, 1
            exit_46_32_228_5:
                    mov rax, 60
                syscall
            exit_46_32_228_5_end:
        if_46_23_228_5_end:
    assert_228_5_end:
    cmp_229_12:
    cmp qword [rsp - 420], 2
    sete r15b
    bool_end_229_12:
    assert_229_5:
        if_46_26_229_5:
        cmp_46_26_229_5:
        cmp r15b, 0
        jne if_46_23_229_5_end
        if_46_26_229_5_code:
            mov rdi, 1
            exit_46_32_229_5:
                    mov rax, 60
                syscall
            exit_46_32_229_5_end:
        if_46_23_229_5_end:
    assert_229_5_end:
    cmp_230_12:
    cmp dword [rsp - 412], 16711680
    sete r15b
    bool_end_230_12:
    assert_230_5:
        if_46_26_230_5:
        cmp_46_26_230_5:
        cmp r15b, 0
        jne if_46_23_230_5_end
        if_46_26_230_5_code:
            mov rdi, 1
            exit_46_32_230_5:
                    mov rax, 60
                syscall
            exit_46_32_230_5_end:
        if_46_23_230_5_end:
    assert_230_5_end:
    mov r15, qword [rsp - 400]
    mov qword [rsp - 444], r15
    neg qword [rsp - 444]
    mov r15, qword [rsp - 408]
    mov qword [rsp - 436], r15
    neg qword [rsp - 436]
    mov rax, qword [rsp - 444]
    mov qword [rsp - 428], rax
    mov rax, qword [rsp - 436]
    mov qword [rsp - 420], rax
    cmp_234_12:
    cmp qword [rsp - 428], -1
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_46_26_234_5:
        cmp_46_26_234_5:
        cmp r15b, 0
        jne if_46_23_234_5_end
        if_46_26_234_5_code:
            mov rdi, 1
            exit_46_32_234_5:
                    mov rax, 60
                syscall
            exit_46_32_234_5_end:
        if_46_23_234_5_end:
    assert_234_5_end:
    cmp_235_12:
    cmp qword [rsp - 420], -2
    sete r15b
    bool_end_235_12:
    assert_235_5:
        if_46_26_235_5:
        cmp_46_26_235_5:
        cmp r15b, 0
        jne if_46_23_235_5_end
        if_46_26_235_5_code:
            mov rdi, 1
            exit_46_32_235_5:
                    mov rax, 60
                syscall
            exit_46_32_235_5_end:
        if_46_23_235_5_end:
    assert_235_5_end:
    lea rsi, [rsp - 428]
    lea rdi, [rsp - 464]
    mov rcx, 20
    rep movsb
    cmp_238_12:
    cmp qword [rsp - 464], -1
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_46_26_238_5:
        cmp_46_26_238_5:
        cmp r15b, 0
        jne if_46_23_238_5_end
        if_46_26_238_5_code:
            mov rdi, 1
            exit_46_32_238_5:
                    mov rax, 60
                syscall
            exit_46_32_238_5_end:
        if_46_23_238_5_end:
    assert_238_5_end:
    cmp_239_12:
    cmp qword [rsp - 456], -2
    sete r15b
    bool_end_239_12:
    assert_239_5:
        if_46_26_239_5:
        cmp_46_26_239_5:
        cmp r15b, 0
        jne if_46_23_239_5_end
        if_46_26_239_5_code:
            mov rdi, 1
            exit_46_32_239_5:
                    mov rax, 60
                syscall
            exit_46_32_239_5_end:
        if_46_23_239_5_end:
    assert_239_5_end:
    cmp_240_12:
    cmp dword [rsp - 448], 16711680
    sete r15b
    bool_end_240_12:
    assert_240_5:
        if_46_26_240_5:
        cmp_46_26_240_5:
        cmp r15b, 0
        jne if_46_23_240_5_end
        if_46_26_240_5_code:
            mov rdi, 1
            exit_46_32_240_5:
                    mov rax, 60
                syscall
            exit_46_32_240_5_end:
        if_46_23_240_5_end:
    assert_240_5_end:
    xor al, al
    lea rdi, [rsp - 504]
    mov rcx, 40
    rep stosb
    mov qword [rsp - 496], 73
    cmp_246_12:
    lea r14, [rsp - 504]
    mov r13, 0
    mov r12, 246
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
    bool_end_246_12:
    assert_246_5:
        if_46_26_246_5:
        cmp_46_26_246_5:
        cmp r15b, 0
        jne if_46_23_246_5_end
        if_46_26_246_5_code:
            mov rdi, 1
            exit_46_32_246_5:
                    mov rax, 60
                syscall
            exit_46_32_246_5_end:
        if_46_23_246_5_end:
    assert_246_5_end:
    lea r15, [rsp - 504]
    mov r14, 1
    mov r13, 248
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    imul r14, 20
    add r15, r14
    object_init_248_13:
        mov qword [r15 + 8], 74
    object_init_248_13_end:
    cmp_249_12:
    lea r14, [rsp - 504]
    mov r13, 1
    mov r12, 249
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
    bool_end_249_12:
    assert_249_5:
        if_46_26_249_5:
        cmp_46_26_249_5:
        cmp r15b, 0
        jne if_46_23_249_5_end
        if_46_26_249_5_code:
            mov rdi, 1
            exit_46_32_249_5:
                    mov rax, 60
                syscall
            exit_46_32_249_5_end:
        if_46_23_249_5_end:
    assert_249_5_end:
    xor al, al
    lea rdi, [rsp - 1016]
    mov rcx, 512
    rep stosb
    lea r15, [rsp - 1016]
    mov r14, 1
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_253_12:
    lea r14, [rsp - 1016]
    mov r13, 1
    mov r12, 253
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 253
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_253_12:
    assert_253_5:
        if_46_26_253_5:
        cmp_46_26_253_5:
        cmp r15b, 0
        jne if_46_23_253_5_end
        if_46_26_253_5_code:
            mov rdi, 1
            exit_46_32_253_5:
                    mov rax, 60
                syscall
            exit_46_32_253_5_end:
        if_46_23_253_5_end:
    assert_253_5_end:
    mov rcx, 8
    lea r15, [rsp - 1016]
    mov r14, 1
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 256
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 1016]
    mov r14, 0
    mov r13, 257
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 257
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_263_12:
    lea r14, [rsp - 1016]
    mov r13, 0
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_263_12:
    assert_263_5:
        if_46_26_263_5:
        cmp_46_26_263_5:
        cmp r15b, 0
        jne if_46_23_263_5_end
        if_46_26_263_5_code:
            mov rdi, 1
            exit_46_32_263_5:
                    mov rax, 60
                syscall
            exit_46_32_263_5_end:
        if_46_23_263_5_end:
    assert_263_5_end:
    cmp_264_12:
        mov rcx, 8
        lea r13, [rsp - 1016]
        mov r12, 0
        mov r11, 265
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 265
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 1016]
        mov r12, 1
        mov r11, 266
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 266
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
    bool_end_264_12:
    assert_264_5:
        if_46_26_264_5:
        cmp_46_26_264_5:
        cmp r15b, 0
        jne if_46_23_264_5_end
        if_46_26_264_5_code:
            mov rdi, 1
            exit_46_32_264_5:
                    mov rax, 60
                syscall
            exit_46_32_264_5_end:
        if_46_23_264_5_end:
    assert_264_5_end:
    mov qword [rsp - 1032], -1
    mov qword [rsp - 1024], 2
    cmp_271_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_271_12:
    assert_271_5:
        if_46_26_271_5:
        cmp_46_26_271_5:
        cmp r15b, 0
        jne if_46_23_271_5_end
        if_46_26_271_5_code:
            mov rdi, 1
            exit_46_32_271_5:
                    mov rax, 60
                syscall
            exit_46_32_271_5_end:
        if_46_23_271_5_end:
    assert_271_5_end:
    cmp_272_12:
    mov r14, 0
    mov r13, 272
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1032], -1
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_46_26_272_5:
        cmp_46_26_272_5:
        cmp r15b, 0
        jne if_46_23_272_5_end
        if_46_26_272_5_code:
            mov rdi, 1
            exit_46_32_272_5:
                    mov rax, 60
                syscall
            exit_46_32_272_5_end:
        if_46_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
    mov r14, 1
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
    cmp qword [rsp + r14 * 8 - 1032], 2
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_46_26_273_5:
        cmp_46_26_273_5:
        cmp r15b, 0
        jne if_46_23_273_5_end
        if_46_26_273_5_code:
            mov rdi, 1
            exit_46_32_273_5:
                    mov rax, 60
                syscall
            exit_46_32_273_5_end:
        if_46_23_273_5_end:
    assert_273_5_end:
    xor al, al
    lea rdi, [rsp - 1160]
    mov rcx, 128
    rep stosb
    print_276_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        sys_print_57_4_276_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_57_4_276_5_end:
    print_276_5_end:
    loop_277_5:
        print_278_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            sys_print_57_4_278_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_57_4_278_9_end:
        print_278_9_end:
        str_in_279_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 1159]
                mov rdx, 127
            syscall
                mov byte [rsp - 1160], al
                sub byte [rsp - 1160], 1
        str_in_279_9_end:
        if_280_12:
        cmp_280_12:
        cmp byte [rsp - 1160], 0
        jne if_282_19
        if_280_12_code:
            jmp loop_277_5_end
        jmp if_280_9_end
        if_282_19:
        cmp_282_19:
        cmp byte [rsp - 1160], 4
        jg if_else_280_9
        if_282_19_code:
            print_283_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                sys_print_57_4_283_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_283_13_end:
            print_283_13_end:
            jmp loop_277_5
        jmp if_280_9_end
        if_else_280_9:
            print_286_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                sys_print_57_4_286_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_286_13_end:
            print_286_13_end:
            str_out_287_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 1159]
                    movsx rdx, byte [rsp - 1160]
                syscall
            str_out_287_13_end:
            print_288_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                sys_print_57_4_288_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_288_13_end:
            print_288_13_end:
            print_289_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                sys_print_57_4_289_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_57_4_289_13_end:
            print_289_13_end:
        if_280_9_end:
    jmp loop_277_5
    loop_277_5_end:
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
section .data
align 16
dat:
times 3 db 0
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
dat.end:
```

## With comments

```nasm

;
; generated by baz
;

default rel

section .bss.stack nobits
align 16
stk:
stk resb 131072
stk.end:

section .text
bits 64
global _start
_start:

; initialize stack pointer
mov rsp, dat.end

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
;[29:1] dat padding : i8[3]
;[29:5] padding: i8[3] (3 B @ [rsp - 224])
;[30:1] # total data size must be a multiple of 16
;[31:1] # baz will message how much padding is missing if that is the case
;[34:1] # all functions are inlined
;[36:1] # arguments can be placed in specified register using `reg_...` syntax
;[44:1] # single statement blocks can ommit { ... }
;[69:1] # function arguments and return are equivalent to mutable references
;[76:1] # default argument type is `i64`
;[83:1] # return target is specified as a variable, in this case `res`
;[85:1] # return variable is a mutable reference to destination
;[95:1] # array arguments are declared with type and []
;[127:7] const yes = 1
;[128:7] const no = 0
;[129:7] const maybe = -1
;[131:1] # constants can be declared in any scope and shadow outer declarations

main:
;   [134:5] var arr : i32[4]
;   [134:9] arr: i32[4] (16 B @ [rsp - 240])
;   [134:9] zero 4 * 4 B = 16 B
;   [134:5] size <= 32 B, use mov
    mov qword [rsp - 240], 0
    mov qword [rsp - 232], 0
;   [135:5] # arrays are initialized to 0
;   [137:5] var answer
;   [137:9] answer: i64 (8 B @ [rsp - 248])
;   [137:9] zero 1 * 8 B = 8 B
;   [137:5] size <= 32 B, use mov
    mov qword [rsp - 248], 0
;   [138:5] assert(answer == 0)
;   [138:12] allocate scratch register -> r15
;   [138:12] ? answer == 0
;   [138:12] ? answer == 0
    cmp_138_12:
    cmp qword [rsp - 248], 0
    sete r15b
    bool_end_138_12:
;   [46:6] assert(x : bool)
    assert_138_5:
;       [138:5] alias x -> r15b
        if_46_26_138_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_138_5:
        cmp r15b, 0
        jne if_46_23_138_5_end
        if_46_26_138_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_138_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_138_5_end:
        if_46_23_138_5_end:
;       [138:5] free scratch register r15
    assert_138_5_end:
;   [139:5] # variables without initializer are zeroed
;   [141:5] answer = maybe
;   [141:14] instructions without scratch register 1, with 2
;   [141:14] maybe
    mov qword [rsp - 248], -1
;   [142:5] assert(answer == -1)
;   [142:12] allocate scratch register -> r15
;   [142:12] ? answer == -1
;   [142:12] ? answer == -1
    cmp_142_12:
    cmp qword [rsp - 248], -1
    sete r15b
    bool_end_142_12:
;   [46:6] assert(x : bool)
    assert_142_5:
;       [142:5] alias x -> r15b
        if_46_26_142_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_142_5:
        cmp r15b, 0
        jne if_46_23_142_5_end
        if_46_26_142_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_142_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_142_5_end:
        if_46_23_142_5_end:
;       [142:5] free scratch register r15
    assert_142_5_end:
;       [145:15] const maybe = 33
;       [146:9] assert(maybe == 33)
;       [146:16] allocate scratch register -> r15
;       [146:16] ? maybe == 33
;       [146:16] ? maybe == 33
        cmp_146_16:
;       [146:16] const eval to true
        bool_end_146_16:
        mov r15b, 1
;       [46:6] assert(x : bool)
        assert_146_9:
;           [146:9] alias x -> r15b
            if_46_26_146_9:
;           [46:26] ? not x
;           [46:26] ? not x
            cmp_46_26_146_9:
            cmp r15b, 0
            jne if_46_23_146_9_end
            if_46_26_146_9_code:
;               [46:32] exit(1)
;               [46:37] allocate named register rdi
                mov rdi, 1
;               [38:6] exit(v : reg_rdi)
                exit_46_32_146_9:
;                   [46:32] alias v -> rdi
;                   [39:5] mov(rax, 60)
;                       [39:14] 60
                        mov rax, 60
;                   [39:19] # exit system call
;                   [40:5] mov(rdi, v)
;                       [40:14] v
;                   [40:19] # return code
;                   [41:5] syscall()
                    syscall
;                   [46:32] free named register rdi
                exit_46_32_146_9_end:
            if_46_23_146_9_end:
;           [146:9] free scratch register r15
        assert_146_9_end:
;   [149:5] assert(maybe == -1)
;   [149:12] allocate scratch register -> r15
;   [149:12] ? maybe == -1
;   [149:12] ? maybe == -1
    cmp_149_12:
;   [149:12] const eval to true
    bool_end_149_12:
    mov r15b, 1
;   [46:6] assert(x : bool)
    assert_149_5:
;       [149:5] alias x -> r15b
        if_46_26_149_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_149_5:
        cmp r15b, 0
        jne if_46_23_149_5_end
        if_46_26_149_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_149_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_149_5_end:
        if_46_23_149_5_end:
;       [149:5] free scratch register r15
    assert_149_5_end:
;   [151:5] var ix = 1
;   [151:9] ix: i64 (8 B @ [rsp - 256])
;   [151:9] ix = 1
;   [151:14] instructions without scratch register 1, with 2
;   [151:14] 1
    mov qword [rsp - 256], 1
;   [152:5] # variables can have an initial value that can be an expression
;   [154:5] arr[ix] = 2
;   [154:5] allocate scratch register -> r15
;   [154:9] set array index
;   [154:9] ix
    mov r15, qword [rsp - 256]
;   [154:9] bounds check
;   [154:9] allocate scratch register -> r14
;   [154:9] line number
    mov r14, 154
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [154:9] free scratch register r14
;   [154:15] instructions without scratch register 1, with 2
;   [154:15] 2
    mov dword [rsp + r15 * 4 - 240], 2
;   [154:5] free scratch register r15
;   [155:5] arr[ix + 1] = arr[ix]
;   [155:5] allocate scratch register -> r15
;   [155:9] set array index
;   [155:9] ix
    mov r15, qword [rsp - 256]
;   [155:14] r15 + 1
    add r15, 1
;   [155:9] bounds check
;   [155:9] allocate scratch register -> r14
;   [155:9] line number
    mov r14, 155
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [155:9] free scratch register r14
;   [155:19] instructions without scratch register 10, with 10
;   [155:19] arr[ix]
;   [155:19] allocate scratch register -> r14
;   [155:23] set array index
;   [155:23] ix
    mov r14, qword [rsp - 256]
;   [155:23] bounds check
;   [155:23] allocate scratch register -> r13
;   [155:23] line number
    mov r13, 155
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [155:23] free scratch register r13
;   [155:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 240]
    mov dword [rsp + r15 * 4 - 240], r13d
;   [155:19] free scratch register r13
;   [155:19] free scratch register r14
;   [155:5] free scratch register r15
;   [156:5] assert(arr[1] == 2)
;   [156:12] allocate scratch register -> r15
;   [156:12] ? arr[1] == 2
;   [156:12] ? arr[1] == 2
    cmp_156_12:
;   [156:12] allocate scratch register -> r14
;   [156:16] set array index
;   [156:16] 1
    mov r14, 1
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
    cmp dword [rsp + r14 * 4 - 240], 2
;   [156:12] free scratch register r14
    sete r15b
    bool_end_156_12:
;   [46:6] assert(x : bool)
    assert_156_5:
;       [156:5] alias x -> r15b
        if_46_26_156_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_156_5:
        cmp r15b, 0
        jne if_46_23_156_5_end
        if_46_26_156_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_156_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_156_5_end:
        if_46_23_156_5_end:
;       [156:5] free scratch register r15
    assert_156_5_end:
;   [157:5] assert(arr[2] == 2)
;   [157:12] allocate scratch register -> r15
;   [157:12] ? arr[2] == 2
;   [157:12] ? arr[2] == 2
    cmp_157_12:
;   [157:12] allocate scratch register -> r14
;   [157:16] set array index
;   [157:16] 2
    mov r14, 2
;   [157:16] bounds check
;   [157:16] allocate scratch register -> r13
;   [157:16] line number
    mov r13, 157
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [157:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 240], 2
;   [157:12] free scratch register r14
    sete r15b
    bool_end_157_12:
;   [46:6] assert(x : bool)
    assert_157_5:
;       [157:5] alias x -> r15b
        if_46_26_157_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_157_5:
        cmp r15b, 0
        jne if_46_23_157_5_end
        if_46_26_157_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_157_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_157_5_end:
        if_46_23_157_5_end:
;       [157:5] free scratch register r15
    assert_157_5_end:
;   [159:5] array_copy(arr[2], arr, 2)
;   [159:5] allocate named register rsi
;   [159:5] allocate named register rdi
;   [159:5] allocate named register rcx
;   [159:29] 2
;   [159:29] 2
    mov rcx, 2
;   [159:16] arr[2]
;   [159:16] allocate scratch register -> r15
;   [159:20] set array index
;   [159:20] 2
    mov r15, 2
;   [159:20] bounds check
;   [159:20] allocate scratch register -> r14
;   [159:20] line number
    mov r14, 159
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [159:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [159:20] free scratch register r13
    cmovg rbp, r14
    jg panic_bounds
;   [159:20] free scratch register r14
    lea rsi, [rsp + r15 * 4 - 240]
;   [159:5] free scratch register r15
;   [159:24] arr
;   [159:24] bounds check
;   [159:24] allocate scratch register -> r15
;   [159:24] line number
    mov r15, 159
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [159:24] free scratch register r15
    lea rdi, [rsp - 240]
    shl rcx, 2
    rep movsb
;   [159:5] free named register rcx
;   [159:5] free named register rdi
;   [159:5] free named register rsi
;   [160:5] # copy from, to, number of elements
;   [161:5] assert(arr[0] == 2)
;   [161:12] allocate scratch register -> r15
;   [161:12] ? arr[0] == 2
;   [161:12] ? arr[0] == 2
    cmp_161_12:
;   [161:12] allocate scratch register -> r14
;   [161:16] set array index
;   [161:16] 0
    mov r14, 0
;   [161:16] bounds check
;   [161:16] allocate scratch register -> r13
;   [161:16] line number
    mov r13, 161
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [161:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 240], 2
;   [161:12] free scratch register r14
    sete r15b
    bool_end_161_12:
;   [46:6] assert(x : bool)
    assert_161_5:
;       [161:5] alias x -> r15b
        if_46_26_161_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_161_5:
        cmp r15b, 0
        jne if_46_23_161_5_end
        if_46_26_161_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_161_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_161_5_end:
        if_46_23_161_5_end:
;       [161:5] free scratch register r15
    assert_161_5_end:
;   [163:5] var arr1 : i32[8]
;   [163:9] arr1: i32[8] (32 B @ [rsp - 288])
;   [163:9] zero 8 * 4 B = 32 B
;   [163:5] size <= 32 B, use mov
    mov qword [rsp - 288], 0
    mov qword [rsp - 280], 0
    mov qword [rsp - 272], 0
    mov qword [rsp - 264], 0
;   [164:5] array_copy(arr, arr1, 4)
;   [164:5] allocate named register rsi
;   [164:5] allocate named register rdi
;   [164:5] allocate named register rcx
;   [164:27] 4
;   [164:27] 4
    mov rcx, 4
;   [164:16] arr
;   [164:16] bounds check
;   [164:16] allocate scratch register -> r15
;   [164:16] line number
    mov r15, 164
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [164:16] free scratch register r15
    lea rsi, [rsp - 240]
;   [164:21] arr1
;   [164:21] bounds check
;   [164:21] allocate scratch register -> r15
;   [164:21] line number
    mov r15, 164
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [164:21] free scratch register r15
    lea rdi, [rsp - 288]
    shl rcx, 2
    rep movsb
;   [164:5] free named register rcx
;   [164:5] free named register rdi
;   [164:5] free named register rsi
;   [165:5] assert(arrays_equal(arr, arr1, 4))
;   [165:12] allocate scratch register -> r15
;   [165:12] ? arrays_equal(arr, arr1, 4)
;   [165:12] ? arrays_equal(arr, arr1, 4)
    cmp_165_12:
;   [165:12] allocate scratch register -> r14
;       [165:12] r14 = arrays_equal(arr, arr1, 4)
;       [165:12] = expression
;       [165:12] arrays_equal(arr, arr1, 4)
;       [165:12] allocate named register rsi
;       [165:12] allocate named register rdi
;       [165:12] allocate named register rcx
;       [165:36] 4
;       [165:36] 4
        mov rcx, 4
;       [165:25] arr
;       [165:25] bounds check
;       [165:25] allocate scratch register -> r13
;       [165:25] line number
        mov r13, 165
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [165:25] free scratch register r13
        lea rsi, [rsp - 240]
;       [165:30] arr1
;       [165:30] bounds check
;       [165:30] allocate scratch register -> r13
;       [165:30] line number
        mov r13, 165
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [165:30] free scratch register r13
        lea rdi, [rsp - 288]
        shl rcx, 2
        repe cmpsb
;       [165:12] free named register rcx
;       [165:12] free named register rdi
;       [165:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [165:12] free scratch register r14
    setne r15b
    bool_end_165_12:
;   [46:6] assert(x : bool)
    assert_165_5:
;       [165:5] alias x -> r15b
        if_46_26_165_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_165_5:
        cmp r15b, 0
        jne if_46_23_165_5_end
        if_46_26_165_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_165_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_165_5_end:
        if_46_23_165_5_end:
;       [165:5] free scratch register r15
    assert_165_5_end:
;   [166:5] # `arrays_equal` is built-in function
;   [168:5] arr1[2] = -1
;   [168:5] allocate scratch register -> r15
;   [168:10] set array index
;   [168:10] 2
    mov r15, 2
;   [168:10] bounds check
;   [168:10] allocate scratch register -> r14
;   [168:10] line number
    mov r14, 168
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [168:10] free scratch register r14
;   [168:15] instructions without scratch register 1, with 2
;   [168:16] -1
    mov dword [rsp + r15 * 4 - 288], -1
;   [168:5] free scratch register r15
;   [169:5] assert(not arrays_equal(arr, arr1, 4))
;   [169:12] allocate scratch register -> r15
;   [169:12] ? not arrays_equal(arr, arr1, 4)
;   [169:12] ? not arrays_equal(arr, arr1, 4)
    cmp_169_12:
;   [169:16] allocate scratch register -> r14
;       [169:16] r14 = arrays_equal(arr, arr1, 4)
;       [169:16] = expression
;       [169:16] arrays_equal(arr, arr1, 4)
;       [169:16] allocate named register rsi
;       [169:16] allocate named register rdi
;       [169:16] allocate named register rcx
;       [169:40] 4
;       [169:40] 4
        mov rcx, 4
;       [169:29] arr
;       [169:29] bounds check
;       [169:29] allocate scratch register -> r13
;       [169:29] line number
        mov r13, 169
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [169:29] free scratch register r13
        lea rsi, [rsp - 240]
;       [169:34] arr1
;       [169:34] bounds check
;       [169:34] allocate scratch register -> r13
;       [169:34] line number
        mov r13, 169
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [169:34] free scratch register r13
        lea rdi, [rsp - 288]
        shl rcx, 2
        repe cmpsb
;       [169:16] free named register rcx
;       [169:16] free named register rdi
;       [169:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [169:12] free scratch register r14
    sete r15b
    bool_end_169_12:
;   [46:6] assert(x : bool)
    assert_169_5:
;       [169:5] alias x -> r15b
        if_46_26_169_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_169_5:
        cmp r15b, 0
        jne if_46_23_169_5_end
        if_46_26_169_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_169_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_169_5_end:
        if_46_23_169_5_end:
;       [169:5] free scratch register r15
    assert_169_5_end:
;   [171:5] ix = 3
;   [171:10] instructions without scratch register 1, with 2
;   [171:10] 3
    mov qword [rsp - 256], 3
;   [172:5] arr[ix] = ~inv(arr[ix - 1])
;   [172:5] allocate scratch register -> r15
;   [172:9] set array index
;   [172:9] ix
    mov r15, qword [rsp - 256]
;   [172:9] bounds check
;   [172:9] allocate scratch register -> r14
;   [172:9] line number
    mov r14, 172
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [172:9] free scratch register r14
;   [172:15] instructions without scratch register 15, with 15
;   [172:16] arr = ~inv(arr[ix - 1])
;   [172:16] = expression
;   [172:16] ~inv(arr[ix - 1])
;   [172:20] allocate scratch register -> r14
;   [172:24] set array index
;   [172:24] ix
    mov r14, qword [rsp - 256]
;   [172:29] r14 - 1
    sub r14, 1
;   [172:24] bounds check
;   [172:24] allocate scratch register -> r13
;   [172:24] line number
    mov r13, 172
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [172:24] free scratch register r13
;   [87:6] inv(i : i32) : i32 res
    inv_172_16:
;       [172:16] alias res -> arr (lea: rsp + r15 * 4 - 240)
;       [172:16] alias i -> arr (lea: rsp + r14 * 4 - 240)
;       [88:5] res = ~i
;       [88:11] instructions without scratch register 3, with 3
;       [88:12] ~i
;       [88:12] allocate scratch register -> r13
        mov r13d, dword [rsp + r14 * 4 - 240]
        mov dword [rsp + r15 * 4 - 240], r13d
;       [88:12] free scratch register r13
        not dword [rsp + r15 * 4 - 240]
;       [172:16] free scratch register r14
    inv_172_16_end:
    not dword [rsp + r15 * 4 - 240]
;   [172:5] free scratch register r15
;   [173:5] assert(arr[ix] == 2)
;   [173:12] allocate scratch register -> r15
;   [173:12] ? arr[ix] == 2
;   [173:12] ? arr[ix] == 2
    cmp_173_12:
;   [173:12] allocate scratch register -> r14
;   [173:16] set array index
;   [173:16] ix
    mov r14, qword [rsp - 256]
;   [173:16] bounds check
;   [173:16] allocate scratch register -> r13
;   [173:16] line number
    mov r13, 173
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [173:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 240], 2
;   [173:12] free scratch register r14
    sete r15b
    bool_end_173_12:
;   [46:6] assert(x : bool)
    assert_173_5:
;       [173:5] alias x -> r15b
        if_46_26_173_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_173_5:
        cmp r15b, 0
        jne if_46_23_173_5_end
        if_46_26_173_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_173_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_173_5_end:
        if_46_23_173_5_end:
;       [173:5] free scratch register r15
    assert_173_5_end:
;   [175:5] faz(arr)
;   [97:6] faz(arg : i32[])
    faz_175_5:
;       [175:5] alias arg -> arr
;       [98:5] arg[1] = 0xfe
;       [98:5] allocate scratch register -> r15
;       [98:9] set array index
;       [98:9] 1
        mov r15, 1
;       [98:9] bounds check
;       [98:9] allocate scratch register -> r14
;       [98:9] line number
        mov r14, 98
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
;       [98:9] free scratch register r14
;       [98:14] instructions without scratch register 1, with 2
;       [98:14] 0xfe
        mov dword [rsp + r15 * 4 - 240], 254
;       [98:5] free scratch register r15
    faz_175_5_end:
;   [176:5] assert(arr[1] == 0xfe)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? arr[1] == 0xfe
;   [176:12] ? arr[1] == 0xfe
    cmp_176_12:
;   [176:12] allocate scratch register -> r14
;   [176:16] set array index
;   [176:16] 1
    mov r14, 1
;   [176:16] bounds check
;   [176:16] allocate scratch register -> r13
;   [176:16] line number
    mov r13, 176
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [176:16] free scratch register r13
    cmp dword [rsp + r14 * 4 - 240], 254
;   [176:12] free scratch register r14
    sete r15b
    bool_end_176_12:
;   [46:6] assert(x : bool)
    assert_176_5:
;       [176:5] alias x -> r15b
        if_46_26_176_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_176_5:
        cmp r15b, 0
        jne if_46_23_176_5_end
        if_46_26_176_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_176_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_176_5_end:
        if_46_23_176_5_end:
;       [176:5] free scratch register r15
    assert_176_5_end:
;   [178:5] var arr3 : i64[] = { 3, 5 }
;   [178:9] arr3: i64[2] (16 B @ [rsp - 304])
;   [178:9] arr3 = { 3, 5 }
;   [178:26] [0]
;   [178:26] instructions without scratch register 1, with 2
;   [178:26] 3
    mov qword [rsp - 304], 3
;   [178:26] [1]
;   [178:29] instructions without scratch register 1, with 2
;   [178:29] 5
    mov qword [rsp - 296], 5
;   [179:5] foo arr3
;   [179:9] allocate scratch register -> r15
;   [179:9] e: i64 (r15)
;   [179:9] i: i64 (8 B @ [rsp - 320])
;   [179:9] const n = 2
;   [179:9] initiate iterator e
    lea r15, [rsp - 304]
;   [179:9] initiate counter i
    mov qword [rsp - 320], 0
    foo_179_5:
;       [180:9] e = e + i + n
;       [180:13] instructions without scratch register 3, with 4
;       [180:13] e
;       [180:17] e + i
;       [180:17] allocate scratch register -> r14
        mov r14, qword [rsp - 320]
        add qword [r15], r14
;       [180:17] free scratch register r14
;       [180:21] e + n
        add qword [r15], 2
        foo_179_5_continue:
            add r15, 8
            inc qword [rsp - 320]
            cmp qword [rsp - 320], 2
            jne foo_179_5
    foo_179_5_end:
;   [179:5] free scratch register r15
;   [182:5] assert(arr3[0] == 3 + 0 + 2)
;   [182:12] allocate scratch register -> r15
;   [182:12] ? arr3[0] == 3 + 0 + 2
;   [182:12] ? arr3[0] == 3 + 0 + 2
    cmp_182_12:
;   [182:12] allocate scratch register -> r14
;   [182:17] set array index
;   [182:17] 0
    mov r14, 0
;   [182:17] bounds check
;   [182:17] allocate scratch register -> r13
;   [182:17] line number
    mov r13, 182
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [182:17] free scratch register r13
;   [182:23] allocate scratch register -> r13
;       [182:23] 3
        mov r13, 3
;       [182:27] r13 + 0
        add r13, 0
;       [182:31] r13 + 2
        add r13, 2
    cmp qword [rsp + r14 * 8 - 304], r13
;   [182:12] free scratch register r13
;   [182:12] free scratch register r14
    sete r15b
    bool_end_182_12:
;   [46:6] assert(x : bool)
    assert_182_5:
;       [182:5] alias x -> r15b
        if_46_26_182_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_182_5:
        cmp r15b, 0
        jne if_46_23_182_5_end
        if_46_26_182_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_182_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_182_5_end:
        if_46_23_182_5_end:
;       [182:5] free scratch register r15
    assert_182_5_end:
;   [183:5] assert(arr3[1] == 5 + 1 + 2)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? arr3[1] == 5 + 1 + 2
;   [183:12] ? arr3[1] == 5 + 1 + 2
    cmp_183_12:
;   [183:12] allocate scratch register -> r14
;   [183:17] set array index
;   [183:17] 1
    mov r14, 1
;   [183:17] bounds check
;   [183:17] allocate scratch register -> r13
;   [183:17] line number
    mov r13, 183
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [183:17] free scratch register r13
;   [183:23] allocate scratch register -> r13
;       [183:23] 5
        mov r13, 5
;       [183:27] r13 + 1
        add r13, 1
;       [183:31] r13 + 2
        add r13, 2
    cmp qword [rsp + r14 * 8 - 304], r13
;   [183:12] free scratch register r13
;   [183:12] free scratch register r14
    sete r15b
    bool_end_183_12:
;   [46:6] assert(x : bool)
    assert_183_5:
;       [183:5] alias x -> r15b
        if_46_26_183_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_183_5:
        cmp r15b, 0
        jne if_46_23_183_5_end
        if_46_26_183_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_183_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_183_5_end:
        if_46_23_183_5_end:
;       [183:5] free scratch register r15
    assert_183_5_end:
;   [184:5] # `foo` is a language construct that iterates over an array injecting:
;   [185:5] # `e`: current element
;   [186:5] # `i`: index starting at 0
;   [187:5] # `n`: constant array size
;   [189:5] var p : point = {0, 0}
;   [189:9] p: point (16 B @ [rsp - 320])
;   [189:9] p = {0, 0}
;   [189:22] copy field 'x'
    mov qword [rsp - 320], 0
;   [189:25] copy field 'y'
    mov qword [rsp - 312], 0
;   [190:5] fooz(p)
;   [71:6] fooz(pt : point)
    fooz_190_5:
;       [190:5] alias pt -> p
;       [72:5] pt.x = 0b10
;       [72:12] instructions without scratch register 1, with 2
;       [72:12] 0b10
        mov qword [rsp - 320], 2
;       [72:20] # binary value 2
;       [73:5] pt.y = 0xb
;       [73:12] instructions without scratch register 1, with 2
;       [73:12] 0xb
        mov qword [rsp - 312], 11
;       [73:20] # hex value 11
    fooz_190_5_end:
;   [191:5] assert(p.x == 2)
;   [191:12] allocate scratch register -> r15
;   [191:12] ? p.x == 2
;   [191:12] ? p.x == 2
    cmp_191_12:
    cmp qword [rsp - 320], 2
    sete r15b
    bool_end_191_12:
;   [46:6] assert(x : bool)
    assert_191_5:
;       [191:5] alias x -> r15b
        if_46_26_191_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_191_5:
        cmp r15b, 0
        jne if_46_23_191_5_end
        if_46_26_191_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_191_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_191_5_end:
        if_46_23_191_5_end:
;       [191:5] free scratch register r15
    assert_191_5_end:
;   [192:5] assert(p.y == 0xb)
;   [192:12] allocate scratch register -> r15
;   [192:12] ? p.y == 0xb
;   [192:12] ? p.y == 0xb
    cmp_192_12:
    cmp qword [rsp - 312], 11
    sete r15b
    bool_end_192_12:
;   [46:6] assert(x : bool)
    assert_192_5:
;       [192:5] alias x -> r15b
        if_46_26_192_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_192_5:
        cmp r15b, 0
        jne if_46_23_192_5_end
        if_46_26_192_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_192_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_192_5_end:
        if_46_23_192_5_end:
;       [192:5] free scratch register r15
    assert_192_5_end:
;   [194:5] var q : point = p
;   [194:9] q: point (16 B @ [rsp - 336])
;   [194:9] q = p
;   [194:21] size <= 16 B, use mov
;   [194:21] allocate named register rax
    mov rax, qword [rsp - 320]
    mov qword [rsp - 336], rax
    mov rax, qword [rsp - 312]
    mov qword [rsp - 328], rax
;   [194:21] free named register rax
;   [195:5] assert(equal(p, q))
;   [195:12] allocate scratch register -> r15
;   [195:12] ? equal(p, q)
;   [195:12] ? equal(p, q)
    cmp_195_12:
;   [195:12] allocate scratch register -> r14
;       [195:12] r14 = equal(p, q)
;       [195:12] = expression
;       [195:12] equal(p, q)
;       [195:12] allocate named register rsi
;       [195:12] allocate named register rdi
;       [195:12] allocate named register rcx
;       [195:18] p
        lea rsi, [rsp - 320]
;       [195:21] q
        lea rdi, [rsp - 336]
        mov rcx, 2
        repe cmpsq
;       [195:12] free named register rcx
;       [195:12] free named register rdi
;       [195:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [195:12] free scratch register r14
    setne r15b
    bool_end_195_12:
;   [46:6] assert(x : bool)
    assert_195_5:
;       [195:5] alias x -> r15b
        if_46_26_195_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_195_5:
        cmp r15b, 0
        jne if_46_23_195_5_end
        if_46_26_195_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_195_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_195_5_end:
        if_46_23_195_5_end:
;       [195:5] free scratch register r15
    assert_195_5_end:
;   [196:5] # `equal` is built-in function to compare user types for equality or same
;   [197:5] # size arrays
;   [199:5] q.x = 3
;   [199:11] instructions without scratch register 1, with 2
;   [199:11] 3
    mov qword [rsp - 336], 3
;   [200:5] assert(not equal(p, q))
;   [200:12] allocate scratch register -> r15
;   [200:12] ? not equal(p, q)
;   [200:12] ? not equal(p, q)
    cmp_200_12:
;   [200:16] allocate scratch register -> r14
;       [200:16] r14 = equal(p, q)
;       [200:16] = expression
;       [200:16] equal(p, q)
;       [200:16] allocate named register rsi
;       [200:16] allocate named register rdi
;       [200:16] allocate named register rcx
;       [200:22] p
        lea rsi, [rsp - 320]
;       [200:25] q
        lea rdi, [rsp - 336]
        mov rcx, 2
        repe cmpsq
;       [200:16] free named register rcx
;       [200:16] free named register rdi
;       [200:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [200:12] free scratch register r14
    sete r15b
    bool_end_200_12:
;   [46:6] assert(x : bool)
    assert_200_5:
;       [200:5] alias x -> r15b
        if_46_26_200_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_200_5:
        cmp r15b, 0
        jne if_46_23_200_5_end
        if_46_26_200_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_200_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_200_5_end:
        if_46_23_200_5_end:
;       [200:5] free scratch register r15
    assert_200_5_end:
;   [202:5] var i = 0
;   [202:9] i: i64 (8 B @ [rsp - 344])
;   [202:9] i = 0
;   [202:13] instructions without scratch register 1, with 2
;   [202:13] 0
    mov qword [rsp - 344], 0
;   [203:5] bar(i)
;   [78:6] bar(arg)
    bar_203_5:
;       [203:5] alias arg -> i
        if_79_8_203_5:
;       [79:8] ? arg == 0
;       [79:8] ? arg == 0
        cmp_79_8_203_5:
        cmp qword [rsp - 344], 0
        jne if_79_5_203_5_end
        if_79_8_203_5_code:
;           [79:17] return
            jmp bar_203_5_end
        if_79_5_203_5_end:
;       [80:5] arg = 0xff
;       [80:11] instructions without scratch register 1, with 2
;       [80:11] 0xff
        mov qword [rsp - 344], 255
    bar_203_5_end:
;   [204:5] assert(i == 0)
;   [204:12] allocate scratch register -> r15
;   [204:12] ? i == 0
;   [204:12] ? i == 0
    cmp_204_12:
    cmp qword [rsp - 344], 0
    sete r15b
    bool_end_204_12:
;   [46:6] assert(x : bool)
    assert_204_5:
;       [204:5] alias x -> r15b
        if_46_26_204_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_204_5:
        cmp r15b, 0
        jne if_46_23_204_5_end
        if_46_26_204_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_204_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_204_5_end:
        if_46_23_204_5_end:
;       [204:5] free scratch register r15
    assert_204_5_end:
;   [206:5] i = 1
;   [206:9] instructions without scratch register 1, with 2
;   [206:9] 1
    mov qword [rsp - 344], 1
;   [207:5] bar(i)
;   [78:6] bar(arg)
    bar_207_5:
;       [207:5] alias arg -> i
        if_79_8_207_5:
;       [79:8] ? arg == 0
;       [79:8] ? arg == 0
        cmp_79_8_207_5:
        cmp qword [rsp - 344], 0
        jne if_79_5_207_5_end
        if_79_8_207_5_code:
;           [79:17] return
            jmp bar_207_5_end
        if_79_5_207_5_end:
;       [80:5] arg = 0xff
;       [80:11] instructions without scratch register 1, with 2
;       [80:11] 0xff
        mov qword [rsp - 344], 255
    bar_207_5_end:
;   [208:5] assert(i == 0xff)
;   [208:12] allocate scratch register -> r15
;   [208:12] ? i == 0xff
;   [208:12] ? i == 0xff
    cmp_208_12:
    cmp qword [rsp - 344], 255
    sete r15b
    bool_end_208_12:
;   [46:6] assert(x : bool)
    assert_208_5:
;       [208:5] alias x -> r15b
        if_46_26_208_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_208_5:
        cmp r15b, 0
        jne if_46_23_208_5_end
        if_46_26_208_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_208_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_208_5_end:
        if_46_23_208_5_end:
;       [208:5] free scratch register r15
    assert_208_5_end:
;   [210:5] var j = 1
;   [210:9] j: i64 (8 B @ [rsp - 352])
;   [210:9] j = 1
;   [210:13] instructions without scratch register 1, with 2
;   [210:13] 1
    mov qword [rsp - 352], 1
;   [211:5] var k = baz(j)
;   [211:9] k: i64 (8 B @ [rsp - 360])
;   [211:9] k = baz(j)
;   [211:13] instructions without scratch register 5, with 5
;   [211:13] k = baz(j)
;   [211:13] = expression
;   [211:13] baz(j)
;   [91:6] baz(arg) : i64 res
    baz_211_13:
;       [211:13] alias res -> k
;       [211:13] alias arg -> j
;       [92:5] res = arg * 2
;       [92:11] instructions without scratch register 5, with 3
;       [92:11] allocate scratch register -> r15
;       [92:11] arg
        mov r15, qword [rsp - 352]
;       [92:17] r15 * 2
;       [92:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 360], r15
;       [92:11] free scratch register r15
    baz_211_13_end:
;   [212:5] assert(k == 2)
;   [212:12] allocate scratch register -> r15
;   [212:12] ? k == 2
;   [212:12] ? k == 2
    cmp_212_12:
    cmp qword [rsp - 360], 2
    sete r15b
    bool_end_212_12:
;   [46:6] assert(x : bool)
    assert_212_5:
;       [212:5] alias x -> r15b
        if_46_26_212_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_212_5:
        cmp r15b, 0
        jne if_46_23_212_5_end
        if_46_26_212_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_212_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_212_5_end:
        if_46_23_212_5_end:
;       [212:5] free scratch register r15
    assert_212_5_end:
;   [214:5] k = baz(1)
;   [214:9] instructions without scratch register 5, with 5
;   [214:9] k = baz(1)
;   [214:9] = expression
;   [214:9] baz(1)
;   [91:6] baz(arg) : i64 res
    baz_214_9:
;       [214:9] alias res -> k
;       [214:9] alias arg -> 1
;       [92:5] res = arg * 2
;       [92:11] instructions without scratch register 4, with 3
;       [92:11] allocate scratch register -> r15
;       [92:11] arg
        mov r15, 1
;       [92:17] r15 * 2
;       [92:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 360], r15
;       [92:11] free scratch register r15
    baz_214_9_end:
;   [215:5] assert(k == 2)
;   [215:12] allocate scratch register -> r15
;   [215:12] ? k == 2
;   [215:12] ? k == 2
    cmp_215_12:
    cmp qword [rsp - 360], 2
    sete r15b
    bool_end_215_12:
;   [46:6] assert(x : bool)
    assert_215_5:
;       [215:5] alias x -> r15b
        if_46_26_215_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_215_5:
        cmp r15b, 0
        jne if_46_23_215_5_end
        if_46_26_215_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_215_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_215_5_end:
        if_46_23_215_5_end:
;       [215:5] free scratch register r15
    assert_215_5_end:
;   [217:5] var p0 : point = {baz(3), 0}
;   [217:9] p0: point (16 B @ [rsp - 376])
;   [217:9] p0 = {baz(3), 0}
;   [217:23] copy field 'x'
;   [217:23] instructions without scratch register 5, with 5
;   [217:23] p0.x = baz(3)
;   [217:23] = expression
;   [217:23] baz(3)
;   [91:6] baz(arg) : i64 res
    baz_217_23:
;       [217:23] alias res -> p0.x (lea: rsp - 376)
;       [217:23] alias arg -> 3
;       [92:5] res = arg * 2
;       [92:11] instructions without scratch register 4, with 3
;       [92:11] allocate scratch register -> r15
;       [92:11] arg
        mov r15, 3
;       [92:17] r15 * 2
;       [92:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 376], r15
;       [92:11] free scratch register r15
    baz_217_23_end:
;   [217:31] copy field 'y'
    mov qword [rsp - 368], 0
;   [218:5] assert(p0.x == 6)
;   [218:12] allocate scratch register -> r15
;   [218:12] ? p0.x == 6
;   [218:12] ? p0.x == 6
    cmp_218_12:
    cmp qword [rsp - 376], 6
    sete r15b
    bool_end_218_12:
;   [46:6] assert(x : bool)
    assert_218_5:
;       [218:5] alias x -> r15b
        if_46_26_218_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_218_5:
        cmp r15b, 0
        jne if_46_23_218_5_end
        if_46_26_218_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_218_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_218_5_end:
        if_46_23_218_5_end:
;       [218:5] free scratch register r15
    assert_218_5_end:
;   [220:5] var pt : point = point_init()
;   [220:9] pt: point (16 B @ [rsp - 392])
;   [220:9] pt = point_init()
;   [220:22] point_init()
;   [118:6] point_init() : point res
    point_init_220_22:
;       [220:22] alias res -> pt
;       [119:5] res.x = -1
;       [119:13] instructions without scratch register 1, with 2
;       [119:14] -1
        mov qword [rsp - 392], -1
;       [120:5] res.y = -2
;       [120:13] instructions without scratch register 1, with 2
;       [120:14] -2
        mov qword [rsp - 384], -2
    point_init_220_22_end:
;   [221:5] assert(pt.x == -1)
;   [221:12] allocate scratch register -> r15
;   [221:12] ? pt.x == -1
;   [221:12] ? pt.x == -1
    cmp_221_12:
    cmp qword [rsp - 392], -1
    sete r15b
    bool_end_221_12:
;   [46:6] assert(x : bool)
    assert_221_5:
;       [221:5] alias x -> r15b
        if_46_26_221_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_221_5:
        cmp r15b, 0
        jne if_46_23_221_5_end
        if_46_26_221_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_221_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_221_5_end:
        if_46_23_221_5_end:
;       [221:5] free scratch register r15
    assert_221_5_end:
;   [222:5] assert(pt.y == -2)
;   [222:12] allocate scratch register -> r15
;   [222:12] ? pt.y == -2
;   [222:12] ? pt.y == -2
    cmp_222_12:
    cmp qword [rsp - 384], -2
    sete r15b
    bool_end_222_12:
;   [46:6] assert(x : bool)
    assert_222_5:
;       [222:5] alias x -> r15b
        if_46_26_222_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_222_5:
        cmp r15b, 0
        jne if_46_23_222_5_end
        if_46_26_222_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_222_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_222_5_end:
        if_46_23_222_5_end:
;       [222:5] free scratch register r15
    assert_222_5_end:
;   [224:5] var x = 1
;   [224:9] x: i64 (8 B @ [rsp - 400])
;   [224:9] x = 1
;   [224:13] instructions without scratch register 1, with 2
;   [224:13] 1
    mov qword [rsp - 400], 1
;   [225:5] var y = 2
;   [225:9] y: i64 (8 B @ [rsp - 408])
;   [225:9] y = 2
;   [225:13] instructions without scratch register 1, with 2
;   [225:13] 2
    mov qword [rsp - 408], 2
;   [227:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [227:9] o1: object (20 B @ [rsp - 428])
;   [227:9] o1 = {{x * 10, y}, 0xff0000}
;   [227:24] copy field 'pos'
;   [227:25] copy field 'x'
;   [227:25] instructions without scratch register 5, with 3
;   [227:25] allocate scratch register -> r15
;   [227:25] x
    mov r15, qword [rsp - 400]
;   [227:29] r15 * 10
;   [227:29] dst is reg, src is const
    imul r15, 10
    mov qword [rsp - 428], r15
;   [227:25] free scratch register r15
;   [227:33] copy field 'y'
;   [227:33] allocate scratch register -> r15
    mov r15, qword [rsp - 408]
    mov qword [rsp - 420], r15
;   [227:33] free scratch register r15
;   [227:37] copy field 'color'
    mov dword [rsp - 412], 16711680
;   [228:5] assert(o1.pos.x == 10)
;   [228:12] allocate scratch register -> r15
;   [228:12] ? o1.pos.x == 10
;   [228:12] ? o1.pos.x == 10
    cmp_228_12:
    cmp qword [rsp - 428], 10
    sete r15b
    bool_end_228_12:
;   [46:6] assert(x : bool)
    assert_228_5:
;       [228:5] alias x -> r15b
        if_46_26_228_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_228_5:
        cmp r15b, 0
        jne if_46_23_228_5_end
        if_46_26_228_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_228_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_228_5_end:
        if_46_23_228_5_end:
;       [228:5] free scratch register r15
    assert_228_5_end:
;   [229:5] assert(o1.pos.y == 2)
;   [229:12] allocate scratch register -> r15
;   [229:12] ? o1.pos.y == 2
;   [229:12] ? o1.pos.y == 2
    cmp_229_12:
    cmp qword [rsp - 420], 2
    sete r15b
    bool_end_229_12:
;   [46:6] assert(x : bool)
    assert_229_5:
;       [229:5] alias x -> r15b
        if_46_26_229_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_229_5:
        cmp r15b, 0
        jne if_46_23_229_5_end
        if_46_26_229_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_229_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_229_5_end:
        if_46_23_229_5_end:
;       [229:5] free scratch register r15
    assert_229_5_end:
;   [230:5] assert(o1.color == 0xff0000)
;   [230:12] allocate scratch register -> r15
;   [230:12] ? o1.color == 0xff0000
;   [230:12] ? o1.color == 0xff0000
    cmp_230_12:
    cmp dword [rsp - 412], 16711680
    sete r15b
    bool_end_230_12:
;   [46:6] assert(x : bool)
    assert_230_5:
;       [230:5] alias x -> r15b
        if_46_26_230_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_230_5:
        cmp r15b, 0
        jne if_46_23_230_5_end
        if_46_26_230_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_230_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_230_5_end:
        if_46_23_230_5_end:
;       [230:5] free scratch register r15
    assert_230_5_end:
;   [232:5] var p1 : point = {-x, -y}
;   [232:9] p1: point (16 B @ [rsp - 444])
;   [232:9] p1 = {-x, -y}
;   [232:23] copy field 'x'
;   [232:23] allocate scratch register -> r15
    mov r15, qword [rsp - 400]
    mov qword [rsp - 444], r15
;   [232:23] free scratch register r15
    neg qword [rsp - 444]
;   [232:27] copy field 'y'
;   [232:27] allocate scratch register -> r15
    mov r15, qword [rsp - 408]
    mov qword [rsp - 436], r15
;   [232:27] free scratch register r15
    neg qword [rsp - 436]
;   [233:5] o1.pos = p1
;   [233:14] size <= 16 B, use mov
;   [233:14] allocate named register rax
    mov rax, qword [rsp - 444]
    mov qword [rsp - 428], rax
    mov rax, qword [rsp - 436]
    mov qword [rsp - 420], rax
;   [233:14] free named register rax
;   [234:5] assert(o1.pos.x == -1)
;   [234:12] allocate scratch register -> r15
;   [234:12] ? o1.pos.x == -1
;   [234:12] ? o1.pos.x == -1
    cmp_234_12:
    cmp qword [rsp - 428], -1
    sete r15b
    bool_end_234_12:
;   [46:6] assert(x : bool)
    assert_234_5:
;       [234:5] alias x -> r15b
        if_46_26_234_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_234_5:
        cmp r15b, 0
        jne if_46_23_234_5_end
        if_46_26_234_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_234_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_234_5_end:
        if_46_23_234_5_end:
;       [234:5] free scratch register r15
    assert_234_5_end:
;   [235:5] assert(o1.pos.y == -2)
;   [235:12] allocate scratch register -> r15
;   [235:12] ? o1.pos.y == -2
;   [235:12] ? o1.pos.y == -2
    cmp_235_12:
    cmp qword [rsp - 420], -2
    sete r15b
    bool_end_235_12:
;   [46:6] assert(x : bool)
    assert_235_5:
;       [235:5] alias x -> r15b
        if_46_26_235_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_235_5:
        cmp r15b, 0
        jne if_46_23_235_5_end
        if_46_26_235_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_235_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_235_5_end:
        if_46_23_235_5_end:
;       [235:5] free scratch register r15
    assert_235_5_end:
;   [237:5] var o2 : object = o1
;   [237:9] o2: object (20 B @ [rsp - 464])
;   [237:9] o2 = o1
;   [237:23] allocate named register rsi
;   [237:23] allocate named register rdi
;   [237:23] allocate named register rcx
    lea rsi, [rsp - 428]
    lea rdi, [rsp - 464]
    mov rcx, 20
    rep movsb
;   [237:23] free named register rcx
;   [237:23] free named register rdi
;   [237:23] free named register rsi
;   [238:5] assert(o2.pos.x == -1)
;   [238:12] allocate scratch register -> r15
;   [238:12] ? o2.pos.x == -1
;   [238:12] ? o2.pos.x == -1
    cmp_238_12:
    cmp qword [rsp - 464], -1
    sete r15b
    bool_end_238_12:
;   [46:6] assert(x : bool)
    assert_238_5:
;       [238:5] alias x -> r15b
        if_46_26_238_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_238_5:
        cmp r15b, 0
        jne if_46_23_238_5_end
        if_46_26_238_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_238_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_238_5_end:
        if_46_23_238_5_end:
;       [238:5] free scratch register r15
    assert_238_5_end:
;   [239:5] assert(o2.pos.y == -2)
;   [239:12] allocate scratch register -> r15
;   [239:12] ? o2.pos.y == -2
;   [239:12] ? o2.pos.y == -2
    cmp_239_12:
    cmp qword [rsp - 456], -2
    sete r15b
    bool_end_239_12:
;   [46:6] assert(x : bool)
    assert_239_5:
;       [239:5] alias x -> r15b
        if_46_26_239_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_239_5:
        cmp r15b, 0
        jne if_46_23_239_5_end
        if_46_26_239_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_239_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_239_5_end:
        if_46_23_239_5_end:
;       [239:5] free scratch register r15
    assert_239_5_end:
;   [240:5] assert(o2.color == 0xff0000)
;   [240:12] allocate scratch register -> r15
;   [240:12] ? o2.color == 0xff0000
;   [240:12] ? o2.color == 0xff0000
    cmp_240_12:
    cmp dword [rsp - 448], 16711680
    sete r15b
    bool_end_240_12:
;   [46:6] assert(x : bool)
    assert_240_5:
;       [240:5] alias x -> r15b
        if_46_26_240_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_240_5:
        cmp r15b, 0
        jne if_46_23_240_5_end
        if_46_26_240_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_240_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_240_5_end:
        if_46_23_240_5_end:
;       [240:5] free scratch register r15
    assert_240_5_end:
;   [242:5] var o3 : object[2]
;   [242:9] o3: object[2] (40 B @ [rsp - 504])
;   [242:9] zero 2 * 20 B = 40 B
;   [242:5] allocate named register rax
;   [242:5] allocate named register rdi
;   [242:5] allocate named register rcx
    xor al, al
    lea rdi, [rsp - 504]
    mov rcx, 40
    rep stosb
;   [242:5] free named register rcx
;   [242:5] free named register rdi
;   [242:5] free named register rax
;   [243:5] o3.pos.y = 73
;   [243:16] instructions without scratch register 1, with 2
;   [243:16] 73
    mov qword [rsp - 496], 73
;   [244:5] # index 0 in an array can be accessed without array index
;   [246:5] assert(o3[0].pos.y == 73)
;   [246:12] allocate scratch register -> r15
;   [246:12] ? o3[0].pos.y == 73
;   [246:12] ? o3[0].pos.y == 73
    cmp_246_12:
;   [246:12] allocate scratch register -> r14
    lea r14, [rsp - 504]
;   [246:12] allocate scratch register -> r13
;   [246:15] set array index
;   [246:15] 0
    mov r13, 0
;   [246:15] bounds check
;   [246:15] allocate scratch register -> r12
;   [246:15] line number
    mov r12, 246
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
;   [246:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [246:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [246:12] free scratch register r14
    sete r15b
    bool_end_246_12:
;   [46:6] assert(x : bool)
    assert_246_5:
;       [246:5] alias x -> r15b
        if_46_26_246_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_246_5:
        cmp r15b, 0
        jne if_46_23_246_5_end
        if_46_26_246_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_246_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_246_5_end:
        if_46_23_246_5_end:
;       [246:5] free scratch register r15
    assert_246_5_end:
;   [248:5] o3[1] = object_init()
;   [248:5] allocate scratch register -> r15
    lea r15, [rsp - 504]
;   [248:5] allocate scratch register -> r14
;   [248:8] set array index
;   [248:8] 1
    mov r14, 1
;   [248:8] bounds check
;   [248:8] allocate scratch register -> r13
;   [248:8] line number
    mov r13, 248
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [248:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [248:5] free scratch register r14
;   [248:13] object_init()
;   [123:6] object_init() : object res
    object_init_248_13:
;       [248:13] alias res -> o3 (lea: r15)
;       [124:5] res.pos.y = 74
;       [124:17] instructions without scratch register 1, with 2
;       [124:17] 74
        mov qword [r15 + 8], 74
    object_init_248_13_end:
;   [248:5] free scratch register r15
;   [249:5] assert(o3[1].pos.y == 74)
;   [249:12] allocate scratch register -> r15
;   [249:12] ? o3[1].pos.y == 74
;   [249:12] ? o3[1].pos.y == 74
    cmp_249_12:
;   [249:12] allocate scratch register -> r14
    lea r14, [rsp - 504]
;   [249:12] allocate scratch register -> r13
;   [249:15] set array index
;   [249:15] 1
    mov r13, 1
;   [249:15] bounds check
;   [249:15] allocate scratch register -> r12
;   [249:15] line number
    mov r12, 249
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 2
    cmovge rbp, r12
    jge panic_bounds
;   [249:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [249:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [249:12] free scratch register r14
    sete r15b
    bool_end_249_12:
;   [46:6] assert(x : bool)
    assert_249_5:
;       [249:5] alias x -> r15b
        if_46_26_249_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_249_5:
        cmp r15b, 0
        jne if_46_23_249_5_end
        if_46_26_249_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_249_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_249_5_end:
        if_46_23_249_5_end:
;       [249:5] free scratch register r15
    assert_249_5_end:
;   [251:5] var worlds : world[8]
;   [251:9] worlds: world[8] (512 B @ [rsp - 1016])
;   [251:9] zero 8 * 64 B = 512 B
;   [251:5] allocate named register rax
;   [251:5] allocate named register rdi
;   [251:5] allocate named register rcx
    xor al, al
    lea rdi, [rsp - 1016]
    mov rcx, 512
    rep stosb
;   [251:5] free named register rcx
;   [251:5] free named register rdi
;   [251:5] free named register rax
;   [252:5] worlds[1].locations[1] = 0xffee
;   [252:5] allocate scratch register -> r15
    lea r15, [rsp - 1016]
;   [252:5] allocate scratch register -> r14
;   [252:12] set array index
;   [252:12] 1
    mov r14, 1
;   [252:12] bounds check
;   [252:12] allocate scratch register -> r13
;   [252:12] line number
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [252:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [252:5] free scratch register r14
;   [252:5] allocate scratch register -> r14
;   [252:25] set array index
;   [252:25] 1
    mov r14, 1
;   [252:25] bounds check
;   [252:25] allocate scratch register -> r13
;   [252:25] line number
    mov r13, 252
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [252:25] free scratch register r13
;   [252:30] instructions without scratch register 1, with 2
;   [252:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [252:5] free scratch register r14
;   [252:5] free scratch register r15
;   [253:5] assert(worlds[1].locations[1] == 0xffee)
;   [253:12] allocate scratch register -> r15
;   [253:12] ? worlds[1].locations[1] == 0xffee
;   [253:12] ? worlds[1].locations[1] == 0xffee
    cmp_253_12:
;   [253:12] allocate scratch register -> r14
    lea r14, [rsp - 1016]
;   [253:12] allocate scratch register -> r13
;   [253:19] set array index
;   [253:19] 1
    mov r13, 1
;   [253:19] bounds check
;   [253:19] allocate scratch register -> r12
;   [253:19] line number
    mov r12, 253
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [253:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [253:12] free scratch register r13
;   [253:12] allocate scratch register -> r13
;   [253:32] set array index
;   [253:32] 1
    mov r13, 1
;   [253:32] bounds check
;   [253:32] allocate scratch register -> r12
;   [253:32] line number
    mov r12, 253
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [253:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [253:12] free scratch register r13
;   [253:12] free scratch register r14
    sete r15b
    bool_end_253_12:
;   [46:6] assert(x : bool)
    assert_253_5:
;       [253:5] alias x -> r15b
        if_46_26_253_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_253_5:
        cmp r15b, 0
        jne if_46_23_253_5_end
        if_46_26_253_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_253_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_253_5_end:
        if_46_23_253_5_end:
;       [253:5] free scratch register r15
    assert_253_5_end:
;   [255:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [255:5] allocate named register rsi
;   [255:5] allocate named register rdi
;   [255:5] allocate named register rcx
;   [258:9] array_size_of(worlds.locations)
;   [258:9] rcx = array_size_of(worlds.locations)
;   [258:9] = expression
;   [258:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [256:9] worlds[1].locations
;   [256:9] allocate scratch register -> r15
    lea r15, [rsp - 1016]
;   [256:9] allocate scratch register -> r14
;   [256:16] set array index
;   [256:16] 1
    mov r14, 1
;   [256:16] bounds check
;   [256:16] allocate scratch register -> r13
;   [256:16] line number
    mov r13, 256
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [256:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [256:9] free scratch register r14
;   [256:9] bounds check
;   [256:9] allocate scratch register -> r14
;   [256:9] line number
    mov r14, 256
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [256:9] free scratch register r14
    lea rsi, [r15]
;   [255:5] free scratch register r15
;   [257:9] worlds[0].locations
;   [257:9] allocate scratch register -> r15
    lea r15, [rsp - 1016]
;   [257:9] allocate scratch register -> r14
;   [257:16] set array index
;   [257:16] 0
    mov r14, 0
;   [257:16] bounds check
;   [257:16] allocate scratch register -> r13
;   [257:16] line number
    mov r13, 257
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [257:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [257:9] free scratch register r14
;   [257:9] bounds check
;   [257:9] allocate scratch register -> r14
;   [257:9] line number
    mov r14, 257
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [257:9] free scratch register r14
    lea rdi, [r15]
;   [255:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [255:5] free named register rcx
;   [255:5] free named register rdi
;   [255:5] free named register rsi
;   [260:5] # `array_copy` is built-in and can use indexed positions
;   [261:5] # `array_size_of` is built-in
;   [263:5] assert(worlds[0].locations[1] == 0xffee)
;   [263:12] allocate scratch register -> r15
;   [263:12] ? worlds[0].locations[1] == 0xffee
;   [263:12] ? worlds[0].locations[1] == 0xffee
    cmp_263_12:
;   [263:12] allocate scratch register -> r14
    lea r14, [rsp - 1016]
;   [263:12] allocate scratch register -> r13
;   [263:19] set array index
;   [263:19] 0
    mov r13, 0
;   [263:19] bounds check
;   [263:19] allocate scratch register -> r12
;   [263:19] line number
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [263:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [263:12] free scratch register r13
;   [263:12] allocate scratch register -> r13
;   [263:32] set array index
;   [263:32] 1
    mov r13, 1
;   [263:32] bounds check
;   [263:32] allocate scratch register -> r12
;   [263:32] line number
    mov r12, 263
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [263:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [263:12] free scratch register r13
;   [263:12] free scratch register r14
    sete r15b
    bool_end_263_12:
;   [46:6] assert(x : bool)
    assert_263_5:
;       [263:5] alias x -> r15b
        if_46_26_263_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_263_5:
        cmp r15b, 0
        jne if_46_23_263_5_end
        if_46_26_263_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_263_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_263_5_end:
        if_46_23_263_5_end:
;       [263:5] free scratch register r15
    assert_263_5_end:
;   [264:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [264:12] allocate scratch register -> r15
;   [264:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [264:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_264_12:
;   [264:12] allocate scratch register -> r14
;       [264:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [264:12] = expression
;       [264:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [264:12] allocate named register rsi
;       [264:12] allocate named register rdi
;       [264:12] allocate named register rcx
;       [267:14] array_size_of(worlds.locations)
;       [267:14] rcx = array_size_of(worlds.locations)
;       [267:14] = expression
;       [267:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [265:14] worlds[0].locations
;       [265:14] allocate scratch register -> r13
        lea r13, [rsp - 1016]
;       [265:14] allocate scratch register -> r12
;       [265:21] set array index
;       [265:21] 0
        mov r12, 0
;       [265:21] bounds check
;       [265:21] allocate scratch register -> r11
;       [265:21] line number
        mov r11, 265
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [265:21] free scratch register r11
        shl r12, 6
        add r13, r12
;       [265:14] free scratch register r12
;       [265:14] bounds check
;       [265:14] allocate scratch register -> r12
;       [265:14] line number
        mov r12, 265
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [265:14] free scratch register r12
        lea rsi, [r13]
;       [264:12] free scratch register r13
;       [266:14] worlds[1].locations
;       [266:14] allocate scratch register -> r13
        lea r13, [rsp - 1016]
;       [266:14] allocate scratch register -> r12
;       [266:21] set array index
;       [266:21] 1
        mov r12, 1
;       [266:21] bounds check
;       [266:21] allocate scratch register -> r11
;       [266:21] line number
        mov r11, 266
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [266:21] free scratch register r11
        shl r12, 6
        add r13, r12
;       [266:14] free scratch register r12
;       [266:14] bounds check
;       [266:14] allocate scratch register -> r12
;       [266:14] line number
        mov r12, 266
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [266:14] free scratch register r12
        lea rdi, [r13]
;       [264:12] free scratch register r13
        shl rcx, 3
        repe cmpsb
;       [264:12] free named register rcx
;       [264:12] free named register rdi
;       [264:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [264:12] free scratch register r14
    setne r15b
    bool_end_264_12:
;   [46:6] assert(x : bool)
    assert_264_5:
;       [264:5] alias x -> r15b
        if_46_26_264_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_264_5:
        cmp r15b, 0
        jne if_46_23_264_5_end
        if_46_26_264_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_264_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_264_5_end:
        if_46_23_264_5_end:
;       [264:5] free scratch register r15
    assert_264_5_end:
;   [270:5] var arr2 : i64[] = { -1, 2 }
;   [270:9] arr2: i64[2] (16 B @ [rsp - 1032])
;   [270:9] arr2 = { -1, 2 }
;   [270:26] [0]
;   [270:26] instructions without scratch register 1, with 2
;   [270:27] -1
    mov qword [rsp - 1032], -1
;   [270:26] [1]
;   [270:30] instructions without scratch register 1, with 2
;   [270:30] 2
    mov qword [rsp - 1024], 2
;   [271:5] assert(array_size_of(arr2) == 2)
;   [271:12] allocate scratch register -> r15
;   [271:12] ? array_size_of(arr2) == 2
;   [271:12] ? array_size_of(arr2) == 2
    cmp_271_12:
;   [271:12] allocate scratch register -> r14
;       [271:12] r14 = array_size_of(arr2)
;       [271:12] = expression
;       [271:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
;   [271:12] free scratch register r14
    sete r15b
    bool_end_271_12:
;   [46:6] assert(x : bool)
    assert_271_5:
;       [271:5] alias x -> r15b
        if_46_26_271_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_271_5:
        cmp r15b, 0
        jne if_46_23_271_5_end
        if_46_26_271_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_271_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_271_5_end:
        if_46_23_271_5_end:
;       [271:5] free scratch register r15
    assert_271_5_end:
;   [272:5] assert(arr2[0] == -1)
;   [272:12] allocate scratch register -> r15
;   [272:12] ? arr2[0] == -1
;   [272:12] ? arr2[0] == -1
    cmp_272_12:
;   [272:12] allocate scratch register -> r14
;   [272:17] set array index
;   [272:17] 0
    mov r14, 0
;   [272:17] bounds check
;   [272:17] allocate scratch register -> r13
;   [272:17] line number
    mov r13, 272
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [272:17] free scratch register r13
    cmp qword [rsp + r14 * 8 - 1032], -1
;   [272:12] free scratch register r14
    sete r15b
    bool_end_272_12:
;   [46:6] assert(x : bool)
    assert_272_5:
;       [272:5] alias x -> r15b
        if_46_26_272_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_272_5:
        cmp r15b, 0
        jne if_46_23_272_5_end
        if_46_26_272_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_272_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_272_5_end:
        if_46_23_272_5_end:
;       [272:5] free scratch register r15
    assert_272_5_end:
;   [273:5] assert(arr2[1] == 2)
;   [273:12] allocate scratch register -> r15
;   [273:12] ? arr2[1] == 2
;   [273:12] ? arr2[1] == 2
    cmp_273_12:
;   [273:12] allocate scratch register -> r14
;   [273:17] set array index
;   [273:17] 1
    mov r14, 1
;   [273:17] bounds check
;   [273:17] allocate scratch register -> r13
;   [273:17] line number
    mov r13, 273
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 2
    cmovge rbp, r13
    jge panic_bounds
;   [273:17] free scratch register r13
    cmp qword [rsp + r14 * 8 - 1032], 2
;   [273:12] free scratch register r14
    sete r15b
    bool_end_273_12:
;   [46:6] assert(x : bool)
    assert_273_5:
;       [273:5] alias x -> r15b
        if_46_26_273_5:
;       [46:26] ? not x
;       [46:26] ? not x
        cmp_46_26_273_5:
        cmp r15b, 0
        jne if_46_23_273_5_end
        if_46_26_273_5_code:
;           [46:32] exit(1)
;           [46:37] allocate named register rdi
            mov rdi, 1
;           [38:6] exit(v : reg_rdi)
            exit_46_32_273_5:
;               [46:32] alias v -> rdi
;               [39:5] mov(rax, 60)
;                   [39:14] 60
                    mov rax, 60
;               [39:19] # exit system call
;               [40:5] mov(rdi, v)
;                   [40:14] v
;               [40:19] # return code
;               [41:5] syscall()
                syscall
;               [46:32] free named register rdi
            exit_46_32_273_5_end:
        if_46_23_273_5_end:
;       [273:5] free scratch register r15
    assert_273_5_end:
;   [275:5] var nm : str
;   [275:9] nm: str (128 B @ [rsp - 1160])
;   [275:9] zero 1 * 128 B = 128 B
;   [275:5] allocate named register rax
;   [275:5] allocate named register rdi
;   [275:5] allocate named register rcx
    xor al, al
    lea rdi, [rsp - 1160]
    mov rcx, 128
    rep stosb
;   [275:5] free named register rcx
;   [275:5] free named register rdi
;   [275:5] free named register rax
;   [276:5] print(hello)
;   [56:6] print(str : i8[])
    print_276_5:
;       [276:5] alias str -> hello
;       [57:4] sys_print(array_size_of(str), address_of(str))
;       [57:14] allocate named register rdx
;       [57:14] rdx = array_size_of(str)
;       [57:14] = expression
;       [57:14] array_size_of(str)
        mov rdx, 21
;       [57:34] allocate named register rsi
;       [57:34] rsi = address_of(str)
;       [57:34] = expression
;       [57:34] address_of(str)
        lea rsi, [rsp - 21]
;       [48:6] sys_print(len : reg_rdx, ptr : reg_rsi)
        sys_print_57_4_276_5:
;           [57:4] alias len -> rdx
;           [57:4] alias ptr -> rsi
;           [49:5] mov(rax, 1)
;               [49:14] 1
                mov rax, 1
;           [49:19] # write system call
;           [50:5] mov(rdi, 0)
;               [50:14] 0
                mov rdi, 0
;           [50:19] # file descriptor for standard out
;           [51:5] mov(rsi, ptr)
;               [51:14] ptr
;           [51:19] # buffer address
;           [52:5] mov(rdx, len)
;               [52:14] len
;           [52:19] # buffer size
;           [53:5] syscall()
            syscall
;           [57:4] free named register rsi
;           [57:4] free named register rdx
        sys_print_57_4_276_5_end:
    print_276_5_end:
;   [277:5] label
    loop_277_5:
;       [278:9] print(prompt1)
;       [56:6] print(str : i8[])
        print_278_9:
;           [278:9] alias str -> prompt1
;           [57:4] sys_print(array_size_of(str), address_of(str))
;           [57:14] allocate named register rdx
;           [57:14] rdx = array_size_of(str)
;           [57:14] = expression
;           [57:14] array_size_of(str)
            mov rdx, 12
;           [57:34] allocate named register rsi
;           [57:34] rsi = address_of(str)
;           [57:34] = expression
;           [57:34] address_of(str)
            lea rsi, [rsp - 33]
;           [48:6] sys_print(len : reg_rdx, ptr : reg_rsi)
            sys_print_57_4_278_9:
;               [57:4] alias len -> rdx
;               [57:4] alias ptr -> rsi
;               [49:5] mov(rax, 1)
;                   [49:14] 1
                    mov rax, 1
;               [49:19] # write system call
;               [50:5] mov(rdi, 0)
;                   [50:14] 0
                    mov rdi, 0
;               [50:19] # file descriptor for standard out
;               [51:5] mov(rsi, ptr)
;                   [51:14] ptr
;               [51:19] # buffer address
;               [52:5] mov(rdx, len)
;                   [52:14] len
;               [52:19] # buffer size
;               [53:5] syscall()
                syscall
;               [57:4] free named register rsi
;               [57:4] free named register rdx
            sys_print_57_4_278_9_end:
        print_278_9_end:
;       [279:9] str_in(nm)
;       [101:6] str_in(s : str)
        str_in_279_9:
;           [279:9] alias s -> nm
;           [102:5] mov(rax, 0)
;               [102:14] 0
                mov rax, 0
;           [102:37] # read system call
;           [103:5] mov(rdi, 0)
;               [103:14] 0
                mov rdi, 0
;           [103:37] # file descriptor for standard input
;           [104:5] mov(rsi, address_of(s.data))
;               [104:14] rsi = address_of(s.data)
;               [104:14] = expression
;               [104:14] address_of(s.data)
                lea rsi, [rsp - 1159]
;           [104:37] # buffer address
;           [105:5] mov(rdx, array_size_of(s.data))
;               [105:14] rdx = array_size_of(s.data)
;               [105:14] = expression
;               [105:14] array_size_of(s.data)
                mov rdx, 127
;           [105:37] # buffer size
;           [106:5] syscall()
            syscall
;           [107:5] mov(s.len, rax - 1)
;               [107:16] instructions without scratch register 2, with 3
;               [107:16] rax
                mov byte [rsp - 1160], al
;               [107:22] s.len - 1
                sub byte [rsp - 1160], 1
;           [107:25] # return value
        str_in_279_9_end:
        if_280_12:
;       [280:12] ? nm.len == 0
;       [280:12] ? nm.len == 0
        cmp_280_12:
        cmp byte [rsp - 1160], 0
        jne if_282_19
        if_280_12_code:
;           [281:13] break
            jmp loop_277_5_end
        jmp if_280_9_end
        if_282_19:
;       [282:19] ? nm.len <= 4
;       [282:19] ? nm.len <= 4
        cmp_282_19:
        cmp byte [rsp - 1160], 4
        jg if_else_280_9
        if_282_19_code:
;           [283:13] print(prompt2)
;           [56:6] print(str : i8[])
            print_283_13:
;               [283:13] alias str -> prompt2
;               [57:4] sys_print(array_size_of(str), address_of(str))
;               [57:14] allocate named register rdx
;               [57:14] rdx = array_size_of(str)
;               [57:14] = expression
;               [57:14] array_size_of(str)
                mov rdx, 20
;               [57:34] allocate named register rsi
;               [57:34] rsi = address_of(str)
;               [57:34] = expression
;               [57:34] address_of(str)
                lea rsi, [rsp - 53]
;               [48:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_57_4_283_13:
;                   [57:4] alias len -> rdx
;                   [57:4] alias ptr -> rsi
;                   [49:5] mov(rax, 1)
;                       [49:14] 1
                        mov rax, 1
;                   [49:19] # write system call
;                   [50:5] mov(rdi, 0)
;                       [50:14] 0
                        mov rdi, 0
;                   [50:19] # file descriptor for standard out
;                   [51:5] mov(rsi, ptr)
;                       [51:14] ptr
;                   [51:19] # buffer address
;                   [52:5] mov(rdx, len)
;                       [52:14] len
;                   [52:19] # buffer size
;                   [53:5] syscall()
                    syscall
;                   [57:4] free named register rsi
;                   [57:4] free named register rdx
                sys_print_57_4_283_13_end:
            print_283_13_end:
;           [284:13] continue
            jmp loop_277_5
        jmp if_280_9_end
        if_else_280_9:
;           [286:13] print(prompt3)
;           [56:6] print(str : i8[])
            print_286_13:
;               [286:13] alias str -> prompt3
;               [57:4] sys_print(array_size_of(str), address_of(str))
;               [57:14] allocate named register rdx
;               [57:14] rdx = array_size_of(str)
;               [57:14] = expression
;               [57:14] array_size_of(str)
                mov rdx, 6
;               [57:34] allocate named register rsi
;               [57:34] rsi = address_of(str)
;               [57:34] = expression
;               [57:34] address_of(str)
                lea rsi, [rsp - 59]
;               [48:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_57_4_286_13:
;                   [57:4] alias len -> rdx
;                   [57:4] alias ptr -> rsi
;                   [49:5] mov(rax, 1)
;                       [49:14] 1
                        mov rax, 1
;                   [49:19] # write system call
;                   [50:5] mov(rdi, 0)
;                       [50:14] 0
                        mov rdi, 0
;                   [50:19] # file descriptor for standard out
;                   [51:5] mov(rsi, ptr)
;                       [51:14] ptr
;                   [51:19] # buffer address
;                   [52:5] mov(rdx, len)
;                       [52:14] len
;                   [52:19] # buffer size
;                   [53:5] syscall()
                    syscall
;                   [57:4] free named register rsi
;                   [57:4] free named register rdx
                sys_print_57_4_286_13_end:
            print_286_13_end:
;           [287:13] str_out(nm)
;           [110:6] str_out(s : str)
            str_out_287_13:
;               [287:13] alias s -> nm
;               [111:5] mov(rax, 1)
;                   [111:14] 1
                    mov rax, 1
;               [111:34] # write system call
;               [112:5] mov(rdi, 0)
;                   [112:14] 0
                    mov rdi, 0
;               [112:34] # file descriptor for standard out
;               [113:5] mov(rsi, address_of(s.data))
;                   [113:14] rsi = address_of(s.data)
;                   [113:14] = expression
;                   [113:14] address_of(s.data)
                    lea rsi, [rsp - 1159]
;               [113:34] # buffer address
;               [114:5] mov(rdx, s.len)
;                   [114:14] s.len
                    movsx rdx, byte [rsp - 1160]
;               [114:34] # buffer size
;               [115:5] syscall()
                syscall
            str_out_287_13_end:
;           [288:13] print(dot)
;           [56:6] print(str : i8[])
            print_288_13:
;               [288:13] alias str -> dot
;               [57:4] sys_print(array_size_of(str), address_of(str))
;               [57:14] allocate named register rdx
;               [57:14] rdx = array_size_of(str)
;               [57:14] = expression
;               [57:14] array_size_of(str)
                mov rdx, 1
;               [57:34] allocate named register rsi
;               [57:34] rsi = address_of(str)
;               [57:34] = expression
;               [57:34] address_of(str)
                lea rsi, [rsp - 60]
;               [48:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_57_4_288_13:
;                   [57:4] alias len -> rdx
;                   [57:4] alias ptr -> rsi
;                   [49:5] mov(rax, 1)
;                       [49:14] 1
                        mov rax, 1
;                   [49:19] # write system call
;                   [50:5] mov(rdi, 0)
;                       [50:14] 0
                        mov rdi, 0
;                   [50:19] # file descriptor for standard out
;                   [51:5] mov(rsi, ptr)
;                       [51:14] ptr
;                   [51:19] # buffer address
;                   [52:5] mov(rdx, len)
;                       [52:14] len
;                   [52:19] # buffer size
;                   [53:5] syscall()
                    syscall
;                   [57:4] free named register rsi
;                   [57:4] free named register rdx
                sys_print_57_4_288_13_end:
            print_288_13_end:
;           [289:13] print(nl)
;           [56:6] print(str : i8[])
            print_289_13:
;               [289:13] alias str -> nl
;               [57:4] sys_print(array_size_of(str), address_of(str))
;               [57:14] allocate named register rdx
;               [57:14] rdx = array_size_of(str)
;               [57:14] = expression
;               [57:14] array_size_of(str)
                mov rdx, 1
;               [57:34] allocate named register rsi
;               [57:34] rsi = address_of(str)
;               [57:34] = expression
;               [57:34] address_of(str)
                lea rsi, [rsp - 61]
;               [48:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_57_4_289_13:
;                   [57:4] alias len -> rdx
;                   [57:4] alias ptr -> rsi
;                   [49:5] mov(rax, 1)
;                       [49:14] 1
                        mov rax, 1
;                   [49:19] # write system call
;                   [50:5] mov(rdi, 0)
;                       [50:14] 0
                        mov rdi, 0
;                   [50:19] # file descriptor for standard out
;                   [51:5] mov(rsi, ptr)
;                       [51:14] ptr
;                   [51:19] # buffer address
;                   [52:5] mov(rdx, len)
;                       [52:14] len
;                   [52:19] # buffer size
;                   [53:5] syscall()
                    syscall
;                   [57:4] free named register rsi
;                   [57:4] free named register rdx
                sys_print_57_4_289_13_end:
            print_289_13_end:
        if_280_9_end:
    jmp loop_277_5
    loop_277_5_end:
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

section .data
align 16
dat:
;[29:5] padding
;[0:1] i8[3]
;[0:1] pad 3 'i8' of size 1
times 3 db 0
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
dat.end:

; max scratch registers in use: 5
;            max frames in use: 10
;               max stack size: 1160 B
;          optimization pass 1: 120
;          optimization pass 2: 0
```
