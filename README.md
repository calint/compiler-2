# compiler-2: baz

Experimental compiler for a minimalistic, specialized language that targets NASM
x86_64 assembly on Linux.

## Intention

* minimalistic language
* gain experience writing compilers
* generate handwritten-like assembler compiled by NASM for x86_64
* super-loop program with non-reentrant inlined functions
* limited support for non-inline functions

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
* limited support for non-inline functions
* keywords: `func`, `type`, `dat`, `var`, `const`, `foo`, `loop`, `if`, `else`,
  `continue`, `break`, `return`
* built-in functions: `array_copy`, `array_size_of`, `arrays_equal`,
  `address_of`, `equal`, `mov`, `syscall`

## Howto

* to compile the compiler that compiles `prog.baz` and assembles the generated
  code run `./make.sh`
* after that use `./run-baz.sh myprogram.baz` or `./run-baz.sh` to compile and
  run `prog.baz`
  * optional parameters: _variable storage size_, _bounds check_, with _line number
    information_ and _jump optimizations_ in boolean expression e.g:
    * `./run-baz.sh myprogram.baz --vars=262144`: reserves 262144 bytes for
      variables, no runtime checks
    * `./run-baz.sh myprogram.baz --vars=262144 --checks=upper`: checks upper
      bounds without line number information and is often enough to ensure
      catching negative values (faster)
    * `./run-baz.sh myprogram.baz --vars=262144 --checks=upper,line`: checks
      upper bounds with line number information
    * `./run-baz.sh myprogram.baz --vars=262144 --checks=upper,lower,line`: checks
      bounds with line number information
    * option `--vars=SIZE` reserves variable storage in bytes (default: 65536,
      decimal or `0x` hex, positive multiple of 16)
    * option `--nopt` disables post processing jump optimizations in boolean
      expression
    * option `--reproduce-source` writes reproduced source to `diff.baz`
      and checks that it matches the input
* to run the tests `qa/coverage/run-tests-all.sh` and see coverage report in
  `qa/coverage/report/`
* syntax highlighting support in neovim (see `etc/nvim/tree-sitter-baz/`)
* todo list of planned fixes and features in `etc/todo.txt`

## Source

```text
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C/C++ Header                    50           2710            830           9143
C++                              1             45              7            227
-------------------------------------------------------------------------------
SUM:                            51           2755            837           9370
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
dat   colon : i8[] = ": "
dat    nums : i64[4] = { 1 } # remaining elements are zeroed
dat      s1 : str = { 3 } # remaining fields are zeroed

# default is to inline functions

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

# limited support for non-inlined functions

func noinline print_num(num) {
    var buf : i8[20]
    var n = num
    var is_negative : bool = false
 
    if n < 0 {
        is_negative = true
        n = -n
    }
 
    var i = 20
    loop {
        i = i - 1
        var ascii = 48 + (n % 10)
        # note: not buf[i] = 48 + ... because expression will be executed as byte sized and n overflows
        buf[i] = ascii 
        n = n / 10
        if n == 0 break
    }
 
    if is_negative {
        i = i - 1
        buf[i] = 45
    }
 
    var write_pos = 0
    loop {
        buf[write_pos] = buf[i]
        write_pos = write_pos + 1
        i = i + 1
        if i == 20 break
    }
 
    sys_print(write_pos, address_of(buf))
}

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

    var counter
    var nm : str
    print(hello)
    loop {
        counter = counter + 1
        print_num(counter)
        print(colon)
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
%macro PUSH_REGS 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro POP_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro
section .text
bits 64
global _start
_start:
lea rbp, [dat]
main:
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
    mov qword [rbp + 240], 0
    cmp_172_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_172_12:
    assert_172_5:
        if_42_26_172_5:
        cmp_42_26_172_5:
        cmp r15b, 0
        jne if_42_23_172_5_end
        if_42_26_172_5_code:
            mov rdi, 1
            exit_42_32_172_5:
                    mov rax, 60
                syscall
            exit_42_32_172_5_end:
        if_42_23_172_5_end:
    assert_172_5_end:
    mov qword [rbp + 240], -1
    cmp_176_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_176_12:
    assert_176_5:
        if_42_26_176_5:
        cmp_42_26_176_5:
        cmp r15b, 0
        jne if_42_23_176_5_end
        if_42_26_176_5_code:
            mov rdi, 1
            exit_42_32_176_5:
                    mov rax, 60
                syscall
            exit_42_32_176_5_end:
        if_42_23_176_5_end:
    assert_176_5_end:
        cmp_180_16:
        bool_end_180_16:
        mov r15b, 1
        assert_180_9:
            if_42_26_180_9:
            cmp_42_26_180_9:
            cmp r15b, 0
            jne if_42_23_180_9_end
            if_42_26_180_9_code:
                mov rdi, 1
                exit_42_32_180_9:
                        mov rax, 60
                    syscall
                exit_42_32_180_9_end:
            if_42_23_180_9_end:
        assert_180_9_end:
    cmp_183_12:
    bool_end_183_12:
    mov r15b, 1
    assert_183_5:
        if_42_26_183_5:
        cmp_42_26_183_5:
        cmp r15b, 0
        jne if_42_23_183_5_end
        if_42_26_183_5_code:
            mov rdi, 1
            exit_42_32_183_5:
                    mov rax, 60
                syscall
            exit_42_32_183_5_end:
        if_42_23_183_5_end:
    assert_183_5_end:
    mov qword [rbp + 248], 1
    mov r15, qword [rbp + 248]
    mov r14, 188
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 224], 2
    mov r15, qword [rbp + 248]
    add r15, 1
    mov r14, 189
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
    cmp_190_12:
    mov r14, 1
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_190_12:
    assert_190_5:
        if_42_26_190_5:
        cmp_42_26_190_5:
        cmp r15b, 0
        jne if_42_23_190_5_end
        if_42_26_190_5_code:
            mov rdi, 1
            exit_42_32_190_5:
                    mov rax, 60
                syscall
            exit_42_32_190_5_end:
        if_42_23_190_5_end:
    assert_190_5_end:
    cmp_191_12:
    mov r14, 2
    mov r13, 191
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_191_12:
    assert_191_5:
        if_42_26_191_5:
        cmp_42_26_191_5:
        cmp r15b, 0
        jne if_42_23_191_5_end
        if_42_26_191_5_code:
            mov rdi, 1
            exit_42_32_191_5:
                    mov rax, 60
                syscall
            exit_42_32_191_5_end:
        if_42_23_191_5_end:
    assert_191_5_end:
    mov rcx, 2
    mov r15, 2
    mov r14, 193
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    mov r13, rcx
    add r13, r15
    cmp r13, 4
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [rbp + r15 * 4 + 224]
    mov r15, 193
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
    cmp_195_12:
    mov r14, 0
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_195_12:
    assert_195_5:
        if_42_26_195_5:
        cmp_42_26_195_5:
        cmp r15b, 0
        jne if_42_23_195_5_end
        if_42_26_195_5_code:
            mov rdi, 1
            exit_42_32_195_5:
                    mov rax, 60
                syscall
            exit_42_32_195_5_end:
        if_42_23_195_5_end:
    assert_195_5_end:
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
    mov rcx, 4
    mov r15, 198
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rsi, [rbp + 224]
    mov r15, 198
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
    cmp_199_12:
        mov rcx, 4
        mov r13, 199
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 199
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_199_12:
    assert_199_5:
        if_42_26_199_5:
        cmp_42_26_199_5:
        cmp r15b, 0
        jne if_42_23_199_5_end
        if_42_26_199_5_code:
            mov rdi, 1
            exit_42_32_199_5:
                    mov rax, 60
                syscall
            exit_42_32_199_5_end:
        if_42_23_199_5_end:
    assert_199_5_end:
    mov r15, 2
    mov r14, 202
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
    mov dword [rbp + r15 * 4 + 256], -1
    cmp_203_12:
        mov rcx, 4
        mov r13, 203
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rsi, [rbp + 224]
        mov r13, 203
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_203_12:
    assert_203_5:
        if_42_26_203_5:
        cmp_42_26_203_5:
        cmp r15b, 0
        jne if_42_23_203_5_end
        if_42_26_203_5_code:
            mov rdi, 1
            exit_42_32_203_5:
                    mov rax, 60
                syscall
            exit_42_32_203_5_end:
        if_42_23_203_5_end:
    assert_203_5_end:
    mov qword [rbp + 248], 3
    mov r15, qword [rbp + 248]
    mov r14, 206
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
    mov r14, qword [rbp + 248]
    sub r14, 1
    mov r13, 206
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    inv_206_16:
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
        not dword [rbp + r15 * 4 + 224]
    inv_206_16_end:
    not dword [rbp + r15 * 4 + 224]
    cmp_207_12:
    mov r14, qword [rbp + 248]
    mov r13, 207
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 2
    sete r15b
    bool_end_207_12:
    assert_207_5:
        if_42_26_207_5:
        cmp_42_26_207_5:
        cmp r15b, 0
        jne if_42_23_207_5_end
        if_42_26_207_5_code:
            mov rdi, 1
            exit_42_32_207_5:
                    mov rax, 60
                syscall
            exit_42_32_207_5_end:
        if_42_23_207_5_end:
    assert_207_5_end:
    faz_209_5:
        mov r15, 1
        mov r14, 94
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
        mov dword [rbp + r15 * 4 + 224], 254
    faz_209_5_end:
    cmp_210_12:
    mov r14, 1
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp dword [rbp + r14 * 4 + 224], 254
    sete r15b
    bool_end_210_12:
    assert_210_5:
        if_42_26_210_5:
        cmp_42_26_210_5:
        cmp r15b, 0
        jne if_42_23_210_5_end
        if_42_26_210_5_code:
            mov rdi, 1
            exit_42_32_210_5:
                    mov rax, 60
                syscall
            exit_42_32_210_5_end:
        if_42_23_210_5_end:
    assert_210_5_end:
    mov qword [rbp + 288], 3
    mov qword [rbp + 296], 5
    lea r15, [rbp + 288]
    mov qword [rbp + 312], 0
    foo_213_5:
        mov r14, qword [rbp + 312]
        add qword [r15], r14
        add qword [r15], 2
        foo_213_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_213_5
    foo_213_5_end:
    cmp_216_12:
    mov r14, 0
    mov r13, 216
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 3
        add r13, 0
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_216_12:
    assert_216_5:
        if_42_26_216_5:
        cmp_42_26_216_5:
        cmp r15b, 0
        jne if_42_23_216_5_end
        if_42_26_216_5_code:
            mov rdi, 1
            exit_42_32_216_5:
                    mov rax, 60
                syscall
            exit_42_32_216_5_end:
        if_42_23_216_5_end:
    assert_216_5_end:
    cmp_217_12:
    mov r14, 1
    mov r13, 217
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
        mov r13, 5
        add r13, 1
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
    sete r15b
    bool_end_217_12:
    assert_217_5:
        if_42_26_217_5:
        cmp_42_26_217_5:
        cmp r15b, 0
        jne if_42_23_217_5_end
        if_42_26_217_5_code:
            mov rdi, 1
            exit_42_32_217_5:
                    mov rax, 60
                syscall
            exit_42_32_217_5_end:
        if_42_23_217_5_end:
    assert_217_5_end:
    mov qword [rbp + 304], 0
    mov qword [rbp + 312], 0
    fooz_224_5:
        mov qword [rbp + 304], 2
        mov qword [rbp + 312], 11
    fooz_224_5_end:
    cmp_225_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_225_12:
    assert_225_5:
        if_42_26_225_5:
        cmp_42_26_225_5:
        cmp r15b, 0
        jne if_42_23_225_5_end
        if_42_26_225_5_code:
            mov rdi, 1
            exit_42_32_225_5:
                    mov rax, 60
                syscall
            exit_42_32_225_5_end:
        if_42_23_225_5_end:
    assert_225_5_end:
    cmp_226_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_226_12:
    assert_226_5:
        if_42_26_226_5:
        cmp_42_26_226_5:
        cmp r15b, 0
        jne if_42_23_226_5_end
        if_42_26_226_5_code:
            mov rdi, 1
            exit_42_32_226_5:
                    mov rax, 60
                syscall
            exit_42_32_226_5_end:
        if_42_23_226_5_end:
    assert_226_5_end:
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
    cmp_229_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_229_12:
    assert_229_5:
        if_42_26_229_5:
        cmp_42_26_229_5:
        cmp r15b, 0
        jne if_42_23_229_5_end
        if_42_26_229_5_code:
            mov rdi, 1
            exit_42_32_229_5:
                    mov rax, 60
                syscall
            exit_42_32_229_5_end:
        if_42_23_229_5_end:
    assert_229_5_end:
    mov qword [rbp + 320], 3
    cmp_234_12:
        lea rsi, [rbp + 304]
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
        sete r14b
    cmp r14b, 0
    sete r15b
    bool_end_234_12:
    assert_234_5:
        if_42_26_234_5:
        cmp_42_26_234_5:
        cmp r15b, 0
        jne if_42_23_234_5_end
        if_42_26_234_5_code:
            mov rdi, 1
            exit_42_32_234_5:
                    mov rax, 60
                syscall
            exit_42_32_234_5_end:
        if_42_23_234_5_end:
    assert_234_5_end:
    mov qword [rbp + 336], 0
    bar_237_5:
        if_75_8_237_5:
        cmp_75_8_237_5:
        cmp qword [rbp + 336], 0
        jne if_75_5_237_5_end
        if_75_8_237_5_code:
            jmp bar_237_5_end
        if_75_5_237_5_end:
        mov qword [rbp + 336], 255
    bar_237_5_end:
    cmp_238_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_238_12:
    assert_238_5:
        if_42_26_238_5:
        cmp_42_26_238_5:
        cmp r15b, 0
        jne if_42_23_238_5_end
        if_42_26_238_5_code:
            mov rdi, 1
            exit_42_32_238_5:
                    mov rax, 60
                syscall
            exit_42_32_238_5_end:
        if_42_23_238_5_end:
    assert_238_5_end:
    mov qword [rbp + 336], 1
    bar_241_5:
        if_75_8_241_5:
        cmp_75_8_241_5:
        cmp qword [rbp + 336], 0
        jne if_75_5_241_5_end
        if_75_8_241_5_code:
            jmp bar_241_5_end
        if_75_5_241_5_end:
        mov qword [rbp + 336], 255
    bar_241_5_end:
    cmp_242_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_242_12:
    assert_242_5:
        if_42_26_242_5:
        cmp_42_26_242_5:
        cmp r15b, 0
        jne if_42_23_242_5_end
        if_42_26_242_5_code:
            mov rdi, 1
            exit_42_32_242_5:
                    mov rax, 60
                syscall
            exit_42_32_242_5_end:
        if_42_23_242_5_end:
    assert_242_5_end:
    mov qword [rbp + 344], 1
    baz_245_13:
        mov r15, qword [rbp + 344]
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_245_13_end:
    cmp_246_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_246_12:
    assert_246_5:
        if_42_26_246_5:
        cmp_42_26_246_5:
        cmp r15b, 0
        jne if_42_23_246_5_end
        if_42_26_246_5_code:
            mov rdi, 1
            exit_42_32_246_5:
                    mov rax, 60
                syscall
            exit_42_32_246_5_end:
        if_42_23_246_5_end:
    assert_246_5_end:
    baz_248_9:
        mov r15, 1
        imul r15, 2
        mov qword [rbp + 352], r15
    baz_248_9_end:
    cmp_249_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_249_12:
    assert_249_5:
        if_42_26_249_5:
        cmp_42_26_249_5:
        cmp r15b, 0
        jne if_42_23_249_5_end
        if_42_26_249_5_code:
            mov rdi, 1
            exit_42_32_249_5:
                    mov rax, 60
                syscall
            exit_42_32_249_5_end:
        if_42_23_249_5_end:
    assert_249_5_end:
    baz_251_23:
        mov r15, 3
        imul r15, 2
        mov qword [rbp + 360], r15
    baz_251_23_end:
    mov qword [rbp + 368], 0
    cmp_252_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_252_12:
    assert_252_5:
        if_42_26_252_5:
        cmp_42_26_252_5:
        cmp r15b, 0
        jne if_42_23_252_5_end
        if_42_26_252_5_code:
            mov rdi, 1
            exit_42_32_252_5:
                    mov rax, 60
                syscall
            exit_42_32_252_5_end:
        if_42_23_252_5_end:
    assert_252_5_end:
    point_init_254_22:
        mov qword [rbp + 376], -1
        mov qword [rbp + 384], -2
    point_init_254_22_end:
    cmp_255_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_255_12:
    assert_255_5:
        if_42_26_255_5:
        cmp_42_26_255_5:
        cmp r15b, 0
        jne if_42_23_255_5_end
        if_42_26_255_5_code:
            mov rdi, 1
            exit_42_32_255_5:
                    mov rax, 60
                syscall
            exit_42_32_255_5_end:
        if_42_23_255_5_end:
    assert_255_5_end:
    cmp_256_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_256_12:
    assert_256_5:
        if_42_26_256_5:
        cmp_42_26_256_5:
        cmp r15b, 0
        jne if_42_23_256_5_end
        if_42_26_256_5_code:
            mov rdi, 1
            exit_42_32_256_5:
                    mov rax, 60
                syscall
            exit_42_32_256_5_end:
        if_42_23_256_5_end:
    assert_256_5_end:
    mov qword [rbp + 392], 1
    mov qword [rbp + 400], 2
    mov r15, qword [rbp + 392]
    imul r15, 10
    mov qword [rbp + 408], r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
    mov dword [rbp + 424], 16711680
    cmp_262_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_262_12:
    assert_262_5:
        if_42_26_262_5:
        cmp_42_26_262_5:
        cmp r15b, 0
        jne if_42_23_262_5_end
        if_42_26_262_5_code:
            mov rdi, 1
            exit_42_32_262_5:
                    mov rax, 60
                syscall
            exit_42_32_262_5_end:
        if_42_23_262_5_end:
    assert_262_5_end:
    cmp_263_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_263_12:
    assert_263_5:
        if_42_26_263_5:
        cmp_42_26_263_5:
        cmp r15b, 0
        jne if_42_23_263_5_end
        if_42_26_263_5_code:
            mov rdi, 1
            exit_42_32_263_5:
                    mov rax, 60
                syscall
            exit_42_32_263_5_end:
        if_42_23_263_5_end:
    assert_263_5_end:
    cmp_264_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_264_12:
    assert_264_5:
        if_42_26_264_5:
        cmp_42_26_264_5:
        cmp r15b, 0
        jne if_42_23_264_5_end
        if_42_26_264_5_code:
            mov rdi, 1
            exit_42_32_264_5:
                    mov rax, 60
                syscall
            exit_42_32_264_5_end:
        if_42_23_264_5_end:
    assert_264_5_end:
    mov r15, qword [rbp + 392]
    mov qword [rbp + 428], r15
    neg qword [rbp + 428]
    mov r15, qword [rbp + 400]
    mov qword [rbp + 436], r15
    neg qword [rbp + 436]
    mov rax, qword [rbp + 428]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 436]
    mov qword [rbp + 416], rax
    cmp_268_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_268_12:
    assert_268_5:
        if_42_26_268_5:
        cmp_42_26_268_5:
        cmp r15b, 0
        jne if_42_23_268_5_end
        if_42_26_268_5_code:
            mov rdi, 1
            exit_42_32_268_5:
                    mov rax, 60
                syscall
            exit_42_32_268_5_end:
        if_42_23_268_5_end:
    assert_268_5_end:
    cmp_269_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_269_12:
    assert_269_5:
        if_42_26_269_5:
        cmp_42_26_269_5:
        cmp r15b, 0
        jne if_42_23_269_5_end
        if_42_26_269_5_code:
            mov rdi, 1
            exit_42_32_269_5:
                    mov rax, 60
                syscall
            exit_42_32_269_5_end:
        if_42_23_269_5_end:
    assert_269_5_end:
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
    cmp_272_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_272_12:
    assert_272_5:
        if_42_26_272_5:
        cmp_42_26_272_5:
        cmp r15b, 0
        jne if_42_23_272_5_end
        if_42_26_272_5_code:
            mov rdi, 1
            exit_42_32_272_5:
                    mov rax, 60
                syscall
            exit_42_32_272_5_end:
        if_42_23_272_5_end:
    assert_272_5_end:
    cmp_273_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_273_12:
    assert_273_5:
        if_42_26_273_5:
        cmp_42_26_273_5:
        cmp r15b, 0
        jne if_42_23_273_5_end
        if_42_26_273_5_code:
            mov rdi, 1
            exit_42_32_273_5:
                    mov rax, 60
                syscall
            exit_42_32_273_5_end:
        if_42_23_273_5_end:
    assert_273_5_end:
    cmp_274_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_274_12:
    assert_274_5:
        if_42_26_274_5:
        cmp_42_26_274_5:
        cmp r15b, 0
        jne if_42_23_274_5_end
        if_42_26_274_5_code:
            mov rdi, 1
            exit_42_32_274_5:
                    mov rax, 60
                syscall
            exit_42_32_274_5_end:
        if_42_23_274_5_end:
    assert_274_5_end:
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
    mov qword [rbp + 472], 73
    cmp_280_12:
    lea r14, [rbp + 464]
    mov r13, 0
    mov r12, 280
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 73
    sete r15b
    bool_end_280_12:
    assert_280_5:
        if_42_26_280_5:
        cmp_42_26_280_5:
        cmp r15b, 0
        jne if_42_23_280_5_end
        if_42_26_280_5_code:
            mov rdi, 1
            exit_42_32_280_5:
                    mov rax, 60
                syscall
            exit_42_32_280_5_end:
        if_42_23_280_5_end:
    assert_280_5_end:
    lea r15, [rbp + 464]
    mov r14, 1
    mov r13, 282
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    imul r14, 20
    add r15, r14
    object_init_282_13:
        mov qword [r15 + 8], 74
    object_init_282_13_end:
    cmp_283_12:
    lea r14, [rbp + 464]
    mov r13, 1
    mov r12, 283
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
    imul r13, 20
    add r14, r13
    cmp qword [r14 + 8], 74
    sete r15b
    bool_end_283_12:
    assert_283_5:
        if_42_26_283_5:
        cmp_42_26_283_5:
        cmp r15b, 0
        jne if_42_23_283_5_end
        if_42_26_283_5_code:
            mov rdi, 1
            exit_42_32_283_5:
                    mov rax, 60
                syscall
            exit_42_32_283_5_end:
        if_42_23_283_5_end:
    assert_283_5_end:
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 1
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    mov qword [r15 + r14 * 8], 65518
    cmp_287_12:
    lea r14, [rbp + 504]
    mov r13, 1
    mov r12, 287
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 287
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_287_12:
    assert_287_5:
        if_42_26_287_5:
        cmp_42_26_287_5:
        cmp r15b, 0
        jne if_42_23_287_5_end
        if_42_26_287_5_code:
            mov rdi, 1
            exit_42_32_287_5:
                    mov rax, 60
                syscall
            exit_42_32_287_5_end:
        if_42_23_287_5_end:
    assert_287_5_end:
    mov rcx, 8
    lea r15, [rbp + 504]
    mov r14, 1
    mov r13, 290
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 290
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rsi, [r15]
    lea r15, [rbp + 504]
    mov r14, 0
    mov r13, 291
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
    shl r14, 6
    add r15, r14
    mov r14, 291
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
    lea rdi, [r15]
    shl rcx, 3
    rep movsb
    cmp_297_12:
    lea r14, [rbp + 504]
    mov r13, 0
    mov r12, 297
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    shl r13, 6
    add r14, r13
    mov r13, 1
    mov r12, 297
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
    cmp qword [r14 + r13 * 8], 65518
    sete r15b
    bool_end_297_12:
    assert_297_5:
        if_42_26_297_5:
        cmp_42_26_297_5:
        cmp r15b, 0
        jne if_42_23_297_5_end
        if_42_26_297_5_code:
            mov rdi, 1
            exit_42_32_297_5:
                    mov rax, 60
                syscall
            exit_42_32_297_5_end:
        if_42_23_297_5_end:
    assert_297_5_end:
    cmp_298_12:
        mov rcx, 8
        lea r13, [rbp + 504]
        mov r12, 0
        mov r10, 299
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 299
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rsi, [r13]
        lea r13, [rbp + 504]
        mov r12, 1
        mov r10, 300
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
        shl r12, 6
        add r13, r12
        mov r12, 300
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
        lea rdi, [r13]
        shl rcx, 3
        repe cmpsb
        sete r14b
    cmp r14b, 0
    setne r15b
    bool_end_298_12:
    assert_298_5:
        if_42_26_298_5:
        cmp_42_26_298_5:
        cmp r15b, 0
        jne if_42_23_298_5_end
        if_42_26_298_5_code:
            mov rdi, 1
            exit_42_32_298_5:
                    mov rax, 60
                syscall
            exit_42_32_298_5_end:
        if_42_23_298_5_end:
    assert_298_5_end:
    mov qword [rbp + 1016], -1
    mov qword [rbp + 1024], 2
    cmp_305_12:
        mov r14, 2
    cmp r14, 2
    sete r15b
    bool_end_305_12:
    assert_305_5:
        if_42_26_305_5:
        cmp_42_26_305_5:
        cmp r15b, 0
        jne if_42_23_305_5_end
        if_42_26_305_5_code:
            mov rdi, 1
            exit_42_32_305_5:
                    mov rax, 60
                syscall
            exit_42_32_305_5_end:
        if_42_23_305_5_end:
    assert_305_5_end:
    cmp_306_12:
    mov r14, 0
    mov r13, 306
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], -1
    sete r15b
    bool_end_306_12:
    assert_306_5:
        if_42_26_306_5:
        cmp_42_26_306_5:
        cmp r15b, 0
        jne if_42_23_306_5_end
        if_42_26_306_5_code:
            mov rdi, 1
            exit_42_32_306_5:
                    mov rax, 60
                syscall
            exit_42_32_306_5_end:
        if_42_23_306_5_end:
    assert_306_5_end:
    cmp_307_12:
    mov r14, 1
    mov r13, 307
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
    cmp qword [rbp + r14 * 8 + 1016], 2
    sete r15b
    bool_end_307_12:
    assert_307_5:
        if_42_26_307_5:
        cmp_42_26_307_5:
        cmp r15b, 0
        jne if_42_23_307_5_end
        if_42_26_307_5_code:
            mov rdi, 1
            exit_42_32_307_5:
                    mov rax, 60
                syscall
            exit_42_32_307_5_end:
        if_42_23_307_5_end:
    assert_307_5_end:
    mov qword [rbp + 1032], 0
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
    print_311_5:
        mov rdx, 21
        lea rsi, [rbp]
        sys_print_53_4_311_5:
                mov rax, 1
                mov rdi, 0
            syscall
        sys_print_53_4_311_5_end:
    print_311_5_end:
    loop_312_5:
        add qword [rbp + 1032], 1
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
        print_315_9:
            mov rdx, 2
            lea rsi, [rbp + 61]
            sys_print_53_4_315_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_53_4_315_9_end:
        print_315_9_end:
        print_316_9:
            mov rdx, 12
            lea rsi, [rbp + 21]
            sys_print_53_4_316_9:
                    mov rax, 1
                    mov rdi, 0
                syscall
            sys_print_53_4_316_9_end:
        print_316_9_end:
        str_in_317_9:
                mov rax, 0
                mov rdi, 0
                lea rsi, [rbp + 1041]
                mov rdx, 127
            syscall
                mov byte [rbp + 1040], al
                sub byte [rbp + 1040], 1
        str_in_317_9_end:
        if_318_12:
        cmp_318_12:
        cmp byte [rbp + 1040], 0
        jne if_320_19
        if_318_12_code:
            jmp loop_312_5_end
        jmp if_318_9_end
        if_320_19:
        cmp_320_19:
        cmp byte [rbp + 1040], 4
        jg if_else_318_9
        if_320_19_code:
            print_321_13:
                mov rdx, 20
                lea rsi, [rbp + 33]
                sys_print_53_4_321_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_321_13_end:
            print_321_13_end:
            jmp loop_312_5
        jmp if_318_9_end
        if_else_318_9:
            print_324_13:
                mov rdx, 6
                lea rsi, [rbp + 53]
                sys_print_53_4_324_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_324_13_end:
            print_324_13_end:
            str_out_325_13:
                    mov rax, 1
                    mov rdi, 0
                    lea rsi, [rbp + 1041]
                    movsx rdx, byte [rbp + 1040]
                syscall
            str_out_325_13_end:
            print_326_13:
                mov rdx, 1
                lea rsi, [rbp + 59]
                sys_print_53_4_326_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_326_13_end:
            print_326_13_end:
            print_327_13:
                mov rdx, 1
                lea rsi, [rbp + 60]
                sys_print_53_4_327_13:
                        mov rax, 1
                        mov rdi, 0
                    syscall
                sys_print_53_4_327_13_end:
            print_327_13_end:
        if_318_9_end:
    jmp loop_312_5
    loop_312_5_end:
    mov rax, 60
    mov rdi, 0
    syscall
print_num:
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
    mov r15, qword [rbx]
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
    mov byte [rbx + 36], 0
    if_136_8:
    cmp_136_8:
    cmp qword [rbx + 28], 0
    jge if_136_5_end
    if_136_8_code:
        mov byte [rbx + 36], 1
        neg qword [rbx + 28]
    if_136_5_end:
    mov qword [rbx + 37], 20
    loop_142_5:
        sub qword [rbx + 37], 1
        mov qword [rbx + 45], 48
        mov r15, qword [rbx + 28]
        mov rax, r15
        cqo
        mov r14, 10
        idiv r14
        mov r15, rdx
        add qword [rbx + 45], r15
        mov r15, qword [rbx + 37]
        mov r14, 146
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
        mov rax, qword [rbx + 28]
        cqo
        mov r15, 10
        idiv r15
        mov qword [rbx + 28], rax
        if_148_12:
        cmp_148_12:
        cmp qword [rbx + 28], 0
        jne if_148_9_end
        if_148_12_code:
            jmp loop_142_5_end
        if_148_9_end:
    jmp loop_142_5
    loop_142_5_end:
    if_151_8:
    cmp_151_8:
    cmp byte [rbx + 36], 0
    je if_151_5_end
    if_151_8_code:
        sub qword [rbx + 37], 1
        mov r15, qword [rbx + 37]
        mov r14, 153
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov byte [rbx + r15 + 8], 45
    if_151_5_end:
    mov qword [rbx + 45], 0
    loop_157_5:
        mov r15, qword [rbx + 45]
        mov r14, 158
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
        mov r14, qword [rbx + 37]
        mov r13, 158
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
        add qword [rbx + 45], 1
        add qword [rbx + 37], 1
        if_161_12:
        cmp_161_12:
        cmp qword [rbx + 37], 20
        jne if_161_9_end
        if_161_12_code:
            jmp loop_157_5_end
        if_161_9_end:
    jmp loop_157_5
    loop_157_5_end:
    mov rdx, qword [rbx + 45]
    lea rsi, [rbx + 8]
    sys_print_164_5:
            mov rax, 1
            mov rdi, 0
        syscall
    sys_print_164_5_end:
    ret
print_num.size equ 53
baz_bounds_panic:
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
db `hello world from baz\n`
db `enter name:\n`
db `that is not a name.\n`
db `hello `
db `.`
db `\n`
db `: `
dq 1
times 24 db 0
db 3
times 127 db 0
dat.end:
section .bss.vars nobits alloc write
align 16
vars:
vars resb 131072
vars.end:
```

## With comments

```nasm

; allocate named register rbp
;
; generated by baz
;

default rel

%macro PUSH_REGS 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

section .text
bits 64
global _start
_start:
lea rbp, [dat]

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
; 
;[9:1] object : 20 B    fields:
;[9:1]       name :  offset :    size :  array? : array size
;[9:1]        pos :       0 :      16 :      no :           
;[9:1]      color :      16 :       4 :      no :           
; 
;[11:1] world : 64 B    fields:
;[11:1]       name :  offset :    size :  array? : array size
;[11:1]  locations :       0 :      64 :     yes :          8
; 
;[13:1] str : 128 B    fields:
;[13:1]       name :  offset :    size :  array? : array size
;[13:1]        len :       0 :       1 :      no :           
;[13:1]       data :       1 :     127 :     yes :        127
; 
;[18:1] # initial data is initialized before variables
;[20:1] dat hello : i8[] = "hello world from baz\n"
;[20:7] hello: i8[21] (21 B @ [rbp])
;[21:1] dat prompt1 : i8[] = "enter name:\n"
;[21:5] prompt1: i8[12] (12 B @ [rbp + 21])
;[22:1] dat prompt2 : i8[] = "that is not a name.\n"
;[22:5] prompt2: i8[20] (20 B @ [rbp + 33])
;[23:1] dat prompt3 : i8[] = "hello "
;[23:5] prompt3: i8[6] (6 B @ [rbp + 53])
;[24:1] dat dot : i8[] = "."
;[24:9] dot: i8[1] (1 B @ [rbp + 59])
;[25:1] dat nl : i8[] = "\n"
;[25:10] nl: i8[1] (1 B @ [rbp + 60])
;[26:1] dat colon : i8[] = ": "
;[26:7] colon: i8[2] (2 B @ [rbp + 61])
;[27:1] dat nums : i64[4] = { 1 }
;[27:8] nums: i64[4] (32 B @ [rbp + 63])
;[27:30] # remaining elements are zeroed
;[28:1] dat s1 : str = { 3 }
;[28:10] s1: str (128 B @ [rbp + 95])
;[28:27] # remaining fields are zeroed
;[30:1] # default is to inline functions
;[32:1] # arguments can be placed in specified register using `reg_...` syntax
;[40:1] # single statement blocks can ommit { ... }
;[65:1] # function arguments and return are equivalent to mutable references
;[72:1] # default argument type is `i64`
;[79:1] # return target is specified as a variable, in this case `res`
;[81:1] # return variable is a mutable reference to destination
;[91:1] # array arguments are declared with type and []
;[123:7] const yes = 1
;[124:7] const no = 0
;[125:7] const maybe = -1
;[127:1] # constants can be declared in any scope and shadow outer declarations
;[129:1] # limited support for non-inlined functions
; 
main:
;   [168:5] var arr : i32[4]
;   [168:9] arr: i32[4] (16 B @ [rbp + 224])
;   [168:9] zero 4 * 4 B = 16 B
;   [168:5] size <= 32 B, use mov
    mov qword [rbp + 224], 0
    mov qword [rbp + 232], 0
;   [169:5] # arrays are initialized to 0
;   [171:5] var answer
;   [171:9] answer: i64 (8 B @ [rbp + 240])
;   [171:9] zero 1 * 8 B = 8 B
;   [171:5] size <= 32 B, use mov
    mov qword [rbp + 240], 0
;   [172:5] assert(answer == 0)
;   [172:12] allocate scratch register -> r15
;   [172:12] ? answer == 0
;   [172:12] ? answer == 0
    cmp_172_12:
    cmp qword [rbp + 240], 0
    sete r15b
    bool_end_172_12:
;   [42:6] assert(x : bool)
    assert_172_5:
;       [172:5] alias x -> r15b
        if_42_26_172_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_172_5:
        cmp r15b, 0
        jne if_42_23_172_5_end
        if_42_26_172_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_172_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_172_5_end:
        if_42_23_172_5_end:
;       [172:5] free scratch register r15
    assert_172_5_end:
;   [173:5] # variables without initializer are zeroed
;   [175:5] answer = maybe
;   [175:14] maybe
    mov qword [rbp + 240], -1
;   [176:5] assert(answer == -1)
;   [176:12] allocate scratch register -> r15
;   [176:12] ? answer == -1
;   [176:12] ? answer == -1
    cmp_176_12:
    cmp qword [rbp + 240], -1
    sete r15b
    bool_end_176_12:
;   [42:6] assert(x : bool)
    assert_176_5:
;       [176:5] alias x -> r15b
        if_42_26_176_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_176_5:
        cmp r15b, 0
        jne if_42_23_176_5_end
        if_42_26_176_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_176_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_176_5_end:
        if_42_23_176_5_end:
;       [176:5] free scratch register r15
    assert_176_5_end:
;       [179:15] const maybe = 33
;       [180:9] assert(maybe == 33)
;       [180:16] allocate scratch register -> r15
;       [180:16] ? maybe == 33
;       [180:16] ? maybe == 33
        cmp_180_16:
;       [180:16] const eval to true
        bool_end_180_16:
        mov r15b, 1
;       [42:6] assert(x : bool)
        assert_180_9:
;           [180:9] alias x -> r15b
            if_42_26_180_9:
;           [42:26] ? not x
;           [42:26] ? not x
            cmp_42_26_180_9:
            cmp r15b, 0
            jne if_42_23_180_9_end
            if_42_26_180_9_code:
;               [42:32] exit(1)
;               [42:37] allocate named register rdi
                mov rdi, 1
;               [34:6] exit(v : reg_rdi)
                exit_42_32_180_9:
;                   [42:32] alias v -> rdi
;                   [35:5] mov(rax, 60)
;                       [35:14] 60
                        mov rax, 60
;                   [35:19] # exit system call
;                   [36:5] mov(rdi, v)
;                       [36:14] v
;                   [36:19] # return code
;                   [37:5] syscall()
                    syscall
;                   [42:32] free named register rdi
                exit_42_32_180_9_end:
            if_42_23_180_9_end:
;           [180:9] free scratch register r15
        assert_180_9_end:
;   [183:5] assert(maybe == -1)
;   [183:12] allocate scratch register -> r15
;   [183:12] ? maybe == -1
;   [183:12] ? maybe == -1
    cmp_183_12:
;   [183:12] const eval to true
    bool_end_183_12:
    mov r15b, 1
;   [42:6] assert(x : bool)
    assert_183_5:
;       [183:5] alias x -> r15b
        if_42_26_183_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_183_5:
        cmp r15b, 0
        jne if_42_23_183_5_end
        if_42_26_183_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_183_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_183_5_end:
        if_42_23_183_5_end:
;       [183:5] free scratch register r15
    assert_183_5_end:
;   [185:5] var ix = 1
;   [185:9] ix: i64 (8 B @ [rbp + 248])
;   [185:9] ix = 1
;   [185:14] 1
    mov qword [rbp + 248], 1
;   [186:5] # variables can have an initial value that can be an expression
;   [188:5] arr[ix] = 2
;   [188:5] allocate scratch register -> r15
;   [188:9] set array index
;   [188:9] ix
    mov r15, qword [rbp + 248]
;   [188:9] bounds check
;   [188:9] allocate scratch register -> r14
;   [188:9] line number
    mov r14, 188
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [188:9] free scratch register r14
;   [188:15] 2
    mov dword [rbp + r15 * 4 + 224], 2
;   [188:5] free scratch register r15
;   [189:5] arr[ix + 1] = arr[ix]
;   [189:5] allocate scratch register -> r15
;   [189:9] set array index
;   [189:9] ix
    mov r15, qword [rbp + 248]
;   [189:14] r15 + 1
    add r15, 1
;   [189:9] bounds check
;   [189:9] allocate scratch register -> r14
;   [189:9] line number
    mov r14, 189
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [189:9] free scratch register r14
;   [189:19] arr[ix]
;   [189:19] allocate scratch register -> r14
;   [189:23] set array index
;   [189:23] ix
    mov r14, qword [rbp + 248]
;   [189:23] bounds check
;   [189:23] allocate scratch register -> r13
;   [189:23] line number
    mov r13, 189
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [189:23] free scratch register r13
;   [189:19] allocate scratch register -> r13
    mov r13d, dword [rbp + r14 * 4 + 224]
    mov dword [rbp + r15 * 4 + 224], r13d
;   [189:19] free scratch register r13
;   [189:19] free scratch register r14
;   [189:5] free scratch register r15
;   [190:5] assert(arr[1] == 2)
;   [190:12] allocate scratch register -> r15
;   [190:12] ? arr[1] == 2
;   [190:12] ? arr[1] == 2
    cmp_190_12:
;   [190:12] allocate scratch register -> r14
;   [190:16] set array index
;   [190:16] 1
    mov r14, 1
;   [190:16] bounds check
;   [190:16] allocate scratch register -> r13
;   [190:16] line number
    mov r13, 190
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [190:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [190:12] free scratch register r14
    sete r15b
    bool_end_190_12:
;   [42:6] assert(x : bool)
    assert_190_5:
;       [190:5] alias x -> r15b
        if_42_26_190_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_190_5:
        cmp r15b, 0
        jne if_42_23_190_5_end
        if_42_26_190_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_190_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_190_5_end:
        if_42_23_190_5_end:
;       [190:5] free scratch register r15
    assert_190_5_end:
;   [191:5] assert(arr[2] == 2)
;   [191:12] allocate scratch register -> r15
;   [191:12] ? arr[2] == 2
;   [191:12] ? arr[2] == 2
    cmp_191_12:
;   [191:12] allocate scratch register -> r14
;   [191:16] set array index
;   [191:16] 2
    mov r14, 2
;   [191:16] bounds check
;   [191:16] allocate scratch register -> r13
;   [191:16] line number
    mov r13, 191
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [191:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [191:12] free scratch register r14
    sete r15b
    bool_end_191_12:
;   [42:6] assert(x : bool)
    assert_191_5:
;       [191:5] alias x -> r15b
        if_42_26_191_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_191_5:
        cmp r15b, 0
        jne if_42_23_191_5_end
        if_42_26_191_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_191_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_191_5_end:
        if_42_23_191_5_end:
;       [191:5] free scratch register r15
    assert_191_5_end:
;   [193:5] array_copy(arr[2], arr, 2)
;   [193:5] allocate named register rsi
;   [193:5] allocate named register rdi
;   [193:5] allocate named register rcx
;   [193:29] 2
;   [193:29] 2
    mov rcx, 2
;   [193:16] arr[2]
;   [193:16] allocate scratch register -> r15
;   [193:20] set array index
;   [193:20] 2
    mov r15, 2
;   [193:20] bounds check
;   [193:20] allocate scratch register -> r14
;   [193:20] line number
    mov r14, 193
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
;   [193:20] allocate scratch register -> r13
    mov r13, rcx
    add r13, r15
    cmp r13, 4
;   [193:20] free scratch register r13
    cmovg rbp, r14
    jg baz_bounds_panic
;   [193:20] free scratch register r14
    lea rsi, [rbp + r15 * 4 + 224]
;   [193:5] free scratch register r15
;   [193:24] arr
;   [193:24] bounds check
;   [193:24] allocate scratch register -> r15
;   [193:24] line number
    mov r15, 193
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [193:24] free scratch register r15
    lea rdi, [rbp + 224]
    shl rcx, 2
    rep movsb
;   [193:5] free named register rcx
;   [193:5] free named register rdi
;   [193:5] free named register rsi
;   [194:5] # copy from, to, number of elements
;   [195:5] assert(arr[0] == 2)
;   [195:12] allocate scratch register -> r15
;   [195:12] ? arr[0] == 2
;   [195:12] ? arr[0] == 2
    cmp_195_12:
;   [195:12] allocate scratch register -> r14
;   [195:16] set array index
;   [195:16] 0
    mov r14, 0
;   [195:16] bounds check
;   [195:16] allocate scratch register -> r13
;   [195:16] line number
    mov r13, 195
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [195:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [195:12] free scratch register r14
    sete r15b
    bool_end_195_12:
;   [42:6] assert(x : bool)
    assert_195_5:
;       [195:5] alias x -> r15b
        if_42_26_195_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_195_5:
        cmp r15b, 0
        jne if_42_23_195_5_end
        if_42_26_195_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_195_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_195_5_end:
        if_42_23_195_5_end:
;       [195:5] free scratch register r15
    assert_195_5_end:
;   [197:5] var arr1 : i32[8]
;   [197:9] arr1: i32[8] (32 B @ [rbp + 256])
;   [197:9] zero 8 * 4 B = 32 B
;   [197:5] size <= 32 B, use mov
    mov qword [rbp + 256], 0
    mov qword [rbp + 264], 0
    mov qword [rbp + 272], 0
    mov qword [rbp + 280], 0
;   [198:5] array_copy(arr, arr1, 4)
;   [198:5] allocate named register rsi
;   [198:5] allocate named register rdi
;   [198:5] allocate named register rcx
;   [198:27] 4
;   [198:27] 4
    mov rcx, 4
;   [198:16] arr
;   [198:16] bounds check
;   [198:16] allocate scratch register -> r15
;   [198:16] line number
    mov r15, 198
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 4
    cmovg rbp, r15
    jg baz_bounds_panic
;   [198:16] free scratch register r15
    lea rsi, [rbp + 224]
;   [198:21] arr1
;   [198:21] bounds check
;   [198:21] allocate scratch register -> r15
;   [198:21] line number
    mov r15, 198
    test rcx, rcx
    cmovs rbp, r15
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r15
    jg baz_bounds_panic
;   [198:21] free scratch register r15
    lea rdi, [rbp + 256]
    shl rcx, 2
    rep movsb
;   [198:5] free named register rcx
;   [198:5] free named register rdi
;   [198:5] free named register rsi
;   [199:5] assert(arrays_equal(arr, arr1, 4))
;   [199:12] allocate scratch register -> r15
;   [199:12] ? arrays_equal(arr, arr1, 4)
;   [199:12] ? arrays_equal(arr, arr1, 4)
    cmp_199_12:
;   [199:12] allocate scratch register -> r14
;       [199:12] r14b = arrays_equal(arr, arr1, 4)
;       [199:12] = expression
;       [199:12] arrays_equal(arr, arr1, 4)
;       [199:12] allocate named register rsi
;       [199:12] allocate named register rdi
;       [199:12] allocate named register rcx
;       [199:36] 4
;       [199:36] 4
        mov rcx, 4
;       [199:25] arr
;       [199:25] bounds check
;       [199:25] allocate scratch register -> r13
;       [199:25] line number
        mov r13, 199
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [199:25] free scratch register r13
        lea rsi, [rbp + 224]
;       [199:30] arr1
;       [199:30] bounds check
;       [199:30] allocate scratch register -> r13
;       [199:30] line number
        mov r13, 199
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [199:30] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [199:12] free named register rcx
;       [199:12] free named register rdi
;       [199:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [199:12] free scratch register r14
    setne r15b
    bool_end_199_12:
;   [42:6] assert(x : bool)
    assert_199_5:
;       [199:5] alias x -> r15b
        if_42_26_199_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_199_5:
        cmp r15b, 0
        jne if_42_23_199_5_end
        if_42_26_199_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_199_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_199_5_end:
        if_42_23_199_5_end:
;       [199:5] free scratch register r15
    assert_199_5_end:
;   [200:5] # `arrays_equal` is built-in function
;   [202:5] arr1[2] = -1
;   [202:5] allocate scratch register -> r15
;   [202:10] set array index
;   [202:10] 2
    mov r15, 2
;   [202:10] bounds check
;   [202:10] allocate scratch register -> r14
;   [202:10] line number
    mov r14, 202
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 8
    cmovge rbp, r14
    jge baz_bounds_panic
;   [202:10] free scratch register r14
;   [202:16] -1
    mov dword [rbp + r15 * 4 + 256], -1
;   [202:5] free scratch register r15
;   [203:5] assert(not arrays_equal(arr, arr1, 4))
;   [203:12] allocate scratch register -> r15
;   [203:12] ? not arrays_equal(arr, arr1, 4)
;   [203:12] ? not arrays_equal(arr, arr1, 4)
    cmp_203_12:
;   [203:16] allocate scratch register -> r14
;       [203:16] r14b = arrays_equal(arr, arr1, 4)
;       [203:16] = expression
;       [203:16] arrays_equal(arr, arr1, 4)
;       [203:16] allocate named register rsi
;       [203:16] allocate named register rdi
;       [203:16] allocate named register rcx
;       [203:40] 4
;       [203:40] 4
        mov rcx, 4
;       [203:29] arr
;       [203:29] bounds check
;       [203:29] allocate scratch register -> r13
;       [203:29] line number
        mov r13, 203
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 4
        cmovg rbp, r13
        jg baz_bounds_panic
;       [203:29] free scratch register r13
        lea rsi, [rbp + 224]
;       [203:34] arr1
;       [203:34] bounds check
;       [203:34] allocate scratch register -> r13
;       [203:34] line number
        mov r13, 203
        test rcx, rcx
        cmovs rbp, r13
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r13
        jg baz_bounds_panic
;       [203:34] free scratch register r13
        lea rdi, [rbp + 256]
        shl rcx, 2
        repe cmpsb
;       [203:16] free named register rcx
;       [203:16] free named register rdi
;       [203:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [203:12] free scratch register r14
    sete r15b
    bool_end_203_12:
;   [42:6] assert(x : bool)
    assert_203_5:
;       [203:5] alias x -> r15b
        if_42_26_203_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_203_5:
        cmp r15b, 0
        jne if_42_23_203_5_end
        if_42_26_203_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_203_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_203_5_end:
        if_42_23_203_5_end:
;       [203:5] free scratch register r15
    assert_203_5_end:
;   [205:5] ix = 3
;   [205:10] 3
    mov qword [rbp + 248], 3
;   [206:5] arr[ix] = ~inv(arr[ix - 1])
;   [206:5] allocate scratch register -> r15
;   [206:9] set array index
;   [206:9] ix
    mov r15, qword [rbp + 248]
;   [206:9] bounds check
;   [206:9] allocate scratch register -> r14
;   [206:9] line number
    mov r14, 206
    test r15, r15
    cmovs rbp, r14
    js baz_bounds_panic
    cmp r15, 4
    cmovge rbp, r14
    jge baz_bounds_panic
;   [206:9] free scratch register r14
;   [206:16] arr = ~inv(arr[ix - 1])
;   [206:16] = expression
;   [206:16] ~inv(arr[ix - 1])
;   [206:20] allocate scratch register -> r14
;   [206:24] set array index
;   [206:24] ix
    mov r14, qword [rbp + 248]
;   [206:29] r14 - 1
    sub r14, 1
;   [206:24] bounds check
;   [206:24] allocate scratch register -> r13
;   [206:24] line number
    mov r13, 206
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [206:24] free scratch register r13
;   [83:6] inv(i : i32) : i32 res
    inv_206_16:
;       [206:16] alias res -> arr (lea: rbp + r15 * 4 + 224)
;       [206:16] alias i -> arr (lea: rbp + r14 * 4 + 224)
;       [84:5] res = ~i
;       [84:12] ~i
;       [84:12] allocate scratch register -> r13
        mov r13d, dword [rbp + r14 * 4 + 224]
        mov dword [rbp + r15 * 4 + 224], r13d
;       [84:12] free scratch register r13
        not dword [rbp + r15 * 4 + 224]
;       [206:16] free scratch register r14
    inv_206_16_end:
    not dword [rbp + r15 * 4 + 224]
;   [206:5] free scratch register r15
;   [207:5] assert(arr[ix] == 2)
;   [207:12] allocate scratch register -> r15
;   [207:12] ? arr[ix] == 2
;   [207:12] ? arr[ix] == 2
    cmp_207_12:
;   [207:12] allocate scratch register -> r14
;   [207:16] set array index
;   [207:16] ix
    mov r14, qword [rbp + 248]
;   [207:16] bounds check
;   [207:16] allocate scratch register -> r13
;   [207:16] line number
    mov r13, 207
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [207:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 2
;   [207:12] free scratch register r14
    sete r15b
    bool_end_207_12:
;   [42:6] assert(x : bool)
    assert_207_5:
;       [207:5] alias x -> r15b
        if_42_26_207_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_207_5:
        cmp r15b, 0
        jne if_42_23_207_5_end
        if_42_26_207_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_207_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_207_5_end:
        if_42_23_207_5_end:
;       [207:5] free scratch register r15
    assert_207_5_end:
;   [209:5] faz(arr)
;   [93:6] faz(arg : i32[])
    faz_209_5:
;       [209:5] alias arg -> arr
;       [94:5] arg[1] = 0xfe
;       [94:5] allocate scratch register -> r15
;       [94:9] set array index
;       [94:9] 1
        mov r15, 1
;       [94:9] bounds check
;       [94:9] allocate scratch register -> r14
;       [94:9] line number
        mov r14, 94
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 4
        cmovge rbp, r14
        jge baz_bounds_panic
;       [94:9] free scratch register r14
;       [94:14] 0xfe
        mov dword [rbp + r15 * 4 + 224], 254
;       [94:5] free scratch register r15
    faz_209_5_end:
;   [210:5] assert(arr[1] == 0xfe)
;   [210:12] allocate scratch register -> r15
;   [210:12] ? arr[1] == 0xfe
;   [210:12] ? arr[1] == 0xfe
    cmp_210_12:
;   [210:12] allocate scratch register -> r14
;   [210:16] set array index
;   [210:16] 1
    mov r14, 1
;   [210:16] bounds check
;   [210:16] allocate scratch register -> r13
;   [210:16] line number
    mov r13, 210
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 4
    cmovge rbp, r13
    jge baz_bounds_panic
;   [210:16] free scratch register r13
    cmp dword [rbp + r14 * 4 + 224], 254
;   [210:12] free scratch register r14
    sete r15b
    bool_end_210_12:
;   [42:6] assert(x : bool)
    assert_210_5:
;       [210:5] alias x -> r15b
        if_42_26_210_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_210_5:
        cmp r15b, 0
        jne if_42_23_210_5_end
        if_42_26_210_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_210_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_210_5_end:
        if_42_23_210_5_end:
;       [210:5] free scratch register r15
    assert_210_5_end:
;   [212:5] var arr3 : i64[] = { 3, 5 }
;   [212:9] arr3: i64[2] (16 B @ [rbp + 288])
;   [212:9] arr3 = { 3, 5 }
;   [212:26] [0]
;   [212:26] 3
    mov qword [rbp + 288], 3
;   [212:26] [1]
;   [212:29] 5
    mov qword [rbp + 296], 5
;   [213:5] foo arr3
;   [213:9] allocate scratch register -> r15
;   [213:9] e: i64 (r15)
;   [213:9] i: i64 (8 B @ [rbp + 312])
;   [213:9] const n = 2
;   [213:9] initiate iterator e
    lea r15, [rbp + 288]
;   [213:9] initiate counter i
    mov qword [rbp + 312], 0
    foo_213_5:
;       [214:9] e = e + i + n
;       [214:13] instructions without scratch register 3, with 4
;       [214:13] e
;       [214:17] e + i
;       [214:17] allocate scratch register -> r14
        mov r14, qword [rbp + 312]
        add qword [r15], r14
;       [214:17] free scratch register r14
;       [214:21] e + n
        add qword [r15], 2
        foo_213_5_continue:
            add r15, 8
            inc qword [rbp + 312]
            cmp qword [rbp + 312], 2
            jne foo_213_5
    foo_213_5_end:
;   [213:5] free scratch register r15
;   [216:5] assert(arr3[0] == 3 + 0 + 2)
;   [216:12] allocate scratch register -> r15
;   [216:12] ? arr3[0] == 3 + 0 + 2
;   [216:12] ? arr3[0] == 3 + 0 + 2
    cmp_216_12:
;   [216:12] allocate scratch register -> r14
;   [216:17] set array index
;   [216:17] 0
    mov r14, 0
;   [216:17] bounds check
;   [216:17] allocate scratch register -> r13
;   [216:17] line number
    mov r13, 216
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [216:17] free scratch register r13
;   [216:23] allocate scratch register -> r13
;       [216:23] 3
        mov r13, 3
;       [216:27] r13 + 0
        add r13, 0
;       [216:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [216:12] free scratch register r13
;   [216:12] free scratch register r14
    sete r15b
    bool_end_216_12:
;   [42:6] assert(x : bool)
    assert_216_5:
;       [216:5] alias x -> r15b
        if_42_26_216_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_216_5:
        cmp r15b, 0
        jne if_42_23_216_5_end
        if_42_26_216_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_216_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_216_5_end:
        if_42_23_216_5_end:
;       [216:5] free scratch register r15
    assert_216_5_end:
;   [217:5] assert(arr3[1] == 5 + 1 + 2)
;   [217:12] allocate scratch register -> r15
;   [217:12] ? arr3[1] == 5 + 1 + 2
;   [217:12] ? arr3[1] == 5 + 1 + 2
    cmp_217_12:
;   [217:12] allocate scratch register -> r14
;   [217:17] set array index
;   [217:17] 1
    mov r14, 1
;   [217:17] bounds check
;   [217:17] allocate scratch register -> r13
;   [217:17] line number
    mov r13, 217
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [217:17] free scratch register r13
;   [217:23] allocate scratch register -> r13
;       [217:23] 5
        mov r13, 5
;       [217:27] r13 + 1
        add r13, 1
;       [217:31] r13 + 2
        add r13, 2
    cmp qword [rbp + r14 * 8 + 288], r13
;   [217:12] free scratch register r13
;   [217:12] free scratch register r14
    sete r15b
    bool_end_217_12:
;   [42:6] assert(x : bool)
    assert_217_5:
;       [217:5] alias x -> r15b
        if_42_26_217_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_217_5:
        cmp r15b, 0
        jne if_42_23_217_5_end
        if_42_26_217_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_217_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_217_5_end:
        if_42_23_217_5_end:
;       [217:5] free scratch register r15
    assert_217_5_end:
;   [218:5] # `foo` is a language construct that iterates over an array injecting:
;   [219:5] # `e`: current element
;   [220:5] # `i`: index starting at 0
;   [221:5] # `n`: constant array size
;   [223:5] var p : point = {0, 0}
;   [223:9] p: point (16 B @ [rbp + 304])
;   [223:9] p = {0, 0}
;   [223:22] copy field 'x'
    mov qword [rbp + 304], 0
;   [223:25] copy field 'y'
    mov qword [rbp + 312], 0
;   [224:5] fooz(p)
;   [67:6] fooz(pt : point)
    fooz_224_5:
;       [224:5] alias pt -> p
;       [68:5] pt.x = 0b10
;       [68:12] 0b10
        mov qword [rbp + 304], 2
;       [68:20] # binary value 2
;       [69:5] pt.y = 0xb
;       [69:12] 0xb
        mov qword [rbp + 312], 11
;       [69:20] # hex value 11
    fooz_224_5_end:
;   [225:5] assert(p.x == 2)
;   [225:12] allocate scratch register -> r15
;   [225:12] ? p.x == 2
;   [225:12] ? p.x == 2
    cmp_225_12:
    cmp qword [rbp + 304], 2
    sete r15b
    bool_end_225_12:
;   [42:6] assert(x : bool)
    assert_225_5:
;       [225:5] alias x -> r15b
        if_42_26_225_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_225_5:
        cmp r15b, 0
        jne if_42_23_225_5_end
        if_42_26_225_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_225_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_225_5_end:
        if_42_23_225_5_end:
;       [225:5] free scratch register r15
    assert_225_5_end:
;   [226:5] assert(p.y == 0xb)
;   [226:12] allocate scratch register -> r15
;   [226:12] ? p.y == 0xb
;   [226:12] ? p.y == 0xb
    cmp_226_12:
    cmp qword [rbp + 312], 11
    sete r15b
    bool_end_226_12:
;   [42:6] assert(x : bool)
    assert_226_5:
;       [226:5] alias x -> r15b
        if_42_26_226_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_226_5:
        cmp r15b, 0
        jne if_42_23_226_5_end
        if_42_26_226_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_226_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_226_5_end:
        if_42_23_226_5_end:
;       [226:5] free scratch register r15
    assert_226_5_end:
;   [228:5] var q : point = p
;   [228:9] q: point (16 B @ [rbp + 320])
;   [228:9] q = p
;   [228:21] size <= 16 B, use mov
;   [228:21] allocate named register rax
    mov rax, qword [rbp + 304]
    mov qword [rbp + 320], rax
    mov rax, qword [rbp + 312]
    mov qword [rbp + 328], rax
;   [228:21] free named register rax
;   [229:5] assert(equal(p, q))
;   [229:12] allocate scratch register -> r15
;   [229:12] ? equal(p, q)
;   [229:12] ? equal(p, q)
    cmp_229_12:
;   [229:12] allocate scratch register -> r14
;       [229:12] r14b = equal(p, q)
;       [229:12] = expression
;       [229:12] equal(p, q)
;       [229:12] allocate named register rsi
;       [229:12] allocate named register rdi
;       [229:12] allocate named register rcx
;       [229:18] p
        lea rsi, [rbp + 304]
;       [229:21] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [229:12] free named register rcx
;       [229:12] free named register rdi
;       [229:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [229:12] free scratch register r14
    setne r15b
    bool_end_229_12:
;   [42:6] assert(x : bool)
    assert_229_5:
;       [229:5] alias x -> r15b
        if_42_26_229_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_229_5:
        cmp r15b, 0
        jne if_42_23_229_5_end
        if_42_26_229_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_229_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_229_5_end:
        if_42_23_229_5_end:
;       [229:5] free scratch register r15
    assert_229_5_end:
;   [230:5] # `equal` is built-in function to compare user types for equality or same
;   [231:5] # size arrays
;   [233:5] q.x = 3
;   [233:11] 3
    mov qword [rbp + 320], 3
;   [234:5] assert(not equal(p, q))
;   [234:12] allocate scratch register -> r15
;   [234:12] ? not equal(p, q)
;   [234:12] ? not equal(p, q)
    cmp_234_12:
;   [234:16] allocate scratch register -> r14
;       [234:16] r14b = equal(p, q)
;       [234:16] = expression
;       [234:16] equal(p, q)
;       [234:16] allocate named register rsi
;       [234:16] allocate named register rdi
;       [234:16] allocate named register rcx
;       [234:22] p
        lea rsi, [rbp + 304]
;       [234:25] q
        lea rdi, [rbp + 320]
        mov rcx, 2
        repe cmpsq
;       [234:16] free named register rcx
;       [234:16] free named register rdi
;       [234:16] free named register rsi
        sete r14b
    cmp r14b, 0
;   [234:12] free scratch register r14
    sete r15b
    bool_end_234_12:
;   [42:6] assert(x : bool)
    assert_234_5:
;       [234:5] alias x -> r15b
        if_42_26_234_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_234_5:
        cmp r15b, 0
        jne if_42_23_234_5_end
        if_42_26_234_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_234_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_234_5_end:
        if_42_23_234_5_end:
;       [234:5] free scratch register r15
    assert_234_5_end:
;   [236:5] var i = 0
;   [236:9] i: i64 (8 B @ [rbp + 336])
;   [236:9] i = 0
;   [236:13] 0
    mov qword [rbp + 336], 0
;   [237:5] bar(i)
;   [74:6] bar(arg)
    bar_237_5:
;       [237:5] alias arg -> i
        if_75_8_237_5:
;       [75:8] ? arg == 0
;       [75:8] ? arg == 0
        cmp_75_8_237_5:
        cmp qword [rbp + 336], 0
        jne if_75_5_237_5_end
        if_75_8_237_5_code:
;           [75:17] return
            jmp bar_237_5_end
        if_75_5_237_5_end:
;       [76:5] arg = 0xff
;       [76:11] 0xff
        mov qword [rbp + 336], 255
    bar_237_5_end:
;   [238:5] assert(i == 0)
;   [238:12] allocate scratch register -> r15
;   [238:12] ? i == 0
;   [238:12] ? i == 0
    cmp_238_12:
    cmp qword [rbp + 336], 0
    sete r15b
    bool_end_238_12:
;   [42:6] assert(x : bool)
    assert_238_5:
;       [238:5] alias x -> r15b
        if_42_26_238_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_238_5:
        cmp r15b, 0
        jne if_42_23_238_5_end
        if_42_26_238_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_238_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_238_5_end:
        if_42_23_238_5_end:
;       [238:5] free scratch register r15
    assert_238_5_end:
;   [240:5] i = 1
;   [240:9] 1
    mov qword [rbp + 336], 1
;   [241:5] bar(i)
;   [74:6] bar(arg)
    bar_241_5:
;       [241:5] alias arg -> i
        if_75_8_241_5:
;       [75:8] ? arg == 0
;       [75:8] ? arg == 0
        cmp_75_8_241_5:
        cmp qword [rbp + 336], 0
        jne if_75_5_241_5_end
        if_75_8_241_5_code:
;           [75:17] return
            jmp bar_241_5_end
        if_75_5_241_5_end:
;       [76:5] arg = 0xff
;       [76:11] 0xff
        mov qword [rbp + 336], 255
    bar_241_5_end:
;   [242:5] assert(i == 0xff)
;   [242:12] allocate scratch register -> r15
;   [242:12] ? i == 0xff
;   [242:12] ? i == 0xff
    cmp_242_12:
    cmp qword [rbp + 336], 255
    sete r15b
    bool_end_242_12:
;   [42:6] assert(x : bool)
    assert_242_5:
;       [242:5] alias x -> r15b
        if_42_26_242_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_242_5:
        cmp r15b, 0
        jne if_42_23_242_5_end
        if_42_26_242_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_242_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_242_5_end:
        if_42_23_242_5_end:
;       [242:5] free scratch register r15
    assert_242_5_end:
;   [244:5] var j = 1
;   [244:9] j: i64 (8 B @ [rbp + 344])
;   [244:9] j = 1
;   [244:13] 1
    mov qword [rbp + 344], 1
;   [245:5] var k = baz(j)
;   [245:9] k: i64 (8 B @ [rbp + 352])
;   [245:9] k = baz(j)
;   [245:13] k = baz(j)
;   [245:13] = expression
;   [245:13] baz(j)
;   [87:6] baz(arg) : i64 res
    baz_245_13:
;       [245:13] alias res -> k
;       [245:13] alias arg -> j
;       [88:5] res = arg * 2
;       [88:11] instructions without scratch register 5, with 3
;       [88:11] allocate scratch register -> r15
;       [88:11] arg
        mov r15, qword [rbp + 344]
;       [88:17] r15 * 2
;       [88:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [88:11] free scratch register r15
    baz_245_13_end:
;   [246:5] assert(k == 2)
;   [246:12] allocate scratch register -> r15
;   [246:12] ? k == 2
;   [246:12] ? k == 2
    cmp_246_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_246_12:
;   [42:6] assert(x : bool)
    assert_246_5:
;       [246:5] alias x -> r15b
        if_42_26_246_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_246_5:
        cmp r15b, 0
        jne if_42_23_246_5_end
        if_42_26_246_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_246_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_246_5_end:
        if_42_23_246_5_end:
;       [246:5] free scratch register r15
    assert_246_5_end:
;   [248:5] k = baz(1)
;   [248:9] k = baz(1)
;   [248:9] = expression
;   [248:9] baz(1)
;   [87:6] baz(arg) : i64 res
    baz_248_9:
;       [248:9] alias res -> k
;       [248:9] alias arg -> 1
;       [88:5] res = arg * 2
;       [88:11] instructions without scratch register 4, with 3
;       [88:11] allocate scratch register -> r15
;       [88:11] arg
        mov r15, 1
;       [88:17] r15 * 2
;       [88:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 352], r15
;       [88:11] free scratch register r15
    baz_248_9_end:
;   [249:5] assert(k == 2)
;   [249:12] allocate scratch register -> r15
;   [249:12] ? k == 2
;   [249:12] ? k == 2
    cmp_249_12:
    cmp qword [rbp + 352], 2
    sete r15b
    bool_end_249_12:
;   [42:6] assert(x : bool)
    assert_249_5:
;       [249:5] alias x -> r15b
        if_42_26_249_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_249_5:
        cmp r15b, 0
        jne if_42_23_249_5_end
        if_42_26_249_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_249_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_249_5_end:
        if_42_23_249_5_end:
;       [249:5] free scratch register r15
    assert_249_5_end:
;   [251:5] var p0 : point = {baz(3), 0}
;   [251:9] p0: point (16 B @ [rbp + 360])
;   [251:9] p0 = {baz(3), 0}
;   [251:23] copy field 'x'
;   [251:23] p0.x = baz(3)
;   [251:23] = expression
;   [251:23] baz(3)
;   [87:6] baz(arg) : i64 res
    baz_251_23:
;       [251:23] alias res -> p0.x (lea: rbp + 360)
;       [251:23] alias arg -> 3
;       [88:5] res = arg * 2
;       [88:11] instructions without scratch register 4, with 3
;       [88:11] allocate scratch register -> r15
;       [88:11] arg
        mov r15, 3
;       [88:17] r15 * 2
;       [88:17] dst is reg, src is const
        imul r15, 2
        mov qword [rbp + 360], r15
;       [88:11] free scratch register r15
    baz_251_23_end:
;   [251:31] copy field 'y'
    mov qword [rbp + 368], 0
;   [252:5] assert(p0.x == 6)
;   [252:12] allocate scratch register -> r15
;   [252:12] ? p0.x == 6
;   [252:12] ? p0.x == 6
    cmp_252_12:
    cmp qword [rbp + 360], 6
    sete r15b
    bool_end_252_12:
;   [42:6] assert(x : bool)
    assert_252_5:
;       [252:5] alias x -> r15b
        if_42_26_252_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_252_5:
        cmp r15b, 0
        jne if_42_23_252_5_end
        if_42_26_252_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_252_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_252_5_end:
        if_42_23_252_5_end:
;       [252:5] free scratch register r15
    assert_252_5_end:
;   [254:5] var pt : point = point_init()
;   [254:9] pt: point (16 B @ [rbp + 376])
;   [254:9] pt = point_init()
;   [254:22] point_init()
;   [114:6] point_init() : point res
    point_init_254_22:
;       [254:22] alias res -> pt
;       [115:5] res.x = -1
;       [115:14] -1
        mov qword [rbp + 376], -1
;       [116:5] res.y = -2
;       [116:14] -2
        mov qword [rbp + 384], -2
    point_init_254_22_end:
;   [255:5] assert(pt.x == -1)
;   [255:12] allocate scratch register -> r15
;   [255:12] ? pt.x == -1
;   [255:12] ? pt.x == -1
    cmp_255_12:
    cmp qword [rbp + 376], -1
    sete r15b
    bool_end_255_12:
;   [42:6] assert(x : bool)
    assert_255_5:
;       [255:5] alias x -> r15b
        if_42_26_255_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_255_5:
        cmp r15b, 0
        jne if_42_23_255_5_end
        if_42_26_255_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_255_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_255_5_end:
        if_42_23_255_5_end:
;       [255:5] free scratch register r15
    assert_255_5_end:
;   [256:5] assert(pt.y == -2)
;   [256:12] allocate scratch register -> r15
;   [256:12] ? pt.y == -2
;   [256:12] ? pt.y == -2
    cmp_256_12:
    cmp qword [rbp + 384], -2
    sete r15b
    bool_end_256_12:
;   [42:6] assert(x : bool)
    assert_256_5:
;       [256:5] alias x -> r15b
        if_42_26_256_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_256_5:
        cmp r15b, 0
        jne if_42_23_256_5_end
        if_42_26_256_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_256_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_256_5_end:
        if_42_23_256_5_end:
;       [256:5] free scratch register r15
    assert_256_5_end:
;   [258:5] var x = 1
;   [258:9] x: i64 (8 B @ [rbp + 392])
;   [258:9] x = 1
;   [258:13] 1
    mov qword [rbp + 392], 1
;   [259:5] var y = 2
;   [259:9] y: i64 (8 B @ [rbp + 400])
;   [259:9] y = 2
;   [259:13] 2
    mov qword [rbp + 400], 2
;   [261:5] var o1 : object = {{x * 10, y}, 0xff0000}
;   [261:9] o1: object (20 B @ [rbp + 408])
;   [261:9] o1 = {{x * 10, y}, 0xff0000}
;   [261:24] copy field 'pos'
;   [261:25] copy field 'x'
;   [261:25] instructions without scratch register 5, with 3
;   [261:25] allocate scratch register -> r15
;   [261:25] x
    mov r15, qword [rbp + 392]
;   [261:29] r15 * 10
;   [261:29] dst is reg, src is const
    imul r15, 10
    mov qword [rbp + 408], r15
;   [261:25] free scratch register r15
;   [261:33] copy field 'y'
;   [261:33] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 416], r15
;   [261:33] free scratch register r15
;   [261:37] copy field 'color'
    mov dword [rbp + 424], 16711680
;   [262:5] assert(o1.pos.x == 10)
;   [262:12] allocate scratch register -> r15
;   [262:12] ? o1.pos.x == 10
;   [262:12] ? o1.pos.x == 10
    cmp_262_12:
    cmp qword [rbp + 408], 10
    sete r15b
    bool_end_262_12:
;   [42:6] assert(x : bool)
    assert_262_5:
;       [262:5] alias x -> r15b
        if_42_26_262_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_262_5:
        cmp r15b, 0
        jne if_42_23_262_5_end
        if_42_26_262_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_262_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_262_5_end:
        if_42_23_262_5_end:
;       [262:5] free scratch register r15
    assert_262_5_end:
;   [263:5] assert(o1.pos.y == 2)
;   [263:12] allocate scratch register -> r15
;   [263:12] ? o1.pos.y == 2
;   [263:12] ? o1.pos.y == 2
    cmp_263_12:
    cmp qword [rbp + 416], 2
    sete r15b
    bool_end_263_12:
;   [42:6] assert(x : bool)
    assert_263_5:
;       [263:5] alias x -> r15b
        if_42_26_263_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_263_5:
        cmp r15b, 0
        jne if_42_23_263_5_end
        if_42_26_263_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_263_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_263_5_end:
        if_42_23_263_5_end:
;       [263:5] free scratch register r15
    assert_263_5_end:
;   [264:5] assert(o1.color == 0xff0000)
;   [264:12] allocate scratch register -> r15
;   [264:12] ? o1.color == 0xff0000
;   [264:12] ? o1.color == 0xff0000
    cmp_264_12:
    cmp dword [rbp + 424], 16711680
    sete r15b
    bool_end_264_12:
;   [42:6] assert(x : bool)
    assert_264_5:
;       [264:5] alias x -> r15b
        if_42_26_264_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_264_5:
        cmp r15b, 0
        jne if_42_23_264_5_end
        if_42_26_264_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_264_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_264_5_end:
        if_42_23_264_5_end:
;       [264:5] free scratch register r15
    assert_264_5_end:
;   [266:5] var p1 : point = {-x, -y}
;   [266:9] p1: point (16 B @ [rbp + 428])
;   [266:9] p1 = {-x, -y}
;   [266:23] copy field 'x'
;   [266:23] allocate scratch register -> r15
    mov r15, qword [rbp + 392]
    mov qword [rbp + 428], r15
;   [266:23] free scratch register r15
    neg qword [rbp + 428]
;   [266:27] copy field 'y'
;   [266:27] allocate scratch register -> r15
    mov r15, qword [rbp + 400]
    mov qword [rbp + 436], r15
;   [266:27] free scratch register r15
    neg qword [rbp + 436]
;   [267:5] o1.pos = p1
;   [267:14] size <= 16 B, use mov
;   [267:14] allocate named register rax
    mov rax, qword [rbp + 428]
    mov qword [rbp + 408], rax
    mov rax, qword [rbp + 436]
    mov qword [rbp + 416], rax
;   [267:14] free named register rax
;   [268:5] assert(o1.pos.x == -1)
;   [268:12] allocate scratch register -> r15
;   [268:12] ? o1.pos.x == -1
;   [268:12] ? o1.pos.x == -1
    cmp_268_12:
    cmp qword [rbp + 408], -1
    sete r15b
    bool_end_268_12:
;   [42:6] assert(x : bool)
    assert_268_5:
;       [268:5] alias x -> r15b
        if_42_26_268_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_268_5:
        cmp r15b, 0
        jne if_42_23_268_5_end
        if_42_26_268_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_268_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_268_5_end:
        if_42_23_268_5_end:
;       [268:5] free scratch register r15
    assert_268_5_end:
;   [269:5] assert(o1.pos.y == -2)
;   [269:12] allocate scratch register -> r15
;   [269:12] ? o1.pos.y == -2
;   [269:12] ? o1.pos.y == -2
    cmp_269_12:
    cmp qword [rbp + 416], -2
    sete r15b
    bool_end_269_12:
;   [42:6] assert(x : bool)
    assert_269_5:
;       [269:5] alias x -> r15b
        if_42_26_269_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_269_5:
        cmp r15b, 0
        jne if_42_23_269_5_end
        if_42_26_269_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_269_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_269_5_end:
        if_42_23_269_5_end:
;       [269:5] free scratch register r15
    assert_269_5_end:
;   [271:5] var o2 : object = o1
;   [271:9] o2: object (20 B @ [rbp + 444])
;   [271:9] o2 = o1
;   [271:23] allocate named register rsi
;   [271:23] allocate named register rdi
;   [271:23] allocate named register rcx
    lea rsi, [rbp + 408]
    lea rdi, [rbp + 444]
    mov rcx, 20
    rep movsb
;   [271:23] free named register rcx
;   [271:23] free named register rdi
;   [271:23] free named register rsi
;   [272:5] assert(o2.pos.x == -1)
;   [272:12] allocate scratch register -> r15
;   [272:12] ? o2.pos.x == -1
;   [272:12] ? o2.pos.x == -1
    cmp_272_12:
    cmp qword [rbp + 444], -1
    sete r15b
    bool_end_272_12:
;   [42:6] assert(x : bool)
    assert_272_5:
;       [272:5] alias x -> r15b
        if_42_26_272_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_272_5:
        cmp r15b, 0
        jne if_42_23_272_5_end
        if_42_26_272_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_272_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_272_5_end:
        if_42_23_272_5_end:
;       [272:5] free scratch register r15
    assert_272_5_end:
;   [273:5] assert(o2.pos.y == -2)
;   [273:12] allocate scratch register -> r15
;   [273:12] ? o2.pos.y == -2
;   [273:12] ? o2.pos.y == -2
    cmp_273_12:
    cmp qword [rbp + 452], -2
    sete r15b
    bool_end_273_12:
;   [42:6] assert(x : bool)
    assert_273_5:
;       [273:5] alias x -> r15b
        if_42_26_273_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_273_5:
        cmp r15b, 0
        jne if_42_23_273_5_end
        if_42_26_273_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_273_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_273_5_end:
        if_42_23_273_5_end:
;       [273:5] free scratch register r15
    assert_273_5_end:
;   [274:5] assert(o2.color == 0xff0000)
;   [274:12] allocate scratch register -> r15
;   [274:12] ? o2.color == 0xff0000
;   [274:12] ? o2.color == 0xff0000
    cmp_274_12:
    cmp dword [rbp + 460], 16711680
    sete r15b
    bool_end_274_12:
;   [42:6] assert(x : bool)
    assert_274_5:
;       [274:5] alias x -> r15b
        if_42_26_274_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_274_5:
        cmp r15b, 0
        jne if_42_23_274_5_end
        if_42_26_274_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_274_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_274_5_end:
        if_42_23_274_5_end:
;       [274:5] free scratch register r15
    assert_274_5_end:
;   [276:5] var o3 : object[2]
;   [276:9] o3: object[2] (40 B @ [rbp + 464])
;   [276:9] zero 2 * 20 B = 40 B
;   [276:5] allocate named register rax
;   [276:5] allocate named register rdi
;   [276:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 464]
    mov rcx, 40
    rep stosb
;   [276:5] free named register rcx
;   [276:5] free named register rdi
;   [276:5] free named register rax
;   [277:5] o3.pos.y = 73
;   [277:16] 73
    mov qword [rbp + 472], 73
;   [278:5] # index 0 in an array can be accessed without array index
;   [280:5] assert(o3[0].pos.y == 73)
;   [280:12] allocate scratch register -> r15
;   [280:12] ? o3[0].pos.y == 73
;   [280:12] ? o3[0].pos.y == 73
    cmp_280_12:
;   [280:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [280:12] allocate scratch register -> r13
;   [280:15] set array index
;   [280:15] 0
    mov r13, 0
;   [280:15] bounds check
;   [280:15] allocate scratch register -> r12
;   [280:15] line number
    mov r12, 280
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [280:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [280:12] free scratch register r13
    cmp qword [r14 + 8], 73
;   [280:12] free scratch register r14
    sete r15b
    bool_end_280_12:
;   [42:6] assert(x : bool)
    assert_280_5:
;       [280:5] alias x -> r15b
        if_42_26_280_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_280_5:
        cmp r15b, 0
        jne if_42_23_280_5_end
        if_42_26_280_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_280_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_280_5_end:
        if_42_23_280_5_end:
;       [280:5] free scratch register r15
    assert_280_5_end:
;   [282:5] o3[1] = object_init()
;   [282:5] allocate scratch register -> r15
    lea r15, [rbp + 464]
;   [282:5] allocate scratch register -> r14
;   [282:8] set array index
;   [282:8] 1
    mov r14, 1
;   [282:8] bounds check
;   [282:8] allocate scratch register -> r13
;   [282:8] line number
    mov r13, 282
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [282:8] free scratch register r13
    imul r14, 20
    add r15, r14
;   [282:5] free scratch register r14
;   [282:13] object_init()
;   [119:6] object_init() : object res
    object_init_282_13:
;       [282:13] alias res -> o3 (lea: r15)
;       [120:5] res.pos.y = 74
;       [120:17] 74
        mov qword [r15 + 8], 74
    object_init_282_13_end:
;   [282:5] free scratch register r15
;   [283:5] assert(o3[1].pos.y == 74)
;   [283:12] allocate scratch register -> r15
;   [283:12] ? o3[1].pos.y == 74
;   [283:12] ? o3[1].pos.y == 74
    cmp_283_12:
;   [283:12] allocate scratch register -> r14
    lea r14, [rbp + 464]
;   [283:12] allocate scratch register -> r13
;   [283:15] set array index
;   [283:15] 1
    mov r13, 1
;   [283:15] bounds check
;   [283:15] allocate scratch register -> r12
;   [283:15] line number
    mov r12, 283
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 2
    cmovge rbp, r12
    jge baz_bounds_panic
;   [283:15] free scratch register r12
    imul r13, 20
    add r14, r13
;   [283:12] free scratch register r13
    cmp qword [r14 + 8], 74
;   [283:12] free scratch register r14
    sete r15b
    bool_end_283_12:
;   [42:6] assert(x : bool)
    assert_283_5:
;       [283:5] alias x -> r15b
        if_42_26_283_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_283_5:
        cmp r15b, 0
        jne if_42_23_283_5_end
        if_42_26_283_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_283_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_283_5_end:
        if_42_23_283_5_end:
;       [283:5] free scratch register r15
    assert_283_5_end:
;   [285:5] var worlds : world[8]
;   [285:9] worlds: world[8] (512 B @ [rbp + 504])
;   [285:9] zero 8 * 64 B = 512 B
;   [285:5] allocate named register rax
;   [285:5] allocate named register rdi
;   [285:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 504]
    mov rcx, 512
    rep stosb
;   [285:5] free named register rcx
;   [285:5] free named register rdi
;   [285:5] free named register rax
;   [286:5] worlds[1].locations[1] = 0xffee
;   [286:5] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [286:5] allocate scratch register -> r14
;   [286:12] set array index
;   [286:12] 1
    mov r14, 1
;   [286:12] bounds check
;   [286:12] allocate scratch register -> r13
;   [286:12] line number
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [286:12] free scratch register r13
    shl r14, 6
    add r15, r14
;   [286:5] free scratch register r14
;   [286:5] allocate scratch register -> r14
;   [286:25] set array index
;   [286:25] 1
    mov r14, 1
;   [286:25] bounds check
;   [286:25] allocate scratch register -> r13
;   [286:25] line number
    mov r13, 286
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [286:25] free scratch register r13
;   [286:30] 0xffee
    mov qword [r15 + r14 * 8], 65518
;   [286:5] free scratch register r14
;   [286:5] free scratch register r15
;   [287:5] assert(worlds[1].locations[1] == 0xffee)
;   [287:12] allocate scratch register -> r15
;   [287:12] ? worlds[1].locations[1] == 0xffee
;   [287:12] ? worlds[1].locations[1] == 0xffee
    cmp_287_12:
;   [287:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [287:12] allocate scratch register -> r13
;   [287:19] set array index
;   [287:19] 1
    mov r13, 1
;   [287:19] bounds check
;   [287:19] allocate scratch register -> r12
;   [287:19] line number
    mov r12, 287
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [287:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [287:12] free scratch register r13
;   [287:12] allocate scratch register -> r13
;   [287:32] set array index
;   [287:32] 1
    mov r13, 1
;   [287:32] bounds check
;   [287:32] allocate scratch register -> r12
;   [287:32] line number
    mov r12, 287
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [287:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [287:12] free scratch register r13
;   [287:12] free scratch register r14
    sete r15b
    bool_end_287_12:
;   [42:6] assert(x : bool)
    assert_287_5:
;       [287:5] alias x -> r15b
        if_42_26_287_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_287_5:
        cmp r15b, 0
        jne if_42_23_287_5_end
        if_42_26_287_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_287_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_287_5_end:
        if_42_23_287_5_end:
;       [287:5] free scratch register r15
    assert_287_5_end:
;   [289:5] array_copy( worlds[1].locations, worlds[0].locations, array_size_of(worlds.locations) )
;   [289:5] allocate named register rsi
;   [289:5] allocate named register rdi
;   [289:5] allocate named register rcx
;   [292:9] array_size_of(worlds.locations)
;   [292:9] rcx = array_size_of(worlds.locations)
;   [292:9] = expression
;   [292:9] array_size_of(worlds.locations)
    mov rcx, 8
;   [290:9] worlds[1].locations
;   [290:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [290:9] allocate scratch register -> r14
;   [290:16] set array index
;   [290:16] 1
    mov r14, 1
;   [290:16] bounds check
;   [290:16] allocate scratch register -> r13
;   [290:16] line number
    mov r13, 290
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [290:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [290:9] free scratch register r14
;   [290:9] bounds check
;   [290:9] allocate scratch register -> r14
;   [290:9] line number
    mov r14, 290
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [290:9] free scratch register r14
    lea rsi, [r15]
;   [289:5] free scratch register r15
;   [291:9] worlds[0].locations
;   [291:9] allocate scratch register -> r15
    lea r15, [rbp + 504]
;   [291:9] allocate scratch register -> r14
;   [291:16] set array index
;   [291:16] 0
    mov r14, 0
;   [291:16] bounds check
;   [291:16] allocate scratch register -> r13
;   [291:16] line number
    mov r13, 291
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 8
    cmovge rbp, r13
    jge baz_bounds_panic
;   [291:16] free scratch register r13
    shl r14, 6
    add r15, r14
;   [291:9] free scratch register r14
;   [291:9] bounds check
;   [291:9] allocate scratch register -> r14
;   [291:9] line number
    mov r14, 291
    test rcx, rcx
    cmovs rbp, r14
    js baz_bounds_panic
    cmp rcx, 8
    cmovg rbp, r14
    jg baz_bounds_panic
;   [291:9] free scratch register r14
    lea rdi, [r15]
;   [289:5] free scratch register r15
    shl rcx, 3
    rep movsb
;   [289:5] free named register rcx
;   [289:5] free named register rdi
;   [289:5] free named register rsi
;   [294:5] # `array_copy` is built-in and can use indexed positions
;   [295:5] # `array_size_of` is built-in
;   [297:5] assert(worlds[0].locations[1] == 0xffee)
;   [297:12] allocate scratch register -> r15
;   [297:12] ? worlds[0].locations[1] == 0xffee
;   [297:12] ? worlds[0].locations[1] == 0xffee
    cmp_297_12:
;   [297:12] allocate scratch register -> r14
    lea r14, [rbp + 504]
;   [297:12] allocate scratch register -> r13
;   [297:19] set array index
;   [297:19] 0
    mov r13, 0
;   [297:19] bounds check
;   [297:19] allocate scratch register -> r12
;   [297:19] line number
    mov r12, 297
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [297:19] free scratch register r12
    shl r13, 6
    add r14, r13
;   [297:12] free scratch register r13
;   [297:12] allocate scratch register -> r13
;   [297:32] set array index
;   [297:32] 1
    mov r13, 1
;   [297:32] bounds check
;   [297:32] allocate scratch register -> r12
;   [297:32] line number
    mov r12, 297
    test r13, r13
    cmovs rbp, r12
    js baz_bounds_panic
    cmp r13, 8
    cmovge rbp, r12
    jge baz_bounds_panic
;   [297:32] free scratch register r12
    cmp qword [r14 + r13 * 8], 65518
;   [297:12] free scratch register r13
;   [297:12] free scratch register r14
    sete r15b
    bool_end_297_12:
;   [42:6] assert(x : bool)
    assert_297_5:
;       [297:5] alias x -> r15b
        if_42_26_297_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_297_5:
        cmp r15b, 0
        jne if_42_23_297_5_end
        if_42_26_297_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_297_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_297_5_end:
        if_42_23_297_5_end:
;       [297:5] free scratch register r15
    assert_297_5_end:
;   [298:5] assert(arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) ))
;   [298:12] allocate scratch register -> r15
;   [298:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;   [298:12] ? arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
    cmp_298_12:
;   [298:12] allocate scratch register -> r14
;       [298:12] r14b = arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [298:12] = expression
;       [298:12] arrays_equal( worlds[0].locations, worlds[1].locations, array_size_of(worlds.locations) )
;       [298:12] allocate named register rsi
;       [298:12] allocate named register rdi
;       [298:12] allocate named register rcx
;       [301:14] array_size_of(worlds.locations)
;       [301:14] rcx = array_size_of(worlds.locations)
;       [301:14] = expression
;       [301:14] array_size_of(worlds.locations)
        mov rcx, 8
;       [299:14] worlds[0].locations
;       [299:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [299:14] allocate scratch register -> r12
;       [299:21] set array index
;       [299:21] 0
        mov r12, 0
;       [299:21] bounds check
;       [299:21] allocate scratch register -> r10
;       [299:21] line number
        mov r10, 299
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
;       [299:21] free scratch register r10
        shl r12, 6
        add r13, r12
;       [299:14] free scratch register r12
;       [299:14] bounds check
;       [299:14] allocate scratch register -> r12
;       [299:14] line number
        mov r12, 299
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [299:14] free scratch register r12
        lea rsi, [r13]
;       [298:12] free scratch register r13
;       [300:14] worlds[1].locations
;       [300:14] allocate scratch register -> r13
        lea r13, [rbp + 504]
;       [300:14] allocate scratch register -> r12
;       [300:21] set array index
;       [300:21] 1
        mov r12, 1
;       [300:21] bounds check
;       [300:21] allocate scratch register -> r10
;       [300:21] line number
        mov r10, 300
        test r12, r12
        cmovs rbp, r10
        js baz_bounds_panic
        cmp r12, 8
        cmovge rbp, r10
        jge baz_bounds_panic
;       [300:21] free scratch register r10
        shl r12, 6
        add r13, r12
;       [300:14] free scratch register r12
;       [300:14] bounds check
;       [300:14] allocate scratch register -> r12
;       [300:14] line number
        mov r12, 300
        test rcx, rcx
        cmovs rbp, r12
        js baz_bounds_panic
        cmp rcx, 8
        cmovg rbp, r12
        jg baz_bounds_panic
;       [300:14] free scratch register r12
        lea rdi, [r13]
;       [298:12] free scratch register r13
        shl rcx, 3
        repe cmpsb
;       [298:12] free named register rcx
;       [298:12] free named register rdi
;       [298:12] free named register rsi
        sete r14b
    cmp r14b, 0
;   [298:12] free scratch register r14
    setne r15b
    bool_end_298_12:
;   [42:6] assert(x : bool)
    assert_298_5:
;       [298:5] alias x -> r15b
        if_42_26_298_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_298_5:
        cmp r15b, 0
        jne if_42_23_298_5_end
        if_42_26_298_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_298_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_298_5_end:
        if_42_23_298_5_end:
;       [298:5] free scratch register r15
    assert_298_5_end:
;   [304:5] var arr2 : i64[] = { -1, 2 }
;   [304:9] arr2: i64[2] (16 B @ [rbp + 1016])
;   [304:9] arr2 = { -1, 2 }
;   [304:26] [0]
;   [304:27] -1
    mov qword [rbp + 1016], -1
;   [304:26] [1]
;   [304:30] 2
    mov qword [rbp + 1024], 2
;   [305:5] assert(array_size_of(arr2) == 2)
;   [305:12] allocate scratch register -> r15
;   [305:12] ? array_size_of(arr2) == 2
;   [305:12] ? array_size_of(arr2) == 2
    cmp_305_12:
;   [305:12] allocate scratch register -> r14
;       [305:12] r14 = array_size_of(arr2)
;       [305:12] = expression
;       [305:12] array_size_of(arr2)
        mov r14, 2
    cmp r14, 2
;   [305:12] free scratch register r14
    sete r15b
    bool_end_305_12:
;   [42:6] assert(x : bool)
    assert_305_5:
;       [305:5] alias x -> r15b
        if_42_26_305_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_305_5:
        cmp r15b, 0
        jne if_42_23_305_5_end
        if_42_26_305_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_305_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_305_5_end:
        if_42_23_305_5_end:
;       [305:5] free scratch register r15
    assert_305_5_end:
;   [306:5] assert(arr2[0] == -1)
;   [306:12] allocate scratch register -> r15
;   [306:12] ? arr2[0] == -1
;   [306:12] ? arr2[0] == -1
    cmp_306_12:
;   [306:12] allocate scratch register -> r14
;   [306:17] set array index
;   [306:17] 0
    mov r14, 0
;   [306:17] bounds check
;   [306:17] allocate scratch register -> r13
;   [306:17] line number
    mov r13, 306
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [306:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], -1
;   [306:12] free scratch register r14
    sete r15b
    bool_end_306_12:
;   [42:6] assert(x : bool)
    assert_306_5:
;       [306:5] alias x -> r15b
        if_42_26_306_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_306_5:
        cmp r15b, 0
        jne if_42_23_306_5_end
        if_42_26_306_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_306_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_306_5_end:
        if_42_23_306_5_end:
;       [306:5] free scratch register r15
    assert_306_5_end:
;   [307:5] assert(arr2[1] == 2)
;   [307:12] allocate scratch register -> r15
;   [307:12] ? arr2[1] == 2
;   [307:12] ? arr2[1] == 2
    cmp_307_12:
;   [307:12] allocate scratch register -> r14
;   [307:17] set array index
;   [307:17] 1
    mov r14, 1
;   [307:17] bounds check
;   [307:17] allocate scratch register -> r13
;   [307:17] line number
    mov r13, 307
    test r14, r14
    cmovs rbp, r13
    js baz_bounds_panic
    cmp r14, 2
    cmovge rbp, r13
    jge baz_bounds_panic
;   [307:17] free scratch register r13
    cmp qword [rbp + r14 * 8 + 1016], 2
;   [307:12] free scratch register r14
    sete r15b
    bool_end_307_12:
;   [42:6] assert(x : bool)
    assert_307_5:
;       [307:5] alias x -> r15b
        if_42_26_307_5:
;       [42:26] ? not x
;       [42:26] ? not x
        cmp_42_26_307_5:
        cmp r15b, 0
        jne if_42_23_307_5_end
        if_42_26_307_5_code:
;           [42:32] exit(1)
;           [42:37] allocate named register rdi
            mov rdi, 1
;           [34:6] exit(v : reg_rdi)
            exit_42_32_307_5:
;               [42:32] alias v -> rdi
;               [35:5] mov(rax, 60)
;                   [35:14] 60
                    mov rax, 60
;               [35:19] # exit system call
;               [36:5] mov(rdi, v)
;                   [36:14] v
;               [36:19] # return code
;               [37:5] syscall()
                syscall
;               [42:32] free named register rdi
            exit_42_32_307_5_end:
        if_42_23_307_5_end:
;       [307:5] free scratch register r15
    assert_307_5_end:
;   [309:5] var counter
;   [309:9] counter: i64 (8 B @ [rbp + 1032])
;   [309:9] zero 1 * 8 B = 8 B
;   [309:5] size <= 32 B, use mov
    mov qword [rbp + 1032], 0
;   [310:5] var nm : str
;   [310:9] nm: str (128 B @ [rbp + 1040])
;   [310:9] zero 1 * 128 B = 128 B
;   [310:5] allocate named register rax
;   [310:5] allocate named register rdi
;   [310:5] allocate named register rcx
    xor al, al
    lea rdi, [rbp + 1040]
    mov rcx, 128
    rep stosb
;   [310:5] free named register rcx
;   [310:5] free named register rdi
;   [310:5] free named register rax
;   [311:5] print(hello)
;   [52:6] print(str : i8[])
    print_311_5:
;       [311:5] alias str -> hello
;       [53:4] sys_print(array_size_of(str), address_of(str))
;       [53:14] allocate named register rdx
;       [53:14] rdx = array_size_of(str)
;       [53:14] = expression
;       [53:14] array_size_of(str)
        mov rdx, 21
;       [53:34] allocate named register rsi
;       [53:34] rsi = address_of(str)
;       [53:34] = expression
;       [53:34] address_of(str)
        lea rsi, [rbp]
;       [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
        sys_print_53_4_311_5:
;           [53:4] alias len -> rdx
;           [53:4] alias ptr -> rsi
;           [45:5] mov(rax, 1)
;               [45:14] 1
                mov rax, 1
;           [45:19] # write system call
;           [46:5] mov(rdi, 0)
;               [46:14] 0
                mov rdi, 0
;           [46:19] # file descriptor for standard out
;           [47:5] mov(rsi, ptr)
;               [47:14] ptr
;           [47:19] # buffer address
;           [48:5] mov(rdx, len)
;               [48:14] len
;           [48:19] # buffer size
;           [49:5] syscall()
            syscall
;           [53:4] free named register rsi
;           [53:4] free named register rdx
        sys_print_53_4_311_5_end:
    print_311_5_end:
;   [312:5] label
    loop_312_5:
;       [313:9] counter = counter + 1
;       [313:19] instructions without scratch register 1, with 3
;       [313:19] counter
;       [313:29] counter + 1
        add qword [rbp + 1032], 1
;       [314:9] print_num(counter)
;       [314:9] address of argument 'counter' to parameter 'num'
;       [314:9] allocate scratch register -> r15
        lea r15, [rbp + 1032]
        mov qword [rbp + 1168], r15
;       [314:9] free scratch register r15
        PUSH_REGS
        lea rbx, [rbp + 1168]
        call print_num
        POP_REGS
;       [315:9] print(colon)
;       [52:6] print(str : i8[])
        print_315_9:
;           [315:9] alias str -> colon
;           [53:4] sys_print(array_size_of(str), address_of(str))
;           [53:14] allocate named register rdx
;           [53:14] rdx = array_size_of(str)
;           [53:14] = expression
;           [53:14] array_size_of(str)
            mov rdx, 2
;           [53:34] allocate named register rsi
;           [53:34] rsi = address_of(str)
;           [53:34] = expression
;           [53:34] address_of(str)
            lea rsi, [rbp + 61]
;           [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
            sys_print_53_4_315_9:
;               [53:4] alias len -> rdx
;               [53:4] alias ptr -> rsi
;               [45:5] mov(rax, 1)
;                   [45:14] 1
                    mov rax, 1
;               [45:19] # write system call
;               [46:5] mov(rdi, 0)
;                   [46:14] 0
                    mov rdi, 0
;               [46:19] # file descriptor for standard out
;               [47:5] mov(rsi, ptr)
;                   [47:14] ptr
;               [47:19] # buffer address
;               [48:5] mov(rdx, len)
;                   [48:14] len
;               [48:19] # buffer size
;               [49:5] syscall()
                syscall
;               [53:4] free named register rsi
;               [53:4] free named register rdx
            sys_print_53_4_315_9_end:
        print_315_9_end:
;       [316:9] print(prompt1)
;       [52:6] print(str : i8[])
        print_316_9:
;           [316:9] alias str -> prompt1
;           [53:4] sys_print(array_size_of(str), address_of(str))
;           [53:14] allocate named register rdx
;           [53:14] rdx = array_size_of(str)
;           [53:14] = expression
;           [53:14] array_size_of(str)
            mov rdx, 12
;           [53:34] allocate named register rsi
;           [53:34] rsi = address_of(str)
;           [53:34] = expression
;           [53:34] address_of(str)
            lea rsi, [rbp + 21]
;           [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
            sys_print_53_4_316_9:
;               [53:4] alias len -> rdx
;               [53:4] alias ptr -> rsi
;               [45:5] mov(rax, 1)
;                   [45:14] 1
                    mov rax, 1
;               [45:19] # write system call
;               [46:5] mov(rdi, 0)
;                   [46:14] 0
                    mov rdi, 0
;               [46:19] # file descriptor for standard out
;               [47:5] mov(rsi, ptr)
;                   [47:14] ptr
;               [47:19] # buffer address
;               [48:5] mov(rdx, len)
;                   [48:14] len
;               [48:19] # buffer size
;               [49:5] syscall()
                syscall
;               [53:4] free named register rsi
;               [53:4] free named register rdx
            sys_print_53_4_316_9_end:
        print_316_9_end:
;       [317:9] str_in(nm)
;       [97:6] str_in(s : str)
        str_in_317_9:
;           [317:9] alias s -> nm
;           [98:5] mov(rax, 0)
;               [98:14] 0
                mov rax, 0
;           [98:37] # read system call
;           [99:5] mov(rdi, 0)
;               [99:14] 0
                mov rdi, 0
;           [99:37] # file descriptor for standard input
;           [100:5] mov(rsi, address_of(s.data))
;               [100:14] rsi = address_of(s.data)
;               [100:14] = expression
;               [100:14] address_of(s.data)
                lea rsi, [rbp + 1041]
;           [100:37] # buffer address
;           [101:5] mov(rdx, array_size_of(s.data))
;               [101:14] rdx = array_size_of(s.data)
;               [101:14] = expression
;               [101:14] array_size_of(s.data)
                mov rdx, 127
;           [101:37] # buffer size
;           [102:5] syscall()
            syscall
;           [103:5] mov(s.len, rax - 1)
;               [103:16] instructions without scratch register 2, with 3
;               [103:16] rax
                mov byte [rbp + 1040], al
;               [103:22] s.len - 1
                sub byte [rbp + 1040], 1
;           [103:25] # return value
        str_in_317_9_end:
        if_318_12:
;       [318:12] ? nm.len == 0
;       [318:12] ? nm.len == 0
        cmp_318_12:
        cmp byte [rbp + 1040], 0
        jne if_320_19
        if_318_12_code:
;           [319:13] break
            jmp loop_312_5_end
        jmp if_318_9_end
        if_320_19:
;       [320:19] ? nm.len <= 4
;       [320:19] ? nm.len <= 4
        cmp_320_19:
        cmp byte [rbp + 1040], 4
        jg if_else_318_9
        if_320_19_code:
;           [321:13] print(prompt2)
;           [52:6] print(str : i8[])
            print_321_13:
;               [321:13] alias str -> prompt2
;               [53:4] sys_print(array_size_of(str), address_of(str))
;               [53:14] allocate named register rdx
;               [53:14] rdx = array_size_of(str)
;               [53:14] = expression
;               [53:14] array_size_of(str)
                mov rdx, 20
;               [53:34] allocate named register rsi
;               [53:34] rsi = address_of(str)
;               [53:34] = expression
;               [53:34] address_of(str)
                lea rsi, [rbp + 33]
;               [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_53_4_321_13:
;                   [53:4] alias len -> rdx
;                   [53:4] alias ptr -> rsi
;                   [45:5] mov(rax, 1)
;                       [45:14] 1
                        mov rax, 1
;                   [45:19] # write system call
;                   [46:5] mov(rdi, 0)
;                       [46:14] 0
                        mov rdi, 0
;                   [46:19] # file descriptor for standard out
;                   [47:5] mov(rsi, ptr)
;                       [47:14] ptr
;                   [47:19] # buffer address
;                   [48:5] mov(rdx, len)
;                       [48:14] len
;                   [48:19] # buffer size
;                   [49:5] syscall()
                    syscall
;                   [53:4] free named register rsi
;                   [53:4] free named register rdx
                sys_print_53_4_321_13_end:
            print_321_13_end:
;           [322:13] continue
            jmp loop_312_5
        jmp if_318_9_end
        if_else_318_9:
;           [324:13] print(prompt3)
;           [52:6] print(str : i8[])
            print_324_13:
;               [324:13] alias str -> prompt3
;               [53:4] sys_print(array_size_of(str), address_of(str))
;               [53:14] allocate named register rdx
;               [53:14] rdx = array_size_of(str)
;               [53:14] = expression
;               [53:14] array_size_of(str)
                mov rdx, 6
;               [53:34] allocate named register rsi
;               [53:34] rsi = address_of(str)
;               [53:34] = expression
;               [53:34] address_of(str)
                lea rsi, [rbp + 53]
;               [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_53_4_324_13:
;                   [53:4] alias len -> rdx
;                   [53:4] alias ptr -> rsi
;                   [45:5] mov(rax, 1)
;                       [45:14] 1
                        mov rax, 1
;                   [45:19] # write system call
;                   [46:5] mov(rdi, 0)
;                       [46:14] 0
                        mov rdi, 0
;                   [46:19] # file descriptor for standard out
;                   [47:5] mov(rsi, ptr)
;                       [47:14] ptr
;                   [47:19] # buffer address
;                   [48:5] mov(rdx, len)
;                       [48:14] len
;                   [48:19] # buffer size
;                   [49:5] syscall()
                    syscall
;                   [53:4] free named register rsi
;                   [53:4] free named register rdx
                sys_print_53_4_324_13_end:
            print_324_13_end:
;           [325:13] str_out(nm)
;           [106:6] str_out(s : str)
            str_out_325_13:
;               [325:13] alias s -> nm
;               [107:5] mov(rax, 1)
;                   [107:14] 1
                    mov rax, 1
;               [107:34] # write system call
;               [108:5] mov(rdi, 0)
;                   [108:14] 0
                    mov rdi, 0
;               [108:34] # file descriptor for standard out
;               [109:5] mov(rsi, address_of(s.data))
;                   [109:14] rsi = address_of(s.data)
;                   [109:14] = expression
;                   [109:14] address_of(s.data)
                    lea rsi, [rbp + 1041]
;               [109:34] # buffer address
;               [110:5] mov(rdx, s.len)
;                   [110:14] s.len
                    movsx rdx, byte [rbp + 1040]
;               [110:34] # buffer size
;               [111:5] syscall()
                syscall
            str_out_325_13_end:
;           [326:13] print(dot)
;           [52:6] print(str : i8[])
            print_326_13:
;               [326:13] alias str -> dot
;               [53:4] sys_print(array_size_of(str), address_of(str))
;               [53:14] allocate named register rdx
;               [53:14] rdx = array_size_of(str)
;               [53:14] = expression
;               [53:14] array_size_of(str)
                mov rdx, 1
;               [53:34] allocate named register rsi
;               [53:34] rsi = address_of(str)
;               [53:34] = expression
;               [53:34] address_of(str)
                lea rsi, [rbp + 59]
;               [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_53_4_326_13:
;                   [53:4] alias len -> rdx
;                   [53:4] alias ptr -> rsi
;                   [45:5] mov(rax, 1)
;                       [45:14] 1
                        mov rax, 1
;                   [45:19] # write system call
;                   [46:5] mov(rdi, 0)
;                       [46:14] 0
                        mov rdi, 0
;                   [46:19] # file descriptor for standard out
;                   [47:5] mov(rsi, ptr)
;                       [47:14] ptr
;                   [47:19] # buffer address
;                   [48:5] mov(rdx, len)
;                       [48:14] len
;                   [48:19] # buffer size
;                   [49:5] syscall()
                    syscall
;                   [53:4] free named register rsi
;                   [53:4] free named register rdx
                sys_print_53_4_326_13_end:
            print_326_13_end:
;           [327:13] print(nl)
;           [52:6] print(str : i8[])
            print_327_13:
;               [327:13] alias str -> nl
;               [53:4] sys_print(array_size_of(str), address_of(str))
;               [53:14] allocate named register rdx
;               [53:14] rdx = array_size_of(str)
;               [53:14] = expression
;               [53:14] array_size_of(str)
                mov rdx, 1
;               [53:34] allocate named register rsi
;               [53:34] rsi = address_of(str)
;               [53:34] = expression
;               [53:34] address_of(str)
                lea rsi, [rbp + 60]
;               [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
                sys_print_53_4_327_13:
;                   [53:4] alias len -> rdx
;                   [53:4] alias ptr -> rsi
;                   [45:5] mov(rax, 1)
;                       [45:14] 1
                        mov rax, 1
;                   [45:19] # write system call
;                   [46:5] mov(rdi, 0)
;                       [46:14] 0
                        mov rdi, 0
;                   [46:19] # file descriptor for standard out
;                   [47:5] mov(rsi, ptr)
;                       [47:14] ptr
;                   [47:19] # buffer address
;                   [48:5] mov(rdx, len)
;                       [48:14] len
;                   [48:19] # buffer size
;                   [49:5] syscall()
                    syscall
;                   [53:4] free named register rsi
;                   [53:4] free named register rdx
                sys_print_53_4_327_13_end:
            print_327_13_end:
        if_318_9_end:
    jmp loop_312_5
    loop_312_5_end:
    ; system call: exit 0
    mov rax, 60
    mov rdi, 0
    syscall

; 
;[131:15] noinline print_num(num)
print_num:
;   [131:25] num: i64 (8 B @ [rbx])
;   [132:5] var buf : i8[20]
;   [132:9] buf: i8[20] (20 B @ [rbx + 8])
;   [132:9] zero 20 * 1 B = 20 B
;   [132:5] size <= 32 B, use mov
    mov qword [rbx + 8], 0
    mov qword [rbx + 16], 0
    mov dword [rbx + 24], 0
;   [133:5] var n = num
;   [133:9] n: i64 (8 B @ [rbx + 28])
;   [133:9] n = num
;   [133:13] num
;   [133:13] allocate scratch register -> r15
    mov r15, qword [rbx]
;   [133:13] allocate scratch register -> r14
    mov r14, qword [r15]
    mov qword [rbx + 28], r14
;   [133:13] free scratch register r14
;   [133:13] free scratch register r15
;   [134:5] var is_negative : bool = false
;   [134:9] is_negative: bool (1 B @ [rbx + 36])
;   [134:9] is_negative = false
    mov byte [rbx + 36], 0
    if_136_8:
;   [136:8] ? n < 0
;   [136:8] ? n < 0
    cmp_136_8:
    cmp qword [rbx + 28], 0
    jge if_136_5_end
    if_136_8_code:
;       [137:9] is_negative = true
        mov byte [rbx + 36], 1
;       [138:9] n = -n
;       [138:14] -n
        neg qword [rbx + 28]
    if_136_5_end:
;   [141:5] var i = 20
;   [141:9] i: i64 (8 B @ [rbx + 37])
;   [141:9] i = 20
;   [141:13] 20
    mov qword [rbx + 37], 20
;   [142:5] label
    loop_142_5:
;       [143:9] i = i - 1
;       [143:13] instructions without scratch register 1, with 3
;       [143:13] i
;       [143:17] i - 1
        sub qword [rbx + 37], 1
;       [144:9] var ascii = 48 + (n % 10)
;       [144:13] ascii: i64 (8 B @ [rbx + 45])
;       [144:13] ascii = 48 + (n % 10)
;       [144:21] instructions without scratch register 8, with 9
;       [144:21] 48
        mov qword [rbx + 45], 48
;       [144:27] ascii + (n % 10)
;       [144:27] allocate scratch register -> r15
;       [144:27] n
        mov r15, qword [rbx + 28]
;       [144:31] r15 % 10
;       [144:31] div const
;       [144:31] allocate named register rax
        mov rax, r15
;       [144:31] allocate named register rdx
        cqo
;       [144:31] allocate scratch register -> r14
        mov r14, 10
        idiv r14
;       [144:31] free scratch register r14
        mov r15, rdx
;       [144:31] free named register rdx
;       [144:31] free named register rax
        add qword [rbx + 45], r15
;       [144:27] free scratch register r15
;       [145:9] # note: not buf[i] = 48 + ... because expression will be executed as byte sized and n overflows
;       [146:9] buf[i] = ascii
;       [146:9] allocate scratch register -> r15
;       [146:13] set array index
;       [146:13] i
        mov r15, qword [rbx + 37]
;       [146:13] bounds check
;       [146:13] allocate scratch register -> r14
;       [146:13] line number
        mov r14, 146
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [146:13] free scratch register r14
;       [146:18] ascii
;       [146:18] allocate scratch register -> r14
        mov r14b, byte [rbx + 45]
        mov byte [rbx + r15 + 8], r14b
;       [146:18] free scratch register r14
;       [146:9] free scratch register r15
;       [147:9] n = n / 10
;       [147:13] instructions without scratch register 5, with 7
;       [147:13] n
;       [147:17] n / 10
;       [147:17] div const
;       [147:17] allocate named register rax
        mov rax, qword [rbx + 28]
;       [147:17] allocate named register rdx
        cqo
;       [147:17] allocate scratch register -> r15
        mov r15, 10
        idiv r15
;       [147:17] free scratch register r15
        mov qword [rbx + 28], rax
;       [147:17] free named register rdx
;       [147:17] free named register rax
        if_148_12:
;       [148:12] ? n == 0
;       [148:12] ? n == 0
        cmp_148_12:
        cmp qword [rbx + 28], 0
        jne if_148_9_end
        if_148_12_code:
;           [148:19] break
            jmp loop_142_5_end
        if_148_9_end:
    jmp loop_142_5
    loop_142_5_end:
    if_151_8:
;   [151:8] ? is_negative
;   [151:8] ? is_negative
    cmp_151_8:
    cmp byte [rbx + 36], 0
    je if_151_5_end
    if_151_8_code:
;       [152:9] i = i - 1
;       [152:13] instructions without scratch register 1, with 3
;       [152:13] i
;       [152:17] i - 1
        sub qword [rbx + 37], 1
;       [153:9] buf[i] = 45
;       [153:9] allocate scratch register -> r15
;       [153:13] set array index
;       [153:13] i
        mov r15, qword [rbx + 37]
;       [153:13] bounds check
;       [153:13] allocate scratch register -> r14
;       [153:13] line number
        mov r14, 153
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [153:13] free scratch register r14
;       [153:18] 45
        mov byte [rbx + r15 + 8], 45
;       [153:9] free scratch register r15
    if_151_5_end:
;   [156:5] var write_pos = 0
;   [156:9] write_pos: i64 (8 B @ [rbx + 45])
;   [156:9] write_pos = 0
;   [156:21] 0
    mov qword [rbx + 45], 0
;   [157:5] label
    loop_157_5:
;       [158:9] buf[write_pos] = buf[i]
;       [158:9] allocate scratch register -> r15
;       [158:13] set array index
;       [158:13] write_pos
        mov r15, qword [rbx + 45]
;       [158:13] bounds check
;       [158:13] allocate scratch register -> r14
;       [158:13] line number
        mov r14, 158
        test r15, r15
        cmovs rbp, r14
        js baz_bounds_panic
        cmp r15, 20
        cmovge rbp, r14
        jge baz_bounds_panic
;       [158:13] free scratch register r14
;       [158:26] buf[i]
;       [158:26] allocate scratch register -> r14
;       [158:30] set array index
;       [158:30] i
        mov r14, qword [rbx + 37]
;       [158:30] bounds check
;       [158:30] allocate scratch register -> r13
;       [158:30] line number
        mov r13, 158
        test r14, r14
        cmovs rbp, r13
        js baz_bounds_panic
        cmp r14, 20
        cmovge rbp, r13
        jge baz_bounds_panic
;       [158:30] free scratch register r13
;       [158:26] allocate scratch register -> r13
        mov r13b, byte [rbx + r14 + 8]
        mov byte [rbx + r15 + 8], r13b
;       [158:26] free scratch register r13
;       [158:26] free scratch register r14
;       [158:9] free scratch register r15
;       [159:9] write_pos = write_pos + 1
;       [159:21] instructions without scratch register 1, with 3
;       [159:21] write_pos
;       [159:33] write_pos + 1
        add qword [rbx + 45], 1
;       [160:9] i = i + 1
;       [160:13] instructions without scratch register 1, with 3
;       [160:13] i
;       [160:17] i + 1
        add qword [rbx + 37], 1
        if_161_12:
;       [161:12] ? i == 20
;       [161:12] ? i == 20
        cmp_161_12:
        cmp qword [rbx + 37], 20
        jne if_161_9_end
        if_161_12_code:
;           [161:20] break
            jmp loop_157_5_end
        if_161_9_end:
    jmp loop_157_5
    loop_157_5_end:
;   [164:5] sys_print(write_pos, address_of(buf))
;   [164:15] allocate named register rdx
    mov rdx, qword [rbx + 45]
;   [164:26] allocate named register rsi
;   [164:26] rsi = address_of(buf)
;   [164:26] = expression
;   [164:26] address_of(buf)
    lea rsi, [rbx + 8]
;   [44:6] sys_print(len : reg_rdx, ptr : reg_rsi)
    sys_print_164_5:
;       [164:5] alias len -> rdx
;       [164:5] alias ptr -> rsi
;       [45:5] mov(rax, 1)
;           [45:14] 1
            mov rax, 1
;       [45:19] # write system call
;       [46:5] mov(rdi, 0)
;           [46:14] 0
            mov rdi, 0
;       [46:19] # file descriptor for standard out
;       [47:5] mov(rsi, ptr)
;           [47:14] ptr
;       [47:19] # buffer address
;       [48:5] mov(rdx, len)
;           [48:14] len
;       [48:19] # buffer size
;       [49:5] syscall()
        syscall
;       [164:5] free named register rsi
;       [164:5] free named register rdx
    sys_print_164_5_end:
    ret
print_num.size equ 53
; 
baz_bounds_panic:
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
;[20:7] hello
;[20:22] i8[21]
db `hello world from baz\n`
;[21:5] prompt1
;[21:22] i8[12]
db `enter name:\n`
;[22:5] prompt2
;[22:22] i8[20]
db `that is not a name.\n`
;[23:5] prompt3
;[23:22] i8[6]
db `hello `
;[24:9] dot
;[24:22] i8[1]
db `.`
;[25:10] nl
;[25:22] i8[1]
db `\n`
;[26:7] colon
;[26:22] i8[2]
db `: `
;[27:8] nums
;[27:15] i64[4]
;[27:26] [0]
;[27:26] i64
dq 1
;[27:15] pad 3 'i64' of size 8
times 24 db 0
;[28:10] s1
;[28:23] i8
db 3
;[28:21] zero remaining fields
times 127 db 0
dat.end:

section .bss.vars nobits alloc write
align 16
vars:
vars resb 131072
vars.end:
; free named register rbp

; max scratch registers in use: 5
;            max frames in use: 10
;              dat var padding: 1 B
;                max vars size: 944 B
;          optimization pass 1: 124
;          optimization pass 2: 0
```
