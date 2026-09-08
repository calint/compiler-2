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
C/C++ Header                    43           1490            776           7109
C++                              1             85            107            559
-------------------------------------------------------------------------------
SUM:                            44           1575            883           7668
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

# constants are declared in global scope

const yes = 1
const no = 0
const maybe = -1

func main() {
    # initial data declared first in main and are initialized with constants
    dat   hello : i8[] = "hello world from baz\n"
    dat prompt1 : i8[] = "enter name:\n"
    dat prompt2 : i8[] = "that is not a name.\n"
    dat prompt3 : i8[] = "hello "
    dat     dot : i8[] = "."
    dat      nl : i8[] = "\n"


    var arr : i32[4]
    # arrays are initialized to 0

    var answer
    # variables without initializer are zeroed
    assert(answer == 0)

    answer = maybe
    assert(answer == -1)

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
    mov qword [rsp - 85], 0
    cmp_119_12:
    cmp qword [rsp - 85], 0
    sete r15b
    bool_end_119_12:
    assert_119_5:
        if_13_29_119_5:
        cmp_13_29_119_5:
        cmp r15b, 0
        jne if_13_26_119_5_end
        if_13_29_119_5_code:
            mov rdi, 1
            exit_13_38_119_5:
                    mov rax, 60
                syscall
            exit_13_38_119_5_end:
        if_13_26_119_5_end:
    assert_119_5_end:
    mov qword [rsp - 85], -1
    cmp_122_12:
    cmp qword [rsp - 85], -1
    sete r15b
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
    mov qword [rsp - 93], 1
    mov r15, qword [rsp - 93]
    mov r14, 126
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 77], 2
    mov r15, qword [rsp - 93]
    add r15, 1
    mov r14, 127
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    mov r13, 127
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    mov r13d, dword [rsp + r14 * 4 - 77]
    mov dword [rsp + r15 * 4 - 77], r13d
    cmp_128_12:
    mov r14, 1
    mov r13, 128
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_128_12:
    assert_128_5:
        if_13_29_128_5:
        cmp_13_29_128_5:
        cmp r15b, 0
        jne if_13_26_128_5_end
        if_13_29_128_5_code:
            mov rdi, 1
            exit_13_38_128_5:
                    mov rax, 60
                syscall
            exit_13_38_128_5_end:
        if_13_26_128_5_end:
    assert_128_5_end:
    cmp_129_12:
    mov r14, 2
    mov r13, 129
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
    sete r15b
    bool_end_129_12:
    assert_129_5:
        if_13_29_129_5:
        cmp_13_29_129_5:
        cmp r15b, 0
        jne if_13_26_129_5_end
        if_13_29_129_5_code:
            mov rdi, 1
            exit_13_38_129_5:
                    mov rax, 60
                syscall
            exit_13_38_129_5_end:
        if_13_26_129_5_end:
    assert_129_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 131
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [rsp + r15 * 4 - 77]
    mov r15, 131
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 77]
    shl rcx, 2
    rep movsb
    cmp_133_12:
    mov r14, 0
    mov r13, 133
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
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
    mov qword [rsp - 125], 0
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
    mov rcx, 4
    mov r15, 136
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
    lea rsi, [rsp - 77]
    mov r15, 136
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
    lea rdi, [rsp - 125]
    shl rcx, 2
    rep movsb
    cmp_137_12:
        mov rcx, 4
        mov r13, 137
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 137
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov r15, 2
    mov r14, 140
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
    mov dword [rsp + r15 * 4 - 125], -1
    cmp_141_12:
        mov rcx, 4
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
        lea rsi, [rsp - 77]
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14, 0
    sete r15b
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
    mov qword [rsp - 93], 3
    mov r15, qword [rsp - 93]
    mov r14, 144
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
    mov r14, qword [rsp - 93]
    sub r14, 1
    mov r13, 144
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    inv_144_16:
        mov r13d, dword [rsp + r14 * 4 - 77]
        mov dword [rsp + r15 * 4 - 77], r13d
        not dword [rsp + r15 * 4 - 77]
    inv_144_16_end:
    not dword [rsp + r15 * 4 - 77]
    cmp_145_12:
    mov r14, qword [rsp - 93]
    mov r13, 145
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 2
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
    faz_147_5:
        mov r15, 1
        mov r14, 73
        test r15, r15
        cmovs rbp, r14
        js panic_bounds
        cmp r15, 4
        cmovge rbp, r14
        jge panic_bounds
        mov dword [rsp + r15 * 4 - 77], 254
    faz_147_5_end:
    cmp_148_12:
    mov r14, 1
    mov r13, 148
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
    cmp dword [rsp + r14 * 4 - 77], 254
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
    mov qword [rsp - 141], 0
    mov qword [rsp - 133], 0
    foo_151_5:
        mov qword [rsp - 141], 2
        mov qword [rsp - 133], 11
    foo_151_5_end:
    cmp_152_12:
    cmp qword [rsp - 141], 2
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
    cmp_153_12:
    cmp qword [rsp - 133], 11
    sete r15b
    bool_end_153_12:
    assert_153_5:
        if_13_29_153_5:
        cmp_13_29_153_5:
        cmp r15b, 0
        jne if_13_26_153_5_end
        if_13_29_153_5_code:
            mov rdi, 1
            exit_13_38_153_5:
                    mov rax, 60
                syscall
            exit_13_38_153_5_end:
        if_13_26_153_5_end:
    assert_153_5_end:
    lea rsi, [rsp - 141]
    lea rdi, [rsp - 157]
    mov rcx, 2
    rep movsq
    cmp_156_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
    setne r15b
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
    mov qword [rsp - 157], 3
    cmp_161_12:
        lea rsi, [rsp - 141]
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14, 0
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
    mov qword [rsp - 165], 0
    bar_164_5:
        if_56_8_164_5:
        cmp_56_8_164_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_164_5_end
        if_56_8_164_5_code:
            jmp bar_164_5_end
        if_56_5_164_5_end:
        mov qword [rsp - 165], 255
    bar_164_5_end:
    cmp_165_12:
    cmp qword [rsp - 165], 0
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
    mov qword [rsp - 165], 1
    bar_168_5:
        if_56_8_168_5:
        cmp_56_8_168_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_168_5_end
        if_56_8_168_5_code:
            jmp bar_168_5_end
        if_56_5_168_5_end:
        mov qword [rsp - 165], 255
    bar_168_5_end:
    cmp_169_12:
    cmp qword [rsp - 165], 255
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
    mov qword [rsp - 173], 1
    baz_172_13:
        mov r15, qword [rsp - 173]
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_172_13_end:
    cmp_173_12:
    cmp qword [rsp - 181], 2
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
    baz_175_9:
        mov r15, 1
        imul r15, 2
        mov qword [rsp - 181], r15
    baz_175_9_end:
    cmp_176_12:
    cmp qword [rsp - 181], 2
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
    baz_178_23:
        mov r15, 2
        imul r15, 2
        mov qword [rsp - 197], r15
    baz_178_23_end:
    mov qword [rsp - 189], 0
    cmp_179_12:
    cmp qword [rsp - 197], 4
    sete r15b
    bool_end_179_12:
    assert_179_5:
        if_13_29_179_5:
        cmp_13_29_179_5:
        cmp r15b, 0
        jne if_13_26_179_5_end
        if_13_29_179_5_code:
            mov rdi, 1
            exit_13_38_179_5:
                    mov rax, 60
                syscall
            exit_13_38_179_5_end:
        if_13_26_179_5_end:
    assert_179_5_end:
    mov qword [rsp - 205], 1
    mov qword [rsp - 213], 2
    mov r15, qword [rsp - 205]
    imul r15, 10
    mov qword [rsp - 233], r15
    mov r15, qword [rsp - 213]
    mov qword [rsp - 225], r15
    mov dword [rsp - 217], 16711680
    cmp_185_12:
    cmp qword [rsp - 233], 10
    sete r15b
    bool_end_185_12:
    assert_185_5:
        if_13_29_185_5:
        cmp_13_29_185_5:
        cmp r15b, 0
        jne if_13_26_185_5_end
        if_13_29_185_5_code:
            mov rdi, 1
            exit_13_38_185_5:
                    mov rax, 60
                syscall
            exit_13_38_185_5_end:
        if_13_26_185_5_end:
    assert_185_5_end:
    cmp_186_12:
    cmp qword [rsp - 225], 2
    sete r15b
    bool_end_186_12:
    assert_186_5:
        if_13_29_186_5:
        cmp_13_29_186_5:
        cmp r15b, 0
        jne if_13_26_186_5_end
        if_13_29_186_5_code:
            mov rdi, 1
            exit_13_38_186_5:
                    mov rax, 60
                syscall
            exit_13_38_186_5_end:
        if_13_26_186_5_end:
    assert_186_5_end:
    cmp_187_12:
    cmp dword [rsp - 217], 16711680
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
    mov r15, qword [rsp - 205]
    mov qword [rsp - 249], r15
    neg qword [rsp - 249]
    mov r15, qword [rsp - 213]
    mov qword [rsp - 241], r15
    neg qword [rsp - 241]
    lea rsi, [rsp - 249]
    lea rdi, [rsp - 233]
    mov rcx, 2
    rep movsq
    cmp_191_12:
    cmp qword [rsp - 233], -1
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
    cmp_192_12:
    cmp qword [rsp - 225], -2
    sete r15b
    bool_end_192_12:
    assert_192_5:
        if_13_29_192_5:
        cmp_13_29_192_5:
        cmp r15b, 0
        jne if_13_26_192_5_end
        if_13_29_192_5_code:
            mov rdi, 1
            exit_13_38_192_5:
                    mov rax, 60
                syscall
            exit_13_38_192_5_end:
        if_13_26_192_5_end:
    assert_192_5_end:
    lea rsi, [rsp - 233]
    lea rdi, [rsp - 269]
    mov rcx, 20
    rep movsb
    cmp_195_12:
    cmp qword [rsp - 269], -1
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
    cmp qword [rsp - 261], -2
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
    cmp_197_12:
    cmp dword [rsp - 253], 16711680
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
    mov qword [rsp - 289], 0
    mov qword [rsp - 281], 0
    mov dword [rsp - 273], 0
    mov qword [rsp - 281], 73
    cmp_202_12:
    lea r14, [rsp - 289]
    mov r13, 0
    mov r12, 202
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
    bool_end_202_12:
    assert_202_5:
        if_13_29_202_5:
        cmp_13_29_202_5:
        cmp r15b, 0
        jne if_13_26_202_5_end
        if_13_29_202_5_code:
            mov rdi, 1
            exit_13_38_202_5:
                    mov rax, 60
                syscall
            exit_13_38_202_5_end:
        if_13_26_202_5_end:
    assert_202_5_end:
    mov rcx, 512
    lea rdi, [rsp - 801]
    xor rax, rax
    rep stosb
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 205
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 205
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    mov qword [r15 + r14 * 8], 65518
    cmp_206_12:
    lea r14, [rsp - 801]
    mov r13, 1
    mov r12, 206
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 206
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
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
    mov rcx, 8
    lea r15, [rsp - 801]
    mov r14, 1
    mov r13, 209
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 209
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rsi, [r15]
    lea r15, [rsp - 801]
    mov r14, 0
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
    shl r14, 6
    add r15, r14
    mov r14, 210
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_215_12:
    lea r14, [rsp - 801]
    mov r13, 0
    mov r12, 215
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 215
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
    cmp qword [r14 + r13 * 8], 65518
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
        mov rcx, 8
        lea r13, [rsp - 801]
        mov r12, 0
        mov r11, 217
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 217
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
        lea rsi, [r13]
        lea r13, [rsp - 801]
        mov r12, 1
        mov r11, 218
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
        shl r12, 6
        add r13, r12
        mov r12, 218
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
    mov rcx, 128
    lea rdi, [rsp - 929]
    xor rax, rax
    rep stosb
    print_str_223_5:
        mov rdx, 21
        lea rsi, [rsp - 21]
        print_24_4_223_5:
                mov rax, 1
                mov rdi, 0
            syscall
        print_24_4_223_5_end:
    print_str_223_5_end:
    loop_224_5:
        print_str_225_9:
            mov rdx, 12
            lea rsi, [rsp - 33]
            print_24_4_225_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            print_24_4_225_9_end:
        print_str_225_9_end:
        str_in_226_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rsp - 928]
                mov rdx, 127
            syscall
                mov byte [rsp - 929], al
                sub byte [rsp - 929], 1
        str_in_226_9_end:
        if_227_12:
        cmp_227_12:
        cmp byte [rsp - 929], 0
        jne if_229_19
        if_227_12_code:
            jmp loop_224_5_end
        jmp if_227_9_end
        if_229_19:
        cmp_229_19:
        cmp byte [rsp - 929], 4
        jg if_else_227_9
        if_229_19_code:
            print_str_230_13:
                mov rdx, 20
                lea rsi, [rsp - 53]
                print_24_4_230_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_230_13_end:
            print_str_230_13_end:
            jmp loop_224_5
        jmp if_227_9_end
        if_else_227_9:
            print_str_233_13:
                mov rdx, 6
                lea rsi, [rsp - 59]
                print_24_4_233_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_233_13_end:
            print_str_233_13_end:
            str_out_234_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rsp - 928]
                    movsx rdx, byte [rsp - 929]
                syscall
            str_out_234_13_end:
            print_str_235_13:
                mov rdx, 1
                lea rsi, [rsp - 60]
                print_24_4_235_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_235_13_end:
            print_str_235_13_end:
            print_str_236_13:
                mov rdx, 1
                lea rsi, [rsp - 61]
                print_24_4_236_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                print_24_4_236_13_end:
            print_str_236_13_end:
        if_227_9_end:
    jmp loop_224_5
    loop_224_5_end:
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

;[98:1] # constants are declared in global scope
main:
;   [105:5] # initial data declared first in main and are initialized with constants
;   [106:5] dat hello : i8[] = "hello world from baz\n"
;   [106:11] hello: i8[21] (21B @ [rsp - 21])
;   [107:5] dat prompt1 : i8[] = "enter name:\n"
;   [107:9] prompt1: i8[12] (12B @ [rsp - 33])
;   [108:5] dat prompt2 : i8[] = "that is not a name.\n"
;   [108:9] prompt2: i8[20] (20B @ [rsp - 53])
;   [109:5] dat prompt3 : i8[] = "hello "
;   [109:9] prompt3: i8[6] (6B @ [rsp - 59])
;   [110:5] dat dot : i8[] = "."
;   [110:13] dot: i8[1] (1B @ [rsp - 60])
;   [111:5] dat nl : i8[] = "\n"
;   [111:14] nl: i8[1] (1B @ [rsp - 61])
;   [114:5] var arr : i32[4]
;   [114:9] arr: i32[4] (16B @ [rsp - 77])
;   [114:9] clear 4 * 4B = 16B
;   [114:5] size <= 32B, use mov
    mov qword [rsp - 77], 0
    mov qword [rsp - 69], 0
;   [115:5] # arrays are initialized to 0
;   [117:5] var answer
;   [117:9] answer: i64 (8B @ [rsp - 85])
;   [117:9] clear 1 * 8B = 8B
;   [117:5] size <= 32B, use mov
    mov qword [rsp - 85], 0
;   [118:5] # variables without initializer are zeroed
;   [119:5] assert(answer == 0)
;   [119:12] allocate scratch register -> r15
;   [119:12] ? answer == 0
;   [119:12] ? answer == 0
    cmp_119_12:
    cmp qword [rsp - 85], 0
    sete r15b
    bool_end_119_12:
;   [13:6] assert(expr : bool) 
    assert_119_5:
;       [119:5] alias expr -> r15b  (lea: )
        if_13_29_119_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_119_5:
        cmp r15b, 0
        jne if_13_26_119_5_end
        if_13_29_119_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_119_5:
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
            exit_13_38_119_5_end:
        if_13_26_119_5_end:
;       [119:5] free scratch register 'r15'
    assert_119_5_end:
;   [121:5] answer = maybe
;   [121:14] maybe
    mov qword [rsp - 85], -1
;   [122:5] assert(answer == -1)
;   [122:12] allocate scratch register -> r15
;   [122:12] ? answer == -1
;   [122:12] ? answer == -1
    cmp_122_12:
    cmp qword [rsp - 85], -1
    sete r15b
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
;   [124:5] var ix = 1
;   [124:9] ix: i64 (8B @ [rsp - 93])
;   [124:9] ix = 1
;   [124:14] 1
    mov qword [rsp - 93], 1
;   [126:5] arr[ix] = 2
;   [126:5] allocate scratch register -> r15
;   [126:9] set array index
;   [126:9] ix
    mov r15, qword [rsp - 93]
;   [126:9] bounds check
;   [126:9] allocate scratch register -> r14
;   [126:9] line number
    mov r14, 126
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [126:9] free scratch register 'r14'
;   [126:15] 2
    mov dword [rsp + r15 * 4 - 77], 2
;   [126:5] free scratch register 'r15'
;   [127:5] arr[ix + 1] = arr[ix]
;   [127:5] allocate scratch register -> r15
;   [127:9] set array index
;   [127:9] ix
    mov r15, qword [rsp - 93]
;   [127:14] r15 + 1
    add r15, 1
;   [127:9] bounds check
;   [127:9] allocate scratch register -> r14
;   [127:9] line number
    mov r14, 127
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [127:9] free scratch register 'r14'
;   [127:19] arr[ix]
;   [127:19] allocate scratch register -> r14
;   [127:23] set array index
;   [127:23] ix
    mov r14, qword [rsp - 93]
;   [127:23] bounds check
;   [127:23] allocate scratch register -> r13
;   [127:23] line number
    mov r13, 127
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [127:23] free scratch register 'r13'
;   [127:19] allocate scratch register -> r13
    mov r13d, dword [rsp + r14 * 4 - 77]
    mov dword [rsp + r15 * 4 - 77], r13d
;   [127:19] free scratch register 'r13'
;   [127:19] free scratch register 'r14'
;   [127:5] free scratch register 'r15'
;   [128:5] assert(arr[1] == 2)
;   [128:12] allocate scratch register -> r15
;   [128:12] ? arr[1] == 2
;   [128:12] ? arr[1] == 2
    cmp_128_12:
;   [128:12] allocate scratch register -> r14
;   [128:16] set array index
;   [128:16] 1
    mov r14, 1
;   [128:16] bounds check
;   [128:16] allocate scratch register -> r13
;   [128:16] line number
    mov r13, 128
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [128:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 77], 2
;   [128:12] free scratch register 'r14'
    sete r15b
    bool_end_128_12:
;   [13:6] assert(expr : bool) 
    assert_128_5:
;       [128:5] alias expr -> r15b  (lea: )
        if_13_29_128_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_128_5:
        cmp r15b, 0
        jne if_13_26_128_5_end
        if_13_29_128_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_128_5:
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
            exit_13_38_128_5_end:
        if_13_26_128_5_end:
;       [128:5] free scratch register 'r15'
    assert_128_5_end:
;   [129:5] assert(arr[2] == 2)
;   [129:12] allocate scratch register -> r15
;   [129:12] ? arr[2] == 2
;   [129:12] ? arr[2] == 2
    cmp_129_12:
;   [129:12] allocate scratch register -> r14
;   [129:16] set array index
;   [129:16] 2
    mov r14, 2
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
    cmp dword [rsp + r14 * 4 - 77], 2
;   [129:12] free scratch register 'r14'
    sete r15b
    bool_end_129_12:
;   [13:6] assert(expr : bool) 
    assert_129_5:
;       [129:5] alias expr -> r15b  (lea: )
        if_13_29_129_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_129_5:
        cmp r15b, 0
        jne if_13_26_129_5_end
        if_13_29_129_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_129_5:
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
            exit_13_38_129_5_end:
        if_13_26_129_5_end:
;       [129:5] free scratch register 'r15'
    assert_129_5_end:
;   [131:5] array_copy(arr[2], arr, 2)
;   [131:5] allocate named register 'rsi'
;   [131:5] allocate named register 'rdi'
;   [131:5] allocate named register 'rcx'
;   [131:29] 2
;   [131:29] 2
    mov rcx, 2
;   [131:16] arr[2]
;   [131:16] allocate scratch register -> r15
;   [131:20] set array index
;   [131:20] 2
    mov r15, 2
;   [131:20] bounds check
;   [131:20] allocate scratch register -> r14
;   [131:20] line number
    mov r14, 131
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
;   [131:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [131:20] free scratch register 'r13'
    cmovg rbp, r14
    jg panic_bounds
;   [131:20] free scratch register 'r14'
    lea rsi, [rsp + r15 * 4 - 77]
;   [131:5] free scratch register 'r15'
;   [131:24] arr
;   [131:24] bounds check
;   [131:24] allocate scratch register -> r15
;   [131:24] line number
    mov r15, 131
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [131:24] free scratch register 'r15'
    lea rdi, [rsp - 77]
    shl rcx, 2
    rep movsb
;   [131:5] free named register 'rcx'
;   [131:5] free named register 'rdi'
;   [131:5] free named register 'rsi'
;   [132:5] # copy from, to, number of elements
;   [133:5] assert(arr[0] == 2)
;   [133:12] allocate scratch register -> r15
;   [133:12] ? arr[0] == 2
;   [133:12] ? arr[0] == 2
    cmp_133_12:
;   [133:12] allocate scratch register -> r14
;   [133:16] set array index
;   [133:16] 0
    mov r14, 0
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
    cmp dword [rsp + r14 * 4 - 77], 2
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
;   [135:5] var arr1 : i32[8]
;   [135:9] arr1: i32[8] (32B @ [rsp - 125])
;   [135:9] clear 8 * 4B = 32B
;   [135:5] size <= 32B, use mov
    mov qword [rsp - 125], 0
    mov qword [rsp - 117], 0
    mov qword [rsp - 109], 0
    mov qword [rsp - 101], 0
;   [136:5] array_copy(arr, arr1, 4)
;   [136:5] allocate named register 'rsi'
;   [136:5] allocate named register 'rdi'
;   [136:5] allocate named register 'rcx'
;   [136:27] 4
;   [136:27] 4
    mov rcx, 4
;   [136:16] arr
;   [136:16] bounds check
;   [136:16] allocate scratch register -> r15
;   [136:16] line number
    mov r15, 136
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 4
    cmovg rbp, r15
    jg panic_bounds
;   [136:16] free scratch register 'r15'
    lea rsi, [rsp - 77]
;   [136:21] arr1
;   [136:21] bounds check
;   [136:21] allocate scratch register -> r15
;   [136:21] line number
    mov r15, 136
    test rcx, rcx
    cmovs rbp, r15
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r15
    jg panic_bounds
;   [136:21] free scratch register 'r15'
    lea rdi, [rsp - 125]
    shl rcx, 2
    rep movsb
;   [136:5] free named register 'rcx'
;   [136:5] free named register 'rdi'
;   [136:5] free named register 'rsi'
;   [137:5] assert(arrays_equal(arr, arr1, 4))
;   [137:12] allocate scratch register -> r15
;   [137:12] ? arrays_equal(arr, arr1, 4)
;   [137:12] ? arrays_equal(arr, arr1, 4)
    cmp_137_12:
;   [137:12] allocate scratch register -> r14
;       [137:12] r14 = arrays_equal(arr, arr1, 4)
;       [137:12] = expression
;       [137:12] arrays_equal(arr, arr1, 4)
;       [137:12] allocate named register 'rsi'
;       [137:12] allocate named register 'rdi'
;       [137:12] allocate named register 'rcx'
;       [137:36] 4
;       [137:36] 4
        mov rcx, 4
;       [137:25] arr
;       [137:25] bounds check
;       [137:25] allocate scratch register -> r13
;       [137:25] line number
        mov r13, 137
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [137:25] free scratch register 'r13'
        lea rsi, [rsp - 77]
;       [137:30] arr1
;       [137:30] bounds check
;       [137:30] allocate scratch register -> r13
;       [137:30] line number
        mov r13, 137
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [137:30] free scratch register 'r13'
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
;       [137:12] free named register 'rcx'
;       [137:12] free named register 'rdi'
;       [137:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [137:12] free scratch register 'r14'
    setne r15b
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
;   [138:5] # note: `arrays_equal` is built-in function
;   [140:5] arr1[2] = -1
;   [140:5] allocate scratch register -> r15
;   [140:10] set array index
;   [140:10] 2
    mov r15, 2
;   [140:10] bounds check
;   [140:10] allocate scratch register -> r14
;   [140:10] line number
    mov r14, 140
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 8
    cmovge rbp, r14
    jge panic_bounds
;   [140:10] free scratch register 'r14'
;   [140:16] -1
    mov dword [rsp + r15 * 4 - 125], -1
;   [140:5] free scratch register 'r15'
;   [141:5] assert(not arrays_equal(arr, arr1, 4))
;   [141:12] allocate scratch register -> r15
;   [141:12] ? not arrays_equal(arr, arr1, 4)
;   [141:12] ? not arrays_equal(arr, arr1, 4)
    cmp_141_12:
;   [141:16] allocate scratch register -> r14
;       [141:16] r14 = arrays_equal(arr, arr1, 4)
;       [141:16] = expression
;       [141:16] arrays_equal(arr, arr1, 4)
;       [141:16] allocate named register 'rsi'
;       [141:16] allocate named register 'rdi'
;       [141:16] allocate named register 'rcx'
;       [141:40] 4
;       [141:40] 4
        mov rcx, 4
;       [141:29] arr
;       [141:29] bounds check
;       [141:29] allocate scratch register -> r13
;       [141:29] line number
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 4
        cmovg rbp, r13
        jg panic_bounds
;       [141:29] free scratch register 'r13'
        lea rsi, [rsp - 77]
;       [141:34] arr1
;       [141:34] bounds check
;       [141:34] allocate scratch register -> r13
;       [141:34] line number
        mov r13, 141
        test rcx, rcx
        cmovs rbp, r13
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r13
        jg panic_bounds
;       [141:34] free scratch register 'r13'
        lea rdi, [rsp - 125]
        shl rcx, 2
        repe cmpsb
;       [141:16] free named register 'rcx'
;       [141:16] free named register 'rdi'
;       [141:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [141:12] free scratch register 'r14'
    sete r15b
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
;   [143:5] ix = 3
;   [143:10] 3
    mov qword [rsp - 93], 3
;   [144:5] arr[ix] = ~inv(arr[ix - 1])
;   [144:5] allocate scratch register -> r15
;   [144:9] set array index
;   [144:9] ix
    mov r15, qword [rsp - 93]
;   [144:9] bounds check
;   [144:9] allocate scratch register -> r14
;   [144:9] line number
    mov r14, 144
    test r15, r15
    cmovs rbp, r14
    js panic_bounds
    cmp r15, 4
    cmovge rbp, r14
    jge panic_bounds
;   [144:9] free scratch register 'r14'
;   [144:16] arr = ~inv(arr[ix - 1])
;   [144:16] = expression
;   [144:16] ~inv(arr[ix - 1])
;   [144:20] allocate scratch register -> r14
;   [144:24] set array index
;   [144:24] ix
    mov r14, qword [rsp - 93]
;   [144:29] r14 - 1
    sub r14, 1
;   [144:24] bounds check
;   [144:24] allocate scratch register -> r13
;   [144:24] line number
    mov r13, 144
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [144:24] free scratch register 'r13'
;   [62:6] inv(i : i32) : i32 res 
    inv_144_16:
;       [144:16] alias res -> dword [rsp + r15 * 4 - 77]  (lea: )
;       [144:16] alias i -> arr  (lea: rsp + r14 * 4 - 77)
;       [63:5] res = ~i
;       [63:12] ~i
;       [63:12] allocate scratch register -> r13
        mov r13d, dword [rsp + r14 * 4 - 77]
        mov dword [rsp + r15 * 4 - 77], r13d
;       [63:12] free scratch register 'r13'
        not dword [rsp + r15 * 4 - 77]
;       [144:16] free scratch register 'r14'
    inv_144_16_end:
    not dword [rsp + r15 * 4 - 77]
;   [144:5] free scratch register 'r15'
;   [145:5] assert(arr[ix] == 2)
;   [145:12] allocate scratch register -> r15
;   [145:12] ? arr[ix] == 2
;   [145:12] ? arr[ix] == 2
    cmp_145_12:
;   [145:12] allocate scratch register -> r14
;   [145:16] set array index
;   [145:16] ix
    mov r14, qword [rsp - 93]
;   [145:16] bounds check
;   [145:16] allocate scratch register -> r13
;   [145:16] line number
    mov r13, 145
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 4
    cmovge rbp, r13
    jge panic_bounds
;   [145:16] free scratch register 'r13'
    cmp dword [rsp + r14 * 4 - 77], 2
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
;   [147:5] faz(arr)
;   [72:6] faz(arg : i32[]) 
    faz_147_5:
;       [147:5] alias arg -> arr  (lea: )
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
    faz_147_5_end:
;   [148:5] assert(arr[1] == 0xfe)
;   [148:12] allocate scratch register -> r15
;   [148:12] ? arr[1] == 0xfe
;   [148:12] ? arr[1] == 0xfe
    cmp_148_12:
;   [148:12] allocate scratch register -> r14
;   [148:16] set array index
;   [148:16] 1
    mov r14, 1
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
    cmp dword [rsp + r14 * 4 - 77], 254
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
;   [150:5] var p : point = {0, 0}
;   [150:9] p: point (16B @ [rsp - 141])
;   [150:9] p = {0, 0}
;   [150:21] copy field 'x'
    mov qword [rsp - 141], 0
;   [150:21] copy field 'y'
    mov qword [rsp - 133], 0
;   [151:5] foo(p)
;   [48:6] foo(pt : point) 
    foo_151_5:
;       [151:5] alias pt -> p  (lea: )
;       [49:5] pt.x = 0b10
;       [49:12] 0b10
        mov qword [rsp - 141], 2
;       [49:20] # binary value 2
;       [50:5] pt.y = 0xb
;       [50:12] 0xb
        mov qword [rsp - 133], 11
;       [50:20] # hex value 11
    foo_151_5_end:
;   [152:5] assert(p.x == 2)
;   [152:12] allocate scratch register -> r15
;   [152:12] ? p.x == 2
;   [152:12] ? p.x == 2
    cmp_152_12:
    cmp qword [rsp - 141], 2
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
;   [153:5] assert(p.y == 0xb)
;   [153:12] allocate scratch register -> r15
;   [153:12] ? p.y == 0xb
;   [153:12] ? p.y == 0xb
    cmp_153_12:
    cmp qword [rsp - 133], 11
    sete r15b
    bool_end_153_12:
;   [13:6] assert(expr : bool) 
    assert_153_5:
;       [153:5] alias expr -> r15b  (lea: )
        if_13_29_153_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_153_5:
        cmp r15b, 0
        jne if_13_26_153_5_end
        if_13_29_153_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_153_5:
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
            exit_13_38_153_5_end:
        if_13_26_153_5_end:
;       [153:5] free scratch register 'r15'
    assert_153_5_end:
;   [155:5] var q : point = p
;   [155:9] q: point (16B @ [rsp - 157])
;   [155:9] q = p
;   [155:21] allocate named register 'rsi'
;   [155:21] allocate named register 'rdi'
;   [155:21] allocate named register 'rcx'
    lea rsi, [rsp - 141]
    lea rdi, [rsp - 157]
    mov rcx, 2
    rep movsq
;   [155:21] free named register 'rcx'
;   [155:21] free named register 'rdi'
;   [155:21] free named register 'rsi'
;   [156:5] assert(equal(p, q))
;   [156:12] allocate scratch register -> r15
;   [156:12] ? equal(p, q)
;   [156:12] ? equal(p, q)
    cmp_156_12:
;   [156:12] allocate scratch register -> r14
;       [156:12] r14 = equal(p, q)
;       [156:12] = expression
;       [156:12] equal(p, q)
;       [156:12] allocate named register 'rsi'
;       [156:12] allocate named register 'rdi'
;       [156:12] allocate named register 'rcx'
;       [156:18] p
        lea rsi, [rsp - 141]
;       [156:21] q
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
;       [156:12] free named register 'rcx'
;       [156:12] free named register 'rdi'
;       [156:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [156:12] free scratch register 'r14'
    setne r15b
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
;   [157:5] # note: `equal` is built-in function to compare user types for equality
;   [158:5] # or same size arrays
;   [160:5] q.x = 3
;   [160:11] 3
    mov qword [rsp - 157], 3
;   [161:5] assert(not equal(p, q))
;   [161:12] allocate scratch register -> r15
;   [161:12] ? not equal(p, q)
;   [161:12] ? not equal(p, q)
    cmp_161_12:
;   [161:16] allocate scratch register -> r14
;       [161:16] r14 = equal(p, q)
;       [161:16] = expression
;       [161:16] equal(p, q)
;       [161:16] allocate named register 'rsi'
;       [161:16] allocate named register 'rdi'
;       [161:16] allocate named register 'rcx'
;       [161:22] p
        lea rsi, [rsp - 141]
;       [161:25] q
        lea rdi, [rsp - 157]
        mov rcx, 2
        repe cmpsq
;       [161:16] free named register 'rcx'
;       [161:16] free named register 'rdi'
;       [161:16] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [161:12] free scratch register 'r14'
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
;   [163:5] var i = 0
;   [163:9] i: i64 (8B @ [rsp - 165])
;   [163:9] i = 0
;   [163:13] 0
    mov qword [rsp - 165], 0
;   [164:5] bar(i)
;   [55:6] bar(arg) 
    bar_164_5:
;       [164:5] alias arg -> i  (lea: )
        if_56_8_164_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_164_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_164_5_end
        if_56_8_164_5_code:
;           [56:17] return
            jmp bar_164_5_end
        if_56_5_164_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 165], 255
    bar_164_5_end:
;   [165:5] assert(i == 0)
;   [165:12] allocate scratch register -> r15
;   [165:12] ? i == 0
;   [165:12] ? i == 0
    cmp_165_12:
    cmp qword [rsp - 165], 0
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
;   [167:5] i = 1
;   [167:9] 1
    mov qword [rsp - 165], 1
;   [168:5] bar(i)
;   [55:6] bar(arg) 
    bar_168_5:
;       [168:5] alias arg -> i  (lea: )
        if_56_8_168_5:
;       [56:8] ? arg == 0
;       [56:8] ? arg == 0
        cmp_56_8_168_5:
        cmp qword [rsp - 165], 0
        jne if_56_5_168_5_end
        if_56_8_168_5_code:
;           [56:17] return
            jmp bar_168_5_end
        if_56_5_168_5_end:
;       [57:5] arg = 0xff
;       [57:11] 0xff
        mov qword [rsp - 165], 255
    bar_168_5_end:
;   [169:5] assert(i == 0xff)
;   [169:12] allocate scratch register -> r15
;   [169:12] ? i == 0xff
;   [169:12] ? i == 0xff
    cmp_169_12:
    cmp qword [rsp - 165], 255
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
;   [171:5] var j = 1
;   [171:9] j: i64 (8B @ [rsp - 173])
;   [171:9] j = 1
;   [171:13] 1
    mov qword [rsp - 173], 1
;   [172:5] var k = baz(j)
;   [172:9] k: i64 (8B @ [rsp - 181])
;   [172:9] k = baz(j)
;   [172:13] k = baz(j)
;   [172:13] = expression
;   [172:13] baz(j)
;   [66:6] baz(arg) : i64 res 
    baz_172_13:
;       [172:13] alias res -> qword [rsp - 181]  (lea: )
;       [172:13] alias arg -> j  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, qword [rsp - 173]
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 181], r15
;       [67:11] free scratch register 'r15'
    baz_172_13_end:
;   [173:5] assert(k == 2)
;   [173:12] allocate scratch register -> r15
;   [173:12] ? k == 2
;   [173:12] ? k == 2
    cmp_173_12:
    cmp qword [rsp - 181], 2
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
;   [175:5] k = baz(1)
;   [175:9] k = baz(1)
;   [175:9] = expression
;   [175:9] baz(1)
;   [66:6] baz(arg) : i64 res 
    baz_175_9:
;       [175:9] alias res -> qword [rsp - 181]  (lea: )
;       [175:9] alias arg -> 1  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 1
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 181], r15
;       [67:11] free scratch register 'r15'
    baz_175_9_end:
;   [176:5] assert(k == 2)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? k == 2
;   [176:12] ? k == 2
    cmp_176_12:
    cmp qword [rsp - 181], 2
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
;   [178:5] var p0 : point = {baz(2), 0}
;   [178:9] p0: point (16B @ [rsp - 197])
;   [178:9] p0 = {baz(2), 0}
;   [178:22] copy field 'x'
;   [178:23] qword [rsp - 197] = baz(2)
;   [178:23] = expression
;   [178:23] baz(2)
;   [66:6] baz(arg) : i64 res 
    baz_178_23:
;       [178:23] alias res -> qword [rsp - 197]  (lea: )
;       [178:23] alias arg -> 2  (lea: )
;       [67:5] res = arg * 2
;       [67:11] allocate scratch register -> r15
;       [67:11] arg
        mov r15, 2
;       [67:17] r15 * 2
;       [67:17] dst is reg, src is const
        imul r15, 2
        mov qword [rsp - 197], r15
;       [67:11] free scratch register 'r15'
    baz_178_23_end:
;   [178:22] copy field 'y'
    mov qword [rsp - 189], 0
;   [179:5] assert(p0.x == 4)
;   [179:12] allocate scratch register -> r15
;   [179:12] ? p0.x == 4
;   [179:12] ? p0.x == 4
    cmp_179_12:
    cmp qword [rsp - 197], 4
    sete r15b
    bool_end_179_12:
;   [13:6] assert(expr : bool) 
    assert_179_5:
;       [179:5] alias expr -> r15b  (lea: )
        if_13_29_179_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_179_5:
        cmp r15b, 0
        jne if_13_26_179_5_end
        if_13_29_179_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_179_5:
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
            exit_13_38_179_5_end:
        if_13_26_179_5_end:
;       [179:5] free scratch register 'r15'
    assert_179_5_end:
;   [181:5] var x = 1
;   [181:9] x: i64 (8B @ [rsp - 205])
;   [181:9] x = 1
;   [181:13] 1
    mov qword [rsp - 205], 1
;   [182:5] var y = 2
;   [182:9] y: i64 (8B @ [rsp - 213])
;   [182:9] y = 2
;   [182:13] 2
    mov qword [rsp - 213], 2
;   [184:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [184:9] o1: object (20B @ [rsp - 233])
;   [184:9] o1 = {{x * 10, y}, 0xff0000}
;   [184:23] copy field 'pos'
;   [184:24] copy field 'x'
;   [184:25] allocate scratch register -> r15
;   [184:25] x
    mov r15, qword [rsp - 205]
;   [184:29] r15 * 10
;   [184:29] dst is reg, src is const
    imul r15, 10
    mov qword [rsp - 233], r15
;   [184:25] free scratch register 'r15'
;   [184:24] copy field 'y'
;   [184:33] allocate scratch register -> r15
    mov r15, qword [rsp - 213]
    mov qword [rsp - 225], r15
;   [184:33] free scratch register 'r15'
;   [184:23] copy field 'color'
    mov dword [rsp - 217], 16711680
;   [185:5] assert(o1.pos.x == 10)
;   [185:12] allocate scratch register -> r15
;   [185:12] ? o1.pos.x == 10
;   [185:12] ? o1.pos.x == 10
    cmp_185_12:
    cmp qword [rsp - 233], 10
    sete r15b
    bool_end_185_12:
;   [13:6] assert(expr : bool) 
    assert_185_5:
;       [185:5] alias expr -> r15b  (lea: )
        if_13_29_185_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_185_5:
        cmp r15b, 0
        jne if_13_26_185_5_end
        if_13_29_185_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_185_5:
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
            exit_13_38_185_5_end:
        if_13_26_185_5_end:
;       [185:5] free scratch register 'r15'
    assert_185_5_end:
;   [186:5] assert(o1.pos.y == 2)
;   [186:12] allocate scratch register -> r15
;   [186:12] ? o1.pos.y == 2
;   [186:12] ? o1.pos.y == 2
    cmp_186_12:
    cmp qword [rsp - 225], 2
    sete r15b
    bool_end_186_12:
;   [13:6] assert(expr : bool) 
    assert_186_5:
;       [186:5] alias expr -> r15b  (lea: )
        if_13_29_186_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_186_5:
        cmp r15b, 0
        jne if_13_26_186_5_end
        if_13_29_186_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_186_5:
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
            exit_13_38_186_5_end:
        if_13_26_186_5_end:
;       [186:5] free scratch register 'r15'
    assert_186_5_end:
;   [187:5] assert(o1.color == 0xff0000)
;   [187:12] allocate scratch register -> r15
;   [187:12] ? o1.color == 0xff0000
;   [187:12] ? o1.color == 0xff0000
    cmp_187_12:
    cmp dword [rsp - 217], 16711680
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
;   [189:5] var p1 : point = {-x, -y}
;   [189:9] p1: point (16B @ [rsp - 249])
;   [189:9] p1 = {-x, -y}
;   [189:22] copy field 'x'
;   [189:23] allocate scratch register -> r15
    mov r15, qword [rsp - 205]
    mov qword [rsp - 249], r15
;   [189:23] free scratch register 'r15'
    neg qword [rsp - 249]
;   [189:22] copy field 'y'
;   [189:27] allocate scratch register -> r15
    mov r15, qword [rsp - 213]
    mov qword [rsp - 241], r15
;   [189:27] free scratch register 'r15'
    neg qword [rsp - 241]
;   [190:5] o1.pos = p1
;   [190:14] allocate named register 'rsi'
;   [190:14] allocate named register 'rdi'
;   [190:14] allocate named register 'rcx'
    lea rsi, [rsp - 249]
    lea rdi, [rsp - 233]
    mov rcx, 2
    rep movsq
;   [190:14] free named register 'rcx'
;   [190:14] free named register 'rdi'
;   [190:14] free named register 'rsi'
;   [191:5] assert(o1.pos.x == -1)
;   [191:12] allocate scratch register -> r15
;   [191:12] ? o1.pos.x == -1
;   [191:12] ? o1.pos.x == -1
    cmp_191_12:
    cmp qword [rsp - 233], -1
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
;   [192:5] assert(o1.pos.y == -2)
;   [192:12] allocate scratch register -> r15
;   [192:12] ? o1.pos.y == -2
;   [192:12] ? o1.pos.y == -2
    cmp_192_12:
    cmp qword [rsp - 225], -2
    sete r15b
    bool_end_192_12:
;   [13:6] assert(expr : bool) 
    assert_192_5:
;       [192:5] alias expr -> r15b  (lea: )
        if_13_29_192_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_192_5:
        cmp r15b, 0
        jne if_13_26_192_5_end
        if_13_29_192_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_192_5:
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
            exit_13_38_192_5_end:
        if_13_26_192_5_end:
;       [192:5] free scratch register 'r15'
    assert_192_5_end:
;   [194:5] var o2 : object = o1
;   [194:9] o2: object (20B @ [rsp - 269])
;   [194:9] o2 = o1
;   [194:23] allocate named register 'rsi'
;   [194:23] allocate named register 'rdi'
;   [194:23] allocate named register 'rcx'
    lea rsi, [rsp - 233]
    lea rdi, [rsp - 269]
    mov rcx, 20
    rep movsb
;   [194:23] free named register 'rcx'
;   [194:23] free named register 'rdi'
;   [194:23] free named register 'rsi'
;   [195:5] assert(o2.pos.x == -1)
;   [195:12] allocate scratch register -> r15
;   [195:12] ? o2.pos.x == -1
;   [195:12] ? o2.pos.x == -1
    cmp_195_12:
    cmp qword [rsp - 269], -1
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
;   [196:5] assert(o2.pos.y == -2)
;   [196:12] allocate scratch register -> r15
;   [196:12] ? o2.pos.y == -2
;   [196:12] ? o2.pos.y == -2
    cmp_196_12:
    cmp qword [rsp - 261], -2
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
;   [197:5] assert(o2.color == 0xff0000)
;   [197:12] allocate scratch register -> r15
;   [197:12] ? o2.color == 0xff0000
;   [197:12] ? o2.color == 0xff0000
    cmp_197_12:
    cmp dword [rsp - 253], 16711680
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
;   [199:5] var o3 : object[1]
;   [199:9] o3: object[1] (20B @ [rsp - 289])
;   [199:9] clear 1 * 20B = 20B
;   [199:5] size <= 32B, use mov
    mov qword [rsp - 289], 0
    mov qword [rsp - 281], 0
    mov dword [rsp - 273], 0
;   [200:5] # index 0 in an array can be accessed without array index
;   [201:5] o3.pos.y = 73
;   [201:16] 73
    mov qword [rsp - 281], 73
;   [202:5] assert(o3[0].pos.y == 73)
;   [202:12] allocate scratch register -> r15
;   [202:12] ? o3[0].pos.y == 73
;   [202:12] ? o3[0].pos.y == 73
    cmp_202_12:
;   [202:12] allocate scratch register -> r14
    lea r14, [rsp - 289]
;   [202:12] allocate scratch register -> r13
;   [202:15] set array index
;   [202:15] 0
    mov r13, 0
;   [202:15] bounds check
;   [202:15] allocate scratch register -> r12
;   [202:15] line number
    mov r12, 202
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 1
    cmovge rbp, r12
    jge panic_bounds
;   [202:15] free scratch register 'r12'
    imul r13, 20
    add r14, r13
;   [202:12] free scratch register 'r13'
    cmp qword [r14 + 8], 73
;   [202:12] free scratch register 'r14'
    sete r15b
    bool_end_202_12:
;   [13:6] assert(expr : bool) 
    assert_202_5:
;       [202:5] alias expr -> r15b  (lea: )
        if_13_29_202_5:
;       [13:29] ? not expr
;       [13:29] ? not expr
        cmp_13_29_202_5:
        cmp r15b, 0
        jne if_13_26_202_5_end
        if_13_29_202_5_code:
;           [13:38] exit(1)
;           [13:43] allocate named register 'rdi'
            mov rdi, 1
;           [5:6] exit(v : reg_rdi) 
            exit_13_38_202_5:
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
            exit_13_38_202_5_end:
        if_13_26_202_5_end:
;       [202:5] free scratch register 'r15'
    assert_202_5_end:
;   [204:5] var worlds : world[8]
;   [204:9] worlds: world[8] (512B @ [rsp - 801])
;   [204:9] clear 8 * 64B = 512B
;   [204:5] allocate named register 'rcx'
;   [204:5] allocate named register 'rdi'
;   [204:5] allocate named register 'rax'
    mov rcx, 512
    lea rdi, [rsp - 801]
    xor rax, rax
    rep stosb
;   [204:5] free named register 'rax'
;   [204:5] free named register 'rdi'
;   [204:5] free named register 'rcx'
;   [205:5] worlds[1].locations[1] = 0xffee
;   [205:5] allocate scratch register -> r15
    lea r15, [rsp - 801]
;   [205:5] allocate scratch register -> r14
;   [205:12] set array index
;   [205:12] 1
    mov r14, 1
;   [205:12] bounds check
;   [205:12] allocate scratch register -> r13
;   [205:12] line number
    mov r13, 205
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [205:12] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [205:5] free scratch register 'r14'
;   [205:5] allocate scratch register -> r14
;   [205:25] set array index
;   [205:25] 1
    mov r14, 1
;   [205:25] bounds check
;   [205:25] allocate scratch register -> r13
;   [205:25] line number
    mov r13, 205
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [205:25] free scratch register 'r13'
;   [205:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [205:5] free scratch register 'r14'
;   [205:5] free scratch register 'r15'
;   [206:5] assert(worlds[1].locations[1] == 0xffee)
;   [206:12] allocate scratch register -> r15
;   [206:12] ? worlds[1].locations[1] == 0xffee
;   [206:12] ? worlds[1].locations[1] == 0xffee
    cmp_206_12:
;   [206:12] allocate scratch register -> r14
    lea r14, [rsp - 801]
;   [206:12] allocate scratch register -> r13
;   [206:19] set array index
;   [206:19] 1
    mov r13, 1
;   [206:19] bounds check
;   [206:19] allocate scratch register -> r12
;   [206:19] line number
    mov r12, 206
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [206:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [206:12] free scratch register 'r13'
;   [206:12] allocate scratch register -> r13
;   [206:32] set array index
;   [206:32] 1
    mov r13, 1
;   [206:32] bounds check
;   [206:32] allocate scratch register -> r12
;   [206:32] line number
    mov r12, 206
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [206:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [206:12] free scratch register 'r13'
;   [206:12] free scratch register 'r14'
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
;   [208:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [208:5] allocate named register 'rsi'
;   [208:5] allocate named register 'rdi'
;   [208:5] allocate named register 'rcx'
;   [210:9] array_size_of(worlds.locations)
;   [211:9] rcx = array_size_of(worlds.locations)
;   [211:9] = expression
;   [211:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [209:9] worlds[1].locations
;   [209:9] allocate scratch register -> r15
    lea r15, [rsp - 801]
;   [209:9] allocate scratch register -> r14
;   [209:16] set array index
;   [209:16] 1
    mov r14, 1
;   [209:16] bounds check
;   [209:16] allocate scratch register -> r13
;   [209:16] line number
    mov r13, 209
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [209:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [209:9] free scratch register 'r14'
;   [209:9] bounds check
;   [209:9] allocate scratch register -> r14
;   [209:9] line number
    mov r14, 209
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [209:9] free scratch register 'r14'
    lea rsi, [r15]
;   [208:5] free scratch register 'r15'
;   [210:9] worlds[0].locations
;   [210:9] allocate scratch register -> r15
    lea r15, [rsp - 801]
;   [210:9] allocate scratch register -> r14
;   [210:16] set array index
;   [210:16] 0
    mov r14, 0
;   [210:16] bounds check
;   [210:16] allocate scratch register -> r13
;   [210:16] line number
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js panic_bounds
    cmp r14, 8
    cmovge rbp, r13
    jge panic_bounds
;   [210:16] free scratch register 'r13'
    shl r14, 6
    add r15, r14
;   [210:9] free scratch register 'r14'
;   [210:9] bounds check
;   [210:9] allocate scratch register -> r14
;   [210:9] line number
    mov r14, 210
    test rcx, rcx
    cmovs rbp, r14
    js panic_bounds
    cmp rcx, 8
    cmovg rbp, r14
    jg panic_bounds
;   [210:9] free scratch register 'r14'
    lea rdi, [r15]
;   [208:5] free scratch register 'r15'
    shl rcx, 3
    rep movsb
;   [208:5] free named register 'rcx'
;   [208:5] free named register 'rdi'
;   [208:5] free named register 'rsi'
;   [213:5] # note: `array_copy` is built-in and can use indexed positions
;   [214:5] # `array_size_of` is built-in
;   [215:5] assert(worlds[0].locations[1] == 0xffee)
;   [215:12] allocate scratch register -> r15
;   [215:12] ? worlds[0].locations[1] == 0xffee
;   [215:12] ? worlds[0].locations[1] == 0xffee
    cmp_215_12:
;   [215:12] allocate scratch register -> r14
    lea r14, [rsp - 801]
;   [215:12] allocate scratch register -> r13
;   [215:19] set array index
;   [215:19] 0
    mov r13, 0
;   [215:19] bounds check
;   [215:19] allocate scratch register -> r12
;   [215:19] line number
    mov r12, 215
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [215:19] free scratch register 'r12'
    shl r13, 6
    add r14, r13
;   [215:12] free scratch register 'r13'
;   [215:12] allocate scratch register -> r13
;   [215:32] set array index
;   [215:32] 1
    mov r13, 1
;   [215:32] bounds check
;   [215:32] allocate scratch register -> r12
;   [215:32] line number
    mov r12, 215
    test r13, r13
    cmovs rbp, r12
    js panic_bounds
    cmp r13, 8
    cmovge rbp, r12
    jge panic_bounds
;   [215:32] free scratch register 'r12'
    cmp qword [r14 + r13 * 8], 65518
;   [215:12] free scratch register 'r13'
;   [215:12] free scratch register 'r14'
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
;   [216:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [216:12] allocate scratch register -> r15
;   [216:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [216:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_216_12:
;   [216:12] allocate scratch register -> r14
;       [216:12] r14 = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [216:12] = expression
;       [216:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [216:12] allocate named register 'rsi'
;       [216:12] allocate named register 'rdi'
;       [216:12] allocate named register 'rcx'
;       [218:14] array_size_of(worlds.locations)
;       [219:14] rcx = array_size_of(worlds.locations)
;       [219:14] = expression
;       [219:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [217:14] worlds[0].locations
;       [217:14] allocate scratch register -> r13
        lea r13, [rsp - 801]
;       [217:14] allocate scratch register -> r12
;       [217:21] set array index
;       [217:21] 0
        mov r12, 0
;       [217:21] bounds check
;       [217:21] allocate scratch register -> r11
;       [217:21] line number
        mov r11, 217
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [217:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [217:14] free scratch register 'r12'
;       [217:14] bounds check
;       [217:14] allocate scratch register -> r12
;       [217:14] line number
        mov r12, 217
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [217:14] free scratch register 'r12'
        lea rsi, [r13]
;       [216:12] free scratch register 'r13'
;       [218:14] worlds[1].locations
;       [218:14] allocate scratch register -> r13
        lea r13, [rsp - 801]
;       [218:14] allocate scratch register -> r12
;       [218:21] set array index
;       [218:21] 1
        mov r12, 1
;       [218:21] bounds check
;       [218:21] allocate scratch register -> r11
;       [218:21] line number
        mov r11, 218
        test r12, r12
        cmovs rbp, r11
        js panic_bounds
        cmp r12, 8
        cmovge rbp, r11
        jge panic_bounds
;       [218:21] free scratch register 'r11'
        shl r12, 6
        add r13, r12
;       [218:14] free scratch register 'r12'
;       [218:14] bounds check
;       [218:14] allocate scratch register -> r12
;       [218:14] line number
        mov r12, 218
        test rcx, rcx
        cmovs rbp, r12
        js panic_bounds
        cmp rcx, 8
        cmovg rbp, r12
        jg panic_bounds
;       [218:14] free scratch register 'r12'
        lea rdi, [r13]
;       [216:12] free scratch register 'r13'
        shl rcx, 3
        repe cmpsb
;       [216:12] free named register 'rcx'
;       [216:12] free named register 'rdi'
;       [216:12] free named register 'rsi'
        sete r14b
    cmp r14, 0
;   [216:12] free scratch register 'r14'
    setne r15b
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
;   [222:5] var nm : str
;   [222:9] nm: str (128B @ [rsp - 929])
;   [222:9] clear 1 * 128B = 128B
;   [222:5] allocate named register 'rcx'
;   [222:5] allocate named register 'rdi'
;   [222:5] allocate named register 'rax'
    mov rcx, 128
    lea rdi, [rsp - 929]
    xor rax, rax
    rep stosb
;   [222:5] free named register 'rax'
;   [222:5] free named register 'rdi'
;   [222:5] free named register 'rcx'
;   [223:5] print_str(hello)
;   [23:6] print_str(str : i8[]) 
    print_str_223_5:
;       [223:5] alias str -> hello  (lea: )
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
        print_24_4_223_5:
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
        print_24_4_223_5_end:
    print_str_223_5_end:
;   [224:5] loop
    loop_224_5:
;       [225:9] print_str(prompt1)
;       [23:6] print_str(str : i8[]) 
        print_str_225_9:
;           [225:9] alias str -> prompt1  (lea: )
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
            print_24_4_225_9:
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
            print_24_4_225_9_end:
        print_str_225_9_end:
;       [226:9] str_in(nm)
;       [81:6] str_in(s : str) 
        str_in_226_9:
;           [226:9] alias s -> nm  (lea: )
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
                lea rsi, [rsp - 928]
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
                mov byte [rsp - 929], al
;               [87:22] s.len - 1
                sub byte [rsp - 929], 1
;           [87:25] # return value
        str_in_226_9_end:
        if_227_12:
;       [227:12] ? nm.len == 0
;       [227:12] ? nm.len == 0
        cmp_227_12:
        cmp byte [rsp - 929], 0
        jne if_229_19
        if_227_12_code:
;           [228:13] break
            jmp loop_224_5_end
        jmp if_227_9_end
        if_229_19:
;       [229:19] ? nm.len <= 4
;       [229:19] ? nm.len <= 4
        cmp_229_19:
        cmp byte [rsp - 929], 4
        jg if_else_227_9
        if_229_19_code:
;           [230:13] print_str(prompt2)
;           [23:6] print_str(str : i8[]) 
            print_str_230_13:
;               [230:13] alias str -> prompt2  (lea: )
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
                print_24_4_230_13:
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
                print_24_4_230_13_end:
            print_str_230_13_end:
;           [231:13] continue
            jmp loop_224_5
        jmp if_227_9_end
        if_else_227_9:
;           [233:13] print_str(prompt3)
;           [23:6] print_str(str : i8[]) 
            print_str_233_13:
;               [233:13] alias str -> prompt3  (lea: )
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
                print_24_4_233_13:
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
                print_24_4_233_13_end:
            print_str_233_13_end:
;           [234:13] str_out(nm)
;           [90:6] str_out(s : str) 
            str_out_234_13:
;               [234:13] alias s -> nm  (lea: )
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
                    lea rsi, [rsp - 928]
;               [93:34] # buffer address
;               [94:5] mov(rdx, s.len)
;                   [94:14] s.len
                    movsx rdx, byte [rsp - 929]
;               [94:21] # buffer size
;               [95:5] syscall()
                syscall
            str_out_234_13_end:
;           [235:13] print_str(dot)
;           [23:6] print_str(str : i8[]) 
            print_str_235_13:
;               [235:13] alias str -> dot  (lea: )
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
                print_24_4_235_13:
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
                print_24_4_235_13_end:
            print_str_235_13_end:
;           [236:13] print_str(nl)
;           [23:6] print_str(str : i8[]) 
            print_str_236_13:
;               [236:13] alias str -> nl  (lea: )
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
                print_24_4_236_13:
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
                print_24_4_236_13_end:
            print_str_236_13_end:
        if_227_9_end:
    jmp loop_224_5
    loop_224_5_end:
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
;               max stack size: 929B
;          optimization pass 1: 94
;          optimization pass 2: 0
```
